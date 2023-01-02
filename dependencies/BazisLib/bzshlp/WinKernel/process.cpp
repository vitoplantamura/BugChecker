#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "process.h"

#pragma region Undocumented API
typedef struct _KAPC_STATE {
	LIST_ENTRY ApcListHead[MaximumMode];
	struct _KPROCESS *Process;
	BOOLEAN KernelApcInProgress;
	BOOLEAN KernelApcPending;
	BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *PRKAPC_STATE;

typedef struct _RTL_USER_PROCESS_PARAMETERS { 
	UCHAR  Reserved1[16];  
	PVOID Reserved2[10];  
	UNICODE_STRING ImagePathName;  
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS,  *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB {
	UCHAR  Reserved1[2]; 
	UCHAR  BeingDebugged; 
	UCHAR  Reserved2[1];
	PVOID Reserved3[2];  
	PVOID Ldr; 
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	UCHAR  Reserved4[104]; 
	PVOID Reserved5[52]; 
	PVOID PostProcessInitRoutine; 
	UCHAR  Reserved6[128];  
	PVOID Reserved7[1]; 
	ULONG SessionId;
} PEB,  *PPEB;


extern "C" 
{
	NTKERNELAPI VOID KeStackAttachProcess (__inout struct _KPROCESS * PROCESS, __out PRKAPC_STATE ApcState);
	NTKERNELAPI VOID KeUnstackDetachProcess (__in PRKAPC_STATE ApcState);
}
#pragma endregion

BazisLib::DDK::ProcessAccessor::ProcessAccessor( HANDLE ProcessID, ActionStatus *pStatus /*= NULL*/ )
	: _hProcess(0)
	, _pProcess(NULL)
{
	OBJECT_ATTRIBUTES objAttr = {0,};
	CLIENT_ID id = {ProcessID, 0};
	objAttr.Attributes = OBJ_KERNEL_HANDLE;

	NTSTATUS st = ZwOpenProcess(&_hProcess, PROCESS_ALL_ACCESS, &objAttr, &id);
	if (!NT_SUCCESS(st))
	{
		ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(st));
		return;
	}

	st = ObReferenceObjectByHandle(_hProcess, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, (PVOID *)&_pProcess, NULL);
	if (!NT_SUCCESS(st))
	{
		ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(st));
		return;
	}

	ASSIGN_STATUS(pStatus, Success);
}

BazisLib::DDK::ProcessAccessor::~ProcessAccessor()
{
	if (_pProcess)
		ObDereferenceObject(_pProcess);
	if (_hProcess)
		ZwClose(_hProcess);
}

BazisLib::ActionStatus BazisLib::DDK::ProcessAccessor::QueryBasicInfo( PROCESS_BASIC_INFORMATION *pInfo )
{
	if (!_hProcess)
		return MAKE_STATUS(InvalidState);
	if (!pInfo)
		return MAKE_STATUS(InvalidPointer);

	ULONG done;
	NTSTATUS st = ZwQueryInformationProcess(_hProcess, ProcessBasicInformation, pInfo, sizeof(*pInfo), &done);
	if (!NT_SUCCESS(st))
		return MAKE_STATUS(ActionStatus::FromNTStatus(st));

	if (done != sizeof(*pInfo))
		return MAKE_STATUS(UnknownError);

	return MAKE_STATUS(Success);
}

BazisLib::ActionStatus BazisLib::DDK::ProcessAccessor::ReadMemory( const void *pAddressInProcess, void *pSystemBuffer, size_t size )
{
	if (!_pProcess)
		return MAKE_STATUS(NotInitialized);

	//Check that pSystemBuffer is actually a kernel-space pointer
	ASSERT((ULONG_PTR)pSystemBuffer & (1ULL << ((sizeof(void *) * 8) - 1)));

	KAPC_STATE apcState;
	KeStackAttachProcess(_pProcess, &apcState);

	memcpy(pSystemBuffer, pAddressInProcess, size);

	KeUnstackDetachProcess(&apcState);
	return MAKE_STATUS(Success);
}

BazisLib::DDK::ProcessAccessor::StartInfo BazisLib::DDK::ProcessAccessor::QueryStartInfo()
{
	PROCESS_BASIC_INFORMATION basicInfo;
	StartInfo info;
	if (!QueryBasicInfo(&basicInfo).Successful())
		return info;

	KAPC_STATE apcState;
	KeStackAttachProcess(_pProcess, &apcState);
	ActionStatus st = DoQueryStartInfoFromProcessContext(&basicInfo, &info);
	KeUnstackDetachProcess(&apcState);
	if (!st.Successful())
		return StartInfo();
	return info;
}

BazisLib::ActionStatus BazisLib::DDK::ProcessAccessor::DoQueryStartInfoFromProcessContext( PPROCESS_BASIC_INFORMATION pBasicInfo, StartInfo *pInfo )
{
	//Ensure that we are running in the context of the queried process
	ASSERT(pBasicInfo->UniqueProcessId == (ULONG_PTR)PsGetCurrentProcessId());

	PPEB pPeb = pBasicInfo->PebBaseAddress;
	if (!pPeb)
		return MAKE_STATUS(InvalidPointer);
	if (!pPeb->ProcessParameters)
		return MAKE_STATUS(InvalidPointer);

	pInfo->ImagePath = pPeb->ProcessParameters->ImagePathName;
	pInfo->CommandLine = pPeb->ProcessParameters->CommandLine;
	return MAKE_STATUS(Success);
}
#endif