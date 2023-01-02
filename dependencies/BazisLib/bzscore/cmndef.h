#pragma once

#if _WIN32 || _WIN64
#if _WIN64
#define BZSLIB64
#endif
#elif __GNUC__
#if __x86_64__ || __ppc64__
#define BZSLIB64
#endif
#else
#error Cannot detect whether current target is 32-bit or 64-bit
#endif

#if defined(_DDK_) || (defined(_WIN32) && defined(_KERNEL_MODE))
#ifndef UNICODE
#define UNICODE
#endif

#define BZSLIB_WINKERNEL

extern "C" 
{
#ifdef _ENABLE_VISUALDDK_HELPERS_
#include "WinKernel/VisualDDKHelpers.h"
#endif

#include <ntddk.h>
#include <ntddstor.h>
#include <ioevent.h>
}

#if defined(DBG) && DBG && !defined(_DEBUG)
#define _DEBUG 1
#endif

#elif defined(WIN32)
#define BZSLIB_WINUSER
#include <winsock2.h>
#include <windows.h>
#include <tchar.h>
#elif defined(BZS_POSIX)
#define BZSLIB_POSIX
#define BZSLIB_UNIXWORLD
#ifdef DEBUG
#define _DEBUG 1
#endif
typedef unsigned short wchar16_t;
#include <stdlib.h>
#include "Posix/compat.h"
#include "Posix/guid_compat.h"
#include "Posix/longlong_compat.h"
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
#define TRUE 1
#define FALSE 0
#elif defined(BZS_KEXT)
#define BZSLIB_KEXT
#define BZSLIB_UNIXWORLD
#ifdef DEBUG
#define _DEBUG 1
#endif
#include "KEXT/cmndef.h"
#else
#error Unknown platform type
#endif

#ifndef _MSC_VER
#define override
#endif

/*extern "C++"
{
	template <typename _CountofType, size_t _SizeOfArray>
	char (*__countof_helper(_CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];
#define __countof(_Array) sizeof(*__countof_helper(_Array))
}*/

#define __countof(x) ((sizeof(x) / sizeof((x)[0])))

#include "platform.h"
#include "tchar_compat.h"

#define BAZISLIB_VERSION	0x0300