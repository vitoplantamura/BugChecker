#include "BugChecker.h"

#if 1 // =FIXFIX= try to get rid of this hack: coroutines still require these externals, even after disabling C++ exceptions.

typedef VOID* PDISPATCHER_CONTEXT;

extern "C" EXCEPTION_DISPOSITION __cdecl
__CxxFrameHandler3(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN PVOID EstablisherFrame,
	IN OUT PCONTEXT ContextRecord,
	IN OUT PDISPATCHER_CONTEXT DispatcherContext
)
{
	return ExceptionContinueSearch;
}

extern "C" EXCEPTION_DISPOSITION
__CxxFrameHandler4(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN PVOID EstablisherFrame,
	IN OUT PCONTEXT ContextRecord,
	IN OUT PDISPATCHER_CONTEXT DispatcherContext
)
{
	return ExceptionContinueSearch;
}

extern "C" EXCEPTION_DISPOSITION
__GSHandlerCheck_EH4(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN PVOID EstablisherFrame,
	IN OUT PCONTEXT ContextRecord,
	IN OUT PDISPATCHER_CONTEXT DispatcherContext
)
{
	return ExceptionContinueSearch;
}

#endif
