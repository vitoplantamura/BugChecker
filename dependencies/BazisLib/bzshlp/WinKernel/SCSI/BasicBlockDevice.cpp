#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "BasicBlockDevice.h"
#include "../../endian.h"

using namespace BazisLib;
using namespace BazisLib::SCSI;
using namespace BazisLib::DDK;
using namespace BazisLib::DDK::SCSI;

SCSIResult BazisLib::DDK::SCSI::BasicSCSIBlockDevice::ExecuteSCSIRequest(GenericSRB &request)
{
	switch (request.RequestType)
	{
	case SCSIOP_READ_CAPACITY:
		return OnReadCapacity((PREAD_CAPACITY_DATA)request.DataBuffer, request.DataTransferSize);
	case SCSIOP_READ:
	case SCSIOP_WRITE:
		return OnBulkOperation(request.RequestType, (_CDB::_CDB10 *)request.pCDB, request.DataBuffer, request.DataTransferSize, &request.DataTransferSize);
	case SCSIOP_TEST_UNIT_READY:
	case SCSIOP_VERIFY:
		return scsiGood;
	case SCSIOP_MODE_SENSE:
		return OnModeSense((PMODE_PARAMETER_HEADER)request.DataBuffer, request.DataTransferSize, &request.DataTransferSize);
	case SCSIOP_REQUEST_SENSE:
		request.DataTransferSize = min(request.DataTransferSize, sizeof(SENSE_DATA));
		return OnRequestSense((PSENSE_DATA)request.DataBuffer, request.DataTransferSize);
	default:
#ifdef _DEBUG
		DbgPrint("Unknown SCSI opcode: %02X\n", request.RequestType);
#endif
		return scsiInvalidOpcode;
	}
}

SCSIResult BazisLib::DDK::SCSI::BasicSCSIBlockDevice::OnReadCapacity( PREAD_CAPACITY_DATA pData, size_t MaxSize )
{
	if (!pData || (MaxSize < sizeof(READ_CAPACITY_DATA)))
		return scsiInvalidField;
	pData->BytesPerBlock = SwapByteOrder32(GetSectorSize());
	pData->LogicalBlockAddress = SwapByteOrder32((unsigned)(GetSectorCount() - 1));
	return scsiGood;
}

SCSIResult BazisLib::DDK::SCSI::BasicSCSIBlockDevice::OnBulkOperation( SCSI_REQUEST_TYPE RequestType, _CDB::_CDB10 *pCdb, void *pBuffer, size_t Size, size_t *pDone )
{
	ULONG blockNumber = (pCdb->LogicalBlockByte0 << 24) | (pCdb->LogicalBlockByte1 << 16) | (pCdb->LogicalBlockByte2 << 8) | (pCdb->LogicalBlockByte3 << 0);
	ULONG blockCount = (pCdb->TransferBlocksMsb << 8) | pCdb->TransferBlocksLsb;

	if (!m_CachedSectorSize)
		m_CachedSectorSize = GetSectorSize();

	ULONG sizeFromCdb = blockCount * m_CachedSectorSize;

	if (Size > sizeFromCdb)
		Size = sizeFromCdb;

	switch (RequestType)
	{
	case SCSIOP_READ:
		*pDone = Read(((ULONGLONG)blockNumber) * m_CachedSectorSize, pBuffer, (unsigned)Size);
		if (!*pDone)
			return scsiReadRetriesExausted;
		else
			return scsiGood;
		break;
	case SCSIOP_WRITE:
		*pDone = Write(((ULONGLONG)blockNumber) * m_CachedSectorSize, pBuffer, (unsigned)Size);
		if (!*pDone)
			return scsiReadRetriesExausted;
		else
			return scsiGood;
		break;
	case SCSIOP_VERIFY:
		return scsiGood;
	default:
		return scsiInvalidOpcode;
	}
}

SCSIResult BazisLib::DDK::SCSI::BasicSCSIBlockDevice::OnModeSense( PMODE_PARAMETER_HEADER pHeader, size_t Size, size_t *pDone )
{
	if (!pHeader || (Size < sizeof(MODE_PARAMETER_HEADER)))
		return scsiInvalidField;
	*pDone = sizeof(MODE_PARAMETER_HEADER);
	memset(pHeader, 0, sizeof(MODE_PARAMETER_HEADER));
	if (!IsWritable())
		pHeader->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
	return scsiGood;
}

SCSIResult SCSI_DEVICE_BASIC_HANDLER_DECL BazisLib::DDK::SCSI::BasicSCSIBlockDevice::OnRequestSense(PSENSE_DATA pSense, size_t Size)
{
	ASSERT(FALSE);	//Should not be called
	SCSIResult result = {ssCommandTerminated};
	return result;
}

BazisLib::ActionStatus BazisLib::DDK::SCSI::BasicSCSIBlockDevice::GetInquiryData(OUT TypedBuffer<InquiryData> &pData )
{
	if (!pData.EnsureSizeAndSetUsed(sizeof(InquiryData)))
		return MAKE_STATUS(NoMemory);

	pData.Fill();

	pData->PeripheralDeviceType = GetSCSIDeviceType(); //DIRECT_ACCESS_DEVICE or similar
	pData->PeripheralQualifier = DEVICE_QUALIFIER_ACTIVE;
	pData->ResponseDataFormat = 2;

	size_t len = GetIDString(VendorId, (char *)pData->VendorID, sizeof(pData->VendorID));
	ASSERT(len <= sizeof(pData->VendorID));

	while (len < sizeof(pData->VendorID))
		pData->VendorID[len++] = ' ';

	len = GetIDString(ProductId, (char *)pData->ProductID, sizeof(pData->ProductID));
	ASSERT(len <= sizeof(pData->ProductID));

	while (len < sizeof(pData->ProductID))
		pData->ProductID[len++] = ' ';

	C_ASSERT(sizeof(pData->ProductRevisionLevel) == 4);
	char sz[5] = {0,};
	_snprintf(sz, sizeof(sz), "%04d", GetRevisionLevel() % 10000);

	memcpy(pData->ProductRevisionLevel, sz, sizeof(pData->ProductRevisionLevel));
	return MAKE_STATUS(Success);
}

#endif