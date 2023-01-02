#pragma once
#include "cmndef.h"
#include "bzsdisk.h"

namespace BazisLib
{
	template <class _Base = AIBasicDisk> class _MBREmulatorT : public _Base, AUTO_OBJECT
	{
	private:
		DECLARE_REFERENCE(AIBasicDisk, m_pBaseDisk);
		unsigned char m_MBRSector[512];
		unsigned m_PartitionByteOffset;

		static const unsigned FirstPartitionSector = 0x3F;
		static const unsigned DiskIDOffsetInMBR = 0x1B8;

#pragma pack(push, 1)
		struct PARTITION_RECORD
		{
			unsigned char Active;
			unsigned char StartH;
			unsigned short StartCS;
			unsigned char Type;
			unsigned char EndH;
			unsigned short EndCS;
			unsigned int StartingLBA;
			unsigned int TotalSectorCount;
		};
#pragma pack(pop)

	public:
		_MBREmulatorT(ManagedPointer<AIBasicDisk> pBaseDisk)
			: INIT_REFERENCE(m_pBaseDisk, pBaseDisk)
		{
			memset(m_MBRSector, 0, sizeof(m_MBRSector));
		}

		virtual unsigned GetSectorSize()
		{
			return m_pBaseDisk->GetSectorSize();
		}

		virtual ULONGLONG GetSectorCount()
		{
			return m_pBaseDisk->GetSectorCount() + FirstPartitionSector;
		}
		
		virtual void Dispose()
		{
			return m_pBaseDisk->Dispose();
		}

		virtual unsigned Read(ULONGLONG ByteOffset, void *pBuffer, unsigned Length)
		{
			if (ByteOffset >= m_PartitionByteOffset)
				return m_pBaseDisk->Read(ByteOffset - m_PartitionByteOffset, pBuffer, Length);
			unsigned sectorSize = m_pBaseDisk->GetSectorSize();
			for (unsigned i = 0; i < Length; i += sectorSize, ByteOffset += sectorSize)
			{
				if (!ByteOffset)
					memcpy(((char *)pBuffer) + i, m_MBRSector, min(sizeof(m_MBRSector), sectorSize));
				else if (ByteOffset < m_PartitionByteOffset)
					memset(((char *)pBuffer) + i, 0, sectorSize);
				else
					return i + m_pBaseDisk->Read(ByteOffset, ((char *)pBuffer) + i, Length - i);
			}
			return Length;
		}

		virtual unsigned Write(ULONGLONG ByteOffset, const void *pBuffer, unsigned Length)
		{
			if (ByteOffset >= m_PartitionByteOffset)
				return m_pBaseDisk->Write(ByteOffset - m_PartitionByteOffset, pBuffer, Length);
			/*if (!ByteOffset)
			{
				__asm int 3;
				memcpy(m_MBRSector, pBuffer, sizeof(m_MBRSector));
			}*/
			if (!ByteOffset && (Length == sizeof(m_MBRSector)))
			{
				//Do not fail, if the first sector is overwritten with the same contents
				if (!memcmp(pBuffer, m_MBRSector, sizeof(m_MBRSector)))
					return Length;
			}
#ifdef _DEBUG
			unsigned sectorSize = m_pBaseDisk->GetSectorSize();
			DbgPrint("MBR Emulator: failing attempt to rewrite %d sectors starting from %d (0x%x)\r\n", Length / sectorSize, (int)(ByteOffset / sectorSize), (int)(ByteOffset / sectorSize));
#endif
			return 0;	//Do not allow to rewrite partition table
		}

		virtual LPCGUID GetStableGuid()
		{
			return m_pBaseDisk->GetStableGuid();
		}

		virtual bool DeviceControl(unsigned CtlCode, void *pBuffer, unsigned InSize, unsigned OutSize, unsigned *pBytesDone)
		{
			return m_pBaseDisk->DeviceControl(CtlCode, pBuffer, InSize, OutSize, pBytesDone);
		}

		virtual ActionStatus Initialize()
		{
			ActionStatus st = m_pBaseDisk->Initialize();
			if (!st.Successful())
				return st;
			m_MBRSector[sizeof(m_MBRSector) - 2] = 0x55;
			m_MBRSector[sizeof(m_MBRSector) - 1] = 0xAA;

			PARTITION_RECORD *pFirstPartition = (PARTITION_RECORD *)&m_MBRSector[0x1BE];
			pFirstPartition->Active = 0;
			pFirstPartition->Type = 0x07;
			pFirstPartition->StartingLBA = FirstPartitionSector;
			pFirstPartition->TotalSectorCount = (unsigned)m_pBaseDisk->GetSectorCount();

			m_PartitionByteOffset = m_pBaseDisk->GetSectorSize() * FirstPartitionSector;

			const GUID *pGUID = m_pBaseDisk->GetStableGuid();
			unsigned signature = 0;
			if (pGUID)
			{
				for (int i = 0; i < (sizeof(GUID) / sizeof(unsigned)); i++)
					signature ^= ((unsigned *)pGUID)[i];
			}
			else
			{
				DateTime dt = DateTime::Now();
				signature = dt.GetLargeInteger()->LowPart ^ dt.GetLargeInteger()->HighPart;
			}

			memcpy(m_MBRSector + DiskIDOffsetInMBR, &signature, sizeof(signature));

			C_ASSERT(sizeof(PARTITION_RECORD) == 16);
			ASSERT(m_pBaseDisk);
			return MAKE_STATUS(Success);
		}

		virtual bool IsWritable()
		{
			return m_pBaseDisk->IsWritable();
		}

		virtual ~_MBREmulatorT() {}
	};

	typedef _MBREmulatorT<> MBREmulator;

}