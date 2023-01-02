#pragma once

#include "cmndef.h"
#include "iodevice.h"
#include "string.h"
#include <mountdev.h>
#include <ntddvol.h>
#include <ntdddisk.h>
#include <ntddstor.h>
#include <ntddscsi.h>
#include <srb.h>
#include "../../bzscore/buffer.h"

namespace BazisLib
{
	namespace DDK
	{
		class StorageDevice : protected IODevice
		{
		private:
			String m_FullDeviceName;

		public:
			StorageDevice(LPCWSTR pwszDevicePrefix = L"BazisStor",
						  bool bDeleteThisAfterRemoveRequest = false,
						  DEVICE_TYPE DeviceType = FILE_DEVICE_DISK,
						  ULONG DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN,
						  bool bExclusive = FALSE,
						  ULONG AdditionalDeviceFlags = DO_POWER_PAGABLE);
			~StorageDevice();

			bool Valid() {return __super::Valid();}
			NTSTATUS AddDevice(Driver *pDriver, PDEVICE_OBJECT PhysicalDeviceObject, const GUID *pInterfaceGuid, const wchar_t *pwszLinkPath = NULL);

		protected:
			PCUNICODE_STRING GetFullDeviceName() {return m_FullDeviceName.ToNTString();}

			/*  This method handles buffered device control codes (defined with METHOD_BUFFERED method). When writing
				an override, consider the following structure:
				switch (ControlCode)
				{
				case IOCTL_xxx:
					return OnXXX();
				case IOCTL_yyy:
					return OnYYY();
				default:
					return __super::OnDeviceControl(...);
				}
			*/
			virtual NTSTATUS OnDeviceControl(ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone) override;

		protected:
			/*  The following methods are standard storage request minihandlers. The requests that have variable return size
				contain two extra parameters for output buffer length and actual byte count put into the buffer.
				For requests with fixed return size, only the data pointer is provided. The size is automatically controlled
				by the framework. If it is less than the size of the structure, STATUS_BUFFER_OVERFLOW is returned without
				calling the minihandler.
			*/

			//The following minihandlers return device information according to the algorithm common to all storage
			//devices. You can safely leave them as they are in a subclass.
			virtual NTSTATUS OnQueryDeviceName(PMOUNTDEV_NAME pName, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS OnQueryUniqueID(PMOUNTDEV_UNIQUE_ID pID, ULONG BufferLength, PULONG_PTR pBytesDone);

			//All the rest minihandlers do nothing and immediately return STATUS_NOT_SUPPORTED. You should override
			//them in your subclass.
			virtual NTSTATUS OnVolumeOnline();
			virtual NTSTATUS OnVolumeOffline();

			virtual NTSTATUS OnCheckVerify(ULONG OriginalControlCode, ULONG *pChangeCount);

			virtual NTSTATUS OnGetVolumeDiskExtents(PVOLUME_DISK_EXTENTS pExtents, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS OnGetPartitionInfo(PPARTITION_INFORMATION pInfo, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS OnGetPartitionInfoEx(PPARTITION_INFORMATION_EX pInfo, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS OnSetPartitionInfo(PPARTITION_INFORMATION pInfo, ULONG InputLength);
			virtual NTSTATUS OnSetPartitionInfoEx(PPARTITION_INFORMATION_EX pInfo, ULONG InputLength);
			virtual NTSTATUS OnDiskIsWritable();
			virtual NTSTATUS OnGetDriveGeometry(PDISK_GEOMETRY pGeo);
			virtual NTSTATUS OnGetDriveGeometryEx(PDISK_GEOMETRY_EX pGeo);
			virtual NTSTATUS OnGetLengthInfo(PGET_LENGTH_INFORMATION pLength);
			virtual NTSTATUS OnGetDriveLayout(PDRIVE_LAYOUT_INFORMATION pLayout, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS OnGetDriveLayoutEx(PDRIVE_LAYOUT_INFORMATION_EX pLayout, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS OnGetHotplugInfo(PSTORAGE_HOTPLUG_INFO pHotplugInfo);
			virtual NTSTATUS OnMediaRemoval(bool bLock);
			virtual NTSTATUS OnGetMediaTypes(PDISK_GEOMETRY pGeometryArray, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS OnDiskVerify(PVERIFY_INFORMATION pVerifyInfo);
			
			virtual NTSTATUS OnGetMediaTypesEx(PGET_MEDIA_TYPES pMediaTypes, PDEVICE_MEDIA_INFO pFirstMediaInfo, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS OnGetDeviceNumber(PSTORAGE_DEVICE_NUMBER pNumber);
			virtual NTSTATUS OnQueryProperty(PSTORAGE_PROPERTY_QUERY pQueryProperty, ULONG InputBufferLength, PSTORAGE_DESCRIPTOR_HEADER pOutput, ULONG OutputBufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS OnGetStableGuid(LPGUID lpGuid);

			virtual NTSTATUS OnScsiMiniportControl(SRB_IO_CONTROL *pControlBlock);

		public:
			static size_t BuildStorageDeviceDescriptor(void *pBuffer, 
				size_t MaxSize, 
				STORAGE_DEVICE_DESCRIPTOR *pReference = NULL, 
				TempStringA ProductID = ConstStringA(""), 
				TempStringA ProductRevision = ConstStringA(""), 
				TempStringA SerialNumber = ConstStringA(""),
				TempArrayReference<char> VendorSpecific = TempArrayReference<char>());

		};

	}
}
