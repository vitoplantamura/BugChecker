#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "pnpdevice.h"
#include "bus.h"
#include "../../bzscore/WinKernel/ntstatusdbg.h"
#include "trace.h"

//#define TRACK_POWER_IRPS

using namespace BazisLib::DDK;

PNPDevice::PNPDevice(DEVICE_TYPE DeviceType,
		  bool bDeleteThisAfterRemoveRequest,
		  ULONG DeviceCharacteristics,
		  bool bExclusive,
		  ULONG AdditionalDeviceFlags,
		  LPCWSTR pwszDeviceName) :
	Device(DeviceType, pwszDeviceName, DeviceCharacteristics, bExclusive, AdditionalDeviceFlags),

	m_CurrentPNPState(NotStarted),
	m_bDeleteThisAfterRemoveRequest(bDeleteThisAfterRemoveRequest)
{

}

NTSTATUS PNPDevice::AddDevice(Driver *pDriver, PDEVICE_OBJECT PhysicalDeviceObject, const GUID *pInterfaceGuid, const wchar_t *pwszLinkPath)
{
	NTSTATUS st = RegisterDevice(pDriver, false, pwszLinkPath);
	if (!NT_SUCCESS(st))
		return st;
	if (PhysicalDeviceObject)
	{
		st = AttachToDeviceStack(PhysicalDeviceObject);
		if (!NT_SUCCESS(st))
			return st;
	}
	if (pInterfaceGuid)
	{
		st = RegisterInterface(pInterfaceGuid);
		if (!NT_SUCCESS(st))
			return st;
	}
	CompleteInitialization();
	return STATUS_SUCCESS;
}

PNPDevice::~PNPDevice()
{
}

void *PNPDevice::RequestInterfaceFromUnderlyingDevice(LPCGUID lpGuid, unsigned Version, PVOID pSpecificData)
{
	return RequestInterface(GetUnderlyingPDO(), lpGuid, Version, pSpecificData);
}

NTSTATUS PNPDevice::ReportStateChange(const GUID &Event, PVOID pAdditionalData, ULONG AdditionalDataSize)
{
	TARGET_DEVICE_CUSTOM_NOTIFICATION *pNotification = (TARGET_DEVICE_CUSTOM_NOTIFICATION *)ExAllocatePool(NonPagedPool, sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION) + AdditionalDataSize);
	pNotification->Version = 1;
	pNotification->Size = sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION);
	pNotification->FileObject = NULL;
	pNotification->NameBufferOffset = -1;
	pNotification->Event = Event;
	if (pAdditionalData && AdditionalDataSize)
		memcpy(pNotification + 1, pAdditionalData, AdditionalDataSize);
	return IoReportTargetDeviceChangeAsynchronous(GetUnderlyingPDO(), pNotification, ExFreePool, pNotification);
}


void *PNPDevice::RequestInterface(PDEVICE_OBJECT pDevice, LPCGUID lpGuid, unsigned Version, PVOID pSpecificData)
{
	OutgoingIRP irp(IRP_MJ_PNP, IRP_MN_QUERY_INTERFACE, pDevice);
	if (!irp.Valid())
		return NULL;
	PIO_STACK_LOCATION IrpSp = irp.GetNextStackLocation();
	INTERFACE iface;
	memset(&iface, 0, sizeof(INTERFACE));
	iface.Size = sizeof(INTERFACE);

	IrpSp->Parameters.QueryInterface.InterfaceType = lpGuid;
	IrpSp->Parameters.QueryInterface.Size = sizeof(INTERFACE);
	IrpSp->Parameters.QueryInterface.Version = (UCHAR)Version;
	IrpSp->Parameters.QueryInterface.Interface = &iface;
	IrpSp->Parameters.QueryInterface.InterfaceSpecificData = pSpecificData;

	NTSTATUS st = irp.Call();
	if (!NT_SUCCESS(st))
		return NULL;

	return ((IUnknownDeviceInterface *)iface.Context)->QueryInterface(lpGuid, Version, pSpecificData);
}

/* Most of the minor PNP codes are handled in the following way:
	1. The corresponding OnXXX() virtual method is called.
	2. If it returns an error, the IRP is completed with an error.
	3. If the method returned no error, the IRP is passed to the next
	   driver in chain using ForwardSuccessfulPacket() method.
    4. If the method returned STATUS_PENDING, the IRP is marked as
	   pending and STATUS_PENDING is returned to the caller. The driver
	   is then responsible for completing the IRP later, forwarding it
	   (for example, using the ForwardSuccessfulPacket() method) and
	   calling the OnPendingIRPCompleted() method.
*/
NTSTATUS PNPDevice::DispatchPNP(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp)
{
	NTSTATUS status = Irp->GetStatus();
	ASSERT(status != STATUS_PENDING);

	switch (IrpSp->MinorFunction)
	{
	case IRP_MN_START_DEVICE:
		if (GetNextDevice())
			status = CallNextDriverSynchronously(Irp);
		else
			status = STATUS_SUCCESS;
		if (NT_SUCCESS(status))
			status = OnStartDevice(IrpSp->Parameters.StartDevice.AllocatedResources, IrpSp->Parameters.StartDevice.AllocatedResourcesTranslated);
		if (status == STATUS_PENDING)
			return status;

		if (NT_SUCCESS(status))
			SetNewPNPState(Started);
	
		Irp->SetIoStatus(status);
		Irp->CompleteRequest();

		if (NT_SUCCESS(status))
			OnDeviceStarted();

		return status;
	case IRP_MN_QUERY_STOP_DEVICE:
		status = OnQueryStopDevice();
		break;
	case IRP_MN_CANCEL_STOP_DEVICE:
		status = OnCancelStopDevice();
		break;
	case IRP_MN_STOP_DEVICE:
		status = OnStopDevice();
		break;
	case IRP_MN_QUERY_REMOVE_DEVICE:
		status = OnQueryRemoveDevice();
		break;
	case IRP_MN_CANCEL_REMOVE_DEVICE:
		status = OnCancelRemoveDevice();
		break;
	case IRP_MN_SURPRISE_REMOVAL:
		status = OnSurpriseRemoval();
		break;
	case IRP_MN_REMOVE_DEVICE:
		if (GetNextDevice())
			status = CallNextDriverSynchronously(Irp);
		else
			status = STATUS_SUCCESS;
		status = OnRemoveDevice(status);
		Irp->SetIoStatus(status);
		Irp->CompleteRequest();
		return status;
	case IRP_MN_QUERY_DEVICE_RELATIONS:
		status = OnQueryDeviceRelations(IrpSp->Parameters.QueryDeviceRelations.Type, (PDEVICE_RELATIONS *)&Irp->GetIoStatus()->Information);
		break;
	case IRP_MN_QUERY_PNP_DEVICE_STATE:
		Irp->GetIoStatus()->Information = 0;
		status = OnQueryPNPDeviceState((PNP_DEVICE_STATE *)&Irp->GetIoStatus()->Information);
		break;
	case IRP_MN_DEVICE_USAGE_NOTIFICATION:
		status = OnDeviceUsageNotification((IrpSp->Parameters.UsageNotification.InPath != FALSE), IrpSp->Parameters.UsageNotification.Type);
		break;
	}
	if (status == STATUS_PENDING)
	{
		Irp->MarkPending();
		return status;
	}
	//Most PnP IRPs require no status change when they are not supported by the driver. Such IRP should be
	//simply passed down the device stack. In most of the cases, the STATUS_NOT_SUPPORTED is already set for such
	//IRP. Our handler methods return this status if they do not handle such request. In that case we do not
	//alter the Irp->IoStatus.Status field.
	if (status != STATUS_NOT_SUPPORTED)
		Irp->SetIoStatus(status);

	return status;
}

NTSTATUS PNPDevice::DispatchPower(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp)
{
	NTSTATUS status = Irp->GetStatus();
	switch (IrpSp->MinorFunction)
	{
	case IRP_MN_QUERY_POWER:
#ifdef TRACK_POWER_IRPS
		DbgPrint("IRP_MN_QUERY_POWER: Type = %d; State = %d; ShutdownType = %d;\n", IrpSp->Parameters.Power.Type, IrpSp->Parameters.Power.State, IrpSp->Parameters.Power.ShutdownType);
#endif
		status = OnSetPower(IrpSp->Parameters.Power.Type, IrpSp->Parameters.Power.State, IrpSp->Parameters.Power.ShutdownType);
		break;
	case IRP_MN_SET_POWER:
#ifdef TRACK_POWER_IRPS
		DbgPrint("IRP_MN_SET_POWER: Type = %d; State = %d; ShutdownType = %d;\n", IrpSp->Parameters.Power.Type, IrpSp->Parameters.Power.State, IrpSp->Parameters.Power.ShutdownType);
#endif
		status = OnQueryPower(IrpSp->Parameters.Power.Type, IrpSp->Parameters.Power.State, IrpSp->Parameters.Power.ShutdownType);
		break;
	}
	ASSERT(status != STATUS_PENDING);
	Irp->SetIoStatus(status);
	return status;
}


NTSTATUS PNPDevice::DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp)
{
	switch (IrpSp->MajorFunction)
	{
	case IRP_MJ_PNP:
		return DispatchPNP(Irp, IrpSp);
	case IRP_MJ_POWER:
		return DispatchPower(Irp, IrpSp);
	default:
		return __super::DispatchRoutine(Irp, IrpSp);
	}
}

NTSTATUS PNPDevice::OnStartDevice(IN PCM_RESOURCE_LIST AllocatedResources, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated)
{
	EnableInterface();
	DEBUG_TRACE(TRACE_PNP_DEVICE_LIFECYCLE, ("PNPDevice: device state changed: Started\n"));
	return STATUS_SUCCESS;
}

NTSTATUS PNPDevice::OnSurpriseRemoval()
{
	SetNewPNPState(SurpriseRemovePending);
	DisableInterface();
	DEBUG_TRACE(TRACE_PNP_DEVICE_LIFECYCLE, ("PNPDevice: device state changed: SurpriseRemovePending\n"));
	return STATUS_SUCCESS;
}

NTSTATUS PNPDevice::OnRemoveDevice(NTSTATUS LowerDeviceRemovalStatus)
{
	DEBUG_TRACE(TRACE_PNP_DEVICE_DELETION, ("PNPDevice::OnRemoveDevice()\n"));
	DEBUG_TRACE(TRACE_PNP_DEVICE_LIFECYCLE, ("PNPDevice: device state changed: Removing\n"));
	SetNewPNPState(Deleted);
	DisableInterface();
	NTSTATUS st = DeleteDevice(true);
	if (!NT_SUCCESS(st))
	{
		DEBUG_TRACE(TRACE_PNP_DEVICE_DELETION, ("PNPDevice::OnRemoveDevice(): call to DeleteDevice() failed with status %wS\n", MapNTStatus(st)));
		return st;
	}
	if (m_bDeleteThisAfterRemoveRequest)
		DeleteThisAfterLastRequest();
	DEBUG_TRACE(TRACE_PNP_DEVICE_DELETION, ("PNPDevice::OnRemoveDevice() completed with status %wS\n", MapNTStatus(LowerDeviceRemovalStatus)));
	return LowerDeviceRemovalStatus;
}

NTSTATUS PNPDevice::OnCancelStopDevice()
{
	DEBUG_TRACE(TRACE_PNP_DEVICE_LIFECYCLE, ("PNPDevice: device stop cancelled\n"));
	if (m_CurrentPNPState == StopPending)
	{
		RestorePreviousPNPState();
		return STATUS_SUCCESS;
	}
	else
		return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS PNPDevice::OnQueryStopDevice()
{
	DEBUG_TRACE(TRACE_PNP_DEVICE_LIFECYCLE, ("PNPDevice: device state changed: StopPending\n"));
	SetNewPNPState(StopPending);
	return STATUS_SUCCESS;
}

NTSTATUS PNPDevice::OnStopDevice()
{
	DEBUG_TRACE(TRACE_PNP_DEVICE_STOPPING, ("PNPDevice: waiting for stop event...\n"));
	NTSTATUS status = WaitForStopEvent(true);
	DEBUG_TRACE(TRACE_PNP_DEVICE_STOPPING, ("PNPDevice: stop event wait done with status %wS...\n", MapNTStatus(status)));
	if (!NT_SUCCESS(status))
		return status;

	SetNewPNPState(Stopped);
	DEBUG_TRACE(TRACE_PNP_DEVICE_LIFECYCLE, ("PNPDevice: device state changed: Stopped\n"));
	return STATUS_SUCCESS;
}

NTSTATUS PNPDevice::OnQueryRemoveDevice()
{
	DEBUG_TRACE(TRACE_PNP_DEVICE_LIFECYCLE, ("PNPDevice: device state changed: RemovePending\n"));
	SetNewPNPState(RemovePending);
	return STATUS_SUCCESS;
}

NTSTATUS PNPDevice::OnCancelRemoveDevice()
{
	DEBUG_TRACE(TRACE_PNP_DEVICE_LIFECYCLE, ("PNPDevice: device remove cancelled\n"));
	if (m_CurrentPNPState == RemovePending)
	{
		RestorePreviousPNPState();
		return STATUS_SUCCESS;
	}
	else
		return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS PNPDevice::OnQueryDeviceRelations(DEVICE_RELATION_TYPE Type, PDEVICE_RELATIONS *ppDeviceRelations)
{
	return STATUS_NOT_SUPPORTED;
}
#endif