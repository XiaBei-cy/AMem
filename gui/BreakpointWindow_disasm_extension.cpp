// 临时文件 - 将此内容添加到 BreakpointWindow.cpp 末尾

// 读取并反汇编PC地址周围的指令
#include <cstdint>
void BreakpointWindow::drawDisassemblyForPC(uint64_t pcAddress, int beforeCount, int afterCount)
{
    if (!disassemblyInitialized || !disassemblyHelper) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "反汇编引擎未初始化");
        return;
    }
    
    if (pcAddress == 0) {
        ImGui::TextDisabled("PC地址无效");
        return;
    }
    
    // ARM64指令是4字节对齐的
    const uint32_t INSTRUCTION_SIZE = 4;
    
    // 计算需要读取的内存范围
    // 为了正确反汇编PC之前的指令，需要从更早的地址开始读取
    uint64_t startAddress = pcAddress - (beforeCount * INSTRUCTION_SIZE);
    uint32_t totalInstructions = beforeCount + 1 + afterCount;  // 包括PC位置的指令
    uint32_t totalSize = totalInstructions * INSTRUCTION_SIZE;
    
    ImGui::Text("读取范围: 0x%llX - 0x%llX (%u 字节)", startAddress, startAddress + totalSize - 1, totalSize);
    ImGui::Separator();
    
    // 从远程进程读取内存
    std::vector<unsigned char> memoryData;
    bool readSuccess = ReadProcessMemoryBytes(startAddress, totalSize, memoryData);
    
    if (!readSuccess || memoryData.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "内存读取失败");
        ImGui::TextWrapped("无法从地址 0x%llX 读取 %u 字节的内存。", startAddress, totalSize);
        ImGui::Separator();
        ImGui::Text("可能的原因:");
        ImGui::BulletText("地址无效或不可访问");
        ImGui::BulletText("进程未连接或句柄无效");
        ImGui::BulletText("内存区域没有执行权限");
        
        // 提供重试按钮
        if (ImGui::Button("重试读取")) {
            // 按钮点击会在下一帧重新绘制
        }
        return;
    }
    
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "内存读取成功 (%zu 字节)", memoryData.size());
    ImGui::Separator();
    
    // 反汇编读取的内存
    DisassemblyResult result = disassemblyHelper->disassembleMultiple(
        startAddress, 
        memoryData.data(), 
        memoryData.size(), 
        totalInstructions
    );
    
    if (!result.success) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "反汇编失败: %s", result.errorMessage.c_str());
        return;
    }
    
    if (result.instructions.empty()) {
        ImGui::TextDisabled("没有反汇编出任何指令");
        return;
    }
    
    // 显示反汇编结果
    ImGui::Text("架构: %s", DisassemblyHelper::getArchitectureName(disassemblyHelper->getCurrentArchitecture()).c_str());
    ImGui::Text("反汇编指令数: %d / %d", (int)result.instructions.size(), totalInstructions);
    ImGui::Separator();
    
    // 使用表格显示反汇编指令
    if (ImGui::BeginTable("DisassemblyPCTable", 5, 
        ImGuiTableFlags_Borders | 
        ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("标记", ImGuiTableColumnFlags_WidthFixed, 40);
        ImGui::TableSetupColumn("地址", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("十六进制", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("助记符", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("操作数", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        for (const auto& instr : result.instructions) {
            bool isCurrentPC = (instr.address == pcAddress);
            
            ImGui::TableNextRow();
            
            // 如果是当前PC，高亮显示整行
            if (isCurrentPC) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 
                    ImGui::GetColorU32(ImVec4(0.3f, 0.5f, 0.3f, 0.4f)));
            }
            
            // 标记列 - 显示PC指示器
            ImGui::TableSetColumnIndex(0);
            if (isCurrentPC) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "=>");
            } else {
                ImGui::TextDisabled("  ");
            }
            
            // 地址列
            ImGui::TableSetColumnIndex(1);
            char addrStr[32];
            snprintf(addrStr, sizeof(addrStr), "0x%llX", instr.address);
            
            // 当前PC用不同颜色显示
            if (isCurrentPC) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", addrStr);
            } else {
                if (ImGui::Selectable(addrStr, false, ImGuiSelectableFlags_SpanAllColumns)) {
                    // 点击地址可以跳转到内存查看器
                    MemoryViewerWindow* viewer = ensureMemoryViewerWindow();
                    if (viewer) {
                        viewer->jumpToAddress(instr.address);
                        Gui::log("跳转到地址: 0x%llX", instr.address);
                    }
                }
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("地址: 0x%llX\n大小: %u 字节%s", 
                                 instr.address, instr.size,
                                 isCurrentPC ? "\n[当前PC]" : "");
            }
            
            // 右键菜单
            char popup_id[64];
            snprintf(popup_id, sizeof(popup_id), "DisasmPCPopup_%llX", instr.address);
            if (ImGui::BeginPopupContextItem(popup_id)) {
                if (ImGui::MenuItem("复制地址")) {
                    ImGui::SetClipboardText(addrStr);
                }
                if (ImGui::MenuItem("复制指令")) {
                    ImGui::SetClipboardText(instr.fullInstruction.c_str());
                }
                if (ImGui::MenuItem("复制十六进制")) {
                    ImGui::SetClipboardText(instr.hexBytes.c_str());
                }
                ImGui::Separator();
                if (ImGui::MenuItem("在内存查看器中打开")) {
                    MemoryViewerWindow* viewer = ensureMemoryViewerWindow();
                    if (viewer) {
                        viewer->jumpToAddress(instr.address);
                    }
                }
                ImGui::EndPopup();
            }
            
            // 十六进制字节列
            ImGui::TableSetColumnIndex(2);
            if (isCurrentPC) {
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "%s", instr.hexBytes.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", instr.hexBytes.c_str());
            }
            
            // 助记符列
            ImGui::TableSetColumnIndex(3);
            if (isCurrentPC) {
                ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.6f, 1.0f), "%s", instr.mnemonic.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "%s", instr.mnemonic.c_str());
            }
            
            // 操作数列
            ImGui::TableSetColumnIndex(4);
            if (!instr.operands.empty()) {
                if (isCurrentPC) {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", instr.operands.c_str());
                } else {
                    ImGui::Text("%s", instr.operands.c_str());
                }
            } else {
                ImGui::TextDisabled("-");
            }
        }
        
        ImGui::EndTable();
    }
    
    // 显示说明
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "提示: => 标记表示当前PC位置");
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "右键点击指令可复制或在内存查看器中查看");
}
