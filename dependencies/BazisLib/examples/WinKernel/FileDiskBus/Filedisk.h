#pragma once

#ifdef __linux__
#include <bzslnx/bzslnx.h>
#else
#include <bzshlp/WinKernel/crossplatform.h>
#endif

#include <bzscore/autofile.h>
#include <bzshlp/bzsdisk.h>

namespace BazisLib
{
	class FileDisk : public AIBasicDisk
	{
	protected:
		ManagedPointer<BazisLib::AIFile> m_pFile;
		ULONGLONG m_FileOffset;
		ULONGLONG m_SectorCount;
		unsigned m_SectorSize;
		unsigned m_MinMBCount;

	private:
		//Do not override this. Use constructor parameter instead
		virtual unsigned GetSectorSize() override;

	protected:
		unsigned SectorSize() {return m_SectorSize;}

	public:

		void Dispose();

		FileDisk(ManagedPointer<AIFile> &pFile, ULONGLONG FileOffset = 0, unsigned SectorSize = 512, unsigned minMBCount = 0);
		~FileDisk();

		virtual ULONGLONG GetSectorCount() override;
		virtual bool IsWritable() override {return true;} 

		virtual unsigned Read(ULONGLONG ByteOffset, void *pBuffer, unsigned Length) override;
		virtual unsigned Write(ULONGLONG ByteOffset, const void *pBuffer, unsigned Length) override;
		virtual LPGUID GetStableGuid() override {return NULL;}

		virtual bool DeviceControl(unsigned CtlCode, void *pBuffer, unsigned InSize, unsigned OutSize, unsigned *pBytesDone) override {return false;}

		virtual ActionStatus Initialize() override;
	};
}
