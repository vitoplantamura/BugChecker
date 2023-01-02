#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "driver.h"
#include "device.h"

using namespace BazisLib;
using namespace DDK;

static Driver *s_MainDriver = NULL;

NTSTATUS _stdcall CPPDriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    )
{
	s_MainDriver = CreateMainDriverInstance();
	if (!s_MainDriver)
		return STATUS_INTERNAL_ERROR;

	s_MainDriver->m_RegistryPath = RegistryPath;
	s_MainDriver->m_DriverObject = DriverObject;
	return s_MainDriver->DriverLoad(RegistryPath);
}

Driver *Driver::GetMainDriver()
{
	return s_MainDriver;
}

NTSTATUS Driver::DispatchRoutine(IN PDEVICE_OBJECT  DeviceObject,  IN PIRP  Irp, bool bIsPowerIrp)
{
	if (!DeviceObject || !Irp)
		return STATUS_ACCESS_VIOLATION;
	Device::Extension *pExt = (Device::Extension *)DeviceObject->DeviceExtension;
	if (!pExt || (pExt->Signature != Device::Extension::DefaultSignature) || !pExt->pDevice)
		return STATUS_INVALID_DEVICE_STATE;
	return pExt->pDevice->ProcessIRP(Irp, bIsPowerIrp);
}

NTSTATUS Driver::DriverLoad(IN PUNICODE_STRING RegistryPath)
{
	if (!RegistryPath)
		return STATUS_INVALID_PARAMETER;
	m_DriverObject->DriverUnload = sDriverUnload;

	for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		m_DriverObject->MajorFunction[i] = sDispatch;

	m_DriverObject->MajorFunction[IRP_MJ_POWER] = sDispatchPower;

	if (m_bRegisterAddDevice)
		m_DriverObject->DriverExtension->AddDevice = sAddDevice;
	return STATUS_SUCCESS;
}

NTSTATUS Driver::AddDevice(IN PDEVICE_OBJECT  PhysicalDeviceObject)
{
	return STATUS_NOT_SUPPORTED;
}

#pragma region Static helper routines

VOID Driver::sDriverUnload (IN PDRIVER_OBJECT DriverObject)
{
	if (s_MainDriver)
		delete s_MainDriver;
	s_MainDriver = NULL;
}

NTSTATUS Driver::sDispatch(IN PDEVICE_OBJECT  DeviceObject,  IN PIRP  Irp)
{
	if (!s_MainDriver)
		return STATUS_INTERNAL_ERROR;
	return s_MainDriver->DispatchRoutine(DeviceObject, Irp, false);
}

NTSTATUS Driver::sDispatchPower(IN PDEVICE_OBJECT  DeviceObject,  IN PIRP  Irp)
{
	if (!s_MainDriver)
		return STATUS_INTERNAL_ERROR;
	return s_MainDriver->DispatchRoutine(DeviceObject, Irp, true);
}

NTSTATUS Driver::sAddDevice(IN PDRIVER_OBJECT  DriverObject, IN PDEVICE_OBJECT  PhysicalDeviceObject)
{
	if (!s_MainDriver)
		return STATUS_INTERNAL_ERROR;
	return s_MainDriver->AddDevice(PhysicalDeviceObject);
}

#pragma endregion
#endif