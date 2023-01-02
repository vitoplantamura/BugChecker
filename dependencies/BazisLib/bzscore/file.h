#pragma once

#include "cmndef.h"
#include "datetime.h"
#include "status.h"
#include "string.h"
#include "fileflags.h"
#include "path.h"

#ifdef BZS_POSIX
#include "Posix/file.h"
#elif defined (BZSLIB_KEXT)
#include "KEXT/file.h"
#elif defined (BZSLIB_WINKERNEL)
#include "WinKernel/file.h"
#elif defined (_WIN32)
#include "Win32/file.h"
#else
#error File API is not defined for current platform.
#endif

namespace BazisLib
{
	//Platform-specific one. Currently unsupported.
	class IoCompletion;

	//Defined in platform-specific file
	using FileFlags::SeekType;
	using FileFlags::FileAttribute;

	using BazisLib::PlatformSpecificFile;

	class File : public PlatformSpecificFile
	{
	public:
		File(const TCHAR *ptszFileName, const FileMode &mode, ActionStatus *pStatus = NULL)
			: PlatformSpecificFile(ptszFileName, mode, pStatus)
		{
		}

		File(const String &FileName, const FileMode &mode, ActionStatus *pStatus = NULL)
			: PlatformSpecificFile(FileName, mode, pStatus)
		{
		}

		using PlatformSpecificFile::Write;
		using PlatformSpecificFile::Read;
		using PlatformSpecificFile::GetSize;
		using PlatformSpecificFile::Seek;
		using PlatformSpecificFile::GetPosition;
		using PlatformSpecificFile::Close;
		using PlatformSpecificFile::Valid;
		using PlatformSpecificFile::GetFileTimes;
		using PlatformSpecificFile::SetFileTimes;
		using PlatformSpecificFile::GetFileAttributes;
		using PlatformSpecificFile::AddFileAttributes;
		using PlatformSpecificFile::RemoveFileAttributes;

		using PlatformSpecificFile::Exists;
		using PlatformSpecificFile::Delete;

	private:
		template <class _Ch> struct _LineEndingHelper{};

	public:
		template <class _CharType> size_t WriteString(const _CharType *pBuffer, size_t CharCount = -1, ActionStatus *pStatus = NULL)
		{
			if (CharCount == -1)
				CharCount = std::char_traits<_CharType>::length(pBuffer);
			return Write(pBuffer, CharCount * sizeof(_CharType), pStatus);
		}

		template <class _CharType> size_t WriteString(const _TempStringImplT<_CharType> &str, ActionStatus *pStatus = NULL)
		{
			return WriteString(str.GetConstBuffer(), str.length(), pStatus);
		}

		template <class _CharType> size_t WriteLine(const _TempStringImplT<_CharType> &str, ActionStatus *pStatus = NULL)
		{
			size_t done1 = WriteString(str.GetConstBuffer(), str.length(), pStatus);
			if (done1 == -1)
				return -1;
			size_t done2 = WriteString(_LineEndingHelper<_CharType>::get());
			if (done2 == -1)
				return -1;
			return done1 + done2;
		}

		static bool Exists(const String &filePath)
		{
			return PlatformSpecificFile::Exists(filePath.c_str());
		}

		static ActionStatus Delete(const String &filePath)
		{
			return PlatformSpecificFile::Delete(filePath.c_str());
		}

		template <typename _CharType> static _DynamicStringT<_CharType> ReadAll(const String &filePath)
		{
			_DynamicStringT<_CharType> result;
			File file(filePath, FileModes::OpenReadOnly);
			if (!file.Valid())
				return result;

			size_t size = (size_t)file.GetSize();
			if (size == -1)
				return result;

			_CharType *pBuf = result.PreAllocate(size / sizeof(_CharType), false);
			if (!pBuf)
				return result;

			size_t done = file.Read(pBuf, size);
			if (done != size)
				return result;

			result.SetLength(done / sizeof(_CharType));
			return result;
		}

		template <typename _CharType> static bool WriteAll(const String &filePath, const _DynamicStringT<_CharType> &value)
		{
			File file(filePath, FileModes::CreateOrTruncateRW);
			if (!file.Valid())
				return false;

			size_t done = file.WriteString(value);
			return (done == value.SizeInBytes());
		}

	};

	template<> struct File::_LineEndingHelper<char> { static const char *get() {return LINE_ENDING_STR_A;}};
	template<> struct File::_LineEndingHelper<wchar_t> { static const wchar_t *get() {return LINE_ENDING_STR_W;}};

	class Directory : public PlatformSpecificDirectory
	{
	public:
		Directory(const String &dir)
			: PlatformSpecificDirectory(dir)
		{
		}

		using PlatformSpecificDirectory::enumerator;
		using PlatformSpecificDirectory::ChangeDirectory;
		using PlatformSpecificDirectory::GetFileAttributes;
		using PlatformSpecificDirectory::AddFileAttributes;
		using PlatformSpecificDirectory::RemoveFileAttributes;
		using PlatformSpecificDirectory::CreateDirectory;
		using PlatformSpecificDirectory::DeleteFile;
		using PlatformSpecificDirectory::RemoveDirectory;
		using PlatformSpecificDirectory::MoveFile;
		using PlatformSpecificDirectory::FindFirstFile;
		
		using PlatformSpecificDirectory::Exists;
		using PlatformSpecificDirectory::Create;
		using PlatformSpecificDirectory::Remove;

		using PlatformSpecificDirectory::GetFullPath;
	};

	using FileModes::CreateOrOpenRW;
	using FileModes::CreateOrTruncateRW;
	using FileModes::CreateNewRW;
	using FileModes::OpenReadOnly;
	using FileModes::OpenReadWrite;
	using FileModes::OpenForSecurityQuery;
	using FileModes::OpenForSecurityModify;
}
