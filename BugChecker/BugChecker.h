#pragma once

#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 0

#include <ntddk.h>
#include <stdio.h>

typedef UCHAR BYTE;
typedef USHORT WORD;
typedef ULONG32 DWORD;
typedef ULONG64 QWORD;

#define ZYAN_NO_LIBC 1
#define ZYDIS_STATIC_DEFINE 1
#define ZYCORE_STATIC_DEFINE 1
#include <Zydis/Zydis.h> // disable Control Flow Guard when compiling BOTH zydis libs!

#ifdef _AMD64_
#define _6432_(a,b) a
#else
#define _6432_(a,b) b
#endif

#define ARRAY_MEMCPY_SIZE_CHECK(D,S) static_assert(sizeof(D)==sizeof(S)); ::memcpy(D, S, sizeof(D));

#define _MIN_(a,b) (((a) < (b)) ? (a) : (b))
#define _MAX_(a,b) (((a) > (b)) ? (a) : (b))

// >>> in order to support EASTL:

namespace std
{
	typedef ::size_t size_t;
}

#define EA_COMPILER_NO_STRUCTURED_BINDING

// <<<

#include "Allocator.h"

//
// Quick and simple scope guard class.
//

#include <EASTL/functional.h>

class scope_guard {
public:
	template<class Callable>
	scope_guard(Callable&& undo_func) : f(eastl::forward<Callable>(undo_func)) { }

	~scope_guard() {
		if (f) f();
	}

	scope_guard() = delete;
	scope_guard(const scope_guard&) = delete;
	scope_guard& operator = (const scope_guard&) = delete;
	scope_guard(scope_guard&&) = delete;
	scope_guard& operator = (scope_guard&&) = delete;

private:
	eastl::function<void()> f;
};
