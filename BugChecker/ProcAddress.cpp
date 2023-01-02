#include "ProcAddress.h"

//=================================================================
//
// This code is from: https://github.com/4d61726b/VirtualKD-Redux/
//
//=================================================================

#include <ntimage.h>

extern "C"
{
	PVOID __stdcall
		RtlImageDirectoryEntryToData(
			IN PVOID Base,
			IN BOOLEAN MappedAsImage,
			IN USHORT DirectoryEntry,
			OUT PULONG Size
		);

	typedef enum _SYSTEM_INFORMATION_CLASS {
		SystemBasicInformation,
		SystemProcessorInformation,
		SystemPerformanceInformation,
		SystemTimeOfDayInformation,
		SystemPathInformation,
		SystemProcessInformation,
		SystemCallCountInformation,
		SystemDeviceInformation,
		SystemProcessorPerformanceInformation,
		SystemFlagsInformation,
		SystemCallTimeInformation,
		SystemModuleInformation,
		SystemLocksInformation,
		SystemStackTraceInformation,
		SystemPagedPoolInformation,
		SystemNonPagedPoolInformation,
		SystemHandleInformation,
		SystemObjectInformation,
		SystemPageFileInformation,
		SystemVdmInstemulInformation,
		SystemVdmBopInformation,
		SystemFileCacheInformation,
		SystemPoolTagInformation,
		SystemInterruptInformation,
		SystemDpcBehaviorInformation,
		SystemFullMemoryInformation,
		SystemLoadGdiDriverInformation,
		SystemUnloadGdiDriverInformation,
		SystemTimeAdjustmentInformation,
		SystemSummaryMemoryInformation,
		SystemNextEventIdInformation,
		SystemEventIdsInformation,
		SystemCrashDumpInformation,
		SystemExceptionInformation,
		SystemCrashDumpStateInformation,
		SystemKernelDebuggerInformation,
		SystemContextSwitchInformation,
		SystemRegistryQuotaInformation,
		SystemExtendServiceTableInformation,
		SystemPrioritySeperation,
		SystemPlugPlayBusInformation,
		SystemDockInformation,
		SystemPowerInformation_,
		SystemProcessorSpeedInformation,
		SystemCurrentTimeZoneInformation,
		SystemLookasideInformation
	} SYSTEM_INFORMATION_CLASS, * PSYSTEM_INFORMATION_CLASS;

	NTSTATUS __stdcall ZwQuerySystemInformation(
		SYSTEM_INFORMATION_CLASS SystemInformationClass,
		PVOID SystemInformation,
		ULONG SystemInformationLength,
		PULONG ReturnLength
	);
}

typedef USHORT WORD;
typedef UCHAR BYTE;

typedef struct _SYSTEM_MODULE {
	ULONG_PTR            Reserved1;
	ULONG_PTR            Reserved2;
	PVOID                ImageBaseAddress;	//+0x10
	ULONG                ImageSize;
	ULONG                Flags;
	WORD                 Id;
	WORD                 Rank;
	WORD                 w018;
	WORD                 NameOffset;
	BYTE                 Name[MAXIMUM_FILENAME_LENGTH];
} SYSTEM_MODULE, * PSYSTEM_MODULE, * PSYSTEM_MODULE_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION {
	ULONG_PTR            ModulesCount;
	SYSTEM_MODULE        Modules[ANYSIZE_ARRAY];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

//! Kernel-mode equivalent of GetModuleHandle()
/*! This function returns the base address of a module loaded into kernel address space (can be a driver
	or a kernel-mode DLL).
	\param pModuleName Specifies the module name as an ANSI null-terminated string.
	\return The function returns the base address of a module, or NULL if it was not found among the loaded modules.
	\remarks The function body was downloaded from <a href="http://alter.org.ua/docs/nt_kernel/procaddr/">here</a>.
*/
PVOID
ProcAddress::GetModuleBase(
	const CHAR* pModuleName
)
{
	PVOID pModuleBase = NULL;
	PULONG pSystemInfoBuffer = NULL;

	__try
	{
		NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
		ULONG    SystemInfoBufferSize = 0;

		status = ZwQuerySystemInformation(SystemModuleInformation,
			&SystemInfoBufferSize,
			0,
			&SystemInfoBufferSize);

		if (!SystemInfoBufferSize)
			return NULL;

		pSystemInfoBuffer = (PULONG)ExAllocatePool(NonPagedPool, SystemInfoBufferSize * 2);

		if (!pSystemInfoBuffer)
			return NULL;

		memset(pSystemInfoBuffer, 0, SystemInfoBufferSize * 2);

		status = ZwQuerySystemInformation(SystemModuleInformation,
			pSystemInfoBuffer,
			SystemInfoBufferSize * 2,
			&SystemInfoBufferSize);

		if (NT_SUCCESS(status))
		{
			PSYSTEM_MODULE_ENTRY pSysModuleEntry =
				((PSYSTEM_MODULE_INFORMATION)(pSystemInfoBuffer))->Modules;
			ULONG i;

			for (i = 0; i < ((PSYSTEM_MODULE_INFORMATION)(pSystemInfoBuffer))->ModulesCount; i++)
			{
				if (_stricmp((char*)pSysModuleEntry[i].Name +
					pSysModuleEntry[i].NameOffset, pModuleName) == 0)
				{
					pModuleBase = pSysModuleEntry[i].ImageBaseAddress;
					break;
				}
			}
		}

	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		pModuleBase = NULL;
	}
	if (pSystemInfoBuffer) {
		ExFreePool(pSystemInfoBuffer);
	}

	return pModuleBase;
} // end ProcAddress::GetModuleBase()

//! Kernel-mode equivalent of GetProcAddress()
/*! This function returns the address of a function exported by a module loaded into kernel address space.
	\param ModuleBase Specifies the module base address (can be determined by calling KernelGetModuleBase()).
	\param pFunctionName Specifies the function name as an ANSI null-terminated string.
	\return The function returns the address of an exported function, or NULL if it was not found.
	\remarks The function body was downloaded from <a href="http://alter.org.ua/docs/nt_kernel/procaddr/">here</a>.
*/
PVOID
ProcAddress::GetProcAddress(
	PVOID ModuleBase,
	const CHAR* pFunctionName
)
{
	ASSERT(ModuleBase && pFunctionName);
	PVOID pFunctionAddress = NULL;

	ULONG size = 0;
	PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)
		RtlImageDirectoryEntryToData(ModuleBase, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size);

#pragma warning(push)
#pragma warning(disable: 4311)
#pragma warning(disable: 4302)
#pragma warning(disable: 4312)
	ULONG_PTR addr = (ULONG_PTR)(PUCHAR)((ULONG)exports - (ULONG)ModuleBase);
#pragma warning(pop)

	PULONG functions = (PULONG)((ULONG_PTR)ModuleBase + exports->AddressOfFunctions);
	PSHORT ordinals = (PSHORT)((ULONG_PTR)ModuleBase + exports->AddressOfNameOrdinals);
	PULONG names = (PULONG)((ULONG_PTR)ModuleBase + exports->AddressOfNames);
	ULONG  max_name = exports->NumberOfNames;
	ULONG  max_func = exports->NumberOfFunctions;

	ULONG i;

	for (i = 0; i < max_name; i++)
	{
		ULONG ord = ordinals[i];
		if (i >= max_name || ord >= max_func) {
			return NULL;
		}
		if (functions[ord] < addr || functions[ord] >= addr + size)
		{
			if (strcmp((PCHAR)ModuleBase + names[i], pFunctionName) == 0)
			{
				pFunctionAddress = (PVOID)((PCHAR)ModuleBase + functions[ord]);
				break;
			}
		}
	}
	return pFunctionAddress;
} // end ProcAddress::GetProcAddress()
