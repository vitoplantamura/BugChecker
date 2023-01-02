#pragma once
#include "../atomic.h"

//Windows CE does not support _CrtSetAllocHook()
#ifndef UNDER_CE

#include <crtdbg.h>

//Do not include this file from different source files in your project!
//Otherwise, you'll get problems with multiple pDetector symbols.

namespace BazisLib
{
	//! Provides a simple way of detecting memory leaks
	/*! To add memory leak detection to your application, perform the following steps:
		<ul>
			<li>Reference memdbg.h from the source file that contains main() function</li>
			<li>Create a MemoryLeakDetector in the beginning of your main() function</li>
			<li>Ensure that your main() function exits normally (no calls as TerminateProcess() are performed)</li>
		</ul>
		In that case at the end of the main() function you'll get either a message in the debug output window
		stating that no memory leaks are detected, or an assertion fail on the allocation count checking code.
	*/
	class MemoryLeakDetector
	{
#ifdef _DEBUG
	private:
		static MemoryLeakDetector *pDetector;
		static _CRT_ALLOC_HOOK pPrevHook;
	private:
		AtomicInt32 m_AllocationCount;

	private:
		static int DebugAllocHook(int allocType, void *userData, size_t size, int blockType, long requestNumber, const unsigned char *filename, int  lineNumber)
		{
			ASSERT(pDetector);
			if (allocType == _HOOK_ALLOC)
				pDetector->m_AllocationCount++;
			else if (allocType == _HOOK_FREE)
				pDetector->m_AllocationCount--;
			if (!pPrevHook)
				return 1;
			return pPrevHook(allocType, userData, size, blockType, requestNumber, filename, lineNumber);
		}
	public:
		MemoryLeakDetector() :
		  m_AllocationCount(0)
		{
			printf("Initializing BazisLib leak detector...\n");	//printf() allocates a memory block internally on first use
			ASSERT(!pDetector);
			pDetector = this;
			pPrevHook = _CrtSetAllocHook(DebugAllocHook);
		}

		~MemoryLeakDetector()
		{
			if (m_AllocationCount)
			{
				OutputDebugString(_T("!!!!!!!!!!!!WARNING!!!!!!!!!!!!!!\nMemory leaks detected!\nBreaking!\n"));
				//! If you got a break here, examine the m_AllocationCount parameter
				ASSERT(!m_AllocationCount);
			}
			else
				OutputDebugString(_T("MemoryLeakDetector: No memory leaks detected\n"));
		}
#endif
	};

#ifdef _DEBUG
	//! Contains the pointer to the only MemoryLeakDetector instance
	/*! 
		\attention If you see the 'symbol MemoryLeakDetector::pDetector already defined' error, you have
		mistakingly included the memdbg.h file from multiple files in your project, that is an incorrect
		use. Please include it only from the file containing main() function.
	*/
	MemoryLeakDetector *MemoryLeakDetector::pDetector = NULL;
	_CRT_ALLOC_HOOK  MemoryLeakDetector::pPrevHook = NULL;
#endif
}

#endif