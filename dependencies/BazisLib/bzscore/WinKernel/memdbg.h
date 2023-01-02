#pragma once

namespace BazisLib
{
	namespace MemoryDebug
	{
		enum {MEMORY_DEBUG_SHIFT = 0};

		void *OnMemoryAllocation(void *pBlock, size_t size);
		void *OnMemoryFree(void *pBlock);

		void OnProgramTermination();
	}
}
