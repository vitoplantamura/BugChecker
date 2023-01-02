#pragma once

#define BZSLIB_SOCKETS_AVAILABLE

#ifdef BZS_POSIX
#include "Posix/socket.h"
#elif defined (_WIN32) && !defined(_NTDDK_) && !defined(_KERNEL_MODE)
#include "Win32/socket.h"
#elif defined (BZSLIB_KEXT)
#include "KEXT/socket.h"
#else
#undef BZSLIB_SOCKETS_AVAILABLE
#ifndef BZSLIB_DISABLE_NO_SOCKET_ERROR
#error Socket API is not defined for current platform.
#endif
#endif

