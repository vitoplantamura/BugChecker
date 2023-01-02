#pragma once

#include "../../bzscore/WinKernel/ntstatusdbg.h"

//The following trace constants are used by BazisLib::DDK itself:

#define TRACE_DEVICE_REGISTRATION	0

#define TRACE_PNP_EVENTS			0

#define TRACE_PNP_DEVICE_DELETION	TRACE_PNP_EVENTS
#define TRACE_PNP_DEVICE_LIFECYCLE	TRACE_PNP_EVENTS
#define TRACE_PNP_DEVICE_STOPPING	TRACE_PNP_EVENTS

#ifdef _DEBUG

#define DEBUG_TRACE(flag, data) ((void)(!(flag) || ((DbgPrint data), 1)))

#else

#define DEBUG_TRACE(flag, data)

#endif
