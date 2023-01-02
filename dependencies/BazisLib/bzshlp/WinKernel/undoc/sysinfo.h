#pragma once
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
	} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

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
} SYSTEM_MODULE, *PSYSTEM_MODULE, *PSYSTEM_MODULE_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION {
  ULONG_PTR            ModulesCount;
  SYSTEM_MODULE        Modules[ANYSIZE_ARRAY];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;
