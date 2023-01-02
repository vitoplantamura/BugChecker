#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "../../bzscore/string.h"

namespace BazisLib
{
	String ANSIStringToString(const TempStringA &str)
	{
#ifdef UNICODE
		String ret;
		wchar_t *pwszStr = ret.PreAllocate(str.length(), false);
		if (!pwszStr)
			return ret;
		size_t done = MultiByteToWideChar(CP_ACP, 0, str.GetConstBuffer(), (unsigned)str.length(), pwszStr, (unsigned)str.length());
		ret.SetLength(done);
		return ret;
#else
		return str;
#endif
	}

	DynamicStringA StringToANSIString(const TempString &str)
	{
#ifdef UNICODE
		DynamicStringA ret;
		char *pszStr = ret.PreAllocate(str.length(), false);
		if (!pszStr)
			return ret;
		size_t done = WideCharToMultiByte(CP_ACP, 0, str.GetConstBuffer(), (unsigned)str.length(), pszStr, (unsigned)str.length(), "?", NULL);
		ret.SetLength(done);
		return ret;
#else
		return str;
#endif
	}

//-------------------------------------------------------------------------------------------------------------------------------------

	String STLANSIStringToString(const std::string &str)
	{
#ifdef UNICODE
		return ANSIStringToString(ConstStringA(str.c_str(), str.length()));
#else
		return str.c_str();
#endif
	}

	std::string StringToANSISTLString(const TempString &str)
	{
#ifdef UNICODE
		return StringToANSIString(str).c_str();
#else
		return std::string(str.GetTempPointer(), str.length());
#endif
	}
}
#endif