#pragma once

namespace BazisLib
{
	class _PathPrivate
	{
	public:
		enum
		{
			_DirectorySeparatorChar = '/',
			_AltDirectorySeparatorChar = _DirectorySeparatorChar,
			_PathVariableSeparator = ':',
		};

		static bool IsAbsolute(const String &path)
		{
			if (path.length() < 1)
				return false;
			return path[0] == '/';
		}
	};

	enum SpecialDirectoryType 
	{
		dirTemp,
	};
}