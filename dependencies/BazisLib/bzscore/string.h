#pragma once
#include "cmndef.h"

#include <string>
#include "strbase.h"
#include "tchar_compat.h"

#ifdef BZSLIB_KEXT
#include <libkern/libkern.h>
#endif

namespace BazisLib
{
	class istring : public std::basic_string<char, case_insensitive_char_traits<char> >
	{
	private:
		typedef std::basic_string<char, case_insensitive_char_traits<char> > _base;
	public:
		istring() {}

		istring(std::string &str)
			: _base(str.c_str())
		{}

		istring(const char *str)
			: _base(str)
		{}

		operator std::string() const
		{
			return std::string(c_str());
		}
	};

	class iwstring : public std::basic_string<wchar_t, case_insensitive_char_traits<wchar_t> >
	{
	private:
		typedef std::basic_string<wchar_t, case_insensitive_char_traits<wchar_t> > _base;
	public:
		iwstring() {}

		iwstring(std::wstring &str)
			: _base(str.c_str())
		{}

		iwstring(const wchar_t *str)
			: _base(str)
		{}

		iwstring(const wchar_t *str, size_t len)
			: _base(str, len)
		{}

		operator std::wstring() const
		{
			return std::wstring(c_str());
		}
	};

	//Provides efficient method for splitting STL strings by a given token
	template <class _StringT> class _SplitStrT
	{
	public:
		_StringT left;
		_StringT right;
		bool valid;
	public:
		_SplitStrT(const _StringT &str, const _StringT &token) :
		  valid(false)
		{
			size_t idx = str.find(token);
			if (idx == -1)
				return;
			left = str.substr(0, idx);
			right = str.substr(idx + token.length());
			valid = true;
		}

		 operator bool() {return valid;}
	};

	template <class _StringT> class _SplitStrByFirstOfT
	{
	public:
		_StringT left;
		_StringT right;
		bool valid;
	public:
		_SplitStrByFirstOfT(const _StringT &str, const _StringT &chars)
			: valid(false)
			, left(str.substr(0,0))
			, right(str.substr(0,0))
		  {
			  size_t idx = str.find_first_of(chars);
			  if (idx == -1)
				  return;
			  left = str.substr(0, idx);
			  idx = str.find_first_not_of(chars, idx);
			  if (idx == -1)
				  return;
			  right = str.substr(idx);
			  valid = true;
		  }

		  operator bool() {return valid;}
	};

	static inline std::string stl_itoa(unsigned number, unsigned radix = 10)
	{
		char sz[40];
#ifdef BZSLIB_KEXT
		if (radix == 16)
			snprintf(sz, sizeof(sz), "%x", number);
		else
			snprintf(sz, sizeof(sz), "%d", number);
#elif defined BZSLIB_UNIXWORLD
		if (radix == 16)
			sprintf(sz, "%x", number);
		else
			sprintf(sz, "%d", number);
#else
		_itoa_s(number, sz, sizeof(sz), radix);
#endif
		return std::string(sz);
	}

#ifdef BZSLIB_KEXT
	static int tolower(unsigned char ch)
	{
		if (ch >= 'A' && ch <= 'Z')
			ch |= 0x20;

		return ch;
	}
#endif

	/*Profiling results (10K iterations):
		strstr():					2.6K (ref)
		strstr() with c_str():		3.0K (1.15x)
		stl_stistr():
		case-sensitive, raw:		10.4K (4x)
		case-sensitive, byteopt:	4.2K  (1.6x)
		case-sensitive, dwordopt:	1.7K  (0.6x)
		case-insensitive, raw:		27.9K (10.7x)
		case-insensitive, byteopt:	17.2K (6.6x)

	  Profiling strings:
		char str[] = "dependency information contained in any manifest files be converted to &quot;#pragma comment(linker,&quot;&lt;insert dependency here&gt;&quot;)&quot; in a";
		char sub[] = "endency here";
	*/
	static inline unsigned stl_stristr(const std::string &str, const std::string &token)
	{
		if (str.empty() || token.empty())
			return -1;
		size_t cbStr = str.length(), cbToken = token.length();
		if (cbStr < cbToken)
			return -1;
		const char *pStr = str.c_str();
		const char *pToken = token.c_str();
		//TODO: optimize
		cbStr -= (cbToken - 1);
		char token0 = (char)tolower(pToken[0]);
		for (unsigned idx = 0; idx < cbStr ; idx++)
			if ((tolower(pStr[idx]) == token0) && !case_insensitive_char_traits<char>::compare(((const char *)pStr) + idx, pToken, cbToken))
				return idx;
		return -1;
	}

	static inline bool CheckStringLength(const wchar_t *pwszString, unsigned MaxSize)
	{
		if (!pwszString)
			return false;
		for (unsigned i = 0; i < MaxSize; i++)
			if (!pwszString[i])
				return true;
		return false;
	}

	template <size_t arraySize> static inline bool CheckStringLengthArray(const wchar_t (&wszString)[arraySize])
	{
		if (!wszString)
			return false;
		for (size_t i = 0; i < arraySize; i++)
			if (!wszString[i])
				return true;
		return false;
	}


#if defined(_WIN32) && !defined(BZSLIB_WINKERNEL)
	typedef std::basic_string<TCHAR> tstring;
#endif

	typedef DynamicString String;

	typedef _SplitStrT<std::string> SplitStrA;
	typedef _SplitStrT<String> SplitStr;
	typedef _SplitStrT<istring> SplitStrI;

	typedef _SplitStrByFirstOfT<TempString>  SplitStrByFirstOf;
	typedef _SplitStrByFirstOfT<TempStringA> SplitStrByFirstOfA;
	typedef _SplitStrByFirstOfT<TempStringW> SplitStrByFirstOfW;

	//The following functions should be implemented inside platform support libraries (bzswin, bzsddk, etc.)
	String ANSIStringToString(const TempStringA &str);
	DynamicStringA StringToANSIString(const TempString &str);

	String STLANSIStringToString(const std::string &str);
	std::string StringToANSISTLString(const TempString &str);

}

#ifdef BZSLIB_WINKERNEL
#include "WinKernel/string.h"
#endif