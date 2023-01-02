#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "memdbg.h"

namespace BazisLib
{
	namespace MemoryDebug
	{
		long AllocCount = 0;

		void *OnMemoryAllocation(void *pBlock, size_t size)
		{
#ifdef _DEBUG
			if (pBlock)
				memset(pBlock, 0xCC, size);
#endif
			InterlockedIncrement(&AllocCount);
			return pBlock;
		}

		void *OnMemoryFree(void *pBlock)
		{
			InterlockedDecrement(&AllocCount);
			return pBlock;
		}

		void OnProgramTermination()
		{
			if (AllocCount)
			{
				DbgPrint("*****************************************************\n");
				DbgPrint("*                      WARNING                      *\n");
				DbgPrint("*  MEMORY LEAKS HAVE BEEN DETECTED IN THE DRIVER    *\n");
				DbgPrint("*  %5d total memory allocations are not freed     *\n", AllocCount);
				DbgPrint("*****************************************************\n");
			}
		}
	}
}
#endif