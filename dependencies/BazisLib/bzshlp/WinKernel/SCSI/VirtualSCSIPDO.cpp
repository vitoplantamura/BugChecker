#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "VirtualSCSIPDO.h"
#include "../storage.h"
#include "../../endian.h"

#include "../ioctldbg.h"

using namespace BazisLib;

const unsigned MaximumTransferLength = 16 * 1024 * 1024;

struct OutgoingSRB : public SCSI::GenericSRB
{
	OutgoingSRB(PSCSI_REQUEST_BLOCK pSRB, void *pDataBuffer)
	{
		RequestType = pSRB->Cdb[0];
		DataTransferSize = pSRB->DataTransferLength;
		pCDB = (SCSI::GenericCDB *)&pSRB->Cdb;
		DataBuffer = pDataBuffer;
	}

	OutgoingSRB(UCHAR (&pCDB)[16], void *pDataBuffer, size_t dataSize)
	{
		RequestType = (SCSI::SCSI_REQUEST_TYPE)pCDB[0];
		this->pCDB = (SCSI::GenericCDB *)pCDB;
		this->DataBuffer = pDataBuffer;
		this->DataTransferSize = dataSize;
	}
};

NTSTATUS BazisLib::DDK::VirtualSCSIPDO::OnScsiExecute( ULONG ScsiExecuteCode, SCSI_REQUEST_BLOCK *pSRB, PVOID pMappedSystemBuffer )
{
	TreatSystemControlAsSCSIRequests();
	if (!m_pSCSIDevice)
		return STATUS_INTERNAL_ERROR;
	if (!pSRB)
		return STATUS_INVALID_PARAMETER;
	if (pSRB->Lun || pSRB->PathId || pSRB->TargetId)
	{
		pSRB->ScsiStatus = SCSISTAT_GOOD;
		pSRB->SrbStatus = SRB_STATUS_NO_DEVICE;
		return STATUS_SUCCESS;
	}

	OutgoingSRB translatedSRB(pSRB, pMappedSystemBuffer);
	SCSIResult result;

	switch (pSRB->Function)
	{
	case SRB_FUNCTION_CLAIM_DEVICE:
		pSRB->DataBuffer = "";
		pSRB->DataTransferLength = 0;
		pSRB->ScsiStatus = SCSISTAT_GOOD;
		pSRB->SrbStatus = SRB_STATUS_SUCCESS;
		break;
	case SRB_FUNCTION_ATTACH_DEVICE:
	case SRB_FUNCTION_RELEASE_DEVICE:
	case SRB_FUNCTION_FLUSH_QUEUE:
	case SRB_FUNCTION_RELEASE_QUEUE:
	case SRB_FUNCTION_FLUSH:
		pSRB->ScsiStatus = SCSISTAT_GOOD;
		pSRB->SrbStatus = SRB_STATUS_SUCCESS;
		break;
	case SRB_FUNCTION_EXECUTE_SCSI:
		result = DoHandleRequest(translatedSRB);
		pSRB->ScsiStatus = result.Status;
		pSRB->DataTransferLength = (ULONG)translatedSRB.DataTransferSize;
		if (pSRB->ScsiStatus == SCSISTAT_GOOD)
			pSRB->SrbStatus = SRB_STATUS_SUCCESS;
		else
		{
			pSRB->SrbStatus = SRB_STATUS_ERROR;
			if (!(pSRB->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) && pSRB->SenseInfoBuffer && pSRB->SenseInfoBufferLength)
			{
				pSRB->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
				pSRB->SenseInfoBufferLength = (UCHAR)result.Export((RequestSenseData *)pSRB->SenseInfoBuffer, pSRB->SenseInfoBufferLength);
			}
			else
				m_PendingSenseData = result;
		}

		if ((pSRB->Cdb[0] == SCSIOP_LOAD_UNLOAD) && m_bUnplugPDOOnEject)
			RequestBusToPlugOut();
		break;
	default:
		return STATUS_INVALID_DEVICE_REQUEST;
	}
	return pSRB->SrbStatus == SRB_STATUS_SUCCESS ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

void BazisLib::DDK::VirtualSCSIPDO::GenerateIDStrings( int UniqueDeviceNumber )
{
	__super::GenerateIDStrings(UniqueDeviceNumber);
	if (m_pSCSIDevice)
	{
		if (ProvideInquiryData().Successful())
		{
			switch (m_pCachedInquiryData->PeripheralDeviceType)
			{
			case DIRECT_ACCESS_DEVICE:
				m_HardwareIDs = L"GenDisk";
				break;
			case OPTICAL_DEVICE:
			case READ_ONLY_DIRECT_ACCESS_DEVICE:
				m_HardwareIDs = L"GenCdrom";
				break;
			}
		}
	}
	if (!m_ReportedDeviceName.empty())
		m_DeviceName = m_ReportedDeviceName;

	if (!m_UniqueID.empty())
		m_InstanceID = m_UniqueID;
}

template <size_t _MaxLen> TempStrPointerWrapperA static inline ScsiFieldToString(const UCHAR (&field)[_MaxLen])
{
	for (int i = _MaxLen - 1; i > 0; i--)
		if (field[i] != ' ')
			return TempStrPointerWrapperA((char *)field, i + 1);
	return TempStrPointerWrapperA((char *)field, 0);
}

NTSTATUS BazisLib::DDK::VirtualSCSIPDO::OnQueryProperty( PSTORAGE_PROPERTY_QUERY pQueryProperty, ULONG InputBufferLength, PSTORAGE_DESCRIPTOR_HEADER pOutput, ULONG BufferLength, PULONG_PTR pBytesDone )
{
	if ((BufferLength >= 8) && (pQueryProperty->QueryType == PropertyStandardQuery) && (pQueryProperty->PropertyId == StorageDeviceProperty))
	{
		if (m_pSCSIDevice)
		{
			if (ProvideInquiryData().Successful() && (m_pCachedInquiryData.GetSize() == sizeof(INQUIRYDATA)))
			{
				*pBytesDone = StorageDevice::BuildStorageDeviceDescriptor(pOutput, BufferLength, NULL, 
					ScsiFieldToString(m_pCachedInquiryData->ProductID),
					ScsiFieldToString(m_pCachedInquiryData->ProductRevisionLevel),
					ConstStringA(""),
					TempArrayReference<char>((char *)m_pCachedInquiryData->VendorSpecific, sizeof(m_pCachedInquiryData->VendorSpecific)));
				return STATUS_SUCCESS;
			}
		}
		
		*pBytesDone = StorageDevice::BuildStorageDeviceDescriptor(pOutput, BufferLength, NULL, ConstStringA("BazisLib Virtual Storage Device"));
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
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS BazisLib::DDK::VirtualSCSIPDO::OnGetDriveGeometry(PDISK_GEOMETRY pGeo)
{
	if (!m_pSCSIDevice)
		return STATUS_INTERNAL_ERROR;

	EmbeddedSRB<SCSIOP_READ_CAPACITY, CDB, READ_CAPACITY_DATA> readCapacitySRB;
	SCSIResult st = DoHandleRequest(readCapacitySRB);

	if (!st.Successful())
		return STATUS_INTERNAL_ERROR;

	unsigned sectorCount = SwapByteOrder32(readCapacitySRB.Data.LogicalBlockAddress) + 1;
	unsigned sectorSize = SwapByteOrder32(readCapacitySRB.Data.BytesPerBlock);

	const int SectorsPerTrack = 64;
	if (sectorCount % 64)
	{
		pGeo->Cylinders.QuadPart = sectorCount;
		pGeo->SectorsPerTrack = 1;
	}
	else
	{
		pGeo->Cylinders.QuadPart = sectorCount / SectorsPerTrack;
		pGeo->SectorsPerTrack = SectorsPerTrack;
	}
	pGeo->TracksPerCylinder = 1;
	pGeo->BytesPerSector = sectorSize;
	pGeo->MediaType = FixedMedia;
	return STATUS_SUCCESS;
}

NTSTATUS BazisLib::DDK::VirtualSCSIPDO::OnDeviceControl( ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone )
{
	switch(ControlCode)
	{
	case IOCTL_STORAGE_QUERY_PROPERTY:
		return OnQueryProperty((PSTORAGE_PROPERTY_QUERY)pInOutBuffer, InputLength, (PSTORAGE_DESCRIPTOR_HEADER)pInOutBuffer, OutputLength, pBytesDone);
	case IOCTL_DISK_GET_DRIVE_GEOMETRY:
		if (OutputLength < sizeof(DISK_GEOMETRY))
			return STATUS_BUFFER_TOO_SMALL;
		*pBytesDone = sizeof(DISK_GEOMETRY);
		return OnGetDriveGeometry((PDISK_GEOMETRY)pInOutBuffer);
	case IOCTL_SCSI_GET_ADDRESS:
		if (OutputLength < sizeof(SCSI_ADDRESS))
			return STATUS_BUFFER_TOO_SMALL;
		else
		{
			PSCSI_ADDRESS pAddr = (PSCSI_ADDRESS)pInOutBuffer;
			pAddr->Length = sizeof(SCSI_ADDRESS);
			pAddr->PortNumber = pAddr->PathId = pAddr->TargetId = pAddr->Lun = 0;
			*pBytesDone = sizeof(SCSI_ADDRESS);
			return STATUS_SUCCESS;	
		}
	case IOCTL_SCSI_PASS_THROUGH:
	case IOCTL_SCSI_PASS_THROUGH_DIRECT:
		ASSERT(FALSE);	//This should have been handled inside DispatchRoutine()
		return STATUS_INTERNAL_ERROR;
	}

	/*if (ControlCode == IOCTL_MOUNTDEV_QUERY_STABLE_GUID)
	{
		LPCGUID pGuid = m_pSCSIDevice ? m_pSCSIDevice->GetStableGuid() : NULL;
		if (pGuid)
		{
			if (OutputLength < sizeof(GUID))
				return STATUS_BUFFER_TOO_SMALL;
			memcpy(pInOutBuffer, pGuid, sizeof(GUID));
			*pBytesDone = sizeof(GUID);
			return STATUS_SUCCESS;
		}
	}*/

	NTSTATUS st = __super::OnDeviceControl(ControlCode, IsInternal, pInOutBuffer, InputLength, OutputLength, pBytesDone);
	if (st != STATUS_INVALID_DEVICE_REQUEST)
		return st;

#ifdef _DEBUG
	DumpDeviceControlCode("VirtualSCSIPDO: Unhandled device IO control code: %s\r\n", ControlCode);
#endif

	if (m_pSCSIDevice)
		return m_pSCSIDevice->DeviceControl(ControlCode, pInOutBuffer, InputLength, OutputLength, (unsigned *)pBytesDone) ? STATUS_SUCCESS : STATUS_INVALID_DEVICE_REQUEST;
	else
		return STATUS_INVALID_DEVICE_REQUEST;
}

template <class _ScsiPassThroughStruct> class PassThroughHelper
{
public:
	class DataBuffer
	{
	private:
		PVOID _pBuffer;

	public:
		DataBuffer(_ScsiPassThroughStruct *pStruct)
			: _pBuffer(NULL)
		{
			if (!pStruct->DataBufferOffset)
				return;
			_pBuffer = ((char *)pStruct) + pStruct->DataBufferOffset;
		}

		operator PVOID()
		{
			return _pBuffer;
		}
	};

	static bool CheckBufferIntegrity(_ScsiPassThroughStruct *pStruct, ULONG InputLength, ULONG OutputLength)
	{
		if (!pStruct->DataBufferOffset)
			return true;
		if (pStruct->DataBufferOffset < sizeof(_ScsiPassThroughStruct))
			return false;

		unsigned dataRelatedBufferSize = 0;

		switch (pStruct->DataIn)
		{
		case SCSI_IOCTL_DATA_IN:
			dataRelatedBufferSize = OutputLength;
			break;
		case SCSI_IOCTL_DATA_OUT:
			dataRelatedBufferSize = InputLength;
			break;
		}

		if (pStruct->DataIn == SCSI_IOCTL_DATA_UNSPECIFIED)
		{
			if (pStruct->DataBufferOffset || pStruct->DataTransferLength)
				return false;
		}
		else
			if ((pStruct->DataBufferOffset + pStruct->DataTransferLength) > dataRelatedBufferSize)
				return false;
		return true;
	}

	static size_t GetOutputBufferSize(_ScsiPassThroughStruct *pStruct)
	{
		return pStruct->DataBufferOffset + pStruct->DataTransferLength;
	}
};

template <class _ScsiPassThroughStruct> class DirectPassThroughHelper
{
public:
	class DataBuffer
	{
	private:
		PVOID _pUserBuffer, _pKernelBuffer;
		size_t _KernelBufferSize;
		bool _CopyBack;

	public:
		DataBuffer(_ScsiPassThroughStruct *pStruct)
			: _pUserBuffer(NULL)
			, _pKernelBuffer(NULL)
			, _KernelBufferSize(0)
			, _CopyBack(false)
		{
			_pUserBuffer = (PVOID)pStruct->DataBuffer;
			_KernelBufferSize = pStruct->DataTransferLength;
			_CopyBack = pStruct->DataIn == SCSI_IOCTL_DATA_IN;
			if (_KernelBufferSize)
			{
				_pKernelBuffer = malloc(_KernelBufferSize);
				if (pStruct->DataIn == SCSI_IOCTL_DATA_OUT)
					memcpy(_pKernelBuffer, _pUserBuffer, _KernelBufferSize);
			}
		}

		operator PVOID()
		{
			return _pKernelBuffer;
		}

		~DataBuffer()
		{
			if (_CopyBack)
				memcpy(_pUserBuffer, _pKernelBuffer, _KernelBufferSize);

			if (_pKernelBuffer)
				free(_pKernelBuffer);
		}
	};

	static bool CheckBufferIntegrity(_ScsiPassThroughStruct *pStruct, ULONG InputLength, ULONG OutputLength)
	{
		return true;
	}

	static unsigned GetOutputBufferSize(_ScsiPassThroughStruct *pStruct)
	{
		return sizeof(_ScsiPassThroughStruct);
	}
};

template<> class PassThroughHelper<SCSI_PASS_THROUGH_DIRECT> : public DirectPassThroughHelper<SCSI_PASS_THROUGH_DIRECT> {};

#ifdef _WIN64
template<> class PassThroughHelper<SCSI_PASS_THROUGH_DIRECT32> : public DirectPassThroughHelper<SCSI_PASS_THROUGH_DIRECT32> {};
#endif

template <class _PassThroughType> NTSTATUS BazisLib::DDK::VirtualSCSIPDO::OnScsiPassThrough(_PassThroughType *pPassThrough, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone)
{
	if (InputLength < sizeof(_PassThroughType))
		return STATUS_INVALID_DEVICE_REQUEST;

	PassThroughHelper<_PassThroughType>::DataBuffer pDataBuffer(pPassThrough);

	if (!PassThroughHelper<_PassThroughType>::CheckBufferIntegrity(pPassThrough, InputLength, OutputLength))
		return STATUS_INVALID_DEVICE_REQUEST;

	if (pPassThrough->SenseInfoOffset)
		if (((pPassThrough->SenseInfoOffset + pPassThrough->SenseInfoLength) > OutputLength) || (pPassThrough->SenseInfoOffset < sizeof(_PassThroughType)))
			return STATUS_INVALID_DEVICE_REQUEST;

	PSENSE_DATA pSenseBuffer = pPassThrough->SenseInfoOffset ? (PSENSE_DATA)(((char *)pPassThrough) + pPassThrough->SenseInfoOffset) : NULL;

	if (!m_pSCSIDevice)
		return STATUS_INTERNAL_ERROR;

	OutgoingSRB translatedSRB(pPassThrough->Cdb, pDataBuffer, pPassThrough->DataTransferLength);
	SCSIResult result = DoHandleRequest(translatedSRB);
	pPassThrough->DataTransferLength = (unsigned)translatedSRB.DataTransferSize;

	pPassThrough->ScsiStatus = result.Status;

	if (!result.Successful())
		pPassThrough->SenseInfoLength = (UCHAR)result.Export((RequestSenseData *)pSenseBuffer, pPassThrough->SenseInfoLength);

	NTSTATUS ntStatus = STATUS_SUCCESS;

	unsigned totalResultSize = sizeof(_PassThroughType);
	if (pPassThrough->SenseInfoLength)
		totalResultSize = pPassThrough->SenseInfoOffset + pPassThrough->SenseInfoLength;

	if (pPassThrough->DataIn == SCSI_IOCTL_DATA_IN)
	{
		size_t maxSize = PassThroughHelper<_PassThroughType>::GetOutputBufferSize(pPassThrough);
		if (maxSize > totalResultSize)
			totalResultSize = (unsigned)maxSize;
	}

	*pBytesDone = totalResultSize;
	return ntStatus;		
}

BazisLib::SCSI::SCSIResult BazisLib::DDK::VirtualSCSIPDO::DoHandleRequest( SCSI::GenericSRB &request )
{
	if (request.RequestType == SCSIOP_INQUIRY)
	{
		if (!ProvideInquiryData().Successful())
			return scsiInternalError;

		request.DataTransferSize = min(request.DataTransferSize, m_pCachedInquiryData.GetSize());
		memcpy(request.DataBuffer, m_pCachedInquiryData.GetData(), request.DataTransferSize);
		return scsiGood;
	}
	else if (request.RequestType == SCSIOP_REQUEST_SENSE)
	{
		request.DataTransferSize = m_PendingSenseData.Export((RequestSenseData *)request.DataBuffer, request.DataTransferSize);
		m_PendingSenseData.Reset();
		return scsiGood;
	}

	SCSIResult result = m_pSCSIDevice->ExecuteSCSIRequest(request);

	m_PendingSenseData = result;
	return result;
}

NTSTATUS BazisLib::DDK::VirtualSCSIPDO::OnStartDevice( IN PCM_RESOURCE_LIST AllocatedResources, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated )
{
	if (!m_pSCSIDevice)
		return STATUS_INVALID_DEVICE_REQUEST;
	if (!m_bAlreadyInitialized)
	{
		ActionStatus status = m_pSCSIDevice->Initialize();
		if (!status.Successful())
			return status.ConvertToNTStatus();
		status = ProvideInquiryData();
		if (!status.Successful())
			return status.ConvertToNTStatus();

		m_bAlreadyInitialized = true;
	}
	return __super::OnStartDevice(AllocatedResources, AllocatedResourcesTranslated);
}

BazisLib::ActionStatus BazisLib::DDK::VirtualSCSIPDO::ProvideInquiryData()
{
	if (!m_bInquiryDataAvailable)
	{
		if (m_pSCSIDevice)
			m_pSCSIDevice->GetInquiryData(m_pCachedInquiryData);
		m_bInquiryDataAvailable = true;
	}

	if (m_pCachedInquiryData)
		return MAKE_STATUS(Success);
	else
		return MAKE_STATUS(NotSupported);
}

template NTSTATUS BazisLib::DDK::VirtualSCSIPDO::OnScsiPassThrough(SCSI_PASS_THROUGH *pPassThrough, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone);
template NTSTATUS BazisLib::DDK::VirtualSCSIPDO::OnScsiPassThrough(SCSI_PASS_THROUGH_DIRECT *pPassThrough, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone);

#ifdef _WIN64
template NTSTATUS BazisLib::DDK::VirtualSCSIPDO::OnScsiPassThrough(SCSI_PASS_THROUGH32 *pPassThrough, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone);
template NTSTATUS BazisLib::DDK::VirtualSCSIPDO::OnScsiPassThrough(SCSI_PASS_THROUGH_DIRECT32 *pPassThrough, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone);
#endif

#endif