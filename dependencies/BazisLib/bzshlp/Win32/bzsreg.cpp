#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)

#define _BZSREG_CPP_

#include "bzsreg.h"
#include <assert.h>

using namespace std;
using namespace BazisLib;

CRegistryParameters::CRegistryParameters(LPCTSTR pszKey)
{
	if (!pszKey)
		return;
	m_KeyName = pszKey;
	LoadKey(pszKey);
}

String CRegistryParameters::GetString(LPCTSTR pszKey)
{
	return m_StringParams[pszKey];
}

int CRegistryParameters::GetInteger(LPCTSTR pszKey)
{
	return m_IntParams[pszKey];
}

bool CRegistryParameters::SetInteger(LPCTSTR pszKey, int Value)
{
	m_IntParams[pszKey] = Value;
	return true;
}

bool CRegistryParameters::SetString(LPCTSTR pszKey, String &Value)
{
	m_StringParams[pszKey] = Value;
	return true;
}

bool CRegistryParameters::LoadKey(LPCTSTR pszKey)
{
	HKEY hKey = 0;
	TCHAR szName[256], szValue[256];
	DWORD dwNameSize;
	DWORD dwType;
	DWORD dwValueSize;

	if (!pszKey)
		return false;
	
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE, 0, NULL, &hKey, NULL))
		return false;

	for (DWORD i = 0; ; i++)
	{
		dwNameSize = sizeof(szName)/sizeof(szName[0]);
		dwValueSize = sizeof(szValue)/sizeof(szValue[0]);
		if (RegEnumValue(hKey, i, szName, &dwNameSize, 0, &dwType, (LPBYTE)szValue, &dwValueSize) != ERROR_SUCCESS)
			break;
		switch (dwType)
		{
		case REG_SZ:
		case REG_EXPAND_SZ:
			m_StringParams[String(szName)] = String(szValue);
			break;
		case REG_DWORD:
			m_IntParams[String(szName)] = reinterpret_cast<int&>(szValue);
			break;
		}
	}

	RegCloseKey(hKey);
	return true;
}

bool CRegistryParameters::SaveKey(LPCTSTR pszKey)
{
	HKEY hKey = 0;

	if (!pszKey)
		pszKey = m_KeyName.c_str();
	if (!pszKey || !pszKey[0])
		return false;

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszKey ? pszKey : m_KeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, 0, NULL, &hKey, NULL))
		return false;

	for (std::map <String, String>::iterator i = m_StringParams.begin(); i != m_StringParams.end(); i++)
	{
		if (RegSetValueEx(hKey, i->first.c_str(), 0, REG_SZ, (LPBYTE)i->second.c_str(), (DWORD)i->second.length()+1))
			assert(false);
	}
	for (std::map <String, int>::iterator i = m_IntParams.begin(); i != m_IntParams.end(); i++)
	{
		if (RegSetValueEx(hKey, i->first.c_str(), 0, REG_DWORD, (LPBYTE)&i->second, 4))
			assert(false);
	}

	RegCloseKey(hKey);
	return true;
}

void CRegistryParameters::Reset()
{
	m_StringParams.clear();
	m_IntParams.clear();
}
#endif