#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "cdrom.h"

#include <ntddcdrm.h>

using namespace BazisLib::DDK;

/*! \page cd_basics CD format basics
	\section cd_intro Introduction
	The purpose of this page is do describe the common properties of standard CD formats and to define how they are translated
	into flat-addressed CD device file under Windows. Also, a quick explanation of TOC is given.

	\section cd_toc Table Of Contents (TOC)
	The TOC is a part of a CD that is not accessible using the standard address space. It is returned as a response to
	IOCTL_CDROM_READ_TOC code and is handled inside BasicCDVolume::OnReadTOC() method. The following protected methods
	are provided by BasicCDVolume to support a custom OnReadToc() implementation:
	<ul>
		<li>BasicCDVolume::InitializeTrackRecord()</li>
		<li>BasicCDVolume::InitializeTOCHeader()</li>
	</ul>

	\section cd_iso ISO format
	The ISO format represents a single track image that can be almost directly mapped into flat-addressed windows device file,
	with only one exception: as the first CD track starts from sector 150, the TOC generated from an ISO file should reference
	the first track as the track starting from sector 150 (0:2.0) despite the fact that it starts from sector 0 in the file.
	Note that Windows device with flat addressing actually represents the first track for single-track images. That means that
	the offset in the device directly corresponds to the offset in ISO file. That way, the only way the 150-sector gap shows itself
	is in the TOC.

	\section cd_cue CUE format
	The CUE file is a text file containing information about tracks inside a binary (raw) file. Each track has the information
	about its format (such as MODE2/2352). The number represents the physical sector size. See the next paragraph for details.

	\section cd_img_bin IMG/BIN formats
	Each IMG/BIN file contains the sectors from the CD in a raw format.	Usually, the CD sector size is greater than the logical
	sector size, as it contains some error correcting information. For example, MODE2/2352 means that the physical sector size
	is 2352 bytes = 2048 bytes data + 304 bytes additional info. When mapped to windows Device File, the additional info is truncated.
	It means that the original file is mapped sector-by-sector, however, the last 304 bytes of each sctor are omitted.
	Additionally, the real sector data in such files does not start from offset 0. The starting offset is determined by file format:
	<ul>
		<li>For MODE1/2352 the offset is 16 bytes</li>
		<li>For MODE2/2352 the offset is 24 bytes</li>
		<li>For raw MODE2/2352 the offset is 0 bytes</li>
		<li>For raw MODE2/2336 the offset is 16 bytes</li>
	</ul>

*/

BasicCDVolume::BasicCDVolume(LPCWSTR pwszDevicePrefix,
							  bool bDeleteThisAfterRemoveRequest,
							  ULONG DeviceType,
							  ULONG DeviceCharacteristics,
							  bool bExclusive,
							  ULONG AdditionalDeviceFlags) :
	BasicStorageVolume(pwszDevicePrefix, bDeleteThisAfterRemoveRequest, DeviceType, DeviceCharacteristics, bExclusive, AdditionalDeviceFlags)
{
}

BasicCDVolume::~BasicCDVolume()
{
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

NTSTATUS BasicCDVolume::OnDeviceControl(ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone)
{
	switch (ControlCode)
	{
	case IOCTL_CDROM_EJECT_MEDIA:
	case IOCTL_DISK_EJECT_MEDIA:
		return OnEject();
	case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
		ASSURE_SIZE_AND_CALL(OnGetDriveGeometry, DISK_GEOMETRY);
	case IOCTL_CDROM_READ_TOC:
		ASSURE_SIZE_AND_CALL(OnReadTOC, CDROM_TOC);
	case IOCTL_CDROM_GET_LAST_SESSION:
		ASSURE_SIZE_AND_CALL(OnGetLastSession, CDROM_TOC_SESSION_DATA);
	default:
		return __super::OnDeviceControl(ControlCode, IsInternal, pInOutBuffer, InputLength, OutputLength, pBytesDone);
	}
}

void BasicCDVolume::BuildCDTrackAddress(unsigned SectorNumber, unsigned char *pAddr)
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

void BasicCDVolume::InitializeTrackRecord(TRACK_DATA *pTrack, unsigned TrackNumber, unsigned SectorNumber)
{
	if (!pTrack)
		return;
	pTrack->Control = 4;
	pTrack->Adr = 1;
	pTrack->TrackNumber = (UCHAR)TrackNumber;
	BuildCDTrackAddress(SectorNumber, pTrack->Address);
}

void BasicCDVolume::InitializeTOCHeader(CDROM_TOC *pTOC, unsigned TrackCount)
{
	memset(pTOC, 0, sizeof(CDROM_TOC));
	unsigned len = 2 + sizeof(TRACK_DATA) * TrackCount;
	pTOC->Length[0] = (len >> 8) & 0xFF;
	pTOC->Length[1] = (len >> 0) & 0xFF;
	
	pTOC->FirstTrack = 1;
	pTOC->LastTrack = 1;
}

NTSTATUS BasicCDVolume::OnReadTOC(CDROM_TOC *pTOC)
{
	if (!pTOC)
		return STATUS_INVALID_PARAMETER;
	InitializeTOCHeader(pTOC, 2);
	InitializeTrackRecord(&pTOC->TrackData[0], 1, 0);
	InitializeTrackRecord(&pTOC->TrackData[1], 0xAA, (unsigned)GetSectorCount());
	return STATUS_SUCCESS;
}

NTSTATUS BasicCDVolume::OnGetLastSession(CDROM_TOC_SESSION_DATA *pSession)
{
	if (!pSession)
		return STATUS_INVALID_PARAMETER;
	memset(pSession, 0, sizeof(CDROM_TOC_SESSION_DATA));
	pSession->Length[0] = 0;
	pSession->Length[1] = 2 + sizeof(TRACK_DATA);
	
	pSession->FirstCompleteSession = 1;
	pSession->LastCompleteSession = 1;

	pSession->TrackData[0].Control = 4;
	pSession->TrackData[0].Adr = 1;
	pSession->TrackData[0].TrackNumber = 1;
	pSession->TrackData[0].Address[2] = 0x02;	//Addr = 0x200
	return STATUS_SUCCESS;
}

NTSTATUS BasicCDVolume::OnEject()
{
	return STATUS_NOT_SUPPORTED;
}
#endif