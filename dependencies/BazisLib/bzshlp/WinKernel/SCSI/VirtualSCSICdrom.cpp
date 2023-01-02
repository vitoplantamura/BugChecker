#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "VirtualSCSICdrom.h"
#include "../../endian.h"

using namespace BazisLib::DDK;

static void BuildMSFTrackAddress(unsigned SectorNumber, unsigned char *pAddr)
{
	if (!pAddr)
		return;
	SectorNumber += 150;
	pAddr[3] = (UCHAR)(SectorNumber % 75);
	SectorNumber /= 75;
	pAddr[2] = (UCHAR)(SectorNumber % 60);
	pAddr[1] = (UCHAR)(SectorNumber / 60);
	pAddr[0] = 0;
}

#define TOC_DATA_TRACK 4

static void InitializeTrackRecord(TRACK_DATA *pTrack, unsigned TrackNumber, unsigned SectorNumber, bool bMSF)
{
	if (!pTrack)
		return;
	pTrack->Control = TOC_DATA_TRACK;
	pTrack->Adr = 1;
	pTrack->TrackNumber = (UCHAR)TrackNumber;
	if (bMSF)
		BuildMSFTrackAddress(SectorNumber, pTrack->Address);
	else
		*((unsigned *)pTrack->Address) = BazisLib::SwapByteOrder32(SectorNumber);
}

SCSIResult BazisLib::DDK::SCSI::VirtualSCSICdrom::ExecuteSCSIRequest(GenericSRB &request)
{
	switch (request.RequestType)
	{
	case SCSIOP_READ_TOC:
		return OnReadTOC(request);
	case SCSIOP_LOAD_UNLOAD:
		return OnLoadUnload((CDB::_LOAD_UNLOAD *)request.pCDB);
// 	case SCSIOP_GET_EVENT_STATUS:
// 		return OnGetEventStatus(&pCDB->GET_EVENT_STATUS_NOTIFICATION, (PNOTIFICATION_EVENT_STATUS_HEADER)pDataBuffer, DataTransferSize, pDataTransferred);
	default:
		return __super::ExecuteSCSIRequest(request);
	}
}

static inline unsigned TrackRecordEndOffset(unsigned recNum)
{
#pragma warning(push)
#pragma warning(disable: 4311)
#pragma warning(disable: 4302)
	return (unsigned)(&((PCDROM_TOC)NULL)->TrackData[recNum + 1]);
#pragma warning(pop)
}

SCSIResult BazisLib::DDK::SCSI::VirtualSCSICdrom::OnReadTOC(Requests::ReadTOC &request)
{
	enum {TocHeaderSize = 4};
	unsigned trackCount = GetTrackCount();
	unsigned tocLen = 2 + sizeof(TRACK_DATA) * (trackCount + 1);

	unsigned bias = request.pCDB->StartingTrack - 1;
	if (bias == -1)
		bias = 0;
	else if (bias >= trackCount)
		return scsiInvalidField;

	if (!request.DataBuffer || (request.DataTransferSize < TocHeaderSize))
		return scsiInvalidField;
	if ((tocLen + 2) > sizeof(CDROM_TOC))
		return scsiInvalidField;

	PCDROM_TOC pData = request.DataBuffer;
	pData->Length[0] = (tocLen >> 8) & 0xFF;
	pData->Length[1] = (tocLen >> 0) & 0xFF;

	pData->FirstTrack = 1;
	pData->LastTrack = trackCount + 1;

	unsigned reportedTrackCount = trackCount - bias;

	for (unsigned i = 0; i < reportedTrackCount; i++)
	{
		if (TrackRecordEndOffset(i) > request.DataTransferSize)
			break;
		InitializeTrackRecord(&pData->TrackData[i], i + 1 + bias, (i + bias) ? GetTrackEndSector(i + bias - 1) : 0, request.pCDB->Msf);
	}
	
	if (TrackRecordEndOffset(reportedTrackCount) <= request.DataTransferSize)
		InitializeTrackRecord(&pData->TrackData[reportedTrackCount], 0xAA, GetTrackEndSector(trackCount - 1), request.pCDB->Msf);

	if (request.pCDB->Format == 0x01)
		for (unsigned i = 0; i < reportedTrackCount; i++)
		{
			if (TrackRecordEndOffset(i) > request.DataTransferSize)
				break;
			pData->TrackData[i].TrackNumber = 1;
		}

	return scsiGood;
}

/*BazisLib::DDK::SCSI::SCSI_STATUS BazisLib::DDK::SCSI::VirtualSCSICdrom::OnGetEventStatus( const _CDB::_GET_EVENT_STATUS_NOTIFICATION *pRequest, PNOTIFICATION_EVENT_STATUS_HEADER pReply, ULONG Size, ULONG *pDone )
{
	if (!pRequest || !pReply || (Size < sizeof(NOTIFICATION_EVENT_STATUS_HEADER)))
		return SRB_STATUS_INVALID_REQUEST;
	pReply->NEA = false;
	pReply->NotificationClass = NOTIFICATION_NO_CLASS_EVENTS;
	*pDone = sizeof(NOTIFICATION_EVENT_STATUS_HEADER);
	return SRB_STATUS_SUCCESS;
}*/
#endif