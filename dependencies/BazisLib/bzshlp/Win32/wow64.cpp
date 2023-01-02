#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "wow64.h"
#include "../../bzscore/assert.h"

#ifndef UNDER_CE

using namespace BazisLib;

WOW64APIProvider::WOW64APIProvider()
{
	HMODULE hKernel32 = GetModuleHandle(_T("kernel32"));
	ASSERT(hKernel32);
    m_fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(hKernel32, "IsWow64Process");
	m_fnDisableRedirection = (LPFN_WOW64DISABLEREDIR)GetProcAddress(hKernel32, "Wow64DisableWow64FsRedirection");
	m_fnResumeRedirection = (LPFN_WOW64REVERTREDIR)GetProcAddress(hKernel32, "Wow64RevertWow64FsRedirection");
}

bool WOW64APIProvider::IsWow64Process(unsigned PID)
{
	if (!m_fnIsWow64Process)
		return false;

	BOOL bIsWow64 = FALSE;

	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, PID ? PID : GetCurrentProcessId());
	if (hProc == INVALID_HANDLE_VALUE)
		return false;
    if (!m_fnIsWow64Process(hProc, &bIsWow64))
    {
		CloseHandle(hProc);
        return false;
    }
	CloseHandle(hProc);
	return (bIsWow64 != 0);
}

bool WOW64APIProvider::DisableFSRedirection(PVOID *pContext)
{
	if (!m_fnDisableRedirection)
		return false;
	return m_fnDisableRedirection(pContext) != 0;
}

bool WOW64APIProvider::ResumeFSRedirection(PVOID pContext)
{
	if (!m_fnDisableRedirection)
		return false;
	return m_fnResumeRedirection(pContext) != 0;
}

#endif
#endif