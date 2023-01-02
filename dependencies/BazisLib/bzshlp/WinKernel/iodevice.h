#pragma once

#include "pnpdevice.h"
#include <scsi.h>

namespace BazisLib
{
	namespace DDK
	{
		class IODevice : public PNPDevice
		{
		private:
			bool m_bTreatSystemControlAsSCSIRequests;

		public:
			IODevice(DEVICE_TYPE DeviceType = FILE_DEVICE_UNKNOWN,
					  bool bDeleteThisAfterRemoveRequest = false,
					  ULONG DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN,
					  bool bExclusive = FALSE,
					  ULONG AdditionalDeviceFlags = DO_POWER_PAGABLE,
					  LPCWSTR pwszDeviceName = NULL);

			virtual ~IODevice();
		protected:

			void TreatSystemControlAsSCSIRequests(bool DoTreat = true)
			{
				m_bTreatSystemControlAsSCSIRequests = DoTreat;
			}

			/*! This method is called every time a device receives an IRP. It Device class it handles the
				following codes by calling the corresponding virtual methods:
				    - IRP_MJ_CREATE
				    - IRP_MJ_CLOSE
					- IRP_MJ_READ
					- IRP_MJ_WRITE
					- IRP_MJ_DEVICE_CONTROL 
					- IRP_MJ_INTERNAL_CONTROL 
				If an IRP does not belong to the types listed above, it simply returns the IoStatus.Status value.
				A typical override should have the following structure:
				switch (IrpSp->MajorFunction)
				{
					case IRP_MJ_XXX:
						return DispatchXXX(...);
					case IRP_MJ_YYY:
						return DispatchYYY(...);
					default:
						return __super::DispatchRoutine(...);
				}
				\return  If the return value is STATUS_PENDING, the packet is not touched after the method returns.
						 In other case, the IoStatus.Status field is updated (if it was not set previously) and the
						 packet is sent to the next driver (if one exists).
				\remarks Note that if you inherit any child class of DDK::Driver, you should ensure that if you
						 redefine the DispatchRoutine() in the way preventing some IRPs from being processed by
						 the DispatchRoutine() of your base class, they will really NOT be processed by that class
						 and some corresponding virtual methods will not be called.
						 Please consider that if you want to redefine device behaviour for certain minor codes
						 of a major code handled by parent class, and do not intend to alter other minors, you should
						 call __super::DispatchRoutine() for these minors explicitly.
						 Consider notes on DispatchRoutine() in other classes to see what major codes they handle.
			*/
			virtual NTSTATUS DISPATCH_ROUTINE_DECL DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp) DISPATCH_ROUTINE_OVERRIDE;

			virtual NTSTATUS DispatchCleanup(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp) {return STATUS_SUCCESS;}

		protected:
			/*  The following minihandlers should process the corresponding request and return the status code.
				For any returned code, it will be copied to IoStatus.Status field of IRP packet.
				pBytesDone parameter always points to IoStatus.Information field allowing to modify it if required.
				
				It is guaranteed that this field always contains 0 when the minihandler is called and that the
				pointer is always valid.
			*/
			virtual NTSTATUS OnCreate(PIO_SECURITY_CONTEXT SecurityContext, ULONG Options, USHORT FileAttributes, USHORT ShareAccess, PFILE_OBJECT pFileObj) {return STATUS_SUCCESS;}

			virtual NTSTATUS OnClose() {return STATUS_SUCCESS;}

			/*! This method is called when the driver receives an IRP_MJ_READ packet. And the DO_BUFFERED_IO flag is set
				in the DEVICE_OBJECT::Flags field.
			*/
			virtual NTSTATUS OnRead(void *pBuffer, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone) {return STATUS_INVALID_DEVICE_REQUEST;}

			/*! This method is called when the driver receives an IRP_MJ_READ packet. And the DO_DIRECT_IO flag is set
				in the DEVICE_OBJECT::Flags field.
			*/
			virtual NTSTATUS OnRead(PMDL pMdl, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone) {return STATUS_INVALID_DEVICE_REQUEST;}
			
			/*! This method is called when the driver receives an IRP_MJ_WRITE packet. And the DO_BUFFERED_IO flag is set
				in the DEVICE_OBJECT::Flags field.
			*/
			virtual NTSTATUS OnWrite(const void *pBuffer, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone) {return STATUS_INVALID_DEVICE_REQUEST;}

			/*! This method is called when the driver receives an IRP_MJ_WRITE packet. And the DO_DIRECT_IO flag is set
				in the DEVICE_OBJECT::Flags field.
			*/
			virtual NTSTATUS OnWrite(PMDL pMdl, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone) {return STATUS_INVALID_DEVICE_REQUEST;}

			/*! This method is called when the driver receives an IRP_MJ_DEVICE_CONTROL or
				IRP_MJ_INTERNAL_DEVICE_CONTROL packet with Transfer Type bits set to METHOD_BUFFERED
			*/
			virtual NTSTATUS OnDeviceControl(ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone) {return STATUS_INVALID_DEVICE_REQUEST;}

			/*! This method is called when the driver receives an IRP_MJ_DEVICE_CONTROL or
				IRP_MJ_INTERNAL_DEVICE_CONTROL packet with Transfer Type bits set to METHOD_IN_DIRECT or METHOD_OUT_DIRECT
				\param pInBuffer Contains the address of the system buffer shadowing the input buffer.
				\param InOutBuffer Contains the MDL for the output buffer that can be used for input of output operation
					   depending on the Transfer Type bits in the IOCTL code.
			*/
			virtual NTSTATUS OnDeviceControl(ULONG ControlCode, bool IsInternal, void *pInBuffer, PMDL InOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone) {return STATUS_INVALID_DEVICE_REQUEST;}

			/*! This method is called when the driver receives an IRP_MJ_DEVICE_CONTROL or
				IRP_MJ_INTERNAL_DEVICE_CONTROL packet with Transfer Type bits set to METHOD_NEITHER
				\param pUserInputBuffer Contains the address of the input buffer in USER SPACE!
				\param pUserOutputBuffer Contains the address of the output buffer in USER SPACE!
			*/
			virtual NTSTATUS OnDeviceControlNeither(ULONG ControlCode, bool IsInternal, void *pUserInputBuffer, void *pUserOutputBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone) {return STATUS_INVALID_DEVICE_REQUEST;}

			/*!	 This method is called when the driver receives an IRP_MJ_SCSI request (IRP_MJ_INTERNAL_CONTROL 
				 with one of the IOCTL_SCSI_EXECUTE_xxx control codes.
				 \param ScsiExecuteCode Specifies one of the IOCTL_SCSI_EXECUTE_xxx codes
				 \param pSRB Specifies the SRB
				 \remarks If the request is a IOCTL_SCSI_EXECUTE_IN request, the handler should specify
						  the returned data size in the SCSI_REQUEST_BLOCK::DataTransferLength field of pSRB.
			*/
			virtual NTSTATUS OnScsiExecute(ULONG ScsiExecuteCode, SCSI_REQUEST_BLOCK *pSRB, PVOID pMappedSystemBuffer) {return STATUS_INVALID_DEVICE_REQUEST;}
		};
	}
}
