#pragma once
#include "cmndef.h"

#ifdef BZSLIB_POSIX
#include <string.h>
#endif

#ifdef BZSLIB_KEXT
static size_t wcslen(const wchar_t *pstr)
{
	for (size_t i = 0; ; i++)
		if (!pstr[i])
			return i;
}
#endif

namespace BazisLib
{
	//! Allows splitting strings into substring chains without any buffer copying
	/*! This class is used by the _TempStringImplT::Split() methods. It allows splitting the string into substrings using
		provided tokens with minimalistic MS C++ syntax and no buffer copying overhead. Here is a usage example:
<pre>
ConstStringA xz = "123  456 \t7890 \t\t\t \t\t   \tabcd efg";
for each(TempStringA str in xz.Split(" \t"))
	fwrite(str.GetConstBuffer(), str.length(), 1, stdout), printf(",");
</pre>
		This example will produce the following output: 
<pre>123,456,7890,abcd,efg,</pre>
		You can also use the SplitByMarker() method to split the string using exact sequence, not any sequence from the
		set of characters. Basically, Split() corresponds to find_first_of() and SplitByMarker() - to find().
	*/

	//! Finds exact 
	template <class _StringClass, typename _SplitSequenceType> class ExactMatchSplitPolicy
	{
	private:
		static size_t SplitterLength(char Splitter) { return 1;}
		static size_t SplitterLength(wchar_t Splitter) { return 1;}
		static size_t SplitterLength(const char *Splitter) { return strlen(Splitter);}
		static size_t SplitterLength(const wchar_t *Splitter) { return wcslen(Splitter);}
		template<class _StringType> static size_t SplitterLength(const _StringType &Splitter) { return Splitter.length();}

	public:
		static off_t FindNextSpacing(const _StringClass &Str, _SplitSequenceType Sequence, off_t PrevOffset)
		{
			return Str.find(Sequence, PrevOffset);
		}

		static off_t FindSpacingEnd(const _StringClass &Str, _SplitSequenceType Sequence, off_t SpacingStart)
		{
			return SpacingStart + SplitterLength(Sequence);
		}

	};

	template <class _StringClass, typename _SplitSequenceType> class SetOfCharactersSplitPolicy
	{
	public:
		static off_t FindNextSpacing(const _StringClass &Str, _SplitSequenceType Sequence, off_t PrevOffset)
		{
			return Str.find_first_of(Sequence, PrevOffset);
		}

		static off_t FindSpacingEnd(const _StringClass &Str, _SplitSequenceType Sequence, off_t SpacingStart)
		{
			return Str.find_first_not_of(Sequence, SpacingStart);
		}

	};

	template <class _StringType, class _SplitSequenceType, class _SplitPolicy> class _StringSplitterT
	{
	private:
		_SplitSequenceType m_Sequence;
		const _StringType &m_StringRef;

	private:

	public:
		class iterator
		{
		private:
			const _StringSplitterT &m_SplitterRef;
			off_t m_ThisOffset, m_Len;

		public:
			iterator (const _StringSplitterT &splitter, off_t offset = 0)
				: m_SplitterRef(splitter)
				, m_ThisOffset(offset)
			{
				off_t nextOff = _SplitPolicy::FindNextSpacing(m_SplitterRef.m_StringRef, m_SplitterRef.m_Sequence, m_ThisOffset);
				if (nextOff == -1)
					m_Len = -1;
				else
					m_Len = nextOff - m_ThisOffset;

				if (!m_Len)
					operator++();
			}

			_StringType operator*()
			{
				return m_SplitterRef.m_StringRef.substr(m_ThisOffset, m_Len);
			}

			void operator++()
			{
				if (m_Len == -1)
					m_ThisOffset = -1;
				if (m_ThisOffset == -1)
					return;

				m_ThisOffset = _SplitPolicy::FindSpacingEnd(m_SplitterRef.m_StringRef, m_SplitterRef.m_Sequence, m_ThisOffset + m_Len);
				off_t nextOff = _SplitPolicy::FindNextSpacing(m_SplitterRef.m_StringRef, m_SplitterRef.m_Sequence, m_ThisOffset);

				if (nextOff == -1)
					m_Len = -1;
				else
					m_Len = nextOff - m_ThisOffset;
			}

			void operator++(int)
			{
				operator++();
			}

			bool operator!=(const iterator &it)
			{
				ASSERT(&it.m_SplitterRef == &m_SplitterRef);
				return (it.m_ThisOffset != m_ThisOffset);
			}
		};

	public:
		iterator begin()
		{
			return iterator(*this);
		}

		iterator end()
		{
			return iterator(*this, -1);
		}

	public:
		_StringSplitterT(const _StringType &str, _SplitSequenceType splitter)
			: m_StringRef(str)
			, m_Sequence(splitter)
		{
		}

	};

	template <size_t _MaxTokenCount, class _StringClass, class _SplitSequenceType, class _SplitPolicy> class _FixedSplitStringT
	{
	private:
		struct TokenBounds
		{
			off_t Start;
			size_t Len;
		} m_TokenBounds[_MaxTokenCount];
		size_t m_TokensValid;
		_StringClass m_OriginalString;

	public:
		_FixedSplitStringT(const _StringClass &Str, _SplitSequenceType splitter)
			: m_OriginalString(Str)
		{
			size_t off = 0;
			for (m_TokensValid = 0; m_TokensValid < _MaxTokenCount; m_TokensValid++)
			{
				off_t spc = _SplitPolicy::FindNextSpacing(m_OriginalString, splitter, off);
				if (spc == -1)
					spc = m_OriginalString.length();
				
				m_TokenBounds[m_TokensValid].Start = off;
				m_TokenBounds[m_TokensValid].Len = spc - off;

				off = _SplitPolicy::FindSpacingEnd(m_OriginalString, splitter, spc);
				if (off >= m_OriginalString.length())
				{
					m_TokensValid++;
					break;
				}
			}
		}

		_StringClass operator[](unsigned idx)
		{
			if (idx >= m_TokensValid)
				return m_OriginalString.substr(0,0);
			return m_OriginalString.substr(m_TokenBounds[idx].Start, m_TokenBounds[idx].Len);
		}

		size_t count()
		{
			return m_TokensValid;
		}

		_StringClass GetRemainingPart()
		{
			if (!m_TokensValid)
				return m_OriginalString;
			return m_OriginalString.substr(m_TokenBounds[m_TokensValid - 1].Start + m_TokenBounds[m_TokensValid - 1].Len);
		}

		size_t GetRemainderOffset()
		{
			if (!m_TokensValid)
				return 0;
			return m_TokenBounds[m_TokensValid - 1].Start + m_TokenBounds[m_TokensValid - 1].Len;
		}

		size_t GetTokenOffset(unsigned idx)
		{
			if (idx >= m_TokensValid)
				return -1;
			return m_TokenBounds[idx].Start;
		}

		size_t GetTokenLength(unsigned idx)
		{
			if (idx >= m_TokensValid)
				return -1;
			return m_TokenBounds[idx].Len;
		}
	};

	template <size_t _MaxTokenCount, class _StringClass> class _FixedCharacterSplitString : public _FixedSplitStringT<_MaxTokenCount, _StringClass, typename _StringClass::_Elem, ExactMatchSplitPolicy<_StringClass, typename _StringClass::_Elem> >
	{
	private:
		typedef _FixedSplitStringT<_MaxTokenCount, _StringClass, typename _StringClass::_Elem, ExactMatchSplitPolicy<_StringClass, typename _StringClass::_Elem> > _Base;

	public:
		_FixedCharacterSplitString(const _StringClass &Str, typename _StringClass::_Elem splitter)
			: _Base(Str, splitter)
		{
		}
	};

	template <size_t _MaxTokenCount, class _StringClass> class _FixedSetOfCharsSplitString : public _FixedSplitStringT<_MaxTokenCount, _StringClass, const typename _StringClass::_Elem *, SetOfCharactersSplitPolicy<_StringClass, const typename _StringClass::_Elem*> >
	{
	private:
		typedef _FixedSplitStringT<_MaxTokenCount, _StringClass, const typename _StringClass::_Elem *, SetOfCharactersSplitPolicy<_StringClass, const typename _StringClass::_Elem*> > _Base;

	public:
		_FixedSetOfCharsSplitString(const _StringClass &Str, const typename _StringClass::_Elem *splitter)
			: _Base(Str, splitter)
		{
		}
	};
}