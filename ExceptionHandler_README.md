# 异常终止捕获模块 (ExceptionHandler)

## 功能特性

这是一个全面的Windows异常捕获和崩溃报告模块，提供以下功能：

### 支持的异常类型
- ✅ **SEH异常** - Windows结构化异常处理
  - 访问违例 (Access Violation)
  - 除零错误 (Divide by Zero)
  - 堆栈溢出 (Stack Overflow)
  - 非法指令 (Illegal Instruction)
  - 等等...

- ✅ **C++标准异常**
  - terminate() 调用
  - 纯虚函数调用
  - 无效参数

- ✅ **信号处理**
  - SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM

- ✅ **自动生成崩溃转储文件** (.dmp文件)
- ✅ **自定义异常回调函数**
- ✅ **详细的异常信息描述**（中英文）

---

## 快速开始

### 1. 基本集成

在 `main.cpp` 文件开头包含头文件：

```cpp
#include "ExceptionHandler.h"
```

### 2. 定义异常回调函数（可选）

```cpp
void OnCrash(const ExceptionHandler::ExceptionInfo& info)
{
    // 显示错误对话框
    std::string message = "程序崩溃!\n";
    message += "原因: " + info.description + "\n";
    message += "Dump文件: " + info.dumpFilePath;
    
    MessageBoxA(nullptr, message.c_str(), "错误", MB_OK | MB_ICONERROR);
    
    // 可以在这里添加日志记录
    // LogToFile(info);
}
```

### 3. 在 main() 函数开头初始化

```cpp
int main(int, char**)
{
    // 初始化异常处理器
    ExceptionHandler::Initialize(
        "MyApplication",      // 应用程序名称
        ".\\CrashDumps\\",   // dump文件保存目录
        OnCrash              // 异常回调函数（可选，传nullptr则不回调）
    );

    // ... 您的原有代码 ...
    
    return 0;
}
```

---

## 完整的集成示例

修改您的 `main.cpp`：

```cpp
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <string>
#include "gui/Gui.h"

// *** 添加异常处理器 ***
#include "ExceptionHandler.h"

// *** 定义崩溃回调函数 ***
void OnApplicationCrash(const ExceptionHandler::ExceptionInfo& info)
{
    std::string message = "程序发生异常崩溃!\n\n";
    message += "异常描述: " + info.description + "\n";
    message += "崩溃转储: " + info.dumpFilePath + "\n\n";
    message += "程序将退出。";
    
    MessageBoxA(nullptr, message.c_str(), "Android 修改器引擎 - 崩溃", 
                MB_OK | MB_ICONERROR | MB_TOPMOST);
}

int main(int, char**)
{
    // *** 首先初始化异常处理器 ***
    ExceptionHandler::Initialize(
        "AndroidModEngine",
        ".\\CrashDumps\\",
        OnApplicationCrash
    );

    // 创建应用程序窗口
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, 
                       GetModuleHandle(nullptr), nullptr, nullptr, 
                       nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Android 修改器引擎", 
                                 WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, 
                                 nullptr, nullptr, wc.hInstance, nullptr);

    // ... 其余代码保持不变 ...

    return 0;
}
```

---

## API 参考

### Initialize 函数

```cpp
bool ExceptionHandler::Initialize(
    const char* applicationName,      // 应用程序名称（用于dump文件命名）
    const char* dumpDirectory,        // dump文件保存目录
    ExceptionCallback callback        // 异常回调函数（可选，可传nullptr）
);
```

**返回值**: 初始化成功返回 `true`

### ExceptionInfo 结构

```cpp
struct ExceptionInfo
{
    ExceptionType type;                  // 异常类型
    DWORD exceptionCode;                 // 异常代码
    PVOID exceptionAddress;              // 异常地址
    std::string description;             // 异常描述（中英文）
    std::string dumpFilePath;            // dump文件路径
    EXCEPTION_POINTERS* pExceptionPointers; // Windows异常指针
};
```

### 手动触发崩溃报告

```cpp
// 在需要手动生成崩溃报告的地方调用
ExceptionHandler::TriggerExceptionReport("自定义原因描述");
```

---

## 生成的文件

崩溃时会在指定目录下生成 `.dmp` 文件：

```
应用程序名_异常类型_时间戳.dmp
```

示例：
- `AndroidModEngine_crash_20231015_143025.dmp`
- `AndroidModEngine_signal_20231015_150130.dmp`
- `AndroidModEngine_terminate_20231015_162045.dmp`

### 如何分析 .dmp 文件

使用 Visual Studio：
1. 打开 Visual Studio
2. 文件 → 打开 → 文件
3. 选择 `.dmp` 文件
4. 点击"使用本机调试"
5. 查看调用堆栈和变量

---

## 注意事项

1. **需要 `DbgHelp.lib`** - 已自动链接
2. **Release 模式** - 确保生成 PDB 文件以便调试
3. **磁盘空间** - dump文件可能较大（几MB到几十MB）
4. **权限** - 确保程序有写入dump目录的权限

---

## 测试异常捕获

在开发阶段，可以使用以下代码测试异常处理是否工作：

```cpp
// 测试访问违例
void TestCrash()
{
    int* ptr = nullptr;
    *ptr = 42;  // 触发崩溃
}

// 在某个按钮点击事件中调用
if (ImGui::Button("测试崩溃捕获"))
{
    TestCrash();
}
```

---

## 项目配置要求

### CMakeLists.txt
无需特殊配置，header-only 实现。

### Visual Studio
- 配置类型：任意（Debug/Release）
- C++ 标准：C++11 或更高
- 字符集：多字节字符集或Unicode均可

---

## 常见问题

**Q: dump文件没有生成？**  
A: 检查目录权限，确保程序可以写入指定目录。

**Q: 回调函数没有被调用？**  
A: 确保在 main() 函数最开始就调用 `Initialize()`。

**Q: 可以不要回调函数吗？**  
A: 可以，传入 `nullptr` 即可：`Initialize("App", ".\\", nullptr)`

**Q: 支持Linux/macOS吗？**  
A: 当前仅支持Windows。Linux需要使用signal/backtrace，macOS需要使用Mach异常处理。

---

## 许可证

此代码可自由使用和修改。

