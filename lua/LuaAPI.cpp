#include "LuaAPI.h"
#include "../socket/client_singleton.h"
#include "../gui/Gui.h"
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <sstream>

// 辅助函数：将 Lua 值转换为字符串（Lua 5.1 兼容版本）
static const char* luaL_tolstring_compat(lua_State* L, int idx, size_t* len) {
    // Lua 5.1 兼容：手动计算绝对索引
    int absidx = (idx < 0) ? lua_gettop(L) + idx + 1 : idx;
    int type = lua_type(L, absidx);
    
    switch (type) {
        case LUA_TSTRING:
            return lua_tolstring(L, absidx, len);
        case LUA_TNUMBER: {
            lua_Number num = lua_tonumber(L, absidx);
            std::ostringstream oss;
            oss << num;
            std::string str = oss.str();
            lua_pushlstring(L, str.c_str(), str.length());
            if (len) *len = str.length();
            return lua_tostring(L, -1);
        }
        case LUA_TBOOLEAN: {
            int b = lua_toboolean(L, absidx);
            const char* str = b ? "true" : "false";
            lua_pushstring(L, str);
            if (len) *len = strlen(str);
            return str;
        }
        case LUA_TNIL: {
            const char* str = "nil";
            lua_pushstring(L, str);
            if (len) *len = strlen(str);
            return str;
        }
        case LUA_TTABLE: {
            const char* str = "table";
            lua_pushstring(L, str);
            if (len) *len = strlen(str);
            return str;
        }
        case LUA_TFUNCTION: {
            const char* str = "function";
            lua_pushstring(L, str);
            if (len) *len = strlen(str);
            return str;
        }
        case LUA_TUSERDATA: {
            const char* str = "userdata";
            lua_pushstring(L, str);
            if (len) *len = strlen(str);
            return str;
        }
        case LUA_TTHREAD: {
            const char* str = "thread";
            lua_pushstring(L, str);
            if (len) *len = strlen(str);
            return str;
        }
        case LUA_TLIGHTUSERDATA: {
            const char* str = "lightuserdata";
            lua_pushstring(L, str);
            if (len) *len = strlen(str);
            return str;
        }
        default: {
            const char* str = lua_typename(L, type);
            lua_pushstring(L, str);
            if (len) *len = strlen(str);
            return str;
        }
    }
}

// 辅助函数：检查地址参数
uint64_t LuaAPI::CheckAddress(lua_State* L, int index) {
    if (lua_isnumber(L, index)) {
        return static_cast<uint64_t>(lua_tonumber(L, index));
    } else if (lua_isstring(L, index)) {
        // 支持十六进制字符串 "0x12345678"
        const char* str = lua_tostring(L, index);
        return std::strtoull(str, nullptr, 0);
    }
    luaL_error(L, "Expected number or hex string for address");
    return 0;
}

void LuaAPI::PushError(lua_State* L, const std::string& msg) {
    lua_pushnil(L);
    lua_pushstring(L, msg.c_str());
}

// ==================== 注册所有API ====================
void LuaAPI::RegisterAll(lua_State* L) {
    // 创建mem表
    lua_newtable(L);
    lua_pushcfunction(L, ReadMemory);
    lua_setfield(L, -2, "read");
    lua_pushcfunction(L, WriteMemory);
    lua_setfield(L, -2, "write");
    lua_pushcfunction(L, ReadMemoryBatch);
    lua_setfield(L, -2, "readBatch");
    lua_pushcfunction(L, ReadInt);
    lua_setfield(L, -2, "readInt");
    lua_pushcfunction(L, ReadLong);
    lua_setfield(L, -2, "readLong");
    lua_pushcfunction(L, ReadShort);
    lua_setfield(L, -2, "readShort");
    lua_pushcfunction(L, ReadByte);
    lua_setfield(L, -2, "readByte");
    lua_pushcfunction(L, WriteInt);
    lua_setfield(L, -2, "writeInt");
    lua_pushcfunction(L, WriteLong);
    lua_setfield(L, -2, "writeLong");
    lua_pushcfunction(L, WriteShort);
    lua_setfield(L, -2, "writeShort");
    lua_pushcfunction(L, WriteByte);
    lua_setfield(L, -2, "writeByte");
    lua_pushcfunction(L, ReadFloat);
    lua_setfield(L, -2, "readFloat");
    lua_pushcfunction(L, ReadDouble);
    lua_setfield(L, -2, "readDouble");
    lua_pushcfunction(L, WriteFloat);
    lua_setfield(L, -2, "writeFloat");
    lua_pushcfunction(L, WriteDouble);
    lua_setfield(L, -2, "writeDouble");
    lua_pushcfunction(L, ReadString);
    lua_setfield(L, -2, "readString");
    lua_pushcfunction(L, WriteString);
    lua_setfield(L, -2, "writeString");
    lua_setglobal(L, "mem");

    // 创建process表
    lua_newtable(L);
    lua_pushcfunction(L, GetProcessList);
    lua_setfield(L, -2, "list");
    lua_pushcfunction(L, AttachProcess);
    lua_setfield(L, -2, "attach");
    lua_pushcfunction(L, GetCurrentPid);
    lua_setfield(L, -2, "getCurrent");
    lua_setglobal(L, "process");

    // 创建module表
    lua_newtable(L);
    lua_pushcfunction(L, GetModuleList);
    lua_setfield(L, -2, "list");
    lua_pushcfunction(L, GetModuleBase);
    lua_setfield(L, -2, "getBase");
    lua_pushcfunction(L, ResolveOffsetChain);
    lua_setfield(L, -2, "resolveOffsetChain");
    lua_setglobal(L, "module");

    // 创建scan表
    lua_newtable(L);
    lua_pushcfunction(L, ScanFirst);
    lua_setfield(L, -2, "first");
    lua_pushcfunction(L, ScanNext);
    lua_setfield(L, -2, "next");
    lua_pushcfunction(L, GetScanResults);
    lua_setfield(L, -2, "getResults");
    lua_pushcfunction(L, ClearScanResults);
    lua_setfield(L, -2, "clear");
    lua_pushcfunction(L, ScanFuzzy);
    lua_setfield(L, -2, "fuzzy");
    lua_setglobal(L, "scan");

    // 创建bp表
    lua_newtable(L);
    lua_pushcfunction(L, SetBreakpoint);
    lua_setfield(L, -2, "set");
    lua_pushcfunction(L, RemoveBreakpoint);
    lua_setfield(L, -2, "remove");
    lua_pushcfunction(L, SuspendBreakpoint);
    lua_setfield(L, -2, "suspend");
    lua_pushcfunction(L, ResumeBreakpoint);
    lua_setfield(L, -2, "resume");
    lua_pushcfunction(L, GetBreakpointInfo);
    lua_setfield(L, -2, "getInfo");
    lua_setglobal(L, "bp");

    // 全局函数
    lua_pushcfunction(L, Log);
    lua_setglobal(L, "log");
    lua_pushcfunction(L, Sleep);
    lua_setglobal(L, "sleep");
    lua_pushcfunction(L, GetTime);
    lua_setglobal(L, "time");
}

// ==================== 内存操作API实现 ====================
int LuaAPI::ReadMemory(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    uint32_t size = static_cast<uint32_t>(luaL_checkinteger(L, 2));

    std::vector<unsigned char> data;
    if (ReadProcessMemoryBytes(address, size, data)) {
        lua_newtable(L);
        for (size_t i = 0; i < data.size(); ++i) {
            lua_pushinteger(L, i + 1);
            lua_pushinteger(L, data[i]);
            lua_settable(L, -3);
        }
        return 1;
    }
    LuaAPI::PushError(L, "Failed to read memory");
    return 2;
}

int LuaAPI::WriteMemory(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    
    std::vector<unsigned char> data;
    if (lua_istable(L, 2)) {
        // 从table读取字节数组
        lua_objlen(L, 2);
        int len = lua_tointeger(L, -1);
        lua_pop(L, 1);
        data.resize(len);
        for (int i = 0; i < len; ++i) {
            lua_pushinteger(L, i + 1);
            lua_gettable(L, 2);
            data[i] = static_cast<unsigned char>(lua_tointeger(L, -1));
            lua_pop(L, 1);
        }
    } else if (lua_isstring(L, 2)) {
        // 从字符串读取
        size_t len;
        const char* str = lua_tolstring(L, 2, &len);
        data.assign(reinterpret_cast<const unsigned char*>(str), 
                   reinterpret_cast<const unsigned char*>(str) + len);
    } else {
        luaL_error(L, "Expected table or string for data");
    }

    bool success = WriteProcessMemoryBytes(address, static_cast<uint32_t>(data.size()), data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::ReadInt(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    std::vector<unsigned char> data;
    if (ReadProcessMemoryBytes(address, 4, data) && data.size() == 4) {
        int32_t value = *reinterpret_cast<int32_t*>(data.data());
        lua_pushinteger(L, value);
        return 1;
    }
    LuaAPI::PushError(L, "Failed to read int");
    return 2;
}

int LuaAPI::WriteInt(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    int32_t value = static_cast<int32_t>(luaL_checkinteger(L, 2));
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(&value),
                                   reinterpret_cast<const unsigned char*>(&value) + 4);
    bool success = WriteProcessMemoryBytes(address, 4, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::ReadLong(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    std::vector<unsigned char> data;
    if (ReadProcessMemoryBytes(address, 8, data) && data.size() == 8) {
        int64_t value = *reinterpret_cast<int64_t*>(data.data());
        lua_pushnumber(L, static_cast<lua_Number>(value));
        return 1;
    }
    LuaAPI::PushError(L, "Failed to read long");
    return 2;
}

int LuaAPI::WriteLong(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    int64_t value = static_cast<int64_t>(luaL_checknumber(L, 2));
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(&value),
                                   reinterpret_cast<const unsigned char*>(&value) + 8);
    bool success = WriteProcessMemoryBytes(address, 8, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::ReadShort(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    std::vector<unsigned char> data;
    if (ReadProcessMemoryBytes(address, 2, data) && data.size() == 2) {
        int16_t value = *reinterpret_cast<int16_t*>(data.data());
        lua_pushinteger(L, value);
        return 1;
    }
    LuaAPI::PushError(L, "Failed to read short");
    return 2;
}

int LuaAPI::WriteShort(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    int16_t value = static_cast<int16_t>(luaL_checkinteger(L, 2));
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(&value),
                                   reinterpret_cast<const unsigned char*>(&value) + 2);
    bool success = WriteProcessMemoryBytes(address, 2, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::ReadByte(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    std::vector<unsigned char> data;
    if (ReadProcessMemoryBytes(address, 1, data) && data.size() == 1) {
        lua_pushinteger(L, data[0]);
        return 1;
    }
    LuaAPI::PushError(L, "Failed to read byte");
    return 2;
}

int LuaAPI::WriteByte(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    unsigned char value = static_cast<unsigned char>(luaL_checkinteger(L, 2));
    std::vector<unsigned char> data = {value};
    bool success = WriteProcessMemoryBytes(address, 1, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::ReadFloat(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    std::vector<unsigned char> data;
    if (ReadProcessMemoryBytes(address, 4, data) && data.size() == 4) {
        float value = *reinterpret_cast<float*>(data.data());
        lua_pushnumber(L, value);
        return 1;
    }
    LuaAPI::PushError(L, "Failed to read float");
    return 2;
}

int LuaAPI::WriteFloat(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    float value = static_cast<float>(luaL_checknumber(L, 2));
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(&value),
                                   reinterpret_cast<const unsigned char*>(&value) + 4);
    bool success = WriteProcessMemoryBytes(address, 4, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::ReadDouble(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    std::vector<unsigned char> data;
    if (ReadProcessMemoryBytes(address, 8, data) && data.size() == 8) {
        double value = *reinterpret_cast<double*>(data.data());
        lua_pushnumber(L, value);
        return 1;
    }
    LuaAPI::PushError(L, "Failed to read double");
    return 2;
}

int LuaAPI::WriteDouble(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    double value = luaL_checknumber(L, 2);
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(&value),
                                   reinterpret_cast<const unsigned char*>(&value) + 8);
    bool success = WriteProcessMemoryBytes(address, 8, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::ReadString(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    uint32_t maxLength = static_cast<uint32_t>(luaL_optinteger(L, 2, 256));
    const char* encoding = luaL_optstring(L, 3, "ascii");

    std::vector<unsigned char> data;
    if (ReadProcessMemoryBytes(address, maxLength, data)) {
        // 查找字符串结束符
        size_t len = 0;
        while (len < data.size() && data[len] != 0) {
            ++len;
        }
        std::string str(reinterpret_cast<const char*>(data.data()), len);
        lua_pushstring(L, str.c_str());
        return 1;
    }
    LuaAPI::PushError(L, "Failed to read string");
    return 2;
}

int LuaAPI::WriteString(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    const char* str = luaL_checkstring(L, 2);
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(str),
                                   reinterpret_cast<const unsigned char*>(str) + strlen(str) + 1);
    bool success = WriteProcessMemoryBytes(address, static_cast<uint32_t>(data.size()), data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::ReadMemoryBatch(lua_State* L) {
    // 实现批量读取
    luaL_error(L, "Not implemented yet");
    return 0;
}

// ==================== 进程和模块API实现 ====================
int LuaAPI::GetProcessList(lua_State* L) {
    std::vector<ProcessInfoItem> processes;
    if (FetchProcessList(processes)) {
        lua_newtable(L);
        for (size_t i = 0; i < processes.size(); ++i) {
            lua_pushinteger(L, i + 1);
            lua_newtable(L);
            lua_pushinteger(L, processes[i].pid);
            lua_setfield(L, -2, "pid");
            lua_pushstring(L, processes[i].name.c_str());
            lua_setfield(L, -2, "name");
            lua_settable(L, -3);
        }
        return 1;
    }
    LuaAPI::PushError(L, "Failed to get process list");
    return 2;
}

int LuaAPI::AttachProcess(lua_State* L) {
    int pid = static_cast<int>(luaL_checkinteger(L, 1));
    int handle;
    if (OpenProcessHandle(pid, handle)) {
        SetCurrentPid(pid);
        lua_pushboolean(L, 1);
        return 1;
    }
    LuaAPI::PushError(L, "Failed to attach process");
    return 2;
}

int LuaAPI::GetCurrentPid(lua_State* L) {
    int pid = ::GetCurrentPid();
    lua_pushinteger(L, pid);
    return 1;
}

int LuaAPI::GetModuleList(lua_State* L) {
    std::vector<ModuleInfoItem> modules;
    if (FetchModuleList(modules)) {
        lua_newtable(L);
        for (size_t i = 0; i < modules.size(); ++i) {
            lua_pushinteger(L, i + 1);
            lua_newtable(L);
            lua_pushstring(L, modules[i].name.c_str());
            lua_setfield(L, -2, "name");
            lua_pushnumber(L, static_cast<lua_Number>(modules[i].base));
            lua_setfield(L, -2, "base");
            lua_pushinteger(L, modules[i].size);
            lua_setfield(L, -2, "size");
            lua_settable(L, -3);
        }
        return 1;
    }
    LuaAPI::PushError(L, "Failed to get module list");
    return 2;
}

int LuaAPI::GetModuleBase(lua_State* L) {
    const char* moduleName = luaL_checkstring(L, 1);
    uint64_t base;
    if (GetModuleBaseByName(moduleName, base)) {
        lua_pushnumber(L, static_cast<lua_Number>(base));
        return 1;
    }
    LuaAPI::PushError(L, "Module not found");
    return 2;
}

int LuaAPI::ResolveOffsetChain(lua_State* L) {
    const char* moduleName = luaL_checkstring(L, 1);
    if (!lua_istable(L, 2)) {
        luaL_error(L, "Expected table for offsets");
    }

    std::vector<uint64_t> offsets;
        lua_objlen(L, 2);
    int len = lua_tointeger(L, -1);
    lua_pop(L, 1);
    for (int i = 0; i < len; ++i) {
        lua_pushinteger(L, i + 1);
        lua_gettable(L, 2);
        offsets.push_back(static_cast<uint64_t>(lua_tonumber(L, -1)));
        lua_pop(L, 1);
    }

    uint64_t address;
    // ResolveModuleOffsetChain 参数: (outAddress, moduleName, baseOffset, offsets, derefFinal, type)
    // baseOffset 设为 0，derefFinal 设为 true（默认值）
    if (ResolveModuleOffsetChain(address, moduleName, 0, offsets, true, PORT_MAIN)) {
        lua_pushnumber(L, static_cast<lua_Number>(address));
        return 1;
    }
    LuaAPI::PushError(L, "Failed to resolve offset chain");
    return 2;
}

// ==================== 扫描API实现 ====================
int LuaAPI::ScanFirst(lua_State* L) {
    // 简化实现，实际需要更复杂的参数处理
    luaL_error(L, "Not fully implemented yet");
    return 0;
}

int LuaAPI::ScanNext(lua_State* L) {
    luaL_error(L, "Not fully implemented yet");
    return 0;
}

int LuaAPI::GetScanResults(lua_State* L) {
    int offset = static_cast<int>(luaL_optinteger(L, 1, 0));
    int count = static_cast<int>(luaL_optinteger(L, 2, 1000));
    
    std::vector<uint64_t> results;
    // 这里需要调用实际的扫描结果获取函数
    // 暂时返回空表
    lua_newtable(L);
    return 1;
}

int LuaAPI::ClearScanResults(lua_State* L) {
    bool success = ClearScanResult();
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::ScanFuzzy(lua_State* L) {
    luaL_error(L, "Not fully implemented yet");
    return 0;
}

// ==================== 断点API实现 ====================
int LuaAPI::SetBreakpoint(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    const char* bpType = luaL_checkstring(L, 2);
    uint32_t bpSize = static_cast<uint32_t>(luaL_optinteger(L, 3, 4));

    uint32_t typeFlag = 0;
    if (strcmp(bpType, "execute") == 0) typeFlag = 0;
    else if (strcmp(bpType, "write") == 0) typeFlag = 1;
    else if (strcmp(bpType, "read") == 0) typeFlag = 2;
    else if (strcmp(bpType, "access") == 0) typeFlag = 3;

    bool success = SetKernelBreakpoint(address, typeFlag, bpSize);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::RemoveBreakpoint(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    bool success = RemoveKernelBreakpoint(address);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::SuspendBreakpoint(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    bool success = SuspendKernelBreakpoint(address);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::ResumeBreakpoint(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    bool success = ResumeKernelBreakpoint(address);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI::GetBreakpointInfo(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    std::vector<HW_HIT_INFO> infos;
    if (ReadKernelBreakpointInfo(address, infos)) {
        lua_newtable(L);
        for (size_t i = 0; i < infos.size(); ++i) {
            lua_pushinteger(L, i + 1);
            lua_newtable(L);
            lua_pushnumber(L, static_cast<lua_Number>(infos[i].hit_addr));
            lua_setfield(L, -2, "addr");
            lua_pushnumber(L, static_cast<lua_Number>(infos[i].hit_time));
            lua_setfield(L, -2, "time");
            lua_settable(L, -3);
        }
        return 1;
    }
    LuaAPI::PushError(L, "Failed to get breakpoint info");
    return 2;
}

// ==================== 工具API实现 ====================
int LuaAPI::Log(lua_State* L) {
    int n = lua_gettop(L);
    std::string msg;
    for (int i = 1; i <= n; ++i) {
        if (i > 1) msg += " ";
        if (lua_isstring(L, i)) {
            msg += lua_tostring(L, i);
        } else {
            size_t len;
            const char* str = luaL_tolstring_compat(L, i, &len);
            msg += str;
            lua_pop(L, 1);  // 弹出转换后的字符串
        }
    }
    Gui::log("%s", msg.c_str());
    return 0;
}

int LuaAPI::Sleep(lua_State* L) {
    int milliseconds = static_cast<int>(luaL_checkinteger(L, 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    return 0;
}

int LuaAPI::GetTime(lua_State* L) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    lua_pushnumber(L, static_cast<lua_Number>(time));
    return 1;
}

