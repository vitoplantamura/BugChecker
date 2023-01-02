#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "volume.h"

using namespace BazisLib::DDK;

BasicStorageVolume::BasicStorageVolume(LPCWSTR pwszDevicePrefix,
			  bool bDeleteThisAfterRemoveRequest,
			  ULONG DeviceType,
			  ULONG DeviceCharacteristics,
			  bool bExclusive,
			  ULONG AdditionalDeviceFlags) :
	StorageDevice(pwszDevicePrefix, bDeleteThisAfterRemoveRequest, DeviceType, DeviceCharacteristics, bExclusive, AdditionalDeviceFlags | DO_DIRECT_IO),
	m_SectorSize(0),
	m_SectorCount(0),
	m_SizeInBytes(0),
	m_ReportedDeviceType(DeviceType),
	m_pWinCacheBugWorkaroundBuffer(NULL),
	m_WinCacheBugWorkaroundBufferSize(0)
{
	CreateDeviceRequestQueue();
}

NTSTATUS BasicStorageVolume::OnStartDevice(IN PCM_RESOURCE_LIST AllocatedResources, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated)
{
	if (!UpdateVolumeSize())
		return STATUS_INVALID_DEVICE_STATE;
	NTSTATUS status = EnableWindowsCacheBugWorkaround();
	if (!NT_SUCCESS(status))
		return status;
	return __super::OnStartDevice(AllocatedResources, AllocatedResourcesTranslated);
}

NTSTATUS BasicStorageVolume::EnableWindowsCacheBugWorkaround(size_t IntermediateBufferSize)
{
	if (!GetSectorSize() || !IntermediateBufferSize || (IntermediateBufferSize % GetSectorSize()))
		return STATUS_INVALID_DEVICE_STATE;
	if (m_pWinCacheBugWorkaroundBuffer && (m_WinCacheBugWorkaroundBufferSize == IntermediateBufferSize))
		return STATUS_SUCCESS;

	if (m_pWinCacheBugWorkaroundBuffer)
		return STATUS_ALREADY_REGISTERED;
	m_pWinCacheBugWorkaroundBuffer = bulk_malloc(IntermediateBufferSize);
	if (!m_pWinCacheBugWorkaroundBuffer)
		return STATUS_NO_MEMORY;
	m_WinCacheBugWorkaroundBufferSize = IntermediateBufferSize;
	return STATUS_SUCCESS;
}

BasicStorageVolume::~BasicStorageVolume()
{
	if (m_pWinCacheBugWorkaroundBuffer)
		bulk_free(m_pWinCacheBugWorkaroundBuffer, m_WinCacheBugWorkaroundBufferSize);
}

bool BasicStorageVolume::UpdateVolumeSize()
{
	m_SectorSize = GetSectorSize();
	m_SizeInBytes = GetTotalSize();
	if (!m_SectorSize || !m_SizeInBytes)
		return false;
	if (m_SizeInBytes % m_SectorSize)
		return false;
	m_SectorCount = m_SizeInBytes / m_SectorSize;
	if (!m_SectorCount)
		return false;
	return true;
}

NTSTATUS BasicStorageVolume::AddDevice(Driver *pDriver, PDEVICE_OBJECT PhysicalDeviceObject)
{
	return __super::AddDevice(pDriver, PhysicalDeviceObject, &GUID_DEVINTERFACE_VOLUME);
}

using namespace BazisLib;
#include "../../bzscore/file.h"

NTSTATUS BasicStorageVolume::OnRead(PMDL pMdl, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone)
{
	if (!pMdl)
		return STATUS_ACCESS_VIOLATION;
	if (GetCurrentPNPState() != Started)
		return STATUS_INVALID_DEVICE_STATE;
	LONGLONG maxLen = m_SizeInBytes - ByteOffset;
	if (maxLen < 0)
		maxLen = 0;
	if (Length > maxLen)
		Length = (ULONG)maxLen;
	void *pBuffer = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);
	if (!pBuffer)
		return STATUS_NO_MEMORY;
	if (!m_pWinCacheBugWorkaroundBuffer)
		return Read(ByteOffset, pBuffer, Length, pBytesDone);

	ULONG_PTR done = 0;
	ULONG_PTR cdone = 0;
	while (done < Length)
	{
		ULONG todo = (ULONG)(Length - done);
		if (todo > m_WinCacheBugWorkaroundBufferSize)
			todo = (ULONG)m_WinCacheBugWorkaroundBufferSize;
		NTSTATUS status = Read(ByteOffset + done, m_pWinCacheBugWorkaroundBuffer, todo, &cdone);
		if (pBytesDone)
			*pBytesDone += cdone;
		memcpy(((char *)pBuffer) + done, m_pWinCacheBugWorkaroundBuffer, cdone);
		done += cdone;
		if (!NT_SUCCESS(status))
			return status;
	}
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnWrite(PMDL pMdl, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone)
{
	if (GetCurrentPNPState() != Started)
		return STATUS_INVALID_DEVICE_STATE;
	if (!pMdl)
		return STATUS_ACCESS_VIOLATION;
	LONGLONG maxLen = m_SizeInBytes - ByteOffset;
	if (maxLen < 0)
		maxLen = 0;
	if (Length > maxLen)
		Length = (ULONG)maxLen;
	void *pBuffer = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);
	if (!pBuffer)
		return STATUS_NO_MEMORY;
	return Write(ByteOffset, pBuffer, Length, pBytesDone);
}

NTSTATUS BasicStorageVolume::OnCreate(PIO_SECURITY_CONTEXT SecurityContext, ULONG Options, USHORT FileAttributes, USHORT ShareAccess, PFILE_OBJECT pFileObj)
{
	if (GetCurrentPNPState() != Started)
		return STATUS_INVALID_DEVICE_STATE;
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnClose()
{
	return STATUS_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------

NTSTATUS BasicStorageVolume::OnCheckVerify(ULONG OriginalControlCode, ULONG *pChangeCount)
{
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnVolumeOnline()
{
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnVolumeOffline()
{
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnGetVolumeDiskExtents( PVOLUME_DISK_EXTENTS pExtents, ULONG BufferLength, PULONG_PTR pBytesDone )
{
	if (!pExtents || (BufferLength < (sizeof(VOLUME_DISK_EXTENTS))))
		return STATUS_BUFFER_TOO_SMALL;
	pExtents->NumberOfDiskExtents = 1;
	pExtents->Extents[0].DiskNumber = 2;
	pExtents->Extents[0].StartingOffset.QuadPart = 0;
	pExtents->Extents[0].ExtentLength.QuadPart = m_SizeInBytes;
	*pBytesDone = sizeof(VOLUME_DISK_EXTENTS);
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnGetPartitionInfo(PPARTITION_INFORMATION pInfo, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	if (BufferLength < sizeof(PARTITION_INFORMATION))
		return STATUS_BUFFER_TOO_SMALL;
	pInfo->StartingOffset.QuadPart= 0LL;
	pInfo->PartitionLength.QuadPart = m_SizeInBytes;
	pInfo->HiddenSectors = 0;
	pInfo->PartitionNumber = 1;
	pInfo->PartitionType = GetPartitionType();
	pInfo->BootIndicator = FALSE;
	pInfo->RecognizedPartition = TRUE;
	pInfo->RewritePartition = FALSE;
	*pBytesDone = sizeof(PARTITION_INFORMATION);
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnGetPartitionInfoEx(PPARTITION_INFORMATION_EX pInfo, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	if (BufferLength < sizeof(PARTITION_INFORMATION_EX))
		return STATUS_BUFFER_TOO_SMALL;
	pInfo->PartitionStyle = PARTITION_STYLE_MBR;
	pInfo->StartingOffset.QuadPart= 0LL;
	pInfo->PartitionLength.QuadPart = m_SizeInBytes;
	pInfo->PartitionNumber = 1;
	pInfo->RewritePartition = FALSE;

	pInfo->Mbr.PartitionType = GetPartitionType();
	pInfo->Mbr.BootIndicator = FALSE;
	pInfo->Mbr.RecognizedPartition = TRUE;
	pInfo->Mbr.HiddenSectors = 0;
	*pBytesDone = sizeof(PARTITION_INFORMATION_EX);
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnGetDeviceNumber(PSTORAGE_DEVICE_NUMBER pNumber)
{
	pNumber->DeviceType = m_ReportedDeviceType;
	pNumber->DeviceNumber = -1;
	pNumber->PartitionNumber = -1;
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnGetDriveGeometry(PDISK_GEOMETRY pGeo)
{
	const int SectorsPerTrack = 64;
	if (m_SectorCount % 64)
	{
		pGeo->Cylinders.QuadPart = m_SectorCount;
		pGeo->SectorsPerTrack = 1;
	}
	else
	{
		pGeo->Cylinders.QuadPart = m_SectorCount / SectorsPerTrack;
		pGeo->SectorsPerTrack = SectorsPerTrack;
	}
	pGeo->TracksPerCylinder = 1;
	pGeo->BytesPerSector = m_SectorSize;
	pGeo->MediaType = FixedMedia;
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnGetLengthInfo(PGET_LENGTH_INFORMATION pLength)
{
	pLength->Length.QuadPart = m_SizeInBytes;
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnDiskIsWritable()
{
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnMediaRemoval(bool bLock)
{
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnGetHotplugInfo(PSTORAGE_HOTPLUG_INFO pHotplugInfo)
{
	pHotplugInfo->Size = sizeof(STORAGE_HOTPLUG_INFO);
	pHotplugInfo->MediaRemovable = TRUE;
	pHotplugInfo->MediaHotplug = FALSE;
	pHotplugInfo->DeviceHotplug = FALSE;
	pHotplugInfo->WriteCacheEnableOverride = NULL;
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnSetPartitionInfo(PPARTITION_INFORMATION pInfo, ULONG InputLength)
{
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnSetPartitionInfoEx(PPARTITION_INFORMATION_EX pInfo, ULONG InputLength)
{
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::OnDiskVerify(PVERIFY_INFORMATION pVerifyInfo)
{
	return STATUS_SUCCESS;
}

NTSTATUS BasicStorageVolume::DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp)
{
	switch (IrpSp->MajorFunction)
	{
		//WARNING! OnRead()/OnWrite() use a common buffer for Windows Cache bug workaround.
		//If the IRP_MJ_READ/IRP_MJ_WRITE handling is changed from queued to concurrent,
		//the buffers should be updated
	case IRP_MJ_PNP:
		if (IrpSp->MinorFunction != IRP_MN_START_DEVICE)
			break;
	case IRP_MJ_READ:
	case IRP_MJ_WRITE:
		if (!Irp->IsFromQueue())
			return EnqueuePacket(Irp);
		break;
	}
	return __super::DispatchRoutine(Irp, IrpSp);
}
#endif