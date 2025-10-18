// ExceptionHandler 使用示例
// 将此代码集成到 main.cpp 中

#include "ExceptionHandler.h"
#include <iostream>
#include <Windows.h>

// 异常回调函数 - 当发生崩溃时会被调用
void OnCrash(const ExceptionHandler::ExceptionInfo& info)
{
    // 显示错误消息框
    std::string message = "程序发生了异常!\n\n";
    message += "异常类型: " + info.description + "\n";
    message += "异常代码: 0x" + std::to_string(info.exceptionCode) + "\n";
    
    if (info.exceptionAddress)
    {
        char addressStr[32];
        sprintf_s(addressStr, "0x%p", info.exceptionAddress);
        message += "异常地址: " + std::string(addressStr) + "\n";
    }
    
    if (!info.dumpFilePath.empty())
    {
        message += "\n崩溃转储文件已保存到:\n" + info.dumpFilePath;
    }
    
    message += "\n\n程序将终止。";
    
    MessageBoxA(nullptr, message.c_str(), "程序崩溃", MB_OK | MB_ICONERROR);
    
    // 可以在这里添加日志记录
    // 例如：将崩溃信息写入日志文件
}

// 在 main() 函数开头调用此函数进行初始化
void InitializeExceptionHandler()
{
    // 初始化异常处理器
    // 参数1: 应用程序名称
    // 参数2: dump文件保存目录
    // 参数3: 异常回调函数
    ExceptionHandler::Initialize(
        "AndroidModifierEngine",  // 应用程序名称
        ".\\CrashDumps\\",        // dump文件保存目录
        OnCrash                    // 异常回调函数
    );
}

// ============= 测试函数（用于测试异常捕获是否工作）=============

// 测试访问违例
void TestAccessViolation()
{
    int* ptr = nullptr;
    *ptr = 42;  // 这会导致访问违例
}

// 测试除零异常
void TestDivideByZero()
{
    int a = 10;
    int b = 0;
    int c = a / b;  // 整数除零
}

// 测试堆栈溢出
void TestStackOverflow()
{
    TestStackOverflow();  // 无限递归
}

// 测试纯虚函数调用
class BaseClass
{
public:
    virtual void PureVirtualFunction() = 0;
    virtual ~BaseClass() {}
};

class DerivedClass : public BaseClass
{
public:
    // 故意不实现纯虚函数
};

void TestPureVirtualCall()
{
    // 这会导致纯虚函数调用错误（需要特定条件）
    BaseClass* obj = new DerivedClass();
    delete obj;
}

// 测试无效参数
void TestInvalidParameter()
{
    FILE* file = nullptr;
    fclose(file);  // 传递空指针
}

// ============= 集成到 main.cpp 的示例 =============

/*
int main(int, char**)
{
    // *** 在程序启动时初始化异常处理器 ***
    InitializeExceptionHandler();

    // ... 原有的窗口创建代码 ...
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Android 修改器引擎", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // ... 其余代码保持不变 ...

    return 0;
}
*/

// ============= 如何手动触发崩溃报告 =============

void ManualCrashReport()
{
    // 如果需要在某个特定错误点手动生成崩溃报告
    ExceptionHandler::TriggerExceptionReport("用户请求的手动崩溃报告");
}

