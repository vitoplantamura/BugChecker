#pragma once
namespace BazisLib
{
	namespace FileHelpers
	{
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
	}
}