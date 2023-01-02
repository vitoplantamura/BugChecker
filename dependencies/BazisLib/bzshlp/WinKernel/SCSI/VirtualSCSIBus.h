#pragma once

#include "BasicBlockDevice.h"
#include "../bus.h"
#include "VirtualSCSIPDO.h"
#include <bzshlp/MBREmulator.h>

namespace BazisLib
{
	namespace DDK
	{
		class VirtualSCSIBus : public BusDevice
		{
		private:
			struct ConnectedDiskRecord
			{
				ManagedPointer<AIBasicDisk> pDisk;
				bool EmulateMBR;
			};

			ConnectedDiskRecord m_Disk;

		public:
			VirtualSCSIBus(ManagedPointer<AIBasicDisk> pDisk, bool EmulateMBR = true, String BusPrefix = L"BazisLibSCSI")
				: BusDevice(BusPrefix, false, FILE_DEVICE_SECURE_OPEN, FALSE, DO_BUFFERED_IO)
			{ 
				m_Disk.pDisk = pDisk;
				m_Disk.EmulateMBR = EmulateMBR;
			}

			virtual NTSTATUS OnStartDevice(IN PCM_RESOURCE_LIST AllocatedResources, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated) override
			{
				NTSTATUS st = __super::OnStartDevice(AllocatedResources, AllocatedResourcesTranslated);
				if (!NT_SUCCESS(st))
					return st;
				SetNewPNPState(Started);

				if (m_Disk.EmulateMBR)
					AddPDO(new VirtualSCSIPDO(new _MBREmulatorT<DDK::SCSI::BasicSCSIBlockDevice>(m_Disk.pDisk)));
				else
					AddPDO(new VirtualSCSIPDO(new SCSI::BasicDiskToSCSIDiskAdapter(m_Disk.pDisk)));

				return STATUS_SUCCESS;
			}

		};
	}
}
