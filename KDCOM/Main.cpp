#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 0

#include <ntddk.h>
#include <stdio.h>

//
// Adapted from the VirtualKD code:
//

extern "C" void HalInitSystem(void*, void*);

#ifndef _AMD64_
extern "C" void __declspec(naked) __stdcall DllEntryPoint()
{
	__asm
	{
		jmp HalInitSystem
	}
}
#endif

#define DEFINE_EMPTYFUNC0(name) NTSTATUS name() {return STATUS_SUCCESS;}
#define DEFINE_EMPTYFUNC1(name) NTSTATUS name(void *) {return STATUS_SUCCESS;}

extern "C"
{
	DEFINE_EMPTYFUNC0(KdD0Transition)
	DEFINE_EMPTYFUNC0(KdD3Transition)
	DEFINE_EMPTYFUNC1(KdDebuggerInitialize1)

	DEFINE_EMPTYFUNC1(KdRestore)
	DEFINE_EMPTYFUNC1(KdSave)

	NTSTATUS KdSetHiberRange(void*)
	{
		return STATUS_NOT_SUPPORTED;	//As in KDCOM.DLL
	}
}

extern "C" NTSTATUS _stdcall DriverEntry(
	IN OUT PDRIVER_OBJECT   DriverObject,
	IN PUNICODE_STRING      RegistryPath
)
{
	return STATUS_SUCCESS;
}

extern "C" NTSTATUS __stdcall KdPower(void*, void*)
{
	return STATUS_NOT_SUPPORTED;
}

//

#pragma pack (push, 2)

//! Values returned by KdReceivePacket
typedef enum _KD_RECV_CODE
{
	KD_RECV_CODE_OK = 0,
	KD_RECV_CODE_TIMEOUT = 1,
	KD_RECV_CODE_FAILED = 2
} KD_RECV_CODE, * PKD_RECV_CODE;

//! Possible packet types used by KdSendPacket()/KdReceivePacket()
enum
{
	KdPacketType2_Ie_StateManipulate = 2,
	KdPacketType3 = 3,
	//! Sent when a data packet was successfully received
	KdPacketAcknowledge = 4,
	//! Sent when a corrupted data packet is received (or it cannot be processed now)
	KdPacketRetryRequest = 5,
	//! Sent by WinDBG to resynchronize target and by target to acknowledge resync
	KdPacketResynchronize = 6,
	KdPacketType7_Ie_StateChange64 = 7,
	//! Not a packet type. When specified to KdReceivePacket(), it checks whether any data is available on COM port.
	KdCheckForAnyPacket = 8,
	KdPacketType11_Ie_FileIO = 11,
};

//! Represents a buffer used by KdSendPacket()/KdReceivePacket().
typedef struct _KD_BUFFER
{
	USHORT  Length;
	USHORT  MaxLength;
#ifdef _AMD64_
	ULONG	__Padding;
#endif
	PUCHAR  pData;
} KD_BUFFER, * PKD_BUFFER;

//! Represents the global state of the KD packet layer
typedef struct _KD_CONTEXT
{
	//! Specifies the packet retry count for KdSendPacket
	ULONG   RetryCount;
	//! Set to TRUE, if the debugger requested breaking execution
	BOOLEAN BreakInRequested;
} KD_CONTEXT, * PKD_CONTEXT;

extern "C"
{
	KD_RECV_CODE
		__stdcall
		KdReceivePacket(
			__in ULONG PacketType,
			__inout_opt PKD_BUFFER FirstBuffer,
			__inout_opt PKD_BUFFER SecondBuffer,
			__out_opt PULONG PayloadBytes,
			__inout_opt PKD_CONTEXT KdContext
		);

	VOID
		__stdcall
		KdSendPacket(
			__in ULONG PacketType,
			__in PKD_BUFFER FirstBuffer,
			__in_opt PKD_BUFFER SecondBuffer,
			__inout PKD_CONTEXT KdContext
		);

}

extern "C"
{
	//! Performs initial KD extension DLL initialization
	NTSTATUS __stdcall KdDebuggerInitialize0(PVOID lpLoaderParameterBlock);
}

#pragma pack (pop)

//
// ------------------------------------------------------------
//

extern "C" typedef VOID(__stdcall* PFNKdSendPacket)
(__in ULONG PacketType, __in PKD_BUFFER FirstBuffer, __in_opt PKD_BUFFER SecondBuffer, __inout PKD_CONTEXT KdContext);

extern "C" typedef KD_RECV_CODE(__stdcall* PFNKdReceivePacket)
(__in ULONG PacketType, __inout_opt PKD_BUFFER FirstBuffer, __inout_opt PKD_BUFFER SecondBuffer, __out_opt PULONG PayloadBytes, __inout_opt PKD_CONTEXT KdContext);

extern "C" typedef VOID(__stdcall* PFNKdSetBugCheckerCallbacks)
(__in PFNKdSendPacket pfnSend, __in PFNKdReceivePacket pfnReceive);

//

class Callbacks
{
public:
	static PFNKdSendPacket pfnSend;
	static PFNKdReceivePacket pfnReceive;
};

PFNKdSendPacket Callbacks::pfnSend = NULL;
PFNKdReceivePacket Callbacks::pfnReceive = NULL;

extern "C" VOID __stdcall KdSetBugCheckerCallbacks(__in PFNKdSendPacket pfnSend, __in PFNKdReceivePacket pfnReceive)
{
	Callbacks::pfnSend = pfnSend;
	Callbacks::pfnReceive = pfnReceive;
}

//

#define DbgKdContinueApi                    0x00003136

typedef struct _DBGKD_CONTINUE
{
	NTSTATUS ContinueStatus;
} DBGKD_CONTINUE, * PDBGKD_CONTINUE;

typedef struct _DBGKD_MANIPULATE_STATE64
{
	ULONG ApiNumber;
	USHORT ProcessorLevel;
	USHORT Processor;
	NTSTATUS ReturnStatus;
	union
	{
		DBGKD_CONTINUE Continue;
	} u;
} DBGKD_MANIPULATE_STATE64, * PDBGKD_MANIPULATE_STATE64;

extern "C" KD_RECV_CODE
__stdcall
KdReceivePacket( // N.B. NTOSKRNL will call us at CLOCK_LEVEL when polling and at HIGH_LEVEL after a State Change!
	__in ULONG PacketType,
	__inout_opt PKD_BUFFER FirstBuffer,
	__inout_opt PKD_BUFFER SecondBuffer,
	__out_opt PULONG PayloadBytes,
	__inout_opt PKD_CONTEXT KdContext
)
{
	auto pfn = Callbacks::pfnReceive;

	if (pfn)
		return pfn(PacketType, FirstBuffer, SecondBuffer, PayloadBytes, KdContext);

	if (*KdDebuggerNotPresent)
	{
		*KdDebuggerNotPresent = FALSE;
		SharedUserData->KdDebuggerEnabled |= 0x2;
	}

	//

	if (PacketType == KdPacketType11_Ie_FileIO &&
		FirstBuffer != NULL &&
		FirstBuffer->pData != NULL && FirstBuffer->MaxLength >= sizeof(ULONG) * 2) // expects DBGKD_FILE_IO, api is: DbgKdCreateFileApi etc...
	{
		// write the Status field and buffer len
		*((ULONG*)FirstBuffer->pData + 1) = STATUS_UNSUCCESSFUL;
		FirstBuffer->Length = sizeof(ULONG) * 2;

		// packet is received
		return KD_RECV_CODE_OK;
	}

	if (PacketType == KdPacketType2_Ie_StateManipulate &&
		FirstBuffer != NULL &&
		FirstBuffer->pData != NULL && FirstBuffer->MaxLength >= sizeof(DBGKD_MANIPULATE_STATE64))
	{
		DBGKD_MANIPULATE_STATE64* pState = (DBGKD_MANIPULATE_STATE64*)FirstBuffer->pData;
		FirstBuffer->Length = sizeof(DBGKD_MANIPULATE_STATE64);

		::memset(pState, 0, sizeof(DBGKD_MANIPULATE_STATE64));

		//

		pState->ProcessorLevel = 0; // Glob::StateChange.ProcessorLevel;
		pState->Processor = 0; // Glob::StateChange.Processor;

		//

		pState->ApiNumber = DbgKdContinueApi;
		pState->ReturnStatus = DBG_CONTINUE; // DBG_EXCEPTION_HANDLED

		pState->u.Continue.ContinueStatus = DBG_CONTINUE; // DBG_EXCEPTION_HANDLED

		return KD_RECV_CODE_OK;
	}

	return KD_RECV_CODE_TIMEOUT;
}

extern "C" VOID
__stdcall
KdSendPacket(
	__in ULONG PacketType,
	__in PKD_BUFFER FirstBuffer,
	__in_opt PKD_BUFFER SecondBuffer,
	__inout PKD_CONTEXT KdContext
)
{
	auto pfn = Callbacks::pfnSend;

	if (pfn)
		return pfn(PacketType, FirstBuffer, SecondBuffer, KdContext);

	if (*KdDebuggerNotPresent)
	{
		*KdDebuggerNotPresent = FALSE;
		SharedUserData->KdDebuggerEnabled |= 0x2;
	}

	return;
}

extern "C" NTSTATUS __stdcall KdDebuggerInitialize0(PVOID lpLoaderParameterBlock)
{
	return STATUS_SUCCESS;
}

extern "C" NTSTATUS __stdcall KdInitialize(int mode, PVOID lpLoaderParameterBlock, void*)
{
	if (mode == 0)
		return KdDebuggerInitialize0(lpLoaderParameterBlock);
	else
		return STATUS_SUCCESS;
}
