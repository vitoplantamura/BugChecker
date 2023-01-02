#pragma once

namespace BazisLib
{
	class IoCompletion;

	namespace FileFlags
	{
		enum SeekType
		{
			FileBegin	= 0,
			FileCurrent = 1,
			FileEnd		= 2,
		};

		enum FileAttribute
		{
			NormalFile	= FILE_ATTRIBUTE_NORMAL,
			Hidden		= FILE_ATTRIBUTE_HIDDEN,
			ReadOnly	= FILE_ATTRIBUTE_READONLY,
			System		= FILE_ATTRIBUTE_SYSTEM,
			Archive		= FILE_ATTRIBUTE_ARCHIVE,
			DirectoryFile	= FILE_ATTRIBUTE_DIRECTORY,
			InvalidAttr	= -1
		};
	}
}

#include "../filehlp.h"
#include "fileapi.h"

namespace BazisLib
{
	namespace DDK
	{
		struct FileMode
		{
			ACCESS_MASK DesiredAccess;
			ULONG FileAttributes;
			ULONG ShareAccess;
			ULONG CreateDisposition;
			ULONG CreateOptions;

			ULONG ObjectAttributes;

			FileMode ShareAll() const
			{
				FileMode copy = *this;
				copy.ShareAccess |= FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
				return copy;
			}

			FileMode AddAttributes(BazisLib::FileFlags::FileAttribute attr) const
			{
				FileMode copy = *this;
				copy.FileAttributes |= (ULONG)attr;
				return copy;
			}

			FileMode AddCreateOptions(ULONG flags) const
			{
				FileMode copy = *this;
				copy.CreateOptions |= flags;
				return copy;
			}
		};
	}

	typedef DDK::FileMode FileMode;

	namespace FileModes
	{
		enum {kDefaultOptions = FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT};
		enum {kDefaultObjectAttributes = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE};

		static const FileMode CreateOrOpenRW		= {SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,		  FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN_IF, kDefaultOptions, kDefaultObjectAttributes};
		static const FileMode CreateOrTruncateRW	= {SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,		  FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_SUPERSEDE, kDefaultOptions, kDefaultObjectAttributes };
		static const FileMode CreateNewRW			= {SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,		  FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_CREATE, kDefaultOptions, kDefaultObjectAttributes };
		static const FileMode OpenReadOnly			= {SYNCHRONIZE | GENERIC_READ,						  FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, kDefaultOptions, kDefaultObjectAttributes };
		static const FileMode OpenReadWrite			= {SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,		  FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, kDefaultOptions, kDefaultObjectAttributes };
		static const FileMode OpenForSecurityQuery	= {READ_CONTROL | ACCESS_SYSTEM_SECURITY, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, kDefaultOptions, kDefaultObjectAttributes };
		static const FileMode OpenForSecurityModify	= {READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, kDefaultOptions, kDefaultObjectAttributes };

		static const FileMode OpenToModifyAttributes	= {SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN, kDefaultOptions, kDefaultObjectAttributes };
		static const FileMode OpenToQueryAttributes	= {SYNCHRONIZE | FILE_READ_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN, kDefaultOptions, kDefaultObjectAttributes };

		static const FileMode OpenDirectoryForQuery	= {SYNCHRONIZE | DIRECTORY_QUERY, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE, kDefaultObjectAttributes };

		static inline FileMode FromFileFlags(FileFlags::FileAccess Access = FileFlags::ReadAccess,
			FileFlags::OpenMode OpenMode = FileFlags::OpenExisting,
			FileFlags::ShareMode ShareMode = FileFlags::ShareRead,
			FileFlags::FileAttribute Attributes = FileFlags::NormalFile)
		{
			FileMode mode = {0,};

			mode.CreateOptions = kDefaultOptions;
			mode.ObjectAttributes = kDefaultObjectAttributes;

#pragma region Set access mode
			if ((Access & FileFlags::QueryAccess) == FileFlags::QueryAccess)
				mode.DesiredAccess |= FILE_READ_ATTRIBUTES;
			if ((Access & FileFlags::ReadAccess) == FileFlags::ReadAccess)
				mode.DesiredAccess |= SYNCHRONIZE | GENERIC_READ;
			if ((Access & FileFlags::WriteAccess) == FileFlags::WriteAccess)
				mode.DesiredAccess |= SYNCHRONIZE | GENERIC_WRITE;
			if ((Access & FileFlags::ReadWriteAccess) == FileFlags::ReadWriteAccess)
				mode.DesiredAccess |= SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE;
			if ((Access & FileFlags::DeleteAccess) == FileFlags::DeleteAccess)
				mode.DesiredAccess |= DELETE;
			if ((Access & FileFlags::AllAccess) == FileFlags::AllAccess)
				mode.DesiredAccess |= GENERIC_ALL;
			if ((Access & FileFlags::ReadSecurity) == FileFlags::ReadSecurity)
				mode.DesiredAccess |= READ_CONTROL | ACCESS_SYSTEM_SECURITY;
			if ((Access & FileFlags::ModifySecurity) == FileFlags::ModifySecurity)
				mode.DesiredAccess |= READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY;
#pragma endregion
#pragma region Set share mode
			if ((ShareMode & FileFlags::ShareRead) == FileFlags::ShareRead)
				mode.ShareAccess |= FILE_SHARE_READ;
			if ((ShareMode & FileFlags::ShareWrite) == FileFlags::ShareWrite)
				mode.ShareAccess |= FILE_SHARE_WRITE;
			if ((ShareMode & FileFlags::ShareDelete) == FileFlags::ShareDelete)
				mode.ShareAccess |= FILE_SHARE_DELETE;
			if ((ShareMode & FileFlags::ShareAll) == FileFlags::ShareAll)
				mode.ShareAccess |= FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
#pragma endregion
#pragma region Set open mode
			switch(OpenMode)
			{
			case FileFlags::OpenExisting:
				mode.CreateDisposition = FILE_OPEN;
				break;
			case FileFlags::OpenAlways:
				mode.CreateDisposition = FILE_OPEN_IF;
				break;
			case FileFlags::CreateAlways:
				mode.CreateDisposition = FILE_SUPERSEDE;
				break;
			case FileFlags::CreateNew:
				mode.CreateDisposition = FILE_CREATE;
				break;
			}
#pragma endregion

			mode.FileAttributes = (ULONG)Attributes;
			return mode;
		}
	}

	namespace DDK
	{
		class File
		{
		protected:
			HANDLE m_hFile;

		private:
			File(const File &)
			{
				ASSERT(FALSE);
			}

			void operator=(File &)
			{
				ASSERT(FALSE);
			}

			void Initialize(const String &Path,
				const FileMode &mode,
				ActionStatus *pStatus)
			{
				OBJECT_ATTRIBUTES fileAttributes;
				UNICODE_STRING fileName = Path;
				IO_STATUS_BLOCK SB;

				InitializeObjectAttributes(&fileAttributes, &fileName, mode.ObjectAttributes, NULL, NULL);

				NTSTATUS status = ZwCreateFile(&m_hFile, mode.DesiredAccess, &fileAttributes, &SB, NULL,
					mode.FileAttributes, mode.ShareAccess, mode.CreateDisposition, mode.CreateOptions, NULL, 0);

				if (!NT_SUCCESS(status))
				{
					m_hFile = 0;
					ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(status));
					return;
				}
				ASSIGN_STATUS(pStatus, Success);
			}

		public:
			File() : m_hFile(0)
			{
			}

			File(const String &Path,
				const FileMode &mode,			
				ActionStatus *pStatus = NULL)
			{
				Initialize(Path, mode, pStatus);
			}

			File(HANDLE HandleToAcquire)
				: m_hFile(HandleToAcquire)
			{
			}

			~File()
			{
				if (m_hFile != 0)
					ZwClose(m_hFile);
			}

			HANDLE DetachHandle()
			{
				HANDLE hRet = m_hFile;
				m_hFile = 0;
				return hRet;
			}

			size_t _ReadHelper(void *pBuffer, size_t size, ActionStatus *pStatus, bool IncompleteReadSupported, PLARGE_INTEGER ByteOffset)
			{
				(void)IncompleteReadSupported;
				if (!m_hFile)
				{
					ASSIGN_STATUS(pStatus, InvalidHandle);
					return 0;
				}
				//If you're hit here, my congratulations. You are trying to read data from a file
				//with APCs disabled. This may happen if your thread is inside a critical region
				//(for example, it is holding a mutex). This condition may cause ZwReadFile() to hang
				//infrequently. See DDK documentation fot ZwReadFile() concerning APCs. If you are
				//accessing a file from IRP dispatch routine, try processing this IRP via IRP queue.
				//See comments in irp.h for implementation guidelines.
				ASSERT(!KeAreApcsDisabled() && (KeGetCurrentIrql() == PASSIVE_LEVEL));
				IO_STATUS_BLOCK SB = {STATUS_SUCCESS,0};
				NTSTATUS status = ZwReadFile(m_hFile, NULL, NULL, NULL, &SB, pBuffer, (ULONG)size, ByteOffset, NULL);
				if (!NT_SUCCESS(status))
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(status));
					return SB.Information;
				}
				ASSIGN_STATUS(pStatus, Success);
				return SB.Information;
			}

			size_t Read(void *pBuffer, size_t size, ActionStatus *pStatus = NULL, bool IncompleteReadSupported = false)
			{
				return _ReadHelper(pBuffer, size, pStatus, false, NULL);
			}

			size_t _WriteHelper(const void *pBuffer, size_t size, ActionStatus *pStatus, PLARGE_INTEGER ByteOffset)
			{
				if (!m_hFile)
				{
					ASSIGN_STATUS(pStatus, InvalidHandle);
					return 0;
				}
				//Oops, I've just prevented strange infrequent ZwWriteFile() hanging in your driver :)
				//See comments above (in Read() method) for explanation.
				ASSERT(!KeAreApcsDisabled() && (KeGetCurrentIrql() == PASSIVE_LEVEL));
				IO_STATUS_BLOCK SB = {STATUS_SUCCESS,0};
				NTSTATUS status = ZwWriteFile(m_hFile, NULL, NULL, NULL, &SB, const_cast<PVOID>(pBuffer), (ULONG)size, ByteOffset, NULL);
				if (!NT_SUCCESS(status))
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(status));
					return SB.Information;
				}
				ASSIGN_STATUS(pStatus, Success);
				return SB.Information;
			}

			size_t Write(const void *pBuffer, size_t size, ActionStatus *pStatus = NULL)
			{
				return _WriteHelper(pBuffer, size, pStatus, NULL);
			}

			LONGLONG GetSize(ActionStatus *pStatus = NULL)
			{
				FILE_STANDARD_INFORMATION info;
				IO_STATUS_BLOCK SB;
				NTSTATUS status = ZwQueryInformationFile(m_hFile, &SB, &info, sizeof(info), FileStandardInformation);
				if (!NT_SUCCESS(status))
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(status));
					return -1;
				}
				ASSIGN_STATUS(pStatus, Success);
				return info.EndOfFile.QuadPart;
			}

			LONGLONG Seek(LONGLONG Offset, FileFlags::SeekType seekType, ActionStatus *pStatus = NULL)
			{
				FILE_POSITION_INFORMATION info;
				LONGLONG refSize;
				NTSTATUS status;
				switch (seekType)
				{
				case FileFlags::FileBegin:
					info.CurrentByteOffset.QuadPart = Offset;
					break;
				case FileFlags::FileCurrent:
					refSize = GetSize();
					info.CurrentByteOffset.QuadPart = GetPosition() + Offset;
					if (info.CurrentByteOffset.QuadPart > refSize)
						info.CurrentByteOffset.QuadPart = refSize;
					if (info.CurrentByteOffset.QuadPart < 0)
						info.CurrentByteOffset.QuadPart = 0;
					break;
				case FileFlags::FileEnd:
					refSize = GetSize();
					info.CurrentByteOffset.QuadPart = refSize + Offset;
					if (info.CurrentByteOffset.QuadPart > refSize)
						info.CurrentByteOffset.QuadPart = refSize;
					if (info.CurrentByteOffset.QuadPart < 0)
						info.CurrentByteOffset.QuadPart = 0;
					break;
				}
				IO_STATUS_BLOCK SB;
				status = ZwSetInformationFile(m_hFile, &SB, &info, sizeof(info), FilePositionInformation);
				if (!NT_SUCCESS(status))
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(status));
					return -1;
				}
				ASSIGN_STATUS(pStatus, Success);
				return info.CurrentByteOffset.QuadPart;
			}

			LONGLONG GetPosition(ActionStatus *pStatus = NULL)
			{
				FILE_POSITION_INFORMATION info;
				IO_STATUS_BLOCK SB;
				NTSTATUS status = ZwQueryInformationFile(m_hFile, &SB, &info, sizeof(info), FilePositionInformation);
				if (!NT_SUCCESS(status))
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(status));
					return -1;
				}
				ASSIGN_STATUS(pStatus, Success);
				return info.CurrentByteOffset.QuadPart;
			}

			void Close()
			{
				if (m_hFile == 0)
					return;
				ZwClose(m_hFile);
				m_hFile = 0;
			}

			bool Valid()
			{
				return (m_hFile != 0);
			}

			ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime)
			{
				FILE_BASIC_INFORMATION info;
				IO_STATUS_BLOCK SB;
				NTSTATUS status = ZwQueryInformationFile(m_hFile, &SB, &info, sizeof(info), FileBasicInformation);
				if (!NT_SUCCESS(status))
					return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				if (pCreationTime)
					*pCreationTime = info.CreationTime;
				if (pLastWriteTime)
					*pLastWriteTime = info.LastWriteTime;
				if (pLastReadTime)
					*pLastReadTime = info.LastAccessTime;
				return MAKE_STATUS(Success);
			}

			ActionStatus SetFileTimes(const DateTime *pCreationTime, const DateTime *pLastWriteTime, const DateTime *pLastReadTime)
			{
				FILE_BASIC_INFORMATION info;
				IO_STATUS_BLOCK SB;
				NTSTATUS status = ZwQueryInformationFile(m_hFile, &SB, &info, sizeof(info), FileBasicInformation);
				if (!NT_SUCCESS(status))
					return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				if (pCreationTime)
					info.CreationTime = *pCreationTime->GetLargeInteger();
				if (pLastWriteTime)
					info.LastWriteTime = *pLastWriteTime->GetLargeInteger();
				if (pLastReadTime)
					info.LastAccessTime = *pLastReadTime->GetLargeInteger();
				status = ZwSetInformationFile(m_hFile, &SB, &info, sizeof(info), FileBasicInformation);
				if (!NT_SUCCESS(status))
					return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				return MAKE_STATUS(Success);
			}


			FileFlags::FileAttribute GetFileAttributes(ActionStatus *pStatus = NULL)
			{
				FILE_ATTRIBUTE_TAG_INFORMATION info;
				IO_STATUS_BLOCK SB;
				NTSTATUS status = ZwQueryInformationFile(m_hFile, &SB, &info, sizeof(info), FileAttributeTagInformation);
				if (!NT_SUCCESS(status))
				{
					ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(status));
					return FileFlags::InvalidAttr;
				}
				ASSIGN_STATUS(pStatus, Success);
				return (FileFlags::FileAttribute)info.FileAttributes;
			}

			ActionStatus AddFileAttributes(FileFlags::FileAttribute Attributes)
			{
				FILE_ATTRIBUTE_TAG_INFORMATION info;
				IO_STATUS_BLOCK SB;
				NTSTATUS status = ZwQueryInformationFile(m_hFile, &SB, &info, sizeof(info), FileAttributeTagInformation);
				if (!NT_SUCCESS(status))
					return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				info.FileAttributes |= Attributes;
				status = ZwSetInformationFile(m_hFile, &SB, &info, sizeof(info), FileAttributeTagInformation);
				if (!NT_SUCCESS(status))
					return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				return MAKE_STATUS(Success);
			}

			ActionStatus RemoveFileAttributes(FileFlags::FileAttribute Attributes)
			{
				FILE_ATTRIBUTE_TAG_INFORMATION info;
				IO_STATUS_BLOCK SB;
				NTSTATUS status = ZwQueryInformationFile(m_hFile, &SB, &info, sizeof(info), FileAttributeTagInformation);
				if (!NT_SUCCESS(status))
					return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				info.FileAttributes &= ~Attributes;
				status = ZwSetInformationFile(m_hFile, &SB, &info, sizeof(info), FileAttributeTagInformation);
				if (!NT_SUCCESS(status))
					return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				return MAKE_STATUS(Success);
			}

			ActionStatus Delete()
			{
				FILE_DISPOSITION_INFORMATION info;
				IO_STATUS_BLOCK SB;
				info.DeleteFile = TRUE;
				NTSTATUS status = ZwSetInformationFile(m_hFile, &SB, &info, sizeof(info), FileDispositionInformation);
				if (!NT_SUCCESS(status))
					return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				return MAKE_STATUS(Success);
			}

			static ActionStatus Delete(const wchar_t *pwszFile, bool ForceDeleteReadonly = false)
			{
				if (ForceDeleteReadonly)
				{
					ActionStatus status;
					File file(pwszFile, FileModes::OpenToModifyAttributes, &status);
					if (!status.Successful())
						return status;
					status = file.RemoveFileAttributes(FileFlags::ReadOnly);
					if (!status.Successful())
						return status;
				}

				ActionStatus status;
				FileMode mode = FileModes::OpenReadWrite.ShareAll();
				mode.DesiredAccess = DELETE;
				File file(pwszFile, mode, &status);
				if (!status.Successful())
					return status;
				return file.Delete();
			}

			ActionStatus Move(const String &NewPath, bool DeleteIfExists)
			{
				int L = (int)NewPath.size() * sizeof(wchar_t);
				FILE_RENAME_INFORMATION *pInfo = (FILE_RENAME_INFORMATION *)malloc(sizeof(FILE_RENAME_INFORMATION) + L);
				if (!pInfo)
					return MAKE_STATUS(NoMemory);
				IO_STATUS_BLOCK SB;
				pInfo->ReplaceIfExists = DeleteIfExists;
				pInfo->RootDirectory = NULL;
				pInfo->FileNameLength = (unsigned)NewPath.size() * sizeof(wchar_t);
				memcpy(pInfo->FileName, NewPath.c_str(), L);
				NTSTATUS status = ZwSetInformationFile(m_hFile, &SB, pInfo, sizeof(FILE_RENAME_INFORMATION) + L, FileRenameInformation);
				free(pInfo);
				if (!NT_SUCCESS(status))
					return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				return MAKE_STATUS(Success);
			}

			ActionStatus QueryDirectoryFile(FILE_INFORMATION_CLASS InformationClass, void *pBuffer, unsigned BufferSize, PCUNICODE_STRING pMask = NULL, bool ResetScan = false)
			{
				IO_STATUS_BLOCK SB;
				NTSTATUS status = ZwQueryDirectoryFile(m_hFile, 0, NULL, NULL, &SB, pBuffer, BufferSize,
					InformationClass, TRUE, const_cast<PUNICODE_STRING>(pMask), ResetScan);
				if (!NT_SUCCESS(status))
					return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				return MAKE_STATUS(Success);
			}

			ActionStatus QueryDirectoryFile(FILE_FULL_DIR_INFORMATION *pInfo, unsigned BufferSize, PCUNICODE_STRING pMask = NULL, bool ResetScan = false)
			{
				return QueryDirectoryFile(FileFullDirectoryInformation, pInfo, BufferSize, pMask, ResetScan);
			}

			ActionStatus Crop()
			{
				ActionStatus status;
				LONGLONG pos = GetPosition(&status);
				if (!status.Successful())
					return status;
				FILE_END_OF_FILE_INFORMATION info;
				IO_STATUS_BLOCK SB;
				info.EndOfFile.QuadPart = pos;
				NTSTATUS ntstatus = ZwSetInformationFile(m_hFile, &SB, &info, sizeof(info), FileEndOfFileInformation);
				if (!NT_SUCCESS(ntstatus))
					return MAKE_STATUS(ActionStatus::FromNTStatus(ntstatus));
				return MAKE_STATUS(Success);
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
				LARGE_INTEGER li;
				li.QuadPart = offset;
				return _ReadHelper(pBuffer, size, pStatus, false, &li);
			}

			size_t WriteAt(const void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL)
			{
				if (pCompletion)
				{
					ASSIGN_STATUS(pStatus, NotSupported);
					return 0;
				}
				LARGE_INTEGER li;
				li.QuadPart = offset;
				return _WriteHelper(pBuffer, size, pStatus, &li);
			}

			NTSTATUS DeviceIoControl(ULONG ControlCode, const void *pInputBuffer = NULL, size_t InputBufferSize = 0, void *pOutputBuffer = NULL, size_t OutputBufferSize = 0, size_t *pDone = NULL)
			{
				IO_STATUS_BLOCK statusBlock;
				ASSERT(!KeAreApcsDisabled() && (KeGetCurrentIrql() == PASSIVE_LEVEL));
				NTSTATUS status = ZwDeviceIoControlFile(m_hFile, NULL, NULL, NULL, &statusBlock, ControlCode, (PVOID)pInputBuffer, (ULONG)InputBufferSize, pOutputBuffer, (ULONG)OutputBufferSize);
				if (pDone)
					*pDone = statusBlock.Information;
				return status;
			}

		public:
			static NTSTATUS GetOpenFileStatus(const String &Path, ULONG directoryFlag = FILE_NON_DIRECTORY_FILE)
			{
				OBJECT_ATTRIBUTES fileAttributes;
				UNICODE_STRING fileName;
				IO_STATUS_BLOCK SB;

				RtlInitUnicodeString(&fileName, Path.c_str());
				InitializeObjectAttributes(&fileAttributes, &fileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

				HANDLE hFile;
				NTSTATUS status = ZwCreateFile(&hFile, FILE_READ_ATTRIBUTES, &fileAttributes, &SB,
					NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					FILE_OPEN, directoryFlag,
					NULL, 0);

				if (!NT_SUCCESS(status))
					return status;
				ZwClose(hFile);
				return STATUS_SUCCESS;
			}

			static bool Exists(const String &Path)
			{
				return (GetOpenFileStatus(Path) != STATUS_OBJECT_NAME_NOT_FOUND);
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
				File m_Dir;
				String m_BasePath;
				String m_FindMask;
				PFILE_FULL_DIR_INFORMATION m_pFullDirInfo;
				ULONG m_AllocatedSize;

			private:
				enumerator(const enumerator &iter)
				{
					ASSERT(FALSE);
				}

				void operator=(const enumerator &iter)
				{
					ASSERT(FALSE);
				}

				ActionStatus Next(bool ResetScan)
				{
					if (!Valid())
						return MAKE_STATUS(FileNotFound);
					if (!m_pFullDirInfo)
						return MAKE_STATUS(NoMemory);
					ActionStatus st = m_Dir.QueryDirectoryFile(m_pFullDirInfo, m_AllocatedSize, m_FindMask.size() ? m_FindMask.ToNTString() : NULL, ResetScan);
					if (!st.Successful())
						DestroyBuffer();
					return st;
				}

				void DestroyBuffer()
				{
					if (m_pFullDirInfo)
					{
						free(m_pFullDirInfo);
						m_pFullDirInfo = NULL;
						m_AllocatedSize = 0;
					}
				}

			public:
				enumerator(const FindFileParams &params) :
				  m_BasePath(params.DirPath),
					  m_FindMask(params.FileMask),
					  m_Dir(params.DirPath, FileModes::OpenDirectoryForQuery),
					  m_pFullDirInfo(NULL),
					  m_AllocatedSize(0)
				  {
					  m_AllocatedSize = sizeof(FILE_FULL_DIR_INFORMATION) + sizeof(wchar_t) * _MAX_PATH;
					  m_pFullDirInfo = (PFILE_FULL_DIR_INFORMATION)malloc(m_AllocatedSize);
					  if (!m_pFullDirInfo)
						  return;
					  if (!Next(true).Successful())
						  DestroyBuffer();
				  }

				  ~enumerator()
				  {
					  DestroyBuffer();
				  }

				  bool Valid() const
				  {
					  return (m_pFullDirInfo != 0);
				  }

				  String GetFullPath() const
				  {
					  if (!Valid())
						  return _T("");
					  if ((m_BasePath.size() > 0) && (m_BasePath[m_BasePath.length() - 1] != '\\'))
						  return m_BasePath + L"\\" + String(m_pFullDirInfo->FileName, m_pFullDirInfo->FileNameLength / sizeof(wchar_t));
					  else
						  return m_BasePath + String(m_pFullDirInfo->FileName, m_pFullDirInfo->FileNameLength / sizeof(wchar_t));
				  }

				  String GetRelativePath() const
				  {
					  if (!Valid())
						  return _T("");
					  return String(m_pFullDirInfo->FileName, m_pFullDirInfo->FileNameLength / sizeof(wchar_t));
				  }

				  bool IsDirectory() const
				  {
					  if (!Valid())
						  return false;
					  return ((m_pFullDirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
				  }

				  virtual bool IsAPseudoEntry() const	//Returns true for "." and ".." entries
				  {
					  if (!Valid())
						  return false;
					  if (m_pFullDirInfo->FileNameLength > (2 * sizeof(wchar_t)))
						  return false;
					  else if (m_pFullDirInfo->FileNameLength == (2 * sizeof(wchar_t)))
						  return (m_pFullDirInfo->FileName[0] == '.') && (m_pFullDirInfo->FileName[1] == '.');
					  else if (m_pFullDirInfo->FileNameLength == (sizeof(wchar_t)))
						  return (m_pFullDirInfo->FileName[0] == '.');
					  else
						  return false;
				  }


				  FileFlags::FileAttribute GetAttributes(ActionStatus *pStatus) const
				  {
					  if (!Valid())
					  {
						  ASSIGN_STATUS(pStatus, FileNotFound);
						  return FileFlags::InvalidAttr;
					  }
					  ASSIGN_STATUS(pStatus, Success);
					  return (FileFlags::FileAttribute)m_pFullDirInfo->FileAttributes;
				  }

				  ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime) const
				  {
					  if (!Valid())
						  return MAKE_STATUS(FileNotFound);
					  if (pCreationTime)
						  *pCreationTime = m_pFullDirInfo->CreationTime;
					  if (pLastWriteTime)
						  *pCreationTime = m_pFullDirInfo->LastWriteTime;
					  if (pLastReadTime)
						  *pCreationTime = m_pFullDirInfo->LastAccessTime;
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
					  return m_pFullDirInfo->EndOfFile.QuadPart;
				  }

				  ActionStatus Next()
				  {
					  return Next(false);
				  }
			};

		public:
			Directory(const String &Path) :
			  m_DirPath(Path)
			  {
			  }

			  String RelativeToAbsolute(const String &path)
			  {
				  return Path::Combine(m_DirPath, path);
			  }

			  ActionStatus ChangeDirectory(const String &Path)
			  {
				  String newPath = Path::Combine(m_DirPath, Path);
				  UNICODE_STRING dirName;
				  OBJECT_ATTRIBUTES dirAttributes;
				  RtlInitUnicodeString(&dirName, newPath.c_str());
				  InitializeObjectAttributes(&dirAttributes, &dirName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

				  HANDLE hDir = 0;
				  IO_STATUS_BLOCK SB;

				  NTSTATUS status = ZwOpenFile(&hDir, DIRECTORY_QUERY, &dirAttributes, &SB, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_DIRECTORY_FILE);
				  if (!NT_SUCCESS(status))
					  return MAKE_STATUS(ActionStatus::FromNTStatus(status));

				  ZwClose(hDir);
				  m_DirPath = newPath;
				  return MAKE_STATUS(Success);
			  }

			  FileFlags::FileAttribute GetFileAttributes(const String &Path, ActionStatus *pStatus = NULL)
			  {
				  File file(RelativeToAbsolute(Path), FileModes::OpenToQueryAttributes, pStatus);
				  if (!file.Valid())
					  return FileFlags::InvalidAttr;
				  return file.GetFileAttributes(pStatus);
			  }

			  ActionStatus AddFileAttributes(const String &Path, FileFlags::FileAttribute Attributes)
			  {
				  ActionStatus status;
				  File file(RelativeToAbsolute(Path), FileModes::OpenToModifyAttributes, &status);
				  if (!file.Valid())
					  return status;
				  return file.AddFileAttributes(Attributes);
			  }

			  ActionStatus RemoveFileAttributes(const String &Path, FileFlags::FileAttribute Attributes)
			  {
				  ActionStatus status;
				  File file(RelativeToAbsolute(Path), FileModes::OpenToModifyAttributes, &status);
				  if (!file.Valid())
					  return status;
				  return file.RemoveFileAttributes(Attributes);
			  }

			  static ActionStatus Create(const String &absolutePath)
			  {
				  UNICODE_STRING dirName;
				  OBJECT_ATTRIBUTES dirAttributes;
				  RtlInitUnicodeString(&dirName, absolutePath.c_str());
				  InitializeObjectAttributes(&dirAttributes, &dirName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

				  HANDLE hDir = 0;
				  IO_STATUS_BLOCK SB;
				  NTSTATUS status = ZwCreateFile(&hDir,
					  DIRECTORY_QUERY,
					  &dirAttributes,
					  &SB,
					  NULL,
					  FILE_ATTRIBUTE_DIRECTORY,
					  FILE_SHARE_READ | FILE_SHARE_WRITE,
					  FILE_CREATE,
					  FILE_DIRECTORY_FILE,
					  NULL, 0);
				  if (!NT_SUCCESS(status))
					  return MAKE_STATUS(ActionStatus::FromNTStatus(status));
				  ZwClose(hDir);
				  return MAKE_STATUS(Success);
			  }

			  ActionStatus CreateDirectory(const String &relativePath)
			  {
				  return Create(Path::Combine(m_DirPath, relativePath));
			  }

			  ActionStatus DeleteFile(const String &Path)
			  {
				  return File::Delete(Path::Combine(m_DirPath, Path).c_str());
			  }

			  static ActionStatus Remove(const String &absolutePath)
			  {
				  ActionStatus status;
				  FileMode mode = FileModes::OpenReadWrite.ShareAll();
				  mode.DesiredAccess = DELETE;
				  mode.CreateOptions &= ~FILE_NON_DIRECTORY_FILE;
				  mode.CreateOptions |= FILE_DIRECTORY_FILE;
				  File file(absolutePath, mode, &status);
				  if (!file.Valid())
					  return status;
				  return file.Delete();
			  }

			  ActionStatus RemoveDirectory(const String &Path)
			  {
				  return Remove(RelativeToAbsolute(Path));
			  }

			  ActionStatus MoveFile(const String &ExistingPath, const String &NewPath, bool DeleteIfExists = false)
			  {
				  ActionStatus status;
				  File file(RelativeToAbsolute(ExistingPath), FileModes::OpenToModifyAttributes, &status);
				  if (file.Valid())
					  return file.Move(RelativeToAbsolute(NewPath), DeleteIfExists);
				  return status;
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
				  return (File::GetOpenFileStatus(Path, FILE_DIRECTORY_FILE) != STATUS_OBJECT_NAME_NOT_FOUND);
			  }

			  TempString &GetFullPath()
			  {
				  return m_DirPath;
			  }
		};
	}

	typedef DDK::File PlatformSpecificFile;
	typedef DDK::Directory PlatformSpecificDirectory;
}