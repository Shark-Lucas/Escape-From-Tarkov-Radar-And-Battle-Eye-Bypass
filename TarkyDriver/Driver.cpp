#include "Driver.h"
#include <string>
#include <TlHelp32.h>
#include <iostream>
#include "atlstr.h"

pfnNtQueryInformationProcess gNtQueryInformationProcess;
HANDLE Driver::driverH = 0;
uintptr_t Driver::currentProcessId = 0;
GUID DummyGuid = { 2 }; //don't matter our var never will be saved

bool CheckDriverStatus() {
	int icheck = 82;
	NTSTATUS status = 0;

	uintptr_t BaseAddr = Driver::GetBaseAddress(GetCurrentProcessId());
	if (BaseAddr == 0) {
		return false;
	}

	int checked = Driver::read(GetCurrentProcessId(), (uintptr_t)&icheck, &status);
	if (checked != icheck) {
		return false;
	}
	return true;
}

NTSTATUS SetSystemEnvironmentPrivilege(BOOLEAN Enable, PBOOLEAN WasEnabled)
{
	if (WasEnabled != nullptr)
		*WasEnabled = FALSE;

	BOOLEAN SeSystemEnvironmentWasEnabled;
	const NTSTATUS Status = RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
		Enable,
		FALSE,
		&SeSystemEnvironmentWasEnabled);

	if (NT_SUCCESS(Status) && WasEnabled != nullptr)
		*WasEnabled = SeSystemEnvironmentWasEnabled;

	return Status;
}

void Driver::SendCommand(MemoryCommand* cmd)
{
	UNICODE_STRING VariableName = RTL_CONSTANT_STRING(VARIABLE_NAME);
	NtSetSystemEnvironmentValueEx(
		&VariableName,
		&DummyGuid,
		cmd,
		sizeof(MemoryCommand),
		ATTRIBUTES);
}

uintptr_t Driver::GetBaseAddress(uintptr_t pid) {
	uintptr_t result = 0;
	MemoryCommand cmd = MemoryCommand();
	cmd.operation = baseOperation * 0x289;
	cmd.magic = COMMAND_MAGIC;
	cmd.data[0] = pid;
	cmd.data[1] = (uintptr_t)&result;
	SendCommand(&cmd);
	return result;
}

NTSTATUS Driver::copy_memory(
	const uintptr_t	src_process_id,
	const uintptr_t src_address,
	const uintptr_t	dest_process_id,
	const uintptr_t	dest_address,
	const size_t	size) {
	uintptr_t result = 0;
	MemoryCommand cmd = MemoryCommand();
	cmd.operation = baseOperation * 0x823;
	cmd.magic = COMMAND_MAGIC;
	cmd.data[0] = (uintptr_t)src_process_id;
	cmd.data[1] = (uintptr_t)src_address;
	cmd.data[2] = (uintptr_t)dest_process_id;
	cmd.data[3] = (uintptr_t)dest_address;
	cmd.data[4] = (uintptr_t)size;
	cmd.data[5] = (uintptr_t)&result;
	SendCommand(&cmd);
	return (NTSTATUS)result;
}

uintptr_t GetKernelModuleExport(uintptr_t kernel_module_base, char* function_name)
{
	if (!kernel_module_base)
		return 0;

	IMAGE_DOS_HEADER dos_header = { 0 };
	IMAGE_NT_HEADERS64 nt_headers = { 0 };

	Driver::read_memory(4, kernel_module_base, (uintptr_t)&dos_header, sizeof(dos_header));

	if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
		return 0;

	Driver::read_memory(4, kernel_module_base + dos_header.e_lfanew, (uintptr_t)&nt_headers, sizeof(nt_headers));

	if (nt_headers.Signature != IMAGE_NT_SIGNATURE)
		return 0;

	const auto export_base = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	const auto export_base_size = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	if (!export_base || !export_base_size)
		return 0;

	const auto export_data = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(VirtualAlloc(nullptr, export_base_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	Driver::read_memory(4, kernel_module_base + export_base, (uintptr_t)export_data, export_base_size);

	const auto delta = reinterpret_cast<uintptr_t>(export_data) - export_base;

	const auto name_table = reinterpret_cast<UINT32*>(export_data->AddressOfNames + delta);
	const auto ordinal_table = reinterpret_cast<UINT16*>(export_data->AddressOfNameOrdinals + delta);
	const auto function_table = reinterpret_cast<UINT32*>(export_data->AddressOfFunctions + delta);

	for (auto i = 0u; i < export_data->NumberOfNames; ++i)
	{
		char* current_function_name = (char*)(name_table[i] + delta);

		if (!_stricmp(current_function_name, function_name))
		{
			const auto function_ordinal = ordinal_table[i];
			const auto function_address = kernel_module_base + function_table[function_ordinal];

			if (function_address >= kernel_module_base + export_base && function_address <= kernel_module_base + export_base + export_base_size)
			{
				VirtualFree(export_data, 0, MEM_RELEASE);
				return 0; // No forwarded exports on 64bit?
			}

			VirtualFree(export_data, 0, MEM_RELEASE);
			return function_address;
		}
	}

	VirtualFree(export_data, 0, MEM_RELEASE);
	return 0;
}

uintptr_t GetKernelModuleAddress(char* module_name)
{
	void* buffer = nullptr;
	DWORD buffer_size = 0;

	NTSTATUS status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation), buffer, buffer_size, &buffer_size);

	while (status == STATUS_INFO_LENGTH_MISMATCH)
	{
		VirtualFree(buffer, 0, MEM_RELEASE);

		buffer = VirtualAlloc(nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (buffer == 0) {
			return 0;
		}
		status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation), buffer, buffer_size, &buffer_size);
	}

	if (!NT_SUCCESS(status))
	{
		VirtualFree(buffer, 0, MEM_RELEASE);
		return 0;
	}

	const PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)buffer;
	if (modules == nullptr) {
		VirtualFree(buffer, 0, MEM_RELEASE);
		return 0;
	}
	for (auto i = 0u; i < modules->NumberOfModules; ++i)
	{
		char* current_module_name = (char*)(modules->Modules[i].FullPathName + modules->Modules[i].OffsetToFileName);

		if (!_stricmp(current_module_name, module_name))
		{
			const uintptr_t result = (uintptr_t)(modules->Modules[i].ImageBase);

			VirtualFree(buffer, 0, MEM_RELEASE);
			return result;
		}
	}

	VirtualFree(buffer, 0, MEM_RELEASE);
	return 0;
}

extern "C"
{
	bool Driver::initialize() {
		currentProcessId = GetCurrentProcessId();
		BOOLEAN SeSystemEnvironmentWasEnabled;

		NTSTATUS status = SetSystemEnvironmentPrivilege(true, &SeSystemEnvironmentWasEnabled);

		if (!NT_SUCCESS(status)) {
			return false;
		}


		BYTE nstosname[] = { 'n','t','o','s','k','r','n','l','.','e','x','e',0 };
		uintptr_t kernelModuleAddress = GetKernelModuleAddress((char*)nstosname);
		memset(nstosname, 0, sizeof(nstosname));

		BYTE pbid[] = { 'P','s','L','o','o','k','u','p','P','r','o','c','e','s','s','B','y','P','r','o','c','e','s','s','I','d',0 };
		BYTE gba[] = { 'P','s','G','e','t','P','r','o','c','e','s','s','S','e','c','t','i','o','n','B','a','s','e','A','d','d','r','e','s','s',0 };
		BYTE mmcp[] = { 'M','m','C','o','p','y','V','i','r','t','u','a','l','M','e','m','o','r','y',0 };
		uintptr_t kernel_PsLookupProcessByProcessId = GetKernelModuleExport(kernelModuleAddress, (char*)pbid);
		uintptr_t kernel_PsGetProcessSectionBaseAddress = GetKernelModuleExport(kernelModuleAddress, (char*)gba);
		uintptr_t kernel_MmCopyVirtualMemory = GetKernelModuleExport(kernelModuleAddress, (char*)mmcp);
		memset(pbid, 0, sizeof(pbid));
		memset(gba, 0, sizeof(gba));
		memset(mmcp, 0, sizeof(mmcp));

		uintptr_t result = 0;
		MemoryCommand cmd = MemoryCommand();
		cmd.operation = baseOperation * 0x612;
		cmd.magic = COMMAND_MAGIC;
		cmd.data[0] = kernel_PsLookupProcessByProcessId;
		cmd.data[1] = kernel_PsGetProcessSectionBaseAddress;
		cmd.data[2] = kernel_MmCopyVirtualMemory;
		cmd.data[3] = (uintptr_t)&result;
		SendCommand(&cmd);
		return result;
	}
}

typedef struct nPROCESS_BASIC_INFORMATION {
	PVOID Reserved1;
	uintptr_t PebBaseAddress;
	PVOID Reserved2[2];
	ULONG_PTR UniqueProcessId;
	PVOID Reserved3;
};

extern "C"
{
	NTSTATUS Driver::read_memory(
		const uintptr_t	process_id,
		const uintptr_t address,
		const uintptr_t buffer,
		const size_t	size) {
		return copy_memory(process_id, address, currentProcessId, buffer, size);
	}
}

NTSTATUS Driver::write_memory(
	const uintptr_t	process_id,
	const uintptr_t address,
	const uintptr_t buffer,
	const size_t	size) {
	return copy_memory(currentProcessId, buffer, process_id, address, size);
}

extern "C"
{
	uintptr_t Driver::GetModuleBaseAddress(DWORD procId, const char* nameAsChar)
	{
		BOOL  bReturnStatus = TRUE;
		DWORD dwSize = 0;
		DWORD dwSizeNeeded = 0;
		DWORD dwBytesRead = 0;
		DWORD dwBufferSize = 0;
		HANDLE hHeap = 0;
		WCHAR* pwszBuffer = NULL;

		smPPROCESS_BASIC_INFORMATION pbi = NULL;

		smPEB peb = { 0 };
		smPEB_LDR_DATA peb_ldr = { 0 };
		smRTL_USER_PROCESS_PARAMETERS peb_upp = { 0 };

		ZeroMemory(&peb, sizeof(peb));
		ZeroMemory(&peb_ldr, sizeof(peb_ldr));
		ZeroMemory(&peb_upp, sizeof(peb_upp));

		// Attempt to access process
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION , FALSE, procId);
		if (hProcess == INVALID_HANDLE_VALUE) {
			return FALSE;
		}

		// Try to allocate buffer 
		hHeap = GetProcessHeap();
		dwSize = sizeof(smPROCESS_BASIC_INFORMATION);
		pbi = (smPPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap,
			HEAP_ZERO_MEMORY,
			dwSize);
		// Did we successfully allocate memory
		if (!pbi) {
			CloseHandle(hProcess);
			return FALSE;
		}

		HMODULE hNtDll = LoadLibrary((L"ntdll.dll"));
		gNtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(hNtDll,"NtQueryInformationProcess");

		// Attempt to get basic info on process
		NTSTATUS dwStatus = gNtQueryInformationProcess(hProcess, ProcessBasicInformation, pbi, dwSize, &dwSizeNeeded);

		// If we had error and buffer was too small, try again
		// with larger buffer size (dwSizeNeeded)
		if (dwStatus >= 0 && dwSize < dwSizeNeeded)
		{
			if (pbi)
				HeapFree(hHeap, 0, pbi);
			pbi = (smPPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap,
				HEAP_ZERO_MEMORY,
				dwSizeNeeded);
			if (!pbi) {
				CloseHandle(hProcess);
				return FALSE;
			}
			dwStatus = gNtQueryInformationProcess(hProcess,
				ProcessBasicInformation,
				pbi, dwSizeNeeded, &dwSizeNeeded);
		}

		LIST_ENTRY currEntry = {};
		LIST_ENTRY* currEntryFlink = {};

		if (dwStatus >= 0)
		{
			if (pbi->PebBaseAddress)
			{
				Driver::initialize();
				if (Driver::read_memory(procId, reinterpret_cast<uintptr_t>(pbi->PebBaseAddress), reinterpret_cast<uintptr_t>(&peb), sizeof(peb)) == 0) {
					if (Driver::read_memory(procId, reinterpret_cast<uintptr_t>(peb.Ldr), reinterpret_cast<uintptr_t>(&peb_ldr), sizeof(peb_ldr)) == 0) {
						Driver::read_memory(procId, reinterpret_cast<uintptr_t>(peb_ldr.InMemoryOrderModuleList.Flink), reinterpret_cast<uintptr_t>(&currEntry), sizeof(currEntry));
						while (currEntryFlink != peb_ldr.InMemoryOrderModuleList.Flink) {
							Driver::read_memory(procId, reinterpret_cast<uintptr_t>(currEntry.Flink), reinterpret_cast<uintptr_t>(&currEntryFlink), sizeof(currEntryFlink));
							nLDR_DATA_TABLE_ENTRY* modPtr = (nLDR_DATA_TABLE_ENTRY*)CONTAINING_RECORD(currEntryFlink, nLDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
							nLDR_DATA_TABLE_ENTRY mod = {};
							Driver::read_memory(procId, reinterpret_cast<uintptr_t>(modPtr), reinterpret_cast<uintptr_t>(&mod), sizeof(nLDR_DATA_TABLE_ENTRY));

							std::string nameAsString = std::string(nameAsChar);
							std::wstring widestr = std::wstring(nameAsString.begin(), nameAsString.end());
							const wchar_t* name = widestr.c_str();

							std::wstring buffer(mod.BaseDllName.Length / sizeof(wchar_t), 0);
							if (Driver::read_memory(procId, reinterpret_cast<uintptr_t>(mod.BaseDllName.Buffer), reinterpret_cast<uintptr_t>(&buffer[0]), mod.BaseDllName.Length)) {
								auto error = GetLastError();
								std::cout << "readprocmemory failed uni.buff " << error << std::endl;
							}
							const wchar_t* name2 = buffer.c_str();

							if (_wcsicmp(name, name2) == 0)
							{
								return reinterpret_cast<uintptr_t>(mod.DllBase) + 0x151A218;
							}
							else {
								LIST_ENTRY nextEntry = {};
								Driver::read_memory(procId, reinterpret_cast<uintptr_t>(currEntry.Flink), reinterpret_cast<uintptr_t>(&nextEntry), sizeof(nextEntry));
								currEntry = nextEntry;
							}
						}
					}
				}
			}
		}
		return 0;
	}
}

extern "C"
{
	uintptr_t Driver::GrabProcessByName(const char* nameAsChar)
	{
		std::string nameAsString = std::string(nameAsChar);
		std::wstring widestr = std::wstring(nameAsString.begin(), nameAsString.end());
		const wchar_t* name = widestr.c_str();

		DWORD pid = 0;
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		PROCESSENTRY32 process;
		ZeroMemory(&process, sizeof(process));
		process.dwSize = sizeof(process);

		if (Process32First(snapshot, &process))
		{
			do
			{
				if (!_wcsicmp(process.szExeFile, name))
				{
					pid = process.th32ProcessID;
					break;
				}
			} while (Process32Next(snapshot, &process));
		}

		CloseHandle(snapshot);
		return pid;
	}
}

extern "C"
{
	uintptr_t Driver::read(const uintptr_t process_id, const uintptr_t address, PNTSTATUS out_status)
	{
		uintptr_t buffer{ };
		read_memory(process_id, address, uintptr_t(&buffer), sizeof(buffer));
		return 0;
	}
}

extern "C"
{
	void Driver::write(const uintptr_t process_id, const uintptr_t address, const uintptr_t& buffer, PNTSTATUS out_status)
	{
		Driver::write_memory(process_id, address, uintptr_t(&buffer), sizeof(uintptr_t));
	}
}
