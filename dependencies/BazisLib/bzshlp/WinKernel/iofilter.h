#pragma once

#include "filter.h"
#include "iodevice.h"

namespace BazisLib
{
	namespace DDK
	{
		class IODeviceFilter : public Filter
		{
		public:
			IODeviceFilter(LPCWSTR pwszBaseDeviceName,
			       bool bDeleteThisAfterRemoveRequest = false,
				   LPCWSTR pwszFilterDeviceName = NULL,
				   DEVICE_TYPE FilterDeviceType = FILE_DEVICE_UNKNOWN,
				   ULONG DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN,
				   bool bExclusive = FALSE,
				   ULONG AdditionalDeviceFlags = DO_POWER_PAGABLE);

			~IODeviceFilter();

		protected:
			virtual NTSTATUS DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp)  override;

		protected:
			//See minihandler/minifilter definition
			virtual NTSTATUS FilterCreate(IncomingIrp *pIrp, PIO_SECURITY_CONTEXT SecurityContext, ULONG Options, USHORT FileAttributes, USHORT ShareAccess, PFILE_OBJECT pFileObj) {return STATUS_NOT_SUPPORTED;}

			virtual NTSTATUS FilterClose(IncomingIrp *pIrp) {return STATUS_NOT_SUPPORTED;}

			/*! This method is called when the driver receives an IRP_MJ_READ packet. And the DO_BUFFERED_IO flag is set
				in the DEVICE_OBJECT::Flags field.
			*/
			virtual NTSTATUS FilterRead(IncomingIrp *pIrp, void *pBuffer, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone) {return STATUS_NOT_SUPPORTED;}

			/*! This method is called when the driver receives an IRP_MJ_READ packet. And the DO_DIRECT_IO flag is set
				in the DEVICE_OBJECT::Flags field.
			*/
			virtual NTSTATUS FilterRead(IncomingIrp *pIrp, PMDL pMdl, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone) {return STATUS_NOT_SUPPORTED;}
			
			/*! This method is called when the driver receives an IRP_MJ_WRITE packet. And the DO_BUFFERED_IO flag is set
				in the DEVICE_OBJECT::Flags field.
			*/
			virtual NTSTATUS FilterWrite(IncomingIrp *pIrp, const void *pBuffer, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone) {return STATUS_NOT_SUPPORTED;}

			/*! This method is called when the driver receives an IRP_MJ_WRITE packet. And the DO_DIRECT_IO flag is set
				in the DEVICE_OBJECT::Flags field.
			*/
			virtual NTSTATUS FilterWrite(IncomingIrp *pIrp, PMDL pMdl, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone) {return STATUS_NOT_SUPPORTED;}

			/*! This method is called when the driver receives an IRP_MJ_DEVICE_CONTROL or
				IRP_MJ_INTERNAL_DEVICE_CONTROL packet with Transfer Type bits set to METHOD_BUFFERED
			*/
			virtual NTSTATUS FilterDeviceControl(IncomingIrp *pIrp, ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone) {return STATUS_NOT_SUPPORTED;}

			/*! This method is called when the driver receives an IRP_MJ_DEVICE_CONTROL or
				IRP_MJ_INTERNAL_DEVICE_CONTROL packet with Transfer Type bits set to METHOD_IN_DIRECT or METHOD_OUT_DIRECT
				\param pInBuffer Contains the address of the system buffer shadowing the input buffer.
				\param InOutBuffer Contains the MDL for the output buffer that can be used for input of output operation
					   depending on the Transfer Type bits in the IOCTL code.
			*/
			virtual NTSTATUS FilterDeviceControl(IncomingIrp *pIrp, ULONG ControlCode, bool IsInternal, void *pInBuffer, PMDL InOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone) {return STATUS_NOT_SUPPORTED;}

			/*! This method is called when the driver receives an IRP_MJ_DEVICE_CONTROL or
				IRP_MJ_INTERNAL_DEVICE_CONTROL packet with Transfer Type bits set to METHOD_NEITHER
				\param pUserInputBuffer Contains the address of the input buffer in USER SPACE!
				\param pUserOutputBuffer Contains the address of the output buffer in USER SPACE!
			*/
			virtual NTSTATUS FilterDeviceControl(IncomingIrp *pIrp, ULONG ControlCode, bool IsInternal, void *pUserInputBuffer, void *pUserOutputBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone) {return STATUS_NOT_SUPPORTED;}
		};
	}
}