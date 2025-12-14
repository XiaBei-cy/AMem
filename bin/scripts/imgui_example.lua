-- ImGui 示例脚本
-- 展示如何使用 Lua 创建和管理 ImGui 窗口

-- 全局变量
local windowId = nil
local counter = 0
local textInput = "Hello, ImGui!"
local sliderValue = 50.0
local checkboxValue = false
local selectedItem = 1
local items = {"选项1", "选项2", "选项3", "选项4"}

-- 窗口绘制回调函数
function drawMyWindow(windowId)
    -- 注意：窗口已经由系统自动管理（自动调用 Begin/End）
    -- 这里只需要绘制窗口内容即可
    
    imgui.text("这是一个由 Lua 创建的 ImGui 窗口！")
    imgui.separator()
    
    -- 显示计数器
    imgui.text("计数器: " .. counter)
    if imgui.button("增加") then
        counter = counter + 1
    end
    imgui.sameLine()
    if imgui.button("减少") then
        counter = counter - 1
    end
    imgui.sameLine()
    if imgui.button("重置") then
        counter = 0
    end
    
    imgui.separator()
    
    -- 文本输入
    local result, changed = imgui.inputText("文本输入", textInput)
    if changed then
        textInput = result
    end
    
    -- 滑块
    local newSliderValue, sliderChanged = imgui.sliderFloat("滑块", sliderValue, 0.0, 100.0)
    if sliderChanged then
        sliderValue = newSliderValue
    end
    
    -- 复选框
    local checkboxTable = {value = checkboxValue}
    local checkboxChanged = imgui.checkbox("启用选项", checkboxTable)
    if checkboxChanged then
        checkboxValue = checkboxTable.value
    end
    
    imgui.separator()
    
    -- 树形节点
    if imgui.treeNode("树形节点示例") then
        imgui.text("这是树形节点的内容")
        imgui.text("可以嵌套更多内容")
        if imgui.treeNode("子节点") then
            imgui.text("子节点内容")
            imgui.treePop()
        end
        imgui.treePop()
    end
    
    -- 折叠标题
    if imgui.collapsingHeader("折叠内容") then
        imgui.text("这是折叠区域的内容")
        imgui.text("可以放置任何内容")
    end
    
    imgui.separator()
    
    -- 列表选择
    local newSelected, listChanged = imgui.listBox("选择项", selectedItem, items)
    if listChanged then
        selectedItem = newSelected
    end
    
    imgui.text("当前选择: " .. items[selectedItem])
    
    imgui.separator()
    
    -- 表格示例
    if imgui.beginTable("数据表", 3) then
        -- 表头
        imgui.tableNextRow()
        imgui.tableNextColumn()
        imgui.text("列1")
        imgui.tableNextColumn()
        imgui.text("列2")
        imgui.tableNextColumn()
        imgui.text("列3")
        
        -- 数据行
        for i = 1, 5 do
            imgui.tableNextRow()
            imgui.tableNextColumn()
            imgui.text("行" .. i .. " 列1")
            imgui.tableNextColumn()
            imgui.text("行" .. i .. " 列2")
            imgui.tableNextColumn()
            imgui.text("行" .. i .. " 列3")
        end
        
        imgui.endTable()
    end
    
    imgui.separator()
    
    -- 带颜色的文本
    imgui.textColored({1.0, 0.0, 0.0, 1.0}, "红色文本")
    imgui.textColored({0.0, 1.0, 0.0, 1.0}, "绿色文本")
    imgui.textColored({0.0, 0.0, 1.0, 1.0}, "蓝色文本")
    
    -- 布局示例
    imgui.columns(2, "布局列", true)
    imgui.setColumnWidth(0, 200)
    imgui.text("左侧列")
    imgui.nextColumn()
    imgui.text("右侧列")
    imgui.columns(1)
end

-- 主函数
function main()
    log("ImGui 示例脚本启动")
    
    -- 创建窗口
    windowId = imgui.createWindow("Lua ImGui 示例窗口", "drawMyWindow")
    log("窗口已创建，ID: " .. windowId)
    
    -- 检查窗口是否打开
    if imgui.isWindowOpen(windowId) then
        log("窗口已打开")
    end
end

-- 运行主函数
main()

