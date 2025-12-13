#include "LuaScriptWindow.h"
#include "../lua/LuaEngine.h"
#include "../imgui/imgui.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

LuaScriptWindow::LuaScriptWindow() {
    name = "Lua脚本管理器";
    refreshScriptList();
}

LuaScriptWindow::~LuaScriptWindow() {
}

void LuaScriptWindow::onDraw() {
    if (!pOpen) return;

    ImGui::Begin(name.c_str(), &pOpen);

    drawScriptControls();
    ImGui::Separator();

    ImGui::Columns(2, "ScriptColumns", true);
    ImGui::SetColumnWidth(0, 300);

    // 左侧：脚本列表
    ImGui::BeginChild("ScriptList", ImVec2(0, 0), false);
    drawScriptList();
    ImGui::EndChild();

    ImGui::NextColumn();

    // 右侧：脚本输出
    ImGui::BeginChild("ScriptOutput", ImVec2(0, 0), false);
    drawScriptOutput();
    ImGui::EndChild();

    ImGui::Columns(1);

    ImGui::End();
}

void LuaScriptWindow::drawScriptControls() {
    if (ImGui::Button("刷新列表")) {
        refreshScriptList();
    }
    ImGui::SameLine();

    if (scriptRunning) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        if (ImGui::Button("停止脚本")) {
            stopScript();
        }
        ImGui::PopStyleColor();
    } else {
        if (ImGui::Button("执行选中")) {
            if (selectedScriptIndex >= 0 && selectedScriptIndex < scriptFiles.size()) {
                std::string filepath = scriptDirectory + "/" + scriptFiles[selectedScriptIndex];
                executeScript(filepath);
            }
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("重载选中")) {
        if (selectedScriptIndex >= 0 && selectedScriptIndex < scriptFiles.size()) {
            reloadScript(scriptFiles[selectedScriptIndex]);
        }
    }

    ImGui::SameLine();
    ImGui::Text("脚本目录: %s", scriptDirectory.c_str());
}

void LuaScriptWindow::drawScriptList() {
    ImGui::Text("脚本列表 (%d)", static_cast<int>(scriptFiles.size()));
    ImGui::Separator();

    if (scriptFiles.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "没有找到脚本文件");
        ImGui::Text("请将.lua文件放在 %s 目录下", scriptDirectory.c_str());
        return;
    }

    for (size_t i = 0; i < scriptFiles.size(); ++i) {
        bool isSelected = (selectedScriptIndex == static_cast<int>(i));
        if (ImGui::Selectable(scriptFiles[i].c_str(), isSelected)) {
            selectedScriptIndex = static_cast<int>(i);
        }

        // 双击执行
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            if (!scriptRunning) {
                std::string filepath = scriptDirectory + "/" + scriptFiles[i];
                executeScript(filepath);
            }
        }
    }
}

void LuaScriptWindow::drawScriptOutput() {
    ImGui::Text("脚本输出");
    ImGui::Separator();

    if (ImGui::Button("清空日志")) {
        outputLog.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("复制日志")) {
        std::string allLog;
        for (const auto& line : outputLog) {
            allLog += line + "\n";
        }
        ImGui::SetClipboardText(allLog.c_str());
    }

    ImGui::Separator();

    // 显示日志
    ImGui::BeginChild("LogContent", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& line : outputLog) {
        ImGui::TextUnformatted(line.c_str());
    }

    // 自动滚动到底部
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

void LuaScriptWindow::refreshScriptList() {
    scriptFiles.clear();
    scriptSelected.clear();

    if (!std::filesystem::exists(scriptDirectory)) {
        std::filesystem::create_directories(scriptDirectory);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(scriptDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            scriptFiles.push_back(entry.path().filename().string());
            scriptSelected.push_back(false);
        }
    }

    // 排序
    std::sort(scriptFiles.begin(), scriptFiles.end());
}

void LuaScriptWindow::executeScript(const std::string& filepath) {
    if (scriptRunning) {
        return;
    }

    auto& engine = LuaEngine::GetInstance();
    if (!engine.IsInitialized()) {
        if (!engine.Initialize()) {
            outputLog.push_back("[错误] Lua引擎初始化失败: " + engine.GetLastError());
            if (outputLog.size() > MAX_LOG_LINES) {
                outputLog.erase(outputLog.begin());
            }
            return;
        }
    }

    scriptRunning = true;
    currentScript = filepath;
    outputLog.push_back("[执行] " + std::filesystem::path(filepath).filename().string());

    bool success = engine.ExecuteFile(filepath);
    if (success) {
        outputLog.push_back("[成功] 脚本执行完成");
    } else {
        outputLog.push_back("[错误] " + engine.GetLastError());
    }

    // 限制日志行数
    if (outputLog.size() > MAX_LOG_LINES) {
        outputLog.erase(outputLog.begin(), outputLog.begin() + (outputLog.size() - MAX_LOG_LINES));
    }

    scriptRunning = false;
    currentScript.clear();
}

void LuaScriptWindow::stopScript() {
    // LuaJIT不支持直接停止正在执行的脚本
    // 这里只能标记状态，实际停止需要在脚本中检查标志
    scriptRunning = false;
    outputLog.push_back("[停止] 脚本执行已停止");
}

void LuaScriptWindow::reloadScript(const std::string& name) {
    auto& engine = LuaEngine::GetInstance();
    if (!engine.IsInitialized()) {
        return;
    }

    bool success = engine.ReloadScript(name);
    if (success) {
        outputLog.push_back("[重载] " + name + " 已重新加载");
    } else {
        outputLog.push_back("[错误] 重载失败: " + engine.GetLastError());
    }

    if (outputLog.size() > MAX_LOG_LINES) {
        outputLog.erase(outputLog.begin(), outputLog.begin() + (outputLog.size() - MAX_LOG_LINES));
    }
}

