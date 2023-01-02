#include "stdafx.h"
#include "ramdisk.h"

#include <bzshlp/WinKernel/scsi/VirtualSCSIPDO.h>
#include <bzshlp/WinKernel/scsi/VirtualSCSIBus.h>

#include <bzshlp/WinKernel/device.h>
#include <bzshlp/WinKernel/driver.h>
#include <bzshlp/WinKernel/crossplatform/crossvol.h>

using namespace BazisLib;
using namespace DDK;

class RAMDiskPDO : public VirtualSCSIPDO
{
public:
	RAMDiskPDO(unsigned MegabyteCount)
		: VirtualSCSIPDO(ManagedPointer<BazisLib::SCSI::AIBasicSCSIDevice>(new DDK::SCSI::BasicDiskToSCSIDiskAdapter(
		new RamDisk(MegabyteCount))))
	{
	}
};


class RAMDiskPDOWithMBR : public VirtualSCSIPDO
{
public:
	RAMDiskPDOWithMBR(unsigned MegabyteCount)
		: VirtualSCSIPDO(new _MBREmulatorT<DDK::SCSI::BasicSCSIBlockDevice>(
		new RamDisk(MegabyteCount)))
	{
	}
};


class RamDiskBus : public BusDevice
{
private:
	void AddRamdisk(unsigned MegabyteCount, bool emulateMBR)
	{
		if (emulateMBR)
			AddPDO(new RAMDiskPDOWithMBR(MegabyteCount));
		else
			AddPDO(new RAMDiskPDO(MegabyteCount));
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

		//Just to demonstrate our bus, let's add 2 RAMDISK devices of different sizes
		AddRamdisk(32768, true);	//This disk will have 1 partition and will immediately get a drive letter
		//AddRamdisk(32, false);	//This disk will appear in Disk Management (under Computer Management) as an uninitialized disk

		return STATUS_SUCCESS;
	}

};

class MyVolume : public BasicStorageVolume
{
	RamDisk *m_pDisk = new RamDisk(32768);

	virtual ULONGLONG GetTotalSize() override
	{
		return m_pDisk->GetSectorCount() * m_pDisk->GetSectorSize();
	}

	virtual NTSTATUS Read(ULONGLONG ByteOffset, PVOID pBuffer, ULONG Length, PULONG_PTR pBytesDone) override
	{
		*pBytesDone = m_pDisk->Read(ByteOffset, pBuffer, Length);
		return STATUS_SUCCESS;
	}

	virtual NTSTATUS Write(ULONGLONG ByteOffset, PVOID pBuffer, ULONG Length, PULONG_PTR pBytesDone) override
	{
		*pBytesDone = m_pDisk->Write(ByteOffset, pBuffer, Length);
		return STATUS_SUCCESS;
	}

	~MyVolume()
	{
		m_pDisk->Release();
	}
};

class DummyPDO : public BusPDO
{
};

class RamDiskBusDriver : public Driver
{
public:
	RamDiskBusDriver() : DDK::Driver(true)
	{
	}

	virtual NTSTATUS AddDevice(IN PDEVICE_OBJECT  PhysicalDeviceObject) override
	{
		/*RamDiskBus *pDev = new RamDiskBus();
		if (!pDev)
			return STATUS_NO_MEMORY;
		NTSTATUS st = pDev->AddDevice(this, PhysicalDeviceObject);
		if (!NT_SUCCESS(st))
			return st;
			
		return STATUS_SUCCESS;*/

		DummyPDO *pVol = new DummyPDO();
		return pVol->AddDevice(this, PhysicalDeviceObject);
	}

	virtual ~RamDiskBusDriver()
	{
	}
};

DDK::Driver *_stdcall CreateMainDriverInstance()
{
	return new RamDiskBusDriver();
}
