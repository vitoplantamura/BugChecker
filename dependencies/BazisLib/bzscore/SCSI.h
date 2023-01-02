#pragma once
#include "cmndef.h"
#include "objmgr.h"
#include "status.h"
#include "buffer.h"

namespace BazisLib
{
	namespace SCSI
	{
		typedef unsigned char SCSI_REQUEST_TYPE;

		enum SCSI_STATUS
		{
			ssGood = 0,
			ssCheckCondition = 2,
			ssConditionMet = 4,
			ssBusy = 8,
			ssIntermediate = 0x10,
			ssIntermediateCondMet = 0x14,
			ssReservationConflict = 0x18,
			ssCommandTerminated = 0x22,
			ssQueueFull = 0x28
		};

		enum DataTransferDirection
		{
			dtNone,
			dtHostToDevice,
			dtDeviceToHost,
		};
		
		template <SCSI_REQUEST_TYPE _Opcode, typename _CommandBlock, typename _BufferType = void> struct _SCSIRequest
		{
			SCSI_REQUEST_TYPE RequestType;
			_BufferType *DataBuffer;
			size_t DataTransferSize;
			_CommandBlock *pCDB;

#ifdef BZSLIB_KEXT
			DataTransferDirection Direction;
#endif

			C_ASSERT(sizeof(_CommandBlock) <= 16);
		};

		template <typename _CommandClass> class _SCSIHelper
		{
		public:
			typedef _SCSIRequest<_CommandClass::OperationCode, typename _CommandClass::CommandBlock, typename _CommandClass::Data> Request;
		};

		struct GenericCDB
		{
			unsigned char Command;
			unsigned char Data[15];
		};

		C_ASSERT(sizeof(GenericCDB) == 16);

		struct GenericSRB : public _SCSIRequest<0, GenericCDB> 
		{
			template <SCSI_REQUEST_TYPE _Opcode, typename _CommandBlock, typename _BufferType> operator _SCSIRequest<_Opcode, _CommandBlock, _BufferType> &()
			{
				return reinterpret_cast<_SCSIRequest<_Opcode, _CommandBlock, _BufferType>&>(*this);
			}
		};

		template<SCSI_REQUEST_TYPE _Opcode, typename _CommandBlock, typename _BufferType = void> struct EmbeddedSRB : public GenericSRB
		{
		public:
			_CommandBlock CDB;
			_BufferType Data;

		public:
			EmbeddedSRB(bool fillWithZeroes = true)
			{
				if (fillWithZeroes)
					memset(this, 0, sizeof(*this));

				RequestType = _Opcode;
				DataBuffer = &Data;
				DataTransferSize = sizeof(Data);
				pCDB = (GenericCDB *)&CDB;
				pCDB->Command = _Opcode;
			}
		};

		struct RequestSenseData
		{
			unsigned char ErrorCode:7;
			unsigned char Valid:1;
			unsigned char SegmentNumber;
			unsigned char SenseKey:4;
			unsigned char Reserved1:1;
			unsigned char ILI:1;
			unsigned char Reserved2:2;
			unsigned char Information[4];
			unsigned char AdditionalSenseLength;
			unsigned char CommandSpecificInformation[4];
			unsigned char AdditionalSenseCode;
			unsigned char AdditionalSenseCodeQualifier;
			unsigned char FieldReplaceableUnitCode;
			unsigned char SenseKeySpecific[3];
		};

		C_ASSERT(sizeof(RequestSenseData) == 18);

		struct InquiryData
		{
			unsigned char PeripheralDeviceType : 5;
			unsigned char PeripheralQualifier : 3;
			unsigned char Reserved1 : 7;
			unsigned char RMB : 1;
			unsigned char VersionByte;
			unsigned char ResponseDataFormat : 4;
			unsigned char TransportSpecific : 4;
			unsigned char AdditionalLength;
			unsigned char CapabilityFlags[3];

			unsigned char VendorID[8];
			unsigned char ProductID[16];
			unsigned char ProductRevisionLevel[4];
			unsigned char VendorSpecific[20];
			unsigned char VersionDescriptors[40];
		};

		C_ASSERT(sizeof(InquiryData) == 96);

		struct SCSIResult
		{
			SCSI_STATUS Status;
			struct Sense
			{
				unsigned Key;
				unsigned ASC, ASCQ;
			} SenseData;

			static SCSIResult Error(unsigned Key, unsigned ASC, unsigned ASCQ)
			{
				SCSIResult result = {ssCheckCondition, {Key, ASC, ASCQ}};
				return result;
			}

			bool Successful()
			{
				return Status == ssGood;
			}

			size_t Export(RequestSenseData *pBuffer, size_t bufferSize)
			{
				if (!pBuffer)
					return 0;

				if (bufferSize < sizeof(RequestSenseData))
				{
					RequestSenseData data;
					Export(&data, sizeof(data));
					memcpy(pBuffer, &data, bufferSize);
					return bufferSize;
				}

				memset(pBuffer, 0, sizeof(RequestSenseData));
				pBuffer->Valid = 1;
				pBuffer->ErrorCode = 0x70;
				pBuffer->SenseKey = SenseData.Key;
				pBuffer->AdditionalSenseCode = (unsigned char)SenseData.ASC;
				pBuffer->AdditionalSenseCodeQualifier = (unsigned char)SenseData.ASCQ;
				pBuffer->AdditionalSenseLength= sizeof(RequestSenseData) - FIELD_OFFSET(RequestSenseData, AdditionalSenseLength);
				return sizeof(RequestSenseData);
			}

			void Reset()
			{
				memset(this, 0, sizeof(*this));
			}
		};

		static const SCSIResult scsiGood = {ssGood, };
		static const SCSIResult scsiCommandTerminated = {ssCommandTerminated, };
		static const SCSIResult scsiInvalidOpcode = {ssCheckCondition, {5, 0x20, 0}};
		static const SCSIResult scsiInvalidField = {ssCheckCondition, {5, 0x24, 0}};
		static const SCSIResult scsiReadRetriesExausted = {ssCheckCondition, {3, 0x11, 1}};
		static const SCSIResult scsiMediumNotPresent = {ssCheckCondition, {2, 0x3A, 0}};
		static const SCSIResult scsiInternalError = {ssCheckCondition, {4, 0x44, 0}};

		C_ASSERT(sizeof(RequestSenseData) == 18);

		class IBasicSCSIDevice
		{
		public:
			virtual SCSIResult ExecuteSCSIRequest(GenericSRB &request)=0;
			virtual ActionStatus Initialize()=0;
			virtual ActionStatus GetInquiryData(TypedBuffer<InquiryData> &pInquiryData)=0;
			virtual void Dispose()=0;
			virtual bool DeviceControl(unsigned CtlCode, void *pBuffer, unsigned InSize, unsigned OutSize, unsigned *pBytesDone)=0;

			virtual bool CanExecuteAsynchronously(const GenericSRB &request)=0;
			virtual bool CanExecuteAsynchronously(unsigned CtlCode)=0;
		};

		MAKE_AUTO_INTERFACE(IBasicSCSIDevice);

		class ACBasicSCSIDevice : public AIBasicSCSIDevice
		{
		public:
			virtual ActionStatus Initialize() override {return MAKE_STATUS(Success);}
			virtual bool CanExecuteAsynchronously(const GenericSRB &request) override {return false;}
			virtual bool CanExecuteAsynchronously(unsigned CtlCode) override {return true;}
			virtual bool DeviceControl(unsigned CtlCode, void *pBuffer, unsigned InSize, unsigned OutSize, unsigned *pBytesDone) {return false;}
			virtual void Dispose() override {}

			virtual SCSIResult ExecuteSCSIRequest(GenericSRB &request) {return scsiInvalidOpcode;}
		};
	}

}