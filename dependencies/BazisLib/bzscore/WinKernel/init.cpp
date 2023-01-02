#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "init.h"
#include "memdbg.h"

#if defined(_IA64_) || defined(_AMD64_)
#pragma section(".CRT$XCA",long,read)
__declspec(allocate(".CRT$XCA")) void (*__ctors_begin__[1])(void)={0};
#pragma section(".CRT$XCZ",long,read)
__declspec(allocate(".CRT$XCZ")) void (*__ctors_end__[1])(void)={0};
#pragma data_seg()
#else
#pragma data_seg(".CRT$XCA")
void (*__ctors_begin__[1])(void)={0};
#pragma data_seg(".CRT$XCZ")
void (*__ctors_end__[1])(void)={0};
#pragma data_seg()
#endif

#pragma data_seg(".STL$A")
void (*___StlStartInitCalls__[1])(void)={0};
#pragma data_seg(".STL$L")
void (*___StlEndInitCalls__[1])(void)={0};
#pragma data_seg(".STL$M")
void (*___StlStartTerminateCalls__[1])(void)={0};
#pragma data_seg(".STL$Z")
void (*___StlEndTerminateCalls__[1])(void)={0};
#pragma data_seg()

NTSTATUS s_LastError = STATUS_SUCCESS;

using namespace BazisLib;

class AtexitEntry
{
public:
	typedef void (_cdecl *PATEXITCB)();

private:
	PATEXITCB m_pCallback;
	AtexitEntry *m_pNext;

	static AtexitEntry *s_pFront;

	~AtexitEntry()
	{
		if (m_pCallback)
			m_pCallback();
	}

public:
	AtexitEntry(PATEXITCB pCallback)
		: m_pNext(s_pFront)
		, m_pCallback(pCallback)
	{
		s_pFront = this;
	}

	static void InvokeAll()
	{
		if (!s_pFront)
			return;
		for (AtexitEntry *pEntry = s_pFront, *pNext = s_pFront->m_pNext; pEntry; pEntry = pNext)
		{
			pNext = pEntry->m_pNext;
			delete pEntry;
		}
		s_pFront = NULL;
	}
};

static NTSTATUS InitializeCppRunTime()
{
	//Run STL constructors
	for (void (**p)(void) = ___StlStartInitCalls__ + 1; p < ___StlEndInitCalls__; p++)
		(*p)();

	//Run other constructors
	for (void (**p)(void) = __ctors_begin__ + 1; p < __ctors_end__; p++)
		(*p)();
	return s_LastError;
}

AtexitEntry *AtexitEntry::s_pFront = NULL;

static void TerminateCppRunTime()
{
	AtexitEntry::InvokeAll();
	for (void (**p)(void) = ___StlStartTerminateCalls__ + 1; p < ___StlEndTerminateCalls__; p++)
		(*p)();
#ifdef _DEBUG
	MemoryDebug::OnProgramTermination();
#endif
}

int __cdecl atexit(void ( __cdecl *destructor )( void ))
{
	if (!destructor)
		return 0;

	AtexitEntry *pEntry = new AtexitEntry(destructor);
	//pEntry is registered in the constructor

	return 1;
}

static PDRIVER_UNLOAD s_OriginalDriverUnload;

static void __stdcall DriverUnloadHook(PDRIVER_OBJECT pDriver)
{
	if (s_OriginalDriverUnload)
		s_OriginalDriverUnload(pDriver);
	TerminateCppRunTime();
}

NTSTATUS _stdcall BazisCoreDriverEntry(
	IN OUT PDRIVER_OBJECT   DriverObject,
	IN PUNICODE_STRING      RegistryPath
	)
{
	NTSTATUS st = InitializeCppRunTime();
	if (!NT_SUCCESS(st))
		return st;

	st = CPPDriverEntry(DriverObject, RegistryPath);
	if (!NT_SUCCESS(st))
		TerminateCppRunTime();
	else if (DriverObject->DriverUnload)
	{
		s_OriginalDriverUnload = DriverObject->DriverUnload;
		DriverObject->DriverUnload = DriverUnloadHook;
	}

	return st;
}

#endif