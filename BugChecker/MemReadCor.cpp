#include "MemReadCor.h"

BYTE MemoryReaderCoroutineState::InputParams[512];

LONG MemoryReaderCoroutineState::ResumePoint;

VOID* MemoryReaderCoroutineState::Pointer;
ULONG MemoryReaderCoroutineState::BytesToRead;
ULONG MemoryReaderCoroutineState::ActualBytesRead;

BYTE MemoryReaderCoroutineState::Buffer[32 * 1024];

PFNMemoryReaderCoroutine MemoryReaderCoroutineState::pCoroutine;

DEBUGGER_STATE MemoryReaderCoroutineState::EndDebuggerState;

ULONG MemoryReaderCoroutineState::BufferPos;

MemoryReaderCoroutineState* MemoryReaderCoroutineState::ReadMemory_End()
{
	this->Pointer = NULL;
	this->BytesToRead = 0;
	this->ActualBytesRead = 0;

	this->ResumePoint = -1;

	return this;
}

MemoryReaderCoroutineState* MemoryReaderCoroutineState::ReadMemory(IN LONG lResumePoint, IN VOID* pvStart, IN ULONG ulLength)
{
	this->Pointer = pvStart;
	this->BytesToRead = ulLength;
	this->ActualBytesRead = 0;

	this->ResumePoint = lResumePoint;

	return this;
}

MemoryReaderCoroutineState* MemoryReaderCoroutineState::ReadMemory(IN LONG lResumePoint, IN WORD* pwWordPtr)
{
	return ReadMemory(lResumePoint, pwWordPtr, sizeof(WORD));
}

MemoryReaderCoroutineState* MemoryReaderCoroutineState::ReadMemory(IN LONG lResumePoint, IN DWORD* pdwDwordPtr)
{
	return ReadMemory(lResumePoint, pdwDwordPtr, sizeof(DWORD));
}

MemoryReaderCoroutineState* MemoryReaderCoroutineState::ReadMemory(IN LONG lResumePoint, IN unsigned long* pulULongPtr)
{
	return ReadMemory(lResumePoint, pulULongPtr, sizeof(unsigned long));
}

MemoryReaderCoroutineState* MemoryReaderCoroutineState::ReadMemory(IN LONG lResumePoint, IN QWORD* pqwQwordPtr)
{
	return ReadMemory(lResumePoint, pqwQwordPtr, sizeof(QWORD));
}

BOOLEAN MemoryReaderCoroutineState::ReadMemory_Succeeded()
{
	return this->ActualBytesRead == this->BytesToRead;
}
