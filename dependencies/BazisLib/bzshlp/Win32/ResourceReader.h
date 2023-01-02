#pragma once
#include <bzscore/objmgr.h>
#include <bzscore/file.h>

namespace BazisLib
{
	namespace Win32
	{
		class ResourceReader
		{
		private:
			HMODULE m_hModule;
			bool m_OwnsModule;

		public:
			class Resource : AUTO_OBJECT
			{
			private:
				HRSRC m_hResource;
				HMODULE m_hModule;

				HGLOBAL m_hLoadedResource;
				PVOID m_pMappedResource;

			public:
				Resource(HMODULE hMod, HRSRC hRes)
					: m_hModule(hMod)
					, m_hResource(hRes)
					, m_hLoadedResource(NULL)
					, m_pMappedResource(NULL)
				{
				}

				~Resource()
				{
					if (m_pMappedResource)
						UnlockResource(m_hLoadedResource);
					if (m_hLoadedResource)
						FreeResource(m_hLoadedResource);
				}

				size_t GetSize()
				{
					return SizeofResource(m_hModule, m_hResource);
				}

				PVOID Map()
				{
					if (!m_hLoadedResource)
						m_hLoadedResource = LoadResource(m_hModule, m_hResource);
					if (!m_hLoadedResource)
						return NULL;
					if (!m_pMappedResource)
						m_pMappedResource = LockResource(m_hLoadedResource);
					return m_pMappedResource;
				}

				ActionStatus SaveToFile(String &fileName)
				{
					ActionStatus st;
					File file(fileName, FileModes::CreateOrTruncateRW, &st);
					if (!st.Successful())
						return st;

					PVOID pData = Map();
					if (!pData)
						return MAKE_STATUS(ActionStatus::FailFromLastError());

					size_t size = GetSize();
					if (!size)
						return MAKE_STATUS(ActionStatus::FailFromLastError());

					file.Write(pData, size, &st);
					return st;
				}

			};

		public:
			ResourceReader()
				: m_OwnsModule(false)
			{
				m_hModule = GetModuleHandle(NULL);
			}

			~ResourceReader()
			{
				if (m_hModule && m_OwnsModule)
					FreeLibrary(m_hModule);
			}

			ManagedPointer<Resource> FindResource(LPCTSTR lpName, LPCTSTR lpType)
			{
				HRSRC hRes = ::FindResource(m_hModule, lpName, lpType);
				if (!hRes)
					return NULL;
				return new Resource(m_hModule, hRes);
			}
		};
	}
}