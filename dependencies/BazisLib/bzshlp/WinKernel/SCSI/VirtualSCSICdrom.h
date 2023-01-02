#pragma once

#include "scsidev.h"
#include "../../bzsdisk.h"
#include "BasicBlockDevice.h"
#include <ntddcdrm.h>

namespace BazisLib
{
	namespace DDK
	{
		namespace SCSI
		{
			class VirtualSCSICdrom : public BasicSCSIBlockDevice
			{
			public:
				enum {CDROM_SECTOR_SIZE = 2048};
			public:
				virtual SCSIResult ExecuteSCSIRequest(GenericSRB &request) override;

			protected:
				virtual SCSIResult OnReadTOC(Requests::ReadTOC &request);
				virtual SCSIResult OnLoadUnload(const _CDB::_LOAD_UNLOAD *pRequest) {return scsiGood;}
				//virtual SCSI_STATUS OnGetEventStatus(const _CDB::_GET_EVENT_STATUS_NOTIFICATION *pRequest, PNOTIFICATION_EVENT_STATUS_HEADER pReply, ULONG Size, ULONG *pDone);

			public:	
				virtual unsigned GetSectorSize() {return CDROM_SECTOR_SIZE;}
				virtual bool IsWritable() {return false;}
				virtual UCHAR GetSCSIDeviceType() { return READ_ONLY_DIRECT_ACCESS_DEVICE; }
				virtual unsigned Write(ULONGLONG ByteOffset, const void *pBuffer, unsigned Length) {return 0;}

			public: //Unimplemented AIBasicDisk methods
				virtual ULONGLONG GetSectorCount()=0;
				virtual unsigned Read(ULONGLONG ByteOffset, void *pBuffer, unsigned Length)=0;

			public:	//CDROM-specific methods
				virtual unsigned GetTrackCount()=0;
				virtual unsigned GetTrackEndSector(unsigned TrackIndex)=0;
			};
		}
	}
}
