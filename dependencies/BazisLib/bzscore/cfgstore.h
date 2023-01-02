#pragma once
#include "cmndef.h"

#ifdef BZS_POSIX
#include "Posix/cfgstore.h"
#elif defined (BZSLIB_KEXT)
#include "KEXT/cfgstore.h"
#elif defined (BZSLIB_WINKERNEL)
#include "WinKernel/registry.h"
#include "Win32/cfgstore.h"
#elif defined (_WIN32)
#include "Win32/registry.h"
#include "Win32/cfgstore.h"
#else
#error Status code mapping is not defined for current platform.
#endif

namespace BazisLib
{
	using BazisLib::ConfigurationPath;
	using BazisLib::ConfigurationKey;
}