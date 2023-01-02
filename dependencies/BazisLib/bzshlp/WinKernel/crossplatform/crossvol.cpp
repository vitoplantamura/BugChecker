#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "crossvol.h"

using namespace BazisLib::DDK;
using namespace BazisLib;

UniversalVolume::UniversalVolume(const ConstManagedPointer <AIBasicDisk> &pDisk, char PartitionType) :
	BasicStorageVolume(L"BazisVolume", true, FILE_DEVICE_DISK),
	m_pDisk(pDisk),
	m_PartitionType(PartitionType)
{
#ifdef _DEBUG
	m_pszDeviceDebugName = "Volume";
#endif
}

NTSTATUS UniversalVolume::OnStartDevice(IN PCM_RESOURCE_LIST AllocatedResources, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated)
{
	if (!m_pDisk)
		return STATUS_INVALID_DEVICE_REQUEST;
	ActionStatus status = m_pDisk->Initialize();
	if (!status.Successful())
	{
#ifdef _DEBUG
		DbgPrint("UniversalVolume: Failed to initialize (%ws)\n", status.GetMostInformativeText().c_str());
#endif
		return status.ConvertToNTStatus();
	}
	return __super::OnStartDevice(AllocatedResources, AllocatedResourcesTranslated);
}


UniversalVolume::~UniversalVolume()
{
}

unsigned UniversalVolume::GetSectorSize()
{
	if (!m_pDisk)
		return 0;
	return m_pDisk->GetSectorSize();
}

ULONGLONG UniversalVolume::GetTotalSize()
{
	if (!m_pDisk)
		return 0;
	return m_pDisk->GetSectorCount() * m_pDisk->GetSectorSize();
}

char UniversalVolume::GetPartitionType()
{
	return m_PartitionType;
}

NTSTATUS UniversalVolume::Read(ULONGLONG ByteOffset, PVOID pBuffer, ULONG Length, PULONG_PTR pBytesDone)
{
	if (!m_pDisk)
		return STATUS_INVALID_DEVICE_STATE;
	ULONG done = m_pDisk->Read(ByteOffset, pBuffer, Length);
	if (!done)
		return STATUS_ACCESS_DENIED;
	*pBytesDone = done;
	return STATUS_SUCCESS;
}

NTSTATUS UniversalVolume::Write(ULONGLONG ByteOffset, PVOID pBuffer, ULONG Length, PULONG_PTR pBytesDone)
{
	if (!m_pDisk)
		return STATUS_INVALID_DEVICE_STATE;
	ULONG done = m_pDisk->Write(ByteOffset, pBuffer, Length);
	if (!done)
		return STATUS_ACCESS_DENIED;
	*pBytesDone = done;
	return STATUS_SUCCESS;
}

NTSTATUS UniversalVolume::OnGetStableGuid(LPGUID lpGuid)
{
	if (!m_pDisk)
		return STATUS_INVALID_DEVICE_STATE;
	LPCGUID lpDiskGuid = m_pDisk->GetStableGuid();
	if (!lpDiskGuid)
		return STATUS_NOT_SUPPORTED;
	*lpGuid = *lpDiskGuid;
	return STATUS_SUCCESS;
}

NTSTATUS UniversalVolume::OnDeviceControl(ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone)
{
	if ((DEVICE_TYPE_FROM_CTL_CODE(ControlCode) != FILE_DEVICE_VIRTUAL_DISK) || IsInternal || !m_pDisk)
		return __super::OnDeviceControl(ControlCode, IsInternal, pInOutBuffer, InputLength, OutputLength, pBytesDone);
	if (m_pDisk->DeviceControl(ControlCode, pInOutBuffer, InputLength, OutputLength, (unsigned *)pBytesDone))
		return STATUS_SUCCESS;
	else
		return STATUS_NOT_SUPPORTED;
}

NTSTATUS UniversalVolume::OnDiskIsWritable()
{
	if (!m_pDisk)
		return STATUS_MEDIA_WRITE_PROTECTED;
	if (!m_pDisk->IsWritable())
		return STATUS_MEDIA_WRITE_PROTECTED;
	return STATUS_SUCCESS;
}
#endif