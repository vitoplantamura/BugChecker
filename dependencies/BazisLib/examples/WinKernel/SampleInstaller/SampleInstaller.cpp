// SampleInstaller.cpp : main source file for SampleInstaller.exe
//

#include "stdafx.h"

#include "resource.h"

#include "MainDlg.h"

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
// 	CMessageLoop theLoop;
// 	_Module.AddMessageLoop(&theLoop);

	CMainDlg dlgMain;
	return dlgMain.DoModal(HWND_DESKTOP);

// 	if(dlgMain.Create(NULL) == NULL)
// 	{
// 		ATLTRACE(_T("Main dialog creation failed!\n"));
// 		return 0;
// 	}
// 
// 	dlgMain.ShowWindow(nCmdShow);
// 
// 	int nRet = theLoop.Run();
// 
// 	_Module.RemoveMessageLoop();
// 	return nRet;
}

#include <bzshlp/Win32/wow64.h>

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	if (BazisLib::Win32::WOW64APIProvider::sIsWow64Process())
	{
		MessageBox(HWND_DESKTOP, L"On 64-bit operating systems please use the 64-bit version of the sample installer", NULL, MB_ICONWARNING);
		return 0;
	}

	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
