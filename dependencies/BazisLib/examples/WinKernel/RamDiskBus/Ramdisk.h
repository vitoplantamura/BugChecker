#pragma once

#ifdef __linux__
#include <bzslnx/bzslnx.h>
#else
#include <bzshlp/WinKernel/crossplatform.h>
#endif

#include <bzscore/file.h>
#include <bzshlp/bzsdisk.h>

#define RAMDISK_SECTOR_SIZE	512

namespace BazisLib
{
	class RamDisk : public AIBasicDisk, AUTO_OBJECT
	{
	protected:
		ULONGLONG m_SectorCount;
		ULONGLONG m_TotalSize;
		std::vector<char *> m_Buffers;

		unsigned DoTransfer(ULONGLONG ByteOffset, void *pBuffer, unsigned Length, bool read);


	public:
		unsigned GetSectorSize() {return RAMDISK_SECTOR_SIZE;}
		virtual void Dispose(){}

		RamDisk(unsigned MegabyteCount);
		~RamDisk();

		virtual ULONGLONG GetSectorCount() override;
		
		virtual unsigned Read(ULONGLONG ByteOffset, void *pBuffer, unsigned Length) override;
		virtual unsigned Write(ULONGLONG ByteOffset, const void *pBuffer, unsigned Length) override;
		virtual LPGUID GetStableGuid() override {return NULL;}

		virtual bool DeviceControl(unsigned CtlCode, void *pBuffer, unsigned InSize, unsigned OutSize, unsigned *pBytesDone) override {return false;}
		virtual ActionStatus Initialize() override {return MAKE_STATUS(Success);}
		virtual bool IsWritable() {return true;}
	};
}
