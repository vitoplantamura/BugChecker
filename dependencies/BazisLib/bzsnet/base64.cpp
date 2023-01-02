#include "stdafx.h"
#include "base64.h"

using namespace BazisLib;
using namespace BazisLib::Network;

template <class _CharType> static inline void Base64Encode(const void *pPtr, unsigned Size, _DynamicStringT<_CharType> &buffer)
{
	static const unsigned char Base64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	static const _CharType crlf[3] = {'\r', '\n'};
	buffer.clear();
	if (!Size || !pPtr)
		return;
	_CharType szTmp[5] = {0,};
	buffer.reserve((Size * 4) / 3);
	const unsigned char *Data = (const unsigned char *)pPtr;
	for (unsigned i = 0; i < Size; i += 3)
	{
		//Note that every 19 symbols the "\r\n" should be concatenated
		//to the target string, so the maximum bytes count that should
		//be added to the output buffer during one iteration is 6 plus
		//null-terminator = 7.
		unsigned tempValue;
		if ((i + 2) < Size)
			tempValue = (Data[i] << 16) | (Data[i+1] << 8) | Data[i+2];
		else
		{
			tempValue = (Data[i] << 16);
			if ((i + 1) < Size)
				tempValue |= (Data[i+1] << 8);
			if ((i + 2) < Size)
				tempValue |= (Data[i+2] << 0);
		}

		szTmp[0] = Base64Alphabet[((tempValue>>18)&0x3F)];
		szTmp[1] = Base64Alphabet[((tempValue>>12)&0x3F)];
		szTmp[2] = Base64Alphabet[((tempValue>>6)&0x3F)];;
		szTmp[3] = Base64Alphabet[(tempValue&0x3F)];

		if ((i + 1) >= Size)
			szTmp[2] = '=';		
		if ((i + 2) >= Size)
			szTmp[3] = '=';		

		buffer.append(szTmp, 4);

		if ((i % 57) == 54)
			buffer.append(crlf, 2);
	}
}

template <class _CharType, class _BufferType> static inline bool Base64Decode(const _TempStringImplT<_CharType> &string, _BufferType &buffer)
{
	static const unsigned char ReversedBase64Alphabet[256] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x3E\0\0\0\x3F\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\0\0\0\0\0\0\0\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\0\0\0\0\0\0\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33";
	size_t size = string.length();
	unsigned nchar = 0;
	register unsigned chopcnt = 0;
	register unsigned tempValue = 0;
	unsigned char tmpBuf[3];
	buffer.reserve((size * 3) / 4);
	for (size_t i = 0, j = 0; i < size; i++)
	{
		//BASE64 does not allow symbols with code less than 0x21 to
		//be used.
		_CharType tch = string[i];
		if ((tch > 0x7F) || (tch <= 0))
			continue;
		unsigned char ch = (unsigned char)tch;
		if (ch <= 0x0D)
			continue; //to filter 0x0d or 0x0a symbols (\r and \n)

		if (ch == '=')
			chopcnt++;
		else
		{
			if (chopcnt)
				return false;
		}

		switch (nchar++)
		{
		case 0:
			tempValue  = (ReversedBase64Alphabet[ch] << 18);
			break;
		case 1:
			tempValue |= (ReversedBase64Alphabet[ch] << 12);
			break;
		case 2:
			tempValue |= (ReversedBase64Alphabet[ch] << 6);
			break;
		case 3:
			tempValue |= (ReversedBase64Alphabet[ch] << 0);
			nchar = 0;
			tmpBuf[0] = (tempValue >> 16) & 0xFF;
			tmpBuf[1] = (tempValue >> 8) & 0xFF;
			tmpBuf[2] = (tempValue >> 0) & 0xFF;
			if (chopcnt > 2)
				return false;
			buffer.append(tmpBuf, 3 - chopcnt);

			break;
		}
	}
	if (nchar)
		return false;
	return true;
}

void Base64::Encode( const void *pPtr, unsigned Size, DynamicStringA &buffer )
{
	return Base64Encode(pPtr, Size, buffer);
}

void Base64::Encode( const void *pPtr, unsigned Size, DynamicStringW &buffer )
{
	return Base64Encode(pPtr, Size, buffer);
}

bool Base64::Decode( const _TempStringImplT<char> &string, BasicBuffer &buffer )
{
	return Base64Decode(string, buffer);
}

bool Base64::Decode( const _TempStringImplT<wchar_t> &string, BasicBuffer &buffer )
{
	return Base64Decode(string, buffer);
}

