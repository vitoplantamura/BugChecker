#pragma once

#ifndef __TCHAR_DEFINED
#ifdef UNICODE
#define __T(x)      L ## x
	typedef wchar_t TCHAR;
#else
#define __T(x)      x
	typedef char TCHAR;
#endif

#define _T(x) __T(x)
#endif