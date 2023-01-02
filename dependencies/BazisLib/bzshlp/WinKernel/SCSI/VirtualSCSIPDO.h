#pragma once

#include "BasicBlockDevice.h"
#include "../bus.h"
#include <mountdev.h>
#include <ntddscsi.h>
#include <Bazislib/bzscore/buffer.h>

typedef struct _KAPC_STATE {
	LIST_ENTRY ApcListHead[MaximumMode];
	struct _KPROCESS *Process;
	BOOLEAN KernelApcInProgress;
	BOOLEAN KernelApcPending;
	BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *PRKAPC_STATE;

extern "C" 
{
	NTKERNELAPI VOID KeStackAttachProcess (__inout struct _KPROCESS * PROCESS, __out PRKAPC_STATE ApcState);
	NTKERNELAPI VOID KeUnstackDetachProcess (__in PRKAPC_STATE ApcState);
}

struct _DISK_GEOMETRY;

namespace BazisLib
{
	namespace DDK
	{
		//! Handles SCSI-related IRPs and forwards them to the AIBasicSCSIDevice-based device implementation
		class VirtualSCSIPDO : public BusPDO
		{
		private:
			// WARNING! See the GetRelatedSCSIDevice() method reference
			ManagedPointer<SCSI::AIBasicSCSIDevice> m_pSCSIDevice;
			bool m_bAlreadyInitialized;
			bool m_bUnplugPDOOnEject;

			String m_ReportedDeviceName;
			String m_UniqueID;

			SCSIResult m_PendingSenseData;
			TypedBuffer<InquiryData> m_pCachedInquiryData;
			bool m_bInquiryDataAvailable;

		private:
			SCSIResult DoHandleRequest(SCSI::GenericSRB &request);

		public:
			VirtualSCSIPDO(const ManagedPointer<SCSI::AIBasicSCSIDevice> &pScsiDevice, const wchar_t *pDeviceName = L"Virtual SCSI Device", const wchar_t *pUniqueID = NULL, bool UnplugPDOOnSCSIEject = false)
				: m_pSCSIDevice(pScsiDevice)
				, m_bAlreadyInitialized(false)
				, m_ReportedDeviceName(pDeviceName ? pDeviceName : L"")
				, m_UniqueID(pUniqueID ? pUniqueID : L"")
				, m_bUnplugPDOOnEject(UnplugPDOOnSCSIEject)
				, m_bInquiryDataAvailable(false)
			{
				CreateDeviceRequestQueue();
			}

		public:
			BazisLib::ActionStatus ProvideInquiryData();

			virtual NTSTATUS OnScsiExecute(ULONG ScsiExecuteCode, SCSI_REQUEST_BLOCK *pSRB, PVOID pMappedSystemBuffer) override;
			virtual void GenerateIDStrings(int UniqueDeviceNumber) override;

			bool ShouldUnplugPDOOnEject()
			{
				return m_bUnplugPDOOnEject;
			}
			
			virtual NTSTATUS OnQueryProperty(PSTORAGE_PROPERTY_QUERY pQueryProperty, ULONG InputBufferLength, PSTORAGE_DESCRIPTOR_HEADER pOutput, ULONG BufferLength, PULONG_PTR pBytesDone);
			virtual NTSTATUS OnStartDevice(IN PCM_RESOURCE_LIST AllocatedResources, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated) override;

			virtual NTSTATUS OnQueryCapabilities(PDEVICE_CAPABILITIES Capabilities) override
			{
				NTSTATUS st = __super::OnQueryCapabilities(Capabilities);
				if (!NT_SUCCESS(st) || !Capabilities) 
					return st;
				Capabilities->UniqueID = !m_UniqueID.empty();
				return STATUS_SUCCESS;
			}

			void ShutdownUnderlyingDevice()
			{
				if (m_pSCSIDevice)
					m_pSCSIDevice->Dispose();
			}

			//! Returns the pointer to the associated AIBasicSCSIDevice. As the m_pSCSIDevice is assigned from constructor only, this method is thread-safe.
			ManagedPointer<SCSI::AIBasicSCSIDevice> GetRelatedSCSIDevice()
			{
				return m_pSCSIDevice;
			}

		private:
			NTSTATUS OnGetDriveGeometry(_DISK_GEOMETRY *pGeo);

			template <class _PassThroughType> NTSTATUS OnScsiPassThrough(_PassThroughType *pPassThrough, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone);

		protected:
			virtual NTSTATUS OnDeviceControl(ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone) override;

			virtual NTSTATUS DISPATCH_ROUTINE_DECL DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp) DISPATCH_ROUTINE_OVERRIDE
			{
				switch (IrpSp->MajorFunction)
				{
				case IRP_MJ_PNP:
					if (IrpSp->MinorFunction == IRP_MN_START_DEVICE)
						if (!Irp->IsFromQueue())
							return EnqueuePacket(Irp);
					break;
				case IRP_MJ_SCSI:
					{
						//Classpnp!ClassCheckMediaState() sends a TEST_UNIT_READY IRP on DISPATCH_LEVEL.
						//As we do not want to put IRQL-related restrictions on the cross-platform SCSI
						//emulation code, we simply enqueue all such IRPs.
						if (KeGetCurrentIrql() > PASSIVE_LEVEL)
						{
							ASSERT(!Irp->IsFromQueue());
							if (!Irp->IsFromQueue())
								return EnqueuePacket(Irp);
						}

						PSCSI_REQUEST_BLOCK pSRB = IrpSp->Parameters.Scsi.Srb;
						if (!pSRB)
							break;
						if (pSRB->Function != SRB_FUNCTION_EXECUTE_SCSI)
							break;
						if (!Irp->IsFromQueue() && !IsRunningFromWorkerThread())
						{
							GenericSRB srb;
							srb.RequestType = pSRB->Cdb[0];
							srb.DataBuffer = pSRB->DataBuffer;
							srb.DataTransferSize = pSRB->DataTransferLength;
							srb.pCDB = (GenericCDB *)pSRB->Cdb;
							switch(pSRB->Cdb[0])
							{
							case SCSIOP_INQUIRY:
							case SCSIOP_REQUEST_SENSE:
								break;
							default:
								if (m_pSCSIDevice && !m_pSCSIDevice->CanExecuteAsynchronously(srb))
									return EnqueuePacket(Irp);
								break;
							}
						}
					}
					break;
				case IRP_MJ_DEVICE_CONTROL:
					if  ((IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_EXECUTE_IN) ||
						(IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_EXECUTE_OUT) ||
						(IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_EXECUTE_NONE) ||
						(IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH) ||
						(IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT))
					{
						if (!Irp->IsFromQueue())
							return EnqueuePacket(Irp);
						else
						{
							//Always processed from worker thread
							if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT)
							{
								KAPC_STATE state = {0,};
								KeStackAttachProcess((struct _KPROCESS *)Irp->GetRequestorProcess(), &state);
								NTSTATUS status;
#ifdef _WIN64
								if (Irp->Is32BitProcess())
									status = OnScsiPassThrough((PSCSI_PASS_THROUGH_DIRECT32)Irp->GetSystemBuffer(),
									IrpSp->Parameters.DeviceIoControl.InputBufferLength,
									IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
									&Irp->GetIoStatus()->Information);
								else
#endif
								status = OnScsiPassThrough((PSCSI_PASS_THROUGH_DIRECT)Irp->GetSystemBuffer(),
									IrpSp->Parameters.DeviceIoControl.InputBufferLength,
									IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
									&Irp->GetIoStatus()->Information);

								KeUnstackDetachProcess(&state);

								Irp->SetIoStatus(status);
								Irp->CompleteRequest();
								return status;
							}
							else if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SCSI_PASS_THROUGH)
							{
								NTSTATUS status;
#ifdef _WIN64
								if (Irp->Is32BitProcess())
									status = OnScsiPassThrough((PSCSI_PASS_THROUGH32)Irp->GetSystemBuffer(), 
										IrpSp->Parameters.DeviceIoControl.InputBufferLength,
										IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
										&Irp->GetIoStatus()->Information);
								else
#endif
								status = OnScsiPassThrough((PSCSI_PASS_THROUGH)Irp->GetSystemBuffer(), 
									IrpSp->Parameters.DeviceIoControl.InputBufferLength,
									IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
									&Irp->GetIoStatus()->Information);
								Irp->SetIoStatus(status);
								Irp->CompleteRequest();
								return status;
							}

						}
					}
					else
						if (!Irp->IsFromQueue() && !IsRunningFromWorkerThread())
							if (m_pSCSIDevice && !m_pSCSIDevice->CanExecuteAsynchronously(IrpSp->Parameters.DeviceIoControl.IoControlCode))
								return EnqueuePacket(Irp);

					break;
				}
				return __super::DispatchRoutine(Irp, IrpSp);
			}

		public:
			void *operator new(size_t size)
			{
				return npagednew(size);
			}

		};
	}
}