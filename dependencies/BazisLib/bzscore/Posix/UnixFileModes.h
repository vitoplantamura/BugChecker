#pragma once

namespace BazisLib
{
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