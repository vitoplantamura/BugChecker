#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "storfilter.h"

using namespace BazisLib::DDK;

StorageFilter::StorageFilter(LPCWSTR pwszBaseDeviceName,
       bool bDeleteThisAfterRemoveRequest,
	   LPCWSTR pwszFilterDeviceName,
	   DEVICE_TYPE FilterDeviceType,
	   ULONG DeviceCharacteristics,
	   bool bExclusive,
	   ULONG AdditionalDeviceFlags) :
	IODeviceFilter(pwszBaseDeviceName, bDeleteThisAfterRemoveRequest, pwszFilterDeviceName, FilterDeviceType, DeviceCharacteristics, bExclusive, AdditionalDeviceFlags)
{
}

StorageFilter::~StorageFilter()
{
}

//Note that filter driver does not fail the request if the buffer size is too small. This macro name
//is left here to maintain the same structure as the StorageDevice class does.
#define ASSURE_SIZE_AND_CALL(method, type) return method(pIrp, (type *)pInOutBuffer);

NTSTATUS StorageFilter::FilterDeviceControl(IncomingIrp *pIrp, ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone)
{
	switch (ControlCode)
	{
	case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
		return FilterQueryDeviceName(pIrp, (PMOUNTDEV_NAME)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
		return FilterQueryUniqueID(pIrp, (PMOUNTDEV_UNIQUE_ID)pInOutBuffer, OutputLength, pBytesDone);

	case IOCTL_VOLUME_ONLINE:
		return FilterVolumeOnline(pIrp);
	case IOCTL_VOLUME_OFFLINE:
		return FilterVolumeOffline(pIrp);

	case IOCTL_DISK_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY2:
		return FilterCheckVerify(pIrp, ControlCode);
	case IOCTL_DISK_GET_PARTITION_INFO:
		return FilterGetPartitionInfo(pIrp, (PPARTITION_INFORMATION)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_DISK_GET_PARTITION_INFO_EX:
		return FilterGetPartitionInfoEx(pIrp, (PPARTITION_INFORMATION_EX)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_DISK_SET_PARTITION_INFO:
		return FilterSetPartitionInfo(pIrp, (PPARTITION_INFORMATION)pInOutBuffer, InputLength);
	case IOCTL_DISK_SET_PARTITION_INFO_EX:
		return FilterSetPartitionInfoEx(pIrp, (PPARTITION_INFORMATION_EX)pInOutBuffer, InputLength);
	case IOCTL_DISK_IS_WRITABLE:
		return FilterDiskIsWritable(pIrp);
	case IOCTL_DISK_GET_DRIVE_GEOMETRY:
		ASSURE_SIZE_AND_CALL(FilterGetDriveGeometry, DISK_GEOMETRY);
	case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
		ASSURE_SIZE_AND_CALL(FilterGetDriveGeometryEx, DISK_GEOMETRY_EX)
	case IOCTL_DISK_GET_LENGTH_INFO:
		ASSURE_SIZE_AND_CALL(FilterGetLengthInfo, GET_LENGTH_INFORMATION);
	case IOCTL_DISK_GET_DRIVE_LAYOUT:
		return FilterGetDriveLayout(pIrp, (PDRIVE_LAYOUT_INFORMATION)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
		return FilterGetDriveLayoutEx(pIrp, (PDRIVE_LAYOUT_INFORMATION_EX)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_STORAGE_GET_HOTPLUG_INFO:
		ASSURE_SIZE_AND_CALL(FilterGetHotplugInfo, STORAGE_HOTPLUG_INFO);
	case IOCTL_DISK_MEDIA_REMOVAL:
		return FilterMediaRemoval(pIrp, ((BOOLEAN *)pInOutBuffer));
	case IOCTL_DISK_GET_MEDIA_TYPES:
	case IOCTL_STORAGE_GET_MEDIA_TYPES:
		return FilterGetMediaTypes(pIrp, (PDISK_GEOMETRY)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_DISK_VERIFY:
		return FilterDiskVerify(pIrp, (PVERIFY_INFORMATION)pInOutBuffer);
	case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:
		return FilterGetMediaTypesEx(pIrp, (PGET_MEDIA_TYPES)pInOutBuffer, (PDEVICE_MEDIA_INFO)((char *)pInOutBuffer + sizeof(GET_MEDIA_TYPES)), OutputLength, pBytesDone);
	case IOCTL_STORAGE_GET_DEVICE_NUMBER:
		ASSURE_SIZE_AND_CALL(FilterGetDeviceNumber, STORAGE_DEVICE_NUMBER);
	case IOCTL_STORAGE_QUERY_PROPERTY:
		return FilterQueryProperty(pIrp, (PSTORAGE_PROPERTY_QUERY)pInOutBuffer, (PSTORAGE_DESCRIPTOR_HEADER)pInOutBuffer, OutputLength, pBytesDone);
	default:
		return __super::FilterDeviceControl(pIrp, ControlCode, IsInternal, pInOutBuffer, InputLength, OutputLength, pBytesDone);
	}
}

#undef ASSURE_SIZE_AND_CALL

NTSTATUS StorageFilter::FilterQueryDeviceName(IncomingIrp *pIrp, PMOUNTDEV_NAME pName, ULONG BufferLength, PULONG_PTR pBytesDone){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterQueryUniqueID(IncomingIrp *pIrp, PMOUNTDEV_UNIQUE_ID pID, ULONG BufferLength, PULONG_PTR pBytesDone){return STATUS_NOT_SUPPORTED;}

NTSTATUS StorageFilter::FilterVolumeOnline(IncomingIrp *pIrp){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterVolumeOffline(IncomingIrp *pIrp){return STATUS_NOT_SUPPORTED;}

NTSTATUS StorageFilter::FilterCheckVerify(IncomingIrp *pIrp, ULONG OriginalControlCode){return STATUS_NOT_SUPPORTED;}

NTSTATUS StorageFilter::FilterGetPartitionInfo(IncomingIrp *pIrp, PPARTITION_INFORMATION pInfo, ULONG BufferLength, PULONG_PTR pBytesDone){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterGetPartitionInfoEx(IncomingIrp *pIrp, PPARTITION_INFORMATION_EX pInfo, ULONG BufferLength, PULONG_PTR pBytesDone){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterSetPartitionInfo(IncomingIrp *pIrp, PPARTITION_INFORMATION pInfo, ULONG InputLength){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterSetPartitionInfoEx(IncomingIrp *pIrp, PPARTITION_INFORMATION_EX pInfo, ULONG InputLength){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterDiskIsWritable(IncomingIrp *pIrp){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterGetDriveGeometry(IncomingIrp *pIrp, PDISK_GEOMETRY pGeo){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterGetDriveGeometryEx(IncomingIrp *pIrp, PDISK_GEOMETRY_EX pGeo){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterGetLengthInfo(IncomingIrp *pIrp, PGET_LENGTH_INFORMATION pLength){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterGetDriveLayout(IncomingIrp *pIrp, PDRIVE_LAYOUT_INFORMATION pLayout, ULONG BufferLength, PULONG_PTR pBytesDone){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterGetDriveLayoutEx(IncomingIrp *pIrp, PDRIVE_LAYOUT_INFORMATION_EX pLayout, ULONG BufferLength, PULONG_PTR pBytesDone){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterGetHotplugInfo(IncomingIrp *pIrp, PSTORAGE_HOTPLUG_INFO pHotplugInfo){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterMediaRemoval(IncomingIrp *pIrp, BOOLEAN *pbLock){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterGetMediaTypes(IncomingIrp *pIrp, PDISK_GEOMETRY pGeometryArray, ULONG BufferLength, PULONG_PTR pBytesDone){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterDiskVerify(IncomingIrp *pIrp, PVERIFY_INFORMATION pVerifyInfo){return STATUS_NOT_SUPPORTED;}

NTSTATUS StorageFilter::FilterGetMediaTypesEx(IncomingIrp *pIrp, PGET_MEDIA_TYPES pMediaTypes, PDEVICE_MEDIA_INFO pFirstMediaInfo, ULONG BufferLength, PULONG_PTR pBytesDone){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterGetDeviceNumber(IncomingIrp *pIrp, PSTORAGE_DEVICE_NUMBER pNumber){return STATUS_NOT_SUPPORTED;}
NTSTATUS StorageFilter::FilterQueryProperty(IncomingIrp *pIrp, PSTORAGE_PROPERTY_QUERY pQueryProperty, PSTORAGE_DESCRIPTOR_HEADER pOutput, ULONG BufferLength, PULONG_PTR pBytesDone){return STATUS_NOT_SUPPORTED;}			
#endif