#include "stdafx.h"
#include <bzscore/string.h>


void KernelModeUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS KernelModeCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS KernelModeDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

bool AutoTestEntry(bool runTestsCausingMemoryLeaks, const TCHAR *pTempDir);

void TestPrint(const char *pFormat, ...)
{
	va_list lst;
	va_start(lst, pFormat);
	BazisLib::DynamicStringA str;
	str.vFormat(pFormat, lst, lst);
	DbgPrint("%s", str.c_str());
	va_end(lst);
}


NTSTATUS CPPDriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath)
{
	UNICODE_STRING DeviceName,Win32Device;
	PDEVICE_OBJECT DeviceObject = NULL;
	NTSTATUS status;
	unsigned i;

	RtlInitUnicodeString(&DeviceName,L"\\Device\\KernelMode0");
	RtlInitUnicodeString(&Win32Device,L"\\DosDevices\\KernelMode0");

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = KernelModeDefaultHandler;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = KernelModeCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = KernelModeCreateClose;
	
	DriverObject->DriverUnload = KernelModeUnload;
	status = IoCreateDevice(DriverObject,
							0,
							&DeviceName,
							FILE_DEVICE_UNKNOWN,
							0,
							FALSE,
							&DeviceObject);
	if (!NT_SUCCESS(status))
		return status;
	if (!DeviceObject)
		return STATUS_UNEXPECTED_IO_ERROR;

	DeviceObject->Flags |= DO_DIRECT_IO;
	DeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;
	status = IoCreateSymbolicLink(&Win32Device, &DeviceName);

	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	if (!AutoTestEntry(false, L"\\??\\C:\\"))
	{
		DbgPrint("Automatic tests failed!\n");
		//DbgBreakPoint();
	}
	else
		DbgPrint("Automatic tests succeeded!\n");

	return STATUS_SUCCESS;
}

void KernelModeUnload(IN PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING Win32Device;
	RtlInitUnicodeString(&Win32Device,L"\\DosDevices\\KernelMode0");
	IoDeleteSymbolicLink(&Win32Device);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS KernelModeCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS KernelModeDefaultHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Irp->IoStatus.Status;
}
