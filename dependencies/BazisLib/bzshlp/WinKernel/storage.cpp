#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "storage.h"

#include "../../bzscore/file.h"
#include "ioctldbg.h"

#include <ntddcdrm.h>

const unsigned MaximumTransferLength = 64 * 1024 * 1024;

using namespace BazisLib::DDK;

#define TRACK_UNHANDLED_STORAGE_REQUESTS

#ifdef TRACK_UNHANDLED_STORAGE_REQUESTS
#define TRACK_STORAGE_REQUEST(string) DbgPrint(string)
#else
#define TRACK_STORAGE_REQUEST(string)
#endif

StorageDevice::StorageDevice(LPCWSTR pwszDevicePrefix,
				  bool bDeleteThisAfterRemoveRequest,
				  DEVICE_TYPE DeviceType,
				  ULONG DeviceCharacteristics,
				  bool bExclusive,
				  ULONG AdditionalDeviceFlags) :
	IODevice(DeviceType, bDeleteThisAfterRemoveRequest, DeviceCharacteristics, bExclusive, AdditionalDeviceFlags)
{
	wchar_t wszFullName[512] = L"\\Device\\";
	if (!pwszDevicePrefix)
	{
		ReportInitializationError(STATUS_INVALID_PARAMETER);
		return;
	}
	wcsncat(wszFullName, pwszDevicePrefix, (sizeof(wszFullName)/sizeof(wszFullName[0])) - 1);
	size_t length = wcslen(wszFullName);
	if (((sizeof(wszFullName)/sizeof(wszFullName[0])) - length) < 12)
	{
		ReportInitializationError(STATUS_INVALID_PARAMETER);
		return;
	}
	for (unsigned num = 0; ; num++)
	{
		swprintf(&wszFullName[length], L"%d", num);
		bool bFound = false;
		switch (File::GetOpenFileStatus(wszFullName))
		{
		case STATUS_OBJECT_NAME_NOT_FOUND:
			bFound = true;
			break;
		case STATUS_SUCCESS:
			break;
		default:
			if (num > 30)
			{
				ReportInitializationError(STATUS_OBJECT_NAME_INVALID);
				bFound = true;
			}
		}
		if (bFound)
			break;
	}

	m_FullDeviceName = wszFullName;
	if (!SetShortDeviceName(m_FullDeviceName.c_str() + (sizeof(L"\\Device\\")/sizeof(wchar_t)) - 1))
	{
		ReportInitializationError(STATUS_INTERNAL_ERROR);
		return;
	}
}

StorageDevice::~StorageDevice()
{
}

NTSTATUS StorageDevice::AddDevice(Driver *pDriver, PDEVICE_OBJECT PhysicalDeviceObject, const GUID *pInterfaceGuid, const wchar_t *pwszLinkPath)
{
	if (!pDriver || !PhysicalDeviceObject || !pInterfaceGuid)
		return STATUS_INVALID_PARAMETER;
	return __super::AddDevice(pDriver, PhysicalDeviceObject, pInterfaceGuid, pwszLinkPath);
}

#define ASSURE_SIZE_AND_CALL(method, type) \
		{\
		if (OutputLength < sizeof(type))\
			return STATUS_BUFFER_TOO_SMALL;\
		NTSTATUS status = method((type *)pInOutBuffer);\
		if (NT_SUCCESS(status))\
			*pBytesDone = sizeof(type);\
		return status;\
		}

NTSTATUS StorageDevice::OnDeviceControl(ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone)
{
	switch (ControlCode)
	{
	case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
		return OnQueryDeviceName((PMOUNTDEV_NAME)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
		return OnQueryUniqueID((PMOUNTDEV_UNIQUE_ID)pInOutBuffer, OutputLength, pBytesDone);

	case IOCTL_VOLUME_ONLINE:
		return OnVolumeOnline();
	case IOCTL_VOLUME_OFFLINE:
		return OnVolumeOffline();
	case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
		return OnGetVolumeDiskExtents((PVOLUME_DISK_EXTENTS)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_DISK_CHECK_VERIFY:
	case IOCTL_CDROM_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY2:
		{
			ULONG *pChangeCount = NULL;
			if ((OutputLength >= sizeof(ULONG)) && pInOutBuffer)
				pChangeCount = (ULONG *)pInOutBuffer;
			if (pChangeCount)
				*pChangeCount = 0;
			NTSTATUS st = OnCheckVerify(ControlCode, pChangeCount);
			if (NT_SUCCESS(st))
				*pBytesDone = sizeof(ULONG);
			return st;
		}
	case IOCTL_DISK_GET_PARTITION_INFO:
		return OnGetPartitionInfo((PPARTITION_INFORMATION)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_DISK_GET_PARTITION_INFO_EX:
		return OnGetPartitionInfoEx((PPARTITION_INFORMATION_EX)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_DISK_SET_PARTITION_INFO:
		return OnSetPartitionInfo((PPARTITION_INFORMATION)pInOutBuffer, InputLength);
	case IOCTL_DISK_SET_PARTITION_INFO_EX:
		return OnSetPartitionInfoEx((PPARTITION_INFORMATION_EX)pInOutBuffer, InputLength);
	case IOCTL_DISK_IS_WRITABLE:
		return OnDiskIsWritable();
	case IOCTL_DISK_GET_DRIVE_GEOMETRY:
		ASSURE_SIZE_AND_CALL(OnGetDriveGeometry, DISK_GEOMETRY);
	case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
		ASSURE_SIZE_AND_CALL(OnGetDriveGeometryEx, DISK_GEOMETRY_EX)
	case IOCTL_DISK_GET_LENGTH_INFO:
		ASSURE_SIZE_AND_CALL(OnGetLengthInfo, GET_LENGTH_INFORMATION);
	case IOCTL_DISK_GET_DRIVE_LAYOUT:
		return OnGetDriveLayout((PDRIVE_LAYOUT_INFORMATION)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
		return OnGetDriveLayoutEx((PDRIVE_LAYOUT_INFORMATION_EX)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_STORAGE_GET_HOTPLUG_INFO:
		ASSURE_SIZE_AND_CALL(OnGetHotplugInfo, STORAGE_HOTPLUG_INFO);
	case IOCTL_DISK_MEDIA_REMOVAL:
		return OnMediaRemoval(*((BOOLEAN *)pInOutBuffer) != FALSE);
	case IOCTL_DISK_GET_MEDIA_TYPES:
	case IOCTL_STORAGE_GET_MEDIA_TYPES:
		return OnGetMediaTypes((PDISK_GEOMETRY)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_DISK_VERIFY:
		return OnDiskVerify((PVERIFY_INFORMATION)pInOutBuffer);
	case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:
		return OnGetMediaTypesEx((PGET_MEDIA_TYPES)pInOutBuffer, (PDEVICE_MEDIA_INFO)((char *)pInOutBuffer + sizeof(GET_MEDIA_TYPES)), OutputLength, pBytesDone);
	case IOCTL_STORAGE_GET_DEVICE_NUMBER:
		ASSURE_SIZE_AND_CALL(OnGetDeviceNumber, STORAGE_DEVICE_NUMBER);
	case IOCTL_STORAGE_QUERY_PROPERTY:
		return OnQueryProperty((PSTORAGE_PROPERTY_QUERY)pInOutBuffer, InputLength, (PSTORAGE_DESCRIPTOR_HEADER)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
		ASSURE_SIZE_AND_CALL(OnGetStableGuid, GUID);
	case IOCTL_SCSI_MINIPORT:
		ASSURE_SIZE_AND_CALL(OnScsiMiniportControl, SRB_IO_CONTROL);
	default:
/*		{
			BazisLib::File f(L"\\Device\\HarddiskVolume1", FileFlags::ReadAccess, FileFlags::OpenExisting, FileFlags::ShareAll);
			NTSTATUS st = f.DeviceIoControl(ControlCode, pInOutBuffer, InputLength, pInOutBuffer, OutputLength, pBytesDone);
			//if (NT_SUCCESS(st))
				return st;
		}*/
		DUMP_DEVIOCTL("Unhandled control code: %s\n", ControlCode);
		return __super::OnDeviceControl(ControlCode, IsInternal, pInOutBuffer, InputLength, OutputLength, pBytesDone);
	}
}

#undef ASSURE_SIZE_AND_CALL

NTSTATUS StorageDevice::OnQueryDeviceName(PMOUNTDEV_NAME pName, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	PCUNICODE_STRING pFullDeviceName = GetFullDeviceName();
	if (!pFullDeviceName || !pFullDeviceName->Buffer)
		return STATUS_INVALID_DEVICE_REQUEST;
	pName->NameLength = pFullDeviceName->Length;
	unsigned totalSize = pName->NameLength + sizeof(USHORT);
	if (BufferLength < totalSize)
	{
		*pBytesDone = sizeof(MOUNTDEV_NAME);
		return STATUS_BUFFER_OVERFLOW;
	}
	memcpy(pName->Name, pFullDeviceName->Buffer, pName->NameLength);
	*pBytesDone = totalSize;
	return STATUS_SUCCESS;
}

NTSTATUS StorageDevice::OnQueryUniqueID(PMOUNTDEV_UNIQUE_ID pID, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	PCUNICODE_STRING pUniqueID = GetInterfaceName();
	if (!pUniqueID || !pUniqueID->Buffer)
		return STATUS_INVALID_DEVICE_REQUEST;
	pID->UniqueIdLength = pUniqueID->Length;
	unsigned totalSize = pID->UniqueIdLength + sizeof(USHORT);
	if (BufferLength < totalSize)
	{
		*pBytesDone = sizeof(MOUNTDEV_UNIQUE_ID);
		return STATUS_BUFFER_OVERFLOW;
	}
	memcpy(pID->UniqueId, pUniqueID->Buffer, pID->UniqueIdLength);
	*pBytesDone = totalSize;
	return STATUS_SUCCESS;
}


NTSTATUS StorageDevice::OnVolumeOnline()
{
	TRACK_STORAGE_REQUEST("Unhandled OnVolumeOnline()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnVolumeOffline()
{
	TRACK_STORAGE_REQUEST("Unhandled OnVolumeOffline()\n");
	return STATUS_NOT_SUPPORTED;
}


NTSTATUS StorageDevice::OnCheckVerify(ULONG OriginalControlCode, ULONG *pChangeCount)
{
	TRACK_STORAGE_REQUEST("Unhandled OnCheckVerify()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetVolumeDiskExtents(PVOLUME_DISK_EXTENTS pExtents, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetVolumeDiskExtents()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetPartitionInfo(PPARTITION_INFORMATION pInfo, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetPartitionInfo()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetPartitionInfoEx(PPARTITION_INFORMATION_EX pInfo, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetPartitionInfoEx()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnSetPartitionInfo(PPARTITION_INFORMATION pInfo, ULONG InputLength)
{
	TRACK_STORAGE_REQUEST("Unhandled OnSetPartitionInfo()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnSetPartitionInfoEx(PPARTITION_INFORMATION_EX pInfo, ULONG InputLength)
{
	TRACK_STORAGE_REQUEST("Unhandled OnSetPartitionInfoEx()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnDiskIsWritable()
{
	TRACK_STORAGE_REQUEST("Unhandled OnDiskIsWritable()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetDriveGeometry(PDISK_GEOMETRY pGeo)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetDriveGeometry()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetDriveGeometryEx(PDISK_GEOMETRY_EX pGeo)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetDriveGeomtryEx()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetLengthInfo(PGET_LENGTH_INFORMATION pLength)
{
	TRACK_STORAGE_REQUEST("Unhandled OnLengthInfo()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetDriveLayout(PDRIVE_LAYOUT_INFORMATION pLayout, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetDriveLayout()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetDriveLayoutEx(PDRIVE_LAYOUT_INFORMATION_EX pLayout, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetDriveLayoutEx()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetHotplugInfo(PSTORAGE_HOTPLUG_INFO pHotplugInfo)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetHotplugInfo()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnMediaRemoval(bool bLock)
{
	TRACK_STORAGE_REQUEST("Unhandled OnMediaRemoval()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetMediaTypes(PDISK_GEOMETRY pGeometryArray, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetMediaTypes()\n");
	return STATUS_NOT_SUPPORTED;
}


NTSTATUS StorageDevice::OnGetMediaTypesEx(PGET_MEDIA_TYPES pMediaTypes, PDEVICE_MEDIA_INFO pFirstMediaInfo, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetMediaTypesEx()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnDiskVerify(PVERIFY_INFORMATION pVerifyInfo)
{
	TRACK_STORAGE_REQUEST("Unhandled OnDiskVerify()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetDeviceNumber(PSTORAGE_DEVICE_NUMBER pNumber)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetDeviceNumber()\n");
	return STATUS_NOT_SUPPORTED;
}

struct StorageDescriptorWrapper
{
	STORAGE_DEVICE_DESCRIPTOR Descriptor;
#pragma warning (suppress : 4200)
	char Data[];
};

NTSTATUS StorageDevice::OnQueryProperty(PSTORAGE_PROPERTY_QUERY pQueryProperty, ULONG InputBufferLength, PSTORAGE_DESCRIPTOR_HEADER pOutput, ULONG BufferLength, PULONG_PTR pBytesDone)
{
	if ((BufferLength >= 8) && (pQueryProperty->QueryType == PropertyStandardQuery) && (pQueryProperty->PropertyId == StorageDeviceProperty))
	{
		*pBytesDone = BuildStorageDeviceDescriptor(pOutput, BufferLength, NULL, ConstStringA("BazisLib Virtual Storage Device"));
		return STATUS_SUCCESS;
	}
	else if ((BufferLength >= 8) && (pQueryProperty->QueryType == PropertyStandardQuery) && (pQueryProperty->PropertyId == StorageAdapterProperty))
	{
		STORAGE_ADAPTER_DESCRIPTOR desc = {0,};

		//DbgPrint("OnQueryProperty() - adapter descriptor [Size = %d]\n",BufferLength);
		desc.Size = desc.Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
		desc.MaximumTransferLength = MaximumTransferLength;
		desc.MaximumPhysicalPages = MaximumTransferLength / PAGE_SIZE;
		desc.BusType = BusTypeVirtual;	//If you get an error here, replace BusTypeVirtual with 14

		memcpy(pOutput, &desc, min(BufferLength, sizeof(desc)));
		*pBytesDone = min(BufferLength, sizeof(desc));
		return STATUS_SUCCESS;
	}
//	DbgPrint("OnQueryProperty(): ID: %d, type: %d, inSize = %d\n", pQueryProperty->PropertyId, pQueryProperty->QueryType, BufferLength);
	TRACK_STORAGE_REQUEST("Unhandled OnQueryProperty()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnGetStableGuid(LPGUID lpGuid)
{
	TRACK_STORAGE_REQUEST("Unhandled OnGetStableGuid()\n");
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS StorageDevice::OnScsiMiniportControl(SRB_IO_CONTROL *pControlBlock)
{
#ifdef TRACK_UNHANDLED_STORAGE_REQUESTS
	DbgPrint("Unhandled OnScsiMiniportControl(hdrl=%d, ctlcode=%d)\n", pControlBlock->HeaderLength, pControlBlock->ControlCode);
#endif
	pControlBlock->ReturnCode = SRB_STATUS_INVALID_REQUEST;
	return STATUS_INVALID_DEVICE_REQUEST;
}

static ULONG AppendStringToBuffer(void *pBuffer, size_t BufferSize, size_t *pInitialOffset, BazisLib::TempArrayReference<char> str, bool appendNullChar)
{
	if (!pBuffer)
		return 0;
	if (str.Empty())
		return 0;

	size_t additionalSize = appendNullChar ? 1 : 0;

	if ((*pInitialOffset + str.GetSizeInBytes() + additionalSize) > BufferSize)
		return 0;
	memcpy((char *)pBuffer + *pInitialOffset, str.GetData(), str.GetSizeInBytes());
	size_t off = *pInitialOffset;
	*pInitialOffset += str.GetSizeInBytes();

	if (appendNullChar)
		((char *)pBuffer)[(*pInitialOffset)++] = 0;

	return (ULONG)off;
}

static inline BazisLib::TempArrayReference<char> BufFromString(const BazisLib::TempStringA &str)
{
	return BazisLib::TempArrayReference<char>((char *)str.GetConstBuffer(), str.length());
}

size_t BazisLib::DDK::StorageDevice::BuildStorageDeviceDescriptor(void *pBuffer, size_t MaxSize, STORAGE_DEVICE_DESCRIPTOR *pReference /*= NULL*/, TempStringA ProductID /*= ConstStringA("")*/, TempStringA ProductRevision /*= ConstStringA("")*/, TempStringA SerialNumber /*= ConstStringA("")*/, TempArrayReference<char> VendorSpecific /*= ConstStringA("")*/)
{
	size_t productIdSize = ProductID.empty() ? 0 : ProductID.length() + 1;
	size_t productRevisionSize = ProductRevision.empty() ? 0 : ProductRevision.length() + 1;
	size_t serialNumberSize = SerialNumber.empty() ? 0 : SerialNumber.length() + 1;

	STORAGE_DEVICE_DESCRIPTOR desc;
	if (!pReference)
	{
		memset(&desc, 0, sizeof(desc));
		pReference = &desc;
	}

	pReference->Size = (ULONG)(sizeof(STORAGE_DEVICE_DESCRIPTOR) + productIdSize + productRevisionSize + serialNumberSize + VendorSpecific.GetSizeInBytes());
	pReference->Version = sizeof(STORAGE_DEVICE_DESCRIPTOR);
	pReference->BusType = BusTypeVirtual;	//If you get an error here, replace BusTypeVirtual with 14
	pReference->RawPropertiesLength = (ULONG)(VendorSpecific.GetSizeInBytes());

	if (MaxSize <= sizeof(STORAGE_DEVICE_DESCRIPTOR))
	{
		memcpy(pBuffer, pReference, MaxSize);
		return MaxSize;
	}

	size_t off = sizeof(STORAGE_DEVICE_DESCRIPTOR);
	AppendStringToBuffer(pBuffer, MaxSize, &off, VendorSpecific, false);
	ASSERT(off == (sizeof(STORAGE_DEVICE_DESCRIPTOR) + VendorSpecific.GetSizeInBytes()));
	
	pReference->ProductIdOffset = AppendStringToBuffer(pBuffer, MaxSize, &off, BufFromString(ProductID), true);
	pReference->ProductRevisionOffset = AppendStringToBuffer(pBuffer, MaxSize, &off, BufFromString(ProductRevision), true);
	pReference->SerialNumberOffset = AppendStringToBuffer(pBuffer, MaxSize, &off, BufFromString(SerialNumber), true);

	memcpy(pBuffer, pReference, sizeof(STORAGE_DEVICE_DESCRIPTOR));
	ASSERT(off == pReference->Size);
	return off;
}
#endif