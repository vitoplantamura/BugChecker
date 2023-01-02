#pragma once

#include "registry.h"
#include "../../bzscore/WinKernel/init.h"

extern "C" NTSTATUS _stdcall DriverEntry(IN OUT PDRIVER_OBJECT, IN PUNICODE_STRING);

namespace BazisLib
{
	namespace DDK
	{
		class Driver
		{
		protected:
			PDRIVER_OBJECT m_DriverObject;
			bool m_bRegisterAddDevice;
			String m_RegistryPath;

		private:
			static VOID sDriverUnload (IN PDRIVER_OBJECT DriverObject);
			static NTSTATUS sDispatch(IN PDEVICE_OBJECT  DeviceObject,  IN PIRP  Irp);
			static NTSTATUS sDispatchPower(IN PDEVICE_OBJECT  DeviceObject,  IN PIRP  Irp);
			static NTSTATUS sAddDevice(IN PDRIVER_OBJECT  DriverObject, IN PDEVICE_OBJECT  PhysicalDeviceObject);

			friend NTSTATUS _stdcall ::CPPDriverEntry(IN OUT PDRIVER_OBJECT   DriverObject, IN PUNICODE_STRING      RegistryPath);
			friend class Device;

			static Driver *GetMainDriver();
		
		protected:
			virtual NTSTATUS DispatchRoutine(IN PDEVICE_OBJECT  DeviceObject,  IN PIRP  Irp, bool bIsPowerIrp);

		protected:			
			virtual NTSTATUS DriverLoad(IN PUNICODE_STRING RegistryPath);
			virtual NTSTATUS AddDevice(IN PDEVICE_OBJECT  PhysicalDeviceObject);
			
			virtual void OnDeviceRegistered(const Device *pDevice) {}
			virtual void OnDeviceUnregistered(const Device *pDevice) {}

		public:
			//! Creates a Driver class instance.
			/*! A typical DDK driver should contain a single Driver object instance that is created inside
				CreateMainDriverInstance() routine.
				\param RegisterAddDevice Specifies whether the driver should register the AddDevice callback within
					   the operating system. PnP drivers should pass TRUE.
			*/
			Driver(bool RegisterAddDevice = true)
			{
				m_bRegisterAddDevice = RegisterAddDevice;

				void *unused = &DriverEntry;	//This ensures that the OBJ file containing DriverEntry() is linked.
			}

			//The destructor is called when the driver is being unloaded
			virtual ~Driver(){}

			void *operator new(size_t size)
			{
				return npagednew(size);
			}

			String GetRegistryPath()
			{
				return m_RegistryPath;
			}
		};

		template <class _DeviceClass, GUID *_pDeviceInterfaceGuid = NULL> class _GenericPnpDriver : public Driver
		{
		public:
			_DeviceClass *m_pDevice;

			_GenericPnpDriver() : DDK::Driver(true)
			{
				m_pDevice = NULL;
			}

			virtual NTSTATUS AddDevice(IN PDEVICE_OBJECT  PhysicalDeviceObject) override
			{
				if (m_pDevice && m_pDevice->Valid())
					return STATUS_ALREADY_REGISTERED;
				m_pDevice = new _DeviceClass();
				if (!m_pDevice)
					return STATUS_NO_MEMORY;
				

				NTSTATUS st = m_pDevice->AddDevice(this, PhysicalDeviceObject, _pDeviceInterfaceGuid, NULL);
				if (!NT_SUCCESS(st))
					return st;
				if (m_pDevice->IsDeleteThisAfterRemoveRequestEnabled())
					m_pDevice = NULL;
				return STATUS_SUCCESS;
			}

			virtual ~_GenericPnpDriver()
			{
				delete m_pDevice;
			}
		};

	}
}

BazisLib::DDK::Driver *_stdcall CreateMainDriverInstance();