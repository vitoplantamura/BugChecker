#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "registry.h"

using namespace BazisLib;
using namespace BazisLib::DDK;

RegistryKey::RegistryKey(LPCWSTR pwszKeyPath, bool ReadOnly) :
	m_hKey(0)
{
	Initialize(0, pwszKeyPath, ReadOnly);
}

void BazisLib::DDK::RegistryKey::Initialize( HANDLE hRoot, const wchar_t *pwszSubkey, bool ReadOnly, size_t SubKeyLen )
{
	if (!pwszSubkey)
		return;
	UNICODE_STRING keyName;
	OBJECT_ATTRIBUTES keyAttributes;
	if (SubKeyLen == -1)
		RtlInitUnicodeString(&keyName, pwszSubkey);
	else
	{
		keyName.Buffer = (PWCH)pwszSubkey;
		keyName.Length = (USHORT)(SubKeyLen * sizeof(wchar_t));
		keyName.MaximumLength = keyName.Length + sizeof(wchar_t);
	}
	InitializeObjectAttributes(&keyAttributes, &keyName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, hRoot, NULL);
	NTSTATUS status;
	if (ReadOnly)
		status = ZwOpenKey(&m_hKey, KEY_READ, &keyAttributes);
	else
		status = ZwCreateKey(&m_hKey, ReadOnly ? KEY_READ : KEY_ALL_ACCESS, &keyAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, 0);
	if (!NT_SUCCESS(status))
		return;
}


RegistryKey::RegistryKey(const RegistryKey &rootKey, const wchar_t *ptszSubkey, bool ReadOnly, size_t SubKeyLen) :
	m_hKey(0)
{
	Initialize(rootKey.m_hKey, ptszSubkey, ReadOnly, SubKeyLen);
}

BazisLib::DDK::RegistryKey::RegistryKey(const accessor &acc, bool ReadOnly /*= false*/)
	: m_hKey(0)
{
	Initialize(acc.m_pKey->m_hKey, acc.m_pName, ReadOnly);
}

BazisLib::DDK::RegistryKey::RegistryKey( Win32RootKey hRootKey, const wchar_t *ptszSubkey, bool ReadOnly )
	: m_hKey(0)
{
	wchar_t *pwszPrefix = NULL;
	switch (hRootKey)
	{
	case HKEY_LOCAL_MACHINE:
		pwszPrefix = L"\\Registry\\Machine";
		break;
	case HKEY_USERS:
		pwszPrefix = L"\\Registry\\User";
		break;
	}
	if (!pwszPrefix)
		return;
	RegistryKey baseKey(pwszPrefix, true);
	if (!baseKey.Valid())
		return;
	Initialize(baseKey.m_hKey, ptszSubkey, ReadOnly);
}

NTSTATUS RegistryKey::ReadString(LPCWSTR pwszValueName, String &value)
{
	if (!pwszValueName)
		return STATUS_INVALID_PARAMETER;
	UNICODE_STRING valueName;
	RtlInitUnicodeString(&valueName, pwszValueName);
	ULONG resultLength = 0;
	NTSTATUS st = ZwQueryValueKey(m_hKey, &valueName, KeyValuePartialInformation, NULL, 0, &resultLength);
	if ((st != STATUS_BUFFER_TOO_SMALL) || !resultLength)
	{
		if (!NT_SUCCESS(st))
			return st;
		else
			return STATUS_UNEXPECTED_IO_ERROR;
	}
	PKEY_VALUE_PARTIAL_INFORMATION pInfo = (PKEY_VALUE_PARTIAL_INFORMATION)malloc(resultLength);
	if (!pInfo)
		return STATUS_NO_MEMORY;
	st = ZwQueryValueKey(m_hKey, &valueName, KeyValuePartialInformation, pInfo, resultLength, &resultLength);
	if (!NT_SUCCESS(st))
	{
		free(pInfo);
		return st;
	}

	switch (pInfo->Type)
	{
	case REG_DWORD:
		{
			wchar_t wsz[32];
			swprintf(wsz,L"%u", *(unsigned int*)pInfo->Data);
			break;
		}
	case REG_SZ:
		value = String((wchar_t *)pInfo->Data, (pInfo->DataLength / sizeof(wchar_t)) - 1);
		break;
	default:
		free(pInfo);
		return STATUS_BAD_FILE_TYPE;
	}

	free(pInfo);
	return STATUS_SUCCESS;
}

bool BazisLib::DDK::RegistryKey::DeleteSubKeyRecursive( LPCWSTR lpSubkey, size_t Length )
{
	ULONG i = 0;
	RegistryKey key(*this, lpSubkey, false, Length);
	if (!key.Valid())
		return false;

	TypedBuffer<KEY_BASIC_INFORMATION> pBuf;
	while (key.EnumSubkey(i, &pBuf))
	{
		//After deletion of a subkey, the subkey indexes are shifted. That way, normally, we should
		//allways delete subkey #0. However, in case of error, we skip it and try deleting next one.

		//Note that the correctness of this function strictly depends on whether it reports
		//deletion errors correctly.
		if (!key.DeleteSubKeyRecursive(pBuf->Name, pBuf->NameLength / sizeof(wchar_t)))
			i++;
	}
	return key.DeleteSelf();
}
#endif