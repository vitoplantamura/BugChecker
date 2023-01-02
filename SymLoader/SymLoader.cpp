// SymLoader.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "SymLoader.h"
#include "StructuredFile.h"
#include "AddSymFileDlg.h"
#include "Utils.h"
#include "DriverControl.h"
#include "KdcomInstall.h"

#include "../BugChecker/Ioctl.h"

#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"") // Enable Visual Styles

HINSTANCE SymLoader::hInst = NULL;
HWND SymLoader::hMainWnd = NULL;

eastl::wstring SymLoader::BcDir;

eastl::unique_ptr<StructuredFile::Entry> SymLoader::config;

#define TEST_PIXEL_00 0x707172
#define TEST_PIXEL_10 0x737475
#define TEST_PIXEL_01 0x767778
#define TEST_PIXEL_11 0x797A7B

class ShowMessageBoxDetails
{
public:
	eastl::string text;
	eastl::string caption;
	UINT type;
};

class PhysMemSearchEndDetails
{
public:
	HWND testWindow = NULL;
	eastl::string width, height, address, stride;
};

LRESULT CALLBACK SymLoader::MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
	{
		WORD id = LOWORD(wParam);

		switch (id)
		{
		case IDC_ADD:
			AddSymFileDlg::Show(hInst, hWnd);
			break;

		case IDC_INFO:
			SymLoader::ShowBcsInfo(hWnd);
			break;

		case IDC_DELETE:
			SymLoader::RemoveBcsFromList(hWnd);
			break;

		case IDC_PHYSMEMSEARCH:
			SymLoader::PhysMemSearch(hWnd);
			break;

		case IDC_FB_ENABLEDD:
		case IDC_FB_DISABLEDD:
			SymLoader::EnableDisplays(hWnd, id == IDC_FB_ENABLEDD);
			break;

		case IDC_FB_SAVE:
			SymLoader::SaveFbDetails(hWnd);
			break;

		case IDC_HOOK_PATCH:
		case IDC_HOOK_CALLBACK:
			SymLoader::KdcomHookRadioClick(hWnd);
			break;

		case IDC_BREAKIN:
			SymLoader::BreakIn(hWnd);
			break;

		case IDC_START_DRV:
		{
			auto ret = DriverControl::Start();
			if (ret.size()) ::MessageBoxA(hWnd, ret.c_str(), "Error", MB_OK | MB_ICONERROR);
			else
			{
				if (SymLoader::VerifyInitResult(hWnd))
					::MessageBoxA(hWnd, "Operation completed successfully.", "Ok", MB_OK);
			}
		}
		break;

		case IDC_STOP_DRV:
		{
			auto ret = DriverControl::Stop();
			if (ret.size()) ::MessageBoxA(hWnd, ret.c_str(), "Error", MB_OK | MB_ICONERROR);
			else ::MessageBoxA(hWnd, "Operation completed successfully.", "Ok", MB_OK);
		}
		break;

		case IDC_HOOK_INSTALL:
			KdcomInstall::Install(hWnd);
			break;

		case IDOK:
		case IDCANCEL:
		{
			DestroyWindow(hWnd);
			return 0;
		}
		}
		break;
	}

	case WM_DESTROY:
	{
		// save and exit.
		StructuredFile::SaveConfig(config.get());
		PostQuitMessage(0);
		return 0;
	}

	case WM_SHOW_MESSAGE_BOX:
	{
		ShowMessageBoxDetails* d = reinterpret_cast<ShowMessageBoxDetails*>(wParam);
		::MessageBoxA(hWnd, d->text.c_str(), d->caption.c_str(), d->type);
		delete d;
	}
	break;

	case WM_PHYSMEMSEARCH_THREAD_END:
	{
		PhysMemSearchEndDetails* d = reinterpret_cast<PhysMemSearchEndDetails*>(wParam);

		::SendMessageW(d->testWindow, WM_CLOSE, 0, 0);

		if (d->address.size())
		{
			::SetWindowTextA(::GetDlgItem(hWnd, IDC_FB_WIDTH), d->width.c_str());
			::SetWindowTextA(::GetDlgItem(hWnd, IDC_FB_HEIGHT), d->height.c_str());
			::SetWindowTextA(::GetDlgItem(hWnd, IDC_FB_ADDRESS), d->address.c_str());
			::SetWindowTextA(::GetDlgItem(hWnd, IDC_FB_STRIDE), d->stride.c_str());

			SaveFbDetails(hWnd, TRUE);

			::MessageBoxA(hWnd, "Operation completed successfully.", "Ok", MB_OK);
		}
		else
		{
			::MessageBoxA(hWnd, "Unable to determine framebuffer details.", "Error", MB_OK | MB_ICONERROR);
		}

		Utils::EnableDlgItems(hWnd, { IDC_PHYSMEMSEARCH }, TRUE);

		delete d;
	}
	break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL SymLoader::RegisterMainWindowClass()
{
	WNDCLASSEX wc;

	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
	wc.hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_SYMLOADER), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE |
		LR_DEFAULTCOLOR | LR_SHARED);
	wc.hCursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEXT("BugChecker Symbol Loader");
	wc.hIconSm = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_SYMLOADER), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	return (RegisterClassEx(&wc)) ? TRUE : FALSE;
}

HWND SymLoader::CreateMainWindow(int nCmdShow)
{
	// create the main window.

	HWND hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAINWND), NULL, NULL);

	if (!hWnd)
		return NULL;

	ListView_SetExtendedListViewStyle(::GetDlgItem(hWnd, IDC_LIST), LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// populate the list view.

	if (!UpdateSymbolsList(hWnd, TRUE))
		return NULL;

	// populate the other controls.

	auto setts = config->get("settings");
	if (setts)
	{
		auto fb = setts->get("framebuffer");
		if (fb)
		{
			auto width = fb->GetSetting("width");
			auto height = fb->GetSetting("height");
			auto address = fb->GetSetting("address");
			auto stride = fb->GetSetting("stride");

			if (width) ::SetWindowTextA(::GetDlgItem(hWnd, IDC_FB_WIDTH), width->name.c_str());
			if (height) ::SetWindowTextA(::GetDlgItem(hWnd, IDC_FB_HEIGHT), height->name.c_str());
			if (address) ::SetWindowTextA(::GetDlgItem(hWnd, IDC_FB_ADDRESS), address->name.c_str());
			if (stride) ::SetWindowTextA(::GetDlgItem(hWnd, IDC_FB_STRIDE), stride->name.c_str());
		}

		auto kdc = setts->get("kdcom");
		if (kdc)
		{
			auto hook = kdc->GetSetting("hook");
			if (hook)
			{
				if (hook->name == "patch")
					::SendMessage(::GetDlgItem(hWnd, IDC_HOOK_PATCH), BM_SETCHECK, BST_CHECKED, 0);
				else if (hook->name == "callback")
					::SendMessage(::GetDlgItem(hWnd, IDC_HOOK_CALLBACK), BM_SETCHECK, BST_CHECKED, 0);
			}
		}
	}

	// show the window.

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

BOOL SymLoader::UpdateSymbolsList(HWND hWnd, BOOL initRun)
{
	auto list = ::GetDlgItem(hWnd, IDC_LIST);

	// add the columns.

	if (initRun)
	{
		auto addCol = [&](const CHAR* psz, const int cx) {

			LVCOLUMNA col = {};

			col.mask = LVCF_WIDTH | LVCF_TEXT;

			col.cx = cx;
			col.pszText = const_cast<LPSTR>(psz);

			::SendMessageA(list, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);
		};

		addCol("BCS Size", 100);
		addCol("Name", 200);
	}

	// load the config file.

	config.reset(StructuredFile::LoadConfig());

	// perform a quick sanity check on the file, if initialization.

	if (initRun)
	{
		BOOL write = FALSE;

		if (!config)
		{
			config.reset(new StructuredFile::Entry());
			write = TRUE;
		}

		if (!config->get("symbols"))
		{
			config->children.push_back(StructuredFile::Entry{ "symbols" });
			write = TRUE;
		}

		if (!config->get("settings"))
		{
			config->children.push_back(StructuredFile::Entry{ "settings" });
			write = TRUE;
		}

		if (write)
			if (!StructuredFile::SaveConfig(config.get()))
			{
				::MessageBoxA(NULL, "Error writing configuration file.", "Fatal Error", MB_OK | MB_ICONERROR);
				return FALSE;
			}
	}
	else
	{
		if (!config)
		{
			::MessageBoxA(NULL, "Error reading configuration file.", "Fatal Error", MB_OK | MB_ICONERROR);
			return FALSE;
		}
	}

	// delete all the items in the list view.

	::SendMessageA(list, LVM_DELETEALLITEMS, 0, 0L);

	// populate the list view.

	auto addItem = [&](int index, const CHAR* psz1, const CHAR* psz2) {

		LVITEMA item = {};

		item.iItem = index;
		item.iSubItem = 0;

		item.mask = LVIF_TEXT;
		item.pszText = (LPSTR)psz1;

		::SendMessageA(list, LVM_INSERTITEMA, 0, (LPARAM)&item);

		item.iItem = index;
		item.iSubItem = 1;

		item.mask = LVIF_TEXT;
		item.pszText = (LPSTR)psz2;

		::SendMessageA(list, LVM_SETITEMA, 0, (LPARAM)&item);
	};

	int index = 0;

	auto syms = config->get("symbols");
	if (syms)
	{
		for (auto& s : syms->children)
		{
			auto name = s.GetSetting("name");
			auto size = s.GetSetting("bcs_file_size");
			auto isk = s.GetSetting("is_kernel");

			eastl::string nameAppend = "";
			if (isk && isk->name == "1") nameAppend = " (KERNEL)";

			addItem(index++, ((!name ? s.name : name->name) + nameAppend).c_str(), !size ? "???" : (size->name + " bytes").c_str());
		}
	}

	// return success.

	return TRUE;
}

void SymLoader::ShowBcsInfo(HWND hWnd)
{
	// get the selected item index.

	auto list = ::GetDlgItem(hWnd, IDC_LIST);

	int sel = ::SendMessageA(list, LVM_GETNEXTITEM, -1, MAKELPARAM(LVIS_SELECTED, 0));

	if (sel < 0)
	{
		::MessageBoxA(hWnd, "Select a symbol file.", "Info", MB_OK);
		return;
	}

	// show the info.

	auto& e = config->get("symbols")->children[sel];

	eastl::string txt;
	const eastl::string indent = "\r\n      ";
	const eastl::string linee = "\r\n";

	txt += "File on disk:" + indent + Utils::WStringToAscii(SymLoader::BcDir) + "\\" + e.name + linee;

	auto name = e.GetSetting("name");
	txt += "Reference name (origin):" + indent + (name ? name->name : "???") + linee;

	auto guid = e.GetSetting("pdb_guid");
	txt += "PDB guid:" + indent + (guid ? guid->name : "???") + linee;

	auto age = e.GetSetting("pdb_age");
	txt += "PDB age:" + indent + (age ? age->name : "???") + linee;

	auto isk = e.GetSetting("is_kernel");
	txt += "Symbol file of ntoskrnl:" + indent + (!isk ? "???" : isk->name == "1" ? "Yes." : "No.") + linee;

	auto size = e.GetSetting("bcs_file_size");
	txt += "BCS file size:" + indent + (size ? size->name + " bytes" : "???") + linee;

	::MessageBoxA(hWnd, txt.c_str(), "Info", MB_OK);
}

void SymLoader::RemoveBcsFromList(HWND hWnd)
{
	// get the selected item index.

	auto list = ::GetDlgItem(hWnd, IDC_LIST);

	int sel = ::SendMessageA(list, LVM_GETNEXTITEM, -1, MAKELPARAM(LVIS_SELECTED, 0));

	if (sel < 0)
	{
		::MessageBoxA(hWnd, "Select a symbol file.", "Info", MB_OK);
		return;
	}

	auto& e = config->get("symbols")->children[sel];

	// cannot remove ntoskrnl.

	auto isk = e.GetSetting("is_kernel");

	if (isk && isk->name == "1")
	{
		::MessageBoxA(hWnd, "Cannot remove a symbol file referring to NTOSKRNL. BugChecker needs it in order to start.\r\n\r\nHowever you can load a different NTOSKRNL symbol file, clicking on the \"Add..\" button: the new file will replace the old one.", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	// confirmation.

	auto name = e.GetSetting("name");

	auto msg = "Are you sure you want to remove \"" + (name ? name->name : e.name) + "\"?";

	if (::MessageBoxA(hWnd, msg.c_str(), "Please Confirm", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	// remove and save.

	::SendMessage(list, LVM_DELETEITEM, sel, 0L);

	config->get("symbols")->children.erase(&e);

	if (!StructuredFile::SaveConfig(config.get()))
	{
		::MessageBoxA(NULL, "Error writing configuration file.", "Fatal Error", MB_OK | MB_ICONERROR);
		return;
	}
}

void SymLoader::PhysMemSearch(HWND hWnd)
{
	Utils::EnableDlgItems(hWnd, { IDC_PHYSMEMSEARCH }, FALSE);

	::UnregisterClassW(L"BCPIXELS", hInst);

	// create a top most window with 4 test pixels in the upper left corner of the screen.

	HWND h = NULL;

	auto wndProc = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
	{
		switch (msg)
		{
		case WM_ERASEBKGND:
		{
			auto hdc = (HDC)wParam;
			RECT rc;
			::GetClientRect(hWnd, &rc);
			::SetMapMode(hdc, MM_TEXT);
			::FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
			::SetPixel(hdc, 0, 0, TEST_PIXEL_00);
			::SetPixel(hdc, 1, 0, TEST_PIXEL_10);
			::SetPixel(hdc, 0, 1, TEST_PIXEL_01);
			::SetPixel(hdc, 1, 1, TEST_PIXEL_11);
			return 1L;
		}

		case WM_CLOSE:
			::DestroyWindow(hWnd);
			break;
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	};

	WNDCLASSEXW wc = {};

	wc.cbSize = sizeof(wc);
	wc.style = CS_NOCLOSE;
	wc.lpfnWndProc = wndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"BCPIXELS";
	wc.hIconSm = NULL;

	if (!::RegisterClassExW(&wc))
	{
		::MessageBoxA(hWnd, "RegisterClassExW failed.", "Error", MB_OK | MB_ICONERROR);
	}
	else
	{
		h = ::CreateWindowExW(
			WS_EX_TOPMOST | WS_EX_NOACTIVATE,
			L"BCPIXELS",
			L"",
			0,
			0, 0, 6, 6,
			NULL,
			NULL,
			hInst,
			NULL
		);

		if (!h)
		{
			::MessageBoxA(hWnd, "CreateWindowExW failed.", "Error", MB_OK | MB_ICONERROR);
		}
		else
		{
			::SetWindowLongW(h, GWL_STYLE, 0);
			::SetWindowPos(h, HWND_TOPMOST, 0, 0, 6, 6, SWP_SHOWWINDOW | SWP_NOACTIVATE);
		}
	}

	if (!h) return;

	// create a worker thread to scan the framebuffer, allowing the test window to appear and draw itself in the UI thread.

	auto Msg = [hWnd](const CHAR* text, const CHAR* caption, const UINT type) {

		auto d = new ShowMessageBoxDetails();

		d->text = text;
		d->caption = caption;
		d->type = type;

		::PostMessageW(hWnd, WM_SHOW_MESSAGE_BOX, reinterpret_cast<WPARAM>(d), 0);
	};

	auto CloseWnd = [h, hWnd](PhysMemSearchEndDetails* d) {

		d->testWindow = h;

		::PostMessageW(hWnd, WM_PHYSMEMSEARCH_THREAD_END, (WPARAM)d, 0);
	};

	Utils::CreateThread([Msg, CloseWnd]() {

		PhysMemSearchEndDetails* returnData = new PhysMemSearchEndDetails();

		// wait some time before starting, allowing the test window to appear.

		::Sleep(750);

		// get the memory range resources of all the display cards in the system.

		eastl::vector<eastl::pair<ULONGLONG, ULONGLONG>> resources;

		// compose the name of the process.

		auto exe = Utils::GetExeFolderWithBackSlash() + L"NativeUtil_";

		if (Utils::Is32bitOS())
			exe += L"32bit";
		else
			exe += L"64bit";

		exe += L".exe";

		// compose the name of the output file.

		auto out = Utils::GetExeFolderWithBackSlash() + L"resources.txt";

		::DeleteFileW(out.c_str());

		// create the process.

		eastl::wstring cmd = L"\"" + exe + L"\"" + L" -save-display-resources";

		STARTUPINFO info = { sizeof(info) };
		PROCESS_INFORMATION processInfo;
		if (!::CreateProcessW(exe.c_str(), const_cast<LPWSTR>(cmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, Utils::GetExeFolderWithBackSlash().c_str(), &info, &processInfo))
		{
			Msg(("Unable to start: \"" + Utils::WStringToAscii(exe) + "\"").c_str(), "Error", MB_OK | MB_ICONERROR);
		}
		else
		{
			// wait for the process to finish.

			::WaitForSingleObject(processInfo.hProcess, INFINITE);
			::CloseHandle(processInfo.hProcess);
			::CloseHandle(processInfo.hThread);

			// read the output file.

			auto file = ::_wfopen(out.c_str(), L"rt");

			if (!file)
			{
				Msg("Unable to open \"resources.txt\".", "Error", MB_OK | MB_ICONERROR);
			}
			else
			{
				char buffer[512];

				while (::fgets(buffer, sizeof(buffer), file))
				{
					for (int i = ::strlen(buffer) - 1; i >= 0; i--)
						if (buffer[i] == '\r' || buffer[i] == '\n')
							buffer[i] = 0;
						else
							break;

					// format: "range:<uint64>-<uint64>"

					if (::strlen(buffer) == 6 + 16 + 1 + 16 &&
						::strstr(buffer, "range:") == buffer &&
						buffer[6 + 16] == '-')
					{
						buffer[6 + 16] = 0;

						resources.emplace_back(
							::_strtoui64(buffer + 6, NULL, 16),
							::_strtoui64(buffer + 6 + 16 + 1, NULL, 16));
					}
				}

				::fclose(file);
			}

			// delete the output file.

			::DeleteFileW(out.c_str());
		}

		// open the BC device.

		auto device = ::CreateFileW(L"\\\\.\\BugChecker",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (device == INVALID_HANDLE_VALUE)
		{
			Msg("Unable to open BugChecker device. Click on the \"Start Driver\" button below.", "Error", MB_OK | MB_ICONERROR);
		}
		else
		{
			// get the driver version.

			DWORD version = 0;
			DWORD bytesRet = 0;

			if (!::DeviceIoControl(device, IOCTL_BUGCHECKER_GET_VERSION, NULL, 0, &version, sizeof(version), &bytesRet, NULL) ||
				bytesRet != sizeof(version))
			{
				Msg("DIoC of IOCTL_BUGCHECKER_GET_VERSION failed.", "Error", MB_OK | MB_ICONERROR);
			}
			else
			{
				// check the driver version.

				if (version != _BUGCHECKER_DRIVER_VERSION_)
				{
					Msg("Wrong driver version.", "Error", MB_OK | MB_ICONERROR);
				}
				else
				{
					// do the memory search.

					auto memSearch = [&device, &Msg](ULONGLONG pattern, ULONGLONG address, ULONGLONG size) -> eastl::pair<BOOL, LONGLONG> {

						DWORD bytesRet = 0;

						DoPhysMemSearchInputData input;
						input.pattern = pattern;
						input.mask = 0x00FFFFFF00FFFFFF;
						input.address = address;
						input.size = size;

						DoPhysMemSearchOutputData output;
						output.relativePos = -1;

						if (!::DeviceIoControl(device, IOCTL_BUGCHECKER_DO_PHYS_MEM_SEARCH, &input, sizeof(input), &output, sizeof(output), &bytesRet, NULL) ||
							bytesRet != sizeof(output))
						{
							Msg("DIoC of IOCTL_BUGCHECKER_DO_PHYS_MEM_SEARCH failed.", "Error", MB_OK | MB_ICONERROR);
							return eastl::pair<BOOL, LONGLONG>(FALSE, -1);
						}
						else
						{
							return eastl::pair<BOOL, LONGLONG>(TRUE, output.relativePos);
						}
					};

					auto BGRtoRGB = [](ULONGLONG c) -> ULONGLONG { return ((c & 0xFF) << 16) | (c & 0xFF00) | (c >> 16); };

					for (auto& res : resources)
					{
						auto ret0 = memSearch((BGRtoRGB(TEST_PIXEL_10) << 32) | BGRtoRGB(TEST_PIXEL_00), res.first, _MIN_(128 * 1024, res.second - res.first));

						if (!ret0.first)
							break;
						else if (ret0.second != -1)
						{
							auto ret1 = memSearch((BGRtoRGB(TEST_PIXEL_11) << 32) | BGRtoRGB(TEST_PIXEL_01), res.first, _MIN_(128 * 1024, res.second - res.first));

							if (!ret1.first)
								break;
							else if (ret1.second != -1)
							{
								ULONGLONG address = res.first + ret0.second;
								ULONG stride = (ULONG)(ret1.second - ret0.second);

								DEVMODEA devMode = {};
								devMode.dmSize = sizeof(devMode);
								devMode.dmDriverExtra = 0;
								if (!::EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &devMode))
								{
									Msg("EnumDisplaySettingsA failed.", "Error", MB_OK | MB_ICONERROR);
								}
								else
								{
									// return the results as strings.

									char buff[128];

									::sprintf(buff, "%i", devMode.dmPelsWidth);
									returnData->width = buff;

									::sprintf(buff, "%i", devMode.dmPelsHeight);
									returnData->height = buff;

									::sprintf(buff, "%llX", address);
									returnData->address = buff;

									::sprintf(buff, "%i", stride);
									returnData->stride = buff;
								}
							}
						}
					}
				}
			}

			::CloseHandle(device);
		}

		// destroy the test window.

		CloseWnd(returnData);
	});
}

void SymLoader::EnableDisplays(HWND hWnd, BOOL enable)
{
	// compose the name of the process.

	auto exe = Utils::GetExeFolderWithBackSlash() + L"NativeUtil_";

	if (Utils::Is32bitOS())
		exe += L"32bit";
	else
		exe += L"64bit";

	exe += L".exe";

	// create the process.

	eastl::wstring cmd = L"\"" + exe + L"\"" + L" " + (enable ? L"-enable-all-displays" : L"-disable-all-displays");

	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;
	if (!::CreateProcessW(exe.c_str(), const_cast<LPWSTR>(cmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, Utils::GetExeFolderWithBackSlash().c_str(), &info, &processInfo))
	{
		::MessageBoxA(hWnd, ("Unable to start: \"" + Utils::WStringToAscii(exe) + "\"").c_str(), "Error", MB_OK | MB_ICONERROR);
	}
	else
	{
		// wait for the process to finish.

		::WaitForSingleObject(processInfo.hProcess, INFINITE);

		DWORD exitCode = 0;
		::GetExitCodeProcess(processInfo.hProcess, &exitCode);

		::CloseHandle(processInfo.hProcess);
		::CloseHandle(processInfo.hThread);

		// display a reboot message, if needed.

		if (exitCode == 1)
			::MessageBoxA(hWnd, "Please reboot the system to complete the operation.", "Reboot Needed", MB_OK | MB_ICONWARNING);
		else
			::MessageBoxA(hWnd, "Operation completed successfully.", "Ok", MB_OK);
	}
}

void SymLoader::SaveFbDetails(HWND hWnd, BOOL silent)
{
	// save the settings.

	auto setts = config->get("settings");
	if (setts)
	{
		auto fb = setts->GetOrAdd("framebuffer");
		if (fb)
		{
			fb->SetSetting("width", Utils::GetDlgItemText(hWnd, IDC_FB_WIDTH).c_str());
			fb->SetSetting("height", Utils::GetDlgItemText(hWnd, IDC_FB_HEIGHT).c_str());
			fb->SetSetting("address", Utils::GetDlgItemText(hWnd, IDC_FB_ADDRESS).c_str());
			fb->SetSetting("stride", Utils::GetDlgItemText(hWnd, IDC_FB_STRIDE).c_str());
		}
	}

	// save the file.

	if (!StructuredFile::SaveConfig(config.get()))
	{
		if (!silent) ::MessageBoxA(NULL, "Error writing configuration file.", "Fatal Error", MB_OK | MB_ICONERROR);
	}
	else
	{
		if (!silent) ::MessageBoxA(hWnd, "Operation completed successfully.", "Ok", MB_OK);
	}
}

void SymLoader::KdcomHookRadioClick(HWND hWnd)
{
	// save the setting.

	BOOL patch = ::IsDlgButtonChecked(hWnd, IDC_HOOK_PATCH) == BST_CHECKED;
	BOOL callback = ::IsDlgButtonChecked(hWnd, IDC_HOOK_CALLBACK) == BST_CHECKED;

	if (!patch && !callback)
		return;

	auto setts = config->get("settings");
	if (setts)
	{
		auto kdc = setts->GetOrAdd("kdcom");
		if (kdc)
		{
			kdc->SetSetting("hook", patch ? "patch" : callback ? "callback" : "");
		}
	}

	// save the file, silently.

	StructuredFile::SaveConfig(config.get());
}

void SymLoader::BreakIn(HWND hWnd)
{
	if (::MessageBoxA(hWnd,
		"This will cause an INT 3 in the Symbol Loader process.\r\n\r\nDo you want to continue?",
		"Please Confirm", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES)
	{
		::__debugbreak();
	}
}

BOOL SymLoader::VerifyInitResult(HWND hWnd)
{
	// open the BC device.

	auto device = ::CreateFileW(L"\\\\.\\BugChecker",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (device == INVALID_HANDLE_VALUE)
	{
		::MessageBoxA(hWnd, "Unable to open BugChecker device.", "Error", MB_OK | MB_ICONERROR);
	}
	else
	{
		// on scope exit: close the handle.

		scope_guard __CloseFile__([=]() { ::CloseHandle(device); });

		// get the driver version.

		DWORD version = 0;
		DWORD bytesRet = 0;

		if (!::DeviceIoControl(device, IOCTL_BUGCHECKER_GET_VERSION, NULL, 0, &version, sizeof(version), &bytesRet, NULL) ||
			bytesRet != sizeof(version))
		{
			::MessageBoxA(hWnd, "DIoC of IOCTL_BUGCHECKER_GET_VERSION failed.", "Error", MB_OK | MB_ICONERROR);
		}
		else
		{
			// check the driver version.

			if (version != _BUGCHECKER_DRIVER_VERSION_)
			{
				::MessageBoxA(hWnd, "Wrong driver version.", "Error", MB_OK | MB_ICONERROR);
			}
			else
			{
				// query the result of BugChecker's initialization.

				BugCheckerInitResult initResult = {};
				bytesRet = 0;

				if (!::DeviceIoControl(device, IOCTL_BUGCHECKER_GET_INIT_RESULT, NULL, 0, &initResult, sizeof(initResult), &bytesRet, NULL) ||
					bytesRet != sizeof(initResult))
				{
					::MessageBoxA(hWnd, "DIoC of IOCTL_BUGCHECKER_GET_INIT_RESULT failed.", "Error", MB_OK | MB_ICONERROR);
				}
				else
				{
					// check if there were errors.

					if (!initResult.resultCode)
					{
						// return success.

						return TRUE;
					}
					else
					{
						auto msg = eastl::string("There was a problem during BugChecker initialization:\r\n\r\n<< ") + initResult.resultString + " >>\r\n\r\nPlease note that the driver is still running.";

						switch (initResult.resultCode)
						{
						case BcInitError_CalculateKernelOffsetsFailed:
						{
							msg += "\r\n\r\nPossible solution: download the symbol file of NTOSKRNL. "
								"Please note that sometimes Windows Update replaces NTOSKRNL with a newer version and symbols need to be downloaded again. "
								"Do you want to download the symbols of NTOSKRNL?";

							if (::MessageBoxA(hWnd, msg.c_str(), "Error", MB_YESNO | MB_ICONERROR) == IDYES)
								AddSymFileDlg::Show(hInst, hWnd, Utils::GuidToString(initResult.kernPdbGuid).c_str(), Utils::Dword32ToString(initResult.kernPdbAge).c_str(), initResult.kernPdbName);
						}
						break;

						case BcInitError_CallSetCallbacks_ProcAddress_GetModuleBase_Failed:
						case BcInitError_CallSetCallbacks_ProcAddress_GetProcAddress_Failed:
						case BcInitError_PatchKdCom_ProcAddress_GetModuleBase_Failed:
						case BcInitError_PatchKdCom_ProcAddress_GetProcAddress_KdSendPacket_Failed:
						case BcInitError_PatchKdCom_ProcAddress_GetProcAddress_KdReceivePacket_Failed:
						case BcInitError_PatchKdCom_FunctionPatch_Patch_KdSendPacket_Failed:
						case BcInitError_PatchKdCom_FunctionPatch_Patch_KdReceivePacket_Failed:
						{
							msg += "\r\n\r\nPossible solution: try clicking on \"Copy/Replace KDCOM\" and rebooting. "
								"Please note that sometimes Windows Update replaces KDCOM.DLL with a newer version and the BugChecker version needs to be restored.";

							::MessageBoxA(hWnd, msg.c_str(), "Error", MB_OK | MB_ICONERROR);
						}
						break;

						default:
						{
							::MessageBoxA(hWnd, msg.c_str(), "Error", MB_OK | MB_ICONERROR);
						}
						break;
						}
					}
				}
			}
		}
	}

	// return failure.

	return FALSE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// compose the BugChecker data and symbols directory.

	wchar_t buff[MAX_PATH];
	if (!::GetWindowsDirectoryW(buff, MAX_PATH))
	{
		::MessageBoxW(NULL, L"Unable to get Windows installation directory.", L"Fatal Error", MB_OK | MB_ICONERROR);
		return 0;
	}

	SymLoader::BcDir = eastl::wstring(buff) + L"\\BugChecker";

	// create the BC dir and test if we can write into it.

	::CreateDirectoryW(SymLoader::BcDir.c_str(), NULL);

	auto testFileName = SymLoader::BcDir + L"\\test_file.bin";

	auto f = ::_wfopen(testFileName.c_str(), L"wb");
	if (f) ::fclose(f);

	::DeleteFileW(testFileName.c_str());

	if (!f)
	{
		::MessageBoxW(NULL, (L"Unable to write to \"" + SymLoader::BcDir + L"\".\r\n\r\nPlease run the program with administrative privileges.").c_str(), L"Fatal Error", MB_OK | MB_ICONERROR);
		return 0;
	}

	// create the main window.

	BOOL(WINAPI * _SetProcessDPIAware)(void) = GetProcAddress(LoadLibraryA("USER32.DLL"), "SetProcessDPIAware");
	if (_SetProcessDPIAware) _SetProcessDPIAware(); // maybe we should remove this in the final version!

	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	::InitCommonControlsEx(&icc); // required to use the List View on Win XP.

	SymLoader::hInst = hInstance;

	if (!SymLoader::RegisterMainWindowClass() || !(SymLoader::hMainWnd = SymLoader::CreateMainWindow(nCmdShow)))
		return FALSE;

	HACCEL hAccelTable = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SYMLOADER));

	// main message loop.

	MSG msg;

	while (::GetMessage(&msg, nullptr, 0, 0))
	{
		if (!::TranslateAccelerator(msg.hwnd, hAccelTable, &msg) && !::IsDialogMessage(SymLoader::hMainWnd, &msg))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}
