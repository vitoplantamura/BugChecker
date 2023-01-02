#pragma once

#include "scsidev.h"
#include "../../bzsdisk.h"
#include "../../algorithms/memalgs.h"

#define SCSI_DEVICE_BASIC_HANDLER_DECL	__forceinline

namespace BazisLib
{
	namespace DDK
	{
		namespace SCSI
		{
			using namespace BazisLib::SCSI;

			class BasicSCSIBlockDevice : public ACBasicSCSIDevice, public AIBasicDisk
			{
			private:
				unsigned m_CachedSectorSize;

				struct
				{
					unsigned Key;
					unsigned ASC, ASCQ;
				} m_LastErrorSenseData;

			public:
				BasicSCSIBlockDevice()
					: m_CachedSectorSize(0)
				{
					BazisLib::Algorithms::ZeroStruct(m_LastErrorSenseData);
				}

			protected:
				enum IDStringType
				{
					VendorId,
					ProductId,
				};

				SCSIResult SCSI_DEVICE_BASIC_HANDLER_DECL OnInquiry(INQUIRYDATA *pData, size_t MaxSize);
				SCSIResult SCSI_DEVICE_BASIC_HANDLER_DECL OnReadCapacity(PREAD_CAPACITY_DATA pData, size_t MaxSize);
				//! Handles read/write/verify
				SCSIResult SCSI_DEVICE_BASIC_HANDLER_DECL OnBulkOperation(SCSI_REQUEST_TYPE RequestType, _CDB::_CDB10 *pCdb, void *pBuffer, size_t Size, size_t *pDone);

				SCSIResult SCSI_DEVICE_BASIC_HANDLER_DECL OnModeSense(PMODE_PARAMETER_HEADER pHeader, size_t Size, size_t *pDone);
				SCSIResult SCSI_DEVICE_BASIC_HANDLER_DECL OnRequestSense(PSENSE_DATA pSense, size_t Size);

			public:
				virtual SCSIResult ExecuteSCSIRequest(GenericSRB &request) override;

			public:	//SCSI-specific methods
				virtual UCHAR GetSCSIDeviceType() {return DIRECT_ACCESS_DEVICE;}
				virtual size_t GetIDString(IDStringType IDType, char *pBuf, size_t MaxSize) {return 0;}
				virtual unsigned GetRevisionLevel() {return 1;}	//Should be between 0 and 9999

			public:	//AIBasicDisk methods
				virtual unsigned GetSectorSize()=0;
				virtual ULONGLONG GetSectorCount()=0;

				virtual unsigned Read(ULONGLONG ByteOffset, void *pBuffer, unsigned Length)=0;
				virtual unsigned Write(ULONGLONG ByteOffset, const void *pBuffer, unsigned Length)=0;
				virtual bool IsWritable()=0;

			public:
				virtual void Dispose() {}
				virtual LPCGUID GetStableGuid() {return NULL;}
				virtual bool DeviceControl(unsigned CtlCode, void *pBuffer, unsigned InSize, unsigned OutSize, unsigned *pBytesDone) {return false;}
				virtual ActionStatus GetInquiryData(OUT TypedBuffer<InquiryData> &pInquiryData) override;

				virtual ActionStatus Initialize() override {return MAKE_STATUS(Success);}
			};

			class BasicDiskToSCSIDiskAdapter : public BasicSCSIBlockDevice
			{
			private:
				DECLARE_REFERENCE(AIBasicDisk, m_pBaseDisk);

			public:
				BasicDiskToSCSIDiskAdapter(ManagedPointer<AIBasicDisk> pDisk)
					: INIT_REFERENCE(m_pBaseDisk, pDisk)
				{
				}

			public:
				virtual unsigned GetSectorSize() {return m_pBaseDisk->GetSectorSize();}
				virtual ULONGLONG GetSectorCount() {return m_pBaseDisk->GetSectorCount();}
				virtual void Dispose() {m_pBaseDisk->Dispose();}

				virtual unsigned Read(ULONGLONG ByteOffset, void *pBuffer, unsigned Length) {return m_pBaseDisk->Read(ByteOffset, pBuffer, Length);}
				virtual unsigned Write(ULONGLONG ByteOffset, const void *pBuffer, unsigned Length) {return m_pBaseDisk->Write(ByteOffset, pBuffer, Length);}
				virtual LPCGUID GetStableGuid() {return m_pBaseDisk->GetStableGuid();}

				virtual bool DeviceControl(unsigned CtlCode, void *pBuffer, unsigned InSize, unsigned OutSize, unsigned *pBytesDone) {return m_pBaseDisk->DeviceControl(CtlCode, pBuffer, InSize, OutSize, pBytesDone);}
				virtual ActionStatus Initialize() {return m_pBaseDisk->Initialize();}
				virtual bool IsWritable() {return m_pBaseDisk->IsWritable();}

			};
		}
	}
}
