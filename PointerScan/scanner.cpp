#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../gui/Gui.h"
 
#include "PointerScanner.hpp"
#include "../socket/client_singleton.h"
#include "../gui/MemoryTypes.h"
#include "types.h"

#define PAGE_SIZE 4096

namespace memchainer {

std::vector<MemoryRegion*> memoryRegionList;//待扫描数据段
std::vector<MemoryRegion*> staticRegionList;//静态区域段
std::unordered_map<std::string, int> RegionMap;

//静态保存一份所有的PointerDir数据 索引获取 避免大批量的重复数据
//地址唯一  值不唯一  但off也不固定
//std::unordered_map<, class Ty>
std::vector<PointerDir> g_PointerDir;

PointerScanner::PointerScanner() {
    // 创建文件缓存
    //fileCache_ = std::make_shared<FileCache>();
    //fileCache_->initialize();
}

PointerScanner::~PointerScanner() {
    // 清理指针缓存
    for (auto* ptr : pointerCache_) {
        delete ptr;
    }
    pointerCache_.clear();
    
}

// 清除已有指针数据
void PointerScanner::clearPointers() {
    // 清理旧指针
    for (auto* ptr : pointerCache_) {
        delete ptr;
    }
    pointerCache_.clear();
    
    // 清理内存区域列表
    for (auto* region : memoryRegionList) {
        delete region;
    }
    memoryRegionList.clear();
    
    for (auto* region : staticRegionList) {
        delete region;
    }
    staticRegionList.clear();
    RegionMap.clear();
}

// 二分查找辅助函数实现
std::pair<std::vector<PointerAllData*>::iterator, std::vector<PointerAllData*>::iterator>
PointerScanner::binarySearchPointers(Address startAddr, Address endAddr) {
    // 手动实现二分查找：查找第一个 value >= startAddr 的位置
    std::vector<PointerAllData*>::iterator startIt = pointerCache_.end();
    {
        int left = 0;
        int right = static_cast<int>(pointerCache_.size()) - 1;
        int result = -1;
        
        while (left <= right) {
            int mid = left + (right - left) / 2;
            
            if (pointerCache_[mid]->value >= startAddr) {
                result = mid;  // 记录可能的结果
                right = mid - 1;  // 继续向左查找更小的满足条件的位置
            } else {
                left = mid + 1;
            }
        }
        
        if (result != -1) {
            startIt = pointerCache_.begin() + result;
        }
    }
    
    // 手动实现二分查找：查找第一个 value > endAddr 的位置
    std::vector<PointerAllData*>::iterator endIt = pointerCache_.end();
    {
        int left = 0;
        int right = static_cast<int>(pointerCache_.size()) - 1;
        int result = static_cast<int>(pointerCache_.size());  // 默认为 end()
        
        while (left <= right) {
            int mid = left + (right - left) / 2;
            
            if (pointerCache_[mid]->value > endAddr) {
                result = mid;  // 记录可能的结果
                right = mid - 1;  // 继续向左查找更小的满足条件的位置
            } else {
                left = mid + 1;
            }
        }
        
        endIt = pointerCache_.begin() + result;
    }
    
    return std::make_pair(startIt, endIt);
}

uint32_t PointerScanner::findPointers(bool clearExisting,
                                      const FindProgressCallback &progressCb) {

  // 如果需要清除已有数据
  if (clearExisting) {
    clearPointers();
  } else {
    // 如果不清除且已有数据，直接返回已有指针数量
    if (!pointerCache_.empty()) {
      Gui::log("使用已有的 %d 个指针", pointerCache_.size());
      return static_cast<uint32_t>(pointerCache_.size());
    }
  }

  // 获取过滤的内存区域
  std::vector<ModuleInfoItem> regions;
  FetchModuleList(regions);
  // 过滤出静态区域(cd cb xa) 和 正常待扫描的区域(a ca  cb cd  xa  )

  for (auto &region : regions) {

    // 过滤不可读区域
    if ((region.flag & 0x1) == 0) {
      continue;
    }

    if (region.type == MemoryType::C_Data ||
        region.type == MemoryType::Code_App ||
        region.type == MemoryType::C_Bss) {

      // pathname只保留最后的名称
      std::string pathname = region.name;
      size_t lastSlash = pathname.find_last_of('/');
      if (lastSlash != std::string::npos) {
        pathname = pathname.substr(lastSlash + 1);
      }
      if (RegionMap.find(pathname) == RegionMap.end()) {
        RegionMap[pathname] = 0;
      } else {
        RegionMap[pathname]++;
      }

      std::string name =
          pathname + "[" + std::to_string(RegionMap[pathname]) + "]";

      if (region.type == MemoryType::C_Bss) {
        // 判断bss区的前一个是不是C_Data
        auto preRegion = std::prev(&region);
        if (preRegion->type != MemoryType::C_Data) {
          continue;
        }
        size_t lastSlash = preRegion->name.find_last_of('/');
        if (lastSlash != std::string::npos) {
          pathname = preRegion->name.substr(lastSlash + 1);
        }

        name = pathname + std::string(".bss");
      }

      MemoryRegion *memRegion = new MemoryRegion(
          region.base,  region.size, region.type, name.c_str());
      staticRegionList.push_back(memRegion);
      memoryRegionList.push_back(memRegion);
    } else if (region.type == MemoryType::Anonymous ||
               region.type == MemoryType::C_Alloc) {

      MemoryRegion *memRegion = new MemoryRegion(
          region.base,  region.size, region.type, nullptr);

      memoryRegionList.push_back(memRegion);
    }
  }

  regions.clear();

  // 扫描所有过滤的区域，并报告进度
  uint32_t totalRegions = static_cast<uint32_t>(memoryRegionList.size());
  uint32_t currentRegion = 0;

  for (const auto *region : memoryRegionList) {
    // TODO 线程池扫描
    scanRegionForPointers(region->startAddress, region->Size);

    // 更新进度
    currentRegion++;
    if (progressCb) {
      float progress = static_cast<float>(currentRegion) / totalRegions;
      progressCb(currentRegion, totalRegions, progress);
    }
  }

  // 排序指针，便于后续二分查找
  Gui::log("正在排序指针数据...");
  std::sort(pointerCache_.begin(), pointerCache_.end(),
            [](PointerAllData *a, PointerAllData *b) {
              // 按指针值排序
              return a->value < b->value;
            });
  // 释放多余内存
  pointerCache_.shrink_to_fit();

  Gui::log("扫描潜在指针完成 指针数量: %d 内存使用: %d MB",
           pointerCache_.size(),
           pointerCache_.size() * sizeof(PointerAllData) / 1024 / 1024);
  return static_cast<uint32_t>(pointerCache_.size());
}



// 使用二分查找在排序的 pointerCache_ 中查找指向指定地址范围的所有指针
// 返回所有 value 在 [startAddr, endAddr] 范围内的指针
std::vector<PointerAllData*> PointerScanner::findPointersInRange(Address startAddr, Address endAddr) {
  std::vector<PointerAllData*> result;
  
  // 二分查找：找到第一个 value >= startAddr 的位置
  auto startIt = std::lower_bound(pointerCache_.begin(), pointerCache_.end(), startAddr,
                                  [](PointerAllData* p, Address addr) {
                                      return p->value < addr;
                                  });
  
  // 二分查找：找到第一个 value > endAddr 的位置
  auto endIt = std::upper_bound(pointerCache_.begin(), pointerCache_.end(), endAddr,
                                [](Address addr, PointerAllData* p) {
                                    return addr < p->value;
                                });
  
  // 计算范围大小并预分配空间
  size_t count = std::distance(startIt, endIt);
  if (count == 0) {
    return result; // 空结果
  }
  
  result.reserve(count);
  
  // 收集范围内的所有指针
  if (startIt != pointerCache_.end() && startIt <= endIt && (*startIt)->value <= endAddr) {
      for (auto it = startIt; it != pointerCache_.end() && it <= endIt && (*it)->value <= endAddr; ++it) {
          result.push_back(*it);
      }
  }
  
  return result;
}





void PointerScanner::scanRegionForPointers(Address startAddress,
                                           uint32_t size) {

  std::vector<std::pair<uint64_t, std::vector<uint8_t>> /*页地址，页数据*/>
      buffer;

  if (ReadBratchMemory(startAddress, size, buffer)) {
    for (auto &item : buffer) {

      for (size_t i = 0; i < item.second.size(); i += sizeof(Address)) {
        if (i + sizeof(Address) > item.second.size()) {
          break;
        }
        Address value = *reinterpret_cast<Address *>(item.second.data() + i);

        // Gui::log("扫描内存找到指针: addr:%llx value:%llx", item.first + i,
        // value);
        if (!isValidAddress(value)) {
          continue;
        }
        auto *pointerallData = new PointerAllData(item.first + i, value, 
                                                  calculateStaticOffset(value));
        pointerCache_.push_back(pointerallData);
      }
    }
  }
}

void PointerScanner::Search1Pointers(
    PointerRange &dirs, std::vector<uint64_t> pointers,
    const ScanOptions &options) {
  auto BaseAddr = pointers[0];
  uint64_t startAddr = BaseAddr - options.maxOffset;
  uint64_t endAddr = BaseAddr;
  
  Gui::log("第0层查找范围: [%llx - %llx], 偏移范围: %lld", 
           startAddr, endAddr, options.maxOffset);

  // 使用二分查找获取所有指向目标地址范围的指针
  auto pointerAllDatas = findPointersInRange(startAddr, endAddr);
  if (pointerAllDatas.empty()) {
    Gui::log("第0层未找到任何指针，程序终止");
    return;
  }

  // 构建第0层的指针范围结果
  PointerRange ranges;
  ranges.level = 0;
  ranges.address = BaseAddr;
  ranges.results.reserve(pointerAllDatas.size());
  
  for (auto &pointerAllData : pointerAllDatas) {
    Offset offset = static_cast<Offset>(BaseAddr - pointerAllData->value);
    pointerAllData->refCount++;
    pointerAllData->Offsets.insert(offset);
    
    PointerDir dir(pointerAllData, offset);
    dir.child = nullptr; // 第0层的子节点为空（这是目标地址）
    ranges.results.emplace_back(std::move(dir));
  }

  Gui::log("第0层找到 %d 个指针", ranges.results.size());

  dirs = ranges;
}

// 判断地址是否在静态区域内
StaticOffset* PointerScanner::calculateStaticOffset(Address addr) {
  //static StaticOffset nullOffset(0, nullptr);
    // 遍历所有静态区域
    for ( auto* region : staticRegionList) {
        // 检查地址是否落在这个区域内
        if (addr >= region->startAddress && addr < ( region->startAddress + region->Size)) {
            return new StaticOffset(addr - region->startAddress, region);
        }
    }
     
    //二分查找
    // auto it = std::lower_bound(staticRegionList.begin(), staticRegionList.end(), addr,
    //                           [](const MemoryRegion* region, Address addr) {
    //                               return region->startAddress < addr;
    //                           });
    // if(it != staticRegionList.end())
    // {
    //     return StaticOffset(addr - (*it)->startAddress, *it);
    // }
    return nullptr;
}

// ============================================================================
// 扫描指针链（深度优先搜索算法）
// ============================================================================
// 
// 算法说明：
// 1. 使用深度优先搜索（DFS）从目标地址向上追溯，查找所有到达静态地址的指针链
// 2. 使用二分查找（O(log n)）在已排序的 pointerCache_ 中快速定位父指针
// 3. 使用栈内存存储临时节点，避免堆内存分配开销
// 4. 使用路径数组进行循环检测，防止无限递归
// 
// 搜索流程：
// 第0层：目标地址 (0x12345678)
//   ↑ offset=0x10
// 第1层：指针A (0x12345668) -> 指向目标地址
//   ↑ offset=0x20  
// 第2层：指针B (0x12345648) -> 指向指针A
//   ↑ offset=0x08
// 第N层：静态指针 (Module.so+0x1000) -> 指向指针B  [找到完整链！]
//
// 性能优化：
// - 二分查找：O(log n) 替代线性搜索 O(n)
// - 栈内存：避免频繁的 new/delete
// - 循环检测：使用定长数组替代 unordered_set
// - 提前终止：达到结果限制后立即停止
// ============================================================================
int PointerScanner::scanPointerChain(Address &targetAddress,
                                     const ScanOptions &options,
                                     const ScanProgressCallback &progressCb) {

  Gui::log("========== 指针链扫描参数 ==========");
  Gui::log("目标地址: 0x%llx", targetAddress);
  Gui::log("最大深度: %d", options.maxDepth);
  Gui::log("最大偏移: %d (0x%x)", options.maxOffset, options.maxOffset);
  Gui::log("线程数量: %d", options.threadCount);
  Gui::log("结果限制: %s (%d)", 
           options.limitResults ? "启用" : "禁用", 
           options.resultLimit);
  Gui::log("====================================");

  // 根据线程数选择单线程或多线程模式
  if (options.threadCount > 1) {
    Gui::log("使用多线程模式进行扫描...");
    return scanPointerChainMultiThread(targetAddress, options, progressCb);
  }
  
  Gui::log("使用单线程模式进行扫描...");
  auto startTime = std::chrono::high_resolution_clock::now();

  // 第0级：目标地址
  std::vector<uint64_t> pointers;
  pointers.push_back(targetAddress);

  Gui::log("========== 开始扫描第0层指针链 =========="); 
  // 处理第0层（目标地址）- 找到所有指向目标地址的指针
  PointerRange level0Results{};
  Search1Pointers(level0Results, pointers, options);
  
  if (level0Results.results.empty()) {
    Gui::log("第0层未找到任何指针，无法构建指针链");
    return 0;
  }

  Gui::log("第0层找到 %d 个初始分支", level0Results.results.size());
  Gui::log("开始深度优先递归扫描（最大深度: %d）...", options.maxDepth);

  // 统计信息
  size_t totalChainsFound = 0;
  size_t totalNodesProcessed = 0;
  size_t totalFirstLevelBranches = level0Results.results.size();
  size_t currentBranchIndex = 0;

  // 临时存储找到的所有静态指针链，最后统一保存到 chains_
  std::vector<std::list<PointerChainNode>> tempChains;
  tempChains.reserve(1000); // 预分配空间，减少重新分配

  // 统计循环检测信息
  size_t cycleDetectionCount = 0;

  // 深度优先搜索递归函数
  // 参数说明：
  // - currentNode: 当前处理的节点
  // - currentDepth: 当前深度（从1开始）
  // - nodesProcessed: 已处理的节点数（引用传递，用于统计）
  // - pathArray: 路径数组，用于循环检测（栈分配）
  // - pathLength: 当前路径长度
  std::function<void(PointerDir *, int, size_t &, Address*, int)> dfsSearch =
      [&](PointerDir *currentNode, int currentDepth, size_t &nodesProcessed, 
          Address* pathArray, int pathLength) {
        
        // 【步骤1：循环检测】
        // 使用线性搜索检测循环（对于深度<32的小数组，比哈希表更快）
        Address nodeAddr = currentNode->Data->address;
        
        for (int i = 0; i < pathLength; ++i) {
          if (pathArray[i] == nodeAddr) {
            cycleDetectionCount++;
            return; // 检测到循环引用，跳过此路径
          }
        }
        
        // 将当前节点地址加入路径数组
        pathArray[pathLength] = nodeAddr;
        int newPathLength = pathLength + 1;
        
        // 【步骤2：检查终止条件】
        // 检查是否达到结果数量限制
        if (options.limitResults && totalChainsFound >= options.resultLimit) {
          return;
        }

        // 检查是否达到最大深度
        if (currentDepth >= options.maxDepth) {
          return;
        }

        // 【步骤3：静态指针检测与指针链构建】
        // 如果当前节点是静态指针，说明找到了一条完整的指针链
        // 必须在递归栈有效时立即构建，因为中间节点使用的是栈内存
        if (currentNode->Data->staticOffset_ != nullptr && 
            currentNode->Data->staticOffset_->staticOffset > 0) {
          // 构建完整指针链（从静态地址 -> 中间节点 -> 目标地址）
          std::list<PointerChainNode> chain;
          
          // 从当前静态节点开始，沿着 child 指针向下遍历到目标地址
          PointerDir* node = currentNode;
          while (node != nullptr) {
            PointerChainNode chainNode(
              node->Data->address, 
              node->Data->value, 
              node->offset,
              node->Data->staticOffset_
            );
            chain.push_back(chainNode);
            node = node->child;
          }
          
          // 保存找到的指针链
          tempChains.push_back(std::move(chain));
          totalChainsFound++;
          
          // 定期报告进度（每找到100条链）
          if (totalChainsFound % 100 == 0 && progressCb) {
            ScanProgressInfo info;
            info.currentBranch = static_cast<uint32_t>(currentBranchIndex);
            info.totalBranches = static_cast<uint32_t>(totalFirstLevelBranches);
            info.chainsFound = static_cast<uint32_t>(totalChainsFound);
            info.nodesProcessed = static_cast<uint32_t>(totalNodesProcessed);
            info.currentDepth = static_cast<uint32_t>(currentDepth);
            info.maxDepth = options.maxDepth;
            info.progress = static_cast<float>(currentBranchIndex) / totalFirstLevelBranches;
            progressCb(info);
          }

          return; // 找到静态指针，终止这条路径的搜索（不再向上追溯）
        }

        // 【步骤4：向上搜索父指针】
        // 获取当前节点的地址，查找所有指向它的指针
        Address baseAddr = currentNode->Data->address;
        Address startAddr = baseAddr - options.maxOffset;
        Address endAddr = baseAddr;
        size_t branchCount = 0;

        // 使用二分查找获取所有可能的父指针
        auto pointerAllDatas = findPointersInRange(startAddr, endAddr);
        if (pointerAllDatas.empty()) {
          return; // 没有找到父指针，这条路径终止
        }
        branchCount = pointerAllDatas.size();
        
        // 遍历所有父指针，递归搜索
        for (auto &pointerAllData : pointerAllDatas) {
          // 提前终止优化：检查是否达到结果限制
          if (options.limitResults && totalChainsFound >= options.resultLimit) {
            break;
          }
          
          // 计算偏移量并更新统计信息
          Offset offset = static_cast<Offset>(baseAddr - pointerAllData->value);
          pointerAllData->refCount++;
          pointerAllData->Offsets.insert(offset);
          
          // 创建临时节点（栈内存），用于递归调用
          // 注意：tempNode 是局部变量，只在递归调用期间有效
          PointerDir tempNode(pointerAllData, offset);
          tempNode.child = currentNode; // 建立父子关系
          
          // 递归搜索下一层（向上追溯）
          dfsSearch(&tempNode, currentDepth + 1, nodesProcessed, pathArray, newPathLength);
        }
    
        nodesProcessed += branchCount;

        // 【优化2：降低进度回调频率】
        // 定期报告进度（降低频率到20000）
        if (progressCb && nodesProcessed % 20000 == 0) {
          ScanProgressInfo info;
          info.currentBranch = static_cast<uint32_t>(currentBranchIndex);
          info.totalBranches = static_cast<uint32_t>(totalFirstLevelBranches);
          info.chainsFound = static_cast<uint32_t>(totalChainsFound);
          info.nodesProcessed = static_cast<uint32_t>(totalNodesProcessed);
          info.currentDepth = static_cast<uint32_t>(currentDepth);
          info.maxDepth = options.maxDepth;
          info.progress = static_cast<float>(currentBranchIndex) / totalFirstLevelBranches;
          progressCb(info);
        }
        
        
      };

  // 从第0层的每个指针开始深度优先搜索
  for (auto &firstLevelPointer : level0Results.results) {
    size_t nodesInThisBranch = 0;
    
    // 为每个第0层分支创建栈数组（用于循环检测）
    // 使用固定大小数组，避免动态内存分配，线性搜索比unordered_set更快
    Address pathArray[32]; // 最大深度32层，足够大且栈分配非常快
    
    // 调用dfsSearch，传递路径数组和初始长度0
    dfsSearch(&firstLevelPointer, 1, nodesInThisBranch, pathArray, 0);
    totalNodesProcessed += nodesInThisBranch;
    currentBranchIndex++;

    // 【优化2：降低进度回调频率】
    // 报告每个分支完成后的进度（降低频率到每20个分支）
    if (progressCb && currentBranchIndex % 20 == 0) {
      ScanProgressInfo info;
      info.currentBranch = static_cast<uint32_t>(currentBranchIndex);
      info.totalBranches = static_cast<uint32_t>(totalFirstLevelBranches);
      info.chainsFound = static_cast<uint32_t>(totalChainsFound);
      info.nodesProcessed = static_cast<uint32_t>(totalNodesProcessed);
      info.currentDepth = 0; // 完成一个分支后回到起点
      info.maxDepth = options.maxDepth;
      info.progress = static_cast<float>(currentBranchIndex) / totalFirstLevelBranches;
      progressCb(info);
    }

    // 【优化3：提前终止】
    // 检查是否达到结果限制
    if (options.limitResults && totalChainsFound >= options.resultLimit) {
      Gui::log("已达到结果限制 %d，停止扫描", options.resultLimit);
      break;
    }
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime)
          .count();
  
  Gui::log("========== 指针链扫描完成 ==========");
  Gui::log("扫描统计:");
  Gui::log("  - 找到有效指针链: %zu 条", totalChainsFound);
  Gui::log("  - 处理节点总数: %zu", totalNodesProcessed);
  Gui::log("  - 初始分支数: %zu", totalFirstLevelBranches);
  Gui::log("  - 循环检测拦截: %zu 次", cycleDetectionCount);
  
  Gui::log("性能指标:");
  Gui::log("  - 扫描耗时: %lld ms (%.2f 秒)", duration, duration / 1000.0);
  if (totalNodesProcessed > 0 && duration > 0) {
    Gui::log("  - 平均处理速度: %.2f 节点/秒", 
             totalNodesProcessed * 1000.0 / duration);
    Gui::log("  - 平均每条链耗时: %.2f ms", 
             totalChainsFound > 0 ? (double)duration / totalChainsFound : 0);
  }
  
  if (totalChainsFound > 0) {
    Gui::log("内存使用:");
    size_t chainMemory = totalChainsFound * sizeof(std::list<PointerChainNode>);
    Gui::log("  - 指针链数据: ~%zu KB", chainMemory / 1024);
  }
  
  // 统一保存所有找到的指针链到 chains_
  if (!tempChains.empty()) {
    Gui::log("开始保存 %zu 条指针链到结果集...", tempChains.size());
    
    // 预分配空间，避免多次扩容
    chains_.reserve(chains_.size() + tempChains.size());
    
    // 移动所有链到最终结果
    for (auto &chain : tempChains) {
      chains_.push_back(std::move(chain));
    }
    
    Gui::log("指针链保存完成，当前结果集共 %zu 条", chains_.size());
  } else {
    Gui::log("未找到任何有效指针链");
  }
  
  Gui::log("====================================");

  pointers.clear();

  return static_cast<int>(totalChainsFound);
}

// 检查地址是否合法的辅助函数
bool PointerScanner::isValidAddress(Address& addr) {
    if ((addr & 0xffff000000000000) == 0xb400000000000000) {
        addr &= 0xffffffffffff;
    }
   // addr = addr & 0xFFFFFFFFFFFF;
    // 过滤掉一些显然无效的地址范围
    if (addr < 0x4500000000) return false; // 过滤NULL和极小值
    if (addr > 0x7FFFFFFFFF) return false; // 过滤超大值

    // 检查地址是否对齐（通常指针是4或8字节对齐的）
    if (addr % 4 != 0) return false;
    
    return true;
}

// ============================================================================
// 多线程扫描实现
// ============================================================================

// 多线程扫描指针链（主入口）
int PointerScanner::scanPointerChainMultiThread(
    Address &targetAddress,
    const ScanOptions &options,
    const ScanProgressCallback &progressCb) {
  
  auto startTime = std::chrono::high_resolution_clock::now();

  // ========== 第1步：获取第0层分支 ==========
  std::vector<uint64_t> pointers;
  pointers.push_back(targetAddress);

  Gui::log("========== 开始扫描第0层指针链 =========="); 
  PointerRange level0Results{};
  Search1Pointers(level0Results, pointers, options);
  
  if (level0Results.results.empty()) {
    Gui::log("第0层未找到任何指针，无法构建指针链");
    return 0;
  }

  size_t totalBranches = level0Results.results.size();
  Gui::log("第0层找到 %zu 个初始分支", totalBranches);
  Gui::log("使用 %d 个线程进行并行扫描...", options.threadCount);

  // ========== 第2步：分配任务到线程 ==========
  uint32_t actualThreadCount = (options.threadCount < static_cast<uint32_t>(totalBranches)) 
                                  ? options.threadCount 
                                  : static_cast<uint32_t>(totalBranches);
  if (actualThreadCount < options.threadCount) {
    Gui::log("警告：分支数(%zu) < 线程数(%d)，实际使用 %d 个线程", 
             totalBranches, options.threadCount, actualThreadCount);
  }

  size_t branchesPerThread = (totalBranches + actualThreadCount - 1) / actualThreadCount;
  
  // 创建线程本地数据
  std::vector<ThreadLocalData> threadLocalData(actualThreadCount);
  std::vector<std::thread> threads;
  
  // 全局原子计数器（用于提前终止）
  std::atomic<size_t> globalChainsFound{0};
  
  // 创建多线程进度跟踪器
  MultiThreadProgress progress;
  progress.totalBranches = totalBranches;
  progress.maxDepth = options.maxDepth;

  // ========== 第3步：启动工作线程 ==========
  Gui::log("开始启动 %d 个工作线程...", actualThreadCount);
  
  for (uint32_t i = 0; i < actualThreadCount; ++i) {
    size_t startIdx = i * branchesPerThread;
    size_t endIdx = (startIdx + branchesPerThread < totalBranches) 
                      ? startIdx + branchesPerThread 
                      : totalBranches;
    
    if (startIdx < totalBranches) {
      threads.emplace_back([this, &level0Results, startIdx, endIdx, &options, 
                           &threadLocalData, i, &globalChainsFound, &progress]() {
        workerThreadDFS(
          level0Results.results,
          startIdx, endIdx,
          options,
          threadLocalData[i],
          globalChainsFound,
          &progress
        );
      });
      
      Gui::log("  线程 %d: 处理分支 [%zu - %zu)", i, startIdx, endIdx);
    }
  }

  // ========== 第4步：启动进度监控线程 ==========
  std::thread progressThread;
  if (progressCb) {
    Gui::log("启动进度监控线程...");
    progressThread = std::thread([&progress, &progressCb, totalBranches, &options]() {
      const int reportIntervalMs = 200; // 每200ms报告一次进度
      
      while (!progress.shouldStop.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(reportIntervalMs));
        
        // 读取当前进度（原子操作，无锁）
        ScanProgressInfo info;
        info.currentBranch = static_cast<uint32_t>(progress.completedBranches.load(std::memory_order_relaxed));
        info.totalBranches = static_cast<uint32_t>(totalBranches);
        info.chainsFound = static_cast<uint32_t>(progress.totalChainsFound.load(std::memory_order_relaxed));
        info.nodesProcessed = static_cast<uint32_t>(progress.totalNodesProcessed.load(std::memory_order_relaxed));
        info.currentDepth = progress.maxDepthReached.load(std::memory_order_relaxed);
        info.maxDepth = options.maxDepth;
        info.progress = (totalBranches > 0) ? static_cast<float>(info.currentBranch) / totalBranches : 0.0f;
        
        // 调用进度回调
        progressCb(info);
        
        // 如果所有分支已完成，退出监控循环
        if (info.currentBranch >= info.totalBranches) {
          break;
        }
      }
    });
  }
  
  // ========== 第5步：等待所有工作线程完成 ==========
  Gui::log("等待所有工作线程完成...");
  
  for (auto& t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
  
  Gui::log("所有工作线程已完成");
  
  // 停止进度监控线程
  if (progressCb) {
    progress.shouldStop.store(true, std::memory_order_relaxed);
    if (progressThread.joinable()) {
      progressThread.join();
    }
    Gui::log("进度监控线程已停止");
    
    // 发送最终进度报告
    ScanProgressInfo finalInfo;
    finalInfo.currentBranch = static_cast<uint32_t>(progress.completedBranches.load(std::memory_order_relaxed));
    finalInfo.totalBranches = static_cast<uint32_t>(totalBranches);
    finalInfo.chainsFound = static_cast<uint32_t>(progress.totalChainsFound.load(std::memory_order_relaxed));
    finalInfo.nodesProcessed = static_cast<uint32_t>(progress.totalNodesProcessed.load(std::memory_order_relaxed));
    finalInfo.currentDepth = progress.maxDepthReached.load(std::memory_order_relaxed);
    finalInfo.maxDepth = options.maxDepth;
    finalInfo.progress = 1.0f;
    progressCb(finalInfo);
  }

  // ========== 第6步：合并结果 ==========
  Gui::log("开始合并线程结果...");
  mergeThreadLocalData(threadLocalData);

  // ========== 第7步：统计和报告 ==========
  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime).count();
  
  size_t totalChainsFound = 0;
  size_t totalNodesProcessed = 0;
  size_t totalCycleDetected = 0;
  
  for (const auto& localData : threadLocalData) {
    totalChainsFound += localData.chainsFound;
    totalNodesProcessed += localData.nodesProcessed;
    totalCycleDetected += localData.cycleDetected;
  }
  
  Gui::log("========== 多线程指针链扫描完成 ==========");
  Gui::log("扫描统计:");
  Gui::log("  - 找到有效指针链: %zu 条", totalChainsFound);
  Gui::log("  - 处理节点总数: %zu", totalNodesProcessed);
  Gui::log("  - 初始分支数: %zu", totalBranches);
  Gui::log("  - 循环检测拦截: %zu 次", totalCycleDetected);
  Gui::log("  - 使用线程数: %d", actualThreadCount);
  
  Gui::log("性能指标:");
  Gui::log("  - 扫描耗时: %lld ms (%.2f 秒)", duration, duration / 1000.0);
  if (totalNodesProcessed > 0 && duration > 0) {
    Gui::log("  - 平均处理速度: %.2f 节点/秒", 
             totalNodesProcessed * 1000.0 / duration);
    Gui::log("  - 平均每条链耗时: %.2f ms", 
             totalChainsFound > 0 ? (double)duration / totalChainsFound : 0);
  }
  
  // 线程效率
  for (uint32_t i = 0; i < actualThreadCount; ++i) {
    Gui::log("  - 线程 %d: 找到 %zu 条链, 处理 %zu 节点", 
             i, threadLocalData[i].chainsFound, threadLocalData[i].nodesProcessed);
  }
  
  Gui::log("===========================================");

  pointers.clear();

  return static_cast<int>(totalChainsFound);
}

// 工作线程的 DFS 函数
void PointerScanner::workerThreadDFS(
    const std::vector<PointerDir>& branches,
    size_t startIdx,
    size_t endIdx,
    const ScanOptions &options,
    ThreadLocalData& localData,
    std::atomic<size_t>& globalChainsFound,
    MultiThreadProgress* progress) {
  
  // DFS 递归函数（Lambda）
  std::function<void(const PointerDir*, int, Address*, int)> dfsSearch = 
      [&](const PointerDir* currentNode, int currentDepth, Address* pathArray, int pathLength) {
        
        // 【步骤1：循环检测】
        Address nodeAddr = currentNode->Data->address;
        
        for (int i = 0; i < pathLength; ++i) {
          if (pathArray[i] == nodeAddr) {
            localData.cycleDetected++;
            return; // 检测到循环引用
          }
        }
        
        pathArray[pathLength] = nodeAddr;
        int newPathLength = pathLength + 1;
        
        // 【步骤2：检查终止条件】
        if (options.limitResults && globalChainsFound >= options.resultLimit) {
          return;
        }

        if (currentDepth >= options.maxDepth) {
          return;
        }

        // 【步骤3：静态指针检测】
        if (currentNode->Data->staticOffset_ != nullptr && 
            currentNode->Data->staticOffset_->staticOffset > 0) {
          // 构建完整指针链
          std::list<PointerChainNode> chain;
          
          const PointerDir* node = currentNode;
          while (node != nullptr) {
            PointerChainNode chainNode(
              node->Data->address, 
              node->Data->value, 
              node->offset,
              node->Data->staticOffset_
            );
            chain.push_back(chainNode);
            node = node->child;
          }
          
          // 保存到线程本地结果
          localData.localChains.push_back(std::move(chain));
          localData.chainsFound++;
          globalChainsFound++;  // 原子操作，用于全局提前终止判断
          
          // 更新全局进度（原子操作，无锁）
          if (progress) {
            progress->totalChainsFound.fetch_add(1, std::memory_order_relaxed);
          }

          return;
        }

        // 【步骤4：向上搜索父指针】
        Address baseAddr = currentNode->Data->address;
        Address startAddr = baseAddr - options.maxOffset;
        Address endAddr = baseAddr;

        auto pointerAllDatas = findPointersInRange(startAddr, endAddr);
        if (pointerAllDatas.empty()) {
          return;
        }
        
        size_t branchCount = pointerAllDatas.size();
        localData.nodesProcessed += branchCount;
        
        // 更新全局节点处理进度
        if (progress) {
          progress->totalNodesProcessed.fetch_add(branchCount, std::memory_order_relaxed);
          // 更新最大深度（使用 CAS 确保正确性）
          uint32_t currentMax = progress->maxDepthReached.load(std::memory_order_relaxed);
          while (currentDepth > currentMax && 
                 !progress->maxDepthReached.compare_exchange_weak(currentMax, currentDepth, 
                                                                   std::memory_order_relaxed)) {
            // CAS 循环直到成功更新或发现已有更大值
          }
        }
        
        for (auto &pointerAllData : pointerAllDatas) {
          // 提前终止
          if (options.limitResults && globalChainsFound >= options.resultLimit) {
            break;
          }
          
          Offset offset = static_cast<Offset>(baseAddr - pointerAllData->value);
          
          // 写入线程本地数据（无竞争）
          localData.localRefCount[pointerAllData]++;
          localData.localOffsets[pointerAllData].insert(offset);
          
          // 创建临时节点
          PointerDir tempNode(pointerAllData, offset);
          tempNode.child = const_cast<PointerDir*>(currentNode);
          
          // 递归搜索
          dfsSearch(&tempNode, currentDepth + 1, pathArray, newPathLength);
        }
      };

  // 处理分配的分支
  for (size_t i = startIdx; i < endIdx; ++i) {
    // 提前终止检查
    if (options.limitResults && globalChainsFound >= options.resultLimit) {
      break;
    }
    
    Address pathArray[32]; // 栈数组用于循环检测
    const PointerDir& branch = branches[i];
    dfsSearch(&branch, 1, pathArray, 0);
    
    // 每完成一个分支，更新进度
    if (progress) {
      progress->completedBranches.fetch_add(1, std::memory_order_relaxed);
    }
  }
}

// 合并所有线程的本地数据到全局结果
void PointerScanner::mergeThreadLocalData(std::vector<ThreadLocalData>& threadLocalData) {
  auto mergeStart = std::chrono::high_resolution_clock::now();
  
  size_t totalChains = 0;
  for (const auto& localData : threadLocalData) {
    totalChains += localData.localChains.size();
  }
  
  // 预分配空间
  chains_.reserve(chains_.size() + totalChains);
  
  Gui::log("合并 %zu 条指针链...", totalChains);
  
  // 合并指针链
  for (auto& localData : threadLocalData) {
    for (auto& chain : localData.localChains) {
      chains_.push_back(std::move(chain));
    }
  }
  
  // 合并统计信息
  Gui::log("合并引用计数和偏移量统计...");
  for (auto& localData : threadLocalData) {
    for (auto& [ptr, count] : localData.localRefCount) {
      ptr->refCount += count;
    }
    for (auto& [ptr, offsets] : localData.localOffsets) {
      ptr->Offsets.insert(offsets.begin(), offsets.end());
    }
  }
  
  auto mergeEnd = std::chrono::high_resolution_clock::now();
  auto mergeDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
      mergeEnd - mergeStart).count();
  
  Gui::log("结果合并完成，耗时: %lld ms", mergeDuration);
  Gui::log("当前总指针链数量: %zu", chains_.size());
}


} // namespace memchainer
