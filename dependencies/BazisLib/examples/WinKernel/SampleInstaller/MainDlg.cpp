// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"
#include "ProjectDescription.h"

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	return FALSE;
}

using namespace BazisLib;

#include <bzscore/path.h>
#include <bzscore/file.h>
#include <bzshlp/textfile.h>

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	m_ComboBox.m_hWnd = GetDlgItem(IDC_COMBO1);

	TCHAR tsz[MAX_PATH] = {0,};
	GetModuleFileName(GetModuleHandle(NULL), tsz, __countof(tsz));
	String samplesDir = Path::GetDirectoryName(TempStrPointerWrapper(tsz));

	for (BazisLib::Directory::enumerator itPlatform = Directory(samplesDir).FindFirstFile(L"*"); itPlatform.Valid(); itPlatform.Next())
	{
		if (!itPlatform.IsDirectory())
			continue;
		if (itPlatform.GetRelativePath().c_str()[0] == '.')
			continue;

		for (BazisLib::Directory::enumerator itSample = Directory(itPlatform.GetFullPath()).FindFirstFile(L"*"); itSample.Valid(); itSample.Next())
		{
			if (!itSample.IsDirectory())
				continue;
			if (itSample.GetRelativePath().c_str()[0] == '.')
				continue;

			for (BazisLib::Directory::enumerator it = Directory(itSample.GetFullPath()).FindFirstFile(L"*.inf"); it.Valid(); it.Next())
			{
				ManagedPointer<AIFile> pFile = new ACFile(it.GetFullPath(), FileModes::OpenReadOnly);
				ManagedPointer<BazisLib::TextANSIUNIXFileReader> pReader = new BazisLib::TextANSIUNIXFileReader(pFile);
				if (!pReader->Valid())
					continue;

				String description;

				while (!pReader->IsEOF())
				{
					DynamicStringA str = pReader->ReadLine();
					if (str.length() && str[0] != ';')
						break;
					description += ANSIStringToString(str.substr(1));
					description += L"\r\n";
				}

				ProjectDescription desc;
				desc.SampleName = Path::GetFileNameWithoutExtension(it.GetFullPath());
				desc.SampleDescription = description;
				desc.INFFile = it.GetFullPath();

				m_Projects.push_back(desc);
				m_ComboBox.InsertString(-1, String::sFormat(L"%s - %s", desc.SampleName.c_str(), itPlatform.GetRelativePath().c_str()).c_str());
			}
		}
	}

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);

	::EnableWindow(GetDlgItem(IDC_BUTTON1), FALSE);
	::EnableWindow(GetDlgItem(IDC_BUTTON2), FALSE);

	// register object for message filtering and idle updates
// 	CMessageLoop* pLoop = _Module.GetMessageLoop();
// 	ATLASSERT(pLoop != NULL);
// 	pLoop->AddMessageFilter(this);
// 	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// unregister message filtering and idle updates
// 	CMessageLoop* pLoop = _Module.GetMessageLoop();
// 	ATLASSERT(pLoop != NULL);
// 	pLoop->RemoveMessageFilter(this);
// 	pLoop->RemoveIdleHandler(this);

	return 0;
}


void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}

LRESULT CMainDlg::OnClose( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
	EndDialog(0);
	return 0;
}


LRESULT CMainDlg::OnBnClickedCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(0);

	return 0;
}

#include <bzshlp/Win32/bzsdev.h>
using namespace BazisLib::Win32;

static bool IsDriverInstalled(LPCTSTR lpHwID)
{
	DeviceInformationSet devs = DeviceInformationSet::GetLocalDevices();
	for (DeviceInformationSet::iterator it = devs.begin(); it != devs.end(); it++)
	{
		if (!it.GetDeviceRegistryProperty(SPDRP_HARDWAREID).compare(lpHwID))
			return true;
	}
	return false;
}

static void RemoveRootEnumeratedDriver(LPCTSTR lpHwID)
{
	DeviceInformationSet devs = DeviceInformationSet::GetLocalDevices();
	for (DeviceInformationSet::iterator it = devs.begin(); it != devs.end(); it++)
	{
		if (!it.GetDeviceRegistryProperty(SPDRP_HARDWAREID).compare(lpHwID))
		{
			it.CallClassInstaller(DIF_REMOVE);
		}
	}
}

#include <newdev.h>
#pragma comment(lib, "newdev")

static ActionStatus InstallRootEnumeratedDriver(const TCHAR *pszHardwareID, const TCHAR *pszINFFile)
{
	if (!IsDriverInstalled(pszHardwareID))
	{
		ActionStatus st = AddRootEnumeratedNode(INFClass::FromINFFile(pszINFFile), pszHardwareID);
		if (!st.Successful())
			return st;
	}
	BOOL RebootRequired = false;
	if (!UpdateDriverForPlugAndPlayDevices(HWND_DESKTOP,
		pszHardwareID,
		pszINFFile,
		INSTALLFLAG_FORCE,
		&RebootRequired))
		return MAKE_STATUS(ActionStatus::FailFromLastError());
	return MAKE_STATUS(Success);;
}



LRESULT CMainDlg::OnCbnSelchangeCombo1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int idx = m_ComboBox.GetCurSel();
	if (idx >= 0 && idx < m_Projects.size())
	{
		SetDlgItemText(IDC_EDIT1, m_Projects[idx].SampleDescription.c_str());
		bool found = IsDriverInstalled((String(L"root\\") + m_Projects[idx].SampleName).c_str());
		SetDlgItemText(IDC_DRIVERSTATE, found ? L"Installed" : L"Not installed");

		::EnableWindow(GetDlgItem(IDC_BUTTON1), TRUE);
		::EnableWindow(GetDlgItem(IDC_BUTTON2), TRUE);
	}

	return 0;
}


LRESULT CMainDlg::OnBnClickedButton1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
	int idx = m_ComboBox.GetCurSel();
	if (idx >= 0 && idx < m_Projects.size())
	{
		ActionStatus st = InstallRootEnumeratedDriver((String(L"root\\") + m_Projects[idx].SampleName).c_str(), m_Projects[idx].INFFile.c_str());
		if (!st.Successful())
			MessageBox(String::sFormat(L"Cannot install driver: %s", st.GetErrorText().c_str()).c_str(), NULL, MB_ICONERROR);
		OnCbnSelchangeCombo1(0,0,0,bHandled);
	}

	return 0;
}


LRESULT CMainDlg::OnBnClickedButton2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
	int idx = m_ComboBox.GetCurSel();
	if (idx >= 0 && idx < m_Projects.size())
	{
		RemoveRootEnumeratedDriver((String(L"root\\") + m_Projects[idx].SampleName).c_str());
		OnCbnSelchangeCombo1(0,0,0,bHandled);
	}

	return 0;
}
