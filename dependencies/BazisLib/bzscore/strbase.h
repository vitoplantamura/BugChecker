#pragma once
#include "cmndef.h"
#include <string>
#include "assert.h"
#include <functional>
#include "splitstr.h"
#include "strfast.h"
#include "tchar_compat.h"

#ifdef BZSLIB_WINKERNEL
#include "WinKernel/kmemory.h"
#include <stdarg.h>
#endif

#ifdef BZSLIB_KEXT
#include <stdarg.h>
#include <libkern/libkern.h>
#endif

#ifdef BZSLIB_POSIX
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#endif


/*! \page bzslib_string BazisLib string classes
	BazisLib has a relatively complex model of string classes providing a significantly higher performance compared to standard STL string class. The main reasons
	for creating this class set are the following:
	<ul>
		<li>Do not copy constant strings to heap when calling a function taking a string reference.</li>
		<li>Allow considering a substring of an existing string without making a copy.</li>
		<li>Allow pre-allocating a given amount of memory and retrieving a writable buffer (to make API functions write directly to a string buffer)</li>
		<li>Support various small features, such as a sprintf() equivalent performing automatic resizing and so on</li>
		<li>Maintain maximum compatibility with STL string methods</li>
		<li>Simplified case-insensitive comparing and searching</li>
	</ul>
	The string framework consists of the following classes:
	<ul>
		<li>BazisLib::String or BazisLib::DynamicString - represents a usual string (allocated from heap or using a small embedded buffer for short strings).</li>
		<li>BazisLib::ConstString - represents a constant string (for example, a string literal)</li> 
		<li>BazisLib::TempString - base class for String and ConstString. Should be used as input type for functions requiring strings.</li>
	</ul>
	The main performance boost is gained due to the TempString class. It allows representing a part of an existing string (DynamicString, ConstString or another TempString).
	A TempString class does not contain the c_str() method, that way it does not need to have a separate null-terminated copy of the string. On the other hand, TempString should
	be used more carefully, as modifying the original string before using a corresponding TempString can lead to unpredictable results.
	\sections usecases Recommended use cases
	<ul>
		<li>When a function needs a constant string as an argument it is recommended to use a reference to BazisLib::TempString</li>
		<li>When a function returns a string, it should return BazisLib::String</li>
		<li>When you need to make an API function write into a BazisLib::String, please call the PreAllocate() method and the SetLength() method afterwards</li>
		<li>When you need to shortly examine a substring of a function (compare it or scan it), use the STL-compatible substr() method that returns a TempString</li>
		<li>When you need to perform a case-insensitive comparing or finding, use the icompare() or ifind() method</li>
		<li>When you want to perform formatting operation (similar to sprintf()) to a string, but you do not know the result length, use the Format() method</li>
		<li>Avoid using BazisLib::TempString instance after a corresponding BazisLib::String was modified or deleted</li>
	</ul>
*/

namespace BazisLib
{
#pragma region Case-insensitive char traits
	template <class _CharT>	class case_insensitive_char_traits : public std::char_traits<_CharT> {};

	template<> class case_insensitive_char_traits<char> : public std::char_traits<char> 
	{
	public:
#if defined(_WIN32) && !defined(BZSLIB_WINKERNEL)
#if _MSC_VER < 1600
		typedef std::_Secure_char_traits_tag _Secure_char_traits;
#endif
#endif

		static int compare(const char* __s1, const char* __s2, size_t __n)
		{
#ifdef BZSLIB_UNIXWORLD
			return strncasecmp(__s1, __s2, __n);
#else
			return _memicmp(__s1, __s2, __n); 
#endif
		}

		static bool eq(const char& _Left, const char& _Right)
		{ return ((_Left | 0x20) == (_Right | 0x20)); }

		static bool lt(const char& _Left, const char& _Right)
		{ return ((_Left | 0x20) < (_Right | 0x20)); }

#if 0
		static int icmp(const char *_s1, const char *_s2)
		{
#ifdef BZSLIB_UNIXWORLD
			return strcasecmp(_s1, _s2);
#else
			return _stricmp(_s1, _s2);
#endif
		}
#endif

		static const char *find(const char *_First, size_t _Count, const char& _Ch)
		{
			for (; 0 < _Count; --_Count, ++_First)
				if (eq(*_First, _Ch))
					return (_First);
			return (0);
		}
	};

	template<> class case_insensitive_char_traits<wchar_t> : public std::char_traits<wchar_t> 
	{
	public:
#if defined(_WIN32) && !defined(BZSLIB_WINKERNEL)
#if _MSC_VER < 1600
		typedef std::_Secure_char_traits_tag _Secure_char_traits;
#endif
#endif

		static int compare(const wchar_t* __s1, const wchar_t* __s2, size_t __n)
		{ 
			for (; 0 < __n; --__n, ++__s1, ++__s2)
			{
				if (lt(*__s1, *__s2))
					return -1;
				else
					if (!eq(*__s1, *__s2))
						return 1;
			}
			return 0;
		}

#if 0
		static int icmp(const wchar_t *_s1, const wchar_t *_s2)
		{
#ifdef BZSLIB_UNIXWORLD
			return wcscasecmp(_s1, _s2);
#else
			return _wcsicmp(_s1, _s2);
#endif
		}
#endif

		static bool eq(const wchar_t& _Left, const wchar_t& _Right)
		{ return ((_Left | 0x20) == (_Right | 0x20)); }

		static bool lt(const wchar_t& _Left, const wchar_t& _Right)
		{ return ((_Left | 0x20) < (_Right | 0x20)); }

		static const wchar_t *find(const wchar_t *_First, size_t _Count, const wchar_t& _Ch)
		{
			for (; 0 < _Count; --_Count, ++_First)
				if (eq(*_First, _Ch))
					return (_First);
			return (0);
		}
	};
#pragma endregion

#ifdef BZSLIB_WINKERNEL
	template <typename _CharType> class NTStringHelper {};

	template<> class NTStringHelper<wchar_t> {public: typedef UNICODE_STRING _Type;};
	template<> class NTStringHelper<char> {public: typedef ANSI_STRING _Type;};
#endif

	template <class _CharType> class _TempStringBaseT
	{
	private:
		_CharType *m_pBuffer;
		size_t m_Length;

	protected:
		//Child classes should not access buffer and length directly, as different platforms may have 
		//special requirements (such as embedded UNICODE_STRING for DDK)
		//The returned string is not required to be null-terminated
		_CharType *GetBuffer() const {return m_pBuffer;}
		void SetBuffer(_CharType *pBuf) {m_pBuffer = pBuf;}
		size_t GetLength() const {return m_Length;}
		void SetLength(size_t len) {m_Length = len;}

		template <typename _CharType2, class _Traits2, size_t _EmbeddedArraySize2, class _Allocator2> friend class _DynamicStringT;

	protected:
		_TempStringBaseT(_CharType *pBuffer, size_t size)
			: m_pBuffer(pBuffer)
			, m_Length(size)
		{
		}

	public:
		enum {npos = -1};
	};

	template <typename _CharType, class _Traits = std::char_traits<_CharType> > class _TempStringImplT : public _TempStringBaseT<_CharType>
	{
	public:
		typedef _TempStringBaseT<_CharType> _Base;

		typedef _TempStringImplT _Myt;
		typedef size_t size_type;
		typedef _CharType _Elem, value_type;

		enum {npos = _Base::npos};

	protected:
		_TempStringImplT(_CharType *pBuffer, size_t size)
			: _Base(pBuffer, size)
		{
		}

	public:
#pragma region compare() methods
		int compare(const _Myt& _Right) const
		{
			return (compare(0, _Base::GetLength(), _Right.GetBuffer(), _Right.GetLength()));
		}

		int compare(size_type _Off, size_type _N0,
			const _Myt& _Right) const
		{
			return (compare(_Off, _N0, _Right, 0, npos));
		}

		int compare(size_type _Off,
			size_type _N0, const _Myt& _Right,
			size_type _Roff, size_type _Count) const
		{
			if (_Right.GetLength() < _Roff)
				return 0;	//Nothing to compare			
			if (_Right.GetLength() - _Roff < _Count)
				_Count = _Right.GetLength() - _Roff;	// trim _Count to size
			return (compare(_Off, _N0, _Right.GetBuffer() + _Roff, _Count));
		}

		int compare(const _Elem *_Ptr) const
		{
			ASSERT(_Ptr);
			return (compare(0, _Base::GetLength(), _Ptr, _Traits::length(_Ptr)));
		}

		int compare(size_type _Off, size_type _N0, const _Elem *_Ptr) const
		{
			ASSERT(_Ptr);
			return (compare(_Off, _N0, _Ptr, _Traits::length(_Ptr)));
		}

		int compare(size_type _Off,
			size_type _N0, const _Elem *_Ptr, size_type _Count) const
		{

			if (_Base::GetLength() < _Off)
				return 0;//Nothing to compare
			if (_Base::GetLength() - _Off < _N0)
				_N0 = _Base::GetLength() - _Off;	// trim _N0 to size

			size_type _Ans = _Traits::compare(_Base::GetBuffer() + _Off, _Ptr,
				_N0 < _Count ? _N0 : _Count);
			return (_Ans != 0 ? (int)_Ans : _N0 < _Count ? -1
				: _N0 == _Count ? 0 : +1);
		}
#pragma endregion
#pragma region icompare() methods
		int icompare(const _Myt& _Right) const
		{
			return (icompare(0, _Base::GetLength(), _Right.GetBuffer(), _Right.GetLength()));
		}

		int icompare(size_type _Off, size_type _N0,
			const _Myt& _Right) const
		{
			return (icompare(_Off, _N0, _Right, 0, npos));
		}

		int icompare(size_type _Off,
			size_type _N0, const _Myt& _Right,
			size_type _Roff, size_type _Count) const
		{
			if (_Right.GetLength() < _Roff)
				return 0;	//Nothing to compare			
			if (_Right.GetLength() - _Roff < _Count)
				_Count = _Right.GetLength() - _Roff;	// trim _Count to size
			return (icompare(_Off, _N0, _Right.GetBuffer() + _Roff, _Count));
		}

		int icompare(const _Elem *_Ptr) const
		{
			ASSERT(_Ptr);
			return (icompare(0, _Base::GetLength(), _Ptr, _Traits::length(_Ptr)));
		}

		int icompare(size_type _Off, size_type _N0, const _Elem *_Ptr) const
		{
			ASSERT(_Ptr);
			return (icompare(_Off, _N0, _Ptr, _Traits::length(_Ptr)));
		}

		int icompare(size_type _Off,
			size_type _N0, const _Elem *_Ptr, size_type _Count) const
		{

			if (_Base::GetLength() < _Off)
				return 0;//Nothing to compare
			if (_Base::GetLength() - _Off < _N0)
				_N0 = _Base::GetLength() - _Off;	// trim _N0 to size

			size_type _Ans = case_insensitive_char_traits<_CharType>::compare(_Base::GetBuffer() + _Off, _Ptr,
				_N0 < _Count ? _N0 : _Count);
			return (_Ans != 0 ? (int)_Ans : _N0 < _Count ? -1
				: _N0 == _Count ? 0 : +1);
		}
#pragma endregion
#pragma region Various helper methods
		size_type length() const
		{
			return _Base::GetLength();
		}

		size_type size() const
		{
			return _Base::GetLength();
		}

		bool empty() const
		{
			return !_Base::GetLength();
		}

		const _CharType *GetTempPointer() const
		{
			return _Base::GetBuffer();
		}

		size_t SizeInBytes(bool IncludingNullCharacter = false) const
		{
			if (IncludingNullCharacter)
				return (_Base::GetLength() + 1) * sizeof(_CharType);
			else
				return _Base::GetLength() * sizeof(_CharType);
		}
#pragma endregion
#pragma region find() methods
		size_type find(const _Myt& _Right, size_type _Off = 0) const
		{
			return (find(_Right.GetBuffer(), _Off, _Right.GetLength()));
		}

		size_type find(const _Elem *_Ptr,
			size_type _Off, size_type _Count) const
		{
			if (_Count == 0 && _Off <= _Base::GetLength())
				return (_Off);	// null string always matches (if inside string)

			size_type _Nm;
			if (_Off < _Base::GetLength() && _Count <= (_Nm = _Base::GetLength() - _Off))
			{	// room for match, look for it
				const _Elem *_Uptr, *_Vptr;
				for (_Nm -= _Count - 1, _Vptr = _Base::GetBuffer() + _Off;
					(_Uptr = _Traits::find(_Vptr, _Nm, *_Ptr)) != 0;
					_Nm -= _Uptr - _Vptr + 1, _Vptr = _Uptr + 1)
					if (_Traits::compare(_Uptr, _Ptr, _Count) == 0)
						return (_Uptr - _Base::GetBuffer());	// found a match
			}

			return (npos);	// no match
		}

		size_type find(const _Elem *_Ptr, size_type _Off = 0) const
		{
			ASSERT(_Ptr);
			return (find(_Ptr, _Off, _Traits::length(_Ptr)));
		}

		size_type find(_Elem _Ch, size_type _Off = 0) const
		{
			return (find((const _Elem *)&_Ch, _Off, 1));
		}

#pragma endregion
#pragma region ifind() methods
		size_type ifind(const _Myt& _Right, size_type _Off = 0) const
		{
			return (ifind(_Right.GetBuffer(), _Off, _Right.GetLength()));
		}

		size_type ifind(const _Elem *_Ptr,
			size_type _Off, size_type _Count) const
		{
			if (_Count == 0 && _Off <= _Base::GetLength())
				return (_Off);	// null string always matches (if inside string)

			size_type _Nm;
			if (_Off < _Base::GetLength() && _Count <= (_Nm = _Base::GetLength() - _Off))
			{	// room for match, look for it
				const _Elem *_Uptr, *_Vptr;
				for (_Nm -= _Count - 1, _Vptr = _Base::GetBuffer() + _Off;
					(_Uptr = case_insensitive_char_traits<_CharType>::find(_Vptr, _Nm, *_Ptr)) != 0;
					_Nm -= _Uptr - _Vptr + 1, _Vptr = _Uptr + 1)
					if (case_insensitive_char_traits<_CharType>::compare(_Uptr, _Ptr, _Count) == 0)
						return (_Uptr - _Base::GetBuffer());	// found a match
			}

			return (npos);	// no match
		}

		size_type ifind(const _Elem *_Ptr, size_type _Off = 0) const
		{
			ASSERT(_Ptr);
			return (ifind(_Ptr, _Off, _Traits::length(_Ptr)));
		}

		size_type ifind(_Elem _Ch, size_type _Off = 0) const
		{
			return (ifind((const _Elem *)&_Ch, _Off, 1));
		}

#pragma endregion
#pragma region rfind() methods
		size_type rfind(const _Myt& _Right, size_type _Off = npos) const
		{
			return (rfind(_Right.GetBuffer(), _Off, _Right.GetLength()));
		}

		size_type rfind(const _Elem *_Ptr,
			size_type _Off, size_type _Count) const
		{
			if (_Count == 0)
				return (_Off < _Base::GetLength() ? _Off : _Base::GetLength());	// null always matches
			if (_Count <= _Base::GetLength())
			{	// room for match, look for it
				const _Elem *_Uptr = _Base::GetBuffer() +
					(_Off < _Base::GetLength() - _Count ? _Off : _Base::GetLength() - _Count);
				for (; ; --_Uptr)
					if (_Traits::eq(*_Uptr, *_Ptr)
						&& _Traits::compare(_Uptr, _Ptr, _Count) == 0)
						return (_Uptr - _Base::GetBuffer());	// found a match
					else if (_Uptr == _Base::GetBuffer())
						break;	// at beginning, no more chance for match
			}

			return (npos);	// no match
		}

		size_type rfind(const _Elem *_Ptr, size_type _Off = npos) const
		{
			ASSERT(_Ptr);
			return (rfind(_Ptr, _Off, _Traits::length(_Ptr)));
		}

		size_type rfind(_Elem _Ch, size_type _Off = npos) const
		{
			return (rfind((const _Elem *)&_Ch, _Off, 1));
		}
#pragma endregion
#pragma region find_first_of() methods
		size_type find_first_of(const _Myt& _Right,
			size_type _Off = 0) const
		{
			return (find_first_of(_Right.GetBuffer(), _Off, _Right.GetLength()));
		}

		size_type find_first_of(const _Elem *_Ptr,
			size_type _Off, size_type _Count) const
		{
			if (0 < _Count && _Off < _Base::GetLength())
			{	// room for match, look for it
				const _Elem *const _Vptr = _Base::GetBuffer() + _Base::GetLength();
				for (const _Elem *_Uptr = _Base::GetBuffer() + _Off; _Uptr < _Vptr; ++_Uptr)
					if (_Traits::find(_Ptr, _Count, *_Uptr) != 0)
						return (_Uptr - _Base::GetBuffer());	// found a match
			}

			return (npos);	// no match
		}

		size_type find_first_of(const _Elem *_Ptr, size_type _Off = 0) const
		{
			ASSERT(_Ptr);
			return (find_first_of(_Ptr, _Off, _Traits::length(_Ptr)));
		}

		size_type find_first_of(_Elem _Ch, size_type _Off = 0) const
		{
			return (find((const _Elem *)&_Ch, _Off, 1));
		}
#pragma endregion
#pragma region find_last_of() methods
		size_type find_last_of(const _Myt& _Right,
			size_type _Off = npos) const
		{
			return (find_last_of(_Right.GetBuffer(), _Off, _Right.size()));
		}

		size_type find_last_of(const _Elem *_Ptr,
			size_type _Off, size_type _Count) const
		{

			if (0 < _Count && 0 < _Base::GetLength())
				for (const _Elem *_Uptr = _Base::GetBuffer()
					+ (_Off < _Base::GetLength() ? _Off : _Base::GetLength() - 1); ; --_Uptr)
					if (_Traits::find(_Ptr, _Count, *_Uptr) != 0)
						return (_Uptr - _Base::GetBuffer());	// found a match
					else if (_Uptr == _Base::GetBuffer())
						break;	// at beginning, no more chance for match

			return (npos);	// no match
		}

		size_type find_last_of(const _Elem *_Ptr,
			size_type _Off = npos) const
		{
			ASSERT(_Ptr);
			return (find_last_of(_Ptr, _Off, _Traits::length(_Ptr)));
		}

		size_type find_last_of(_Elem _Ch, size_type _Off = npos) const
		{
			return (rfind((const _Elem *)&_Ch, _Off, 1));
		}
#pragma endregion
#pragma region find_first_not_of() methods
		size_type find_first_not_of(const _Myt& _Right,
			size_type _Off = 0) const
		{
			return (find_first_not_of(_Right.GetBuffer(), _Off,
				_Right.size()));
		}

		size_type find_first_not_of(const _Elem *_Ptr,
			size_type _Off, size_type _Count) const
		{
			if (_Off < _Base::GetLength())
			{	// room for match, look for it
				const _Elem *const _Vptr = _Base::GetBuffer() + _Base::GetLength();
				for (const _Elem *_Uptr = _Base::GetBuffer() + _Off; _Uptr < _Vptr; ++_Uptr)
					if (_Traits::find(_Ptr, _Count, *_Uptr) == 0)
						return (_Uptr - _Base::GetBuffer());
			}
			return (npos);
		}

		size_type find_first_not_of(const _Elem *_Ptr,
			size_type _Off = 0) const
		{
			ASSERT(_Ptr);
			return (find_first_not_of(_Ptr, _Off, _Traits::length(_Ptr)));
		}

		size_type find_first_not_of(_Elem _Ch, size_type _Off = 0) const
		{
			return (find_first_not_of((const _Elem *)&_Ch, _Off, 1));
		}
#pragma endregion
#pragma region find_last_not_of() methods
		size_type find_last_not_of(const _Myt& _Right,
			size_type _Off = npos) const
		{
			return (find_last_not_of(_Right.GetBuffer(), _Off, _Right.size()));
		}

		size_type find_last_not_of(const _Elem *_Ptr,
			size_type _Off, size_type _Count) const
		{

			if (0 < _Base::GetLength())
				for (const _Elem *_Uptr = _Base::GetBuffer()
					+ (_Off < _Base::GetLength() ? _Off : _Base::GetLength() - 1); ; --_Uptr)
					if (_Traits::find(_Ptr, _Count, *_Uptr) == 0)
						return (_Uptr - _Base::GetBuffer());
					else if (_Uptr == _Base::GetBuffer())
						break;
			return (npos);
		}

		size_type find_last_not_of(const _Elem *_Ptr,
			size_type _Off = npos) const
		{
			ASSERT(_Ptr);
			return (find_last_not_of(_Ptr, _Off, _Traits::length(_Ptr)));
		}

		size_type find_last_not_of(_Elem _Ch, size_type _Off = npos) const
		{
			return (find_last_not_of((const _Elem *)&_Ch, _Off, 1));
		}
#pragma endregion
#pragma region other STL-related methods
		_Myt substr(size_type _Off = 0, size_type _Count = npos) const
		{
			if (_Off >= _Base::GetLength())
				return _Myt(NULL, 0);
			if (_Count == npos)
				_Count = _Base::GetLength() - _Off;
			else if (_Count >= (_Base::GetLength() - _Off))
				_Count = _Base::GetLength() - _Off;
			return (_Myt(_Base::GetBuffer() + _Off, _Count));
		}

		size_t max_size()
		{
			return (size_t)-1LL;
		}

		const _CharType &operator[](size_t idx) const
		{
			if (idx >= _Base::GetLength())
				return *((_CharType *)NULL);
			return _Base::GetBuffer()[idx];
		}

		const _CharType &at(size_t idx) const
		{
			if (idx >= _Base::GetLength())
				return *((_CharType *)NULL);
			return _Base::GetBuffer()[idx];
		}
#pragma endregion
	public:
		const _CharType *GetConstBuffer() const
		{
			return _Base::GetBuffer();
		}

#ifdef BZSLIB_WINKERNEL
	public:
		void FillNTString(typename NTStringHelper<_CharType>::_Type *pString) const
		{
			pString->Buffer = GetBuffer();
			pString->Length = (USHORT)(_Base::GetLength() * sizeof(_CharType));
			pString->MaximumLength = pString->Length;
		}

		operator typename NTStringHelper<_CharType>::_Type() const
		{
			typename NTStringHelper<_CharType>::_Type result;
			FillNTString(&result);
			return result;
		}

#endif
	public:
		//! See the _StringSplitterT reference for details
		_StringSplitterT<_Myt, const _CharType *, SetOfCharactersSplitPolicy<_Myt, const _CharType *> > Split(const _CharType *pDelimiterCharacters) const
		{
			ASSERT(pDelimiterCharacters);
			return _StringSplitterT<_Myt, const _CharType *, SetOfCharactersSplitPolicy<_Myt, const _CharType *> >(*this, pDelimiterCharacters);
		}

		//! See the _StringSplitterT reference for details
		_StringSplitterT<_Myt, const _Myt &, SetOfCharactersSplitPolicy<_Myt, const _Myt &> > Split(const _Myt &DelimiterCharacters) const
		{
			return _StringSplitterT<_Myt, const _Myt &, SetOfCharactersSplitPolicy<_Myt, const _Myt &> >(*this, DelimiterCharacters);
		}

		//! See the _StringSplitterT reference for details
		_StringSplitterT<_Myt, const _CharType *, ExactMatchSplitPolicy<_Myt, const _CharType *> > SplitByMarker(const _CharType *pExactMarker) const
		{
			ASSERT(pExactMarker);
			return _StringSplitterT<_Myt, const _CharType *, ExactMatchSplitPolicy<_Myt, const _CharType *> >(*this, pExactMarker);
		}

		//! See the _StringSplitterT reference for details
		_StringSplitterT<_Myt, const _Myt &, ExactMatchSplitPolicy<_Myt, const _Myt &> > SplitByMarker(const _Myt &ExactMarker) const
		{
			return _StringSplitterT<_Myt, const _Myt &, ExactMatchSplitPolicy<_Myt, const _Myt &> >(*this, ExactMarker);
		}

		_StringSplitterT<_Myt, _CharType, ExactMatchSplitPolicy<_Myt, _CharType > > SplitByMarker(_CharType ExactMarker) const
		{
			return _StringSplitterT<_Myt, _CharType , ExactMatchSplitPolicy<_Myt, _CharType> >(*this, ExactMarker);
		}

		FastStringRoutines::_ConfigLineSplitter<_Myt> SplitConfigLine(_CharType CommentChar = 0) const
		{
			return FastStringRoutines::_ConfigLineSplitter<_Myt>(*this, CommentChar);
		}

		_Myt Strip(const _CharType *CharsToStrip)
		{
			ASSERT(CharsToStrip);
			off_t start = find_first_not_of(CharsToStrip);
			off_t end = find_last_not_of(CharsToStrip);
			if (end == -1)
				return substr(start);
			else
				return substr(start, end - start + 1);
		}

		_Myt StripLeft(const _CharType *CharsToStrip)
		{
			ASSERT(CharsToStrip);
			return substr(find_first_not_of(CharsToStrip));
		}

		_Myt StripRight(const _CharType *CharsToStrip)
		{
			ASSERT(CharsToStrip);
			off_t end = find_last_not_of(CharsToStrip);
			if (end == -1)
				return *this;
			else
				return substr(0, end + 1);
		}

	public:
		class _CaseInsensitiveLess : public std::binary_function<_Base, _Base, bool>
		{
		public:
			bool operator()(const _TempStringImplT &_Left, const _TempStringImplT &_Right) const
			{
				return _Left.icompare(_Right) < 0;
			}
		};
	};

	template <class _CharType, class _Traits = std::char_traits<_CharType> > class _ConstStringT : public _TempStringImplT<_CharType, _Traits>
	{
	private:
		typedef _TempStringImplT<_CharType, _Traits> _Base;

	public:
		_ConstStringT(const _CharType *p)
			: _Base(const_cast<_CharType *>(p), _Traits::length(p))
		{
		}

		_ConstStringT(const _CharType *p, size_t len)
			: _Base(const_cast<_CharType *>(p), (len == -1) ? _Traits::length(p) : len)
		{
		}

		_ConstStringT(const _ConstStringT &str)
			: _Base(str.GetBuffer(), str.GetLength())
		{
		}

		//TempString does not have a c_str() method, as it may not have a terminating NULL
		const _CharType *c_str() const
		{
			return _Base::GetBuffer();
		}
	};

	//! Similar to _ConstStringT, however, is used only to pass an existing temporary LPxSTR pointer as a TempString &
	template <class _CharType, class _Traits = std::char_traits<_CharType> > class _TempStrPointerWrapperT : public _TempStringImplT<_CharType, _Traits>
	{
	private:
		typedef _TempStringImplT<_CharType, _Traits> _Base;

	public:
		_TempStrPointerWrapperT(const _CharType *p)
			: _Base(const_cast<_CharType *>(p), _Traits::length(p))
		{
		}

		_TempStrPointerWrapperT(const _CharType *p, size_t len)
			: _Base(const_cast<_CharType *>(p), (len == -1) ? _Traits::length(p) : len)
		{
		}

		//TempString does not have a c_str() method, as it may not have a terminating NULL
		const _CharType *c_str() const
		{
			return _Base::GetBuffer();
		}

		template <size_t _Size> static _TempStrPointerWrapperT FromPaddedString(const _CharType (&str)[_Size], _CharType paddingChar = '\0')
		{
			return FromPaddedString(str, _Size, paddingChar);
		}

		static _TempStrPointerWrapperT FromPaddedString(const _CharType *pStr, size_t len, _CharType paddingChar = '\0')
		{
			for (size_t i = 0; i < len; i++)
				if (!pStr[i] || (pStr[i] == paddingChar))
					return _TempStrPointerWrapperT(pStr, i);
			return _TempStrPointerWrapperT(pStr, len);
		}
	};

	class HeapAllocator
	{
	public:
		static void *alloc(size_t size) {return ::malloc((unsigned)size);}
		static void free(void *p) {return ::free(p);}
		static void *realloc(void *p, size_t newSize, size_t prevSize) 
		{
#if defined (BZSLIB_WINKERNEL) || defined (BZSLIB_KEXT)
			return ::_realloc(p, newSize, prevSize);
#else
			return ::realloc(p, newSize);
#endif
		}
	};

	template <typename _CharType, class _Traits = std::char_traits<_CharType>, size_t _EmbeddedArraySize = 16, class _Allocator = HeapAllocator> class _DynamicStringBaseT : public _TempStringImplT<_CharType, _Traits>
	{
	private:
		typedef _TempStringImplT<_CharType, _Traits> _TempStrType, _Base;
		typedef _DynamicStringBaseT _Myt;

	private:
		//! Contains number of characters (not bytes) allocated, including space for NULL
		size_t m_Allocated;	
		_CharType m_EmbeddedArray[_EmbeddedArraySize];

	private:
		bool IsUsingEmbeddedArray()
		{
			if (_EmbeddedArraySize == 0)
				return false;
			return (_Base::GetBuffer() == m_EmbeddedArray);
		}

	protected:
		bool IsMyPtr(const _CharType *p)
		{
			if (p < _Base::GetBuffer())
				return false;
			if (p >= (_Base::GetBuffer() + GetAllocated()))
				return false;
			return true;
		}

	protected:
		_DynamicStringBaseT(size_t AllocationSizeNotIncludingNull)
			: _Base((AllocationSizeNotIncludingNull >= _EmbeddedArraySize) ? ((_CharType *)_Allocator::alloc(sizeof(_CharType) * (AllocationSizeNotIncludingNull + 1))) : m_EmbeddedArray, 0)
			, m_Allocated(AllocationSizeNotIncludingNull + 1)
		{
			_Base::GetBuffer()[0] = 0;
		}

		~_DynamicStringBaseT()
		{
#ifdef BZSLIB_WINKERNEL
			C_ASSERT(sizeof(m_EmbeddedArray) >= sizeof(typename NTStringHelper<_CharType>::_Type));
#endif

			if (_Base::GetBuffer() && (_Base::GetBuffer() != m_EmbeddedArray))
				_Allocator::free(_Base::GetBuffer());
		}

	private:
		void SetBuffer(_CharType *pBuf) {return _Base::SetBuffer(pBuf);}	//Buffer management is done inside this class. Use PreAllocate() from child classes.

		void SetAllocated(size_t newAllocated)
		{
			m_Allocated = newAllocated;
		}

	public:
		//! Returns number of characters (not bytes) allocated, including space for NULL
		size_t GetAllocated() const
		{
			return m_Allocated;
		}

		_CharType *PreAllocate(size_t CountNotIncludingNull, bool KeepOldData)
		{
			if (GetAllocated() > CountNotIncludingNull)
				return _Base::GetBuffer();
			if (CountNotIncludingNull < _EmbeddedArraySize)
			{
				SetAllocated(_EmbeddedArraySize);
				return _Base::GetBuffer();
			}
			if (KeepOldData)
			{
				if (IsUsingEmbeddedArray())
				{
					SetBuffer((_CharType *)_Allocator::alloc(sizeof(_CharType) * (CountNotIncludingNull + 1)));
					memcpy(_Base::GetBuffer(), m_EmbeddedArray, GetAllocated() * sizeof(_CharType));
				}
				else
				{
					SetBuffer((_CharType *)_Allocator::realloc(_Base::GetBuffer(),
						sizeof(_CharType) * (CountNotIncludingNull + 1),
						GetAllocated() * sizeof(_CharType)));
				}
			}
			else
			{
				if (_Base::GetBuffer() && !IsUsingEmbeddedArray())
					_Allocator::free(_Base::GetBuffer());
				SetBuffer((_CharType *)_Allocator::alloc(sizeof(_CharType) * (CountNotIncludingNull + 1)));
				SetLength(0);
			}
			SetAllocated(CountNotIncludingNull + 1);
			if (_Base::GetLength() >= CountNotIncludingNull)
				SetLength(CountNotIncludingNull - 1);
			return _Base::GetBuffer();
		}

		void SetLength(size_t CharsNotIncludingNull)
		{
			if (CharsNotIncludingNull >= GetAllocated())
				CharsNotIncludingNull = GetAllocated() - 1;
			if (!GetAllocated())
				CharsNotIncludingNull = 0;
			else
				_Base::GetBuffer()[CharsNotIncludingNull] = 0;
			_Base::SetLength(CharsNotIncludingNull);
		}

		bool IsMySubstring(const _TempStrType &str)
		{
			return IsMySubstring(str.GetBuffer());
		}

#ifdef BZSLIB_WINKERNEL
	public:
		//! Returns a writable NT-style string (UNICODE_STRING/ANSI_STRING)
		/*! This method returns a writable NT string (UNICODE_STRING/ANSI_STRING) structure representing
			the string. 
			\attention The pointer returned by ToNTString() method is valid until any other method of DynamicString
					   is called. <b>Note that if you modify the string returned by ToNTString() call, you need
					   to call the UpdateLengthFromNTString() method immediately after the modification.</b>
		    \remarks The returned NT string structure is stored in the "embedded buffer" attached to every string,
					  normally used for storing short strings.
						
		*/
		typename NTStringHelper<_CharType>::_Type *ToNTString(size_t BytesToAllocate = 0)
		{
			//Ensure that the embedded array is no longer used
			PreAllocate((BytesToAllocate <= __countof(m_EmbeddedArray)) ? (__countof(m_EmbeddedArray) + 1) : BytesToAllocate, true);
			ASSERT(!IsUsingEmbeddedArray());
			typename NTStringHelper<_CharType>::_Type *pNTString = (typename NTStringHelper<_CharType>::_Type *)m_EmbeddedArray;
			pNTString->Buffer = _Base::GetBuffer();
			pNTString->Length = (USHORT)(_Base::GetLength() * sizeof(_CharType));
			pNTString->MaximumLength = (USHORT)(GetAllocated() * sizeof(_CharType));
			return pNTString;
		}

		//! Updates string length after modification of a structure returned by a preceding ToNTString() call.
		void UpdateLengthFromNTString()
		{
			//A call to ToNTString() will ensure that the embedded array is no longer used.
			ASSERT(!IsUsingEmbeddedArray());
			typename NTStringHelper<_CharType>::_Type *pNTString = (typename NTStringHelper<_CharType>::_Type *)m_EmbeddedArray;
			size_t newLen = pNTString->Length / sizeof(_CharType);
			if (newLen >= GetAllocated())
				newLen = GetAllocated() - 1;
			SetLength(newLen);
		}

		PWCHAR ExportToPagedPool(unsigned ExtraTrailingZeroes = 0)
		{
			unsigned bsize = ((unsigned)size() + 1 + ExtraTrailingZeroes) * sizeof(wchar_t);
			PWCHAR pR = (PWCHAR)ExAllocatePool(PagedPool, bsize);
			if (!pR)
				return NULL;
			memcpy(pR, _Base::GetBuffer(), bsize - (ExtraTrailingZeroes * sizeof(wchar_t)));
			if (ExtraTrailingZeroes)
				memset(pR + size() + 1, 0, ExtraTrailingZeroes * sizeof(wchar_t));
			return pR;
		}

#endif
	};

	static inline size_t vsnprintf_helper(char *pBuffer, size_t MaxSize, const char *pFormat, va_list args)
	{
#ifdef BZSLIB_UNIXWORLD
		return vsnprintf(pBuffer, MaxSize, pFormat, args);
#else
//#if !defined(_DDK) && !defined(UNDER_CE)	//_vsnprintf_s() does not support NULL pBuffer just to assess the size
#if 0
		return _vsnprintf_s(pBuffer, MaxSize * sizeof(char), MaxSize, pFormat, args);
#else
#pragma warning(push)
#pragma warning(disable:4996)
		return _vsnprintf(pBuffer, MaxSize, pFormat, args);
#pragma warning(pop)
#endif
#endif
	}

#ifndef BZSLIB_KEXT
	static inline size_t vsnprintf_helper(wchar_t *pBuffer, size_t MaxSize, const wchar_t *pFormat, va_list args)
	{
#ifdef BZS_POSIX
		return vswprintf(pBuffer, MaxSize, pFormat, args);
#else
//#if !defined(_DDK) && !defined(UNDER_CE)
#if 0
		return _vsnwprintf_s(pBuffer, MaxSize * sizeof(wchar_t), MaxSize, pFormat, args);
#else
#pragma warning(push)
#pragma warning(disable:4996)
		return _vsnwprintf(pBuffer, MaxSize, pFormat, args);
#pragma warning(pop)
#endif
#endif
	}
#endif

	template <typename _CharType, class _Traits = std::char_traits<_CharType>, size_t _EmbeddedArraySize = 16, class _Allocator = HeapAllocator> class _DynamicStringT : public _DynamicStringBaseT<_CharType, _Traits, _EmbeddedArraySize, _Allocator>
	{
	private:
		typedef _TempStringImplT<_CharType, _Traits> _TempStrType;
		typedef _DynamicStringT _Myt;
		typedef _DynamicStringBaseT<_CharType, _Traits, _EmbeddedArraySize, _Allocator> _Base;

	protected:
		_CharType *GetBuffer() const {return _Base::GetBuffer();}

	public:
		_DynamicStringT()
			: _Base(0)
		{
		}


		_DynamicStringT(const _CharType *ptr, size_t len = -1)
			: _Base((len != -1) ? len : (_Traits::length(ptr)))
		{
			size_t length = (len != -1) ? len : (_Traits::length(ptr));

			memcpy(_Base::GetBuffer(), ptr, length * sizeof(_CharType));
			_Base::SetLength(length);
		}

		_DynamicStringT(const _TempStrType &str)
			: _Base(str.GetLength())
		{
			if (str.GetLength())
			{
				memcpy(GetBuffer(), str.GetBuffer(), str.GetLength() * sizeof(_CharType));
				_Base::SetLength(str.GetLength());
			}
		}

		_DynamicStringT(const _DynamicStringT &str)
			: _Base(str.GetLength())
		{
			if (str.GetLength())
			{
				memcpy(GetBuffer(), str.GetBuffer(), str.GetLength() * sizeof(_CharType));
				_Base::SetLength(str.GetLength());
			}
		}

	public:
		typedef typename _Base::size_type size_type;
		typedef typename _Base::_Elem _Elem;
		enum {npos = _Base::npos};

		_CharType &operator[](size_t idx)
		{
			if (idx >= _Base::GetLength())
				return *((_CharType *)NULL);
			return GetBuffer()[idx];
		}

		const _CharType &operator[](size_t idx) const
		{
			if (idx >= _Base::GetLength())
				return *((_CharType *)NULL);
			return GetBuffer()[idx];
		}

#pragma region assign() methods and assignment operators
	public:
		_Myt& assign(const _TempStrType& _Right)
		{
			return (assign(_Right, 0, npos));
		}

		_Myt& assign(const _TempStrType& _Right, size_type _Roff, size_type _Count)
		{
			if (_Roff > _Right.GetLength())
				_Roff = _Right.GetLength();
			if ((_Roff + _Count) >= _Right.GetLength())
				_Count = _Right.GetLength() - _Roff;

			if (this == &_Right)
			{
				memmove(GetBuffer(), GetBuffer() + _Roff, _Count * sizeof(_CharType));
			}
			else
			{
				_Base::PreAllocate(_Count, false);
				memcpy(GetBuffer(), _Right.GetBuffer() + _Roff, _Count * sizeof(_CharType));
			}
			_Base::SetLength(_Count);
			return (*this);
		}

		_Myt& assign(const _Elem *_Ptr, size_type _Count)
		{
			if (_DynamicStringT<_CharType>::IsMyPtr(_Ptr))
				return (assign(*this, _Ptr - GetBuffer(), _Count));	// substring

			_DynamicStringT<_CharType>::PreAllocate(_Count, false);
			memcpy(GetBuffer(), _Ptr, _Count * sizeof(_CharType));
			_DynamicStringT<_CharType>::SetLength(_Count);
			return (*this);
		}

		_Myt& assign(const _Elem *_Ptr)
		{
			return (assign(_Ptr, _Traits::length(_Ptr)));
		}

		_Myt& assign(size_type _Count, _Elem _Ch)
		{
			PreAllocate(_Count, false);
			for (size_t i = 0; i < _Count; i++)
				GetBuffer()[i] = _Ch;
			SetLength(_Count);
			return (*this);
		}

#ifdef BZSLIB_WINKERNEL
		void operator=(const typename NTStringHelper<_CharType>::_Type &string)
		{
			if (!string.Buffer)
				SetLength(0);
			assign(string.Buffer, string.Length / sizeof(_CharType));
		}
#endif


		_DynamicStringT &operator=(const _TempStrType &str)
		{
			assign(str);
			return *this;
		}

		_DynamicStringT &operator=(const _DynamicStringT &str)
		{
			assign(str);
			return *this;
		}

		_DynamicStringT &operator=(const _CharType *ptr)
		{
			assign(ptr);
			return *this;
		}

		_DynamicStringT &operator=(_CharType ch)
		{
			assign(&ch, 1);
			return *this;
		}

#ifdef BZSLIB_WINKERNEL
		_DynamicStringT &operator=(const typename NTStringHelper<_CharType>::_Type *pStr)
		{
			assign(pStr->Buffer, pStr->Length / sizeof(_CharType));
			return *this;
		}
#endif

#pragma endregion
#pragma region append() methods and appending operators
		_Myt& append(const _TempStrType& _Right)
		{
			return (append(_Right, 0, npos));
		}

		_Myt& append(const _TempStrType& _Right, size_t _Roff, size_t _Count)
		{
			if (_Roff > _Right.GetLength())
				_Roff = _Right.GetLength();
			if ((_Roff + _Count) >= _Right.GetLength())
				_Count = _Right.GetLength() - _Roff;
			return append(_Right.GetBuffer() + _Roff, _Count);
		}

		_Myt& append(const _Elem *_Ptr, size_t _Count)
		{
			_Base::PreAllocate(_Base::GetLength() + _Count, true);
			memcpy(GetBuffer() + _Base::GetLength(), _Ptr, _Count * sizeof(_CharType));
			_Base::SetLength(_Base::GetLength() + _Count);
			return *this;
		}

		_Myt& append(const _Elem *_Ptr)
		{
			return (append(_Ptr, _Traits::length(_Ptr)));
		}

		_Myt& append(size_type _Count, _Elem _Ch)
		{
			PreAllocate(_Base::GetLength() + _Count, true);
			for (size_t i = 0; i < _Count; i++)
				GetBuffer()[_Base::GetLength() + i] = _Ch;
			SetLength(_Base::GetLength() + _Count);
			return *this;
		}

		_Myt& operator+=(const _TempStrType& _Right)
		{
			return append(_Right);
		}

		_Myt& operator+=(const _Myt& _Right)
		{
			return append(_Right);
		}

		_Myt& operator+=(const _Elem *_Ptr)
		{
			return append(_Ptr);
		}

		_Myt& operator+=(_Elem _Ch)
		{
			return append(&_Ch, 1);
		}

#pragma endregion
#pragma region insert() methods
		_Myt& insert(size_type _Off, const _Myt& _Right)
		{
			return (insert(_Off, _Right, 0, npos));
		}

		_Myt& insert(size_type _Off, const _Myt& _Right, size_type _Roff, size_type _Count)
		{
			if (_Roff > _Right.GetLength())
				_Roff = _Right.GetLength();
			if ((_Roff + _Count) >= _Right.GetLength())
				_Count = _Right.GetLength() - _Roff;
			return insert(_Off, _Right.GetBuffer() + _Roff, _Count);
		}

		_Myt& insert(size_type _Off, const _Elem *_Ptr, size_type _Count)
		{
			if (_Off >= _Base::GetLength())
				_Off = _Base::GetLength() - 1;
			if (_Off == -1)
				_Off = 0;
			PreAllocate(_Base::GetLength() + _Count, true);
			size_t _ToMove = _Base::GetLength() - _Off;
			if (_ToMove)
				memmove(GetBuffer() + _Off + _Count, GetBuffer() + _Off, _ToMove * sizeof(_CharType));
			memcpy(GetBuffer() + _Off, _Ptr, _Count * sizeof(_CharType));
			SetLength(_Base::GetLength() + _Count);
			return *this;
		}

		_Myt& insert(size_type _Off, const _Elem *_Ptr)
		{
			return (insert(_Off, _Ptr, _Traits::length(_Ptr)));
		}

		_Myt& insert(size_type _Off, size_type _Count, _Elem _Ch)
		{
			if (_Off >= _Base::GetLength())
				_Off = _Base::GetLength() - 1;
			if (_Off == -1)
				_Off = 0;
			PreAllocate(_Base::GetLength() + _Count, true);
			size_t _ToMove = _Base::GetLength() - _Off;
			if (_ToMove)
				memmove(GetBuffer() + _Off + _Count, GetBuffer() + _Off, _ToMove * sizeof(_CharType));
			for (size_t i = 0; i < _Count; i++)
				GetBuffer()[_Off + i] = _Ch;
			SetLength(_Base::GetLength() + _Count);
			return *this;
		}
#pragma endregion
#pragma region replace() methods
		_Myt& replace(size_type _Off, size_type _N0, const _Myt& _Right)
		{
			return (replace(_Off, _N0, _Right, 0, npos));
		}

		_Myt& replace(size_type _Off, size_type _N0, const _Myt& _Right, size_type _Roff, size_type _Count)
		{
			if (_Roff > _Right.GetLength())
				_Roff = _Right.GetLength();
			if ((_Roff + _Count) >= _Right.GetLength())
				_Count = _Right.GetLength() - _Roff;
			return replace(_Off, _N0, _Right.GetBuffer() + _Roff, _Count);
		}

		_Myt& replace(size_type _Off, size_type _N0, const _Elem *_Ptr, size_type _Count)
		{
			if (_Off >= _Base::GetLength())
				_Off = _Base::GetLength() - 1;
			if (_Off == -1)
				_Off = 0;
			if (_Count > _N0)
			{
				size_t _Delta = _Count - _N0;
				_Base::PreAllocate(_Base::GetLength() + _Delta, true);
				size_t _ToMove = _Base::GetLength() - _Off;
				if (_ToMove)
					memmove(GetBuffer() + _Off + _Delta, GetBuffer() + _Off, _ToMove * sizeof(_CharType));
				_Base::SetLength(_Base::GetLength() + _Delta);
			}
			else
			{
				size_t _Delta = _N0 - _Count;
				size_t _ToMove = _Base::GetLength() - _Off;
				if (_ToMove && _Delta)
					memmove(GetBuffer() + _Off, GetBuffer() + _Off + _Delta, _ToMove * sizeof(_CharType));
				_Base::SetLength(_Base::GetLength() - _Delta);
			}
			memcpy(_Base::GetBuffer() + _Off, _Ptr, _Count * sizeof(_CharType));
			return *this;
		}

		_Myt& replace(size_type _Off, size_type _N0, const _Elem *_Ptr)
		{
			return (replace(_Off, _N0, _Ptr, _Traits::length(_Ptr)));
		}

		_Myt& replace(size_type _Off, size_type _N0, size_type _Count, _Elem _Ch)
		{
			if (_Off >= _Base::GetLength())
				_Off = _Base::GetLength() - 1;
			if (_Off == -1)
				_Off = 0;
			if (_Count > _N0)
			{
				size_t _Delta = _Count - _N0;
				PreAllocate(_Base::GetLength() + _Delta, true);
				size_t _ToMove = _Base::GetLength() - _Off;
				if (_ToMove)
					memmove(GetBuffer() + _Off + _Delta, GetBuffer() + _Off, _ToMove * sizeof(_CharType));
				SetLength(_Base::GetLength() + _Delta);
			}
			else
			{
				size_t _Delta = _N0 - _Count;
				size_t _ToMove = _Base::GetLength() - _Off;
				if (_ToMove && _Delta)
					memmove(GetBuffer() + _Off, GetBuffer() + _Off + _Delta, _ToMove * sizeof(_CharType));
				SetLength(_Base::GetLength() - _Delta);
			}
			for (size_t i = 0; i < _Count; i++)
				GetBuffer()[_Off + i] = _Ch;
			return *this;
		}

#pragma endregion
#pragma region Various STL-compatible methods
		size_t copy(_CharType *_Dest,
			size_t _Count, size_t _Off = 0) const
		{
			if (_Off >= _Base::GetLength())
				_Off = _Base::GetLength() - 1;
			if (_Off == -1)
				_Off = 0;
			size_t _Max = _Base::GetLength() - _Off;
			if (_Count > _Max)
				_Count = _Max;
			memcpy(_Dest, GetBuffer() + _Off, _Count * sizeof(_CharType));
			return _Count;
		}

		_CharType &at(size_t idx)
		{
			if (idx >= _Base::GetLength())
				return *((_CharType *)NULL);
			return GetBuffer()[idx];
		}

		size_t capacity() const
		{
			return _Base::GetAllocated();
		}

		void clear()
		{
			_Base::SetLength(0);
		}

		const _CharType *data() const
		{
			return c_str();
		}

		_Myt& erase(size_type _Off = 0, size_type _Count = npos)
		{
			if (_Off >= _Base::GetLength())
				return *this;
			size_t _Max = _Base::GetLength() - _Off;
			if (_Count > _Max)
				_Count = _Max;
			size_t _Remaining = _Base::GetLength() - (_Off + _Count);
			if (_Remaining)
				memmove(GetBuffer() + _Off, GetBuffer() + _Off + _Count, _Remaining * sizeof(_CharType));
			SetLength(_Off + _Remaining);
			return *this;
		}

		void push_back(_CharType ch)
		{
			append(&ch, 1);
		}

		void reserve(size_t _Count = 0)
		{
			_Base::PreAllocate(_Count, true);
		}

		void resize(size_type _Count, _Elem _Ch = 0)
		{
			if (_Count < _Base::GetLength())
				_Base::SetLength(_Count);
			else
				append(_Count - _Base::GetLength(), _Ch);
		}

		const _CharType *c_str() const
		{
			return GetBuffer() ? GetBuffer() : (_CharType *)L"\0\0\0\0";
		}

#pragma endregion

	public:

		//2 copies of va_list are needed, as MacOS does not support reusing va_lists
		size_t vFormat(const _CharType *Format, va_list lst_copy1, va_list lst_copy2)
		{
			size_t fsize = vsnprintf_helper(NULL, 0, Format, lst_copy1);
			_Base::PreAllocate(fsize, false);
			size_t fsize_actual = vsnprintf_helper(GetBuffer(), fsize + 1, Format, lst_copy2);
			ASSERT(fsize_actual == fsize);
			if (fsize_actual == -1)
				fsize_actual = 0;
			_Base::SetLength(fsize_actual);
			return fsize_actual;
		}

		size_t Format(const _CharType *Format, ...)
		{
			va_list lst, lstcopy;
			va_start(lst, Format);
			va_start(lstcopy, Format);
			size_t done = vFormat(Format, lst, lstcopy);
			va_end(lst);
			va_end(lstcopy);
			return done;
		}

		size_t AppendFormat(const _CharType *Format, ...)
		{
			va_list lst;
			va_start(lst, Format);
			size_t fsize = vsnprintf_helper(NULL, 0, Format, lst);
			va_end(lst);
			va_start(lst, Format);
			PreAllocate(_Base::GetLength() + fsize, true);
			fsize = vsnprintf_helper(GetBuffer() + _Base::GetLength(), fsize + 1, Format, lst);
			if (fsize == -1)
				fsize = 0;
			SetLength(_Base::GetLength() + fsize);
			va_end(lst);
			return fsize;
		}

		static _DynamicStringT sFormat(const _CharType *Format, ...)
		{
			_DynamicStringT ret;
			va_list va_lst1, va_lst2;
			va_start(va_lst1, Format);
			va_start(va_lst2, Format);
			ret.vFormat(Format, va_lst1, va_lst2);
			va_end(va_lst1);
			va_end(va_lst2);
			return ret;
		}

	};

	typedef _TempStringImplT<char> TempStringA;
	typedef _ConstStringT<char>		ConstStringA;
	typedef _DynamicStringT<char>	DynamicStringA;
	typedef _TempStrPointerWrapperT<char> TempStrPointerWrapperA;

#ifdef BZSLIB_WINKERNEL
	typedef _TempStringImplT<wchar_t>	TempString, TempStringW;
	typedef _ConstStringT<wchar_t>		ConstString, ConstStringW;
	typedef _DynamicStringT<wchar_t>	DynamicString, DynamicStringW;
	typedef _TempStrPointerWrapperT<wchar_t> TempStrPointerWrapper, TempStrPointerWrapperW;
#else
	typedef _TempStringImplT<TCHAR> TempString;
	typedef _ConstStringT<TCHAR>	ConstString;
	typedef _DynamicStringT<TCHAR>	DynamicString;
	typedef _TempStrPointerWrapperT<TCHAR> TempStrPointerWrapper;

	typedef _TempStringImplT<wchar_t>	TempStringW;
	typedef _ConstStringT<wchar_t>		ConstStringW;
	typedef _DynamicStringT<wchar_t>	DynamicStringW;
	typedef _TempStrPointerWrapperT<wchar_t> TempStrPointerWrapperW;
#endif

	template <class _StringT> class _CaseInsensitiveLess : public std::binary_function<_StringT, _StringT, bool>
	{
	public:
		bool operator()(const _StringT &_Left, const _StringT &_Right) const
		{
			return (case_insensitive_char_traits<typename _StringT::_Elem>::compare(_Left.c_str(), _Right.c_str(), max(_Left.length(), _Right.length())) < 0);
		}
	};

}

#include "strop.h"