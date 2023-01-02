#pragma once
#include <sys/vnode_if.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/proc.h>
#include "../atomic.h"

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

//Eliminates the "64-to-32 assignment loses precision" warning
#define	TIMESPEC_TO_TIMEVAL_NOWARN(tv, ts) {					\
	(tv)->tv_sec = (ts)->tv_sec;					\
	(tv)->tv_usec = (int)((ts)->tv_nsec / 1000);				\
}


#include "../filehlp.h"

namespace BazisLib
{
	namespace KEXT
	{
		enum FileModeFlags
		{
			kDisableCaching = 0x0001,
		};

		struct FileMode
		{
			int Flags;
			int AccessMask;
			FileModeFlags AdditionalFlags;

			FileMode AddRawFlags(int flags) const
			{
				FileMode copy = *this;
				copy.Flags |= flags;
				return copy;
			}

			FileMode AddExtendedFlags(FileModeFlags flags) const
			{
				FileMode copy = *this;
				copy.AdditionalFlags = (FileModeFlags)(copy.AdditionalFlags | flags);
				return copy;
			}
		};

		class File
		{
		private:
			vnode_t m_VNode;
			AtomicUInt64 m_Offset;
			_AtomicPointerT<vnode_attr> m_pAttributeBuffer;
			//vfs_context_t m_Context;

		private:
			File(const File &anotherFile) {ASSERT(FALSE);}
			void operator=(const File &anotherFile) {ASSERT(FALSE);}

			class VFSContext
			{
			private:
				vfs_context_t m_Context;

			public:
				VFSContext(File *pFile)
				{
					m_Context = vfs_context_create(NULL);
				}

				~VFSContext()
				{
					if (m_Context)
						vfs_context_rele(m_Context);
				}

				operator vfs_context_t()
				{
					return m_Context;
				}
			};

			void ApplyAdditionalFlags(unsigned standardFlags, FileModeFlags flags)
			{
				if (standardFlags & O_TRUNC)
					Crop();
				if (flags & kDisableCaching)
					vnode_setnocache(m_VNode);
			}

		public:
			static void ExportTimespec(const timespec &ts, DateTime *pTimestamp)
			{
				if (pTimestamp)
				{
					timeval tv = {0,};
					TIMESPEC_TO_TIMEVAL_NOWARN(&tv, &ts);

					*pTimestamp = tv;
				}
			}

		public:
			File(const TCHAR *ptszFileName, const FileMode &mode, ActionStatus *pStatus = NULL)
				: m_VNode(NULL)
				, m_Offset(0)
				, m_pAttributeBuffer(NULL)
				//, m_Context(NULL)
			{
				int err = vnode_open(ptszFileName, mode.Flags, mode.AccessMask, 0, &m_VNode, VFSContext(this));
				ASSIGN_STATUS(pStatus, ActionStatus::FromUnixError(err));
				if (m_VNode)
					ApplyAdditionalFlags(mode.Flags, mode.AdditionalFlags);
			}

			File(const String &FileName, const FileMode &mode, ActionStatus *pStatus = NULL)
				: m_VNode(NULL)
				, m_Offset(0)
				, m_pAttributeBuffer(NULL)
				//, m_Context(NULL)
			{
				int err = vnode_open(FileName.c_str(), mode.Flags, mode.AccessMask, 0, &m_VNode, VFSContext(this));
				ASSIGN_STATUS(pStatus, ActionStatus::FromUnixError(err));
				if (m_VNode)
					ApplyAdditionalFlags(mode.Flags, mode.AdditionalFlags);
			}

			File()
				: m_VNode(NULL)
				, m_Offset(0)
				, m_pAttributeBuffer(NULL)
				//, m_Context(NULL)
			{
			}

			~File()
			{
				if (m_VNode)
					vnode_close(m_VNode, 0, VFSContext(this));
// 				if (m_Context)
// 					vfs_context_rele(m_Context);
				delete (vnode_attr *)(void *)m_pAttributeBuffer;
			}

			vnode_t GetHandleForSingleUse() {return m_VNode;}

			size_t Read(void *pBuffer, size_t size, ActionStatus *pStatus = NULL, bool IncompleteReadSupported = false)
			{
				int remaining = 0;
				int err = vn_rdwr(UIO_READ, m_VNode, (char *)pBuffer, (int)size, m_Offset, UIO_SYSSPACE, IO_NOAUTH, vfs_context_ucred(VFSContext(this)), &remaining, kernproc);
				size_t done = size - remaining;
				m_Offset += done;
				ASSIGN_STATUS(pStatus, ActionStatus::FromUnixError(err));
				return (size_t)done;
			}

			size_t Write(const void *pBuffer, size_t size, ActionStatus *pStatus = NULL)
			{
				int remaining = 0;
				int err = vn_rdwr(UIO_WRITE, m_VNode, (char *)pBuffer, (int)size, m_Offset, UIO_SYSSPACE, IO_NOAUTH, vfs_context_ucred(VFSContext(this)), &remaining, kernproc);
				size_t done = size - remaining;
				m_Offset += done;
				ASSIGN_STATUS(pStatus, ActionStatus::FromUnixError(err));
				return (size_t)done;
			}

			const vnode_attr *QueryVNodeAttributes(int mask, ActionStatus *pStatus = NULL)
			{
				m_pAttributeBuffer.AssignIfNULLOrDeleteIfNot(new vnode_attr());
				VATTR_INIT(m_pAttributeBuffer);
				m_pAttributeBuffer->va_active = mask;
				int err = vnode_getattr(m_VNode, m_pAttributeBuffer, VFSContext(this));
				ASSIGN_STATUS(pStatus, ActionStatus::FromUnixError(err));
				return !err ? (vnode_attr *)m_pAttributeBuffer : NULL;
			}

			LONGLONG GetSize(ActionStatus *pStatus = NULL)
			{
				const vnode_attr *pAttr = QueryVNodeAttributes(VNODE_ATTR_va_data_size, pStatus);
				if (!pAttr)
					return -1LL;
				return pAttr->va_data_size;
			}

			LONGLONG Seek(LONGLONG Offset, FileFlags::SeekType seekType = FileFlags::FileBegin, ActionStatus *pStatus = NULL)
			{
				LONGLONG newOffset;

				LONGLONG curSize = GetSize(pStatus);
				if (curSize == -1LL)
					return -1LL;

				switch(seekType)
				{
				case FileFlags::FileBegin:
					newOffset = Offset;
					break;
				case FileFlags::FileEnd:
					newOffset = curSize + Offset;
					break;
				case FileFlags::FileCurrent:
					newOffset = m_Offset + Offset;
					break;
				default:
					ASSIGN_STATUS(pStatus, InvalidParameter);
					return -1LL;
				}

				m_Offset = newOffset;

				ASSIGN_STATUS(pStatus, Success);
				return newOffset;
			}

			LONGLONG GetPosition(ActionStatus *pStatus = NULL)
			{
				ASSIGN_STATUS(pStatus, Success);
				return m_Offset;
			}

			void Close()
			{
				if (!m_VNode)
					return;
				vnode_close(m_VNode, 0, VFSContext(this));
				m_VNode = NULL;
			}

			bool Valid()
			{
				return m_VNode != NULL;
			}

			vnode_t DetachHandle()
			{
				vnode_t h = m_VNode;
				m_VNode = NULL;
				return h;
			}

			ActionStatus Crop()
			{
				ActionStatus st;
				LONGLONG off = GetPosition(&st);
				if (!st.Successful())
					return st;

				m_pAttributeBuffer.AssignIfNULLOrDeleteIfNot(new vnode_attr());
				VATTR_INIT(m_pAttributeBuffer);

				m_pAttributeBuffer->va_active = VNODE_ATTR_va_data_size;
				m_pAttributeBuffer->va_data_size = off;
				int err = vnode_setattr(m_VNode, m_pAttributeBuffer, VFSContext(this));
				return MAKE_STATUS(ActionStatus::FromUnixError(err));
			}

			ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime)
			{
				ActionStatus st;
				const vnode_attr *pAttr = QueryVNodeAttributes(VNODE_ATTR_va_create_time | VNODE_ATTR_va_modify_time | VNODE_ATTR_va_access_time, &st);
				if (!st.Successful())
					return st;

				ExportTimespec(pAttr->va_create_time, pCreationTime);
				ExportTimespec(pAttr->va_modify_time, pLastWriteTime);
				ExportTimespec(pAttr->va_access_time, pLastReadTime);
				return MAKE_STATUS(Success);
			}

			ActionStatus SetFileTimes(const DateTime *pCreationTime, const DateTime *pLastWriteTime, const DateTime *pLastReadTime)
			{
				ActionStatus st;
				QueryVNodeAttributes(VNODE_ATTR_va_create_time | VNODE_ATTR_va_modify_time | VNODE_ATTR_va_access_time, &st);
				if (!st.Successful())
					return st;

				ASSERT(m_pAttributeBuffer);

				m_pAttributeBuffer->va_active = 0;
				timeval tv;
				if (pCreationTime)
				{
					m_pAttributeBuffer->va_active |= VNODE_ATTR_va_create_time;
					tv = pCreationTime->ToTimeval();
					TIMEVAL_TO_TIMESPEC(&tv, &m_pAttributeBuffer->va_create_time);
				}

				if (pLastWriteTime)
				{
					m_pAttributeBuffer->va_active |= VNODE_ATTR_va_modify_time;
					tv = pLastWriteTime->ToTimeval();
					TIMEVAL_TO_TIMESPEC(&tv, &m_pAttributeBuffer->va_modify_time);
				}

				if (pLastReadTime)
				{
					m_pAttributeBuffer->va_active |= VNODE_ATTR_va_access_time;
					tv = pLastReadTime->ToTimeval();
					TIMEVAL_TO_TIMESPEC(&tv, &m_pAttributeBuffer->va_access_time);
				}

				int err = vnode_setattr(m_VNode, m_pAttributeBuffer, VFSContext(this));
				return MAKE_STATUS(ActionStatus::FromUnixError(err));
			}

			void SetCachingEnabled(bool enabled)
			{
				if (enabled)
					vnode_clearnocache(m_VNode);
				else
					vnode_setnocache(m_VNode);
			}

			bool IsCachingEnabled()
			{
				return vnode_isnocache(m_VNode);
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

			FileFlags::FileAttribute GetFileAttributes(ActionStatus *pStatus = NULL) {ASSIGN_STATUS(pStatus, NotSupported); return FileFlags::NormalFile;}
			ActionStatus AddFileAttributes(FileFlags::FileAttribute Attributes) {return MAKE_STATUS(NotSupported);}
			ActionStatus RemoveFileAttributes(FileFlags::FileAttribute Attributes) {return MAKE_STATUS(NotSupported);}

			static bool Exists(const TCHAR *Path)
			{
				const FileMode mode = {O_RDONLY, 0755};
				File f(Path, mode);
				return f.Valid();
			}

			//! Reserved. Deleting files from MacOS KEXTs is not supported.
			/*
				MacOS KEXTs cannot delete files/directories using the publicly available API. 
				See the remarks section regarding the file deletion process in MacOS kernel.
				
				\remarks When user-mode code deletes a file using the unlink() call, the private unlink1() function gets called in kernel (vfs_syscalls.c).
				The unlink1() function receives the information about the deleted file in the nameidata structure containing user-space addresses then it prepares
				the kernel-mode name info (componentname structure) and calls the vn_remove() function (kpi_vfs.c). The vn_remove() function is also private.
				The vn_remove() function is just a wrapper around VNOP_REMOVE() and VNOP_COMPOUND_REMOVE() (vnode_compound_remove_available() decides which one to call).
				The VNOP_REMOVE() in turn calls the FS-specific implementation of REMOVE using the v_op function table. It's not exported from the kernel either.

				It looks like the vn_remove() would be the correct entry point to delete a file from the kernel mode, but is has been declared private to avoid
				wrong use of the undocumented componentname structure.

				If you need to delete a file from the KEXT, the best way is to use a user-mode daemon handling file deletion requests on behalf of your driver.
				Alternatively, you can hack into the kernel and call vn_remove() by address or call the function from the v_op table directly, but it's unsupported, 
				relies on unsupported structures (e.g. componentname) that can be changed in the future and Apple won't be pleased about such unreliable code in your KEXT.
			*/
			static ActionStatus Delete(const TCHAR *Path, bool ForceDeleteReadonly = false)
			{
				return MAKE_STATUS(NotSupported);
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
				bool m_Valid;

			private:
				enumerator(const enumerator &iter) { ASSERT(FALSE); }
				void operator=(enumerator &iter) { ASSERT(FALSE); }

			public:
				enumerator(const FindFileParams &params) 
					: m_BasePath(params.DirPath)
					, m_Valid(false)
				{
				}

				~enumerator()
				{
				}

				bool Valid() const
				{
					return m_Valid;
				}

				String GetFullPath() const
				{
					return "";
				}

				String GetRelativePath() const
				{
					return "";
				}

				bool IsDirectory() const
				{
					return false;
				}

				bool IsAPseudoEntry() const
				{
					return false;
				}

				FileFlags::FileAttribute GetAttributes(ActionStatus *pStatus) const
				{
					ASSIGN_STATUS(pStatus, NotSupported);
					return FileFlags::InvalidAttr;
				}

				ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime) const
				{
					return MAKE_STATUS(NotSupported);
				}

				LONGLONG GetSize(ActionStatus *pStatus) const
				{
					ASSIGN_STATUS(pStatus, NotSupported);
					return 0;
				}

				ActionStatus Next()
				{
					return MAKE_STATUS(NotSupported);
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
				return MAKE_STATUS(NotSupported);	//Not supported for KEXTs
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
				const FileMode mode = {O_DIRECTORY, 0755};
				ActionStatus st;
				File f(Path, mode, &st);
				return f.Valid();
			}

			static ActionStatus Create(const String &Path)
			{
				return MAKE_STATUS(NotSupported);	//Not supported for KEXTs
			}

			static ActionStatus Remove(const String &Path)
			{
				return MAKE_STATUS(NotSupported);	//Not supported for KEXTs
			}

			TempString &GetFullPath()
			{
				return m_DirPath;
			}

		};
	}

	typedef KEXT::FileMode FileMode;
	typedef KEXT::File PlatformSpecificFile;
	typedef KEXT::Directory PlatformSpecificDirectory;
}

#include "../Posix/UnixFileModes.h"

