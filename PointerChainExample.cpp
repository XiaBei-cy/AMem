#include "PointerChainTree.h"
#include <memory>

// 全局指针链树对象
static std::unique_ptr<PointerChainTreeView> g_pointer_chain_tree;

// 初始化示例数据
void InitializePointerChainExample() {
    try {
        g_pointer_chain_tree = std::make_unique<PointerChainTreeView>();
        
        // 创建根节点
        PointerChainNode* root = new PointerChainNode("BaseAddress", 0x140000000);
        root->AddOffset(0);
        root->AddOffset(8);
        root->AddOffset(16);
        
        // 第一层：3个分支（树形展开）
        PointerChainNode* nodeA = root->AddChild("分支A", 0x10000000);
        nodeA->AddOffset(0x10);
        nodeA->AddOffset(0x20);
        
        PointerChainNode* nodeB = root->AddChild("分支B", 0x20000000);
        nodeB->AddOffset(0x30);
        
        PointerChainNode* nodeC = root->AddChild("分支C", 0x30000000);
        nodeC->AddOffset(0x40);
        nodeC->AddOffset(0x50);
        nodeC->AddOffset(0x60);
        
        // 第二层：nodeA 的子节点
        PointerChainNode* nodeA1 = nodeA->AddChild("A-1", 0x11000000);
        nodeA1->AddOffset(0x100);
        
        PointerChainNode* nodeA2 = nodeA->AddChild("A-2", 0x12000000);
        nodeA2->AddOffset(0x200);
        nodeA2->AddOffset(0x210);
        
        // 第二层：nodeB 的子节点
        PointerChainNode* nodeB1 = nodeB->AddChild("B-1", 0x21000000);
        nodeB1->AddOffset(0x300);
        
        // 第二层：nodeC 的多个子节点（展示扇形）
        PointerChainNode* nodeC1 = nodeC->AddChild("C-1", 0x31000000);
        nodeC1->AddOffset(0x400);
        
        PointerChainNode* nodeC2 = nodeC->AddChild("C-2", 0x32000000);
        nodeC2->AddOffset(0x500);
        
        PointerChainNode* nodeC3 = nodeC->AddChild("C-3", 0x33000000);
        nodeC3->AddOffset(0x600);
        
        // 第三层：更深层级
        PointerChainNode* nodeA11 = nodeA1->AddChild("A-1-1", 0x11100000);
        nodeA11->AddOffset(0x1000);
        
        PointerChainNode* nodeA12 = nodeA1->AddChild("A-1-2", 0x11200000);
        nodeA12->AddOffset(0x1100);
        
        // 默认折叠某些节点以展示折叠功能
        nodeC->collapsed = true;  // 折叠分支C
        nodeA1->collapsed = true; // 折叠A-1
        
        g_pointer_chain_tree->SetRoot(root);
    }
    catch (...) {
        g_pointer_chain_tree.reset();
    }
}

// 渲染指针链示例
void RenderPointerChainExample() {
    static bool initialized = false;
    if (!initialized) {
        InitializePointerChainExample();
        initialized = true;
    }
    
    if (!g_pointer_chain_tree) return;
    if (!g_pointer_chain_tree->root) return;
    
    // 设置窗口大小
    ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("指针链蓝图编辑器", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    // 顶部控制栏
    ImGui::BeginChild("ControlBar", ImVec2(0, 80), ImGuiChildFlags_Borders);
    
    ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "🎨 指针链可视化编辑器");
    ImGui::Separator();
    
    // 控制按钮行
    ImGui::BeginGroup();
    
    // 显示模式
    if (ImGui::Checkbox("十六进制", &g_pointer_chain_tree->show_hex)) {}
    ImGui::SameLine();
    
    if (ImGui::Checkbox("紧凑模式", &g_pointer_chain_tree->compact_mode)) {}
    ImGui::SameLine();
    
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();
    
    // 缩放显示
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.8f, 1.0f), "缩放: %.1f%%", g_pointer_chain_tree->zoom_level * 100.0f);
    ImGui::SameLine();
    
    // 重置缩放按钮
    if (ImGui::Button("🔍 重置缩放")) {
        g_pointer_chain_tree->zoom_level = 1.0f;
    }
    ImGui::SameLine();
    
    // 重置按钮
    if (ImGui::Button("🔄 重置布局")) {
        g_pointer_chain_tree->SetRoot(g_pointer_chain_tree->root);
    }
    ImGui::SameLine();
    
    if (ImGui::Button("❌ 清除选择")) {
        g_pointer_chain_tree->selected_node = nullptr;
    }
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // 操作提示
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "💡 操作:");
    ImGui::SameLine();
    ImGui::Text("🖱️ 左键拖动节点");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("🖱️ 右键拖动画布");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Text("🖱️ 滚轮缩放");
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "点击 ▼/▶ 折叠节点");
    
    ImGui::EndChild();
    
    ImGui::Spacing();
    
    // 主画布区域
    ImGui::BeginChild("Canvas", ImVec2(0, -150), ImGuiChildFlags_Borders);
    
    // 深色背景
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    draw_list->AddRectFilled(canvas_p0, ImVec2(canvas_p0.x + canvas_size.x, canvas_p0.y + canvas_size.y), 
                              IM_COL32(25, 25, 28, 255));
    
    // 渲染树
    try {
        if (g_pointer_chain_tree && g_pointer_chain_tree->root) {
            g_pointer_chain_tree->Render();
        }
    }
    catch (...) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "渲染错误");
    }
    
    ImGui::EndChild();
    
    // 底部信息栏
    ImGui::BeginChild("InfoBar", ImVec2(0, 0), ImGuiChildFlags_Borders);
    
    if (g_pointer_chain_tree->selected_node) {
        PointerChainNode* selected = g_pointer_chain_tree->selected_node;
        
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "选中节点:");
        ImGui::SameLine();
        ImGui::Text("%s", selected->name.c_str());
        
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(20, 0));
        ImGui::SameLine();
        
        ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.0f, 1.0f), "地址:");
        ImGui::SameLine();
        if (g_pointer_chain_tree->show_hex) {
            ImGui::Text("0x%llX", selected->address);
        } else {
            ImGui::Text("%llu", selected->address);
        }
        
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(20, 0));
        ImGui::SameLine();
        
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "偏移数:");
        ImGui::SameLine();
        ImGui::Text("%zu", selected->offsets.size());
        
        if (!selected->offsets.empty()) {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            ImGui::SameLine();
            
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "偏移链:");
            ImGui::SameLine();
            
            std::string offsets_str;
            for (size_t i = 0; i < selected->offsets.size(); i++) {
                if (i > 0) offsets_str += " → ";
                offsets_str += std::to_string(selected->offsets[i]);
            }
            ImGui::Text("%s", offsets_str.c_str());
        }
        
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(20, 0));
        ImGui::SameLine();
        
        ImGui::Text("子节点: %zu", selected->children.size());
        
        if (!selected->children.empty()) {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(20, 0));
            ImGui::SameLine();
            
            ImGui::TextColored(
                selected->collapsed ? ImVec4(1.0f, 0.5f, 0.5f, 1.0f) : ImVec4(0.5f, 1.0f, 0.5f, 1.0f), 
                selected->collapsed ? "已折叠 ▶" : "已展开 ▼"
            );
        }
        
    } else {
        ImGui::TextDisabled("未选择节点 - 点击节点查看详情");
    }
    
    ImGui::EndChild();
    
    ImGui::End();
}

// 清理资源
void CleanupPointerChainExample() {
    g_pointer_chain_tree.reset();
} 