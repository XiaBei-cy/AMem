#pragma once

#include <cstdint>
#include <string>

#ifdef HAVE_LUAJIT
// LuaJIT使用lua.hpp
extern "C" {
#include "lua.hpp"
}

#endif

/**
 * Lua API绑定
 * 将所有C++ API暴露给Lua脚本
 */
class LuaAPI {
public:
    // 注册所有API到Lua状态机
    static void RegisterAll(lua_State* L);

    // ==================== 内存操作API ====================
    static int ReadMemory(lua_State* L);
    static int WriteMemory(lua_State* L);
    static int ReadMemoryBatch(lua_State* L);
    static int ReadInt(lua_State* L);
    static int ReadLong(lua_State* L);
    static int ReadShort(lua_State* L);
    static int ReadByte(lua_State* L);
    static int WriteInt(lua_State* L);
    static int WriteLong(lua_State* L);
    static int WriteShort(lua_State* L);
    static int WriteByte(lua_State* L);
    static int ReadFloat(lua_State* L);
    static int ReadDouble(lua_State* L);
    static int WriteFloat(lua_State* L);
    static int WriteDouble(lua_State* L);
    static int ReadString(lua_State* L);
    static int WriteString(lua_State* L);

    // ==================== 进程和模块API ====================
    static int GetProcessList(lua_State* L);
    static int AttachProcess(lua_State* L);
    static int GetCurrentPid(lua_State* L);
    static int GetModuleList(lua_State* L);
    static int GetModuleBase(lua_State* L);
    static int ResolveOffsetChain(lua_State* L);

    // ==================== 内存扫描API ====================
    static int ScanFirst(lua_State* L);
    static int ScanNext(lua_State* L);
    static int GetScanResults(lua_State* L);
    static int ClearScanResults(lua_State* L);
    static int ScanFuzzy(lua_State* L);

    // ==================== 断点API ====================
    static int SetBreakpoint(lua_State* L);
    static int RemoveBreakpoint(lua_State* L);
    static int SuspendBreakpoint(lua_State* L);
    static int ResumeBreakpoint(lua_State* L);
    static int GetBreakpointInfo(lua_State* L);

    // ==================== 工具API ====================
    static int Log(lua_State* L);
    static int Sleep(lua_State* L);
    static int GetTime(lua_State* L);

private:
    // 辅助函数
    static uint64_t CheckAddress(lua_State* L, int index);
    static void PushError(lua_State* L, const std::string& msg);
};

