#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "registry.h"

BazisLib::ActionStatus BazisLib::DoDeleteSubkeyRecursive( HKEY hKey, LPCTSTR lpSubkey )
{
	DWORD i = 0;
	TCHAR tszSubkey[512];
	HKEY hSubkey = 0;
#ifdef UNDER_CE
	int err = RegOpenKeyEx(hKey, lpSubkey, 0, NULL, &hSubkey);
#else
	int err = RegOpenKey(hKey, lpSubkey, &hSubkey);
#endif
	if (err != ERROR_SUCCESS)
		return MAKE_STATUS(BazisLib::ActionStatus::FromWin32Error(err));
	if (!hSubkey)
		return MAKE_STATUS(BazisLib::UnknownError);
	DWORD dwNameSize = __countof(tszSubkey);

	ActionStatus lastProblem = MAKE_STATUS(BazisLib::Success);

#ifdef UNDER_CE
	while (!RegEnumKeyEx(hSubkey, i, tszSubkey, &dwNameSize, NULL, NULL, NULL, NULL))
#else
	while (!RegEnumKey(hSubkey, i, tszSubkey, dwNameSize))
#endif
	{
		//After deletion of a subkey, the subkey indexes are shifted. That way, normally, we should
		//allways delete subkey #0. However, in case of error, we skip it and try deleting next one.
		
		//Note that the correctness of this function strictly depends on whether it reports
		//deletion errors correctly.
		ActionStatus st = DoDeleteSubkeyRecursive(hSubkey, tszSubkey);
		if (!st.Successful())
			i++;
		else
			lastProblem = st;
		dwNameSize = __countof(tszSubkey);
	}
	RegCloseKey(hSubkey);
	err = RegDeleteKey(hKey, lpSubkey);
	if (err == ERROR_SUCCESS)
		return lastProblem;
	else
		return MAKE_STATUS(BazisLib::ActionStatus::FromWin32Error(err));
}
#endif