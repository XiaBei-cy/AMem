
项目全部由ai开发，快成石山了，有兴趣优化的欢迎pr
我要重构去写QT版本的了
当前项目仅只有ui，内存库回头发频道里面

[视频效果](https://www.bilibili.com/video/BV1KpWbzeEFU/)

[TG频道](https://t.me/androidmem)


# Android Cheat Engine (ImGui版)

<div align="center">

**基于 ImGui + DirectX12 的跨平台 Android 内存修改工具**

[功能特性](#功能特性) • [快速开始](#快速开始) • [使用指南](#使用指南) 

</div>

---

## 📋 项目简介

这是一个功能强大的 Android 内存修改工具，类似于 PC 端的 Cheat Engine。通过 Socket 连接远程 Android 设备，提供了内存扫描、指针链分析、断点调试等专业功能。

### 主要特点

- 🎨 **现代化 UI** - 基于 Dear ImGui，支持中文界面
- 🚀 **高性能扫描** - 多线程指针链扫描，充分利用多核 CPU
- 🔗 **指针链分析** - 可视化指针链树，支持复杂的指针路径分析
- 🐛 **内核级调试** - 硬件断点、内存断点支持
- 📡 **远程连接** - 通过 Socket 连接 Android 设备
- 💾 **崩溃保护** - 完整的异常捕获和 dump 生成

---

## ✨ 功能特性

### 1. 内存扫描 (ScanWindow)
- ✅ 精确数值扫描（1/2/4/8字节）
- ✅ 模糊扫描（变大/变小/未变化）
- ✅ 增量扫描（首次扫描 → 再次扫描）
- ✅ 扫描范围设置（所有内存/堆/栈/匿名）
- ✅ 实时进度显示
- ✅ 结果过滤和导出

  <img width="964" height="619" alt="搜索" src="https://github.com/user-attachments/assets/e569c833-1444-43f6-9937-87c557466abe" />


### 2. 指针链扫描 (PointerChainWindow)
- ✅ 自动查找指向目标地址的指针链
- ✅ 可配置扫描参数：
  - 最大深度（1-20层）
  - 最大偏移量
  - 结果数量限制
  - 线程数量
- ✅ 多线程并行扫描（2-16线程）
- ✅ 实时进度监控
- ✅ 指针链可视化显示
- ✅ 指针链树形编辑器（开发中）
<img width="1145" height="649" alt="指针扫描" src="https://github.com/user-attachments/assets/06e748d9-24c7-4eee-8b7c-395996552f5f" />


### 3. 内存查看器 (MemoryViewerWindow)
- ✅ 十六进制内存查看
- ✅ 内存编辑功能
- ✅ 支持跳转到指定地址
- ✅ 多种数据类型解析
<img width="1032" height="568" alt="内存查看" src="https://github.com/user-attachments/assets/a4e09eea-8745-4f32-9c93-3ee02e067185" />


### 4. 断点调试 (BreakpointWindow)
- ✅ 硬件断点（读/写/执行）
- ✅ 断点命中信息查看
- ✅ 断点暂停/恢复
- ✅ 反汇编显示（需要 Capstone）
<img width="967" height="603" alt="断点" src="https://github.com/user-attachments/assets/7fd1fe99-966b-4094-9cc5-87a6a5900c23" />
<img width="975" height="645" alt="反汇编" src="https://github.com/user-attachments/assets/5fd54c8b-9087-4246-b34e-e44c772b3aa8" />


### 5. 进程管理
- ✅ 进程列表查看
- ✅ 模块列表查看
- ✅ 进程附加/分离
- ✅ 模块基址查询
<img width="1111" height="705" alt="模块列表" src="https://github.com/user-attachments/assets/16e59225-3453-48a0-873b-4e8c48ccc4a2" />


### 6. 异常处理
- ✅ SEH 异常捕获
- ✅ C++ 标准异常捕获
- ✅ 信号处理
- ✅ 自动生成 .dmp 崩溃转储文件

---

## 🚀 快速开始

### 环境要求

#### 必需环境
- **操作系统**: Windows 10/11 (x64)
- **编译器**: Visual Studio 2022 (需支持 C++17)
- **CMake**: 3.16 或更高版本
- **DirectX**: DirectX 12 SDK

#### 可选依赖
- **Capstone**: 反汇编库（用于断点反汇编功能）
  ```bash
  # 使用 vcpkg 安装
  vcpkg install capstone:x64-windows
  ```

### 编译步骤

#### 方法一：使用批处理脚本（推荐）

```bash
# 双击或运行
build.bat
```

#### 方法二：手动构建

```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 生成 Visual Studio 项目
cmake -G "Visual Studio 17 2022" -A x64 ..

# 3. 编译项目（Release 模式）
cmake --build . --config Release

# 4. 运行程序
.\Release\ImGuiProject.exe
```

#### 方法三：使用 Visual Studio

```bash
# 1. 用 Visual Studio 打开 CMakeLists.txt
# 2. 选择 x64-Release 配置
# 3. 点击"生成" → "生成解决方案"
# 4. 运行项目
```

### 输出文件

编译成功后，可执行文件位于：
```
build/Release/ImGuiProject.exe
```

---

## 📖 使用指南

### 1. 连接 Android 设备

1. 启动程序
2. 点击 **"服务器连接"** 按钮
3. 输入 Android 设备的 IP 地址和端口
4. 点击 **"连接"**

> ⚠️ **注意**: 需要在 Android 设备上运行相应的服务端程序

### 2. 附加进程

1. 连接成功后，点击 **"选择进程"**
2. 在进程列表中搜索或选择目标进程
3. 双击进程名称完成附加

### 3. 内存扫描

#### 精确数值扫描
1. 打开 **"数值扫描"** 窗口
2. 设置扫描类型（1/2/4/8字节）
3. 输入要搜索的数值
4. 点击 **"首次扫描"**
5. 修改游戏内数值后，输入新值
6. 点击 **"再次扫描"**
7. 重复步骤 5-6 直到找到目标地址

#### 模糊扫描
1. 不输入具体数值
2. 选择扫描条件（变大/变小/未变化）
3. 逐步缩小结果范围

### 4. 指针链扫描

1. 打开 **"指针链扫描"** 窗口
2. 点击 **"获取潜在指针"** 按钮（首次使用）
3. 输入目标地址（十六进制）
4. 配置扫描参数：
   - **最大深度**: 推荐 5-7 层
   - **最大偏移量**: 推荐 500-2000
   - **线程数量**: 根据 CPU 核心数设置
5. 点击 **"开始扫描"**
6. 等待扫描完成，查看结果

### 5. 设置断点

1. 打开 **"断点管理"** 窗口
2. 输入断点地址
3. 选择断点类型：
   - **执行断点** (Execute)
   - **写入断点** (Write)
   - **读写断点** (Access)
4. 点击 **"设置断点"**
5. 触发断点后，查看命中信息

---

## 🏗️ 技术架构

### 项目结构

```
imguiApp/
├── main.cpp                    # 程序入口
├── CMakeLists.txt             # CMake 构建配置
├── build.bat                  # 快速编译脚本
├── ExceptionHandler.h         # 异常处理模块
│
├── imgui/                     # ImGui 库
│   ├── imgui.cpp/h           # ImGui 核心
│   └── backends/             # 渲染后端
│       ├── imgui_impl_win32.cpp/h
│       └── imgui_impl_dx12.cpp/h
│
├── gui/                       # GUI 窗口模块
│   ├── Gui.cpp/h             # GUI 框架
│   ├── Window.cpp/h          # 窗口基类
│   ├── CEWindow.cpp/h        # 主窗口
│   ├── ScanWindow.cpp/h      # 内存扫描窗口
│   ├── PointerChainWindow.cpp/h  # 指针链窗口
│   ├── MemoryViewerWindow.cpp/h  # 内存查看器
│   ├── BreakpointWindow.cpp/h    # 断点窗口
│   ├── ModulesWindow.cpp/h       # 模块列表
│   ├── ProcessListWindow.cpp/h   # 进程列表
│   └── DisassemblyHelper.cpp/h   # 反汇编辅助
│
├── PointerScan/               # 指针扫描引擎
│   ├── PointerScanner.hpp    # 扫描器主类
│   ├── scanner.cpp           # 扫描实现
│   ├── types.h               # 数据类型定义
│   ├── formatter.cpp/h       # 结果格式化
│   └── MULTITHREAD_DESIGN.md # 多线程设计文档
│
└── socket/                    # 网络通信模块
    ├── client.hpp            # Socket 客户端
    └── client_singleton.cpp/h # 单例模式封装
```

### 核心技术栈

| 技术 | 版本 | 用途 |
|------|------|------|
| C++ | C++17 | 核心语言 |
| CMake | 3.16+ | 构建系统 |
| Dear ImGui | 最新 | UI 框架 |
| DirectX 12 | - | 图形渲染 |
| Capstone | 5.x | 反汇编引擎 |
| Winsock2 | - | 网络通信 |

### 指针扫描算法

#### 多线程设计

采用 **分区并行 + 延迟统计** 策略：

```
第0层分支 (1000个分支)
    ↓
按线程数分区 (例如4线程)
    ↓
┌─────────┬─────────┬─────────┬─────────┐
│ 线程1   │ 线程2   │ 线程3   │ 线程4   │
│ 0-249   │ 250-499 │ 500-749 │ 750-999 │
└─────────┴─────────┴─────────┴─────────┘
    ↓         ↓         ↓         ↓
  DFS       DFS       DFS       DFS
    ↓         ↓         ↓         ↓
 本地结果  本地结果  本地结果  本地结果
    └─────────┴─────────┴─────────┘
                 ↓
            合并所有结果
```

**优势**：
- ✅ 零锁竞争（完全无锁设计）
- ✅ 数据局部性好
- ✅ 负载均衡
- ✅ 易于实现和调试

详见：[PointerScan/MULTITHREAD_DESIGN.md](PointerScan/MULTITHREAD_DESIGN.md)

---

## 📚 开发文档

### 已有文档

1. **[ExceptionHandler_README.md](ExceptionHandler_README.md)**
   - 异常处理模块使用指南
   - 崩溃捕获和 dump 生成
   - 完整的集成示例

2. **[MULTITHREAD_DESIGN.md](PointerScan/MULTITHREAD_DESIGN.md)**
   - 多线程指针扫描设计方案
   - 性能瓶颈分析
   - 三种并行方案对比

3. **[树形布局说明.txt](树形布局说明.txt)**
   - 指针链树形可视化编辑器
   - 节点布局算法
   - 交互操作说明

4. **[PointerChainZoomAndCollapse说明.md](PointerChainZoomAndCollapse说明.md)**
   - 指针链缩放和折叠功能（待完善）

### API 参考

#### PointerScanner 类

```cpp
namespace memchainer {

// 扫描配置
struct ScanOptions {
    uint32_t maxDepth = 10;        // 最大深度
    int64_t maxOffset = 500;       // 最大偏移量
    bool limitResults = false;     // 是否限制结果
    uint32_t resultLimit = 99999999;
    uint32_t threadCount = 4;      // 线程数量
};

class PointerScanner {
public:
    // 查找所有潜在指针
    uint32_t findPointers(bool clearExisting = true, 
                         const FindProgressCallback& progressCb = nullptr);
    
    // 扫描指针链
    int scanPointerChain(Address& targetAddress,
                        const ScanOptions& options,
                        const ScanProgressCallback& progressCb = nullptr);
    
    // 获取指针数量
    uint32_t getPointerCount() const;
    
    // 获取指针链结果
    const std::vector<std::list<PointerChainNode>>& getChains() const;
};

}
```

#### Socket 通信接口

```cpp
// 服务器连接
bool FetchServerVersion(ServerVersionInfo& outInfo);

// 进程管理
bool FetchProcessList(std::vector<ProcessInfoItem>& outList);
bool OpenProcessHandle(int pid, int& outHandle);

// 内存操作
bool ReadProcessMemoryBytes(uint64_t address, uint32_t size, 
                           std::vector<unsigned char>& out);
bool WriteProcessMemoryBytes(uint64_t address, uint32_t size, 
                            std::vector<unsigned char>& data);

// 内存扫描
int ScanValueWithProgress(uint32_t flags, 
                         std::vector<unsigned char>& Value,
                         ScanProgressCallback callback, 
                         void* userData);

// 断点管理
bool SetKernelBreakpoint(uint64_t address, uint32_t bpType, uint32_t bpSize);
bool RemoveKernelBreakpoint(uint64_t address);
```

---

## ⚙️ 构建配置

### CMake 选项

```cmake
# 选择渲染后端
option(USE_DX11 "Use DirectX 11 backend" OFF)
option(USE_DX12 "Use DirectX 12 backend" ON)

# Capstone 库路径
set(CAPSTONE_ROOT "C:/Program Files/capstone" CACHE PATH "Capstone installation directory")
```

### 编译宏定义

- `USE_DX12` - 使用 DirectX 12 渲染
- `HAVE_CAPSTONE` - 启用反汇编功能
- `IMGUI_DISABLE_DEBUG_TOOLS` - 禁用 ImGui 调试工具
- `DX12_ENABLE_DEBUG_LAYER` - 启用 D3D12 调试层（Debug 模式）

---

## 🔧 故障排除

### 编译问题

**问题**: CMake 找不到 Capstone
```
解决方案：
1. 安装 Capstone: vcpkg install capstone:x64-windows
2. 设置 CAPSTONE_ROOT 环境变量
3. 或者在 CMakeLists.txt 中修改路径
```

**问题**: DirectX 12 SDK 未找到
```
解决方案：
1. 安装 Windows SDK (最新版本)
2. 确保 Visual Studio 已安装 C++ 桌面开发组件
```

### 运行问题

**问题**: 程序启动后中文显示为方框
```
解决方案：
- 确保系统中存在中文字体（微软雅黑、宋体等）
- 检查 main.cpp 中字体加载路径是否正确
```

**问题**: 无法连接 Android 设备
```
解决方案：
1. 确认 Android 设备已运行服务端程序
2. 检查 IP 地址和端口是否正确
3. 检查防火墙设置
4. 确认设备在同一网络
```

**问题**: 指针扫描速度很慢
```
解决方案：
1. 减小最大深度（推荐 5-7）
2. 减小最大偏移量（推荐 500-2000）
3. 增加线程数量（根据 CPU 核心数）
4. 限制结果数量
```

**问题**: 程序崩溃
```
解决方案：
- 检查 CrashDumps 目录下的 .dmp 文件
- 使用 Visual Studio 打开 .dmp 文件分析崩溃原因
- 查看异常处理器输出的描述信息
```


## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！


## 🙏 致谢

- [Dear ImGui](https://github.com/ocornut/imgui) - 优秀的即时模式 GUI 库
- [Capstone](https://www.capstone-engine.org/) - 强大的反汇编引擎
- Cheat Engine - 灵感来源

<div align="center">

**⭐ 如果这个项目对你有帮助，请给一个 Star！**


</div>
