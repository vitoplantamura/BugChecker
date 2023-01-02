#include "stdafx.h"	
#ifdef BZSLIB_WINKERNEL
#include "iodevice.h"

using namespace BazisLib::DDK;

IODevice::IODevice(DEVICE_TYPE DeviceType,
		  bool bDeleteThisAfterRemoveRequest,
		  ULONG DeviceCharacteristics,
		  bool bExclusive,
		  ULONG AdditionalDeviceFlags,
		  LPCWSTR pwszDeviceName)
	: PNPDevice(DeviceType, bDeleteThisAfterRemoveRequest, DeviceCharacteristics, bExclusive, AdditionalDeviceFlags, pwszDeviceName)
	, m_bTreatSystemControlAsSCSIRequests(false)
	
{

}

IODevice::~IODevice()
{
}

NTSTATUS IODevice::DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp)
{
	//DbgPrint("<<<%s: IRP(0x%X;0x%X)>>>\n", m_pszDeviceDebugName ? m_pszDeviceDebugName : "(unknown)", IrpSp->MajorFunction, IrpSp->MinorFunction);
	NTSTATUS status;

	//Note that the IoStatus.Information field should be set to 0 before calling any minihandler.
	switch (IrpSp->MajorFunction)
	{
	case IRP_MJ_CREATE:
		Irp->GetIoStatus()->Information = 0;
		status = OnCreate(IrpSp->Parameters.Create.SecurityContext,
						  IrpSp->Parameters.Create.Options,
						  IrpSp->Parameters.Create.FileAttributes,
						  IrpSp->Parameters.Create.ShareAccess,
						  IrpSp->FileObject);
		break;
	case IRP_MJ_CLOSE:
		Irp->GetIoStatus()->Information = 0;
		status = OnClose();
		break;
	case IRP_MJ_READ:
		Irp->GetIoStatus()->Information = 0;
		if (GetDeviceObject()->Flags & DO_BUFFERED_IO)
			status = OnRead(Irp->GetSystemBuffer(),
						  IrpSp->Parameters.Read.Length,
						  IrpSp->Parameters.Read.Key,
						  IrpSp->Parameters.Read.ByteOffset.QuadPart,
						  &Irp->GetIoStatus()->Information);
		else if (GetDeviceObject()->Flags & DO_DIRECT_IO)
			status = OnRead(Irp->GetMdlAddress(),
						  IrpSp->Parameters.Read.Length,
						  IrpSp->Parameters.Read.Key,
						  IrpSp->Parameters.Read.ByteOffset.QuadPart,
						  &Irp->GetIoStatus()->Information);
		else
			status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	case IRP_MJ_WRITE:
		Irp->GetIoStatus()->Information = 0;
		if (GetDeviceObject()->Flags & DO_BUFFERED_IO)
			status = OnWrite(Irp->GetSystemBuffer(),
						  IrpSp->Parameters.Read.Length,
						  IrpSp->Parameters.Read.Key,
						  IrpSp->Parameters.Read.ByteOffset.QuadPart,
						  &Irp->GetIoStatus()->Information);
		else if (GetDeviceObject()->Flags & DO_DIRECT_IO)
			status = OnWrite(Irp->GetMdlAddress(),
						  IrpSp->Parameters.Read.Length,
						  IrpSp->Parameters.Read.Key,
						  IrpSp->Parameters.Read.ByteOffset.QuadPart,
						  &Irp->GetIoStatus()->Information);
		else
			status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	case IRP_MJ_DEVICE_CONTROL:
	case IRP_MJ_INTERNAL_DEVICE_CONTROL:
		Irp->GetIoStatus()->Information = 0;
		if (((IrpSp->MajorFunction == IRP_MJ_SCSI) && (m_bTreatSystemControlAsSCSIRequests)) ||
					(IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_EXECUTE_IN) ||
					(IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_EXECUTE_OUT) ||
					(IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_EXECUTE_NONE))
		{
			PVOID pMappedSystemBuffer = NULL;
			PMDL pMdl = Irp->GetMdlAddress();
			if (pMdl)
				pMappedSystemBuffer = MmGetSystemAddressForMdlSafe(pMdl, HighPagePriority);
			if (!pMappedSystemBuffer)
				pMappedSystemBuffer = IrpSp->Parameters.Scsi.Srb->DataBuffer;
			status = OnScsiExecute(IrpSp->Parameters.DeviceIoControl.IoControlCode, IrpSp->Parameters.Scsi.Srb, pMappedSystemBuffer);
			if (NT_SUCCESS(status))
				Irp->GetIoStatus()->Information = IrpSp->Parameters.Scsi.Srb->DataTransferLength;
		}
		else
		{
			switch (METHOD_FROM_CTL_CODE(IrpSp->Parameters.DeviceIoControl.IoControlCode))
			{
			case METHOD_BUFFERED:
				status = OnDeviceControl(IrpSp->Parameters.DeviceIoControl.IoControlCode,
					(IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL),
					Irp->GetSystemBuffer(),
					IrpSp->Parameters.DeviceIoControl.InputBufferLength,
					IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
					&Irp->GetIoStatus()->Information);
				break;
			case METHOD_IN_DIRECT:
			case METHOD_OUT_DIRECT:
				status = OnDeviceControl(IrpSp->Parameters.DeviceIoControl.IoControlCode,
					(IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL),
					Irp->GetSystemBuffer(),
					Irp->GetMdlAddress(),
					IrpSp->Parameters.DeviceIoControl.InputBufferLength,
					IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
					&Irp->GetIoStatus()->Information);
				break;
			case METHOD_NEITHER:
				status = OnDeviceControlNeither(IrpSp->Parameters.DeviceIoControl.IoControlCode,
					(IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL),
					IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
					Irp->GetUserBuffer(),
					IrpSp->Parameters.DeviceIoControl.InputBufferLength,
					IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
					&Irp->GetIoStatus()->Information);
				break;
			}		
		}
		break;
	case IRP_MJ_CLEANUP:
		return DispatchCleanup(Irp, IrpSp);
	default:
		return __super::DispatchRoutine(Irp, IrpSp);
	}
	Irp->SetIoStatus(status);
	Irp->CompleteRequest();
	return status;
}
#endif