# CMake 版本管理使用指南

## 📋 概述

本项目使用 CMake 的 `configure_file()` 机制自动生成版本信息头文件，实现版本的集中管理和自动化。

---

## 🔧 配置方式

### 1. 在 CMakeLists.txt 中定义版本

```cmake
# 项目版本
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

# 协议版本（与服务端通信使用）
set(PROTOCOL_VERSION_MAJOR 1)
set(PROTOCOL_VERSION_MINOR 4)
set(PROTOCOL_VERSION_PATCH 0)
```

### 2. 自动生成的信息

CMake 会自动生成以下信息：
- ✅ **构建日期和时间**：每次编译时自动更新
- ✅ **Git 提交哈希**：当前代码版本
- ✅ **Git 分支名称**：当前所在分支
- ✅ **编译器信息**：编译器类型和版本

### 3. 生成的头文件位置

```
build/generated/version.h  (由 version.h.in 生成)
```

---

## 💻 代码中使用版本信息

### 基础用法

```cpp
#include <version.h>  // 自动生成的头文件
#include <iostream>

void showVersionInfo() {
    // 方法1: 使用宏定义
    std::cout << "项目版本: " 
              << PROJECT_VERSION_MAJOR << "."
              << PROJECT_VERSION_MINOR << "."
              << PROJECT_VERSION_PATCH << std::endl;
    
    std::cout << "协议版本: " << PROTOCOL_VERSION << std::endl;
    
    // 方法2: 使用内联函数
    std::cout << "完整版本: " << version::getVersionString() << std::endl;
    std::cout << "构建时间: " << version::getBuildDateTime() << std::endl;
}
```

### 在连接窗口中显示版本

```cpp
// 在 ServerConnectWindow.cpp 中
#include <version.h>

void ServerConnectWindow::updateStatus(bool ok, const char* action) {
    if (ok) {
        ServerVersionInfo versionInfo;
        if (FetchServerVersion(versionInfo)) {
            // 显示客户端和服务端版本
            Gui::log("客户端版本: %s", version::getVersionString());
            Gui::log("客户端协议: %s", version::getProtocolVersion());
            Gui::log("服务端版本: %d (%s)", 
                     versionInfo.version, 
                     versionInfo.versionString.c_str());
            
            // 版本兼容性检查（简化版）
            if (versionInfo.version < 400 && PROTOCOL_VERSION_MINOR >= 4) {
                Gui::log("⚠ 警告: 服务端版本较旧，部分功能可能不可用");
            }
        }
    }
}
```

### 版本比较

```cpp
#include <version.h>

void checkMinimumVersion() {
    // 检查是否满足最低版本要求
    if (version::compareVersion(1, 2, 0) >= 0) {
        // 当前版本 >= 1.2.0
        enableAdvancedFeatures();
    } else {
        showUpgradePrompt();
    }
}
```

### 在关于对话框中显示

```cpp
void AboutWindow::onDraw() {
    ImGui::Text("程序名称: ImGuiProject");
    ImGui::Text("版本: %s", PROJECT_VERSION);
    ImGui::Text("协议版本: %s", PROTOCOL_VERSION);
    ImGui::Separator();
    ImGui::Text("构建日期: %s", BUILD_DATE);
    ImGui::Text("构建时间: %s", BUILD_TIME);
    ImGui::Text("Git 提交: %s", GIT_COMMIT_HASH);
    ImGui::Text("Git 分支: %s", GIT_BRANCH);
    ImGui::Separator();
    ImGui::Text("编译器: %s %s", COMPILER_ID, COMPILER_VERSION);
}
```

---

## 🔄 版本更新流程

### 修改版本号

编辑 `CMakeLists.txt`：

```cmake
# 发布新版本：1.0.0 → 1.1.0
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 1)  # 更新次版本号
set(PROJECT_VERSION_PATCH 0)  # 重置补丁号
```

### 更新协议版本

```cmake
# 添加新功能，协议升级：1.4.0 → 1.5.0
set(PROTOCOL_VERSION_MAJOR 1)
set(PROTOCOL_VERSION_MINOR 5)  # 协议增加新功能
set(PROTOCOL_VERSION_PATCH 0)
```

### 重新构建

```bash
# 删除旧的构建目录
rmdir /s /q build

# 重新生成
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## 📊 版本信息示例输出

编译时 CMake 输出：
```
-- Project Version: 1.0.0
-- Protocol Version: 1.4.0
-- Build Date: 2024-10-23 16:55:16
-- Git Commit: a1b2c3d
-- Git Branch: main
```

程序运行时输出：
```
客户端版本: 1.0.0
客户端协议: 1.4.0
服务端版本: 350 (Server v3.5.0)
构建时间: 2024-10-23 16:55:16
```

---

## ⚙️ 高级配置

### 条件编译（基于版本）

```cpp
#include <version.h>

void someFeature() {
    #if PROTOCOL_VERSION_MINOR >= 4
        // 使用 v1.4+ 的新功能
        useBatchMemoryRead();
    #else
        // 降级到旧实现
        useOldMemoryRead();
    #endif
}
```

### 动态检查服务端协议版本

```cpp
#include <version.h>

bool isServerCompatible(int serverProtocolMinor) {
    // 服务端协议版本必须 >= 客户端所需的最低版本
    const int MIN_REQUIRED_PROTOCOL = 2;  // 至少需要 v1.2
    
    if (serverProtocolMinor < MIN_REQUIRED_PROTOCOL) {
        Gui::log("错误: 服务端协议版本过旧");
        Gui::log("需要: v1.%d+", MIN_REQUIRED_PROTOCOL);
        Gui::log("当前: v1.%d", serverProtocolMinor);
        return false;
    }
    
    // 检查是否支持当前客户端的所有功能
    if (serverProtocolMinor < PROTOCOL_VERSION_MINOR) {
        Gui::log("警告: 服务端不支持所有新功能");
        Gui::log("客户端协议: v%s", PROTOCOL_VERSION);
        Gui::log("建议升级服务端");
    }
    
    return true;
}
```

### 自定义版本字符串格式

```cpp
#include <version.h>
#include <sstream>

std::string getFullVersionString() {
    std::ostringstream oss;
    oss << "ImGuiProject v" << PROJECT_VERSION;
    
    #ifdef _DEBUG
        oss << " (Debug)";
    #else
        oss << " (Release)";
    #endif
    
    if (std::string(GIT_COMMIT_HASH) != "unknown") {
        oss << " [" << GIT_COMMIT_HASH << "]";
    }
    
    return oss.str();
}
// 输出示例: "ImGuiProject v1.0.0 (Release) [a1b2c3d]"
```

---

## 🎯 最佳实践

### 1. 版本号规范

遵循语义化版本（Semantic Versioning）：

- **MAJOR**: 不兼容的 API 变更
- **MINOR**: 向后兼容的新功能
- **PATCH**: 向后兼容的 Bug 修复

### 2. 协议版本管理

```cmake
# 客户端和服务端需要保持同步
# 服务端版本映射规则：
# 0-99   → v1.0.x
# 100-199 → v1.1.x
# 200-299 → v1.2.x
# 300-399 → v1.3.x
# 400+    → v1.4.x
```

### 3. 版本检查时机

```cpp
// 在连接成功后立即检查
void ServerConnectWindow::updateStatus(bool ok, const char* action) {
    if (ok) {
        ServerVersionInfo info;
        if (FetchServerVersion(info)) {
            // 1. 检查服务端版本范围
            int serverProtocolMinor = info.version / 100;
            
            // 2. 验证兼容性
            if (!isServerCompatible(serverProtocolMinor)) {
                GetSocketMgr().GetClient(PORT_MAIN)->Close();
                status = "版本不兼容";
                return;
            }
            
            // 3. 显示版本信息
            Gui::log("✓ 版本兼容");
        }
    }
}
```

---

## 🚀 快速参考

### 常用宏定义

| 宏 | 说明 | 示例值 |
|----|------|--------|
| `PROJECT_VERSION` | 完整项目版本 | "1.0.0" |
| `PROJECT_VERSION_MAJOR` | 主版本号 | 1 |
| `PROTOCOL_VERSION` | 协议版本 | "1.4.0" |
| `PROTOCOL_VERSION_MINOR` | 协议次版本号 | 4 |
| `BUILD_DATE` | 构建日期 | "2024-10-23" |
| `GIT_COMMIT_HASH` | Git 提交哈希 | "a1b2c3d" |

### 常用函数

| 函数 | 返回值 | 说明 |
|------|--------|------|
| `version::getVersionString()` | `const char*` | 完整版本字符串 |
| `version::getProtocolVersion()` | `const char*` | 协议版本字符串 |
| `version::getBuildDateTime()` | `const char*` | 构建时间 |
| `version::compareVersion(m,n,p)` | `int` | 版本比较 |

---

## 🔍 故障排除

### 问题：version.h 文件不存在

**原因**：未执行 CMake configure

**解决**：
```bash
cd build
cmake ..
```

### 问题：版本信息未更新

**原因**：CMake 缓存了旧值

**解决**：
```bash
# 清除缓存重新生成
rmdir /s /q build
mkdir build
cd build
cmake ..
```

### 问题：Git 信息显示 "unknown"

**原因**：不是 Git 仓库或未安装 Git

**解决**：这是正常的，不影响使用。如需显示 Git 信息：
```bash
git init
git add .
git commit -m "Initial commit"
```

---

## 📝 注意事项

1. ⚠️ `version.h.in` 文件中的 `@变量@` 语法在IDE中会显示语法错误，这是正常的
2. ⚠️ 生成的 `version.h` 在 `build/generated/` 目录，不要手动修改
3. ⚠️ 每次修改 CMakeLists.txt 中的版本号后需要重新运行 CMake
4. ✅ 版本信息会在每次编译时自动更新（日期、时间、Git 信息）

---

**最后更新：2024-10-23**
