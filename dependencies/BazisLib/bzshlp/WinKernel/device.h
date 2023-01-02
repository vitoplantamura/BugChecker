#pragma once

#include "cmndef.h"
#include "../../bzscore/sync.h"
#include "irp.h"
#include "../../bzscore/string.h"

namespace BazisLib
{
	namespace DDK
	{
		class Driver;

		namespace Security
		{
			class TranslatedAcl;
			class TranslatedSecurityDescriptor;
			class Sid;
		}

		/*
			The main idea behind the device class hierarchy is to simplify common request handling. For this
			purpose, two types of handlers are being introduced:
			
			The first type is the Dispatcher type. Routines, such as DispatchRoutine(), take IncomingIrp pointer
			and PIO_STACK_LOCATION. They may handle the IRP, call the lower driver or complete the IRP using
			IncomingIrp::CompleteRequest(). On the other hand, if they want to leave IRP untouched, they simply
			return some status (or call __super::DispatchXXX()). In that case, the IRP will be forwarded to the
			lower driver, if one exists. If not, it will be completed with the default status.

			The second handler type is the so called minihandler type. Such routines are usually named as OnXXX and
			do not take IncomingIrp pointer. Instead, they take specialized arguments that simplify IRP handling for
			each exact IRP type. Minihandlers do not touch IRP in any way. The result of their execution is reflected
			by returned status. Depending on their return value, the framework decides whether to complete the IRP,
			or to pass it to the lower driver. For more details on this behaviour, see comments before OnXXX() method
			groups in child classes.
		*/

		//! Represents a DDK device. Suitable for both legacy, WDM and filter devices.
		class Device
		{
		private:
			DEVICE_TYPE m_DeviceType;
			ULONG m_DeviceCharacteristics;
			ULONG m_AdditionalDeviceFlags;
			bool m_bExclusive;
			const wchar_t *m_pwszDeviceName;

			Driver *m_pDriver;

			bool m_bDeletePending;
			//This field is set to 1 on device registration.
			long m_OutstandingIORequestCount;

			KEvent m_DeleteAllowed;
			KEvent m_StopAllowed;

			//This field is set to TRUE when the device is deleted from the system and is unregistered inside
			//the driver. Instead of deleting it in the middle of IRP handling, this flag should be set by calling
			//DeleteThisAfterLastRequest().
			bool m_bDestroyObjectAfterLastRequest;

			class IrpQueue *m_pDeviceQueue;
			HANDLE m_hServiceThread;
			PETHREAD m_pServiceThread;
			
		private:
			//In case of initialization failure, contains the status code.
			NTSTATUS m_InitializationStatus;

			//The following fields are fully managed by Device class. No child class should
			//modify them. For that purpose, they are marked as private and the corresponding
			//GetXXX() inlines are provided. This ensures no performance penalty and prevents
			//the developer from accidently modifying them.
			PDEVICE_OBJECT m_pDeviceObject;
			PDEVICE_OBJECT m_pNextDevice;
			PDEVICE_OBJECT m_pUnderlyingPDO;

			UNICODE_STRING m_InterfaceName;
			String m_LinkName;
			bool m_bInterfaceEnabled;

#ifdef _DEBUG
		protected:
			//Device debug name used in event logging
			const char *m_pszDeviceDebugName;
#endif

		protected:

			void DeleteThisAfterLastRequest() {m_bDestroyObjectAfterLastRequest = true;}
			bool ReportInitializationError(NTSTATUS status);

			PDEVICE_OBJECT GetDeviceObject() {return m_pDeviceObject;}
			PDEVICE_OBJECT GetNextDevice() {return m_pNextDevice;}
			PDEVICE_OBJECT GetUnderlyingPDO() {return m_pUnderlyingPDO;}
			PCUNICODE_STRING GetInterfaceName() {return &m_InterfaceName;}

			Driver *GetDriver() {return m_pDriver;}
			const wchar_t *GetShortDeviceName() {return m_pwszDeviceName;}

			//If called after device registration, fails
			bool SetShortDeviceName(const wchar_t *pwszNewName);

		private:
			struct Extension
			{
				enum {DefaultSignature = 'VEDB'};
				unsigned Signature;
				Device *pDevice;
			};

			friend class Driver;

		private:
			void IncrementIOCount();
			void DecrementIOCount();
			NTSTATUS ProcessIRP(IN PIRP  Irp, bool bIsPowerIrp);
			NTSTATUS PostProcessIRP(IncomingIrp *pIrp, NTSTATUS ProcessingStatus, bool FromDispatcherThread);

		protected:
			void OnPendingIRPCompleted(IN PIRP Irp) {(void)Irp; DecrementIOCount(); }
			NTSTATUS WaitForStopEvent(bool FromIRPHandler);

		public:
			/*! Prepares a new device for creation. Does not call \b IoCreateDevice().
			\param DeviceType Contains the type for device
			\param pwszDeviceName Contains the name for the device (without \\Device prefix)
			\param DeviceCharacteristics Contains device characteristics passed to \b IoCreateDevice()
			\param bExclusive Exclusiveness flag passed to \b IoCreateDevice()
			\param AdditionalDeviceFlags Flags that are set in DEVICE_OBJECT::Flags immediately after
				   device creation.
			*/
			Device(DEVICE_TYPE DeviceType = FILE_DEVICE_UNKNOWN,
				   const wchar_t *pwszDeviceName = NULL,
				   ULONG DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN,
				   bool bExclusive = false,
				   ULONG AdditionalDeviceFlags = DO_POWER_PAGABLE);

			/*! Creates and registers the system device using IoCreateDevice()
			\param CreateDosDevicesLink Specifies whether to create a symbollic link in \DosDevices
			\param CompleteInitialization Specifies whether to mark device as initialized
			\param pDriver Specifies the driver object used in device creation, or NULL to use the main driver.
			\remarks Note that as long as the device is marked as initialized, no more registration-based actions
					 are allowed for the device. For more details, see the CompleteInitialization() method.
			*/
			NTSTATUS RegisterDevice(Driver *pDriver, bool bCompleteInitialization = true, const wchar_t *pwszLinkPath = NULL);

			/*! Completes the initialization process by clearing the DO_DEVICE_INITIALIZING flag.
			*/
			void CompleteInitialization();

			/*! Attaches the device to the highest device object in the chain. If the device is already attached to another
				device, or CompleteInitialization() was called, this method returns STATUS_INVALID_DEVICE_STATE.
			*/
			NTSTATUS AttachToDeviceStack(PDEVICE_OBJECT DeviceObject);

			/*! Attaches the device to another device. If the device is already attached to another device, this
				method returns STATUS_INVALID_DEVICE_STATE.
			*/
			NTSTATUS AttachToDevice(String &DevicePath);

			/* Registers a device interface with the specified GUID. Note that once an interface is registered with
			   this method, no more interfaces can be registered for this device.
			   \remarks Note that the resources associated with the interface are automatically freed on device
						deletion.
						Also note that the device interface can be registered only when the device is attached to a
						physical device object. That way, it the AttachToDeviceStack() was not called for the device,
						any call to RegisterInterface() will fail.
			*/
			NTSTATUS RegisterInterface(IN CONST GUID *pGuid, IN PCUNICODE_STRING ReferenceString = NULL);
			NTSTATUS EnableInterface();
			NTSTATUS DisableInterface();

			/*! Detaches the device from another device, if it was previously attached. If not, returns an error.
			*/
			NTSTATUS DetachDevice();

			/*! Deletes the device from system, previously detaching it from another device, if it was attached.
				Note that this routine also shuts down the request processing thread, if one has been started.
				That way, it should be called even if the object is being deleted without calling to IoCreateDevice().
			*/
			NTSTATUS DeleteDevice(bool FromIRPHandler);
			
			bool Valid();

			virtual ~Device();

			static NTSTATUS sApplyDACL(class Security::TranslatedAcl *pACL, PCWSTR pLinkPath);
			NTSTATUS ApplyDACL(class Security::TranslatedAcl *pACL);

		protected:
			/*! Sends an IRP to the next device (returned by the IoAttachDeviceToDeviceStack() call) and
				waits for the next driver to process request.
			    \remarks If the device was not attached to another device, this routine can be safely
						 called and will always return STATUS_INVALID_DEVICE_REQUEST.
				\remarks Please avoid using this method as a default handler for IRP types you do not want
						 to handle. Use __super::DispatchRoutine() instead.
			*/
			NTSTATUS CallNextDriverSynchronously(IN IncomingIrp *Irp);

			/*! Creates a device request queue allowing subsequent EnqueuePacket() calls. Also creates a
				dispatcher thread that dequeues IRPs from the queue and processes them. The thread is
				automatically stopped on device deletion.
				\remarks This method is not thread-safe. Two simultanious calls to CreateDeviceQueue() may
						 cause the system to start losing packets. It is highly recommended to call this method
						 from your device object constructor.
			*/
			NTSTATUS CreateDeviceRequestQueue();

			/*! Enqueues an incoming IRP in the device queue. If the device queue was not previously created
				using the Device::CreateDeviceRequestQueue() call, this method returns STATUS_INTERNAL_ERROR.
				\return If the method succeeds, it returns STATUS_PENDING.
				\remarks See comments in the beginning of IRP.H for use example.
			*/
			NTSTATUS EnqueuePacket(IN IncomingIrp *Irp);

			/*! Returns TRUE if called from the worker thread
				\remarks This method can be used in assertions to ensure that some non-thread-safe code
						 is running from the correct thread and no race condition occurs.
			*/
			bool IsRunningFromWorkerThread() {return PsGetCurrentThread() == m_pServiceThread;}

		private:
			/*! Skips current stack location and calls the next driver in chain. If no such driver exists,
				the routine completes IRP and returns the value of Irp->IoStatus.Status.
				\remarks Please avoid using this method as a default handler for IRP types you do not want
						 to handle. Use __super::DispatchRoutine() instead.
			    \remarks Note that if you cannot use this routine in the following way:
				<PRE>
							return ForwardPacketToNextDriver(Irp);		//!!!BAD!!!
				</PRE>
						 If this routine returns STATUS_PENDING, your method will also return STATUS_PENDING
						 and nobody will be responsible for decrementing outstanding I/O operation count.
			    \remarks This method is fully PowerIRP-aware.
			*/
			NTSTATUS ForwardPacketToNextDriver(IN IncomingIrp *Irp);

			/*! Sends an IRP to the next device (returned by the IoAttachDeviceToDeviceStack() call) and completes
				the IRP using IoCompleteRequest() call. This routine is used for completing dequeued IRPs.
			    \remarks If the device was not attached to another device, this routine can be safely
						 called and will always return STATUS_INVALID_DEVICE_REQUEST.
				\remarks Please avoid using this method as a default handler for IRP types you do not want
						 to handle. Use __super::DispatchRoutine() instead.
			*/
			NTSTATUS ForwardPacketToNextDriverWithIrpCompletion(IN IncomingIrp *Irp);

			static void RequestDispatcherThreadBody(IN PVOID pParam);

		protected:
			
			static NTSTATUS EventSettingCompletionRoutine(IN PDEVICE_OBJECT  DeviceObject, IN PIRP Irp, IN PVOID Context);
			static NTSTATUS IrpCompletingCompletionRoutine(IN PDEVICE_OBJECT  DeviceObject, IN PIRP Irp, IN PVOID Context);

			
			/*! Device::DispatchRoutine actually does nothing and returns the IoStatus.Status field of the IRP.
				All IRP handling functionality is defined in child classes.
			*/
			virtual NTSTATUS DISPATCH_ROUTINE_DECL DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp) {return Irp->GetStatus();}
		};
	}
}