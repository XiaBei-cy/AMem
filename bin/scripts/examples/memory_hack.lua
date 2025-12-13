-- 内存修改示例脚本
-- 演示如何读取和修改内存

log("=== 内存修改示例 ===")

-- 示例地址（需要根据实际情况修改）
local address = 0x12345678

log("读取地址: 0x" .. string.format("%X", address))

-- 读取整数
local value = mem.readInt(address)
if value then
    log("当前值: " .. value)
    
    -- 修改值
    log("修改为: 9999")
    mem.writeInt(address, 9999)
    
    -- 验证
    local newValue = mem.readInt(address)
    if newValue then
        log("验证值: " .. newValue)
    end
else
    log("读取失败（可能地址无效或未连接）")
end

log("=== 脚本执行完成 ===")

