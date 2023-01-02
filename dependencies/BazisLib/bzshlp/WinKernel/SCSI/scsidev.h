#pragma once

#include "../../../bzscore/objmgr.h"
#include "../../../bzscore/SCSI.h"
extern "C" {
#include <scsi.h>
#include <ntddcdrm.h>
}

namespace BazisLib
{
	namespace DDK
	{
		using namespace BazisLib::SCSI;

		namespace SCSI
		{
			namespace Requests
			{
				typedef _SCSIRequest<SCSIOP_INQUIRY, _CDB::_CDB6INQUIRY, INQUIRYDATA> Inquiry;
				typedef _SCSIRequest<SCSIOP_READ_TOC, _CDB::_READ_TOC, CDROM_TOC> ReadTOC;
			}
		}
	}
}