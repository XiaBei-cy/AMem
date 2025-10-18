#include "client.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <ctime>
#include "client_singleton.h"

static WindowsSocketClient g_client_singleton;
static int g_selected_pid = 0;
static int g_process_handle = 0;
static uint64_t g_selected_base = 0; // optional

WindowsSocketClient& GetSocketClient()
{
	return g_client_singleton;
}

bool GetMemType(int& outType){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	unsigned char command = CMD_GETARCHITECTURE;
	if (!client.Send(&command, sizeof(command))) return false;
	unsigned char type = 0;
	if (!client.Receive(&type, sizeof(type))) return false;
	outType = type;
	return true;
}


bool InitDriver(std::string& Card,std::string& resStr){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	unsigned char command = CMD_INITRWDRIVER;
	if (!client.Send(&command, sizeof(command))) return false;
	int Cardlen = Card.size();
	if (!client.Send(&Cardlen, sizeof(Cardlen))) return false;
	if (!client.Send(Card.data(), Card.size())) return false;

	int ret = 0;//0失败（str 错误信息） 1成功 2模块已加载 （都是时间戳）
	if (!client.Receive(&ret, sizeof(ret))) return false;
	int resStrlen = 0;
	if (!client.Receive(&resStrlen, sizeof(resStrlen))) return false;
	if (resStrlen == 0) return false;
	std::vector<char> resStrVec(resStrlen);
	if (!client.Receive(resStrVec.data(), resStrlen)) return false;

	if (ret > 0) {
		//格式化字符串中13位时间戳为日期时间
		try {
			// 确保字符串以null结尾
			std::string timestampStr(resStrVec.data(), resStrVec.size());
			uint64_t timestamp_ms = std::stoull(timestampStr);
			// 13位时间戳是毫秒级，需要转换为秒级
			time_t timestamp = static_cast<time_t>(timestamp_ms / 1000);
			
			// 检查时间戳是否在合理范围内（1970年开始）
			if (timestamp > 0) { 
				std::tm* timeinfo = std::localtime(&timestamp);
				if (timeinfo != nullptr) {
					char dateTime[20];
					std::strftime(dateTime, sizeof(dateTime), "%Y-%m-%d %H:%M:%S", timeinfo);
					resStr = dateTime;
				} else {
					resStr = "时间格式化失败";
				}
			} else {
				resStr = "无效的时间戳";
			}
		} catch (const std::exception& e) {
			resStr = "时间戳解析失败";
		}
	}
	else{
		resStr.assign(resStrVec.data(), resStrVec.size());
	}

	return true;
}



void SetCurrentPid(int pid)
{
	g_selected_pid = pid;
}

int GetCurrentPid()
{
	return g_selected_pid;
}

bool OpenProcessHandle(int pid, int& outHandle)
{
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	#pragma pack(1)
	struct { unsigned char command; int pid; } op;
	#pragma pack()
	op.command = CMD_OPENPROCESS;
	op.pid = pid;
	if (!client.Send(&op, sizeof(op))) return false;
	int handle = 0;
	if (!client.Receive(&handle, sizeof(handle))) return false;
	outHandle = handle;
	g_process_handle = handle;
	return true;
}

bool EnsureOpenHandle(int& outHandle)
{
	if (g_process_handle) { outHandle = g_process_handle; return true; }
	if (g_selected_pid == 0) return false;
	return OpenProcessHandle(g_selected_pid, outHandle);
}

bool FetchServerVersion(ServerVersionInfo& outInfo)
{
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected())
		return false;

	unsigned char command = CMD_GETVERSION;
	if (!client.Send(&command, sizeof(command)))
		return false;

	CeVersion version{};
	if (!client.Receive(&version, sizeof(version)))
		return false;
	outInfo.version = version.version;
	outInfo.versionString.clear();
	if (version.stringsize > 0) {
		std::vector<char> versionString(version.stringsize);
		if (client.Receive(versionString.data(), versionString.size())) {
			outInfo.versionString.assign(versionString.data(), versionString.size());
		}
	}
	return true;
}

bool FetchProcessList(std::vector<ProcessInfoItem>& outList)
{
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected())
		return false;

	unsigned char command = CMD_GETPROCESSLIST;
	if (!client.Send(&command, sizeof(command)))
		return false;

	int len = 0;
	if (!client.Receive(&len, 4))
		return false;

	outList.clear();
	while (len--) {
		struct { int pid; int size; } proc{};
		if (!client.Receive(&proc, sizeof(proc))) break;

		std::vector<char> name(proc.size);
		if (!client.Receive(name.data(), proc.size)) break;

		ProcessInfoItem item{};
		item.pid = proc.pid;
		item.name.assign(name.data(), name.size());
		outList.push_back(std::move(item));
	}
	return true;
}

bool FetchModuleList(std::vector<ModuleInfoItem>& outList)
{
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;

	unsigned char command = CMD_GETMODULELIST;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;

	int len = 0;
	if (!client.Receive(&len, 4)) return false;

	CeModuleListEntry entry{};
	outList.clear();
	int magic = 0x1145;
	while (len > 0) {
		if (!client.Receive(&magic, sizeof(magic))) return false;
		if (magic != 0x1145) return false;
		std::memset(&entry, 0, sizeof(entry));
		if (!client.Receive(&entry, sizeof(entry))) break;

		std::vector<char> name(entry.modulenamesize);
		if (!client.Receive(name.data(), entry.modulenamesize)) break;
		ModuleInfoItem mi{};
		mi.base = entry.modulebase;
		mi.size = entry.modulesize;
		mi.type = entry.result;
		mi.flag = entry.flag;
		mi.name="";//A类为空
		if (entry.modulenamesize > 0) {
			mi.name.assign(name.data(), entry.modulenamesize);
		}
		outList.push_back(std::move(mi));
		len--;
	}

	return true;
}


bool ScanSetRange(int type){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	
	unsigned char command = CMD_SETRANGE;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;
	if (!client.Send(&type, sizeof(type))) return false;
	
	return true;
}


int ScanValue(uint32_t flags,std::vector<unsigned char>& Value,uint64_t start,uint64_t end){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;

	unsigned char command = CMD_SCANVALUE;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;



		#pragma pack(1)
		struct {
			uint64_t start;
			uint64_t end;
			int size;
			unsigned int flag;
		} scanParams;
		#pragma pack()
	scanParams.start=start;
	scanParams.end=end;
	scanParams.size=Value.size();
	scanParams.flag=flags;
	client.Send(&scanParams,sizeof(scanParams));
	
	client.Send(Value.data(),Value.size());
	
	
	//进度回调
	while (true) {
      ScanProgress progress;
      if (!client.Receive(&progress, sizeof(progress))) {
        std::cerr << "接收进度失败" << std::endl;
        break;
      }
      if (progress.msgType == 1) {
        std::cout << "\r进度: " << std::fixed << std::setprecision(2)
                  << progress.percent << "%, 命中: " << progress.matchCount
                  << " scan " << progress.scannedBytes << " total "
                  << progress.totalBytes << std::flush;
      } else if (progress.msgType == 2) {
        std::cout << "\r进度: 100.00%, 命中: " << progress.matchCount
                  << " scan " << progress.scannedBytes << " total "
                  << progress.totalBytes << std::endl;
        break;
      } else if (progress.msgType == 3) {
        std::cerr << "扫描出错" << std::endl;
        break;
      }
    }
    
    int len;
    client.Receive(&len, 4);
    std::cout << "scan result size " << std::dec << len << std::endl;

	return len;

}


int ScanNextValue(std::vector<unsigned char>& Value,int flag,uint64_t start,uint64_t end){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	
	unsigned char command = CMD_SCANNEXTVALUE;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;

	      // 发送扫描参数
      #pragma pack(1)
      struct {
        uint64_t start;
        uint64_t end;
        int size;
        unsigned int flag;
      } scanParams;
      #pragma pack()

	scanParams.start=start;
	scanParams.end=end;
	scanParams.size=Value.size();
	scanParams.flag=flag;
	client.Send(&scanParams,sizeof(scanParams));
	
	client.Send(Value.data(),Value.size());
	
	
	   // 循环接收进度
    while (true) {
      ScanProgress progress;
      if (!client.Receive(&progress, sizeof(progress))) {
        std::cerr << "接收进度失败" << std::endl;
        break;
      }
      if (progress.msgType == 1) {
        std::cout << "\r进度: " << std::fixed << std::setprecision(2)
                  << progress.percent << "%, 命中: " << progress.matchCount
                  << " scan " << progress.scannedBytes
                  << " total "<<progress.totalBytes
                  << std::flush;
      } else if (progress.msgType == 2) {
        std::cout << "\r进度: 100.00%, 命中: " << progress.matchCount
                  << " scan " << progress.scannedBytes
                  << " total "<<progress.totalBytes
                  << std::endl;
        break;
      } else if (progress.msgType == 3) {
        std::cerr << "扫描出错" << std::endl;
        break;
      }
    }
    
    int len;
    client.Receive(&len, 4);
    std::cout << "scan result size "<< std::dec << len << std::endl;
	
	return len;
	
}


int GetScanResultCount(){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	
	unsigned char command = CMD_GETSCANRESULT_COUNT;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;
	int count = 0;
	if (!client.Receive(&count, sizeof(count))) return false;
	return count;
}

bool GetScanResult(int offset,int count,std::vector<std::pair<uint64_t,uint64_t>>& results/*address,Value*/){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	
	unsigned char command = CMD_GETSCANRESULT;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;
	
	CeGetScanResultInput input;
	input.offset=offset;
	input.count=count;
	if (!client.Send(&input, sizeof(input))) return false;
	
	CeGetScanResultOutput output;
	if (!client.Receive(&output, sizeof(output))) return false;
	if (output.actual_count==0) return false;

	results.reserve(output.actual_count);
	client.Receive(results.data(),output.actual_count*sizeof(uint64_t)*2);

	return true;
}

bool RemoveScanResult(std::vector<uint64_t>address){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	
	unsigned char command = CMD_REMOVESCANRESULT;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;
	if (!client.Send(address.data(),address.size()*sizeof(uint64_t))) return false;
	
	return true;
}

bool ClearScanResult(){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;

	unsigned char command = CMD_CLEARSCANRESULT;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;
	
	return true;
}

int ScanValueWithProgress(uint32_t flags, std::vector<unsigned char>& Value, ScanProgressCallback callback, void* userData, uint64_t start, uint64_t end){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return 0;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return 0;

	unsigned char command = CMD_SCANVALUE;
	if (!client.Send(&command, sizeof(command))) return 0;
	if (!client.Send(&handle, sizeof(handle))) return 0;

	#pragma pack(1)
	struct {
		uint64_t start;
		uint64_t end;
		int size;
		unsigned int flag;
	} scanParams;
	#pragma pack()
	
	scanParams.start = start;
	scanParams.end = end;
	scanParams.size = Value.size();
	scanParams.flag = flags;
	client.Send(&scanParams, sizeof(scanParams));
	client.Send(Value.data(), Value.size());
	
	// 进度回调循环
	while (true) {
		ScanProgress progress;
		if (!client.Receive(&progress, sizeof(progress))) {
			break;
		}
		
		if (callback) {
			callback(progress.percent, progress.matchCount, progress.scannedBytes, progress.totalBytes, userData);
		}
		
		if (progress.msgType == 2) { // 扫描完成
			break;
		} else if (progress.msgType == 3) { // 扫描出错
			return 0;
		}
	}
	
	int len;
	if (!client.Receive(&len, sizeof(len))) return 0;
	return len;
}

int ScanNextValueWithProgress(std::vector<unsigned char>& Value, int flag, ScanProgressCallback callback, void* userData, uint64_t start, uint64_t end){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return 0;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return 0;

	unsigned char command = CMD_SCANNEXTVALUE;
	if (!client.Send(&command, sizeof(command))) return 0;
	if (!client.Send(&handle, sizeof(handle))) return 0;

	#pragma pack(1)
	struct {
		uint64_t start;
		uint64_t end;
		int size;
		unsigned int flag;
	} scanParams;
	#pragma pack()

	scanParams.start = start;
	scanParams.end = end;
	scanParams.size = Value.size();
	scanParams.flag = flag;
	client.Send(&scanParams, sizeof(scanParams));
	client.Send(Value.data(), Value.size());

	// 进度回调循环
	while (true) {
		ScanProgress progress;
		if (!client.Receive(&progress, sizeof(progress))) {
			break;
		}
		
		if (callback) {
			callback(progress.percent, progress.matchCount, progress.scannedBytes, progress.totalBytes, userData);
		}
		
		if (progress.msgType == 2) { // 扫描完成
			break;
		} else if (progress.msgType == 3) { // 扫描出错
			return 0;
		}
	}

	int len;
	if (!client.Receive(&len, sizeof(len))) return 0;
	return len;
}



int ScanFuzzyValueWithProgress(uint32_t flags, ScanProgressCallback callback, void* userData, uint64_t start, uint64_t end){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return 0;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return 0;

	unsigned char command = CMD_SCANFUZZYVALUE;
	if (!client.Send(&command, sizeof(command))) return 0;
	if (!client.Send(&handle, sizeof(handle))) return 0;

	#pragma pack(1)
	struct {
		uint64_t start;
		uint64_t end;
		unsigned int flag;
	} scanParams;
	#pragma pack()
	
	scanParams.start = start;
	scanParams.end = end;
	scanParams.flag = flags;
	client.Send(&scanParams, sizeof(scanParams));

	// 进度回调循环
	while (true) {
		ScanProgress progress;
		if (!client.Receive(&progress, sizeof(progress))) {
			break;
		}
		
		if (callback) {
			callback(progress.percent, progress.matchCount, progress.scannedBytes, progress.totalBytes, userData);
		}
		
		if (progress.msgType == 2) { // 扫描完成
			break;
		} else if (progress.msgType == 3) { // 扫描出错
			return 0;
		}
	}
	
	int len;
	if (!client.Receive(&len, sizeof(len))) return 0;
	return len;
}


bool ReadProcessMemoryBytes(uint64_t address, uint32_t size, std::vector<unsigned char>& out)
{
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	#pragma pack(1)
	struct { unsigned char command; CeReadProcessMemoryInput input; } op;
	#pragma pack()
	op.command = CMD_READPROCESSMEMORY;
	op.input.handle = handle;
	op.input.address = address;
	op.input.size = size;
	op.input.compress = 0;
	if (!client.Send(&op, sizeof(op))) return false;

	CeReadProcessMemoryOutput outHdr{};
	if (!client.Receive(&outHdr, sizeof(outHdr))) return false;
	if (outHdr.read <= 0) { out.clear(); return true; }
	out.resize(outHdr.read);
	return client.Receive(out.data(), out.size());
}

bool WriteProcessMemoryBytes(uint64_t address, uint32_t size, std::vector<unsigned char>& data){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	
	#pragma pack(1)
	struct { 
		unsigned char command;
	 CeWriteProcessMemoryInput input;
	  } op;
	#pragma pack()
	op.command = CMD_WRITEPROCESSMEMORY;
	op.input.handle = handle;
	op.input.address = address;
	op.input.size = size;
	if (!client.Send(&op, sizeof(op))) return false;
	 client.Send(data.data(), data.size());

	 CeWriteProcessMemoryOutput output;
	 if (!client.Receive(&output, sizeof(output))) return false;
	 return output.written == size;
}

bool ReadProcessMemory_(uint64_t address, uint32_t size,void* out,int32_t& Realread)
{
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	#pragma pack(1)
	struct { 
		unsigned char command;
	 CeReadProcessMemoryInput input;
	  } op;
	#pragma pack()
	op.command = CMD_READPROCESSMEMORY;
	op.input.handle = handle;
	op.input.address = address;
	op.input.size = size;
	op.input.compress = 0;
	if (!client.Send(&op, sizeof(op))) return false;

	client.Receive(&Realread, sizeof(Realread));
	return client.Receive(out, size);
}

bool  ReadBratchMemory(uint64_t address,uint32_t size,std::vector<std::pair<uint64_t, std::vector<uint8_t>>>& out){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	#pragma pack(1)
	struct { 
		unsigned char command;
		int handle;
		 uint64_t address; uint32_t size; } op;
	#pragma pack()
	op.command = CMD_READBRATCHMEMORY;
	op.handle = handle;
	op.address = address;
	op.size = size;
	if (!client.Send(&op, sizeof(op))) return false;

	int len = 0;
	if (!client.Receive(&len, sizeof(len))) return false;
	if (len <= 0) { out.clear(); return true; }

	out.resize(len);
	for (int i = 0; i < len; i++) {
		uint64_t addr = 0;
		std::vector<unsigned char> data;
		data.resize(4096);
		if (!client.Receive(&addr, sizeof(addr))) return false;
		if (!client.Receive(data.data(), 4096)) return false;
		out[i] = {addr, data};
	}
	return true;
}

bool ReadBratchAddr( std::vector<std::pair<uint64_t, int32_t>/*addr,size*/>& addrs,
	 std::vector<std::pair<uint64_t, std::vector<uint8_t>>/*addr,data*/>& out){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;

	// 发送命令
	unsigned char command = CMD_READBRATCHADDR;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;
	
	// 发送地址列表
	int len = addrs.size();
	if (!client.Send(&len, sizeof(len))) return false;
	
	std::vector<CeReadBratchAddr> input(len);
	for (int i = 0; i < len; i++) {
		input[i].addr = addrs[i].first;
		input[i].size = addrs[i].second;
	}
	if (!client.Send(input.data(), len * sizeof(CeReadBratchAddr))) return false;
	
	// 接收数据
	out.clear();
	out.resize(len);
	int result = 0;
	if (!client.Receive(&result, sizeof(result))) return false;
	
	for (int i = 0; i < len; i++) {
		uint64_t addr = 0;
		int32_t size = input[i].size;
		std::vector<unsigned char> data(size);
		if (!client.Receive(&addr, sizeof(addr))) return false;

		if (!client.Receive(data.data(), size)) return false;
		out[i] = {addr, data};
	}
	return true;
}

bool GetModuleBaseByName(const std::string& moduleName, uint64_t& outBase)
{
	std::vector<ModuleInfoItem> mods;
	if (!FetchModuleList(mods)) return false;
	for (const auto& m : mods) {
		if (_stricmp(m.name.c_str(), moduleName.c_str()) == 0) { outBase = m.base; return true; }
	}
	return false;
}

static bool read_u64(uint64_t address, uint64_t& value)
{
	std::vector<unsigned char> buf;
	if (!ReadProcessMemoryBytes(address, 8, buf)) return false;
	if (buf.size() < 8) return false;
	value =  (uint64_t)buf[0]
			| ((uint64_t)buf[1] << 8)
			| ((uint64_t)buf[2] << 16)
			| ((uint64_t)buf[3] << 24)
			| ((uint64_t)buf[4] << 32)
			| ((uint64_t)buf[5] << 40)
			| ((uint64_t)buf[6] << 48)
			| ((uint64_t)buf[7] << 56);
	return true;
}

bool ResolveModuleOffsetChain(uint64_t& outAddress, const std::string& moduleName, uint64_t baseOffset, const std::vector<uint64_t>& offsets, bool derefFinal)
{
	uint64_t base = 0;
	if (!GetModuleBaseByName(moduleName, base)) return false;
	uint64_t addr = base + baseOffset;
	if (offsets.empty()) { outAddress = addr; return true; }
	for (size_t i = 0; i < offsets.size(); ++i) {
		uint64_t ptr = 0;
		if (!read_u64(addr, ptr)) return false;
		addr = ptr + offsets[i];
	}
	if (derefFinal) {
		uint64_t finalPtr = 0;
		if (!read_u64(addr, finalPtr)) return false;
		addr = finalPtr;
	}
	outAddress = addr;
	return true;
} 


//=================内核断点相关=================
bool SetKernelBreakpoint(uint64_t address, uint32_t bpType, uint32_t bpSize){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;

	unsigned char command = CMD_KERNEL_SETBREAKPOINT;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;

	if (!client.Send(&address, sizeof(address))) return false;
	if (!client.Send(&bpType, sizeof(bpType))) return false;
	if (!client.Send(&bpSize, sizeof(bpSize))) return false;

	int result = 0;
	if (!client.Receive(&result, sizeof(result))) return false;
	if (result == 0) return false;
	
	return true;
	
}


bool RemoveKernelBreakpoint(uint64_t address){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	
	unsigned char command = CMD_KERNEL_REMOVEBREAKPOINT;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;
	if (!client.Send(&address, sizeof(address))) return false;

	int result = 0;
	if (!client.Receive(&result, sizeof(result))) return false;
	if (result == 0) return false;

	return true;
}

bool SuspendKernelBreakpoint(uint64_t address){	
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	
	unsigned char command = CMD_KERNEL_SUSPENDBREAKPOINT;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;
	if (!client.Send(&address, sizeof(address))) return false;

	int result = 0;
	if (!client.Receive(&result, sizeof(result))) return false;
	if (result == 0) return false;
	
	return true;
}

bool ResumeKernelBreakpoint(uint64_t address){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	
	unsigned char command = CMD_KERNEL_RESUMEBREAKPOINT;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;
	if (!client.Send(&address, sizeof(address))) return false;

	int result = 0;
	if (!client.Receive(&result, sizeof(result))) return false;
	if (result == 0) return false;
	
	return true;
}

bool ReadKernelBreakpointInfo(uint64_t address,std::vector<HW_HIT_INFO>& infos){
	WindowsSocketClient& client = GetSocketClient();
	if (!client.IsConnected()) return false;
	int handle = 0;
	if (!EnsureOpenHandle(handle)) return false;
	
	unsigned char command = CMD_KERNEL_READHWBPINFO;
	if (!client.Send(&command, sizeof(command))) return false;
	if (!client.Send(&handle, sizeof(handle))) return false;
	if (!client.Send(&address, sizeof(address))) return false;

	int result = 0;
	uint64_t TotalCount = 0;
	if (!client.Receive(&result, sizeof(result))) return false;
	if (!client.Receive(&TotalCount, sizeof(TotalCount))) return false;
	
	if (result > 0) {
		infos.resize(result);
		if (!client.Receive(infos.data(), result * sizeof(HW_HIT_INFO))) return false;
		
	}
	return true;
}











