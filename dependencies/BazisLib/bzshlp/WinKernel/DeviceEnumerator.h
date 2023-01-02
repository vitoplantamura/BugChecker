#pragma once
#include "../../bzscore/string.h"

extern "C" POBJECT_TYPE *IoDriverObjectType;

namespace BazisLib
{
	namespace DDK
	{
		class DriverObjectFinder
		{
		private:
			PDRIVER_OBJECT m_pDriver;

		public:
			DriverObjectFinder(const wchar_t *pszDriverName, POBJECT_TYPE pDriverObjectType = *IoDriverObjectType);


			PDRIVER_OBJECT GetDriverObject()
			{
				return m_pDriver;
			}

			~DriverObjectFinder()
			{
				Reset();
			}

			bool Valid()
			{
				return m_pDriver != NULL;
			}

			void Reset()
			{
				if (m_pDriver)
					ObDereferenceObject(m_pDriver);
				m_pDriver = NULL;
			}
		};

		class DeviceEnumerator : public DriverObjectFinder
		{
		private:
			PDEVICE_OBJECT *m_ppDevices;
			size_t m_DeviceObjectCount;

		public:

			DeviceEnumerator(const wchar_t *pszDriverName, POBJECT_TYPE pDriverObjectType = *IoDriverObjectType);
			~DeviceEnumerator();

			size_t DeviceCount()
			{
				return m_DeviceObjectCount;
			}

			PDEVICE_OBJECT GetDeviceByIndex(size_t index)
			{
				if (!m_ppDevices || (index >= m_DeviceObjectCount))
					return NULL;
				return m_ppDevices[index];
			}
		};
	}
}