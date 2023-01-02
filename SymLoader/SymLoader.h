#pragma once

#include "resource.h"

#include "StructuredFile.h"

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

class SymLoader
{
private:

	static BOOL RegisterMainWindowClass();
	static HWND CreateMainWindow(int nCmdShow);

	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static void ShowBcsInfo(HWND hWnd);
	static void RemoveBcsFromList(HWND hWnd);
	static void PhysMemSearch(HWND hWnd);
	static void EnableDisplays(HWND hWnd, BOOL enable);
	static void SaveFbDetails(HWND hWnd, BOOL silent = FALSE);
	static void KdcomHookRadioClick(HWND hWnd);
	static void BreakIn(HWND hWnd);
	static BOOL VerifyInitResult(HWND hWnd);

public:

	static HINSTANCE hInst;
	static HWND hMainWnd;

	static eastl::wstring BcDir;

	static eastl::unique_ptr<StructuredFile::Entry> config; // in sync with the List View.

	static BOOL UpdateSymbolsList(HWND hWnd, BOOL initRun = FALSE);

	friend int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow);
};
