#pragma once

#include "autolock.h"
#include "cmndef.h"

namespace BazisLib
{
	enum EventType
	{
		kManualResetEvent,
		kAutoResetEvent,
	};
}

#ifdef BZSLIB_POSIX
#include "Posix/sync.h"
#elif defined (BZSLIB_KEXT)
#include "KEXT/sync.h"
#elif defined (BZSLIB_WINKERNEL)
#include "WinKernel/sync.h"
#elif defined (_WIN32)
#include "Win32/sync.h"
#else
#error No synchronization classes defined for current platform
#endif

using BazisLib::Mutex;
using BazisLib::Event;
using BazisLib::Semaphore;
using BazisLib::RWLock;

namespace BazisLib
{
	typedef FastLocker<Mutex> MutexLocker;
	typedef FastLocker<Mutex> MutexUnlocker;

	typedef _ReadLocker<RWLock>				ReadLocker;
	typedef _WriteLocker<RWLock>			WriteLocker;
	typedef _WriteFromReadLocker<RWLock>	WriteFromReadLocker;
}
