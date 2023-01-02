#pragma once

#include "framework.h"

#include "..\pdb\headers\bcsfile.h"

#include <EASTL/vector.h>
#include <EASTL/string.h>

class AddSymFileDlg
{
public:

	static void Show(HINSTANCE hInst, HWND hWnd, const CHAR* pdbGuid = NULL, const CHAR* pdbAge = NULL, const CHAR* pdbName = NULL);

private:

	static eastl::string initPdbGuid;
	static eastl::string initPdbAge;
	static eastl::string initPdbName;

	enum class DownloadAction
	{
		Everything,
		ConvertAndCheckBcs,
		OnlyCheckBcs
	};

	static const BCSFILE_DATATYPE* GetDatatype(VOID* symbols, const char* typeName); // from the sources of the SYS file.
	static BOOLEAN IsBcsValid(BCSFILE_HEADER* header); // from the sources of the SYS file.

	static INT_PTR CALLBACK Func(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static INT_PTR InitDialog(HWND hDlg);
	static void FromFile(HWND hWnd);
	static void Download(HWND hWnd, DownloadAction action, const eastl::wstring& sourceFile);
	static eastl::vector<int> GetAllDlgCtrlIds();
	static INT_PTR DownloadThreadCompleted(HWND hWnd, WPARAM wParam);
};
