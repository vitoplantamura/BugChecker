#pragma once

namespace BazisLib
{
	class _PathPrivate
	{
	public:
		enum
		{
			_DirectorySeparatorChar = '\\',
			_AltDirectorySeparatorChar = '/',
			_PathVariableSeparator = ';',
		};

		static bool IsAbsolute(const TempString &path)
		{
			if (path.length() < 2)
				return false;
			return path[1] == ':' || (path[0] == '\\' && path[1] == '\\');
		}
	};

	enum SpecialDirectoryType 
	{
#ifndef BZSLIB_WINKERNEL
		dirWindows = 0x8A000001, 
		dirSystem,
		dirDrivers,
		dirTemp,
#endif
	};

	namespace Win32
	{
#ifndef BZSLIB_WINKERNEL
		static inline SpecialDirectoryType SpecialDirFromCSIDL(int CSIDL)
		{
			return (SpecialDirectoryType)CSIDL;
		}
#endif
	}
}