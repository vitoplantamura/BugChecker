#pragma once

#define IOCTL_BUGCHECKER_GET_VERSION			CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_BUGCHECKER_DO_PHYS_MEM_SEARCH		CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_BUGCHECKER_GET_INIT_RESULT		CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0802, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_UNKNOWN_BASE FILE_DEVICE_UNKNOWN

#define _BUGCHECKER_DRIVER_VERSION_ ( 1 )

typedef struct
{
	ULONGLONG pattern;
	ULONGLONG mask;
	ULONGLONG address;
	ULONGLONG size;

} DoPhysMemSearchInputData;

typedef struct
{
	LONGLONG relativePos; // -1 = not found.

} DoPhysMemSearchOutputData;

typedef struct
{
	BYTE kernPdbGuid[16];
	DWORD kernPdbAge;
	CHAR kernPdbName[256];

	ULONG resultCode;
	CHAR resultString[256];

} BugCheckerInitResult;

enum BcInitError
{
	BcInitError_IoCreateDeviceFailed = 1000,
	BcInitError_IoCreateSymbolicLinkFailed,
	BcInitError_TryGetKernelDebugInfoWithoutSymbolsFailed,
	BcInitError_MmMapIoSpaceFailed,
	BcInitError_FramebufferInfoNotProvided,
	BcInitError_CalculateKernelOffsetsFailed,
	BcInitError_KdcomHookInfoNotProvided,
	BcInitError_CallSetCallbacks_ProcAddress_GetModuleBase_Failed,
	BcInitError_CallSetCallbacks_ProcAddress_GetProcAddress_Failed,
	BcInitError_PatchKdCom_ProcAddress_GetModuleBase_Failed,
	BcInitError_PatchKdCom_ProcAddress_GetProcAddress_KdSendPacket_Failed,
	BcInitError_PatchKdCom_ProcAddress_GetProcAddress_KdReceivePacket_Failed,
	BcInitError_PatchKdCom_FunctionPatch_Patch_KdSendPacket_Failed,
	BcInitError_PatchKdCom_FunctionPatch_Patch_KdReceivePacket_Failed
};
