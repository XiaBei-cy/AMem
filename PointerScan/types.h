#pragma once

#include <cstdint>
#include <set>
#include <unordered_set>
#include <vector>
#include <cstring>

namespace memchainer {

// 使用明确大小的类型
using Address = uint64_t;
using Offset = int32_t;




// 内存区域描述
struct MemoryRegion {
    Address startAddress;
    int Size;
    int type;
    char count;          // 用于区分同名区域
    char name[48];

    MemoryRegion(Address start, int size, int t = 0,const char* n = nullptr, int c = 0)
        : startAddress(start), Size(size), type(t), count(c) {
        if (n) {
            int len = strlen(n);
            strncpy(name, n, len);
            name[len+1] = '\0';
        } else {
            name[0] = '\0';
        }
    }
};

// 全局内存区域列表
extern std::vector<MemoryRegion*> memoryRegionList;
extern std::vector<MemoryRegion*> staticRegionList;

// // 指针数据结构
// struct PointerData {
//     Address address;      // 指针地址
//     Address value;        // 指针指向的值
//     Offset offset;        // 相对偏移

//     PointerData(Address addr, Address val, Offset off = 0)
//         : address(addr), value(val), offset(off) {}
// };

struct StaticOffset
{
    uint64_t staticOffset; // 静态偏移量
    // uint16_t regionIndex; // 内存区域索引

    // StaticOffset(uint64_t staticOff, uint16_t regionIndex)
    //     : staticOffset(staticOff), regionIndex(regionIndex) {}
    // StaticOffset()
    //     : staticOffset(0), regionIndex(0) {}



    MemoryRegion* region; // 内存区域
    StaticOffset(uint64_t staticOff, MemoryRegion* reg)
        : staticOffset(staticOff), region(reg) {}
    StaticOffset()
        : staticOffset(0), region(nullptr) {}
};



// 指针数据结构
struct PointerAllData {
    Address address;      // 指针地址
    Address value;        // 指针指向的值

    StaticOffset* staticOffset_; // 静态偏移量
    int32_t refCount;     // 引用计数
    std::unordered_set<int32_t> Offsets;//指针下的偏移量

    PointerAllData(Address addr, Address val, StaticOffset* staticOff = new StaticOffset(0, nullptr))
        : address(addr), value(val), staticOffset_(staticOff) {
            refCount = 0;
        }
};

// 指针方向结构
// struct PointerDir {
//     Address value;        // 指针值
//     Address address;      // 地址
//     Offset offset;        // 偏移量
//     StaticOffset staticOffset_; // 静态偏移量
//     PointerDir* child;    // 指向子节点的指针（下一层） 
//     //因为是从底往上 所以这个其实是父节点  后续遍历只需沿着child节点往下遍历

//     PointerDir(Address val = 0, Address addr = 0, Offset off = 0, StaticOffset staticOff = StaticOffset(0, nullptr))
//         : value(val), address(addr), offset(off), staticOffset_(staticOff), child(nullptr) {};
//     PointerDir(Address val = 0, Address addr = 0,PointerDir* c = nullptr)
//         : value(val), address(addr) {
//             offset = 0;
//             staticOffset_ = StaticOffset(0, nullptr);
//             child = c;
//         };

// };

//再扫描过程中，这个结构体数据过多
struct PointerDir {
    PointerAllData* Data;//or 索引？
    PointerDir* child;    // 指向子节点的指针（下一层） 
    Offset offset;        // 偏移量
    //因为是从底往上 所以这个其实是父节点  后续遍历只需沿着child节点往下遍历
    PointerDir(PointerAllData* D=nullptr,Offset Off=0): Data(D) ,offset(Off)  {}

};


// 指针范围结构
struct PointerRange {
    int level;            // 层级
    Address address;      // 地址
    std::vector<PointerDir> results; // 地址对应的指针列表

    PointerRange(int lvl, Address addr, std::vector<PointerDir>&& res = {})
        : level(lvl), address(addr), results(std::move(res)) {}
    PointerRange() 
        : level(0), address(0), results() {}
};


// 指针链节点结构
struct PointerChainNode {
    Address address;      // 指针地址
    Address value;        // 指针值
    Offset offset;        // 偏移量
    StaticOffset* staticOffset; // 静态偏移量

    PointerChainNode(Address addr = 0, Address val = 0, Offset off = 0, 
                    StaticOffset* staticOff = new StaticOffset(0, nullptr))
        : address(addr), value(val), offset(off), 
          staticOffset(staticOff) {}
};



} // namespace memchainer
