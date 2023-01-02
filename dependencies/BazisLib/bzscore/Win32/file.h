#pragma once
#include "security.h"

namespace BazisLib
{
	class IoCompletion;

	namespace FileFlags
	{
		enum SeekType
		{
			FileBegin	= FILE_BEGIN,
			FileCurrent = FILE_CURRENT,
			FileEnd		= FILE_END,
		};

		enum FileAttribute
		{
			NormalFile	= FILE_ATTRIBUTE_NORMAL,
			Hidden		= FILE_ATTRIBUTE_HIDDEN,
			ReadOnly	= FILE_ATTRIBUTE_READONLY,
			System		= FILE_ATTRIBUTE_SYSTEM,
			Archive		= FILE_ATTRIBUTE_ARCHIVE,
			DirectoryFile	= FILE_ATTRIBUTE_DIRECTORY,
			InvalidAttr	= (DWORD)-1,
		};
	}
}

#include "../filehlp.h"

namespace BazisLib
{
	namespace Win32
	{
		struct FileMode
		{
			DWORD dwDesiredAccess;
			DWORD dwShareMode;
			DWORD dwCreationDisposition;
			DWORD dwFlagsAndAttributes;

			FileMode ShareAll() const
			{
				FileMode copy = *this;
				copy.dwShareMode |= FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
				return copy;
			}

			FileMode ShareNone() const
			{
				FileMode copy = *this;
				copy.dwShareMode = 0;
				return copy;
			}

			FileMode AddAttributes(BazisLib::FileFlags::FileAttribute attr) const
			{
				FileMode copy = *this;
				copy.dwFlagsAndAttributes |= (DWORD)attr;
				return copy;
			}

			FileMode AddRawFlags(DWORD flags) const
			{
				FileMode copy = *this;
				copy.dwFlagsAndAttributes |= flags;
				return copy;
			}
		};

		class File
		{
		private:
			HANDLE m_hFile;

		private:
			File(const File &anotherFile) {ASSERT(FALSE);}
			void operator=(const File &anotherFile) {ASSERT(FALSE);}

		public:
			File(const TCHAR *ptszFileName, const FileMode &mode, ActionStatus *pStatus = NULL, PSECURITY_ATTRIBUTES pSecAttributes = NULL)
				: m_hFile(INVALID_HANDLE_VALUE)
			{
				m_hFile = CreateFile(ptszFileName, mode.dwDesiredAccess, mode.dwShareMode, pSecAttributes, mode.dwCreationDisposition, mode.dwFlagsAndAttributes, 0);
				if (m_hFile == INVALID_HANDLE_VALUE && pStatus)
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
				else
					ASSIGN_STATUS(pStatus, Success);
			}

			File(const String &FileName, const FileMode &mode, ActionStatus *pStatus = NULL, PSECURITY_ATTRIBUTES pSecAttributes = NULL)
				: m_hFile(INVALID_HANDLE_VALUE)
			{
				m_hFile = CreateFile(FileName.c_str(), mode.dwDesiredAccess, mode.dwShareMode, pSecAttributes, mode.dwCreationDisposition, mode.dwFlagsAndAttributes, 0);
				if (m_hFile == INVALID_HANDLE_VALUE && pStatus)
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
				else
					ASSIGN_STATUS(pStatus, Success);
			}

			File()
				: m_hFile(INVALID_HANDLE_VALUE)
			{
			}

			~File()
			{
				if (m_hFile != INVALID_HANDLE_VALUE)
					CloseHandle(m_hFile);
			}

			HANDLE GetHandleForSingleUse() {return m_hFile;}

			size_t Read(void *pBuffer, size_t size, ActionStatus *pStatus = NULL, bool IncompleteReadSupported = false)
			{
				DWORD done = 0;
				BOOL success = ReadFile(m_hFile, pBuffer, (DWORD)size, &done, NULL);
				if (!success)
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
					return done;
				}
				else if (!done)
				{
					ASSIGN_STATUS(pStatus, EndOfFile);
					return 0;
				}

				ASSIGN_STATUS(pStatus, Success);
				return done;
			}

			size_t Write(const void *pBuffer, size_t size, ActionStatus *pStatus = NULL)
			{
				DWORD done = 0;
				SetLastError(0);
				BOOL success = WriteFile(m_hFile, pBuffer, (DWORD)size, &done, NULL);
				if (done != size)
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
					return done;
				}
				ASSIGN_STATUS(pStatus, Success);
				return done;
			}

			LONGLONG GetSize(ActionStatus *pStatus = NULL)
			{
				ULARGE_INTEGER fileSize;
				fileSize.QuadPart = ((ULONGLONG)-1LL);
				fileSize.LowPart = GetFileSize(m_hFile, &fileSize.HighPart);
				if ((fileSize.LowPart != ((ULONGLONG)-1LL)) && (GetLastError() == ERROR_SUCCESS))
					ASSIGN_STATUS(pStatus, Success);
				else
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
				return fileSize.QuadPart;
			}

			LONGLONG Seek(LONGLONG Offset, FileFlags::SeekType seekType = FileFlags::FileBegin, ActionStatus *pStatus = NULL)
			{
				LARGE_INTEGER offset;
				offset.QuadPart = Offset;
				SetLastError(ERROR_SUCCESS);
				offset.LowPart = SetFilePointer(m_hFile, offset.LowPart, &offset.HighPart, (DWORD)seekType);
				if ((offset.LowPart != ((ULONGLONG)-1LL)) && (GetLastError() == ERROR_SUCCESS))
					ASSIGN_STATUS(pStatus, Success);
				else
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
				return offset.QuadPart;
			}

			LONGLONG GetPosition(ActionStatus *pStatus = NULL)
			{
				return Seek(0LL, FileFlags::FileCurrent, pStatus);
			}

			void Close()
			{
				if (m_hFile == INVALID_HANDLE_VALUE)
					return;
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
			}

			bool Valid()
			{
				return (m_hFile != INVALID_HANDLE_VALUE);
			}

			HANDLE DetachHandle()
			{
				HANDLE h = m_hFile;
				m_hFile = INVALID_HANDLE_VALUE;
				return h;
			}

			ActionStatus Crop()
			{
				if (SetEndOfFile(m_hFile))
					return MAKE_STATUS(Success);
				else
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
			}

			ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime)
			{
				FILETIME creation, lastWrite, lastRead;
				if (!GetFileTime(m_hFile,
					pCreationTime ? &creation : NULL,
					pLastReadTime ? &lastRead : NULL,
					pLastWriteTime ? &lastWrite : NULL))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));

				if (pCreationTime)
					*pCreationTime = creation;
				if (pLastWriteTime)
					*pLastWriteTime = lastWrite;
				if (pLastReadTime)
					*pLastReadTime = lastRead;
				return MAKE_STATUS(Success);
			}

			ActionStatus SetFileTimes(const DateTime *pCreationTime, const DateTime *pLastWriteTime, const DateTime *pLastReadTime)
			{
				if (!SetFileTime(m_hFile,
					pCreationTime ? pCreationTime->GetFileTime(): NULL,
					pLastReadTime ? pLastReadTime->GetFileTime() : NULL,
					pLastWriteTime ? pLastWriteTime->GetFileTime() : NULL))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			bool AsynchronousIOSupported()
			{
				return false;
			}

			size_t ReadAt(void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL)
			{
				ASSIGN_STATUS(pStatus, NotSupported);
				return 0;
			}

			size_t WriteAt(const void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL) 
			{
				ASSIGN_STATUS(pStatus, NotSupported);
				return 0;
			}

			DWORD DeviceIoControl(DWORD dwIoControlCode,
				LPCVOID lpInBuffer = NULL,
				DWORD nInBufferSize = 0,
				LPVOID lpOutBuffer = NULL,
				DWORD nOutBufferSize = 0,
				ActionStatus *pStatus = NULL)
			{
				DWORD done = 0;
				if (!::DeviceIoControl(m_hFile, dwIoControlCode, (LPVOID)lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, &done, NULL))
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
					return 0;
				}
				ASSIGN_STATUS(pStatus, Success);
				return done;
			}

			ActionStatus DeviceIoControlNoBuffers(DWORD dwIoControlCode)
			{
				DWORD done = 0;
				if (!::DeviceIoControl(m_hFile, dwIoControlCode, NULL, 0, NULL, 0, &done, NULL))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

#pragma region Security-related methods
		public:
			Security::Sid GetOwner()
			{
				Security::Sid sid;
				PSID pOwner = NULL;
				PSECURITY_DESCRIPTOR pDesc = NULL;
				::GetSecurityInfo(m_hFile, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pOwner, NULL, NULL, NULL, &pDesc);
				if (!pDesc)
					return sid;
				sid = pOwner;
				LocalFree(pDesc);
				return sid;
			}

			ActionStatus SetOwner(const Security::Sid &NewOwner)
			{
				if (!::SetSecurityInfo(m_hFile, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, NewOwner, NULL, NULL, NULL))
					return MAKE_STATUS(Success);
				else
					return MAKE_STATUS(ActionStatus::FailFromLastError());
			}

			Security::TranslatedAcl GetDACL()
			{
				Security::TranslatedAcl acl;
				PACL pAcl = NULL;
				PSECURITY_DESCRIPTOR pDesc = NULL;
				::GetSecurityInfo(m_hFile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pAcl, NULL, &pDesc);
				if (!pDesc)
					return acl;
				acl = pAcl;
				LocalFree(pDesc);
				return acl;
			}

			static Security::TranslatedAcl GetDACLForPath(const BazisLib::String &path, BazisLib::ActionStatus *pStatus = NULL)
			{
				Security::TranslatedAcl acl;
				PACL pAcl = NULL;
				PSECURITY_DESCRIPTOR pDesc = NULL;
				::GetNamedSecurityInfo((LPCWSTR)path.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pAcl, NULL, &pDesc);
				if (!pDesc)
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError());
					return acl;
				}
				acl = pAcl;
				ASSIGN_STATUS(pStatus, Success);
				LocalFree(pDesc);
				return acl;
			}

			ActionStatus SetDACL(Security::TranslatedAcl &NewDACL)
			{
				if (!::SetSecurityInfo(m_hFile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NewDACL.BuildNTAcl(), NULL))
					return MAKE_STATUS(Success);
				else
					return MAKE_STATUS(ActionStatus::FailFromLastError());
			}

			static ActionStatus SetDACLForPath(const BazisLib::String &path, Security::TranslatedAcl &NewDACL)
			{
				if (!::SetNamedSecurityInfo(const_cast<LPTSTR>(path.c_str()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NewDACL.BuildNTAcl(), NULL))
					return MAKE_STATUS(Success);
				else
					return MAKE_STATUS(ActionStatus::FailFromLastError());
			}

			ActionStatus TakeOwnership()
			{
				return SetOwner(Security::Sid::CurrentUserSid());
			}
#pragma endregion

			FileFlags::FileAttribute GetFileAttributes(ActionStatus *pStatus = NULL) {ASSIGN_STATUS(pStatus, NotSupported); return FileFlags::NormalFile;}
			ActionStatus AddFileAttributes(FileFlags::FileAttribute Attributes) {return MAKE_STATUS(NotSupported);}
			ActionStatus RemoveFileAttributes(FileFlags::FileAttribute Attributes) {return MAKE_STATUS(NotSupported);}

			static bool Exists(LPCTSTR Path)
			{
				HANDLE hFile = CreateFile(Path,
					0,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					0,
					0);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					return ::GetFileAttributes(Path) != INVALID_FILE_ATTRIBUTES;
				}
				CloseHandle(hFile);
				return true;
			}

			static ActionStatus Delete(LPCTSTR Path, bool ForceDeleteReadonly = false)
			{
				if (ForceDeleteReadonly)
				{
					DWORD dwAttr = ::GetFileAttributes(Path);
					if (dwAttr == INVALID_FILE_ATTRIBUTES)
						return MAKE_STATUS(InvalidParameter);
					if (dwAttr & FILE_ATTRIBUTE_READONLY)
						dwAttr &= ~FILE_ATTRIBUTE_READONLY;
					::SetFileAttributes(Path, dwAttr);
				}
				if (!::DeleteFile(Path))
					return MAKE_STATUS(ActionStatus::FailFromLastError());
				else
					return MAKE_STATUS(Success);
			}
		};

		class AsyncFile : File
		{
		private:
			ULONGLONG m_Offset;

		public:
			AsyncFile(const TCHAR *ptszFileName, const FileMode &mode, ActionStatus *pStatus = NULL, PSECURITY_ATTRIBUTES pSecAttributes = NULL)
				: File(ptszFileName, mode.AddRawFlags(FILE_FLAG_OVERLAPPED), pStatus, pSecAttributes)
			{
			}

			AsyncFile(const String &FileName, const FileMode &mode, ActionStatus *pStatus = NULL, PSECURITY_ATTRIBUTES pSecAttributes = NULL)
				: File(FileName, mode.AddRawFlags(FILE_FLAG_OVERLAPPED), pStatus, pSecAttributes)
			{
			}

			LONGLONG Seek(LONGLONG Offset, FileFlags::SeekType seekType, ActionStatus *pStatus = NULL)
			{
				return FileHelpers::DoSeek(&m_Offset, GetSize(), Offset, seekType, false, pStatus);
			}

			ActionStatus Crop()
			{
				File::Seek(m_Offset, FileFlags::FileBegin);
				return File::Crop();
			}

			size_t Read(void *pBuffer, size_t size, ActionStatus *pStatus = NULL, bool IncompleteReadSupported = false)
			{
				size_t done = ReadAt(pBuffer, m_Offset, size, NULL, pStatus);
				m_Offset += done;
				return done;
			}

			size_t Write(const void *pBuffer, size_t size, ActionStatus *pStatus = NULL)
			{
				size_t done = WriteAt(pBuffer, m_Offset, size, NULL, pStatus);
				m_Offset += done;
				return done;
			}

			bool AsynchronousIOSupported()
			{
				return true;
			}

			size_t ReadAt(void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL)
			{
				if (pCompletion)
				{
					ASSIGN_STATUS(pStatus, NotSupported);
					return 0;
				}
				DWORD done = 0;
				OVERLAPPED over = {0,};
				over.Offset = (DWORD)(offset);
				over.OffsetHigh = (DWORD)(offset >> 32);
				ReadFile(GetHandleForSingleUse(), pBuffer, (DWORD)size, &done, &over);
				GetOverlappedResult(GetHandleForSingleUse(), &over, &done, TRUE);
				if (done != size)
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
					return done;
				}
				ASSIGN_STATUS(pStatus, Success);
				return done;
			}

			size_t WriteAt(const void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL) 
			{
				if (pCompletion)
				{
					ASSIGN_STATUS(pStatus, NotSupported);
					return 0;
				}
				DWORD done = 0;
				OVERLAPPED over = {0,};
				over.Offset = (DWORD)(offset);
				over.OffsetHigh = (DWORD)(offset >> 32);
				WriteFile(GetHandleForSingleUse(), pBuffer, (DWORD)size, &done, &over);
				GetOverlappedResult(GetHandleForSingleUse(), &over, &done, TRUE);
				if (done != size)
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
					return done;
				}
				ASSIGN_STATUS(pStatus, Success);
				return done;
			}

			//TODO: implement
			virtual FileFlags::FileAttribute GetFileAttributes(ActionStatus *pStatus = NULL) {ASSIGN_STATUS(pStatus, NotSupported); return FileFlags::NormalFile;}
			virtual ActionStatus AddFileAttributes(FileFlags::FileAttribute Attributes) {return MAKE_STATUS(NotSupported);}
			virtual ActionStatus RemoveFileAttributes(FileFlags::FileAttribute Attributes) {return MAKE_STATUS(NotSupported);}
		};

		class Directory
		{
		private:
			String m_DirPath;

		public:
			class enumerator;

			class FindFileParams
			{
				String DirPath;
				String FileMask;

				friend class enumerator;
				friend class Directory;
			};

			class enumerator
			{
			private:
				WIN32_FIND_DATA m_FindData;
				HANDLE m_hFind;
				String m_BasePath;

			private:
				enumerator(const enumerator &iter) { ASSERT(FALSE); }
				void operator=(enumerator &iter) { ASSERT(FALSE); }

			public:
				enumerator(const FindFileParams &params) 
				  : m_BasePath(params.DirPath)
				{
					String findArg = params.DirPath + _T("\\") + ((params.FileMask.size() == 0) ? _T("*.*") : params.FileMask);
					m_hFind = ::FindFirstFile(findArg.c_str(), &m_FindData);
				}

				~enumerator()
				{
					if (m_hFind != INVALID_HANDLE_VALUE)
#ifdef UNDER_CE
						CloseHandle(m_hFind);
#else
						FindClose(m_hFind);
#endif
				}

				bool Valid() const
				{
					return (m_hFind != INVALID_HANDLE_VALUE) && (m_hFind != 0);
				}

				String GetFullPath() const
				{
					if (!Valid())
						return String();
					return Path::Combine(m_BasePath, m_FindData.cFileName);
				}

				String GetRelativePath() const
				{
					if (!Valid())
						return String();
					return m_FindData.cFileName;
				}

				bool IsDirectory() const
				{
					if (!Valid())
						return false;
					return ((m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
				}

				bool IsAPseudoEntry() const
				{
					return (m_FindData.cFileName[0] == '.') && ((m_FindData.cFileName[1] == '.') || !m_FindData.cFileName[1]);
				}


				FileFlags::FileAttribute GetAttributes(ActionStatus *pStatus) const
				{
					if (!Valid())
					{
						ASSIGN_STATUS(pStatus, FileNotFound);
						return FileFlags::InvalidAttr;
					}
					ASSIGN_STATUS(pStatus, Success);
					return (FileFlags::FileAttribute)m_FindData.dwFileAttributes;
				}

				ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime) const
				{
					if (!Valid())
						return MAKE_STATUS(FileNotFound);
					if (pCreationTime)
						*pCreationTime = m_FindData.ftCreationTime;
					if (pLastWriteTime)
						*pLastWriteTime = m_FindData.ftLastWriteTime;
					if (pLastReadTime)
						*pLastReadTime = m_FindData.ftLastAccessTime;
					return MAKE_STATUS(Success);
				}

				LONGLONG GetSize(ActionStatus *pStatus = NULL) const
				{
					if (!Valid())
					{
						ASSIGN_STATUS(pStatus, FileNotFound);
						return -1LL;
					}
					ASSIGN_STATUS(pStatus, Success);
					return (((ULONGLONG)m_FindData.nFileSizeHigh) << 32) | m_FindData.nFileSizeLow;
				}

				ActionStatus Next()
				{
					if (!Valid())
						return MAKE_STATUS(FileNotFound);
					if (::FindNextFile(m_hFind, &m_FindData))
						return MAKE_STATUS(Success);
					FindClose(m_hFind);
					m_hFind = INVALID_HANDLE_VALUE;
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				}
			};

		public:
			Directory(const String &Path)
				: m_DirPath(Path)
			{
			}

			String RelativeToAbsolute(const String &path)
			{
				return Path::Combine(m_DirPath, path);
			}

			ActionStatus ChangeDirectory(const String &Path)
			{
				String newPath = RelativeToAbsolute(Path);
				DWORD dwAttr = ::GetFileAttributes(newPath.c_str());
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
					return MAKE_STATUS(InvalidParameter);
				m_DirPath = newPath;
				return MAKE_STATUS(Success);
			}

			FileFlags::FileAttribute GetFileAttributes(const String &Path, ActionStatus *pStatus = NULL)
			{
				String filePath = RelativeToAbsolute(Path);
				DWORD dwAttr = ::GetFileAttributes(filePath.c_str());
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
					return FileFlags::InvalidAttr;
				}
				return (FileFlags::FileAttribute)dwAttr;
			}

			ActionStatus AddFileAttributes(const String &Path, FileFlags::FileAttribute Attributes)
			{
				String filePath = RelativeToAbsolute(Path);
				const String::value_type *ptszFilePath = filePath.c_str();
				DWORD dwAttr = ::GetFileAttributes(ptszFilePath);
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				if (!::SetFileAttributes(ptszFilePath, dwAttr | Attributes))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			ActionStatus RemoveFileAttributes(const String &Path, FileFlags::FileAttribute Attributes)
			{
				String filePath = RelativeToAbsolute(Path);
				const String::value_type *ptszFilePath = filePath.c_str();
				DWORD dwAttr = ::GetFileAttributes(ptszFilePath);
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				if (!::SetFileAttributes(ptszFilePath, dwAttr & ~Attributes))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			ActionStatus CreateDirectory(const String &Path)
			{
				String dirPath = RelativeToAbsolute(Path);
				if (!::CreateDirectory(dirPath.c_str(), NULL))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			ActionStatus DeleteFile(const String &Path)
			{
				String filePath = RelativeToAbsolute(Path);
				if (!::DeleteFile(filePath.c_str()))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			ActionStatus RemoveDirectory(const String &Path)
			{
				String dirPath = RelativeToAbsolute(Path);
				if (!::RemoveDirectory(dirPath.c_str()))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			ActionStatus MoveFile(const String &ExistingPath, const String &NewPath, bool DeleteIfExists = false)
			{
				String srcPath = RelativeToAbsolute(ExistingPath);
				String dstPath = RelativeToAbsolute(NewPath);
				if (!DeleteIfExists && File::Exists(dstPath.c_str()))
					return MAKE_STATUS(AlreadyExists);
				if (!::MoveFile(srcPath.c_str(), dstPath.c_str()))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			FindFileParams FindFirstFile(const String &FileMask = _T(""))
			{
				FindFileParams params;
				params.DirPath = m_DirPath;
				params.FileMask = FileMask;
				return params;
			}

			static bool Exists(const String &Path)
			{
				DWORD attr = ::GetFileAttributes(Path.c_str());
				if (attr == INVALID_FILE_ATTRIBUTES)
					return false;
				if (attr & FILE_ATTRIBUTE_DIRECTORY)
					return true;
				return false;
			}

			static ActionStatus Create(const String &Path, bool recursive = true)
			{
				if (!::CreateDirectory(Path.c_str(), NULL))
				{
					ActionStatus st = MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
					if (recursive && st.ConvertToHResult() == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
					{
						String parentPath = Path::GetDirectoryName(Path);
						if (parentPath.empty() || parentPath == Path)
							return st;
						st = Create(parentPath);
						if (!st.Successful())
							return st;
						if (!::CreateDirectory(Path.c_str(), NULL))
							return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
						else
							return MAKE_STATUS(Success);
					}
					return st;
				}
				return MAKE_STATUS(Success);
			}

			static ActionStatus Remove(const String &Path)
			{
				if (!::RemoveDirectory(Path.c_str()))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			TempString &GetFullPath()
			{
				return m_DirPath;
			}

		};
	}


	typedef Win32::FileMode FileMode;
	typedef Win32::File PlatformSpecificFile;
	typedef Win32::Directory PlatformSpecificDirectory;

	namespace FileModes
	{
		static const FileMode CreateOrOpenRW		= {GENERIC_READ | GENERIC_WRITE,		  FILE_SHARE_READ, OPEN_ALWAYS,   FILE_ATTRIBUTE_NORMAL};
		static const FileMode CreateOrTruncateRW	= {GENERIC_READ | GENERIC_WRITE,		  FILE_SHARE_READ, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL};
		static const FileMode CreateNewRW			= {GENERIC_READ | GENERIC_WRITE,		  FILE_SHARE_READ, CREATE_NEW,	  FILE_ATTRIBUTE_NORMAL};
		static const FileMode OpenReadOnly			= {GENERIC_READ,						  FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL};
		static const FileMode OpenReadWrite			= {GENERIC_READ | GENERIC_WRITE,		  FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL};
		static const FileMode OpenForSecurityQuery	= {READ_CONTROL | ACCESS_SYSTEM_SECURITY, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL};
		static const FileMode OpenForSecurityModify	= {READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL};

		static inline FileMode FromFileFlags(FileFlags::FileAccess Access = FileFlags::ReadAccess,
			FileFlags::OpenMode OpenMode = FileFlags::OpenExisting,
			FileFlags::ShareMode ShareMode = FileFlags::ShareRead,
			FileFlags::FileAttribute Attributes = FileFlags::NormalFile)
		{
			FileMode mode = {0,};
#pragma region Set access mode
			if ((Access & FileFlags::QueryAccess) == FileFlags::QueryAccess)
				mode.dwDesiredAccess |= FILE_READ_ATTRIBUTES;
			if ((Access & FileFlags::ReadAccess) == FileFlags::ReadAccess)
				mode.dwDesiredAccess |= GENERIC_READ;
			if ((Access & FileFlags::WriteAccess) == FileFlags::WriteAccess)
				mode.dwDesiredAccess |= GENERIC_WRITE;
			if ((Access & FileFlags::ReadWriteAccess) == FileFlags::ReadWriteAccess)
				mode.dwDesiredAccess |= GENERIC_READ | GENERIC_WRITE;
			if ((Access & FileFlags::DeleteAccess) == FileFlags::DeleteAccess)
				mode.dwDesiredAccess |= DELETE;
			if ((Access & FileFlags::AllAccess) == FileFlags::AllAccess)
				mode.dwDesiredAccess |= GENERIC_ALL;
			if ((Access & FileFlags::ReadSecurity) == FileFlags::ReadSecurity)
				mode.dwDesiredAccess |= READ_CONTROL | ACCESS_SYSTEM_SECURITY;
			if ((Access & FileFlags::ModifySecurity) == FileFlags::ModifySecurity)
				mode.dwDesiredAccess |= READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY;
#pragma endregion
#pragma region Set share mode
			if ((ShareMode & FileFlags::ShareRead) == FileFlags::ShareRead)
				mode.dwShareMode |= FILE_SHARE_READ;
			if ((ShareMode & FileFlags::ShareWrite) == FileFlags::ShareWrite)
				mode.dwShareMode |= FILE_SHARE_WRITE;
			if ((ShareMode & FileFlags::ShareDelete) == FileFlags::ShareDelete)
				mode.dwShareMode |= FILE_SHARE_DELETE;
			if ((ShareMode & FileFlags::ShareAll) == FileFlags::ShareAll)
				mode.dwShareMode |= FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
#pragma endregion
#pragma region Set open mode
			switch(OpenMode)
			{
			case FileFlags::OpenExisting:
				mode.dwCreationDisposition = OPEN_EXISTING;
				break;
			case FileFlags::OpenAlways:
				mode.dwCreationDisposition = OPEN_ALWAYS;
				break;
			case FileFlags::CreateAlways:
				mode.dwCreationDisposition = CREATE_ALWAYS;
				break;
			case FileFlags::CreateNew:
				mode.dwCreationDisposition = CREATE_NEW;
				break;
			}
#pragma endregion

			mode.dwFlagsAndAttributes |= (DWORD)Attributes;
			return mode;
		}
	}
}