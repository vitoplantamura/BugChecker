#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "iofilter.h"

using namespace BazisLib::DDK;

IODeviceFilter::IODeviceFilter(LPCWSTR pwszBaseDeviceName,
       bool bDeleteThisAfterRemoveRequest,
	   LPCWSTR pwszFilterDeviceName,
	   DEVICE_TYPE FilterDeviceType,
	   ULONG DeviceCharacteristics,
	   bool bExclusive,
	   ULONG AdditionalDeviceFlags) :
	Filter(pwszBaseDeviceName, bDeleteThisAfterRemoveRequest, pwszFilterDeviceName, FilterDeviceType, DeviceCharacteristics, bExclusive, AdditionalDeviceFlags)
{
}

IODeviceFilter::~IODeviceFilter()
{
}

NTSTATUS IODeviceFilter::DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp)
{
	NTSTATUS status;

	//Note that the IoStatus.Information field should be set to 0 before calling any minihandler.
	switch (IrpSp->MajorFunction)
	{
	case IRP_MJ_CREATE:
		status = FilterCreate(Irp, IrpSp->Parameters.Create.SecurityContext,
						  IrpSp->Parameters.Create.Options,
						  IrpSp->Parameters.Create.FileAttributes,
						  IrpSp->Parameters.Create.ShareAccess,
						  IrpSp->FileObject);
		break;
	case IRP_MJ_CLOSE:
		status = FilterClose(Irp);
		break;
	case IRP_MJ_READ:
		if (GetDeviceObject()->Flags & DO_BUFFERED_IO)
			status = FilterRead(Irp, Irp->GetSystemBuffer(),
						  IrpSp->Parameters.Read.Length,
						  IrpSp->Parameters.Read.Key,
						  IrpSp->Parameters.Read.ByteOffset.QuadPart,
						  &Irp->GetIoStatus()->Information);
		else if (GetDeviceObject()->Flags & DO_DIRECT_IO)
			status = FilterRead(Irp, Irp->GetMdlAddress(),
						  IrpSp->Parameters.Read.Length,
						  IrpSp->Parameters.Read.Key,
						  IrpSp->Parameters.Read.ByteOffset.QuadPart,
						  &Irp->GetIoStatus()->Information);
		else
			status = STATUS_NOT_SUPPORTED;
		break;
	case IRP_MJ_WRITE:
		if (GetDeviceObject()->Flags & DO_BUFFERED_IO)
			status = FilterWrite(Irp, Irp->GetSystemBuffer(),
						  IrpSp->Parameters.Read.Length,
						  IrpSp->Parameters.Read.Key,
						  IrpSp->Parameters.Read.ByteOffset.QuadPart,
						  &Irp->GetIoStatus()->Information);
		else if (GetDeviceObject()->Flags & DO_DIRECT_IO)
			status = FilterWrite(Irp, Irp->GetMdlAddress(),
						  IrpSp->Parameters.Read.Length,
						  IrpSp->Parameters.Read.Key,
						  IrpSp->Parameters.Read.ByteOffset.QuadPart,
						  &Irp->GetIoStatus()->Information);
		else
			status = STATUS_NOT_SUPPORTED;
		break;
	case IRP_MJ_DEVICE_CONTROL:
	case IRP_MJ_INTERNAL_DEVICE_CONTROL:
		switch (METHOD_FROM_CTL_CODE(IrpSp->Parameters.DeviceIoControl.IoControlCode))
		{
		case METHOD_BUFFERED:
			status = FilterDeviceControl(Irp, IrpSp->Parameters.DeviceIoControl.IoControlCode,
									 (IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL),
									 Irp->GetSystemBuffer(),
									 IrpSp->Parameters.DeviceIoControl.InputBufferLength,
									 IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
									 &Irp->GetIoStatus()->Information);
			break;
		case METHOD_IN_DIRECT:
		case METHOD_OUT_DIRECT:
			status = FilterDeviceControl(Irp, IrpSp->Parameters.DeviceIoControl.IoControlCode,
									 (IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL),
									 Irp->GetSystemBuffer(),
									 Irp->GetMdlAddress(),
									 IrpSp->Parameters.DeviceIoControl.InputBufferLength,
									 IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
									 &Irp->GetIoStatus()->Information);
			break;
		case METHOD_NEITHER:
			status = FilterDeviceControl(Irp, IrpSp->Parameters.DeviceIoControl.IoControlCode,
									 (IrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL),
									 IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
									 Irp->GetUserBuffer(),
									 IrpSp->Parameters.DeviceIoControl.InputBufferLength,
									 IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
									 &Irp->GetIoStatus()->Information);
			break;
		}
		break;
	default:
		return __super::DispatchRoutine(Irp, IrpSp);
	}

	if (status == STATUS_NOT_SUPPORTED)
		return Irp->GetStatus();
	
	Irp->SetIoStatus(status);
	Irp->CompleteRequest();
	return status;
}

#endif