#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "services.h"
#include <shellapi.h>

#ifndef UNDER_CE

const TCHAR * BazisLib::Win32::Service::GetStateName( DWORD dwState )
{
	switch (dwState)
	{
	case SERVICE_CONTINUE_PENDING:
		return _T("continuing");
	case SERVICE_PAUSE_PENDING:
		return _T("pausing");
	case SERVICE_PAUSED:
		return _T("paused");
	case SERVICE_RUNNING:
		return _T("running");
	case SERVICE_START_PENDING:
		return _T("starting");
	case SERVICE_STOP_PENDING:
		return _T("stopping");
	case SERVICE_STOPPED:
		return _T("stopped");
	default:
		return _T("unknown");
	}
}

const TCHAR * BazisLib::Win32::Service::GetServiceTypeName( DWORD DriverType )
{
	switch (DriverType)
	{
	case SERVICE_FILE_SYSTEM_DRIVER:
		return _T("FS driver");
	case SERVICE_KERNEL_DRIVER:
		return _T("driver");
	case SERVICE_WIN32_OWN_PROCESS:
	case SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS:
		return _T("win32");
	case SERVICE_WIN32_SHARE_PROCESS:
	case SERVICE_WIN32_SHARE_PROCESS | SERVICE_INTERACTIVE_PROCESS:
		return _T("shared");
	default:
		return _T("unknown");
	}
}

const TCHAR * BazisLib::Win32::Service::GetStartTypeName( DWORD DriverType )
{
	switch (DriverType)
	{
	case SERVICE_BOOT_START:
		return _T("boot");
	case SERVICE_SYSTEM_START:
		return _T("system");
	case SERVICE_DEMAND_START:
		return _T("manual");
	case SERVICE_DISABLED:
		return _T("disabled");
	case SERVICE_AUTO_START:
		return _T("auto");
	default:
		return _T("unknown");
	}
}

BazisLib::String BazisLib::Win32::Service::TranslateDriverPath( LPCTSTR lpReportedPath )
{
	if (!lpReportedPath)
		return String();
	if (!lpReportedPath[0])
		return _T("");
	if (lpReportedPath[0] && (lpReportedPath[1] == ':'))
		return lpReportedPath;
	if ((lpReportedPath[0] == '\"') && lpReportedPath[1] && (lpReportedPath[2] == ':'))
		return lpReportedPath;
	if (!_tcsncmp(lpReportedPath, _T("\\??\\"), 4))
		return lpReportedPath + 4;
	else if (!_tcsncmp(lpReportedPath, _T("\\DosDevices\\"), 12))
		return lpReportedPath + 12;
	else if (!_tcsncmp(lpReportedPath, _T("\\SystemRoot\\"), 12))
		lpReportedPath += 12;
	else if (!_tcsncmp(lpReportedPath, _T("\\Device\\"), 8))
	{
		const TCHAR *p = _tcschr(lpReportedPath + 8, '\\');
		if (!p)
			return _T("");
		unsigned devNameLen = (unsigned)(p - lpReportedPath);
		TCHAR tszDriveLtr[3] = {0, ':', 0};
		TCHAR tszPath[MAX_PATH];
		for (tszDriveLtr[0] = 'C'; tszDriveLtr[0] <= 'Z'; tszDriveLtr[0]++)
			if (QueryDosDevice(tszDriveLtr, tszPath, __countof(tszPath)))
				if (!_tcsnicmp(tszPath, lpReportedPath, devNameLen))
					return Path::Combine(tszDriveLtr, lpReportedPath + devNameLen + 1);
		return _T("");		
	}
	else if ((lpReportedPath[0] == '\\') || ((lpReportedPath[0] == '\"') && (lpReportedPath[1] == '\\')))
		return lpReportedPath;

	return Path::Combine(Path::GetSpecialDirectoryLocation(dirWindows), lpReportedPath);
}

BazisLib::String BazisLib::Win32::Service::Win32PathToDriverPath( LPCTSTR lpWin32Path, bool AddSystemRootPrefix, bool DoNotAddDosDevicesPrefix )
{
	if (!lpWin32Path)
		return _T("");
	if (!lpWin32Path[0])
		return _T("");
	TCHAR tszWinDir[MAX_PATH + 1];
	size_t len = GetWindowsDirectory(tszWinDir, __countof(tszWinDir)) + 1;
	_tcsncat(tszWinDir, _T("\\"), __countof(tszWinDir));
	if (!_tcsnicmp(lpWin32Path, tszWinDir, len))
	{
		if (AddSystemRootPrefix)
			return Path::Combine(_T("\\SystemRoot\\"), (lpWin32Path + len));
		else
			return (lpWin32Path + len);
	}
	if (DoNotAddDosDevicesPrefix)
		return lpWin32Path;
	if (lpWin32Path[0] == '\"')
		return String(_T("\"\\??\\")) + (lpWin32Path + 1);
/*	if (lpWin32Path[1] == ':')
	{
		TCHAR tszDosDev[3] = {lpWin32Path[0], ':', 0};
		if (QueryDosDevice(tszDosDev, tszWinDir, __countof(tszWinDir)))
		{
			return FilePath(tszWinDir).AppendAndReturn(lpWin32Path + 3);
		}
	}*/
	return String(_T("\\??\\")) + lpWin32Path;
}

//BazisLib::FilePath BazisLib::Win32::Service::GetServiceBinaryPath( LPCTSTR lpReportedPath, bool ChopCommandLineArgs /*= true*/, bool ProbeWin32Paths /*= true*/ )
/*{
	//For some services, path containing spaces can be specified without quotation marks.
	if (ProbeWin32Paths && (lpReportedPath[0] && (lpReportedPath[1] == ':')))
		if (File::Exists(lpReportedPath))
			return lpReportedPath;
	if (!ChopCommandLineArgs || !_tcsncmp(lpReportedPath, _T("\\??\\"), 4) || !_tcsncmp(lpReportedPath, _T("\\DosDevices\\"), 12))
		return TranslateServiceCmdLine(lpReportedPath);
	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(lpReportedPath, &argc);
	if (!argv && !argc)
	{
		if (argv)
			LocalFree(argv);
		return _T("");
	}
	FilePath fp = TranslateServiceCmdLine(argv[0]);
	LocalFree(argv);
	return fp;
}*/

using namespace BazisLib::Win32;

OwnProcessServiceBase *OwnProcessServiceBase::s_pService = NULL;

BazisLib::Win32::OwnProcessServiceBase::OwnProcessServiceBase( LPCTSTR lpServiceName, bool Interactive /*= true*/ )
	: m_ServiceName(lpServiceName)
	, m_hThisService(NULL)
	, m_bInteractive(Interactive)
{
	//Only one OwnProcessServicBase instance can exist at a single time
	ASSERT(!s_pService);
	s_pService = this;
}

BazisLib::Win32::OwnProcessServiceBase::~OwnProcessServiceBase()
{
	ASSERT(s_pService == this);
	s_pService = NULL;
}

int BazisLib::Win32::OwnProcessServiceBase::EntryPoint()
{
	SERVICE_TABLE_ENTRY entry = {(LPTSTR)m_ServiceName.c_str(), sServiceMain};
	if (!StartServiceCtrlDispatcher(&entry))
		return GetLastError();
	return 0;
}

VOID WINAPI BazisLib::Win32::OwnProcessServiceBase::sServiceMain( DWORD dwArgc, LPTSTR* lpszArgv )
{
	ASSERT(s_pService);
	if (s_pService)
	{
		s_pService->m_hThisService = RegisterServiceCtrlHandlerEx(s_pService->m_ServiceName.c_str(), sHandlerEx, s_pService);
		s_pService->OnStartService(dwArgc, lpszArgv);
	}
}

BazisLib::ActionStatus BazisLib::Win32::OwnProcessServiceBase::ReportState( DWORD dwState /*= SERVICE_RUNNING*/, DWORD dwControlsAccepted /*= SERVICE_ACCEPT_STOP*/, DWORD dwExitCode /*= 0*/, DWORD dwSpecificError /*= 0*/, DWORD dwCheckPoint /*= 0*/, DWORD dwWaitHint /*= 0*/ )
{
	SERVICE_STATUS status = {0,};
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	if (m_bInteractive)
		status.dwServiceType |= SERVICE_INTERACTIVE_PROCESS;
	status.dwCurrentState = dwState;
	status.dwControlsAccepted = dwControlsAccepted;
	status.dwWin32ExitCode = dwExitCode;
	status.dwServiceSpecificExitCode = dwSpecificError;
	status.dwCheckPoint = dwCheckPoint;
	status.dwWaitHint = dwWaitHint;
	if (SetServiceStatus(m_hThisService, &status))
		return MAKE_STATUS(Success);
	else
		return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
}

DWORD WINAPI BazisLib::Win32::OwnProcessServiceBase::sHandlerEx( DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext )
{
	if (dwControl == SERVICE_INTERROGATE)
		return NOERROR;
	ASSERT(lpContext == s_pService);
	ASSERT(s_pService);
	return ((OwnProcessServiceBase *)lpContext)->OnCommand(dwControl, dwEventType, lpEventData).GetErrorCode();
}

//-------------------------------------------------------------------------------------------------------------------------------------

BazisLib::ActionStatus BazisLib::Win32::OwnProcessServiceWithWorkerThread::OnStartService( int argc, LPTSTR *argv )
{
	ReportState(m_bReportRunningAutomatically ? SERVICE_RUNNING : SERVICE_START_PENDING);
	ActionStatus st = WorkerThreadBody(m_StopEvent.GetHandle());
	ReportState(SERVICE_STOPPED, 0, st.GetErrorCode());
	return st;
}

BazisLib::ActionStatus BazisLib::Win32::OwnProcessServiceWithWorkerThread::OnCommand( DWORD dwControl, DWORD dwEventType, LPVOID lpEventData )
{
	if (dwControl == SERVICE_CONTROL_STOP)
	{
		m_StopEvent.Set();
		ReportState(SERVICE_STOP_PENDING, 0);
	}
	else
		return MAKE_STATUS(NotSupported);
	return MAKE_STATUS(Success);
}

#endif
#endif