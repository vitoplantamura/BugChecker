#pragma once

#include "../../bzsdisk.h"
#include "../volume.h"

namespace BazisLib
{
	namespace DDK
	{
		class UniversalVolume : public BasicStorageVolume
		{
		private:
			ManagedPointer<AIBasicDisk> m_pDisk;
			char m_PartitionType;

		public:
			UniversalVolume(const ConstManagedPointer <AIBasicDisk> &pDisk, char PartitionType = PARTITION_FAT32);
			~UniversalVolume();

		protected:

			virtual unsigned GetSectorSize() override;
			virtual ULONGLONG GetTotalSize() override;
			virtual char GetPartitionType() override;

			virtual NTSTATUS Read(ULONGLONG ByteOffset, PVOID pBuffer, ULONG Length, PULONG_PTR pBytesDone) override;
			virtual NTSTATUS Write(ULONGLONG ByteOffset, PVOID pBuffer, ULONG Length, PULONG_PTR pBytesDone) override;
			virtual NTSTATUS OnGetStableGuid(LPGUID lpGuid) override;

			virtual NTSTATUS OnDeviceControl(ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone) override;
			virtual NTSTATUS OnStartDevice(IN PCM_RESOURCE_LIST AllocatedResources, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated) override;

			virtual NTSTATUS OnDiskIsWritable() override;
			

/*			virtual NTSTATUS DispatchPNP(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp) override
			{
				DbgPrint("CrossVol:   %s\n", PnPMinorFunctionString(IrpSp->MinorFunction));
				return __super::DispatchPNP(Irp, IrpSp);
			}*/
		};
	}
}