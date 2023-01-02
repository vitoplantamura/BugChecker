#include "BcCoroutine.h"

#include "Root.h"
#include "Platform.h"

ProcessAwaiterRetVal BcAwaiterBase::ProcessAwaiter(DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer, PULONG PayloadBytes)
{
	Root::I->DebuggerState = DEBST_COROUTINE;

	// check if the coroutine has finished.

	if (!Root::I->AwaiterPtr)
	{
		if (Root::I->AwaiterStack.size())
		{
			Root::I->AwaiterPtr = Root::I->AwaiterStack.back();
			Root::I->AwaiterStack.pop_back();
			return ProcessAwaiterRetVal::RepeatLoop;
		}
		else
		{
			Root::I->DebuggerState = DEBST_CONTINUE;
			return ProcessAwaiterRetVal::RepeatLoop;
		}
	}

	// prepare the manipulate state request according to the awaiter's type.

	if (!::strcmp(Root::I->AwaiterPtr->function, "BcAwaiter_StateManipulate"))
	{
		BcAwaiter_StateManipulate* ptr = (BcAwaiter_StateManipulate*)Root::I->AwaiterPtr;
		ptr->prepareRequest(pState, SecondBuffer);
		if (PayloadBytes) *PayloadBytes = SecondBuffer->Length;
		return ProcessAwaiterRetVal::Ok;
	}
	else if (!::strcmp(Root::I->AwaiterPtr->function, "BcAwaiter_DiscoverPosInModules"))
	{
		BcAwaiter_DiscoverPosInModules* ptr = (BcAwaiter_DiscoverPosInModules*)Root::I->AwaiterPtr;
		ptr->PrepareRequest();
		return ProcessAwaiterRetVal::RepeatLoop;
	}
	else if (!::strcmp(Root::I->AwaiterPtr->function, "BcAwaiter_RecvTimeout"))
	{
		return ProcessAwaiterRetVal::Timeout;
	}
	else if (!::strcmp(Root::I->AwaiterPtr->function, "BcAwaiter_ReadMemory"))
	{
		BcAwaiter_ReadMemory* ptr = (BcAwaiter_ReadMemory*)Root::I->AwaiterPtr;
		ptr->PrepareRequest();
		return ProcessAwaiterRetVal::RepeatLoop;
	}
	else if (!::strcmp(Root::I->AwaiterPtr->function, "BcAwaiter_Join"))
	{
		BcAwaiter_Join* ptr = (BcAwaiter_Join*)Root::I->AwaiterPtr;

		Root::I->AwaiterStack.push_back(const_cast<BcAwaiterBase*>(Root::I->AwaiterPtr));
		Root::I->AwaiterPtr = NULL;

		ptr->coroutine.Start(&Root::I->AwaiterPtr);

		Root::I->DebuggerState = DEBST_PROCESS_AWAITER;
		return ProcessAwaiterRetVal::RepeatLoop;
	}
	else
		return ProcessAwaiterRetVal::RepeatLoop;
}

BcAwaiter_DiscoverPosInModules::BcAwaiter_DiscoverPosInModules(CHAR* outputBufferParam, ImageDebugInfo* debugInfoParam, ULONG_PTR pointerParam)
{
	function = "BcAwaiter_DiscoverPosInModules";
	retVal = TRUE;

	::strcpy(outputBufferParam, "");
	::memset(debugInfoParam, 0, sizeof(*debugInfoParam));

	outputBuffer = outputBufferParam;
	debugInfo = debugInfoParam;
	bytePointer = (BYTE*)pointerParam;
}

void BcAwaiter_DiscoverPosInModules::PrepareRequest()
{
	auto params = (DiscoverBytePointerPosInModules_Params*)Root::I->MemReadCor.InputParams;

	params->pszOutputBuffer = outputBuffer;
	params->pidiDebugInfo = debugInfo;
	params->pbBytePointer = bytePointer;

	params->pvPsLoadedModuleList = (VOID*)(ULONG_PTR)Root::I->PsLoadedModuleList;
	params->pvMmUserProbeAddress = Platform::UserProbeAddress;
	params->pvPCRAddress = Platform::GetPcrAddress();

	Root::I->MemReadCor.ResumePoint = 0;

	Root::I->MemReadCor.Pointer = NULL;
	Root::I->MemReadCor.BytesToRead = 0;
	Root::I->MemReadCor.ActualBytesRead = 0;
	Root::I->MemReadCor.BufferPos = 0;

	Root::I->MemReadCor.pCoroutine = Platform::DiscoverBytePointerPosInModules;
	Root::I->MemReadCor.EndDebuggerState = DEBST_COROUTINE;

	Root::I->DebuggerState = DEBST_MEMREADCOR;
	return;
}

void BcAwaiter_ReadMemory::PrepareRequest()
{
	Root::I->MemReadCor.ResumePoint = 2; // important, see the DEBST_MEMREADCOR handler.

	Root::I->MemReadCor.Pointer = NULL;
	Root::I->MemReadCor.BytesToRead = 0;
	Root::I->MemReadCor.ActualBytesRead = 0;
	Root::I->MemReadCor.BufferPos = 0;

	Root::I->MemReadCor.pCoroutine = NULL;
	Root::I->MemReadCor.EndDebuggerState = DEBST_COROUTINE;

	Root::I->DebuggerState = DEBST_MEMREADCOR;
	return;
}
