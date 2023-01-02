#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "DeviceEnumerator.h"

extern "C" NTSTATUS __stdcall ObReferenceObjectByName(
	PUNICODE_STRING Name,
	ULONG Attributes,
	PACCESS_STATE AccessState OPTIONAL,
	ACCESS_MASK Access,
	POBJECT_TYPE Type,
	KPROCESSOR_MODE bMode,
	ULONG Unknown OPTIONAL,
	PVOID *BodyPointer);

extern "C" NTSTATUS
IoEnumerateDeviceObjectList(
							IN PDRIVER_OBJECT  DriverObject,
							IN PDEVICE_OBJECT  *DeviceObjectList,
							IN ULONG  DeviceObjectListSize,
							OUT PULONG  ActualNumberDeviceObjects
							); 


using namespace BazisLib::DDK;

BazisLib::DDK::DriverObjectFinder::DriverObjectFinder(const wchar_t *pszDriverName, POBJECT_TYPE pDriverObjectType)
	: m_pDriver(NULL)
{
	String strFullPath = L"\\Driver\\";
	if (wcschr(pszDriverName, '\\'))
		strFullPath = L"";
	strFullPath += pszDriverName;
	NTSTATUS status = ObReferenceObjectByName(strFullPath.ToNTString(), OBJ_CASE_INSENSITIVE, NULL, 0, pDriverObjectType, KernelMode, 0, (PVOID *)&m_pDriver);
	if (!NT_SUCCESS(status))
		return;
}

DeviceEnumerator::DeviceEnumerator(const wchar_t *pszDriverName, POBJECT_TYPE pDriverObjectType)
	: DriverObjectFinder(pszDriverName, pDriverObjectType)
	, m_ppDevices(NULL)
	, m_DeviceObjectCount(0)
{
	if (!Valid())
		return;

	ULONG actualCount = 0;
	NTSTATUS status = IoEnumerateDeviceObjectList(GetDriverObject(), NULL, 0, &actualCount);

	if (!actualCount)
	{
		Reset();
		return;
	}

	m_DeviceObjectCount = actualCount;
	m_ppDevices = new PDEVICE_OBJECT[actualCount];

	status = IoEnumerateDeviceObjectList(GetDriverObject(), m_ppDevices, actualCount * sizeof(PDEVICE_OBJECT), &actualCount);

	if (!NT_SUCCESS(status))
	{
		Reset();
		return;
	}
}

BazisLib::DDK::DeviceEnumerator::~DeviceEnumerator()
{
	if (m_ppDevices)
		for (size_t i = 0; i < m_DeviceObjectCount; i++)
			ObDereferenceObject(m_ppDevices[i]);

	delete m_ppDevices;
}
#endif