#include "LuaScriptWindow.h"
#include "../lua/LuaEngine.h"
#include "../imgui/imgui.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

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

    if (ImGui::Button("选择文件")) {
        std::string selectedFile;
        if (openFileDialog(selectedFile)) {
            // 规范化路径
            try {
                std::filesystem::path filePath(selectedFile);
                std::string normalizedPath = std::filesystem::canonical(filePath).string();
                std::string filename = filePath.filename().string();
                
                // 检查是否已存在（使用规范化路径比较）
                bool exists = false;
                for (size_t i = 0; i < scriptFilePaths.size(); ++i) {
                    try {
                        std::filesystem::path existingPath(scriptFilePaths[i]);
                        std::string existingNormalized = std::filesystem::canonical(existingPath).string();
                        if (existingNormalized == normalizedPath) {
                            exists = true;
                            selectedScriptIndex = static_cast<int>(i);
                            outputLog.push_back("[提示] 文件已在列表中: " + filename);
                            break;
                        }
                    } catch (...) {
                        // 如果路径无效，直接比较字符串
                        if (scriptFilePaths[i] == normalizedPath || scriptFilePaths[i] == selectedFile) {
                            exists = true;
                            selectedScriptIndex = static_cast<int>(i);
                            break;
                        }
                    }
                }
                
                if (!exists) {
                    scriptFiles.push_back(filename);
                    scriptFilePaths.push_back(normalizedPath);
                    scriptSelected.push_back(false);
                    selectedScriptIndex = static_cast<int>(scriptFiles.size() - 1);
                    outputLog.push_back("[添加] " + filename + " (" + normalizedPath + ")");
                    
                    // 限制日志行数
                    if (outputLog.size() > MAX_LOG_LINES) {
                        outputLog.erase(outputLog.begin(), outputLog.begin() + (outputLog.size() - MAX_LOG_LINES));
                    }
                }
            } catch (const std::exception& e) {
                outputLog.push_back("[错误] 无法处理文件: " + std::string(e.what()));
            }
        }
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
                std::string filepath;
                if (selectedScriptIndex < static_cast<int>(scriptFilePaths.size()) && 
                    !scriptFilePaths[selectedScriptIndex].empty()) {
                    // 使用完整路径（外部文件）
                    filepath = scriptFilePaths[selectedScriptIndex];
                } else {
                    // 使用相对路径（默认目录中的文件）
                    filepath = scriptDirectory + "/" + scriptFiles[selectedScriptIndex];
                }
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
        ImGui::Text("或点击'选择文件'按钮从其他目录选择");
        return;
    }

    for (size_t i = 0; i < scriptFiles.size(); ++i) {
        bool isSelected = (selectedScriptIndex == static_cast<int>(i));
        
        // 显示文件名，如果是外部文件则显示路径提示
        std::string displayName = scriptFiles[i];
        if (i < scriptFilePaths.size() && !scriptFilePaths[i].empty()) {
            std::filesystem::path filePath(scriptFilePaths[i]);
            std::filesystem::path dirPath = filePath.parent_path();
            std::string dirStr = dirPath.string();
            // 如果路径很长，只显示最后一部分
            if (dirStr.length() > 40) {
                dirStr = "..." + dirStr.substr(dirStr.length() - 20);
            }
            displayName += " [" + dirStr + "]";
        }
        
        if (ImGui::Selectable(displayName.c_str(), isSelected)) {
            selectedScriptIndex = static_cast<int>(i);
        }

        // 双击执行
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            if (!scriptRunning) {
                std::string filepath;
                if (i < scriptFilePaths.size() && !scriptFilePaths[i].empty()) {
                    filepath = scriptFilePaths[i];
                } else {
                    filepath = scriptDirectory + "/" + scriptFiles[i];
                }
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
    scriptFilePaths.clear();
    scriptSelected.clear();

    if (!std::filesystem::exists(scriptDirectory)) {
        std::filesystem::create_directories(scriptDirectory);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(scriptDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            try {
                std::string normalizedPath = std::filesystem::canonical(entry.path()).string();
                scriptFiles.push_back(entry.path().filename().string());
                scriptFilePaths.push_back(normalizedPath);  // 存储规范化完整路径
                scriptSelected.push_back(false);
            } catch (...) {
                // 如果无法规范化，使用原始路径
                scriptFiles.push_back(entry.path().filename().string());
                scriptFilePaths.push_back(entry.path().string());
                scriptSelected.push_back(false);
            }
        }
    }

    // 排序（同时保持 scriptFilePaths 同步）
    std::vector<std::pair<std::string, std::string>> pairs;
    for (size_t i = 0; i < scriptFiles.size(); ++i) {
        pairs.push_back({scriptFiles[i], scriptFilePaths[i]});
    }
    std::sort(pairs.begin(), pairs.end());
    
    scriptFiles.clear();
    scriptFilePaths.clear();
    for (const auto& pair : pairs) {
        scriptFiles.push_back(pair.first);
        scriptFilePaths.push_back(pair.second);
    }
}

bool LuaScriptWindow::openFileDialog(std::string& selectedFile) {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Lua脚本文件\0*.lua\0所有文件\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameA(&ofn) == TRUE) {
        selectedFile = szFile;
        return true;
    }
    
    return false;
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

