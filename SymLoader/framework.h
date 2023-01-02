#ifdef _WIN64
#error BugChecker Symbol Loader is meant to be distributed only as a 32 bit binary. However there should be no limitation in the code preventing a successful 64 bit build.
#endif

// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#define IGNORE_BC_MODIFICATIONS_TO_EASTL // see EASTL.cpp for more info...

#include "targetver.h"
//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers (commented in order to use GetOpenFileNameA)
// Windows Header Files
#include <winsock2.h>
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>

#define _MIN_(a, b)  (((a) < (b)) ? (a) : (b))

#define WM_DOWNLOAD_THREAD_COMPLETED (WM_APP+0)
#define WM_SHOW_MESSAGE_BOX (WM_APP+1)
#define WM_PHYSMEMSEARCH_THREAD_END (WM_APP+2)

//
// Quick and simple scope guard class.
//

#include <EASTL/functional.h>

class scope_guard {
public:
	template<class Callable>
	scope_guard(Callable&& undo_func) : f(std::forward<Callable>(undo_func)) { }

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
