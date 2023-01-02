#include "stdafx.h"
#ifdef BZSLIB_WINUSER
#include "../path.h"
#include <shlobj.h>

using namespace BazisLib;

String Path::GetCurrentPath()
{
	TCHAR tsz[MAX_PATH];
	GetCurrentDirectory(sizeof(tsz)/sizeof(tsz[0]), tsz);
	return tsz;
}

String Path::GetSpecialDirectoryLocation(SpecialDirectoryType dir)
{
	TCHAR tsz[MAX_PATH];
	tsz[0] = 0;
	switch (dir)
	{
	case dirWindows:
		GetWindowsDirectory(tsz, __countof(tsz));
		break;
	case dirSystem:
		GetSystemDirectory(tsz, __countof(tsz));
		break;
	case dirDrivers:
		GetSystemDirectory(tsz, __countof(tsz));
		_tcsncat_s(tsz, __countof(tsz), _T("\\drivers"), _TRUNCATE);
		break;
	case dirTemp:
		GetTempPath(__countof(tsz), tsz);
		break;
	default:
		SHGetSpecialFolderPath(HWND_DESKTOP, tsz, dir, TRUE);
		break;			
	}
	return tsz;
}

static std::vector<String> GetAllRootPaths()
{
	std::vector<String> allStrings;
	DWORD driveMask = GetLogicalDrives();

	TCHAR tszRoot[] = _T("?:\\");

	for (unsigned i = 0, j = (1 << (i - 'A')); i <= 'Z'; i++, j <<= 1)
		if (driveMask & j)
		{
			tszRoot[0] = i;
			allStrings.push_back(tszRoot);
		}
	return allStrings;
}

#endif