#pragma once

#include "device.h"
#include "cmndef.h"

namespace BazisLib
{
	namespace DDK
	{
		class IUnknownDeviceInterface;

		enum DevicePNPState
		{
			NotStarted = 0,         // Not started yet
			Started,                // Device has received the START_DEVICE IRP
			StopPending,            // Device has received the QUERY_STOP IRP
			Stopped,                // Device has received the STOP_DEVICE IRP
			RemovePending,          // Device has received the QUERY_REMOVE IRP
			SurpriseRemovePending,  // Device has received the SURPRISE_REMOVE IRP
			Deleted,                // Device has received the REMOVE_DEVICE IRP
			Unknown                 // Unknown state
		};

		class PNPDevice : protected Device
		{
		private:
			DevicePNPState m_CurrentPNPState;
			DevicePNPState m_PreviousPNPState;
			bool m_bDeleteThisAfterRemoveRequest;

		protected:
			void SetNewPNPState(DevicePNPState NewState) {m_PreviousPNPState = m_CurrentPNPState; m_CurrentPNPState = NewState;}
			void RestorePreviousPNPState() {m_CurrentPNPState = m_PreviousPNPState;}
			DevicePNPState GetCurrentPNPState() {return m_CurrentPNPState;}

		public:
			PNPDevice(DEVICE_TYPE DeviceType = FILE_DEVICE_UNKNOWN,
					  bool bDeleteThisAfterRemoveRequest = false,
					  ULONG DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN,
					  bool bExclusive = FALSE,
					  ULONG AdditionalDeviceFlags = DO_POWER_PAGABLE,
					  LPCWSTR pwszDeviceName = NULL);

			NTSTATUS AddDevice(Driver *pDriver, PDEVICE_OBJECT PhysicalDeviceObject, const GUID *pInterfaceGuid = NULL, const wchar_t *pwszLinkPath = NULL);

			bool Valid() {return Device::Valid() && GetNextDevice() && GetUnderlyingPDO();}

			virtual ~PNPDevice();

			static void *RequestInterface(PDEVICE_OBJECT pDevice, LPCGUID lpGuid, unsigned Version, PVOID pSpecificData = NULL);

			bool IsDeleteThisAfterRemoveRequestEnabled() { return m_bDeleteThisAfterRemoveRequest; }

		protected:
			//! Requests an interface from an underlying PDO.
			/*! This method is used to request the PDO provided by BusPDO class.
				Calling this method results in invoking the BusPDO::CreateObjectByInterfaceID() method via
				the IRP_MN_QUERY_INTERFACE call.
				\remarks If the underlying PDO is not provided by BazisLib, or the interface is not supported,
						 the function will return NULL.						
			*/
			void *RequestInterfaceFromUnderlyingDevice(LPCGUID lpGuid, unsigned Version, PVOID pSpecificData = NULL);

			//! Reports device state change to PnP manager
			/*! This method sends an asynchronous message to the PnP manager reporting the device state change.
				The exact type of the message is determined by the Event parameter.
				\param Event Specifies the event type. Event type GUIDs are defined in the <b>ioevent.h</b> file
							 (for example, GUID_IO_MEDIA_ARRIVAL).
			*/
			NTSTATUS ReportStateChange(const GUID &Event, PVOID pAdditionalData = NULL, ULONG AdditionalDataSize = 0);

		protected:
			/*  These are standard PNP event minihandlers. In order for them to be called on the corresponding IRP,
				an overriden DispatchRoutine() should ensure that either PNPDevice::DispatchRoutine() or
				PNPDevice::DispatchPNP() processes IRPs with IRP_MJ_PNP code.

				Note that these handlers do not take IncomingIRP pointer. That way, they do not need to complete IRP.
				Further actions taken with IRP depend on the return value of such method. If NTSUCCESS(status) is true,
				the IoStatus.Status field is updated the IRP is passed down the device stack. If not, the IRP is being
				completed with the status returned by minihandler. However, there is one exception. If a minihandler
				returns STATUS_NOT_SUPPORTED, the IRP is passed down the driver stack and the IoStatus.Status field is
				left untouched. This reflects the PnP IRP handling sequence.
			*/
			
			//See IODevice::DispatchRoutine() description for application hints.

			/*! Called when IRP_MN_START_DEVICE is received. Enables the device interface and returns STATUS_SUCCESS.
				If this method returns an unsuccessful status, the device would not be marked as started.
				
				Consider the following method structure when writing an override:
					NTSTATUS status = __super::OnStartDevice(...);
					if (!NT_SUCCESS(status))
						return status;
					
					<<< Perform actual initialization >>>
					
					status = ... ;
					return status;
				
			*/
			virtual NTSTATUS OnStartDevice(IN PCM_RESOURCE_LIST AllocatedResources, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated);

			//! Called when the IRP_MN_START IRP is successfully completed
			virtual void OnDeviceStarted() {};

			virtual NTSTATUS OnSurpriseRemoval();

			/*! Called when IRP_MN_REMOVE_DEVICE is received. Disables the device interface and deletes the
				device object. The OnRemoveDevice() minihandler is called after the lower driver processes the
				IRP. 
				\param LowerDeviceRemovalStatus Contains the status returned by lower driver.

				Consider the following method structure when writing an override:
					
					<<< Perform actual cleanup >>>
					status = ...
					if (NT_SUCCESS(status))
						return __super::OnRemoveDevice(LowerDeviceRemovalStatus);

					//Additionally, if you want to report an error code, do it AFTER calling __super::OnRemoveDevice():

					NTSTATUS status2 = __super::OnRemoveDevice(LowerDeviceRemovalStatus);
					if (!NT_SUCCESS(status2))
						return status2;
					
					return status;
			*/
			virtual NTSTATUS OnRemoveDevice(NTSTATUS LowerDeviceRemovalStatus);
			
			virtual NTSTATUS OnStopDevice();
			virtual NTSTATUS OnQueryStopDevice();
			virtual NTSTATUS OnCancelStopDevice();
			virtual NTSTATUS OnQueryRemoveDevice();
			virtual NTSTATUS OnCancelRemoveDevice();
			virtual NTSTATUS OnQueryDeviceRelations(DEVICE_RELATION_TYPE Type, PDEVICE_RELATIONS *ppDeviceRelations);
			
			virtual NTSTATUS OnQueryPNPDeviceState(PNP_DEVICE_STATE *pState){return STATUS_SUCCESS;}
			virtual NTSTATUS OnDeviceUsageNotification(bool InPath, DEVICE_USAGE_NOTIFICATION_TYPE Type) {return STATUS_SUCCESS;}

			virtual NTSTATUS OnSetPower(POWER_STATE_TYPE Type, POWER_STATE State, POWER_ACTION ShutdownType) {return STATUS_SUCCESS;}
			virtual NTSTATUS OnQueryPower(POWER_STATE_TYPE Type, POWER_STATE State, POWER_ACTION ShutdownType) {return STATUS_SUCCESS;}
				
		protected:
			/*! Actual handler for IRP_MJ_POWER requests.
			*/
			virtual NTSTATUS DispatchPower(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp);

			/*! Actual handler for IRP_MJ_PNP requests.
			*/
			virtual NTSTATUS DispatchPNP(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp);

			/*! IRP handler for Plug-and-play device. Handles IRP_MJ_PNP requests by calling DispatchPNP().
			*/
			virtual NTSTATUS DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp) override;
		};
	}

}