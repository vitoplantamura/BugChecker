#pragma once

namespace BazisLib
{
	namespace Win32
	{
		class MemoryMappedFile
		{
		private:
			HANDLE m_hFile;
			HANDLE m_hMapping;
			LPVOID m_pData;

		public:
			MemoryMappedFile(const wchar_t *pwszFile, bool readOnly = false, unsigned mappingSize = 0)
				: m_hFile(INVALID_HANDLE_VALUE)
				, m_hMapping(INVALID_HANDLE_VALUE)
				, m_pData(NULL)
			{
				m_hFile = CreateFile(pwszFile, 
					readOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					readOnly ? OPEN_EXISTING : OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

				if (m_hFile == INVALID_HANDLE_VALUE)
					return;

				m_hMapping = CreateFileMapping(m_hFile, NULL, readOnly ? PAGE_READONLY : PAGE_READWRITE,
					0, (DWORD)mappingSize, NULL);

				if (!m_hMapping || (m_hMapping == INVALID_HANDLE_VALUE))
					return;

				m_pData = MapViewOfFile(m_hMapping, readOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS, 0, 0, (DWORD)mappingSize);
			}

			~MemoryMappedFile()
			{
				if (m_pData)
					UnmapViewOfFile(m_pData);
				if (m_hMapping && (m_hMapping != INVALID_HANDLE_VALUE))
					CloseHandle(m_hMapping);
				if (m_hFile != INVALID_HANDLE_VALUE)
					CloseHandle(m_hFile);
			}

			operator void *()
			{
				return m_pData;
			}

			bool ChangeSize(unsigned size, ULONGLONG offset = 0, bool readOnly = false)
			{
				if (m_pData)
				{
					UnmapViewOfFile(m_pData);
					m_pData = NULL;
				}

				if (m_hMapping && (m_hMapping != INVALID_HANDLE_VALUE))
				{
					CloseHandle(m_hMapping);
					m_hMapping = INVALID_HANDLE_VALUE;
				}

				m_hMapping = CreateFileMapping(m_hFile, NULL, readOnly ? PAGE_READONLY : PAGE_READWRITE,
					0, (DWORD)size, NULL);

				if (!m_hMapping || (m_hMapping == INVALID_HANDLE_VALUE))
					return false;

				m_pData = MapViewOfFile(m_hMapping, readOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS, (DWORD)(offset >> 32), (DWORD)offset, (DWORD)size);
				return true;
			}

			ULONGLONG GetFileSize()
			{
				DWORD dwHigh = 0;
				DWORD dwLow = ::GetFileSize(m_hFile, &dwHigh);

				return (((ULONGLONG)dwHigh) << 32) | dwLow;
			}
		};
	}
}