-- 自动扫描示例脚本
-- 使用方法：在脚本管理器中执行此脚本

log("=== 自动扫描脚本开始 ===")

-- 检查是否已附加进程
local pid = process.getCurrent()
if pid == 0 then
    log("错误: 请先附加进程")
    return
end

log("当前进程ID: " .. pid)

-- 要搜索的值
local targetValue = 100
log("搜索值: " .. targetValue)

-- 首次扫描
log("开始首次扫描...")
-- 注意：这里需要实现完整的扫描API
-- local count = scan.first(targetValue, "dword", "exact")
-- log("首次扫描找到 " .. count .. " 个结果")

log("=== 脚本执行完成 ===")

