#pragma once
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>

namespace BazisLib
{
	class IoCompletion;

	namespace FileFlags
	{
		enum SeekType
		{
			FileBegin	= SEEK_SET,
			FileCurrent = SEEK_CUR,
			FileEnd		= SEEK_END,
		};

		enum FileAttribute
		{
			NormalFile,
			InvalidAttr	= -1,
		};
	}
}

#include "../filehlp.h"

namespace BazisLib
{
	namespace Posix
	{
		struct FileMode
		{
			int Flags;
			int AccessMask;

			FileMode AddRawFlags(int flags) const
			{
				FileMode copy = *this;
				copy.Flags |= flags;
				return copy;
			}
		};

		class File
		{
		private:
			int m_hFile;

		private:
			File(const File &anotherFile) {ASSERT(FALSE);}
			void operator=(const File &anotherFile) {ASSERT(FALSE);}

		public:
			static void ExportTimespec(const timespec &ts, DateTime *pTimestamp)
			{
				if (pTimestamp)
				{
					timeval tv = {0,};
					TIMESPEC_TO_TIMEVAL(&tv, &ts);
					*pTimestamp = tv;
				}
			}

		public:
			File(const TCHAR *ptszFileName, const FileMode &mode, ActionStatus *pStatus = NULL)
				: m_hFile(-1)
			{
				m_hFile = open(ptszFileName, mode.Flags, mode.AccessMask);
				if ((m_hFile == -1 || m_hFile == 0) && pStatus)
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
				else
					ASSIGN_STATUS(pStatus, Success);
			}

			File(const String &FileName, const FileMode &mode, ActionStatus *pStatus = NULL)
				: m_hFile(-1)
			{
				m_hFile = open(FileName.c_str(), mode.Flags, mode.AccessMask);
				if ((m_hFile == -1 || m_hFile == 0) && pStatus)
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
				else
					ASSIGN_STATUS(pStatus, Success);
			}

			File()
				: m_hFile(-1)
			{
			}

			~File()
			{
				if (m_hFile != -1)
					close(m_hFile);
			}

			int GetHandleForSingleUse() {return m_hFile;}

			size_t Read(void *pBuffer, size_t size, ActionStatus *pStatus = NULL, bool IncompleteReadSupported = false)
			{
				ssize_t done = read(m_hFile, pBuffer, size);
				if (done != -1)
				{
					ASSIGN_STATUS(pStatus, Success);
					return (size_t)done;
				}
				else
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
					return 0;
				}
			}

			size_t Write(const void *pBuffer, size_t size, ActionStatus *pStatus = NULL)
			{
				ssize_t done = write(m_hFile, pBuffer, size);
				if (done != -1)
				{
					ASSIGN_STATUS(pStatus, Success);
					return (size_t)done;
				}
				else
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
					return 0;
				}
			}

#ifdef __linux
			typedef struct stat64 StatStruct;

			ActionStatus Stat(StatStruct *pStatData)
			{
				if (fstat64(m_hFile, pStatData) != 0)
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}
#else
			typedef struct stat StatStruct;

			ActionStatus Stat(StatStruct *pStatData)
			{
				if (fstat(m_hFile, pStatData) != 0)
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}
#endif

			LONGLONG GetSize(ActionStatus *pStatus = NULL)
			{
				StatStruct statData;
				ActionStatus st = Stat(&statData);
				if (!st.Successful())
				{
					if (pStatus)
						*pStatus = st;
					return -1LL;
				}
				ASSIGN_STATUS(pStatus, Success);
				C_ASSERT(sizeof(statData.st_size) == 8);
				return statData.st_size;
			}

			LONGLONG Seek(LONGLONG Offset, FileFlags::SeekType seekType = FileFlags::FileBegin, ActionStatus *pStatus = NULL)
			{
#ifndef __linux__
				C_ASSERT(sizeof(lseek(0,0,0)) == 8);
				off_t result = lseek(m_hFile, Offset, (int)seekType);
#else
				C_ASSERT(sizeof(lseek64(0,0,0)) == 8);
				off64_t result = lseek64(m_hFile, Offset, (int)seekType);
#endif
				if ((result == -1) || (result == -1LL))
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
					return -1LL;
				}

				ASSIGN_STATUS(pStatus, Success);
				return result;
			}

			LONGLONG GetPosition(ActionStatus *pStatus = NULL)
			{
				off_t result = lseek(m_hFile, 0, SEEK_CUR);
				if ((result == -1) || (result == -1LL))
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
					return -1LL;
				}

				ASSIGN_STATUS(pStatus, Success);
				return result;
			}

			void Close()
			{
				if (m_hFile == -1 || !m_hFile)
					return;
				close(m_hFile);
				m_hFile = -1;
			}

			bool Valid()
			{
				return (m_hFile != -1) && m_hFile;
			}

			int DetachHandle()
			{
				int h = m_hFile;
				m_hFile = -1;
				return h;
			}

			ActionStatus Crop()
			{
				ActionStatus st;
				LONGLONG off = GetPosition(&st);
				if (!st.Successful())
					return st;
				ftruncate(m_hFile, off);
				return MAKE_STATUS(Success);
			}

#ifdef __linux__
#define tsid(x) x
#else
#define tsid(x) x ## espec
#endif

			ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime)
			{
				StatStruct statData;
				ActionStatus st = Stat(&statData);
				if (!st.Successful())
					return st;
				ExportTimespec(statData.tsid(st_ctim), pCreationTime);
				ExportTimespec(statData.tsid(st_atim), pLastWriteTime);
				ExportTimespec(statData.tsid(st_mtim), pLastReadTime);
				return MAKE_STATUS(Success);
			}

			ActionStatus SetFileTimes(const DateTime *pCreationTime, const DateTime *pLastWriteTime, const DateTime *pLastReadTime)
			{
				if (pCreationTime)
					return MAKE_STATUS(NotSupported);
				timeval times[2];	//[0] - last access, [1] - last modification
				memset(&times, 0, sizeof(times));
				if (pLastReadTime)
					times[0] = pLastReadTime->ToTimeval();
				if (pLastWriteTime)
					times[1] = pLastWriteTime->ToTimeval();

				if (!pLastWriteTime || !pLastReadTime)
				{
					StatStruct statVal;
					ActionStatus st = Stat(&statVal);
					if (!st.Successful())
						return st;
					if (!pLastReadTime)
						TIMESPEC_TO_TIMEVAL(&times[0], &statVal.tsid(st_atim));
					if (!pLastWriteTime)
						TIMESPEC_TO_TIMEVAL(&times[1], &statVal.tsid(st_mtim));
				}
				if (futimes(m_hFile, times) == -1)
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				else
					return MAKE_STATUS(Success);
			}

#undef tsid

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

			FileFlags::FileAttribute GetFileAttributes(ActionStatus *pStatus = NULL) {ASSIGN_STATUS(pStatus, NotSupported); return FileFlags::NormalFile;}
			ActionStatus AddFileAttributes(FileFlags::FileAttribute Attributes) {return MAKE_STATUS(NotSupported);}
			ActionStatus RemoveFileAttributes(FileFlags::FileAttribute Attributes) {return MAKE_STATUS(NotSupported);}

			static bool Exists(const TCHAR *Path)
			{
				struct stat stData;
				int result = stat(Path, &stData);
				return !result;
			}

			static ActionStatus Delete(const TCHAR *Path, bool ForceDeleteReadonly = false)
			{
				int result = unlink(Path);
				if (!result)
					return MAKE_STATUS(Success);
				else
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
			}
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
				String m_BasePath;
				struct dirent m_Entry;
				bool m_Valid;
				DIR *m_pDir;
				File::StatStruct _StatStruct;

			private:
				enumerator(const enumerator &iter) { ASSERT(FALSE); }
				void operator=(enumerator &iter) { ASSERT(FALSE); }

			public:
				enumerator(const FindFileParams &params) 
				  : m_BasePath(params.DirPath)
				  , m_Valid(false)
				  , m_pDir(NULL)
				{
					m_pDir = opendir(params.DirPath.c_str());
					if (!m_pDir)
						return;
					if (Next().Successful())
						m_Valid = true;
				}

				~enumerator()
				{
					if (m_pDir)
						closedir(m_pDir);
				}

				bool Valid() const
				{
					return m_Valid;
				}

				String GetFullPath() const
				{
					return Path::Combine(m_BasePath, m_Entry.d_name);
				}

				String GetRelativePath() const
				{
					return m_Entry.d_name;
				}

				bool IsDirectory() const
				{
					return (_StatStruct.st_mode & S_IFMT) == S_IFDIR;
				}

				bool IsAPseudoEntry() const
				{
					if (m_Entry.d_name[0] == '.' && (m_Entry.d_name[1] == '.' || m_Entry.d_name[1] == 0))
						return true;
					return false;
				}

				FileFlags::FileAttribute GetAttributes(ActionStatus *pStatus) const
				{
					ASSIGN_STATUS(pStatus, NotSupported);
					return FileFlags::InvalidAttr;
				}

#ifdef __linux__
#define tsid(x) x
#else
#define tsid(x) x ## espec
#endif

				ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime) const
				{
					if (!m_Valid)
						return MAKE_STATUS(NotInitialized);
					File::ExportTimespec(_StatStruct.tsid(st_ctim), pCreationTime);
					File::ExportTimespec(_StatStruct.tsid(st_atim), pLastWriteTime);
					File::ExportTimespec(_StatStruct.tsid(st_mtim), pLastReadTime);
					return MAKE_STATUS(Success);
				}

#undef tsid

				LONGLONG GetSize(ActionStatus *pStatus) const
				{
					if (!m_Valid)
					{
						ASSIGN_STATUS(pStatus, NotInitialized);
						return -1LL;
					}
					ASSIGN_STATUS(pStatus, Success);
					C_ASSERT(sizeof(_StatStruct.st_size) == 8);
					return _StatStruct.st_size;
				}

				ActionStatus Next()
				{
					struct dirent *pDir = NULL;
					int result = readdir_r(m_pDir, &m_Entry, &pDir);
					if (result == -1 || !pDir)
					{
						m_Valid = false;
						return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
					}

#ifdef __linux__
					 result = stat64(Path::Combine(m_BasePath, pDir->d_name).c_str(), &_StatStruct);
#else
					result = stat(Path::Combine(m_BasePath, pDir->d_name).c_str(), &_StatStruct);
#endif
					if (result == -1)
					{
						m_Valid = false;
						return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
					}

					return MAKE_STATUS(Success);
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
				String newDir = Path::Combine(m_DirPath, Path);
				if (!Directory::Exists(newDir))
					return MAKE_STATUS(FileNotFound);
				m_DirPath = newDir;
				return MAKE_STATUS(Success);
			}

			FileFlags::FileAttribute GetFileAttributes(const String &Path, ActionStatus *pStatus = NULL)
			{
				ASSIGN_STATUS(pStatus, NotSupported);
				return FileFlags::InvalidAttr;
			}

			ActionStatus AddFileAttributes(const String &Path, FileFlags::FileAttribute Attributes)
			{
				return MAKE_STATUS(NotSupported);
			}

			ActionStatus RemoveFileAttributes(const String &Path, FileFlags::FileAttribute Attributes)
			{
				return MAKE_STATUS(NotSupported);
			}

			ActionStatus CreateDirectory(const String &Path)
			{
				String newPath = Path::Combine(m_DirPath, Path);
				return Create(newPath);
			}

			ActionStatus DeleteFile(const String &Path)
			{
				String newPath = Path::Combine(m_DirPath, Path);
				return File::Delete(newPath.c_str());
			}

			ActionStatus RemoveDirectory(const String &Path)
			{
				String newPath = Path::Combine(m_DirPath, Path);
				return Remove(newPath);
			}

			ActionStatus MoveFile(const String &ExistingPath, const String &NewPath, bool DeleteIfExists = false)
			{
				String fullOld = Path::Combine(m_DirPath, ExistingPath);
				String fullNew = Path::Combine(m_DirPath, NewPath);

				if (!File::Exists(fullOld.c_str()))
					return MAKE_STATUS(FileNotFound);

				if (File::Exists(fullNew.c_str()))
				{
					if (DeleteIfExists)
					{
						ActionStatus st = File::Delete(fullNew.c_str());
						if (!st.Successful())
							return st;
					}
					else
						return MAKE_STATUS(AlreadyExists);
				}

				int result = rename(fullOld.c_str(), fullNew.c_str());
				if (result == 0)
					return MAKE_STATUS(Success);
				else
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
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
				struct stat statData;
				int result = stat(Path.c_str(), &statData);
				if (result == -1)
					return false;
				return (statData.st_mode & S_IFMT) == S_IFDIR;
			}

			static ActionStatus Create(const String &Path)
			{
				int result = mkdir(Path.c_str(), 0755);
				if (result == 0)
					return MAKE_STATUS(Success);
				else
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
			}

			static ActionStatus Remove(const String &Path)
			{
				int result = rmdir(Path.c_str());
				if (result == 0)
					return MAKE_STATUS(Success);
				else
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
			}

			TempString &GetFullPath()
			{
				return m_DirPath;
			}

		};
	}


	typedef Posix::FileMode FileMode;
	typedef Posix::File PlatformSpecificFile;
	typedef Posix::Directory PlatformSpecificDirectory;

	namespace FileModes
	{
		enum {kDefaultFileMode = 0755};

		static const FileMode CreateOrOpenRW		= {O_CREAT | O_RDWR, kDefaultFileMode};
		static const FileMode CreateOrTruncateRW	= {O_CREAT | O_RDWR | O_TRUNC, kDefaultFileMode};
		static const FileMode CreateNewRW			= {O_CREAT | O_EXCL | O_RDWR, kDefaultFileMode};
		static const FileMode OpenReadOnly			= {O_RDONLY, kDefaultFileMode};
		static const FileMode OpenReadWrite			= {O_RDWR, kDefaultFileMode};
		static const FileMode OpenForSecurityQuery	= {O_RDONLY, kDefaultFileMode};
		static const FileMode OpenForSecurityModify	= {O_RDWR, kDefaultFileMode};

		static inline FileMode FromFileFlags(FileFlags::FileAccess Access = FileFlags::ReadAccess,
			FileFlags::OpenMode OpenMode = FileFlags::OpenExisting,
			FileFlags::ShareMode ShareMode = FileFlags::ShareRead,
			FileFlags::FileAttribute Attributes = FileFlags::NormalFile)
		{
			FileMode mode = {0,};
#pragma region Set access mode
			if (Access & (FileFlags::ReadAccess | FileFlags::WriteAccess | FileFlags::ReadWriteAccess | FileFlags::DeleteAccess | FileFlags::ModifySecurity))
				mode.Flags = O_RDWR;
			else if (Access & (FileFlags::ReadAccess | FileFlags::WriteAccess))
				mode.Flags = O_RDONLY;
#pragma endregion
#pragma region Set share mode
#pragma endregion
#pragma region Set open mode
			switch(OpenMode)
			{
			case FileFlags::OpenExisting:
				break;
			case FileFlags::OpenAlways:
				mode.Flags |= O_CREAT;
				break;
			case FileFlags::CreateAlways:
				mode.Flags |= O_CREAT | O_TRUNC;
				break;
			case FileFlags::CreateNew:
				mode.Flags |= O_CREAT | O_EXCL;
				break;
			}
#pragma endregion
			mode.AccessMask = kDefaultFileMode;
			return mode;
		}
	}
}