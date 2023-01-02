#pragma once

#include "BugChecker.h"

class Allocator
{
private:

	static constexpr int ArenaSize = 13 * 1024 * 1024;
	static constexpr int BlockSize = _6432_(64, 32);
	static constexpr int HeaderSize = _6432_(16, 8);

	static constexpr int BlocksNum = ArenaSize / BlockSize;
	static constexpr int BitmapSize = BlocksNum / 8;

	static VOID* Arena;
	static VOID* Bitmap;

public:

	static BOOLEAN ForceUse; // TRUE to force use of BC allocator; default: FALSE.
	static int Start0to99; // where to start when searching for free blocks; 0: start; 99: max value, near the end of the arena; default: 0.

	static VOID Init();
	static VOID Uninit();

	static VOID* Alloc(size_t size);
	static VOID Free(VOID* ptr);

	static BOOLEAN IsPtrInArena(VOID* ptr);
	static size_t UserSizeInBytes(VOID* ptr);

	static VOID GetPoolMap(CHAR* out, LONG num);

	static VOID FatalError(const CHAR* msg);

	static BOOLEAN MustUseExXxxPool();

	static BOOLEAN TestBitmap(); // for internal tests only.
	static size_t GetSetBitsNumInBitmap(); // for internal tests only.

	static size_t Perf_AllocationsNum; // for internal tests only.
	static size_t Perf_TotalAllocatedSize; // for internal tests only.
};

//

void* __cdecl operator new(size_t count);
void* __cdecl operator new(size_t, void* address);
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line);
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line);
void __cdecl operator delete(void* address);
void __cdecl operator delete[](void* address);
void __cdecl operator delete(void*, void*);
void __cdecl operator delete(void* address, size_t);
