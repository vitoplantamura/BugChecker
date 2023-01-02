#pragma once

#include "BugChecker.h"

//

class MemoryReaderCoroutineState;
typedef MemoryReaderCoroutineState* (*PFNMemoryReaderCoroutine) (MemoryReaderCoroutineState* pMemReadCor);

//

typedef enum _DEBUGGER_STATE DEBUGGER_STATE;

class MemoryReaderCoroutineState
{
public:

	// === methods ===

	MemoryReaderCoroutineState* ReadMemory_End();

	BOOLEAN ReadMemory_Succeeded();

	MemoryReaderCoroutineState* ReadMemory(IN LONG lResumePoint, IN VOID* pvStart, IN ULONG ulLength);
	MemoryReaderCoroutineState* ReadMemory(IN LONG lResumePoint, IN WORD* pwWordPtr);
	MemoryReaderCoroutineState* ReadMemory(IN LONG lResumePoint, IN DWORD* pdwDwordPtr);
	MemoryReaderCoroutineState* ReadMemory(IN LONG lResumePoint, IN unsigned long* pulULongPtr);
	MemoryReaderCoroutineState* ReadMemory(IN LONG lResumePoint, IN QWORD* pqwQwordPtr);

	// === caller only accessed fields ===

	static PFNMemoryReaderCoroutine pCoroutine;

	static DEBUGGER_STATE EndDebuggerState;

	static ULONG BufferPos;

	// === caller and callee accessed fields ===

	static BYTE InputParams[512];

	static LONG ResumePoint;
	
	static VOID* Pointer;
	static ULONG BytesToRead;
	static ULONG ActualBytesRead;

	static BYTE Buffer[32 * 1024];
};
