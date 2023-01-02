#include "Utils.h"

void Utils::EnableDlgItems(HWND hWnd, const eastl::vector<int>& ctrls, BOOL bEnable)
{
	for (int ctrl : ctrls)
		::EnableWindow(::GetDlgItem(hWnd, ctrl), bEnable);
}

eastl::string Utils::GetDlgItemText(HWND hWnd, int ctrl)
{
	HWND h = ::GetDlgItem(hWnd, ctrl);
	if (!h) return "";

	int l = ::GetWindowTextLengthA(h);
	if (l <= 0) return "";

	eastl::vector<CHAR> buffer(l + 16, 0);
	::GetWindowTextA(h, buffer.begin(), buffer.size());

	return eastl::string(buffer.begin());
}

eastl::wstring Utils::GetExeFolderWithBackSlash()
{
	TCHAR szFileName[MAX_PATH];
	::GetModuleFileName(NULL, szFileName, MAX_PATH);

	eastl::wstring exeFolder = szFileName;
	exeFolder.erase(exeFolder.find_last_of(L'\\') + 1);

	return exeFolder;
}

eastl::wstring Utils::AsciiToWString(const eastl::string& str)
{
	eastl::wstring ret;

	eastl::transform(str.begin(), str.end(), eastl::back_inserter(ret), [](const char c) {
		if (static_cast<BYTE>(c) & 0x80)
			return L'?';
		else
			return (wchar_t)c;
	});

	return ret;
}

eastl::string Utils::WStringToAscii(const eastl::wstring& str)
{
	eastl::string ret;

	eastl::transform(str.begin(), str.end(), eastl::back_inserter(ret), [](const wchar_t c) {
		if (static_cast<USHORT>(c) & 0b1111111110000000)
			return '?';
		else
			return (CHAR)c;
	});

	return ret;
}

BOOL Utils::Is32bitOS()
{
	SYSTEM_INFO info{};
	::GetNativeSystemInfo(&info);
	return info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL;
}

eastl::wstring Utils::AsciiToLower(const eastl::wstring& str)
{
	eastl::wstring ret;

	eastl::transform(str.begin(), str.end(), eastl::back_inserter(ret), [](const wchar_t in) -> wchar_t { if (in >= L'A' && in <= L'Z') return in - (L'Z' - L'z'); else return in; });

	return ret;
}

BOOL Utils::FileExists(const wchar_t* psz)
{
	auto f = ::_wfopen(psz, L"rb");
	if (f) ::fclose(f);
	return f != NULL;
}

eastl::string Utils::GuidToString(BYTE* guid)
{
	CHAR buffer[128];

	GUID* g = (GUID*)guid;

	::sprintf(buffer, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
		g->Data1, g->Data2, g->Data3,
		g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
		g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

	return buffer;
}

eastl::string Utils::Dword32ToString(DWORD dw)
{
	CHAR buffer[64];

	::sprintf(buffer, "%X", dw);

	return buffer;
}
