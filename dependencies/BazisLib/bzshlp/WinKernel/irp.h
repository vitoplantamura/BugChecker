#pragma once

#include "../../bzscore/sync.h"

extern "C" 
{
	NTKERNELAPI PEPROCESS IoGetRequestorProcess( __in PIRP Irp);
	NTKERNELAPI BOOLEAN IoIs32bitProcess(IN PIRP  Irp  OPTIONAL);
}

namespace BazisLib
{
	namespace DDK
	{
		//! PowerIRP-aware class that encapsulates an IRP. No extra action is required to process IRP_MJ_POWER requests.
		/*! A typical driver uses at least 4 different strategies for processing IRPs. We'll overview them here:
				\li Call lower driver synchronously, perform some actions, and complete the IRP.
				\li Complete IRP by itself without calling the lower driver.
				\li Handle IRP (such as most PnP IRPs), set IoStatus.Status and pass the IRP to the lower driver
				   if one exists using IoSkipCurrentIrpStackLocation() and IoCallDriver().
			    \li Mark the IRP as pending, put it into IRP queue, unqueue it later and complete it
				   afterwards.

		    The first three strategies can be simply implemented using OO approach: an overridable handler (able to
			call handlers of inherited classes) processes the IRP (performs synchronous lower driver calls if required),
			and decides what to do after the handling routine completes: to complete the IRP, or to pass it to the
			next driver.

			The fourth strategy can also be simply implemented using IrpQueue class. See "Queueing IRPs" section below.

			The IncomingIRP class allows to implement such strategies smoothly:
				\li The most simple situation: the driver does not want to process this IRP, it returns some status.
					In that case the framework sees that the IRP was untouched. So, it sets the IoStatus.Status field
					to the status returned by driver and passes the IRP to the next driver if one exists. If not, it
					completes the IRP.
				\li A little bit more complex: the driver alters the IoStatus field. Then the framework sees the IoStatusSet
				   flag and does not alter the IoStatus. This allows returning one status, and setting another one in IRP.
			    \li Just another little bit of complexity: the driver completes the IRP by calling CompleteRequest() method.
				   The framework sees the IrpCompleted flag and does not call the next driver, or touch the IRP anyhow.
			    \li The complex one: the driver calls the lower driver synchronously. The framework sees the LowerDriverCalled
				   flag and also does not attempt to call lower driver. It completes the IRP if required.
			    \li The most complex one: the driver marks the IRP as pending. The framework does not touch the IRP
				   and returns the STATUS_PENDING result code. The driver completes the IRP later.

		    Please avoid using thr m_pIrp field directly, because it may make the m_Flags field outdated. Consider
			adding inline methods instead. Do not forget to set the LowerDriverCalled flag if the lower driver gets
			called using IoCallDriver() call.

			The layout of the IncomingIRP class produces minimal overhead (4 extra DWORDs are allocated in stack per each
			IRP) and allows maintaining code readability.

			<h2>Queueing IRPs</h2>
			The BazisLib::DDK framework provides a smooth way for managing IRP queues. You should take 3 simple steps
			to convert a synchronous driver to queue-based one:
				\li	In the constructor of your device class add the following line:
				<PRE>
	CreateDeviceRequestQueue();
				</PRE>
					This method creates a device IRP queue and starts dispatcher thread that dequeues and processes IRPs.
				\li  Override the DispatchRoutine() method and use the following layout to implement it:
				<PRE>
	NTSTATUS YourDeviceClass::DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp)
	{
		switch (IrpSp->MajorFunction)
		{
		case IRP_MJ_XXX:
		case IRP_MJ_YYY:
			if (!Irp->IsFromQueue())
				return EnqueuePacket(Irp);
			break;
		}
		return __super::DispatchRoutine(Irp, IrpSp);
	}
				</PRE>
					This ensures that all IRP_MJ_XXX and IRP_MJ_YYY IRPs will be put into queue and dispatched only
					in the context of the dispatcher thread.
				\li  Take into consideration that the minihandlers for queued major codes will now be called only from
					the context of the dispatcher thread. This means that only one instance of such method will be active
					at a single time, and that you can safely remove synchronization objects that were used in synchronous
					driver version to prevent simultanious resource access from those minihandlers.
		*/
		class IncomingIrp
		{
		private:
			IRP *m_pIrp;
			long m_Flags;
			
			friend class Device;

			IncomingIrp(IRP *pIrp, bool bIsPowerIrp, bool bStartNextPowerIrp)
			{
				ASSERT(pIrp);
				m_pIrp = pIrp;
				m_Flags = bIsPowerIrp ? IsPowerIrp : 0;
				if (bStartNextPowerIrp)
					m_Flags |= StartNextPowerIrp;
			}

			IncomingIrp(IRP *pIrp, long Flags)
			{
				ASSERT(pIrp);
				ASSERT(!(Flags & ~ValidFlags));
				m_pIrp = pIrp;
				m_Flags = Flags;
			}

			~IncomingIrp() {}

		public:

			enum
			{
				LowerDriverCalled	= 0x01,
				IrpCompleted		= 0x02,
				IrpMarkedPending	= 0x04,
				IoStatusSet			= 0x08,

				IsPowerIrp			= 0x10,
				StartNextPowerIrp	= 0x20,
				FromDispatchThread	= 0x40,

				ValidFlags			= 0x7F,
			};

			bool IsFromQueue() {return (m_Flags & FromDispatchThread) != 0;}

#pragma region GetXXX methods
			PIO_STATUS_BLOCK GetIoStatus() {return &m_pIrp->IoStatus;}
			void *GetSystemBuffer() {return m_pIrp->AssociatedIrp.SystemBuffer;}
			PMDL GetMdlAddress() {return m_pIrp->MdlAddress;}
			void *GetUserBuffer() {return m_pIrp->UserBuffer;}

			PEPROCESS GetRequestorProcess() {return IoGetRequestorProcess(m_pIrp);}

			NTSTATUS GetStatus() {return m_pIrp->IoStatus.Status;}

			bool Is32BitProcess() {return IoIs32bitProcess(m_pIrp) != FALSE;}
#pragma endregion
			
#pragma region SetXXX methods
			void SetIoStatus(NTSTATUS Status, unsigned Information)
			{
				ASSERT(Status != STATUS_PENDING);
				m_pIrp->IoStatus.Status = Status;
				m_pIrp->IoStatus.Information = Information;
				InterlockedOr(&m_Flags, IoStatusSet);
			}

			void SetIoStatus(NTSTATUS Status)
			{
				ASSERT(Status != STATUS_PENDING);
				m_pIrp->IoStatus.Status = Status;
				InterlockedOr(&m_Flags, IoStatusSet);
			}
#pragma endregion

#pragma region Action methods

			void CompleteRequest(CCHAR PriorityBoost = IO_NO_INCREMENT)
			{
				ASSERT(!(m_Flags & IrpCompleted));
				InterlockedOr(&m_Flags, IrpCompleted);
				if (m_Flags & StartNextPowerIrp)
					PoStartNextPowerIrp(m_pIrp);
				IoCompleteRequest(m_pIrp, IO_NO_INCREMENT);
			}

			void MarkPending()
			{
				InterlockedOr(&m_Flags, IrpMarkedPending);
				IoMarkIrpPending(m_pIrp);
			}

#pragma endregion

			//Do not export the IRP pointer directly. Add inline methods instead.
//			IRP *GetIRPPointer() {return m_pIRP;}
		};

		class OutgoingIRP
		{
		private:
			PIRP m_pIrp;
			bool m_bSynchronous;
			KEvent m_Event;
			IO_STATUS_BLOCK m_StatusBlock;
			PIO_STACK_LOCATION m_pSp;
			PDEVICE_OBJECT m_pDevObj;

		public:
			OutgoingIRP(ULONG MajorFunction, UCHAR MinorFunction, PDEVICE_OBJECT pDeviceObject,
				bool Synchronous = true, PVOID pBuffer = NULL, ULONG Length = 0, PLARGE_INTEGER StartingOffset = NULL) :
				m_bSynchronous(Synchronous),
				m_pSp(NULL),
				m_pDevObj(pDeviceObject),
				m_Event(false, kAutoResetEvent)
			{
				ASSERT(Synchronous);	//Asynchronous IRPs not supported in this version of framework
				m_pIrp = IoBuildSynchronousFsdRequest(MajorFunction, pDeviceObject, pBuffer, Length, StartingOffset, m_Event, &m_StatusBlock);
				if (!m_pIrp)
					return;
				m_pSp = IoGetNextIrpStackLocation(m_pIrp);
				m_pSp->MinorFunction = MinorFunction;

				m_pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
				m_pIrp->IoStatus.Information = 0;
			}

			OutgoingIRP(ULONG IoControlCode, PDEVICE_OBJECT pDeviceObject,
				bool InternalControl, PVOID pInBuffer = NULL, ULONG InBufLength = 0, PVOID pOutBuffer = NULL, ULONG OutBufLength = 0) :
				m_bSynchronous(true),
				m_pSp(NULL),
				m_pDevObj(pDeviceObject),
				m_Event(false, kAutoResetEvent)
			{
				m_pIrp = IoBuildDeviceIoControlRequest(IoControlCode, pDeviceObject, pInBuffer, InBufLength, pOutBuffer, OutBufLength, InternalControl, m_Event, &m_StatusBlock);
				if (!m_pIrp)
					return;
				m_pSp = IoGetNextIrpStackLocation(m_pIrp);
			}
			bool Valid() {return (m_pIrp != NULL);}

			NTSTATUS Call(bool Wait = true)
			{
				ASSERT(m_pDevObj);
				ASSERT(m_pIrp);
				NTSTATUS st = IoCallDriver(m_pDevObj, m_pIrp);
				if (!NT_SUCCESS(st))
					return st;
				if (Wait)
					st = m_Event.WaitEx();
				if (!NT_SUCCESS(st))
					return st;
				m_pIrp = NULL;
				return m_StatusBlock.Status;
			}

			NTSTATUS Wait()
			{
				return m_Event.WaitEx();
			}

			PIO_STATUS_BLOCK GetIoStatus() {return &m_StatusBlock;}
			
			~OutgoingIRP()
			{
				//No IoFreeIrp() is needed.
			}

			PIO_STACK_LOCATION GetNextStackLocation()
			{
				return m_pSp;
			}

		};
	}
}