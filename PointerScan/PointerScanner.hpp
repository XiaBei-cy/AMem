#pragma once


#include "types.h"
#include <functional>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <unordered_set>

namespace memchainer {

    // 配置选项
    struct ScanOptions {
        uint32_t maxDepth = 10;      // 最大指针链深度
        int64_t maxOffset = 500;      // 最大允许的偏移量
        bool limitResults = false;   // 是否限制结果数量
        uint32_t resultLimit = 99999999; // 结果数量限制
        uint32_t batchSize = 10000;  // 处理批次大小
        uint32_t threadCount = 4;    // 线程数量
    };

class PointerScanner {
public:


    // 指针查找进度回调函数类型
    using FindProgressCallback = std::function<void(uint32_t currentRegion, uint32_t totalRegions, float progress)>;
    
    // 扫描进度信息结构（深度优先搜索）
    struct ScanProgressInfo {
        uint32_t currentBranch;      // 当前处理的分支索引
        uint32_t totalBranches;      // 总分支数
        uint32_t chainsFound;        // 已找到的指针链数量
        uint32_t nodesProcessed;     // 已处理的节点数量
        uint32_t currentDepth;       // 当前搜索深度
        uint32_t maxDepth;           // 最大深度
        float progress;              // 总体进度 (0.0 - 1.0)
    };
    
    // 扫描进度回调函数类型
    using ScanProgressCallback = std::function<void(const ScanProgressInfo& info)>;

    PointerScanner();
    ~PointerScanner();

    // 查找指针（clearExisting: 是否清除已有指针数据，progressCb: 进度回调）
    uint32_t findPointers(bool clearExisting = true, const FindProgressCallback& progressCb = nullptr);
    
    // 在已排序的 pointerCache_ 中使用二分查找，查找 value 在 [startAddr, endAddr] 范围内的所有指针
    // 返回：包含所有匹配指针的 vector
    std::vector<PointerAllData*> findPointersInRange(Address startAddr, Address endAddr);
    
    // 清除已有指针数据
    void clearPointers();
    
    // 获取已加载的指针数量
    uint32_t getPointerCount() const { return static_cast<uint32_t>(pointerCache_.size()); }
    
    // 获取指针缓存（用于显示潜在指针数据）
    const std::vector<PointerAllData*>& getPointerCache() const { return pointerCache_; }
    
    // 扫描特定区域内的指针
    void scanRegionForPointers(Address startAddress, uint32_t size);
        // 过滤内存区域指针
    void Search1Pointers(
        PointerRange& dirs,
        std::vector<uint64_t> pointers,
        const ScanOptions& options
        );
    // 扫描指针链（深度优先搜索）
    int scanPointerChain(
        Address& targetAddress,
        const ScanOptions& options,
        const ScanProgressCallback& progressCb = nullptr);


    // 判断地址是否在静态区域内
    StaticOffset* calculateStaticOffset(Address addr);

    // 检查地址是否有效
    bool isValidAddress(Address& addr);

        // 获取指针链
    const std::vector<std::list<PointerChainNode>>& getChains() const { return chains_; }


private:
    // ========== 多线程支持 ==========
    
    // 线程本地数据结构：每个线程维护独立的数据，避免锁竞争
    struct ThreadLocalData {
        std::vector<std::list<PointerChainNode>> localChains;  // 本地找到的指针链
        std::unordered_map<PointerAllData*, int> localRefCount;  // 本地引用计数
        std::unordered_map<PointerAllData*, std::unordered_set<Offset>> localOffsets;  // 本地偏移量集合
        
        // 统计信息
        size_t chainsFound = 0;
        size_t nodesProcessed = 0;
        size_t cycleDetected = 0;
        
        ThreadLocalData() = default;
        ThreadLocalData(const ThreadLocalData&) = delete;
        ThreadLocalData& operator=(const ThreadLocalData&) = delete;
    };
    
    // 多线程进度跟踪结构：使用原子变量实现无锁进度监控
    struct MultiThreadProgress {
        std::atomic<size_t> completedBranches{0};  // 已完成的分支数
        std::atomic<size_t> totalChainsFound{0};   // 已找到的总链数
        std::atomic<size_t> totalNodesProcessed{0}; // 已处理的总节点数
        std::atomic<uint32_t> maxDepthReached{0};  // 当前达到的最大深度
        std::atomic<bool> shouldStop{false};       // 停止标志
        
        size_t totalBranches = 0;                   // 总分支数（只读，不需要原子）
        uint32_t maxDepth = 0;                      // 最大深度限制（只读）
    };
    
    // 多线程扫描入口（内部函数）
    int scanPointerChainMultiThread(
        Address &targetAddress,
        const ScanOptions &options,
        const ScanProgressCallback &progressCb);
    
    // 工作线程的 DFS 函数
    void workerThreadDFS(
        const std::vector<PointerDir>& branches,
        size_t startIdx,
        size_t endIdx,
        const ScanOptions &options,
        ThreadLocalData& localData,
        std::atomic<size_t>& globalChainsFound,
        MultiThreadProgress* progress);
    
    // 合并所有线程的本地数据到全局结果
    void mergeThreadLocalData(std::vector<ThreadLocalData>& threadLocalData);
    
    // ========== 辅助函数 ==========
    
    // 二分查找辅助函数：在 pointerCache_ 中查找 [startAddr, endAddr] 范围内的指针
    // 返回 pair<startIt, endIt>，区间 [startIt, endIt) 包含所有满足条件的指针
    std::pair<std::vector<PointerAllData*>::iterator, std::vector<PointerAllData*>::iterator>
    binarySearchPointers(Address startAddr, Address endAddr);

    // ========== 数据成员 ==========
    
    // 指针缓存：按 value 排序，支持二分查找
    std::vector<PointerAllData*> pointerCache_;
    
    // 存储所有指针链
    // 使用双向链表存储，便于查找
    std::vector<std::list<PointerChainNode>> chains_;
};

} // namespace memchainer
