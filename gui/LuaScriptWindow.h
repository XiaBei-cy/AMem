#pragma once

#include "Window.h"
#include <vector>
#include <string>
#include <filesystem>

class LuaScriptWindow : public Window {
public:
    LuaScriptWindow();
    ~LuaScriptWindow();

    void onDraw() override;

private:
    void drawScriptList();
    void drawScriptOutput();
    void drawScriptControls();
    void refreshScriptList();
    void executeScript(const std::string& filepath);
    void stopScript();
    void reloadScript(const std::string& name);

    // 脚本列表
    std::vector<std::string> scriptFiles;
    std::vector<bool> scriptSelected;
    int selectedScriptIndex = -1;

    // 脚本输出日志
    std::vector<std::string> outputLog;
    static const int MAX_LOG_LINES = 1000;

    // 脚本执行状态
    bool scriptRunning = false;
    std::string currentScript;

    // 脚本目录
    std::string scriptDirectory = "./scripts";
};

