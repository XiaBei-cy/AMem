# 多线程指针扫描设计方案

## 🎯 设计目标
- 充分利用多核 CPU 提升扫描速度
- 避免数据竞争和锁竞争
- 保持代码可维护性

## 📊 性能瓶颈分析

### 当前单线程瓶颈
```cpp
// 1. 写操作冲突点
pointerAllData->refCount++;              // 竞争写
pointerAllData->Offsets.insert(offset);  // 竞争写

// 2. 读操作（无冲突）
auto pointers = findPointersInRange(startAddr, endAddr);  // 只读 pointerCache_
```

### 关键观察
1. **读操作占大头**：95% 时间在二分查找和递归遍历
2. **写操作很少**：每个节点只写一次 refCount 和 Offsets
3. **统计非关键**：refCount 和 Offsets 只用于最终统计，不影响扫描逻辑

## 🚀 多线程方案：分区并行 + 延迟统计

### 方案1：按第0层分支分区（推荐）✅

**核心思路**：
- 将第0层的初始分支（level0Results.results）分成 N 份
- 每个线程处理一份分支，独立进行 DFS
- 每个线程维护独立的统计数据
- 最后合并所有线程的结果

**优势**：
- ✅ 零锁竞争（完全无锁设计）
- ✅ 数据局部性好（每个线程独立工作）
- ✅ 负载均衡（分支数量大致相等）
- ✅ 易于实现和调试

**实现要点**：

```cpp
// 线程本地数据结构
struct ThreadLocalData {
    std::vector<std::list<PointerChainNode>> localChains;  // 本地指针链
    std::unordered_map<PointerAllData*, int> localRefCount;  // 本地引用计数
    std::unordered_map<PointerAllData*, std::unordered_set<Offset>> localOffsets;  // 本地偏移量
    size_t chainsFound = 0;
    size_t nodesProcessed = 0;
    size_t cycleDetected = 0;
};

// 扫描流程
1. 将 level0Results.results 分成 threadCount 份
2. 启动 threadCount 个线程，每个线程：
   - 处理自己的分支范围
   - 只读 pointerCache_（无竞争）
   - 写入 ThreadLocalData（无竞争）
3. 等待所有线程完成
4. 合并所有 ThreadLocalData 到全局结果
```

### 方案2：按深度分层并行（不推荐）❌

**问题**：
- 需要同步点，线程等待开销大
- 深度越深分支越多，负载不均衡
- 实现复杂

### 方案3：工作窃取（Work Stealing）⚠️

**适用场景**：
- 分支数量差异很大
- 需要动态负载均衡

**缺点**：
- 需要线程安全队列
- 实现复杂度高

## 📝 详细实现设计

### 数据结构修改

```cpp
// PointerScanner.hpp

class PointerScanner {
private:
    // 线程本地数据
    struct ThreadLocalData {
        std::vector<std::list<PointerChainNode>> localChains;
        std::unordered_map<PointerAllData*, int> localRefCount;
        std::unordered_map<PointerAllData*, std::unordered_set<Offset>> localOffsets;
        size_t chainsFound = 0;
        size_t nodesProcessed = 0;
        size_t cycleDetected = 0;
    };
    
    // 多线程扫描入口
    int scanPointerChainMultiThread(
        Address &targetAddress,
        const ScanOptions &options,
        const ScanProgressCallback &progressCb);
    
    // 单线程工作函数
    void workerThreadDFS(
        const std::vector<PointerDir>& branches,
        size_t startIdx,
        size_t endIdx,
        const ScanOptions &options,
        ThreadLocalData& localData);
};
```

### 核心算法流程

```cpp
int PointerScanner::scanPointerChainMultiThread(...) {
    // 1. 获取第0层分支
    PointerRange level0Results;
    Search1Pointers(level0Results, pointers, options);
    
    // 2. 分配任务到线程
    size_t totalBranches = level0Results.results.size();
    size_t branchesPerThread = (totalBranches + threadCount - 1) / threadCount;
    
    std::vector<ThreadLocalData> threadLocalData(threadCount);
    std::vector<std::thread> threads;
    
    // 3. 启动工作线程
    for (uint32_t i = 0; i < threadCount; ++i) {
        size_t startIdx = i * branchesPerThread;
        size_t endIdx = std::min(startIdx + branchesPerThread, totalBranches);
        
        if (startIdx < totalBranches) {
            threads.emplace_back([&, i, startIdx, endIdx]() {
                workerThreadDFS(
                    level0Results.results,
                    startIdx, endIdx,
                    options,
                    threadLocalData[i]
                );
            });
        }
    }
    
    // 4. 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 5. 合并结果
    for (auto& localData : threadLocalData) {
        // 合并指针链
        chains_.insert(chains_.end(), 
            localData.localChains.begin(), 
            localData.localChains.end());
        
        // 合并统计信息
        for (auto& [ptr, count] : localData.localRefCount) {
            ptr->refCount += count;
        }
        for (auto& [ptr, offsets] : localData.localOffsets) {
            ptr->Offsets.insert(offsets.begin(), offsets.end());
        }
    }
}
```

### 工作线程实现

```cpp
void PointerScanner::workerThreadDFS(
    const std::vector<PointerDir>& branches,
    size_t startIdx, size_t endIdx,
    const ScanOptions &options,
    ThreadLocalData& localData) {
    
    // DFS 递归函数（Lambda）
    std::function<void(PointerDir*, int, Address*, int)> dfsSearch = 
        [&](PointerDir* currentNode, int depth, Address* path, int pathLen) {
            // ... 与原来的 DFS 逻辑相同
            
            // 但是写操作改为写入本地数据
            localData.localRefCount[pointerAllData]++;
            localData.localOffsets[pointerAllData].insert(offset);
            
            // 找到链时写入本地
            if (currentNode->Data->staticOffset_->staticOffset > 0) {
                localData.localChains.push_back(std::move(chain));
                localData.chainsFound++;
            }
        };
    
    // 处理分配的分支
    for (size_t i = startIdx; i < endIdx; ++i) {
        Address pathArray[32];
        PointerDir& branch = const_cast<PointerDir&>(branches[i]);
        dfsSearch(&branch, 1, pathArray, 0);
    }
}
```

## 📊 性能预期

### 理论加速比
- **单线程**: T
- **4线程**: T / 3.5 （约85%效率）
- **8线程**: T / 6.5 （约80%效率）

### 开销分析
- **线程创建**: ~1ms
- **任务分配**: ~0.1ms
- **结果合并**: ~10-100ms（取决于结果数量）
- **总开销**: < 1% （对于大型扫描任务）

## ⚠️ 注意事项

### 线程安全
1. ✅ `pointerCache_` 只读，线程安全
2. ✅ `ThreadLocalData` 独立，无竞争
3. ⚠️ 进度回调需要加锁或使用原子变量

### 内存使用
- 每个线程额外内存：
  - localChains: ~M/N MB
  - localRefCount: ~K/N MB
  - localOffsets: ~K/N MB
- 总额外内存：~原内存的20-30%

### 调试建议
1. 先实现单线程版本并充分测试
2. 添加 `threadCount=1` 的兼容模式
3. 对比单线程和多线程结果的一致性

## 🔄 渐进式实现

### 阶段1：基础多线程（本周）
- 实现分区并行
- 基本的结果合并
- 简单的进度报告

### 阶段2：优化（下周）
- 动态负载均衡
- 更精确的进度报告
- NUMA 优化（如果需要）

### 阶段3：高级特性（可选）
- 工作窃取
- 自适应线程数
- GPU 加速（CUDA/OpenCL）

