#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "../string.h"

namespace BazisLib
{
	String ANSIStringToString(const TempStringA &str)
	{
		ANSI_STRING srcString;
		str.FillNTString(&srcString);
		String ret;
		size_t newLen = RtlAnsiStringToUnicodeSize(&srcString) / sizeof(wchar_t);
		if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(ret.ToNTString(newLen), &srcString, FALSE)))
			ret.SetLength(0);
		else
			ret.UpdateLengthFromNTString();
		return ret;
	}

	DynamicStringA StringToANSIString(const TempString &str)
	{
		UNICODE_STRING srcString;
		str.FillNTString(&srcString);
		DynamicStringA ret;
		size_t newLen = RtlUnicodeStringToAnsiSize(&srcString);
		if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(ret.ToNTString(newLen), &srcString, FALSE)))
			ret.SetLength(0);
		else
			ret.UpdateLengthFromNTString();
		return ret;
	}
}

#endif