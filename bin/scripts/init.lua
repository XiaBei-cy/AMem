-- Lua脚本初始化文件
-- 这个文件会在Lua引擎初始化时自动执行

log("Lua脚本引擎已初始化")
log("LuaJIT版本: " .. tostring(jit and jit.version or "unknown"))

-- 示例：列出当前进程
log("正在获取进程列表...")
local processes = process.list()
if processes then
    log("找到 " .. #processes .. " 个进程")
    for i = 1, math.min(5, #processes) do
        log(string.format("  [%d] PID: %d, 名称: %s", i, processes[i].pid, processes[i].name))
    end
else
    log("获取进程列表失败（可能未连接服务器）")
end

