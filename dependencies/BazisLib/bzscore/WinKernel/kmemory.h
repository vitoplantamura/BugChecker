#pragma once

void * __cdecl npagednew(size_t size);

#ifndef _KERNEL_MODE
extern "C" {
 void   __cdecl free(void * _Memory);
 void * __cdecl malloc(size_t _Size);
 void * __cdecl realloc(void * _Memory, size_t _Size);
}
#endif

#define EXTENDED_REALLOC
extern "C" void * __cdecl _realloc(void * _Memory, size_t _Size, size_t _OldSize);

static inline void *bulk_malloc(size_t size)
{
	return ExAllocatePool(PagedPool, size);
}

static inline void bulk_free(void *p, size_t)
{
	if (p)
		ExFreePool(p);
}

namespace BazisLib
{
	namespace DDK
	{
		class NonPagedObject
		{
		public:
			void *operator new(size_t size)
			{
				return npagednew(size);
			}
		};
	}
}

