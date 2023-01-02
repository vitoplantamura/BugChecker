#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "devapi.h"

using namespace BazisLib::DDK;

ExternalDeviceObjectReference::ExternalDeviceObjectReference(const wchar_t *pwszDevicePath, ACCESS_MASK AccessMask) :
	m_pDeviceObject(NULL),
	m_pFileObject(NULL)
{
	UNICODE_STRING nameString;
	if (!pwszDevicePath)
		return;
	RtlInitUnicodeString(&nameString, pwszDevicePath);
	NTSTATUS st = IoGetDeviceObjectPointer(&nameString, AccessMask, &m_pFileObject, &m_pDeviceObject);
	if (!NT_SUCCESS(st))
	{
		m_pDeviceObject = NULL;
		m_pFileObject = NULL;
	}
}
#endif