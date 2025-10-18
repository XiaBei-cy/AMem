#pragma once
#include "imgui/imgui.h"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <cmath>
#include <unordered_map>

// 指针链节点
class PointerChainNode {
public:
    std::string name;                        // 节点名称
    uint64_t address;                        // 地址
    std::vector<int> offsets;                // 偏移量列表
    std::vector<PointerChainNode*> children; // 子节点
    void* user_data;                         // 用户数据
    
    // 节点位置（用于拖动）
    ImVec2 position;
    bool position_initialized;
    bool collapsed;                          // 是否折叠子节点
    
    PointerChainNode(const std::string& node_name, uint64_t addr = 0)
        : name(node_name), address(addr), user_data(nullptr)
        , position(0, 0), position_initialized(false), collapsed(false) {}
    
    ~PointerChainNode() {
        for (auto child : children) {
            delete child;
        }
    }
    
    // 添加偏移量
    PointerChainNode* AddOffset(int offset) {
        offsets.push_back(offset);
        return this;
    }
    
    // 添加多个偏移量
    PointerChainNode* AddOffsets(const std::vector<int>& offset_list) {
        offsets.insert(offsets.end(), offset_list.begin(), offset_list.end());
        return this;
    }
    
    // 添加子节点
    PointerChainNode* AddChild(const std::string& child_name, uint64_t addr = 0) {
        PointerChainNode* child = new PointerChainNode(child_name, addr);
        children.push_back(child);
        return child;
    }
    
    // 设置地址
    PointerChainNode* SetAddress(uint64_t addr) {
        address = addr;
        return this;
    }
};

// 节点渲染信息
struct NodeRenderInfo {
    ImVec2 size;                       // 节点大小
    std::vector<ImVec2> output_ports;  // 输出端口位置（相对于节点）
    ImVec2 input_port;                 // 输入端口位置（相对于节点）
};

// 指针链树渲染器（蓝图风格）
class PointerChainTreeView {
public:
    PointerChainNode* root;
    PointerChainNode* selected_node;
    bool show_hex;           // 是否显示十六进制
    bool compact_mode;       // 紧凑模式
    float zoom_level;        // 缩放级别
    
    PointerChainTreeView() 
        : root(nullptr)
        , selected_node(nullptr)
        , show_hex(true)
        , compact_mode(false)
        , dragging_node(nullptr)
        , canvas_offset(0, 0)
        , canvas_size(0, 0)
        , zoom_level(1.0f)
    {}
    
    ~PointerChainTreeView() {
        if (root) delete root;
    }
    
    void SetRoot(PointerChainNode* node) {
        if (root) delete root;
        root = node;
        
        // 初始化节点位置
        InitializeNodePositions();
    }
    
    void Render() {
        if (!root) return;
        
        try {
            // 获取画布信息
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            canvas_size = ImGui::GetContentRegionAvail();
            
            // 绘制背景网格
            DrawGrid(canvas_pos, canvas_size);
            
            // 创建画布区域
            ImGui::InvisibleButton("canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft);
            bool canvas_hovered = ImGui::IsItemHovered();
            
            // 处理画布拖动（右键）
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                canvas_offset.x += delta.x;
                canvas_offset.y += delta.y;
            }
            
            // 处理鼠标滚轮缩放
            if (canvas_hovered && ImGui::GetIO().MouseWheel != 0.0f) {
                float wheel = ImGui::GetIO().MouseWheel;
                float zoom_delta = wheel * 0.1f;
                
                // 计算鼠标位置相对于画布的坐标
                ImVec2 mouse_pos = ImGui::GetMousePos();
                ImVec2 mouse_canvas_pos = ImVec2(
                    mouse_pos.x - canvas_pos.x - canvas_offset.x,
                    mouse_pos.y - canvas_pos.y - canvas_offset.y
                );
                
                // 更新缩放级别（限制在0.2到3.0之间）
                float old_zoom = zoom_level;
                zoom_level += zoom_delta;
                zoom_level = std::max(0.2f, std::min(3.0f, zoom_level));
                
                // 调整偏移以保持鼠标位置不变（缩放中心为鼠标位置）
                float zoom_ratio = zoom_level / old_zoom;
                canvas_offset.x = mouse_pos.x - canvas_pos.x - mouse_canvas_pos.x * zoom_ratio;
                canvas_offset.y = mouse_pos.y - canvas_pos.y - mouse_canvas_pos.y * zoom_ratio;
            }
            
            // 渲染所有节点
            render_infos.clear();
            RenderAllNodes();
            
            // 绘制所有连接线
            DrawAllConnections();
            
            // 处理节点拖动
            HandleNodeDragging();
            
        }
        catch (...) {
            // 渲染失败时的保护
        }
    }
    
private:
    std::unordered_map<PointerChainNode*, NodeRenderInfo> render_infos;
    PointerChainNode* dragging_node;
    ImVec2 drag_offset;
    ImVec2 canvas_offset;
    ImVec2 canvas_size;
    
    void InitializeNodePositions() {
        if (!root) return;
        
        // 计算每个节点的子树宽度
        CalculateTreeWidths(root);
        
        // 设置根节点位置（居中）
        float root_x = 600.0f;  // 根节点起始 X 位置
        float root_y = 50.0f;   // 根节点起始 Y 位置
        
        root->position = ImVec2(root_x, root_y);
        root->position_initialized = true;
        
        // 递归布局子节点（调整位置以适应新的节点宽度）
        float node_half_width = compact_mode ? 90.0f : 110.0f;
        LayoutChildren(root, root_x + node_half_width, root_y + 140.0f, 0);
    }
    
    // 计算每个节点的子树总宽度
    float CalculateTreeWidths(PointerChainNode* node) {
        if (!node) return 0;
        
        if (node->children.empty()) {
            // 叶子节点宽度（紧凑化后更窄）
            float leaf_width = compact_mode ? 200.0f : 240.0f;
            node->user_data = (void*)(intptr_t)((int)leaf_width);
            return leaf_width;
        }
        
        float total_width = 0;
        for (auto child : node->children) {
            total_width += CalculateTreeWidths(child);
        }
        
        // 加上子节点之间的间距（减小间距）
        float spacing = compact_mode ? 80.0f : 100.0f;
        total_width += (node->children.size() - 1) * spacing;
        
        // 至少和单个节点一样宽
        float min_width = compact_mode ? 200.0f : 240.0f;
        total_width = std::max(total_width, min_width);
        
        node->user_data = (void*)(intptr_t)((int)total_width);
        return total_width;
    }
    
    // 递归布局子节点
    void LayoutChildren(PointerChainNode* node, float parent_x, float y, int depth) {
        if (!node || node->children.empty()) return;
        
        float total_width = (intptr_t)node->user_data;
        float start_x = parent_x - total_width / 2.0f;
        float current_x = start_x;
        
        for (auto child : node->children) {
            if (!child->position_initialized) {
                float child_width = (intptr_t)child->user_data;
                
                // 子节点居中在其子树宽度内
                float child_x = current_x + child_width / 2.0f;
                
                // 调整节点居中偏移（因为节点更窄了）
                float node_half_width = compact_mode ? 90.0f : 110.0f;
                child->position = ImVec2(child_x - node_half_width, y);
                child->position_initialized = true;
                
                // 递归布局这个子节点的子节点（层间距更紧凑）
                LayoutChildren(child, child_x, y + 160.0f, depth + 1);
                
                // 移动到下一个子节点位置
                float spacing = compact_mode ? 80.0f : 100.0f;
                current_x += child_width + spacing;
            }
        }
    }
    
    void DrawGrid(ImVec2 canvas_pos, ImVec2 canvas_size) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        const float GRID_SIZE = 64.0f * zoom_level;
        const ImU32 GRID_COLOR = IM_COL32(40, 40, 40, 100);
        
        // 计算网格偏移
        float grid_offset_x = fmodf(canvas_offset.x, GRID_SIZE);
        float grid_offset_y = fmodf(canvas_offset.y, GRID_SIZE);
        
        // 绘制垂直线
        for (float x = grid_offset_x; x < canvas_size.x; x += GRID_SIZE) {
            draw_list->AddLine(
                ImVec2(canvas_pos.x + x, canvas_pos.y),
                ImVec2(canvas_pos.x + x, canvas_pos.y + canvas_size.y),
                GRID_COLOR
            );
        }
        
        // 绘制水平线
        for (float y = grid_offset_y; y < canvas_size.y; y += GRID_SIZE) {
            draw_list->AddLine(
                ImVec2(canvas_pos.x, canvas_pos.y + y),
                ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + y),
                GRID_COLOR
            );
        }
    }
    
    void RenderAllNodes() {
        if (!root) return;
        RenderNodeRecursive(root);
    }
    
    void RenderNodeRecursive(PointerChainNode* node) {
        if (!node) return;
        
        // 渲染当前节点
        bool is_root = (node == root);
        NodeRenderInfo info = RenderNode(node, is_root);
        render_infos[node] = info;
        
        // 如果节点未折叠，递归渲染子节点
        if (!node->collapsed) {
            for (auto child : node->children) {
                RenderNodeRecursive(child);
            }
        }
    }
    
    NodeRenderInfo RenderNode(PointerChainNode* node, bool is_root) {
        NodeRenderInfo info;
        
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 node_pos = ImVec2(
            canvas_pos.x + (node->position.x * zoom_level) + canvas_offset.x,
            canvas_pos.y + (node->position.y * zoom_level) + canvas_offset.y
        );
        
        // 紧凑模式：节点更窄
        float node_width = (compact_mode ? 180.0f : 220.0f) * zoom_level;
        
        // 节点样式
        ImVec4 bg_color = is_root ? 
            ImVec4(0.15f, 0.25f, 0.35f, 0.95f) : 
            ImVec4(0.18f, 0.22f, 0.28f, 0.95f);
        
        ImVec4 border_color = (node == selected_node) ?
            ImVec4(1.0f, 0.8f, 0.2f, 1.0f) :
            (is_root ? ImVec4(0.4f, 0.6f, 0.9f, 1.0f) : ImVec4(0.4f, 0.5f, 0.6f, 0.8f));
        
        float border_size = (node == selected_node) ? 3.0f : 2.0f;
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        // 临时存储节点内容高度
        float content_height = 0;
        
        // 标题栏（只显示地址）
        float title_height = 32 * zoom_level;
        content_height += title_height;
        
        // 输出端口（水平排列）
        if (!node->offsets.empty()) {
            content_height += 8 * zoom_level;  // 分隔线
            content_height += 40 * zoom_level; // 偏移量行高度
        }
        
        content_height += 10 * zoom_level; // 底部边距
        
        float node_height = content_height;
        info.size = ImVec2(node_width, node_height);
        
        // 绘制节点背景
        draw_list->AddRectFilled(
            node_pos,
            ImVec2(node_pos.x + node_width, node_pos.y + node_height),
            ImGui::ColorConvertFloat4ToU32(bg_color),
            6.0f * zoom_level
        );
        
        // 绘制节点边框
        draw_list->AddRect(
            node_pos,
            ImVec2(node_pos.x + node_width, node_pos.y + node_height),
            ImGui::ColorConvertFloat4ToU32(border_color),
            6.0f * zoom_level,
            0,
            border_size * zoom_level
        );
        
        // 绘制标题栏背景
        draw_list->AddRectFilled(
            node_pos,
            ImVec2(node_pos.x + node_width, node_pos.y + title_height),
            IM_COL32(30, 45, 65, 255),
            6.0f * zoom_level,
            ImDrawFlags_RoundCornersTop
        );
        
        // 折叠/展开按钮（如果有子节点）
        if (!node->children.empty()) {
            ImVec2 collapse_btn_pos = ImVec2(node_pos.x + 8 * zoom_level, node_pos.y + 8 * zoom_level);
            float collapse_btn_size = 16 * zoom_level;
            
            ImVec2 mouse_pos = ImGui::GetMousePos();
            bool btn_hovered = (mouse_pos.x >= collapse_btn_pos.x && 
                              mouse_pos.x <= collapse_btn_pos.x + collapse_btn_size &&
                              mouse_pos.y >= collapse_btn_pos.y && 
                              mouse_pos.y <= collapse_btn_pos.y + collapse_btn_size);
            
            // 绘制折叠按钮
            ImU32 btn_color = btn_hovered ? IM_COL32(100, 150, 200, 255) : IM_COL32(70, 120, 170, 255);
            draw_list->AddRectFilled(
                collapse_btn_pos,
                ImVec2(collapse_btn_pos.x + collapse_btn_size, collapse_btn_pos.y + collapse_btn_size),
                btn_color,
                3.0f * zoom_level
            );
            
            // 绘制折叠图标
            const char* collapse_icon = node->collapsed ? "▶" : "▼";
            ImVec2 icon_pos = ImVec2(
                collapse_btn_pos.x + 3 * zoom_level, 
                collapse_btn_pos.y + 1 * zoom_level
            );
            
            if (zoom_level >= 0.5f) {  // 只在缩放较大时显示图标
                draw_list->AddText(icon_pos, IM_COL32(255, 255, 255, 255), collapse_icon);
            }
            
            // 检测点击
            if (btn_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                node->collapsed = !node->collapsed;
            }
        }
        
        // 地址显示（标题栏中心，去掉节点名称）
        const char* icon = is_root ? "📌 " : "🔗 ";
        char addr_str[128];
        if (show_hex) {
            snprintf(addr_str, sizeof(addr_str), "%s0x%llX", icon, node->address);
        } else {
            snprintf(addr_str, sizeof(addr_str), "%s%llu", icon, node->address);
        }
        
        if (zoom_level >= 0.5f) {  // 只在缩放较大时显示地址
            ImVec2 addr_pos = ImVec2(node_pos.x + (node->children.empty() ? 10 : 30) * zoom_level, node_pos.y + 8 * zoom_level);
            draw_list->AddText(addr_pos, IM_COL32(100, 230, 255, 255), addr_str);
        }
        
        float y_cursor = node_pos.y + title_height;
        
        // 输入端口（非根节点）
        if (!is_root) {
            ImVec2 input_port_pos = ImVec2(node_pos.x - 8 * zoom_level, node_pos.y + title_height / 2);
            info.input_port = ImVec2(-8 * zoom_level, title_height / 2);
            
            draw_list->AddCircleFilled(input_port_pos, 6.0f * zoom_level, IM_COL32(80, 140, 200, 255));
            draw_list->AddCircle(input_port_pos, 6.0f * zoom_level, IM_COL32(130, 190, 255, 255), 12, 2.0f * zoom_level);
        }
        
        // 输出端口（水平排列）
        if (!node->offsets.empty()) {
            // 分隔线
            draw_list->AddLine(
                ImVec2(node_pos.x + 8 * zoom_level, y_cursor),
                ImVec2(node_pos.x + node_width - 8 * zoom_level, y_cursor),
                IM_COL32(60, 70, 80, 255),
                1.0f * zoom_level
            );
            y_cursor += 8 * zoom_level;
            
            // 计算水平排列的起始位置（居中）
            float box_width = 50.0f * zoom_level;
            float box_height = 28.0f * zoom_level;
            float box_spacing = 6.0f * zoom_level;
            float total_width = node->offsets.size() * box_width + (node->offsets.size() - 1) * box_spacing;
            float start_x = node_pos.x + (node_width - total_width) * 0.5f;
            
            for (size_t i = 0; i < node->offsets.size(); i++) {
                float box_x = start_x + i * (box_width + box_spacing);
                ImVec2 box_pos = ImVec2(box_x, y_cursor);
                ImVec2 box_size = ImVec2(box_width, box_height);
                
                // 偏移量方框
                draw_list->AddRectFilled(
                    box_pos,
                    ImVec2(box_pos.x + box_size.x, box_pos.y + box_size.y),
                    IM_COL32(40, 85, 40, 255),
                    3.0f * zoom_level
                );
                draw_list->AddRect(
                    box_pos,
                    ImVec2(box_pos.x + box_size.x, box_pos.y + box_size.y),
                    IM_COL32(90, 170, 90, 255),
                    3.0f * zoom_level,
                    0,
                    1.3f * zoom_level
                );
                
                // 偏移量数值（居中显示）
                if (zoom_level >= 0.5f) {  // 只在缩放较大时显示数值
                    char value[32];
                    if (show_hex) {
                        snprintf(value, sizeof(value), "0x%X", node->offsets[i]);
                    } else {
                        snprintf(value, sizeof(value), "%d", node->offsets[i]);
                    }
                    ImVec2 text_size = ImGui::CalcTextSize(value);
                    ImVec2 text_pos = ImVec2(
                        box_pos.x + (box_size.x - text_size.x) * 0.5f,
                        box_pos.y + (box_size.y - text_size.y) * 0.5f
                    );
                    draw_list->AddText(text_pos, IM_COL32(230, 255, 230, 255), value);
                }
                
                // 输出端口（在方框底部中心）
                ImVec2 port_world_pos = ImVec2(
                    box_pos.x + box_size.x * 0.5f,
                    box_pos.y + box_size.y + 8 * zoom_level
                );
                
                // 保存相对于节点的端口位置
                ImVec2 port_relative = ImVec2(
                    port_world_pos.x - node_pos.x,
                    port_world_pos.y - node_pos.y
                );
                info.output_ports.push_back(port_relative);
                
                // 绘制输出端口
                draw_list->AddCircleFilled(port_world_pos, 5.0f * zoom_level, IM_COL32(90, 190, 90, 255));
                draw_list->AddCircle(port_world_pos, 5.0f * zoom_level, IM_COL32(140, 240, 140, 255), 12, 1.8f * zoom_level);
            }
        }
        
        // 节点拖动检测
        ImVec2 mouse_pos = ImGui::GetMousePos();
        bool is_hovered = (mouse_pos.x >= node_pos.x && mouse_pos.x <= node_pos.x + node_width &&
                          mouse_pos.y >= node_pos.y && mouse_pos.y <= node_pos.y + title_height);
        
        if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            selected_node = node;
            dragging_node = node;
            drag_offset = ImVec2(mouse_pos.x - node_pos.x, mouse_pos.y - node_pos.y);
        }
        
        return info;
    }
    
    void HandleNodeDragging() {
        if (dragging_node && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            
            dragging_node->position.x = (mouse_pos.x - canvas_pos.x - drag_offset.x - canvas_offset.x) / zoom_level;
            dragging_node->position.y = (mouse_pos.y - canvas_pos.y - drag_offset.y - canvas_offset.y) / zoom_level;
        }
        
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            dragging_node = nullptr;
        }
    }
    
    void DrawAllConnections() {
        if (!root) return;
        DrawConnectionsRecursive(root);
    }
    
    void DrawConnectionsRecursive(PointerChainNode* node) {
        if (!node || node->collapsed) return;  // 如果节点折叠，不绘制子节点连接
        
        auto it = render_infos.find(node);
        if (it == render_infos.end()) return;
        
        NodeRenderInfo& info = it->second;
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        
        // 为每个子节点绘制连接
        for (size_t i = 0; i < node->children.size(); i++) {
            PointerChainNode* child = node->children[i];
            auto child_it = render_infos.find(child);
            if (child_it == render_infos.end()) continue;
            
            // 获取输出端口位置（世界坐标）
            ImVec2 output_pos;
            if (i < info.output_ports.size()) {
                // 使用第 i 个输出端口
                output_pos = ImVec2(
                    canvas_pos.x + (node->position.x * zoom_level) + info.output_ports[i].x + canvas_offset.x,
                    canvas_pos.y + (node->position.y * zoom_level) + info.output_ports[i].y + canvas_offset.y
                );
            } else {
                // 如果端口不够，使用节点底部中心（根据节点宽度调整）
                float node_center = (compact_mode ? 90.0f : 110.0f) * zoom_level;
                output_pos = ImVec2(
                    canvas_pos.x + (node->position.x * zoom_level) + node_center + canvas_offset.x,
                    canvas_pos.y + (node->position.y * zoom_level) + info.size.y + canvas_offset.y
                );
            }
            
            // 获取输入端口位置（世界坐标）
            ImVec2 input_pos = ImVec2(
                canvas_pos.x + (child->position.x * zoom_level) + child_it->second.input_port.x + canvas_offset.x,
                canvas_pos.y + (child->position.y * zoom_level) + child_it->second.input_port.y + canvas_offset.y
            );
            
            // 绘制从第 i 个输出端口到第 i 个子节点的连接
            DrawBezierConnection(output_pos, input_pos, i);
        }
        
        // 递归绘制子节点的连接
        for (auto child : node->children) {
            DrawConnectionsRecursive(child);
        }
    }
    
    void DrawBezierConnection(ImVec2 start, ImVec2 end, int connection_index = 0) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        
        // 计算垂直和水平距离
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        
        // 树形布局优化的控制点
        // 让连接线先向下，再水平移动，最后指向目标
        float vertical_offset = std::max(dy * 0.4f, 50.0f * zoom_level);
        
        ImVec2 cp1 = ImVec2(start.x, start.y + vertical_offset);
        ImVec2 cp2 = ImVec2(end.x, end.y - vertical_offset);
        
        // 连接线颜色（可以根据 connection_index 变化）
        ImU32 line_color = IM_COL32(80, 180, 140, 230);
        ImU32 highlight_color = IM_COL32(140, 240, 190, 180);
        ImU32 shadow_color = IM_COL32(0, 0, 0, 100);
        
        // 绘制阴影
        float shadow_offset = 2.0f * zoom_level;
        draw_list->AddBezierCubic(
            ImVec2(start.x + shadow_offset, start.y + shadow_offset), 
            ImVec2(cp1.x + shadow_offset, cp1.y + shadow_offset), 
            ImVec2(cp2.x + shadow_offset, cp2.y + shadow_offset), 
            ImVec2(end.x + shadow_offset, end.y + shadow_offset),
            shadow_color,
            4.5f * zoom_level,
            32
        );
        
        // 绘制主曲线
        draw_list->AddBezierCubic(
            start, cp1, cp2, end,
            line_color,
            3.8f * zoom_level,
            32
        );
        
        // 绘制高光
        draw_list->AddBezierCubic(
            start, cp1, cp2, end,
            highlight_color,
            2.0f * zoom_level,
            32
        );
        
        // 绘制箭头
        ImVec2 direction = ImVec2(end.x - cp2.x, end.y - cp2.y);
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0.01f) {
            direction.x /= length;
            direction.y /= length;
            
            ImVec2 perp = ImVec2(-direction.y, direction.x);
            
            float arrow_size = 14.0f * zoom_level;
            float arrow_width = 7.0f * zoom_level;
            ImVec2 arrow_p1 = ImVec2(end.x - direction.x * arrow_size + perp.x * arrow_width, 
                                     end.y - direction.y * arrow_size + perp.y * arrow_width);
            ImVec2 arrow_p2 = ImVec2(end.x - direction.x * arrow_size - perp.x * arrow_width, 
                                     end.y - direction.y * arrow_size - perp.y * arrow_width);
            
            draw_list->AddTriangleFilled(end, arrow_p1, arrow_p2, line_color);
        }
    }
}; 