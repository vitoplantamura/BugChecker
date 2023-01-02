#pragma once
#include "string.h"
#include <vector>

#ifdef BZSLIB_UNIXWORLD
#include "Posix/path.h"
#elif defined (BZSLIB_WINKERNEL)
#include "WinKernel/path.h"
#elif defined (_WIN32)
#include "Win32/path.h"
#else
#error Path API is not defined for current platform.
#endif

namespace BazisLib
{
	static const TCHAR _PathSeparators[2] = {_PathPrivate::_DirectorySeparatorChar, _PathPrivate::_AltDirectorySeparatorChar};

	class Path
	{
	public:
		enum 
		{
			DirectorySeparatorChar = _PathPrivate::_DirectorySeparatorChar,
			AltDirectorySeparatorChar = _PathPrivate::_AltDirectorySeparatorChar,
			PathVariableSeparator = _PathPrivate::_PathVariableSeparator,
		};

		static String Combine(const String &left, const String &right)
		{
			if (left.empty())
				return right;
			else if (right.empty())
				return left;

			if (_PathPrivate::IsAbsolute(right))
				return String();

			if (left[left.length() - 1] == DirectorySeparatorChar || left[left.length() - 1] == AltDirectorySeparatorChar)
				return left + right;
			else
			{
				TCHAR sep[2] = {DirectorySeparatorChar, 0};
				return left + sep + right;
			}
		}

		static String Combine(const String &left, const TCHAR *pRight)
		{
			return Combine(left, TempStrPointerWrapper(pRight));
		}


		static inline size_t _min(size_t a, size_t b)
		{
			if (a < b)
				return a;
			else
				return b;
		}

		static String GetDirectoryName(const TempString &path)
		{
			size_t spos1 = path.rfind(DirectorySeparatorChar), spos2 = path.rfind(AltDirectorySeparatorChar);
			size_t spos = _min(spos1, spos2);
			if (spos == -1)
				return ConstString(_T(""));
			else
				return path.substr(0, spos);
		}

		static String GetFileName(const TempString &path)
		{
			size_t spos1 = path.rfind(DirectorySeparatorChar), spos2 = path.rfind(AltDirectorySeparatorChar);
			size_t spos = _min(spos1, spos2);
			if (spos == -1)
				return path;
			else
				return path.substr(spos + 1);
		}

		static String GetPathWithoutExtension(const TempString &path)
		{
			size_t dpos = path.rfind('.');
			if (dpos == DynamicString::npos)
				return path;
			size_t spos1 = path.rfind(DirectorySeparatorChar), spos2 = path.rfind(AltDirectorySeparatorChar);
			size_t spos = _min(spos1, spos2);
			if ((spos != DynamicString::npos) && (dpos <= spos))
				return path;
			return path.substr(0, dpos);
		}
		
		static String GetFileNameWithoutExtension(const TempString &path)
		{
			return GetFileName(GetPathWithoutExtension(path));
		}

		static String GetExtensionExcludingDot(const TempString &path)
		{
			size_t dpos = path.rfind('.');
			if (dpos == DynamicString::npos)
				return ConstString(_T(""));
			size_t spos1 = path.rfind(DirectorySeparatorChar), spos2 = path.rfind(AltDirectorySeparatorChar);
			size_t spos = _min(spos1, spos2);
			if ((spos != DynamicString::npos) && (dpos <= spos))
				return ConstString(_T(""));
			return path.substr(dpos + 1);
		}

		static bool IsAbsolute(const TempString &path)
		{
			return _PathPrivate::IsAbsolute(path);
		}

		static std::vector<String> GetAllRootPaths();

		static String GetCurrentPath();
		static String GetSpecialDirectoryLocation(SpecialDirectoryType dir);
	};
}