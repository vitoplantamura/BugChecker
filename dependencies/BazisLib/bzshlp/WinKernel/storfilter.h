#pragma once

#include "iofilter.h"
#include "storage.h"

namespace BazisLib
{
	namespace DDK
	{
		class StorageFilter : public IODeviceFilter
		{
		public:
			/*! Initializes a storage filter class instance
				\param bDeleteThisAfterRemoveRequest Specifies whether the device object (class instance) should
													 be deleted automatically after handling IRP_MN_REMOVE_DEVICE.
			*/
			StorageFilter(LPCWSTR pwszBaseDeviceName,
			       bool bDeleteThisAfterRemoveRequest = false,
				   LPCWSTR pwszFilterDeviceName = NULL,
				   DEVICE_TYPE FilterDeviceType = FILE_DEVICE_UNKNOWN,
				   ULONG DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN,
				   bool bExclusive = FALSE,
				   ULONG AdditionalDeviceFlags = DO_POWER_PAGABLE);

			~StorageFilter();

		protected:
			virtual NTSTATUS FilterDeviceControl(IncomingIrp *pIrp, ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone)  override;

		protected:
			//The following handlers act similar to PnP minihandlers. Return STATUS_NOT_SUPPORTED if you want
			//to forward the IRP to the next driver. Simply call CallNextDriverSynchronously(pIrp) to get
			//the filtered driver called. Return any other status to get the IRP completed without calling filtered
			//driver.
			//WARNING! DO NOT COMPLETE THE IRP INSIDE A FILTER. RETURN A CORRESPONDING STATUS INSTEAD.
			virtual NTSTATUS FilterQueryDeviceName(IncomingIrp *pIrp, PMOUNTDEV_NAME pName, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS FilterQueryUniqueID(IncomingIrp *pIrp, PMOUNTDEV_UNIQUE_ID pID, ULONG BufferLength, PULONG_PTR pBytesDone);

			virtual NTSTATUS FilterVolumeOnline(IncomingIrp *pIrp);
			virtual NTSTATUS FilterVolumeOffline(IncomingIrp *pIrp);

			virtual NTSTATUS FilterCheckVerify(IncomingIrp *pIrp, ULONG OriginalControlCode);

			virtual NTSTATUS FilterGetPartitionInfo(IncomingIrp *pIrp, PPARTITION_INFORMATION pInfo, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS FilterGetPartitionInfoEx(IncomingIrp *pIrp, PPARTITION_INFORMATION_EX pInfo, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS FilterSetPartitionInfo(IncomingIrp *pIrp, PPARTITION_INFORMATION pInfo, ULONG InputLength);
			virtual NTSTATUS FilterSetPartitionInfoEx(IncomingIrp *pIrp, PPARTITION_INFORMATION_EX pInfo, ULONG InputLength);
			virtual NTSTATUS FilterDiskIsWritable(IncomingIrp *pIrp);
			virtual NTSTATUS FilterGetDriveGeometry(IncomingIrp *pIrp, PDISK_GEOMETRY pGeo);
			virtual NTSTATUS FilterGetDriveGeometryEx(IncomingIrp *pIrp, PDISK_GEOMETRY_EX pGeo);
			virtual NTSTATUS FilterGetLengthInfo(IncomingIrp *pIrp, PGET_LENGTH_INFORMATION pLength);
			virtual NTSTATUS FilterGetDriveLayout(IncomingIrp *pIrp, PDRIVE_LAYOUT_INFORMATION pLayout, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS FilterGetDriveLayoutEx(IncomingIrp *pIrp, PDRIVE_LAYOUT_INFORMATION_EX pLayout, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS FilterGetHotplugInfo(IncomingIrp *pIrp, PSTORAGE_HOTPLUG_INFO pHotplugInfo);
			virtual NTSTATUS FilterMediaRemoval(IncomingIrp *pIrp, BOOLEAN *pbLock);
			virtual NTSTATUS FilterGetMediaTypes(IncomingIrp *pIrp, PDISK_GEOMETRY pGeometryArray, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS FilterDiskVerify(IncomingIrp *pIrp, PVERIFY_INFORMATION pVerifyInfo);
			
			virtual NTSTATUS FilterGetMediaTypesEx(IncomingIrp *pIrp, PGET_MEDIA_TYPES pMediaTypes, PDEVICE_MEDIA_INFO pFirstMediaInfo, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS FilterGetDeviceNumber(IncomingIrp *pIrp, PSTORAGE_DEVICE_NUMBER pNumber);
			virtual NTSTATUS FilterQueryProperty(IncomingIrp *pIrp, PSTORAGE_PROPERTY_QUERY pQueryProperty, PSTORAGE_DESCRIPTOR_HEADER pOutput, ULONG BufferLength, PULONG_PTR pBytesDone);			
		};
	}
}
