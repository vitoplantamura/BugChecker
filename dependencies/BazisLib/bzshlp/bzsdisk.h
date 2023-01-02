#pragma once
#include "cmndef.h"
#include "../bzscore/objmgr.h"
#include "../bzscore/status.h"

namespace BazisLib
{
	class AIBasicBlockDevice : AUTO_INTERFACE
	{
	public:
		virtual unsigned GetSectorSize()=0;
		virtual ULONGLONG GetSectorCount()=0;
		virtual void Dispose()=0;
		
		virtual unsigned Read(ULONGLONG ByteOffset, void *pBuffer, unsigned Length)=0;
		virtual unsigned Write(ULONGLONG ByteOffset, const void *pBuffer, unsigned Length)=0;
		virtual bool DeviceControl(unsigned CtlCode, void *pBuffer, unsigned InSize, unsigned OutSize, unsigned *pBytesDone)=0;
		virtual ActionStatus Initialize()=0;

		virtual ~AIBasicBlockDevice() {}
	};

	class AIBasicDisk : public AIBasicBlockDevice
	{
	public:
		virtual LPCGUID GetStableGuid()=0;
		virtual bool IsWritable()=0;

		virtual ~AIBasicDisk() {}
	};
}
