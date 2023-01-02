#pragma once

namespace BazisLib
{
	namespace DDK
	{
		/*! This class represents a device created by some other driver that is used by current driver
			for example (for attaching a filter). This class is CopyConstructor-aware.			
		*/
		class ExternalDeviceObjectReference
		{
			PDEVICE_OBJECT m_pDeviceObject;
			PFILE_OBJECT m_pFileObject;

		public:
			ExternalDeviceObjectReference(const wchar_t *pwszDevicePath, ACCESS_MASK AccessMask = FILE_READ_ATTRIBUTES);
			
			ExternalDeviceObjectReference(const ExternalDeviceObjectReference &src)
			{
				m_pDeviceObject = src.m_pDeviceObject;
				m_pFileObject = src.m_pFileObject;
				if (m_pFileObject)
					ObReferenceObject(m_pFileObject);
			}
			
			~ExternalDeviceObjectReference()
			{
				if (m_pFileObject)
					ObDereferenceObject(m_pFileObject);
			}

			bool Valid()
			{
				return m_pDeviceObject && m_pFileObject;
			}

			PDEVICE_OBJECT GetDeviceObject() {return m_pDeviceObject;}
			PFILE_OBJECT GetFileObject() {return m_pFileObject;}
		};

		static inline INT_PTR GetObjectReferenceCount(PVOID pObject) {return ((INT_PTR *)pObject)[-6];}
		static inline INT_PTR GetObjectHandleCount(PVOID pObject) {return ((INT_PTR *)pObject)[-5];}
	}
}