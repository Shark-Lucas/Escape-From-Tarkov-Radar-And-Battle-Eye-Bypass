//#include "Driver.h"
//#include <string>
//#include <TlHelp32.h>
//#include <iostream>
//
//typedef NTSTATUS(NTAPI* pfnNtQueryInformationProcess)(
//	IN  HANDLE ProcessHandle,
//	IN  PROCESSINFOCLASS ProcessInformationClass,
//	OUT PVOID ProcessInformation,
//	IN  ULONG ProcessInformationLength,
//	OUT PULONG ReturnLength    OPTIONAL
//	);
//
//pfnNtQueryInformationProcess gNtQueryInformationProcess;
//HMODULE hNtDll;
//
//HANDLE Driver::driverH = 0;
//uintptr_t Driver::currentProcessId = 0;
//GUID DummyGuid = { 2 }; //don't matter our var never will be saved
//
//bool CheckDriverStatus() {
//	int icheck = 82;
//	NTSTATUS status = 0;
//
//	uintptr_t BaseAddr = Driver::GetBaseAddress(GetCurrentProcessId());
//	if (BaseAddr == 0) {
//		return false;
//	}
//
//	int checked = Driver::read(GetCurrentProcessId(), (uintptr_t)&icheck, &status);
//	if (checked != icheck) {
//		return false;
//	}
//	return true;
//}
//
//NTSTATUS SetSystemEnvironmentPrivilege(BOOLEAN Enable, PBOOLEAN WasEnabled)
//{
//	if (WasEnabled != nullptr)
//		*WasEnabled = FALSE;
//
//	BOOLEAN SeSystemEnvironmentWasEnabled;
//	const NTSTATUS Status = RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
//		Enable,
//		FALSE,
//		&SeSystemEnvironmentWasEnabled);
//
//	if (NT_SUCCESS(Status) && WasEnabled != nullptr)
//		*WasEnabled = SeSystemEnvironmentWasEnabled;
//
//	return Status;
//}
//
//void Driver::SendCommand(MemoryCommand* cmd)
//{
//	UNICODE_STRING VariableName = RTL_CONSTANT_STRING(VARIABLE_NAME);
//	NtSetSystemEnvironmentValueEx(
//		&VariableName,
//		&DummyGuid,
//		cmd,
//		sizeof(MemoryCommand),
//		ATTRIBUTES);
//}
//
//uintptr_t Driver::GetBaseAddress(uintptr_t pid) {
//	uintptr_t result = 0;
//	MemoryCommand cmd = MemoryCommand();
//	cmd.operation = baseOperation * 0x289;
//	cmd.magic = COMMAND_MAGIC;
//	cmd.data[0] = pid;
//	cmd.data[1] = (uintptr_t)&result;
//	SendCommand(&cmd);
//	return result;
//}
//
//NTSTATUS Driver::copy_memory(
//	const uintptr_t	src_process_id,
//	const uintptr_t src_address,
//	const uintptr_t	dest_process_id,
//	const uintptr_t	dest_address,
//	const size_t	size) {
//	uintptr_t result = 0;
//	MemoryCommand cmd = MemoryCommand();
//	cmd.operation = baseOperation * 0x823;
//	cmd.magic = COMMAND_MAGIC;
//	cmd.data[0] = (uintptr_t)src_process_id;
//	cmd.data[1] = (uintptr_t)src_address;
//	cmd.data[2] = (uintptr_t)dest_process_id;
//	cmd.data[3] = (uintptr_t)dest_address;
//	cmd.data[4] = (uintptr_t)size;
//	cmd.data[5] = (uintptr_t)&result;
//	SendCommand(&cmd);
//	return (NTSTATUS)result;
//}
//
//uintptr_t GetKernelModuleExport(uintptr_t kernel_module_base, char* function_name)
//{
//	if (!kernel_module_base)
//		return 0;
//
//	IMAGE_DOS_HEADER dos_header = { 0 };
//	IMAGE_NT_HEADERS64 nt_headers = { 0 };
//
//	Driver::read_memory(4, kernel_module_base, (uintptr_t)&dos_header, sizeof(dos_header));
//
//	if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
//		return 0;
//
//	Driver::read_memory(4, kernel_module_base + dos_header.e_lfanew, (uintptr_t)&nt_headers, sizeof(nt_headers));
//
//	if (nt_headers.Signature != IMAGE_NT_SIGNATURE)
//		return 0;
//
//	const auto export_base = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
//	const auto export_base_size = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
//
//	if (!export_base || !export_base_size)
//		return 0;
//
//	const auto export_data = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(VirtualAlloc(nullptr, export_base_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
//
//	Driver::read_memory(4, kernel_module_base + export_base, (uintptr_t)export_data, export_base_size);
//
//	const auto delta = reinterpret_cast<uintptr_t>(export_data) - export_base;
//
//	const auto name_table = reinterpret_cast<UINT32*>(export_data->AddressOfNames + delta);
//	const auto ordinal_table = reinterpret_cast<UINT16*>(export_data->AddressOfNameOrdinals + delta);
//	const auto function_table = reinterpret_cast<UINT32*>(export_data->AddressOfFunctions + delta);
//
//	for (auto i = 0u; i < export_data->NumberOfNames; ++i)
//	{
//		char* current_function_name = (char*)(name_table[i] + delta);
//
//		if (!_stricmp(current_function_name, function_name))
//		{
//			const auto function_ordinal = ordinal_table[i];
//			const auto function_address = kernel_module_base + function_table[function_ordinal];
//
//			if (function_address >= kernel_module_base + export_base && function_address <= kernel_module_base + export_base + export_base_size)
//			{
//				VirtualFree(export_data, 0, MEM_RELEASE);
//				return 0; // No forwarded exports on 64bit?
//			}
//
//			VirtualFree(export_data, 0, MEM_RELEASE);
//			return function_address;
//		}
//	}
//
//	VirtualFree(export_data, 0, MEM_RELEASE);
//	return 0;
//}
//
//uintptr_t GetKernelModuleAddress(char* module_name)
//{
//	void* buffer = nullptr;
//	DWORD buffer_size = 0;
//
//	NTSTATUS status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation), buffer, buffer_size, &buffer_size);
//
//	while (status == STATUS_INFO_LENGTH_MISMATCH)
//	{
//		VirtualFree(buffer, 0, MEM_RELEASE);
//
//		buffer = VirtualAlloc(nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//		if (buffer == 0) {
//			return 0;
//		}
//		status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation), buffer, buffer_size, &buffer_size);
//	}
//
//	if (!NT_SUCCESS(status))
//	{
//		VirtualFree(buffer, 0, MEM_RELEASE);
//		return 0;
//	}
//
//	const PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)buffer;
//	if (modules == nullptr) {
//		VirtualFree(buffer, 0, MEM_RELEASE);
//		return 0;
//	}
//	for (auto i = 0u; i < modules->NumberOfModules; ++i)
//	{
//		char* current_module_name = (char*)(modules->Modules[i].FullPathName + modules->Modules[i].OffsetToFileName);
//
//		if (!_stricmp(current_module_name, module_name))
//		{
//			const uintptr_t result = (uintptr_t)(modules->Modules[i].ImageBase);
//
//			VirtualFree(buffer, 0, MEM_RELEASE);
//			return result;
//		}
//	}
//
//	VirtualFree(buffer, 0, MEM_RELEASE);
//	return 0;
//}
//
//extern "C"
//{
//	bool Driver::initialize() {
//		currentProcessId = GetCurrentProcessId();
//		BOOLEAN SeSystemEnvironmentWasEnabled;
//
//		NTSTATUS status = SetSystemEnvironmentPrivilege(true, &SeSystemEnvironmentWasEnabled);
//
//		if (!NT_SUCCESS(status)) {
//			return false;
//		}
//
//
//		BYTE nstosname[] = { 'n','t','o','s','k','r','n','l','.','e','x','e',0 };
//		uintptr_t kernelModuleAddress = GetKernelModuleAddress((char*)nstosname);
//		memset(nstosname, 0, sizeof(nstosname));
//
//		BYTE pbid[] = { 'P','s','L','o','o','k','u','p','P','r','o','c','e','s','s','B','y','P','r','o','c','e','s','s','I','d',0 };
//		BYTE gba[] = { 'P','s','G','e','t','P','r','o','c','e','s','s','S','e','c','t','i','o','n','B','a','s','e','A','d','d','r','e','s','s',0 };
//		BYTE mmcp[] = { 'M','m','C','o','p','y','V','i','r','t','u','a','l','M','e','m','o','r','y',0 };
//		uintptr_t kernel_PsLookupProcessByProcessId = GetKernelModuleExport(kernelModuleAddress, (char*)pbid);
//		uintptr_t kernel_PsGetProcessSectionBaseAddress = GetKernelModuleExport(kernelModuleAddress, (char*)gba);
//		uintptr_t kernel_MmCopyVirtualMemory = GetKernelModuleExport(kernelModuleAddress, (char*)mmcp);
//		memset(pbid, 0, sizeof(pbid));
//		memset(gba, 0, sizeof(gba));
//		memset(mmcp, 0, sizeof(mmcp));
//
//		uintptr_t result = 0;
//		MemoryCommand cmd = MemoryCommand();
//		cmd.operation = baseOperation * 0x612;
//		cmd.magic = COMMAND_MAGIC;
//		cmd.data[0] = kernel_PsLookupProcessByProcessId;
//		cmd.data[1] = kernel_PsGetProcessSectionBaseAddress;
//		cmd.data[2] = kernel_MmCopyVirtualMemory;
//		cmd.data[3] = (uintptr_t)&result;
//		SendCommand(&cmd);
//		return result;
//	}
//}
//
//PEB GetPEB(DWORD procId) {
//	PROCESS_BASIC_INFORMATION pbi;
//	PEB peb{ 0 };
//	HANDLE hProcess;
//
//	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, procId);
//
//	hNtDll = LoadLibrary(L"NtDll.dll");
//
//	gNtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(hNtDll, "NtQueryInformationProcess");
//
//	NTSTATUS status = gNtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
//
//	//if (NT_SUCCESS(status)) {
//	//	Driver::read_memory(procId, (uintptr_t)pbi.PebBaseAddress, (uintptr_t)&peb, sizeof(peb));
//	//}
//
//	return Driver::read2<PEB>(procId, (uintptr_t)pbi.PebBaseAddress);
//}
//
//struct _RTL_BALANCED_NODE
//{
//	union
//	{
//		struct _RTL_BALANCED_NODE* Children[2];                             //0x0
//		struct
//		{
//			struct _RTL_BALANCED_NODE* Left;                                //0x0
//			struct _RTL_BALANCED_NODE* Right;                               //0x8
//		};
//	};
//	union
//	{
//		struct
//		{
//			UCHAR Red : 1;                                                    //0x10
//			UCHAR Balance : 2;                                                //0x10
//		};
//		ULONGLONG ParentValue;                                              //0x10
//	};
//};
//
////0x120 bytes (sizeof)
//struct newLDR_DATA_TABLE_ENTRY
//{
//	struct _LIST_ENTRY InLoadOrderLinks;                                    //0x0
//	struct _LIST_ENTRY InMemoryOrderLinks;                                  //0x10
//	struct _LIST_ENTRY InInitializationOrderLinks;                          //0x20
//	VOID* DllBase;                                                          //0x30
//	VOID* EntryPoint;                                                       //0x38
//	ULONG SizeOfImage;                                                      //0x40
//	struct _UNICODE_STRING FullDllName;                                     //0x48
//	struct _UNICODE_STRING BaseDllName;                                     //0x58
//	union
//	{
//		UCHAR FlagGroup[4];                                                 //0x68
//		ULONG Flags;                                                        //0x68
//		struct
//		{
//			ULONG PackagedBinary : 1;                                         //0x68
//			ULONG MarkedForRemoval : 1;                                       //0x68
//			ULONG ImageDll : 1;                                               //0x68
//			ULONG LoadNotificationsSent : 1;                                  //0x68
//			ULONG TelemetryEntryProcessed : 1;                                //0x68
//			ULONG ProcessStaticImport : 1;                                    //0x68
//			ULONG InLegacyLists : 1;                                          //0x68
//			ULONG InIndexes : 1;                                              //0x68
//			ULONG ShimDll : 1;                                                //0x68
//			ULONG InExceptionTable : 1;                                       //0x68
//			ULONG ReservedFlags1 : 2;                                         //0x68
//			ULONG LoadInProgress : 1;                                         //0x68
//			ULONG LoadConfigProcessed : 1;                                    //0x68
//			ULONG EntryProcessed : 1;                                         //0x68
//			ULONG ProtectDelayLoad : 1;                                       //0x68
//			ULONG ReservedFlags3 : 2;                                         //0x68
//			ULONG DontCallForThreads : 1;                                     //0x68
//			ULONG ProcessAttachCalled : 1;                                    //0x68
//			ULONG ProcessAttachFailed : 1;                                    //0x68
//			ULONG CorDeferredValidate : 1;                                    //0x68
//			ULONG CorImage : 1;                                               //0x68
//			ULONG DontRelocate : 1;                                           //0x68
//			ULONG CorILOnly : 1;                                              //0x68
//			ULONG ChpeImage : 1;                                              //0x68
//			ULONG ReservedFlags5 : 2;                                         //0x68
//			ULONG Redirected : 1;                                             //0x68
//			ULONG ReservedFlags6 : 2;                                         //0x68
//			ULONG CompatDatabaseProcessed : 1;                                //0x68
//		};
//	};
//	USHORT ObsoleteLoadCount;                                               //0x6c
//	USHORT TlsIndex;                                                        //0x6e
//	struct _LIST_ENTRY HashLinks;                                           //0x70
//	ULONG TimeDateStamp;                                                    //0x80
//	struct _ACTIVATION_CONTEXT* EntryPointActivationContext;                //0x88
//	VOID* Lock;                                                             //0x90
//	struct _LDR_DDAG_NODE* DdagNode;                                        //0x98
//	struct _LIST_ENTRY NodeModuleLink;                                      //0xa0
//	struct _LDRP_LOAD_CONTEXT* LoadContext;                                 //0xb0
//	VOID* ParentDllBase;                                                    //0xb8
//	VOID* SwitchBackContext;                                                //0xc0
//	struct _RTL_BALANCED_NODE BaseAddressIndexNode;                         //0xc8
//	struct _RTL_BALANCED_NODE MappingInfoIndexNode;                         //0xe0
//	ULONGLONG OriginalBase;                                                 //0xf8
//	union _LARGE_INTEGER LoadTime;                                          //0x100
//	ULONG BaseNameHashValue;                                                //0x108
//	enum _LDR_DLL_LOAD_REASON LoadReason;                                   //0x10c
//	ULONG ImplicitPathOptions;                                              //0x110
//	ULONG ReferenceCount;                                                   //0x114
//	ULONG DependentLoadFlags;                                               //0x118
//	UCHAR SigningLevel;                                                     //0x11c
//};
//
//newLDR_DATA_TABLE_ENTRY* GetLDREntryInternal(const wchar_t* moduleName, PEB* peb) {
//	newLDR_DATA_TABLE_ENTRY* modEntry = nullptr;
//	LIST_ENTRY head = peb->Ldr->InMemoryOrderModuleList;
//	LIST_ENTRY curr = head;
//	int count = 0;
//
//	for (auto curr = head; curr.Flink != &peb->Ldr->InMemoryOrderModuleList; curr = *curr.Flink) {
//		newLDR_DATA_TABLE_ENTRY* mod = (newLDR_DATA_TABLE_ENTRY*)CONTAINING_RECORD(curr.Flink, newLDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
//		count++;
//
//		if (mod->BaseDllName.Buffer) {
//			if (wcscmp(moduleName, (const wchar_t*)mod->BaseDllName.Buffer) == 0) {
//				modEntry = mod;
//				break;
//			}
//		}
//	}
//
//	return modEntry;
//}
//
//extern "C"
//{
//	NTSTATUS Driver::read_memory(
//		const uintptr_t	process_id,
//		const uintptr_t address,
//		const uintptr_t buffer,
//		const size_t	size) {
//		return copy_memory(process_id, address, currentProcessId, buffer, size);
//	}
//}
//
//NTSTATUS Driver::write_memory(
//	const uintptr_t	process_id,
//	const uintptr_t address,
//	const uintptr_t buffer,
//	const size_t	size) {
//	return copy_memory(currentProcessId, buffer, process_id, address, size);
//}
//
//extern "C"
//{
//	uintptr_t Driver::GetModuleBaseAddress(DWORD procId, const char* nameAsChar)
//	{
//		std::string nameAsString = std::string(nameAsChar);
//		std::wstring widestr = std::wstring(nameAsString.begin(), nameAsString.end());
//		const wchar_t* name = widestr.c_str();
//
//		PEB peb = GetPEB(procId);
//
//		if (peb.SessionId == 0) {
//			return 0;
//		}
//
//		newLDR_DATA_TABLE_ENTRY* modEntry = GetLDREntryInternal(name, &peb);
//
//		if (hNtDll)
//			FreeLibrary(hNtDll);
//		gNtQueryInformationProcess = NULL;
//
//		char* baseAddy = (char*)modEntry->DllBase;
//		return atoi(baseAddy);
//	}
//}
//
//extern "C"
//{
//	uintptr_t Driver::GrabProcessByName(const char* nameAsChar)
//	{
//		std::string nameAsString = std::string(nameAsChar);
//		std::wstring widestr = std::wstring(nameAsString.begin(), nameAsString.end());
//		const wchar_t* name = widestr.c_str();
//
//		DWORD pid = 0;
//		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//
//		PROCESSENTRY32 process;
//		ZeroMemory(&process, sizeof(process));
//		process.dwSize = sizeof(process);
//
//		if (Process32First(snapshot, &process))
//		{
//			do
//			{
//				if (!_wcsicmp(process.szExeFile, name))
//				{
//					pid = process.th32ProcessID;
//					break;
//				}
//			} while (Process32Next(snapshot, &process));
//		}
//
//		CloseHandle(snapshot);
//		return pid;
//	}
//}
//
//extern "C"
//{
//	uintptr_t Driver::read(const uintptr_t process_id, const uintptr_t address, PNTSTATUS out_status)
//	{
//		uintptr_t buffer{ };
//		read_memory(process_id, address, uintptr_t(&buffer), sizeof(uintptr_t));
//		return buffer;
//	}
//}
//
//extern "C"
//{
//	void Driver::write(const uintptr_t process_id, const uintptr_t address, const uintptr_t& buffer, PNTSTATUS out_status)
//	{
//		Driver::write_memory(process_id, address, uintptr_t(&buffer), sizeof(uintptr_t));
//	}
//}
