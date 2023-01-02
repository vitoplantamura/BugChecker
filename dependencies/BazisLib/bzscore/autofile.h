#pragma once
#include "file.h"
#include "status.h"
#include "objmgr.h"
#include "path.h"

namespace BazisLib
{
	class AIFile : AUTO_INTERFACE
	{
	public:
		//! Reads data from the file (or stream)
		/*!
			\param IncompleteReadSupported If this parameter is <b>true</b>, the Read() method can return
										   a value different from the <b>size</b> parameter even when the EOF
										   is not reached. For example, this can happen if a file is actually
										   a network stream and some data is already in the buffer. However,
										   if this parameter is false, the stream will wait till the end of 
										   connection, or till the requested amount of bytes will be received.
										   Non-stream implementations can safely ignore this parameter and
										   always wait for the end-of-file, or exactly the requested number
										   of bytes.
		*/
		virtual size_t Read(void *pBuffer, size_t size, ActionStatus *pStatus = NULL, bool IncompleteReadSupported = false)=0;
		virtual size_t Write(const void *pBuffer, size_t size, ActionStatus *pStatus = NULL)=0;

		virtual LONGLONG GetSize(ActionStatus *pStatus = NULL)=0;
		virtual LONGLONG Seek(LONGLONG Offset, FileFlags::SeekType seekType = FileFlags::FileBegin, ActionStatus *pStatus = NULL)=0;
		virtual LONGLONG GetPosition(ActionStatus *pStatus = NULL)=0;
		
		virtual void Close()=0;
		virtual bool Valid()=0;

		//! Sets the end of file to current position
		virtual ActionStatus Crop()=0;
	};

	class AIAsyncFile : AUTO_INTERFACE
	{
		//Asynchronous operations. To use them, create file with one of AsyncXxx access modes
		virtual bool AsynchronousIOSupported()=0;
		virtual size_t ReadAt(void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL)=0;
		virtual size_t WriteAt(const void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL)=0;
	};

	class AIFSFile : AUTO_INTERFACE
	{
		virtual ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime)=0;
		virtual ActionStatus SetFileTimes(const DateTime *pCreationTime, const DateTime *pLastWriteTime, const DateTime *pLastReadTime)=0;

		//Attribute-related ops
		/*virtual FileFlags::FileAttribute GetFileAttributes(ActionStatus *pStatus = NULL)=0;
		virtual ActionStatus AddFileAttributes(FileFlags::FileAttribute Attributes)=0;
		virtual ActionStatus RemoveFileAttributes(FileFlags::FileAttribute Attributes)=0;*/
	};

	class AIDirectoryIterator : AUTO_INTERFACE
	{
	public:
		virtual String GetFullPath() const=0;
		virtual String GetRelativePath() const=0;
		virtual bool IsDirectory() const=0;
		virtual bool IsAPseudoEntry() const=0;	//Returns true for "." and ".." entries
		virtual bool Valid() const=0;

		virtual FileFlags::FileAttribute GetAttributes(ActionStatus *pStatus = NULL) const=0;
		virtual ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime) const=0;
		virtual LONGLONG GetSize(ActionStatus *pStatus = NULL) const=0;

		virtual ActionStatus FindNextFile()=0;
	};

	class AIDirectory : AUTO_INTERFACE
	{
	public:
		virtual ActionStatus ChangeDirectory(const String &Path)=0;
		virtual FileFlags::FileAttribute GetFileAttributes(const String &Path, ActionStatus *pStatus = NULL)=0;
		
		virtual ActionStatus AddFileAttributes(const String &Path, FileFlags::FileAttribute Attributes)=0;
		virtual ActionStatus RemoveFileAttributes(const String &Path, FileFlags::FileAttribute Attributes)=0;

		//! Creates a file instance and ensures that it is valid
		/*! This function tries to create or open a file and returns a pointer to a corresponding object.
			If the operation failed (file object is not valid), it is released and a value of NULL is being
			returned.
		*/
		virtual AIFile *CreateFile(const String &Path,
								   FileFlags::FileAccess Access = FileFlags::ReadAccess,
								   FileFlags::OpenMode OpenMode = FileFlags::OpenExisting,
								   FileFlags::ShareMode ShareMode = FileFlags::ShareRead,
								   FileFlags::FileAttribute Attributes = FileFlags::NormalFile,
								   ActionStatus *pStatus = NULL)=0;

		virtual AIDirectory *CreateDirectory(const String &Path, ActionStatus *pStatus = NULL)=0;
		virtual ActionStatus DeleteFile(const String &Path)=0;
		virtual ActionStatus RemoveDirectory(const String &Path)=0;
		virtual ActionStatus MoveFile(const String &ExistingPath, const String &NewPath, bool DeleteIfExists = false)=0;

		virtual AIDirectoryIterator *FindFirstFile(const String &FileMask = _T(""))=0;

	};

	//This class should be used as a base for deriving custom file classes. It implements all AIFile methods
	//except Valid() in the default way - returning NotSupported status.
	class BasicFileBase : public AIFile, public AIAsyncFile, public AIFSFile
	{
	public:
		virtual size_t Read(void *pBuffer, size_t size, ActionStatus *pStatus = NULL, bool IncompleteReadSupported = false) override {ASSIGN_STATUS(pStatus, NotSupported); return 0;}
		virtual size_t Write(const void *pBuffer, size_t size, ActionStatus *pStatus = NULL) override {ASSIGN_STATUS(pStatus, NotSupported); return 0;}

		virtual LONGLONG GetSize(ActionStatus *pStatus = NULL) override {ASSIGN_STATUS(pStatus, NotSupported); return 0;}
		virtual LONGLONG Seek(LONGLONG Offset, FileFlags::SeekType seekType, ActionStatus *pStatus = NULL) override {ASSIGN_STATUS(pStatus, NotSupported); return 0;}
		virtual LONGLONG GetPosition(ActionStatus *pStatus = NULL) override {ASSIGN_STATUS(pStatus, NotSupported); return 0;}
		
		virtual void Close() override {}
		virtual bool Valid() override=0;
		virtual ActionStatus Crop() {return MAKE_STATUS(NotSupported);}

		virtual ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime) override {return MAKE_STATUS(NotSupported);}
		virtual ActionStatus SetFileTimes(const DateTime *pCreationTime, const DateTime *pLastWriteTime, const DateTime *pLastReadTime) override {return MAKE_STATUS(NotSupported);}

		virtual bool AsynchronousIOSupported() override {return false;}
		virtual size_t ReadAt(void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL) override {ASSIGN_STATUS(pStatus, NotSupported); return 0;}
		virtual size_t WriteAt(const void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL) override {ASSIGN_STATUS(pStatus, NotSupported); return 0;}

		/*virtual FileFlags::FileAttribute GetFileAttributes(ActionStatus *pStatus = NULL) override {ASSIGN_STATUS(pStatus, NotSupported); return FileFlags::NormalFile;}
		virtual ActionStatus AddFileAttributes(FileFlags::FileAttribute Attributes) override {return MAKE_STATUS(NotSupported);}
		virtual ActionStatus RemoveFileAttributes(FileFlags::FileAttribute Attributes) override {return MAKE_STATUS(NotSupported);}*/
	public:
		static inline LONGLONG DoSeek(ULONGLONG *RealOffset, ULONGLONG FileSize, LONGLONG Offset, FileFlags::SeekType seekType, bool CheckForEnd, ActionStatus *pStatus = NULL)
		{
			switch (seekType)
			{
			case FileFlags::FileBegin:
				*RealOffset = Offset;
				break;
			case FileFlags::FileCurrent:
				*RealOffset += Offset;
				break;
			case FileFlags::FileEnd:
				*RealOffset = FileSize + Offset;
				break;
			}
			if (CheckForEnd)
			{
				if (*RealOffset > FileSize)
					*RealOffset = FileSize;
			}
			ASSIGN_STATUS(pStatus, Success);
			return *RealOffset;
		}

		static inline size_t DoReadFromMemoryLite(void *pBufferToWrite, size_t MaxSize, const void *pBufferWithData, size_t BufferWithDataSize, size_t *pOffset, size_t *OffsetInTargetBuffer = NULL)
		{
			size_t off = *pOffset;
			if (off > BufferWithDataSize)
				off = BufferWithDataSize;

			if (OffsetInTargetBuffer)
				MaxSize -= *OffsetInTargetBuffer;

			if (MaxSize > (BufferWithDataSize - off))
				MaxSize = BufferWithDataSize - off;

			if (OffsetInTargetBuffer)
			{
				memcpy((char *)pBufferToWrite + *OffsetInTargetBuffer, ((char *)pBufferWithData) + off, MaxSize);
				*OffsetInTargetBuffer += MaxSize;
			}
			else
				memcpy(pBufferToWrite, ((char *)pBufferWithData) + off, MaxSize);

			off += MaxSize;
			*pOffset = off;
			return MaxSize;
		}

		static inline size_t DoReadFromMemory(void *pBufferToWrite, size_t MaxSize, const void *pBufferWithData, size_t BufferWithDataSize, size_t *pOffset)
		{
			if (!pBufferToWrite || !MaxSize || !pBufferWithData || !BufferWithDataSize || !pOffset)
				return 0;
			return DoReadFromMemoryLite(pBufferToWrite, MaxSize, pBufferWithData, BufferWithDataSize, pOffset);
		}
	};

	template <typename _FileClass> class _ACFile : public AIFile, public AIAsyncFile, public AIFSFile
	{
	protected:
		_FileClass _File;

	public:
		_ACFile(const TCHAR *ptszFileName, const FileMode &mode, ActionStatus *pStatus = NULL)
			: _File(ptszFileName, mode, pStatus)
		{
		}

		_ACFile(const String &FileName, const FileMode &mode, ActionStatus *pStatus = NULL)
			: _File(FileName, mode, pStatus)
		{
		}

	public:
		virtual size_t Read(void *pBuffer, size_t size, ActionStatus *pStatus = NULL, bool IncompleteReadSupported = false) {return _File.Read(pBuffer, size, pStatus, IncompleteReadSupported);}
		virtual size_t Write(const void *pBuffer, size_t size, ActionStatus *pStatus = NULL){return _File.Write(pBuffer, size, pStatus);}

		virtual LONGLONG GetSize(ActionStatus *pStatus = NULL){return _File.GetSize(pStatus);}
		virtual LONGLONG Seek(LONGLONG Offset, FileFlags::SeekType seekType = FileFlags::FileBegin, ActionStatus *pStatus = NULL){return _File.Seek(Offset, seekType, pStatus);}
		virtual LONGLONG GetPosition(ActionStatus *pStatus = NULL){return _File.GetPosition(pStatus);}

		virtual void Close(){_File.Close();}
		virtual bool Valid(){return _File.Valid();}

		//! Sets the end of file to current position
		virtual ActionStatus Crop(){return _File.Crop();}

		virtual ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime) {return _File.GetFileTimes(pCreationTime, pLastWriteTime, pLastReadTime);}
		virtual ActionStatus SetFileTimes(const DateTime *pCreationTime, const DateTime *pLastWriteTime, const DateTime *pLastReadTime){return _File.SetFileTimes(pCreationTime, pLastWriteTime, pLastReadTime);}

		//Asynchronous operations. To use them, create file with one of AsyncXxx access modes
		virtual bool AsynchronousIOSupported() {return _File.AsynchronousIOSupported();}
		virtual size_t ReadAt(void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL) {return _File.ReadAt(pBuffer, offset, size, pCompletion, pStatus);}
		virtual size_t WriteAt(const void *pBuffer, ULONGLONG offset, size_t size, IoCompletion *pCompletion = NULL, ActionStatus *pStatus = NULL){return _File.WriteAt(pBuffer, offset, size, pCompletion, pStatus);}

		//Attribute-related ops
		virtual FileFlags::FileAttribute GetFileAttributes(ActionStatus *pStatus = NULL) {return _File.GetFileAttributes(pStatus);}
		virtual ActionStatus AddFileAttributes(FileFlags::FileAttribute Attributes){return _File.AddFileAttributes(Attributes);}
		virtual ActionStatus RemoveFileAttributes(FileFlags::FileAttribute Attributes){return RemoveFileAttributes(Attributes);}
	};

	typedef _ACFile<File> ACFile;

	template <typename _Directory> class _ACDirectory : public AIDirectory
	{
	private:
		_Directory _Dir;
	
	private:
		class ACDirectoryIterator : public AIDirectoryIterator
		{
		private:
			typename _Directory::enumerator _Iter;

		public:
			ACDirectoryIterator(Directory::FindFileParams iter)
				: _Iter(iter)
			{
			}

		public:
			virtual String GetFullPath() const {return _Iter.GetFullPath();}
			virtual String GetRelativePath() const {return _Iter.GetRelativePath();}
			virtual bool IsDirectory() const {return _Iter.IsDirectory();}
			virtual bool IsAPseudoEntry() const {return _Iter.IsAPseudoEntry();}
			virtual bool Valid() const {return _Iter.Valid();}

			virtual FileFlags::FileAttribute GetAttributes(ActionStatus *pStatus = NULL) const {return _Iter.GetAttributes(pStatus);}
			virtual ActionStatus GetFileTimes(DateTime *pCreationTime, DateTime *pLastWriteTime, DateTime *pLastReadTime) const {return _Iter.GetFileTimes(pCreationTime, pLastWriteTime, pLastReadTime);}
			virtual LONGLONG GetSize(ActionStatus *pStatus = NULL) const {return _Iter.GetSize(pStatus);}

			virtual ActionStatus FindNextFile() {return _Iter.Next();}
		};

	public:
		_ACDirectory(const String &dir)
			: _Dir(dir)
		{
		}

		_ACDirectory(TCHAR *pDir)
			: _Dir(pDir)
		{
		}

		virtual ActionStatus ChangeDirectory(const String &Path) {return _Dir.ChangeDirectory(Path);}
		virtual FileFlags::FileAttribute GetFileAttributes(const String &Path, ActionStatus *pStatus = NULL){return _Dir.GetFileAttributes(Path, pStatus);}
		
		virtual ActionStatus AddFileAttributes(const String &Path, FileFlags::FileAttribute Attributes){return _Dir.AddFileAttributes(Path, Attributes);}
		virtual ActionStatus RemoveFileAttributes(const String &Path, FileFlags::FileAttribute Attributes){return _Dir.RemoveFileAttributes(Path, Attributes);}

		//! Creates a file instance and ensures that it is valid
		/*! This function tries to create or open a file and returns a pointer to a corresponding object.
			If the operation failed (file object is not valid), it is released and a value of NULL is being
			returned.
		*/
		virtual AIFile *CreateFile(const String &Path,
								   FileFlags::FileAccess Access = FileFlags::ReadAccess,
								   FileFlags::OpenMode OpenMode = FileFlags::OpenExisting,
								   FileFlags::ShareMode ShareMode = FileFlags::ShareRead,
								   FileFlags::FileAttribute Attributes = FileFlags::NormalFile,
								   ActionStatus *pStatus = NULL)
		{
			String fullPath = Path::Combine(_Dir.GetFullPath(), Path);
			return new ACFile(fullPath, FileModes::FromFileFlags(Access, OpenMode, ShareMode, Attributes), pStatus);
		}

		virtual AIDirectory *CreateDirectory(const String &Path, ActionStatus *pStatus) 
		{
			String fullPath = Path::Combine(_Dir.GetFullPath(), Path);
			ActionStatus st = Directory::Create(fullPath);
			if (!st.Successful())
			{
				if (pStatus)
					*pStatus = COPY_STATUS(st);
				return NULL;
			}
			return new _ACDirectory(fullPath);
		}
		virtual ActionStatus DeleteFile(const String &Path){return _Dir.DeleteFile(Path);}
		virtual ActionStatus RemoveDirectory(const String &Path){return _Dir.RemoveDirectory(Path);}
		virtual ActionStatus MoveFile(const String &ExistingPath, const String &NewPath, bool DeleteIfExists = false){return _Dir.MoveFile(ExistingPath, NewPath, DeleteIfExists);}

		virtual AIDirectoryIterator *FindFirstFile(const String &FileMask = _T(""))
		{
			return new ACDirectoryIterator(_Dir.FindFirstFile(FileMask));
		}
	};

	typedef _ACDirectory<Directory> ACDirectory;
}