#include "stdafx.h"
#include "filedisk.h"

#include <bzshlp/WinKernel/scsi/VirtualSCSIPDO.h>
#include <bzshlp/WinKernel/scsi/VirtualSCSIBus.h>

#include <bzshlp/WinKernel/device.h>
#include <bzshlp/WinKernel/driver.h>
#include <bzshlp/WinKernel/crossplatform/crossvol.h>

using namespace BazisLib;
using namespace DDK;

class FileDiskPDO : public VirtualSCSIPDO
{
public:
	FileDiskPDO(ManagedPointer<AIBasicDisk> pDisk)
		: VirtualSCSIPDO(ManagedPointer<BazisLib::SCSI::AIBasicSCSIDevice>(new DDK::SCSI::BasicDiskToSCSIDiskAdapter(pDisk)))
	{
	}
};

class FileDiskPDOWithMBR : public VirtualSCSIPDO
{
public:
	FileDiskPDOWithMBR(ManagedPointer<AIBasicDisk> pDisk)
		: VirtualSCSIPDO(new _MBREmulatorT<DDK::SCSI::BasicSCSIBlockDevice>(pDisk))
	{
	}
};


class RamDiskBus : public BusDevice
{
private:
	void AddFileDisk(const wchar_t *pFileName, unsigned minMBCount, bool emulateMBR)
	{
		ManagedPointer<AIFile> pFile = new ACFile(pFileName, FileModes::CreateOrOpenRW);
		if (!pFile || !pFile->Valid())
			return;

		ManagedPointer<AIBasicDisk> pDisk = new FileDisk(pFile, 0, 512, minMBCount);

		if (emulateMBR)
			AddPDO(new FileDiskPDOWithMBR(pDisk));
		else
			AddPDO(new FileDiskPDO(pDisk));
	}

public:
	RamDiskBus(String BusPrefix = L"BazisLibSCSI")
		: BusDevice(BusPrefix, true, FILE_DEVICE_SECURE_OPEN, FALSE, DO_BUFFERED_IO)
	{
	}

	virtual NTSTATUS OnStartDevice(IN PCM_RESOURCE_LIST AllocatedResources, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated) override
	{
		NTSTATUS st = __super::OnStartDevice(AllocatedResources, AllocatedResourcesTranslated);
		if (!NT_SUCCESS(st))
			return st;
		SetNewPNPState(Started);

		AddFileDisk(L"\\??\\C:\\test0.dsk", 128, true);		//This disk will have 1 partition and will immediately get a drive letter
		AddFileDisk(L"\\??\\C:\\test1.dsk", 128, false);	//This disk will need to be initialized in Device Manager

		return STATUS_SUCCESS;
	}

};

class FileDiskBusDriver : public Driver
{
public:
	FileDiskBusDriver() : DDK::Driver(true)
	{
	}

	virtual NTSTATUS AddDevice(IN PDEVICE_OBJECT  PhysicalDeviceObject) override
	{
		RamDiskBus *pDev = new RamDiskBus();
		if (!pDev)
			return STATUS_NO_MEMORY;
		NTSTATUS st = pDev->AddDevice(this, PhysicalDeviceObject);
		if (!NT_SUCCESS(st))
			return st;

		return STATUS_SUCCESS;
	}

	virtual ~FileDiskBusDriver()
	{
	}
};

DDK::Driver *_stdcall CreateMainDriverInstance()
{
	return new FileDiskBusDriver();
}
