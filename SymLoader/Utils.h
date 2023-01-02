#pragma once

#include "framework.h"

#include <EASTL/functional.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>

#include <process.h>

class Utils
{
public:

	static void EnableDlgItems(HWND hWnd, const eastl::vector<int>& ctrls, BOOL bEnable);
	static eastl::string GetDlgItemText(HWND hWnd, int ctrl);
	static eastl::wstring GetExeFolderWithBackSlash();
	static eastl::wstring AsciiToWString(const eastl::string& str);
	static eastl::string WStringToAscii(const eastl::wstring& str);
	static BOOL Is32bitOS();
	static eastl::wstring AsciiToLower(const eastl::wstring& str);
	static BOOL FileExists(const wchar_t* psz);
	static eastl::string GuidToString(BYTE* guid);
	static eastl::string Dword32ToString(DWORD dw);

	template <typename Functor>
	static void CreateThread(Functor&& functor) // this works on XP. Make sure to capture everything by value.
	{
		using t = eastl::function<void()>;

		::_beginthread([](void* _f)
			{
				auto f = (t*)_f;
				(*f)();
				delete f;
			},
			0,
			new t(eastl::forward<Functor>(functor)));
	}

};
