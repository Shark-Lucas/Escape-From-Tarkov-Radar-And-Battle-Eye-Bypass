#pragma once

#include <Windows.h>
#include <winternl.h>
#pragma comment(lib, "ntdll.lib")
#include <stdio.h>

#define baseOperation 0x6256

#define VARIABLE_NAME L"keRdjvbgC"
#define COMMAND_MAGIC baseOperation*0x7346

#define EFI_VARIABLE_NON_VOLATILE                          0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS                    0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS                        0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD                 0x00000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS            0x00000010
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define EFI_VARIABLE_APPEND_WRITE                          0x00000040
#define ATTRIBUTES (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE (22L)
#define RTL_CONSTANT_STRING(s) { sizeof(s) - sizeof((s)[0]), sizeof(s), (PWSTR)s }

extern GUID DummyGuid;

extern "C"
{
	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlAdjustPrivilege(
			_In_ ULONG Privilege,
			_In_ BOOLEAN Enable,
			_In_ BOOLEAN Client,
			_Out_ PBOOLEAN WasEnabled
		);

	NTSYSCALLAPI
		NTSTATUS
		NTAPI
		NtSetSystemEnvironmentValueEx(
			_In_ PUNICODE_STRING VariableName,
			_In_ LPGUID VendorGuid,
			_In_reads_bytes_opt_(ValueLength) PVOID Value,
			_In_ ULONG ValueLength,
			_In_ ULONG Attributes
		);
}

typedef struct _MemoryCommand
{
	int magic;
	int operation;
	unsigned long long data[10];
} MemoryCommand;

constexpr auto STATUS_INFO_LENGTH_MISMATCH = 0xC0000004;

constexpr auto SystemModuleInformation = 11;
constexpr auto SystemHandleInformation = 16;
constexpr auto SystemExtendedHandleInformation = 64;

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

// Unicode path usually prefix with '\\?\'
#define MAX_UNICODE_PATH	32767L

// Used in PEB struct
typedef ULONG smPPS_POST_PROCESS_INIT_ROUTINE;

// Used in PEB struct
typedef struct _smPEB_LDR_DATA {
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} smPEB_LDR_DATA, *smPPEB_LDR_DATA;

// Used in PEB struct
typedef struct _smRTL_USER_PROCESS_PARAMETERS {
	BYTE Reserved1[16];
	PVOID Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} smRTL_USER_PROCESS_PARAMETERS, *smPRTL_USER_PROCESS_PARAMETERS;

// Used in PROCESS_BASIC_INFORMATION struct
typedef struct _smPEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	smPPEB_LDR_DATA Ldr;
	smPRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	BYTE Reserved4[104];
	PVOID Reserved5[52];
	smPPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE Reserved6[128];
	PVOID Reserved7[1];
	ULONG SessionId;
} smPEB, *smPPEB;

// Used with NtQueryInformationProcess
typedef struct _smPROCESS_BASIC_INFORMATION {
	LONG ExitStatus;
	smPPEB PebBaseAddress;
	ULONG_PTR AffinityMask;
	LONG BasePriority;
	ULONG_PTR UniqueProcessId;
	ULONG_PTR InheritedFromUniqueProcessId;
} smPROCESS_BASIC_INFORMATION, *smPPROCESS_BASIC_INFORMATION;

typedef struct _smPROCESSINFO
{
	DWORD	dwPID;
	DWORD	dwParentPID;
	DWORD	dwSessionID;
	DWORD	dwPEBBaseAddress;
	DWORD	dwAffinityMask;
	LONG	dwBasePriority;
	LONG	dwExitStatus;
	BYTE	cBeingDebugged;
	TCHAR	szImgPath[MAX_UNICODE_PATH];
	TCHAR	szCmdLine[MAX_UNICODE_PATH];
} smPROCESSINFO;

typedef enum _LDR_DLL_LOAD_REASON
{
	LoadReasonStaticDependency,
	LoadReasonStaticForwarderDependency,
	LoadReasonDynamicForwarderDependency,
	LoadReasonDelayloadDependency,
	LoadReasonDynamicLoad,
	LoadReasonAsImageLoad,
	LoadReasonAsDataLoad,
	LoadReasonUnknown = -1
} LDR_DLL_LOAD_REASON, *PLDR_DLL_LOAD_REASON;

struct RTL_BALANCED_NODE
{
	union
	{
		struct _RTL_BALANCED_NODE* Children[2];
		struct
		{
			struct _RTL_BALANCED_NODE* Left;
			struct _RTL_BALANCED_NODE* Right;
		};
	};
	union
	{
		struct
		{
			UCHAR Red : 1;
			UCHAR Balance : 2;
		};
		ULONG ParentValue;
	};
};

typedef struct nLDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	union
	{
		LIST_ENTRY InInitializationOrderLinks;
		LIST_ENTRY InProgressLinks;
	};
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	union
	{
		UCHAR FlagGroup[4];
		ULONG Flags;
		struct
		{
			ULONG PackagedBinary : 1;
			ULONG MarkedForRemoval : 1;
			ULONG ImageDll : 1;
			ULONG LoadNotificationsSent : 1;
			ULONG TelemetryEntryProcessed : 1;
			ULONG ProcessStaticImport : 1;
			ULONG InLegacyLists : 1;
			ULONG InIndexes : 1;
			ULONG ShimDll : 1;
			ULONG InExceptionTable : 1;
			ULONG ReservedFlags1 : 2;
			ULONG LoadInProgress : 1;
			ULONG LoadConfigProcessed : 1;
			ULONG EntryProcessed : 1;
			ULONG ProtectDelayLoad : 1;
			ULONG ReservedFlags3 : 2;
			ULONG DontCallForThreads : 1;
			ULONG ProcessAttachCalled : 1;
			ULONG ProcessAttachFailed : 1;
			ULONG CorDeferredValidate : 1;
			ULONG CorImage : 1;
			ULONG DontRelocate : 1;
			ULONG CorILOnly : 1;
			ULONG ReservedFlags5 : 3;
			ULONG Redirected : 1;
			ULONG ReservedFlags6 : 2;
			ULONG CompatDatabaseProcessed : 1;
		} s;
	} u;
	USHORT ObsoleteLoadCount;
	USHORT TlsIndex;
	LIST_ENTRY HashLinks;
	ULONG TimeDateStamp;
	struct _ACTIVATION_CONTEXT* EntryPointActivationContext;
	PVOID Lock;
	PVOID DdagNode;
	LIST_ENTRY NodeModuleLink;
	struct _LDRP_LOAD_CONTEXT* LoadContext;
	PVOID ParentDllBase;
	PVOID SwitchBackContext;
	RTL_BALANCED_NODE BaseAddressIndexNode;
	RTL_BALANCED_NODE MappingInfoIndexNode;
	ULONG_PTR OriginalBase;
	LARGE_INTEGER LoadTime;
	ULONG BaseNameHashValue;
	LDR_DLL_LOAD_REASON LoadReason;
	ULONG ImplicitPathOptions;
	ULONG ReferenceCount;
	ULONG DependentLoadFlags;
	UCHAR SigningLevel; // Since Windows 10 RS2
};

// NtQueryInformationProcess in NTDLL.DLL
typedef NTSTATUS(NTAPI* pfnNtQueryInformationProcess)(
	IN	HANDLE ProcessHandle,
	IN	PROCESSINFOCLASS ProcessInformationClass,
	OUT	PVOID ProcessInformation,
	IN	ULONG ProcessInformationLength,
	OUT	PULONG ReturnLength	OPTIONAL
	);

uintptr_t GetKernelModuleAddress(char* module_name);
uintptr_t GetKernelModuleExport(uintptr_t kernel_module_base, char* function_name);
NTSTATUS SetSystemEnvironmentPrivilege(BOOLEAN Enable, PBOOLEAN WasEnabled);

namespace Driver
{
	extern "C" __declspec(dllexport) bool	initialize();
	extern uintptr_t currentProcessId;
	extern HANDLE driverH;

	void SendCommand(MemoryCommand* cmd);
	NTSTATUS copy_memory(uintptr_t src_process_id, uintptr_t src_address, uintptr_t dest_process_id, uintptr_t dest_address, size_t size);
	extern "C" __declspec(dllexport) NTSTATUS read_memory(uintptr_t process_id, uintptr_t address, uintptr_t buffer, size_t size);
	NTSTATUS write_memory(uintptr_t process_id, uintptr_t address, uintptr_t buffer, size_t size);
	uintptr_t GetBaseAddress(uintptr_t pid);
	extern "C" __declspec(dllexport) uintptr_t GetModuleBaseAddress(DWORD procId, const char* nameAsChar);
	extern "C" __declspec(dllexport) uintptr_t GrabProcessByName(const char* nameAsChar);
	extern "C" __declspec(dllexport) void write(const uintptr_t process_id, const uintptr_t address, const uintptr_t& buffer, PNTSTATUS out_status = 0);
	extern "C" __declspec(dllexport) uintptr_t read(const uintptr_t process_id, const uintptr_t address, PNTSTATUS out_status = 0);

	template <typename T>
	__declspec(dllexport) T read2(const uintptr_t process_id, const uintptr_t address, PNTSTATUS out_status = 0)
	{
		T buffer{ };
		NTSTATUS status = read_memory(process_id, address, uintptr_t(&buffer), sizeof(T));
		if (out_status)
			*out_status = status;
		return buffer;
	}
}

