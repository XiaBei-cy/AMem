#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "client.hpp"

class WindowsSocketClient;

WindowsSocketClient& GetSocketClient();

struct ServerVersionInfo {
	int version = 0;
	std::string versionString;
};

struct ProcessInfoItem {
	int pid = 0;
	std::string name;
};

struct ModuleInfoItem {
	uint64_t base = 0;
	int type;//模块类型
    int flag;//模块读写标志位
	int size = 0;
	std::string name;
};

enum MemType {
    MemType_Null = 0,
    MemType_IO = 1,
    MemType_Syscall = 2,
    MemType_Kernel = 3,
    MemType_SysHook = 4
};


bool FetchServerVersion(ServerVersionInfo& outInfo);
bool GetMemType(int& outType);
bool InitDriver(std::string& Card,std::string& resStr);

bool FetchProcessList(std::vector<ProcessInfoItem>& outList);

// Process / Module helpers
void SetCurrentPid(int pid);
int GetCurrentPid();
bool OpenProcessHandle(int pid, int& outHandle);
bool EnsureOpenHandle(int& outHandle);
bool FetchModuleList(std::vector<ModuleInfoItem>& outList);

// Memory helpers
bool ReadProcessMemoryBytes(uint64_t address, uint32_t size, std::vector<unsigned char>& out);
bool ReadProcessMemory_(uint64_t address, uint32_t size,void* out,int32_t& Realread);
bool ReadBratchMemory(uint64_t address, uint32_t size, std::vector<std::pair<uint64_t, std::vector<uint8_t>>>& out);

bool ReadBratchAddr( std::vector<std::pair<uint64_t, int32_t>/*addr,size*/>& addrs,
	 std::vector<std::pair<uint64_t, std::vector<uint8_t>>/*addr,data*/>& out);


bool WriteProcessMemoryBytes(uint64_t address, uint32_t size, std::vector<unsigned char>& data);

// Resolve helpers
bool GetModuleBaseByName(const std::string& moduleName, uint64_t& outBase);
bool ResolveModuleOffsetChain(uint64_t& outAddress, const std::string& moduleName, uint64_t baseOffset, const std::vector<uint64_t>& offsets, bool derefFinal = true); 

// Scan helpers
bool ScanSetRange(int type);
int ScanValue(uint32_t flags, std::vector<unsigned char>& Value, uint64_t start = 0, uint64_t end = UINT64_MAX);
int ScanNextValue(std::vector<unsigned char>& Value, int flag, uint64_t start = 0, uint64_t end = UINT64_MAX);
int GetScanResultCount();
bool GetScanResult(int offset, int count, std::vector<std::pair<uint64_t, uint64_t>>& results);
bool RemoveScanResult(std::vector<uint64_t> address);
bool ClearScanResult();

// 带进度回调的扫描函数
typedef void(*ScanProgressCallback)(float progress, uint64_t matchCount, uint64_t scannedBytes, uint64_t totalBytes, void* userData);
int ScanValueWithProgress(uint32_t flags, std::vector<unsigned char>& Value, ScanProgressCallback callback, void* userData, uint64_t start = 0, uint64_t end = UINT64_MAX);
int ScanNextValueWithProgress(std::vector<unsigned char>& Value, int flag, ScanProgressCallback callback, void* userData, uint64_t start = 0, uint64_t end = UINT64_MAX); 
int ScanFuzzyValueWithProgress(uint32_t flags, ScanProgressCallback callback, void* userData, uint64_t start = 0, uint64_t end = UINT64_MAX);


//内核断点相关
bool SetKernelBreakpoint(uint64_t address, uint32_t bpType, uint32_t bpSize);
bool RemoveKernelBreakpoint(uint64_t address);
bool SuspendKernelBreakpoint(uint64_t address);
bool ResumeKernelBreakpoint(uint64_t address);
bool ReadKernelBreakpointInfo(uint64_t address, std::vector<HW_HIT_INFO>& infos);