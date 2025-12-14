#include "LuaAPI_Memory.h"
#include "LuaAPI.h"
#include "../socket/client_singleton.h"
#include <string>
#include <vector>
#include <cstring>

// ==================== 注册内存操作API ====================
void LuaAPI_Memory::Register(lua_State* L) {
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
}

// ==================== 内存操作API实现 ====================
int LuaAPI_Memory::ReadMemory(lua_State* L) {
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

int LuaAPI_Memory::WriteMemory(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    
    std::vector<unsigned char> data;
    if (lua_istable(L, 2)) {
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

int LuaAPI_Memory::ReadInt(lua_State* L) {
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

int LuaAPI_Memory::WriteInt(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    int32_t value = static_cast<int32_t>(luaL_checkinteger(L, 2));
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(&value),
                                   reinterpret_cast<const unsigned char*>(&value) + 4);
    bool success = WriteProcessMemoryBytes(address, 4, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI_Memory::ReadLong(lua_State* L) {
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

int LuaAPI_Memory::WriteLong(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    int64_t value = static_cast<int64_t>(luaL_checknumber(L, 2));
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(&value),
                                   reinterpret_cast<const unsigned char*>(&value) + 8);
    bool success = WriteProcessMemoryBytes(address, 8, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI_Memory::ReadShort(lua_State* L) {
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

int LuaAPI_Memory::WriteShort(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    int16_t value = static_cast<int16_t>(luaL_checkinteger(L, 2));
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(&value),
                                   reinterpret_cast<const unsigned char*>(&value) + 2);
    bool success = WriteProcessMemoryBytes(address, 2, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI_Memory::ReadByte(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    std::vector<unsigned char> data;
    if (ReadProcessMemoryBytes(address, 1, data) && data.size() == 1) {
        lua_pushinteger(L, data[0]);
        return 1;
    }
    LuaAPI::PushError(L, "Failed to read byte");
    return 2;
}

int LuaAPI_Memory::WriteByte(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    unsigned char value = static_cast<unsigned char>(luaL_checkinteger(L, 2));
    std::vector<unsigned char> data = {value};
    bool success = WriteProcessMemoryBytes(address, 1, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI_Memory::ReadFloat(lua_State* L) {
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

int LuaAPI_Memory::WriteFloat(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    float value = static_cast<float>(luaL_checknumber(L, 2));
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(&value),
                                   reinterpret_cast<const unsigned char*>(&value) + 4);
    bool success = WriteProcessMemoryBytes(address, 4, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI_Memory::ReadDouble(lua_State* L) {
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

int LuaAPI_Memory::WriteDouble(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    double value = luaL_checknumber(L, 2);
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(&value),
                                   reinterpret_cast<const unsigned char*>(&value) + 8);
    bool success = WriteProcessMemoryBytes(address, 8, data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI_Memory::ReadString(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    uint32_t maxLength = static_cast<uint32_t>(luaL_optinteger(L, 2, 256));
    const char* encoding = luaL_optstring(L, 3, "ascii");

    std::vector<unsigned char> data;
    if (ReadProcessMemoryBytes(address, maxLength, data)) {
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

int LuaAPI_Memory::WriteString(lua_State* L) {
    uint64_t address = LuaAPI::CheckAddress(L, 1);
    const char* str = luaL_checkstring(L, 2);
    std::vector<unsigned char> data(reinterpret_cast<const unsigned char*>(str),
                                   reinterpret_cast<const unsigned char*>(str) + strlen(str) + 1);
    bool success = WriteProcessMemoryBytes(address, static_cast<uint32_t>(data.size()), data);
    lua_pushboolean(L, success ? 1 : 0);
    return 1;
}

int LuaAPI_Memory::ReadMemoryBatch(lua_State* L) {
    luaL_error(L, "Not implemented yet");
    return 0;
}

