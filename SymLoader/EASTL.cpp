#include "framework.h"

#include <EASTL/vector.h>

//
// >>> we use EASTL instead of the default MS STL, because the latter doesn't work
// on Windows XP. For example, std::vector<int> requires InitializeCriticalSectionEx
// which is not available on XP.
// 
// For the same reason, we don't use MFC for this application.
//
// The only modification to EASTL is this code:
// 
//   #ifndef IGNORE_BC_MODIFICATIONS_TO_EASTL
//   #undef EA_COMPILER_HAS_INTTYPES // =BC= required for compilation with BugChecker.sys.
//   #endif
// 
// after the "#define EA_COMPILER_HAS_INTTYPES" line in "eabase.h".
//
// We only use the templates from EASTL, without any implementation file:
//

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::operator new(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::operator new(size);
}

namespace eastl
{
    EASTL_API void AssertionFailure(const char* pExpression)
    {
    }

    EASTL_API allocator gDefaultAllocator;

    EASTL_API allocator* GetDefaultAllocator()
    {
        return &gDefaultAllocator;
    }
}

// <<<
