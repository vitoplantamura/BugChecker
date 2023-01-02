#pragma once

#ifndef _BZSCMN_FILE_H_INCLUDED
#error The file_base.h should be included ONLY from BZSCMN/file.h
#endif

namespace BazisLib
{
	namespace _PlatformSpecificConstants
	{
		enum
		{
			_PathSeparator = '\\',
		};

		enum 
		{
			//Seek mode definition
			_FileBegin		= FILE_BEGIN,
			_FileCurrent	= FILE_CURRENT,
			_FileEnd		= FILE_END,

			//File access modes
			_QueryAccess	= FILE_READ_ATTRIBUTES,
			_ReadAccess	    = GENERIC_READ,
			_WriteAccess	= GENERIC_WRITE,
			_DeleteAccess	= DELETE,
			_AllAccess		= GENERIC_ALL,
			_ReadSecurity	= READ_CONTROL | ACCESS_SYSTEM_SECURITY,
			_ModifySecurity	= READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY,

			//Sharing modes
			_NoShare		= 0,
			_ShareRead		= FILE_SHARE_READ,
			_ShareWrite		= FILE_SHARE_WRITE,
			_ShareDelete	= FILE_SHARE_DELETE,
			_ShareAll		= FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,

			//File open modes
			_OpenExisting	= OPEN_EXISTING,
			_OpenAlways		= OPEN_ALWAYS,		
			_CreateAlways	= CREATE_ALWAYS,
			_CreateNew		= CREATE_NEW,

			//File attributes
			_NormalFile		= FILE_ATTRIBUTE_NORMAL,
			_DirectoryFile	= FILE_ATTRIBUTE_DIRECTORY,
			_ReadOnly		= FILE_ATTRIBUTE_READONLY,
			_Hidden			= FILE_ATTRIBUTE_HIDDEN,
			_System			= FILE_ATTRIBUTE_SYSTEM,
			_Archive		= FILE_ATTRIBUTE_ARCHIVE,
			_InvalidAttr	= (DWORD)-1,

		};
	};

	enum SpecialDirectoryType 
	{
		dirWindows = 0x8A000001, 
		dirSystem,
		dirDrivers,
	};

	static inline SpecialDirectoryType SpecialDirFromCSIDL(int CSIDL)
	{
		return (SpecialDirectoryType)CSIDL;
	}

	namespace Win32
	{
#ifdef UNDER_CE
		class FilePathIteratorBase
		{
		protected:
			HANDLE m_hFindObj;
			WIN32_FIND_DATA m_FindData;

			FilePathIteratorBase() :
				m_hFindObj(INVALID_HANDLE_VALUE)
			{
				memset(&m_FindData, 0, sizeof(m_FindData));
				m_FindData.cFileName[0] = '\\';
			}

			~FilePathIteratorBase()
			{
				if (m_hFindObj != INVALID_HANDLE_VALUE)
					FindClose(m_hFindObj);
			}
		};
#else
		class FilePathIteratorBase
		{
		protected:
			enum {InvalidDriveLetter = '0'};

		protected:
			DWORD m_FoundDrivesMask;
			String m_RootPath;

			FilePathIteratorBase() :
				m_RootPath(_T("?:\\"), 4),
				m_FoundDrivesMask(GetLogicalDrives())
			{
				m_RootPath[0] = InvalidDriveLetter;
				for (unsigned i = 'A', j = 1; i <= 'Z'; i++, j <<= 1)
					if (m_FoundDrivesMask & j)
					{
						m_RootPath[0] = i;
						break;
					}
			}
		};
#endif
	}

	namespace Win32
	{
		namespace Security
		{
			class TranslatedSecurityDescriptor;
		}
	}

	typedef Win32::FilePathIteratorBase  _PlatformSpecificFilePathIteratorBase;
	typedef Win32::Security::TranslatedSecurityDescriptor SecurityDescriptor;
}