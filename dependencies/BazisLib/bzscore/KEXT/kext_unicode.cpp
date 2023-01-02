#include "stdafx.h"

#ifdef BZSLIB_KEXT
#include "unicode.h"

using namespace BazisLib;
using namespace BazisLib::KEXT;

#include <sys/utfconv.h>

BazisLib::DynamicStringA BazisLib::KEXT::UTF16ToUTF8( const wchar16_t *pString, size_t length /*= -1*/ )
{
	if (length == -1)
	{
		for (length = 0; pString[length]; length++);
	}

	DynamicStringA result;

	size_t size = utf8_encodelen(pString, length * 2, '/', 0);
	if (!size || size == -1)
		return result;

	size_t done = 0;
	char *pCharBuf = result.PreAllocate(size, false);
	utf8_encodestr(pString, length * 2, (uint8_t *)pCharBuf, &done, size + 1, '/', 0);
	result.SetLength(done);
	return result;
}



BazisLib::_DynamicStringT<wchar16_t> BazisLib::KEXT::UTF8ToUTF16( const char *pString, size_t length /*= -1*/ )
{
	if (length == -1)
	{
		for (length = 0; pString[length]; length++);
	}

	BazisLib::_DynamicStringT<wchar16_t> result;
	size_t done = 0;
	wchar16_t *pCharBuf = result.PreAllocate(length, false);
	utf8_decodestr((const uint8_t *)pString, length, pCharBuf, &done, (length + 1) * sizeof(pCharBuf[0]), '/', 0);
	result.SetLength(done / sizeof(pCharBuf[0]));
	return result;
}


#endif