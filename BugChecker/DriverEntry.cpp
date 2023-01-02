#include "BugChecker.h"

#include "MemReadCor.h"
#include "ProcAddress.h"
#include "KdCom.h"
#include "DbgKd.h"
#include "Ps2Keyb.h"
#include "Glyph.h"
#include "X86Step.h"
#include "Platform.h"
#include "Utils.h"
#include "Symbols.h"
#include "Ioctl.h"
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/sort.h>
#include <EASTL/functional.h>
#include <ntimage.h>
#include "BcCoroutine.h"
#include "Root.h"
#include "Wnd.h"
#include "Main.h"
#include "Cmd.h"

//

static unsigned __int64 recvCalls = 0;
static unsigned __int64 sendCalls = 0;

static char str1[120] = "";

char g_debugStr[256] = "";

//

extern "C" static KD_RECV_CODE
__stdcall
KdReceivePacket( // N.B. NTOSKRNL will call us at CLOCK_LEVEL when polling and at HIGH_LEVEL after a State Change!
	__in ULONG PacketType,
	__inout_opt PKD_BUFFER FirstBuffer,
	__inout_opt PKD_BUFFER SecondBuffer,
	__out_opt PULONG PayloadBytes,
	__inout_opt PKD_CONTEXT KdContext
)
{
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

	//

	recvCalls++;

	if (!Root::I->DebuggerPresentSet && Root::I->VideoAddr)
	{
		if (*KdDebuggerNotPresent) // NOTE: we already set this in our custom KDCOM.DLL.
		{
			*KdDebuggerNotPresent = FALSE;
			SharedUserData->KdDebuggerEnabled |= 0x2; // WARNING, implemented in Windows 11 22H2: https://msrc-blog.microsoft.com/2022/04/05/randomizing-the-kuser_shared_data-structure-on-windows/
		}

		Root::I->DebuggerPresentSet = TRUE;
	}

	if (Root::I->bcDebug && Root::I->VideoAddr)
	{
		static CHAR szText[sizeof(g_debugStr) + 128];
		::sprintf(szText, "recvCalls => %x, (%s) ", (int)recvCalls, g_debugStr);
		Glyph::DrawStr(szText, 0x1F, 0, 0, Root::I->VideoAddr);
	}

	//

	if (PacketType == KdCheckForAnyPacket)
	{
		if (Root::I->VideoRestoreBufferTimer &&
			__rdtsc() - Root::I->VideoRestoreBufferTimer > Root::I->VideoRestoreBufferTimeOut)
		{
			Root::I->VideoRestoreBufferTimer = 0;
			Wnd::SaveOrRestoreFrameBuffer(TRUE);
		}
	}

	if (PacketType == KdPacketType2_Ie_StateManipulate &&
		FirstBuffer != NULL &&
		FirstBuffer->pData != NULL && FirstBuffer->MaxLength >= sizeof(DBGKD_MANIPULATE_STATE64))
	{
		while (TRUE)
		{
			DBGKD_MANIPULATE_STATE64* pState = (DBGKD_MANIPULATE_STATE64*)FirstBuffer->pData;
			FirstBuffer->Length = sizeof(DBGKD_MANIPULATE_STATE64);

			::memset(pState, 0, sizeof(DBGKD_MANIPULATE_STATE64));

			pState->ProcessorLevel = Root::I->StateChange.ProcessorLevel;
			pState->Processor = Root::I->StateChange.Processor;

			switch (Root::I->DebuggerState)
			{
			case DEBST_STATE_CHANGED:
			case DEBST_COROUTINE:
			case DEBST_PROCESS_AWAITER:
			{
				if (Root::I->DebuggerState == DEBST_STATE_CHANGED)
				{
					if (Root::I->AwaiterPtr) Allocator::FatalError("UNEXPECTED_STATE_CHANGE");

					Main::EntryPoint().Start(&Root::I->AwaiterPtr);
				}
				else if (Root::I->DebuggerState == DEBST_COROUTINE)
					BcAwaiterBase::Resume(Root::I->AwaiterPtr);

				switch (BcAwaiterBase::ProcessAwaiter(pState, SecondBuffer, PayloadBytes))
				{
				case ProcessAwaiterRetVal::Ok:
					return KD_RECV_CODE_OK;
				case ProcessAwaiterRetVal::Timeout:
					return KD_RECV_CODE_TIMEOUT;
				case ProcessAwaiterRetVal::RepeatLoop:
					continue;
				}

				*((volatile int*)0) = 0xDEADC0DE;
			}
			break;

			case DEBST_MEMREADCOR:
			{
				BOOLEAN cpp20Coro = !Root::I->MemReadCor.pCoroutine && Root::I->AwaiterPtr && !::strcmp(Root::I->AwaiterPtr->function, "BcAwaiter_ReadMemory");

				const ULONG _chunkSize_ = 1024;
				ULONG btr = Root::I->MemReadCor.BytesToRead - Root::I->MemReadCor.BufferPos;

				if (btr <= _chunkSize_)
				{
					if (btr && Root::I->MemReadCor.ActualBytesRead == btr)
						Root::I->MemReadCor.ActualBytesRead = Root::I->MemReadCor.BytesToRead;

					if (cpp20Coro) // C++20 coroutines case.
					{
						BcAwaiter_ReadMemory* ptr = (BcAwaiter_ReadMemory*)Root::I->AwaiterPtr;

						Root::I->MemReadCor.ResumePoint--; // initialized to 2 before starting.

						if (Root::I->MemReadCor.ResumePoint == 1)
						{
							Root::I->MemReadCor.Pointer = (VOID*)ptr->pointer;
							Root::I->MemReadCor.BytesToRead = ptr->size;
						}
						else if (Root::I->MemReadCor.ResumePoint <= 0)
						{
							if (Root::I->MemReadCor.ActualBytesRead == Root::I->MemReadCor.BytesToRead)
								ptr->retVal = Root::I->MemReadCor.Buffer;
						}
					}
					else // old BugChecker custom coroutines case.
					{
						Root::I->MemReadCor.pCoroutine(&Root::I->MemReadCor);
					}

					if (Root::I->MemReadCor.ResumePoint <= 0 ||
						Root::I->MemReadCor.BytesToRead > sizeof(Root::I->MemReadCor.Buffer))
					{
						Root::I->DebuggerState = Root::I->MemReadCor.EndDebuggerState;
						continue;
					}
					else
					{
						btr = Root::I->MemReadCor.BytesToRead;
						Root::I->MemReadCor.BufferPos = 0;
					}
				}
				else
				{
					btr -= _chunkSize_;
					Root::I->MemReadCor.BufferPos += _chunkSize_;
				}

				pState->ApiNumber = DbgKdReadVirtualMemoryApi;

				pState->ReturnStatus = STATUS_PENDING;

				pState->u.ReadMemory.TargetBaseAddress = (ULONG64)Root::I->MemReadCor.Pointer + Root::I->MemReadCor.BufferPos;
				pState->u.ReadMemory.TransferCount = btr > _chunkSize_ ? _chunkSize_ : btr;
				pState->u.ReadMemory.ActualBytesRead = 0;
			}
			break;

			case DEBST_CONTINUE:
			default:
			{
				if (!Root::I->Trace)
				{
					pState->ApiNumber = DbgKdContinueApi;
					pState->ReturnStatus = DBG_CONTINUE; // DBG_EXCEPTION_HANDLED

					pState->u.Continue.ContinueStatus = DBG_CONTINUE; // DBG_EXCEPTION_HANDLED
				}
				else
				{
					pState->ApiNumber = DbgKdContinueApi2;
					pState->ReturnStatus = DBG_CONTINUE;

					pState->u.Continue2.ContinueStatus = DBG_CONTINUE;
					pState->u.Continue2.AnyControlSet._6432_(Amd64ControlSet, X86ControlSet).TraceFlag = TRUE;
				}

				Root::I->Trace = FALSE;

				Root::I->VideoRestoreBufferTimer = __rdtsc();
			}
			break;
			}

			return KD_RECV_CODE_OK;
		}
	}

	//

	return KD_RECV_CODE_TIMEOUT;
}

extern "C" static VOID
__stdcall
KdSendPacket(
	__in ULONG PacketType,
	__in PKD_BUFFER FirstBuffer,
	__in_opt PKD_BUFFER SecondBuffer,
	__inout PKD_CONTEXT KdContext
)
{
	if (PacketType == KdPacketType2_Ie_StateManipulate)
	{
		if (FirstBuffer->pData != NULL && FirstBuffer->Length > 0)
		{
			DBGKD_MANIPULATE_STATE64* pState = (DBGKD_MANIPULATE_STATE64*)FirstBuffer->pData;

			if (Root::I->DebuggerState == DEBST_COROUTINE)
			{
				if (!::strcmp(Root::I->AwaiterPtr->function, "BcAwaiter_StateManipulate"))
				{
					BcAwaiter_StateManipulate* ptr = (BcAwaiter_StateManipulate*)Root::I->AwaiterPtr;
					ptr->retVal = ptr->processResponse(pState, SecondBuffer);
				}
			}
			else if (pState->ApiNumber == DbgKdReadVirtualMemoryApi && Root::I->DebuggerState == DEBST_MEMREADCOR)
			{
				if (NT_SUCCESS(pState->ReturnStatus) &&
					SecondBuffer->pData && SecondBuffer->Length > 0 &&
					pState->u.ReadMemory.ActualBytesRead == SecondBuffer->Length)
				{
					::memcpy(Root::I->MemReadCor.Buffer + Root::I->MemReadCor.BufferPos, SecondBuffer->pData, SecondBuffer->Length);

					Root::I->MemReadCor.ActualBytesRead = SecondBuffer->Length;
				}
				else
				{
					Root::I->MemReadCor.ActualBytesRead = 0;
				}
			}
		}
	}

	if (PacketType == KdPacketType7_Ie_StateChange64 &&
		FirstBuffer != NULL &&
		FirstBuffer->pData != NULL && FirstBuffer->Length > 0)
	{
		Root::I->StateChange = *(DBGKD_ANY_WAIT_STATE_CHANGE*)FirstBuffer->pData;

		Root::I->DebuggerState = DEBST_STATE_CHANGED;
	}

	if (PacketType == KdPacketType3_Ie_DebugIO &&
		FirstBuffer != NULL &&
		FirstBuffer->pData != NULL && FirstBuffer->Length > 0 &&
		SecondBuffer != NULL &&
		SecondBuffer->pData != NULL && SecondBuffer->Length > 0)
	{
		auto debugIo = (DBGKD_DEBUG_IO*)FirstBuffer->pData;

		if (debugIo->ApiNumber == DbgKdPrintStringApi &&
			debugIo->u.PrintString.LengthOfString <= SecondBuffer->Length)
		{
			if (Root::I)
			{
				eastl::string s((CHAR*)SecondBuffer->pData, debugIo->u.PrintString.LengthOfString);

				for (int i = 0; i < s.size(); i++)
					if (s[i] < 32)
						s[i] = ' ';
					else if (s[i] >= 128)
						s[i] = '?';

				Root::I->LogWindow.AddString(s.c_str());
			}
		}
	}

	sendCalls++;

	if (sendCalls == 1 && !Root::I->DebuggerPresentSet)
	{
		KD_DEBUGGER_NOT_PRESENT = TRUE;
		SharedUserData->KdDebuggerEnabled &= ~0x2;
	}

	if (Root::I->bcDebug)
	{
		static char str[256];
		::sprintf(str, "%X", PacketType == KdPacketType7_Ie_StateChange64 ? Root::I->StateChange.NewState : PacketType);

		if (PacketType == KdPacketType2_Ie_StateManipulate)
			::sprintf(str + ::strlen(str), "(%X)", ((DBGKD_MANIPULATE_STATE64*)FirstBuffer->pData)->ApiNumber);
		else if (PacketType == KdPacketType7_Ie_StateChange64)
			::sprintf(str + ::strlen(str), "(%X)", Root::I->StateChange.u.Exception.ExceptionRecord.ExceptionCode);

		::strcat(str, ",");
		if (::strlen(str1) + ::strlen(str) < sizeof(str1) - 16)
			::strcat(str1, str);
		else
			::strcpy(str1, str);

		if (Root::I->VideoAddr)
		{
			static CHAR szText[512]; // more than str1
			::sprintf(szText, "sendCalls => %x, ", (int)sendCalls);
			::strcat(szText, str1);
			Glyph::DrawStr(szText, 0x1F, 0, 1, Root::I->VideoAddr);
		}
	}
}

///////////////////////////////////////////////////////////////////////

static BugCheckerInitResult BcInitResult = {};

static VOID ReportInitError(ULONG code, const CHAR* msg)
{
	::DbgPrint(msg);

	// save the error: Symbol Loader may request this info later.

	auto& init = BcInitResult;

	init.resultCode = code;

	size_t s = _MIN_(sizeof(init.resultString) - 1, ::strlen(msg));

	::memcpy(init.resultString, msg, s);
	init.resultString[s] = 0;
}

#if 0
static VOID PrintInitErrorOnScreen(const CHAR* psz)
{
	if (!Root::I || !Root::I->VideoAddr) return;

	// write continuously the string for several seconds: the OS may overwrite it.

	LARGE_INTEGER start, freq;

	start = ::KeQueryPerformanceCounter(&freq);

	while (::KeQueryPerformanceCounter(NULL).QuadPart - start.QuadPart < freq.QuadPart * 4)
		Glyph::DrawStr(psz, 0x4F, 0, 0, Root::I->VideoAddr);
}
#endif

static VOID CallSetCallbacks(BOOLEAN set)
{
	auto lpKD = ProcAddress::GetModuleBase("KDCOM.DLL");
	if (!lpKD) lpKD = ProcAddress::GetModuleBase("KDBC.DLL");

	if (!lpKD)
		::ReportInitError(BcInitError_CallSetCallbacks_ProcAddress_GetModuleBase_Failed, "CallSetCallbacks::ProcAddress::GetModuleBase of KDCOM.DLL and KDBC.DLL failed.");
	else
	{
		auto pfSet = (PFNKdSetBugCheckerCallbacks)ProcAddress::GetProcAddress(lpKD, "KdSetBugCheckerCallbacks");

		if (!pfSet)
			::ReportInitError(BcInitError_CallSetCallbacks_ProcAddress_GetProcAddress_Failed, "CallSetCallbacks::ProcAddress::GetProcAddress of KdSetBugCheckerCallbacks failed.");
		else
		{
			pfSet(
				set ? KdSendPacket : NULL,
				set ? KdReceivePacket : NULL);
		}
	}
}

static VOID PatchKdCom()
{
	auto lpKD = ProcAddress::GetModuleBase("KDCOM.DLL");
	if (!lpKD) lpKD = ProcAddress::GetModuleBase("KDBC.DLL");

	if (!lpKD)
		::ReportInitError(BcInitError_PatchKdCom_ProcAddress_GetModuleBase_Failed, "PatchKdCom::ProcAddress::GetModuleBase of KDCOM.DLL and KDBC.DLL failed.");
	else
	{
		auto pfSend = ProcAddress::GetProcAddress(lpKD, "KdSendPacket");
		auto pfRecv = ProcAddress::GetProcAddress(lpKD, "KdReceivePacket");

		if (!pfSend)
			::ReportInitError(BcInitError_PatchKdCom_ProcAddress_GetProcAddress_KdSendPacket_Failed, "PatchKdCom::ProcAddress::GetProcAddress of KdSendPacket failed.");
		else if (!pfRecv)
			::ReportInitError(BcInitError_PatchKdCom_ProcAddress_GetProcAddress_KdReceivePacket_Failed, "PatchKdCom::ProcAddress::GetProcAddress of KdReceivePacket failed.");
		else
		{
			Root::I->KdSendPacketFp = new FunctionPatch();
			Root::I->KdReceivePacketFp = new FunctionPatch();

			if (!Root::I->KdSendPacketFp->Patch(pfSend, KdSendPacket))
				::ReportInitError(BcInitError_PatchKdCom_FunctionPatch_Patch_KdSendPacket_Failed, "PatchKdCom::FunctionPatch::Patch of KdSendPacket failed.");
			if (!Root::I->KdReceivePacketFp->Patch(pfRecv, KdReceivePacket))
				::ReportInitError(BcInitError_PatchKdCom_FunctionPatch_Patch_KdReceivePacket_Failed, "PatchKdCom::FunctionPatch::Patch of KdReceivePacket failed.");
		}
	}
}

static NTSTATUS DrvDispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	Irp->IoStatus.Status = ntStatus;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return ntStatus;
}

static NTSTATUS DrvDispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

static NTSTATUS DrvDispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

	// process the ioctl.

	switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_BUGCHECKER_GET_VERSION:
	{
		if (irpStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(DWORD))
		{
			DWORD* pl = (DWORD*)Irp->AssociatedIrp.SystemBuffer;
			*pl = (DWORD)_BUGCHECKER_DRIVER_VERSION_;

			ntStatus = STATUS_SUCCESS;
		}
	}
	break;

	case IOCTL_BUGCHECKER_DO_PHYS_MEM_SEARCH:
	{
		if (irpStack->Parameters.DeviceIoControl.InputBufferLength == sizeof(DoPhysMemSearchInputData) &&
			irpStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(DoPhysMemSearchOutputData))
		{
			DoPhysMemSearchOutputData out{ -1 };

			auto in = (DoPhysMemSearchInputData*)Irp->AssociatedIrp.SystemBuffer;

			// attempt the search.

			PHYSICAL_ADDRESS pAddr;
			pAddr.QuadPart = in->address;

			auto mem = ::MmMapIoSpace(pAddr, in->size, MmNonCached);

			if (mem)
			{
				ULONGLONG* p = (ULONGLONG*)mem;

				for (int i = 0; i < in->size; i += sizeof(in->pattern), p++)
				{
					if ((*p & in->mask) == in->pattern)
					{
						out.relativePos = i;
						break;
					}
				}

				::MmUnmapIoSpace(mem, in->size);
			}

			// return the response.

			*(DoPhysMemSearchOutputData*)Irp->AssociatedIrp.SystemBuffer = out;
			ntStatus = STATUS_SUCCESS;
		}
	}
	break;

	case IOCTL_BUGCHECKER_GET_INIT_RESULT:
	{
		if (irpStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(BugCheckerInitResult))
		{
			auto p = (BugCheckerInitResult*)Irp->AssociatedIrp.SystemBuffer;
			*p = BcInitResult;

			ntStatus = STATUS_SUCCESS;
		}
	}
	break;
	}

	// return.

	Irp->IoStatus.Status = ntStatus;

	if (ntStatus == STATUS_SUCCESS)
		Irp->IoStatus.Information = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	else
		Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return ntStatus;
}

#ifdef _AMD64_
extern "C" VOID BreakInAndDeleteBreakPoints();
#else
extern "C" __declspec(noinline) __declspec(naked) VOID BreakInAndDeleteBreakPoints()
{
	__asm
	{
		int 3
		ret
	}
}
#endif

static VOID DrvUnload(PDRIVER_OBJECT  DriverObject)
{
	// initial BC shutdown.

	if (Root::I)
	{
		// delete all the BreakPoints. (MUST BE THE FIRST THING TO DO)

		if (Root::I->BreakPoints.size())
			::BreakInAndDeleteBreakPoints();

		// remove the notify routines.

		if (Root::I->ProcessNotifyCreated)
		{
			Root::I->ProcessNotifyCreated = FALSE;
			::PsSetCreateProcessNotifyRoutine(&Platform::CreateProcessNotifyRoutine, TRUE);
		}

		if (Root::I->ImageNotifyCreated)
		{
			Root::I->ImageNotifyCreated = FALSE;
			::PsRemoveLoadImageNotifyRoutine(&Platform::LoadImageNotifyRoutine);
		}

		// unmap the vm command buffer.

		if (Root::I->VmFifo)
		{
			::MmUnmapIoSpace(Root::I->VmFifo, Root::I->VmFifoSize);
			Root::I->VmFifo = NULL;
		}

		// unhook kdcom.

		if (Root::I->kdcomHook == KdcomHook::Callback)
			::CallSetCallbacks(FALSE);
		else if (Root::I->kdcomHook == KdcomHook::Patch)
		{
			if (Root::I->KdSendPacketFp && Root::I->KdReceivePacketFp)
			{
				delete Root::I->KdSendPacketFp;
				delete Root::I->KdReceivePacketFp;
			}
		}
	}

	// usual driver shutdown.

	UNICODE_STRING usDosDeviceName;
	::RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\BugChecker");

	::IoDeleteSymbolicLink(&usDosDeviceName);
	::IoDeleteDevice(DriverObject->DeviceObject);

	// final BC shutdown.

	if (Root::I)
	{
		// unmap video memory.

		if (Root::I->VideoAddr)
		{
			::MmUnmapIoSpace(Root::I->VideoAddr, Root::I->fbMapSize);
			Root::I->VideoAddr = NULL;
		}

		// delete the Root object.

		delete Root::I;
		Root::I = NULL;
	}

	Allocator::Uninit(); // must be the last thing to do before driver unload!
}

//

#define _CRTALLOC(x) __declspec(allocate(x))

typedef void (__cdecl* _PVFV)(void);

#pragma const_seg(".CRT$XCA")
extern _CRTALLOC(".CRT$XCA") _PVFV __xc_a[] = { NULL };
#pragma const_seg(".CRT$XCZ")
extern _CRTALLOC(".CRT$XCZ") _PVFV __xc_z[] = { NULL };
#pragma const_seg()

#pragma comment(linker, "/merge:.CRT=.rdata")

//

CHAR BcVersion[256] = "";

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT  pDriverObject, PUNICODE_STRING  /*pRegistryPath*/)
{
	// print a message.

	::sprintf(BcVersion, "BugChecker %s version 0.1; compiled on %s %s.", _6432_("amd64", "x86"), __DATE__, __TIME__);

	::DbgPrint(BcVersion);

	// execute static initialization routines. >>> WARNING <<< we support only construction, not destruction!

	_PVFV* pfbegin = __xc_a;
	_PVFV* pfend = __xc_z;

	while (pfbegin < pfend)
	{
		if (*pfbegin != NULL)
			(**pfbegin)();
		++pfbegin;
	}

	// free Cmds memory before returning from this fn.

	class _FreeCmds_
	{
	public:
		~_FreeCmds_()
		{
			Cmd_Startup::Free();
		}
	} _FreeCmds_;

	// usual initialization of an nt driver.

	NTSTATUS NtStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT pDeviceObject = NULL;
	UNICODE_STRING usDriverName, usDosDeviceName;

	::RtlInitUnicodeString(&usDriverName, L"\\Device\\BugChecker");
	::RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\BugChecker");

	NtStatus = ::IoCreateDevice(pDriverObject, 0, &usDriverName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);

	if (NtStatus != STATUS_SUCCESS)
	{
		::ReportInitError(BcInitError_IoCreateDeviceFailed, "IoCreateDevice failed.");
	}
	else
	{
		pDeviceObject->Flags |= IO_TYPE_DEVICE;
		pDeviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

		NtStatus = ::IoCreateSymbolicLink(&usDosDeviceName, &usDriverName);

		if (NtStatus != STATUS_SUCCESS)
		{
			::IoDeleteDevice(pDeviceObject);

			::ReportInitError(BcInitError_IoCreateSymbolicLinkFailed, "IoCreateSymbolicLink failed.");
		}
		else
		{
			pDriverObject->DriverUnload = ::DrvUnload;

			pDriverObject->MajorFunction[IRP_MJ_CREATE] = ::DrvDispatchCreate;
			pDriverObject->MajorFunction[IRP_MJ_CLOSE] = ::DrvDispatchClose;
			pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ::DrvDispatchIoctl;

			// BC initialization.

			Allocator::Init(); // must be the first thing to do during initialization!

			Platform::UserProbeAddress = (PVOID)MM_USER_PROBE_ADDRESS;

			// try to get pdb's guid and age of the kernel. Cannot continue without this info.

			if (!Platform::TryGetKernelDebugInfoWithoutSymbols(pDriverObject, &Platform::KernelDebugInfo))
			{
				::ReportInitError(BcInitError_TryGetKernelDebugInfoWithoutSymbolsFailed, "TryGetKernelDebugInfoWithoutSymbols failed.");
			}
			else
			{
				// copy the kernel pdb info in the Symbol Loader struct.

				ARRAY_MEMCPY_SIZE_CHECK(BcInitResult.kernPdbGuid, Platform::KernelDebugInfo.guid);
				ARRAY_MEMCPY_SIZE_CHECK(BcInitResult.kernPdbName, Platform::KernelDebugInfo.szPdb);

				BcInitResult.kernPdbAge = Platform::KernelDebugInfo.age;

				// root obj initialization: read the config file and load all the symbols.

				Root::I = new Root();

				// create all the Cmd objects.

				Cmd_Startup::CreateInstances();

				// map the video framebuffer.

				if (!Root::I->fbStride && Root::I->fbWidth)
					Root::I->fbStride = Root::I->fbWidth * 4;

				if (Root::I->fbWidth &&
					Root::I->fbHeight &&
					Root::I->fbAddress &&
					Root::I->fbStride)
				{
					Root::I->fbMapSize = Root::I->fbWidth * Root::I->fbHeight * 4;

					PHYSICAL_ADDRESS pAddr;
					pAddr.QuadPart = Root::I->fbAddress;

					Root::I->VideoAddr = (unsigned int*)MmMapIoSpace(pAddr,
						Root::I->fbMapSize,
						MmNonCached);

					if (!Root::I->VideoAddr)
					{
						::ReportInitError(BcInitError_MmMapIoSpaceFailed, "Unable to map framebuffer memory: MmMapIoSpace failed.");
					}
				}
				else
				{
					::ReportInitError(BcInitError_FramebufferInfoNotProvided, "Unable to map framebuffer memory: info not provided in config file.");
				}

				if (Root::I->VideoAddr)
				{
					// map the vm command buffer.

					if (Root::I->VmFifoPhys && Root::I->VmFifoSize)
					{
						PHYSICAL_ADDRESS pAddr;
						pAddr.QuadPart = Root::I->VmFifoPhys;

						Root::I->VmFifo = (DWORD*)::MmMapIoSpace(pAddr, Root::I->VmFifoSize, MmNonCached);

						if (!Root::I->VmFifo) ::DbgPrint("BugChecker: Root::I->VmFifo: MmMapIoSpace failed.");
					}

					// get kernel info using the kernel pdb file.

					auto symF = Root::I->GetSymbolFileByGuidAndAge(Platform::KernelDebugInfo.guid, Platform::KernelDebugInfo.age);
					eastl::string result = symF == NULL ? "KERNEL_SYMBOLS_NOT_LOADED" : Platform::CalculateKernelOffsets(symF->GetHeader());

					if (result.size())
					{
						::ReportInitError(BcInitError_CalculateKernelOffsetsFailed,
							("CalculateKernelOffsets failed: " + result +
								"; PDB Guid: " + Utils::GuidToString(Platform::KernelDebugInfo.guid) +
								", PDB Age: " + Utils::Dword32ToString(Platform::KernelDebugInfo.age) +
								", PDB Name: " + eastl::string(Platform::KernelDebugInfo.szPdb) +
								".").c_str()
						);

						// ::PrintInitErrorOnScreen(result.c_str());
					}

					if (!result.size())
					{
						// create a snapshot of all the user modules in the system and install the notify routines.

						Platform::GetModulesSnapshot();

						Root::I->ProcessNotifyCreated = ::PsSetCreateProcessNotifyRoutine(&Platform::CreateProcessNotifyRoutine, FALSE) == STATUS_SUCCESS;
						Root::I->ImageNotifyCreated = ::PsSetLoadImageNotifyRoutine(&Platform::LoadImageNotifyRoutine) == STATUS_SUCCESS;

						if (!Root::I->ProcessNotifyCreated) ::DbgPrint("BugChecker: Root::I->ProcessNotifyCreated == FALSE.");
						if (!Root::I->ImageNotifyCreated) ::DbgPrint("BugChecker: Root::I->ImageNotifyCreated == FALSE.");

						// hook KDCOM.

						if (Root::I->kdcomHook == KdcomHook::Callback)
							::CallSetCallbacks(TRUE);
						else if (Root::I->kdcomHook == KdcomHook::Patch)
							::PatchKdCom();
						else
							::ReportInitError(BcInitError_KdcomHookInfoNotProvided, "Unable to hook KDCOM: info not provided in config file.");
					}
				}
			}
		}
	}

	return NtStatus;
}
