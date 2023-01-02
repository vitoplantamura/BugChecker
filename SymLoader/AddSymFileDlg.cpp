#include "AddSymFileDlg.h"

#include "Utils.h"
#include "SymLoader.h"
#include "StructuredFile.h"

#include "resource.h"

#include <stdio.h>
#include <wchar.h>

#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/utility.h>
#include <EASTL/unique_ptr.h>

//

eastl::string AddSymFileDlg::initPdbGuid;
eastl::string AddSymFileDlg::initPdbAge;
eastl::string AddSymFileDlg::initPdbName;

//

enum class ThreadCompletedResult {
	Ok,
	Error_OfferToOpenUrl,
	Error_PrintMsg
};

class ThreadReturnData
{
public:
	eastl::string res;

	GUID pdbGuid;
	ULONG pdbAge;

	BOOL isKernel;

	ULONG bcsFileSize;
};

using ThreadCompletedParamType = eastl::pair<ThreadCompletedResult, ThreadReturnData>;

//

void AddSymFileDlg::Show(HINSTANCE hInst, HWND hWnd, const CHAR* pdbGuid /*= NULL*/, const CHAR* pdbAge /*= NULL*/, const CHAR* pdbName /*= NULL*/)
{
	initPdbGuid = "";
	initPdbAge = "";
	initPdbName = "";

	if (pdbGuid && pdbAge && pdbName)
	{
		initPdbGuid = pdbGuid;
		initPdbAge = pdbAge;
		initPdbName = pdbName;
	}

	DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDWND), hWnd, Func);
}

INT_PTR AddSymFileDlg::InitDialog(HWND hDlg)
{
	// create the status bar.

	::CreateWindowExW(0, STATUSCLASSNAMEW, NULL, WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0, hDlg,
		reinterpret_cast<HMENU>(IDC_STATUS_BAR), ::GetModuleHandleW(NULL), NULL);

	// get the previous settings from the config file.

	eastl::string msdiaCurr = "140";
	eastl::string serverCurr = "";

	auto setts = SymLoader::config->get("settings");
	if (setts)
	{
		auto add = setts->get("add_sym_win");
		if (add)
		{
			auto msdiaSett = add->GetSetting("msdia");
			auto serverSett = add->GetSetting("server");

			if (msdiaSett) msdiaCurr = msdiaSett->name;
			if (serverSett) serverCurr = serverSett->name;
		}
	}

	// initialize the combo box.

	auto cb = ::GetDlgItem(hDlg, IDC_SYMSERVER);

	auto add = [&](const CHAR* psz) { ::SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)psz); };

	add("https://msdl.microsoft.com/download/symbols/");
	add("https://chromium-browser-symsrv.commondatastorage.googleapis.com");
	add("https://symbols.mozilla.org/");
	add("https://software.intel.com/sites/downloads/symbols/");
	add("https://driver-symbols.nvidia.com/");
	add("https://download.amd.com/dir/bin");

	::SendMessage(cb, CB_SETCURSEL, 0, 0);

	if (serverCurr.size()) ::SetWindowTextA(cb, serverCurr.c_str());

	// initialize the radio buttons.

	auto is32 = Utils::Is32bitOS();

	::SetWindowTextA(::GetDlgItem(hDlg, IDC_MSDIA140), is32 ? "Use MSDIA140.DLL 32-bit (recommended)" : "Use MSDIA140.DLL 64-bit (recommended)");
	::SetWindowTextA(::GetDlgItem(hDlg, IDC_MSDIA100), is32 ? "Use MSDIA100.DLL 32-bit (for XP compatibility)" : "Use MSDIA100.DLL 64-bit (for XP compatibility)");

	::SendMessage(::GetDlgItem(hDlg, msdiaCurr == "140" ? IDC_MSDIA140 : IDC_MSDIA100), BM_SETCHECK, BST_CHECKED, 0);

	// set the status bar text.

	::SendMessageA(::GetDlgItem(hDlg, IDC_STATUS_BAR), SB_SETTEXTA, 0, (LPARAM)"Ready.");

	// set the text of the edit controls.

	::SetWindowTextA(::GetDlgItem(hDlg, IDC_PDBGUID), initPdbGuid.c_str());
	::SetWindowTextA(::GetDlgItem(hDlg, IDC_PDBAGE), initPdbAge.c_str());
	::SetWindowTextA(::GetDlgItem(hDlg, IDC_PDBFILENAME), initPdbName.c_str());

	// set the focus.

	::SetFocus(::GetDlgItem(hDlg, IDC_PDBFILENAME));

	// return FALSE after setting the focus.

	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK AddSymFileDlg::Func(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
	case WM_INITDIALOG:
		return InitDialog(hDlg);

	case WM_DOWNLOAD_THREAD_COMPLETED:
		return DownloadThreadCompleted(hDlg, wParam);

	case WM_COMMAND:

		if (LOWORD(wParam) == IDC_FROMFILE)
			FromFile(hDlg);
		else if (LOWORD(wParam) == IDC_DOWNLOAD)
			Download(hDlg, DownloadAction::Everything, L"");
		else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;

	case WM_DESTROY:
	{
		// save some control state in the config file.

		BOOL msdia140Checked = ::IsDlgButtonChecked(hDlg, IDC_MSDIA140) == BST_CHECKED;

		auto symServer = Utils::GetDlgItemText(hDlg, IDC_SYMSERVER);
		symServer.trim(" /");
		symServer += "/";

		auto setts = SymLoader::config->get("settings");
		if (setts)
		{
			auto add = setts->GetOrAdd("add_sym_win");
			if (add)
			{
				add->SetSetting("msdia", msdia140Checked ? "140" : "100");
				add->SetSetting("server", symServer.c_str());
			}
		}
	}
		break;
	}

	return (INT_PTR)FALSE;
}

void AddSymFileDlg::FromFile(HWND hWnd)
{
	// delete the 2 output files, eventually present from a previous execution.

	auto pdbOutput = Utils::GetExeFolderWithBackSlash() + L"output.pdb";
	auto bcsOutput = Utils::GetExeFolderWithBackSlash() + L"output.bcs";

	::DeleteFileW(pdbOutput.c_str());
	::DeleteFileW(bcsOutput.c_str());

	// declare pdb vars required later.

	CHAR pdbName[256] = {};
	GUID pdbGuid = {};
	DWORD pdbAge = 0;

	// show the open file dialog.

	OPENFILENAMEW ofn;
	wchar_t szFile[260] = {};

	::ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
	ofn.lpstrFilter = L"*.EXE;*.SYS;*.DLL;*.PDB;*.BCS\0*.EXE;*.SYS;*.DLL;*.PDB;*.BCS\0*.*\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrTitle = Utils::Is32bitOS() ? L"Open" : L">>>>>> WARNING <<<<<< go to \"C:\\Windows\\SysNative\" for 64bit SYSTEM32!";

	if (!::GetOpenFileNameW(&ofn))
	{
		return;
	}
	else
	{
		// get the file extension.

		eastl::wstring ext = ofn.lpstrFile;

		ext.erase(0, ext.find_last_of(L'.') + 1);

		ext = Utils::AsciiToLower(ext);

		// === SPECIAL CASE ===: pdb and bcs files are processed in a different way.

		if (ext == L"pdb")
		{
			if (!::CopyFileW(ofn.lpstrFile, pdbOutput.c_str(), FALSE))
			{
				::MessageBoxA(hWnd, "Unable to copy input file.", "Error", MB_OK | MB_ICONERROR);
				return;
			}

			Download(hWnd, DownloadAction::ConvertAndCheckBcs, ofn.lpstrFile);
			return;
		}
		else if (ext == L"bcs")
		{
			if (!::CopyFileW(ofn.lpstrFile, bcsOutput.c_str(), FALSE))
			{
				::MessageBoxA(hWnd, "Unable to copy input file.", "Error", MB_OK | MB_ICONERROR);
				return;
			}

			Download(hWnd, DownloadAction::OnlyCheckBcs, ofn.lpstrFile);
			return;
		}

		// read the entire file; we should optimize this to read only the portions that we need...

		auto f = ::_wfopen(ofn.lpstrFile, L"rb");
		if (!f)
		{
			::MessageBoxA(hWnd, "Unable to open the file.", "Error", MB_OK | MB_ICONERROR);
			return;
		}
		else
		{
			::fseek(f, 0L, SEEK_END);
			auto filesize = ::ftell(f);
			::fseek(f, 0L, SEEK_SET);

			auto buffer = new BYTE[filesize];
			if (::fread(buffer, 1, filesize, f) != filesize)
				filesize = 0;
			::fclose(f);

			if (filesize <= 0)
			{
				::MessageBoxA(hWnd, "Unable to read the file.", "Error", MB_OK | MB_ICONERROR);
				return;
			}
			else
			{
				// try to parse the PE file, in order to extract the debug info.

				IMAGE_DOS_HEADER* pdh = (IMAGE_DOS_HEADER*)buffer;

				if (pdh->e_magic == IMAGE_DOS_SIGNATURE)
				{
					IMAGE_NT_HEADERS64* pnth = (IMAGE_NT_HEADERS64*)(buffer + pdh->e_lfanew);

					if (pnth->Signature == IMAGE_NT_SIGNATURE)
					{
						int sizeofINTH = 0;
						IMAGE_DATA_DIRECTORY* pdeb = NULL;

						if (pnth->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
						{
							sizeofINTH = sizeof(IMAGE_NT_HEADERS32);
							pdeb = &((IMAGE_NT_HEADERS32*)pnth)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
						}
						else if (pnth->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
						{
							sizeofINTH = sizeof(IMAGE_NT_HEADERS64);
							pdeb = &((IMAGE_NT_HEADERS64*)pnth)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
						}

						if (sizeofINTH && pdeb)
						{
							auto nsec = pnth->FileHeader.NumberOfSections;

							IMAGE_SECTION_HEADER* psec = (IMAGE_SECTION_HEADER*)((ULONG_PTR)pnth + sizeofINTH);

							auto ndeb = pdeb->Size / sizeof(IMAGE_DEBUG_DIRECTORY);

							ULONG rva = 0;

							for (int i = 0; i < nsec; i++)
							{
								auto& sh = psec[i];

								if (!pdeb->VirtualAddress)
									continue;
								if (pdeb->VirtualAddress >= sh.VirtualAddress)
								{
									if ((pdeb->VirtualAddress + pdeb->Size) <= (sh.VirtualAddress + sh.SizeOfRawData))
									{
										rva = pdeb->VirtualAddress - sh.VirtualAddress + sh.PointerToRawData;
										break;
									}
								}
							}

							if (rva && rva < (ULONG)filesize)
							{
								IMAGE_DEBUG_DIRECTORY* dir = (IMAGE_DEBUG_DIRECTORY*)(buffer + rva);

								for (DWORD i = 0; i < ndeb; i++)
								{
									if (dir[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
									{
										BYTE* cv = (BYTE*)(buffer + dir[i].PointerToRawData);
										ULONG dim = dir[i].SizeOfData;

										DWORD sig = *(DWORD*)cv;

										if (sig == 'SDSR')
										{
											BYTE* guid = cv + sizeof(DWORD);
											pdbGuid = *(GUID*)guid;

											DWORD* age = (DWORD*)(guid + 16);
											pdbAge = *age;

											CHAR* pdbn = (CHAR*)((BYTE*)age + sizeof(DWORD));
											ULONG nfn = dim - (ULONG)((ULONG_PTR)pdbn - (ULONG_PTR)cv);
											::memcpy(pdbName, pdbn, nfn < sizeof(pdbName) - 1 ? nfn : sizeof(pdbName) - 1); // pdbName is already zero-initialized.

											break;
										}
									}
								}
							}
						}
					}
				}
			}

			delete[] buffer;
		}
	}

	// display the pdb info.

	if (!::strlen(pdbName))
	{
		::MessageBoxA(hWnd, "Unable to parse the file or PDB info not present.", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	::SetWindowTextA(::GetDlgItem(hWnd, IDC_PDBFILENAME), pdbName);

	CHAR szGuid[128];

	::sprintf(szGuid, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
		pdbGuid.Data1, pdbGuid.Data2, pdbGuid.Data3,
		pdbGuid.Data4[0], pdbGuid.Data4[1], pdbGuid.Data4[2], pdbGuid.Data4[3],
		pdbGuid.Data4[4], pdbGuid.Data4[5], pdbGuid.Data4[6], pdbGuid.Data4[7]);

	::SetWindowTextA(::GetDlgItem(hWnd, IDC_PDBGUID), szGuid);

	CHAR szAge[64];

	::sprintf(szAge, "%X", pdbAge);

	::SetWindowTextA(::GetDlgItem(hWnd, IDC_PDBAGE), szAge);

	// try to download the file.

	Download(hWnd, DownloadAction::Everything, ofn.lpstrFile);
}

eastl::vector<int> AddSymFileDlg::GetAllDlgCtrlIds()
{
	return eastl::vector<int> { IDC_SYMSERVER, IDC_PDBFILENAME, IDC_PDBGUID, IDC_PDBAGE, IDC_MSDIA140, IDC_MSDIA100, IDC_FROMFILE, IDC_DOWNLOAD, IDCANCEL };
}

void AddSymFileDlg::Download(HWND hWnd, DownloadAction action, const eastl::wstring& sourceFile)
{
	// delete the 2 output files, eventually present from a previous execution.
	//   if ConvertAndCheckBcs, pdbOutput is the input file.
	//   if OnlyCheckBcs, bcsOutput is the input file.

	auto pdbOutput = Utils::GetExeFolderWithBackSlash() + L"output.pdb";
	auto bcsOutput = Utils::GetExeFolderWithBackSlash() + L"output.bcs";

	if (action != DownloadAction::ConvertAndCheckBcs)
		::DeleteFileW(pdbOutput.c_str());

	if (action != DownloadAction::OnlyCheckBcs)
		::DeleteFileW(bcsOutput.c_str());

	// disable all the controls.

	Utils::EnableDlgItems(hWnd, GetAllDlgCtrlIds(), FALSE);

	// get the guid.

	auto g = Utils::GetDlgItemText(hWnd, IDC_PDBGUID);
	eastl::string pdbGuid;

	for (const CHAR c : g)
		if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))
			pdbGuid += c;

	// get the server.

	auto symServer = Utils::GetDlgItemText(hWnd, IDC_SYMSERVER);

	symServer.trim(" /");

	symServer += "/";

	// get the other url components.

	auto pdbFilename = Utils::GetDlgItemText(hWnd, IDC_PDBFILENAME);

	pdbFilename.trim(" ");

	auto pdbAge = Utils::GetDlgItemText(hWnd, IDC_PDBAGE);

	pdbAge.trim(" ");

	// check the state of the radio button.

	BOOL msdia140Checked = ::IsDlgButtonChecked(hWnd, IDC_MSDIA140) == BST_CHECKED;

	auto statusBarHWnd = ::GetDlgItem(hWnd, IDC_STATUS_BAR);

	// start the worker thread.

	Utils::CreateThread([=]() {

		ThreadCompletedResult resCode = ThreadCompletedResult::Ok;

		ThreadReturnData retData{};
		auto& resStr = retData.res;

		eastl::wstring exe = L"";

		// download the PDB file.

		if (action == DownloadAction::Everything)
		{
			// set the status bar text.

			::PostMessageA(statusBarHWnd, SB_SETTEXTA, 0, (LPARAM)(action == DownloadAction::Everything ? "Step 1 of 2: downloading PDB file..." : "Downloading PDB file..."));

			// compose the url.

			eastl::string url =
				symServer +
				pdbFilename +
				"/" +
				pdbGuid +
				pdbAge +
				"/" +
				pdbFilename;

			// try to download the file.

			HRESULT hr = ::URLDownloadToFileW(NULL, Utils::AsciiToWString(url).c_str(), pdbOutput.c_str(), 0, NULL);

			if (FAILED(hr))
			{
				resCode = ThreadCompletedResult::Error_OfferToOpenUrl;
				resStr = url;
			}
		}

		// convert the PDB file to a BCS file.

		if (resCode == ThreadCompletedResult::Ok && action != DownloadAction::OnlyCheckBcs)
		{
			// update the status bar.

			::PostMessageA(statusBarHWnd, SB_SETTEXTA, 0, (LPARAM)(action == DownloadAction::Everything ? "Step 2 of 2: generating BCS file..." : "Generating BCS file..."));

			// compose the name of the pdb.exe process.

			exe = Utils::GetExeFolderWithBackSlash() + L"pdb_";

			if (msdia140Checked)
				exe += L"msdia140";
			else
				exe += L"msdia100";

			exe += L"_";

			if (Utils::Is32bitOS())
				exe += L"32bit";
			else
				exe += L"64bit";

			exe += L".exe";

			// convert the PDB to BCS.

			eastl::wstring cmd = L"\"" + exe + L"\"" + L" " + L"\"" + pdbOutput + L"\"";

			STARTUPINFO info = { sizeof(info) };
			PROCESS_INFORMATION processInfo;
			if (!::CreateProcessW(exe.c_str(), const_cast<LPWSTR>(cmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, Utils::GetExeFolderWithBackSlash().c_str(), &info, &processInfo))
			{
				resCode = ThreadCompletedResult::Error_PrintMsg;
				resStr = "Unable to start: \"" + Utils::WStringToAscii(exe) + "\"";
			}
			else
			{
				// wait for the pdb.exe process to finish.

				::WaitForSingleObject(processInfo.hProcess, INFINITE);
				::CloseHandle(processInfo.hProcess);
				::CloseHandle(processInfo.hThread);
			}
		}

		// check the BCS file.

		if (resCode == ThreadCompletedResult::Ok)
		{
			// try to open the BCS file.

			auto f = ::_wfopen(bcsOutput.c_str(), L"rb");
			if (!f)
			{
				resCode = ThreadCompletedResult::Error_PrintMsg;
				resStr =
					exe.size() == 0 ? "Unable to open the BCS file." :
					"Error executing \"" + Utils::WStringToAscii(exe) + "\" (no BCS file created).\r\n\r\nPlease make sure that you registered the correct version of MSDIAxxx.DLL. Please note that if you are on a 64bit OS, you should register the 64bit version of MsDia, and vice versa.";
			}
			else
			{
				::fseek(f, 0L, SEEK_END);
				auto filesize = ::ftell(f);
				::fseek(f, 0L, SEEK_SET);

				auto buffer = new BYTE[filesize];
				if (::fread(buffer, 1, filesize, f) != filesize)
					filesize = 0;
				::fclose(f);

				if (filesize <= 0)
				{
					resCode = ThreadCompletedResult::Error_PrintMsg;
					resStr = "Error reading the BCS file.";
				}
				else if (!IsBcsValid(reinterpret_cast<BCSFILE_HEADER*>(buffer)))
				{
					resCode = ThreadCompletedResult::Error_PrintMsg;
					resStr = "Invalid BCS file.";
				}
				else
				{
					// try to guess whether the symbols refer to a kernel image.

					BOOL isKernel = FALSE;

					if (GetDatatype(buffer, "_LDR_DATA_TABLE_ENTRY") &&
						GetDatatype(buffer, "_DBGKD_GET_VERSION64"))
					{
						isKernel = TRUE;
					}

					// return the info.

					resCode = ThreadCompletedResult::Ok;

					retData.pdbGuid = *(GUID*)reinterpret_cast<BCSFILE_HEADER*>(buffer)->guid;
					retData.pdbAge = reinterpret_cast<BCSFILE_HEADER*>(buffer)->age;

					retData.isKernel = isKernel;

					retData.bcsFileSize = filesize;

					retData.res = sourceFile.size() == 0 ? pdbFilename : Utils::WStringToAscii(sourceFile);
					retData.res.erase(0, retData.res.find_last_of("/\\") + 1);
				}

				delete[] buffer;
			}
		}

		// call the completion function on the UI thread.

		::PostMessageA(hWnd, WM_DOWNLOAD_THREAD_COMPLETED, reinterpret_cast<WPARAM>(new ThreadCompletedParamType(resCode, retData)), 0);
	});
}

INT_PTR AddSymFileDlg::DownloadThreadCompleted(HWND hWnd, WPARAM wParam)
{
	eastl::unique_ptr<ThreadCompletedParamType> res(reinterpret_cast<ThreadCompletedParamType*>(wParam));

	// re-enable the controls.

	Utils::EnableDlgItems(hWnd, GetAllDlgCtrlIds(), TRUE);

	// restore the status bar.

	::SendMessageA(::GetDlgItem(hWnd, IDC_STATUS_BAR), SB_SETTEXTA, 0, (LPARAM)"Ready.");

	// do the requested action.

	switch (res->first)
	{
	case ThreadCompletedResult::Ok:
	{
		// move the pdb and bcs files to the BugChecker dir.

		wchar_t szName[128];

		auto& g = res->second.pdbGuid;

		::swprintf(szName, L"%08lX%04hX%04hX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%X",
			g.Data1, g.Data2, g.Data3,
			g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
			g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7],
			res->second.pdbAge);

		auto fn = SymLoader::BcDir + L"\\" + szName;

		auto move = [](const eastl::wstring& src, const eastl::wstring& dst)
		{
			BOOL ret = ::CopyFileW(src.c_str(), dst.c_str(), FALSE);

			::DeleteFileW(src.c_str());

			return ret;
		};

		auto pdbSrc = Utils::GetExeFolderWithBackSlash() + L"output.pdb";
		auto bcsSrc = Utils::GetExeFolderWithBackSlash() + L"output.bcs";

		auto pdbDst = fn + L".pdb";
		auto bcsDst = fn + L".bcs";

		move(pdbSrc, pdbDst); // this can fail.

		if (!move(bcsSrc, bcsDst))
		{
			::MessageBoxA(hWnd, "Unable to copy BCS file to BugChecker's directory.", "Error", MB_OK | MB_ICONERROR);
			break;
		}

		// update the config file.

		{
			// open the file -- CHANGED: prefer re-using the main "config" instance already in memory.

#if 0
			eastl::unique_ptr<StructuredFile::Entry> config{ StructuredFile::LoadConfig() };
#else
			auto& config = SymLoader::config;
#endif

			StructuredFile::Entry* symbols;

			if (!config || !(symbols = config->get("symbols")))
			{
				::MessageBoxA(hWnd, "Unable to open/read configuration file.", "Error", MB_OK | MB_ICONERROR);
				break;
			}

			// if the symbol file is already present, delete it.

			auto bcsName = Utils::WStringToAscii(eastl::wstring(szName) + L".bcs");

			int index = -1;

			StructuredFile::Entry* bcs = symbols->get(bcsName.c_str(), &index);

			if (bcs && index >= 0)
				symbols->children.erase(symbols->children.begin() + index);

			// add the item (if it's a kernel bcs, put it on top).

			if (res->second.isKernel)
			{
				// remove a kernel bcs if already present.

				if (symbols->children.size() > 0)
				{
					auto firstIsk = symbols->children.begin()->GetSetting("is_kernel");
					if (firstIsk && firstIsk->name == "1")
						symbols->children.erase(symbols->children.begin());
				}

				// insert.

				symbols->children.insert(symbols->children.begin(), StructuredFile::Entry{ bcsName });
				bcs = symbols->children.begin();
			}
			else
			{
				symbols->children.push_back(StructuredFile::Entry{ bcsName });
				bcs = &symbols->children.back();
			}

			// save all the metadata.

			bcs->AddSetting("name", res->second.res.c_str());

			CHAR buffer[512];

			auto& pdbGuid = res->second.pdbGuid;

			::sprintf(buffer, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
				pdbGuid.Data1, pdbGuid.Data2, pdbGuid.Data3,
				pdbGuid.Data4[0], pdbGuid.Data4[1], pdbGuid.Data4[2], pdbGuid.Data4[3],
				pdbGuid.Data4[4], pdbGuid.Data4[5], pdbGuid.Data4[6], pdbGuid.Data4[7]);

			bcs->AddSetting("pdb_guid", buffer);

			::sprintf(buffer, "%X", res->second.pdbAge);

			bcs->AddSetting("pdb_age", buffer);

			bcs->AddSetting("is_kernel", res->second.isKernel ? "1" : "0");

			::sprintf(buffer, "%i", res->second.bcsFileSize);

			bcs->AddSetting("bcs_file_size", buffer);

			// write the config file.

			if (!StructuredFile::SaveConfig(config.get()))
			{
				::MessageBoxA(hWnd, "Unable to write configuration file.", "Error", MB_OK | MB_ICONERROR);
				break;
			}
		}

		// close the dialog.

		::EndDialog(hWnd, 0);

		// update the list view on the parent window.

		SymLoader::UpdateSymbolsList(SymLoader::hMainWnd);
	}
	break;

	case ThreadCompletedResult::Error_OfferToOpenUrl:
		if (::MessageBoxA(hWnd,
			"Unable to download the symbol file.\r\n\r\nIf you are on Windows XP (which has outdated TLS support), you can install Chrome version 49, set it as the default browser and use it to download the PDB file. On XP, an other solution is to install the BugChecker's HttpToHttpsProxy on a separate machine with a more recent OS.\r\n\r\nDo you want to open the PDB url with the default browser?",
			"Download Error", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES)
		{
			::ShellExecuteA(0, 0, res->second.res.c_str(), 0, 0, SW_SHOW);
		}
		break;

	case ThreadCompletedResult::Error_PrintMsg:
		::MessageBoxA(hWnd, res->second.res.c_str(), "Error", MB_OK | MB_ICONERROR);
		break;
	}

	// return TRUE (message processed).

	return (INT_PTR)TRUE;
}

template<typename T, typename P>
static T* BinarySearch(T* table, int num, const P& pred)
{
	int start_index = 0;
	int end_index = num - 1;

	while (start_index <= end_index)
	{
		int middle = start_index + (end_index - start_index) / 2;

		auto& s = table[middle];

		auto comp = pred(s);
		if (!comp)
			return &s;
		else if (comp > 0)
			start_index = middle + 1;
		else
			end_index = middle - 1;
	}

	return NULL;
}

const BCSFILE_DATATYPE* AddSymFileDlg::GetDatatype(VOID* symbols, const char* typeName)
{
	// verify the integrity of the symbols file.

	BCSFILE_HEADER* header = (BCSFILE_HEADER*)symbols;

	if (!IsBcsValid(header) || header->datatypesSize == 0 || header->datatypeMembersSize == 0)
		return NULL;

	auto dts = (BCSFILE_DATATYPE*)((BYTE*)symbols + header->datatypes);
	int dtsNum = header->datatypesSize / sizeof(BCSFILE_DATATYPE);

	CHAR* names = (CHAR*)((BYTE*)symbols + header->names);

	// binary searches the datatype.

	auto search = ::BinarySearch(dts, dtsNum, [&typeName, &names](const auto& c) {
		return ::strcmp(typeName, names + c.name);
	});

	if (!search)
		return NULL;

	// multiple types with the same name can be present: return the most complete type.

	int i = search - dts - 1;

	while (i >= 0)
		if (::strcmp(typeName, names + dts[i].name))
			break;
		else
			i--;

	for (i++; i < dtsNum && !::strcmp(typeName, names + dts[i].name); i++)
		if (dts[i].length > search->length)
			search = &dts[i];

	return search;
}

BOOLEAN AddSymFileDlg::IsBcsValid(BCSFILE_HEADER* header)
{
	return
		header &&
		header->magic == BCSFILE_HEADER_SIGNATURE &&
		header->version == BCSFILE_HEADER_VERSION;
}
