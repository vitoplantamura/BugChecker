#pragma once
#include "cmndef.h"

namespace BazisLib
{
	namespace FastStringRoutines
	{
		template <class _CharType> static inline bool IsSeparatorChar(_CharType ch)
		{
			return (ch == ' ') || (ch == '\t');
		}

		template <class _CharType, bool (*_IsSeparatorChar)(_CharType), bool _TreatSingleQuotesAsDoubleQuotes> inline size_t FindNextTokenStartAndEnd(const _CharType *pszStr, size_t Length, size_t *pLength, _CharType CommentChar, bool *pCommentCharFound = NULL)
		{
			ASSERT(pszStr && pLength);

			size_t TokenStart = -1;
			bool bInsideQuotedBlock = false;
			size_t BackslashCnt = 0, i;

			size_t LastQuoteSymbolOffset = -1;

			if (pCommentCharFound)
				*pCommentCharFound = false;

			for (i = 0; i < Length; i++)
			{
				//If a quote symbol follows even number of backslashes, it is not skipped, odd number
				//of backslashes means sequence terminating with '\"'/'\''. It is skipped.
				if (pszStr[i] == '\\')
					BackslashCnt++;
				else
				{
					if ((pszStr[i] == '\"')	|| (_TreatSingleQuotesAsDoubleQuotes && (pszStr[i] == '\'')))
						if (!(BackslashCnt % 2))
							bInsideQuotedBlock = !bInsideQuotedBlock, LastQuoteSymbolOffset = i;
					BackslashCnt = 0;
				}

				if (!bInsideQuotedBlock && (pszStr[i] == CommentChar))	
				{
					if (pCommentCharFound)
						*pCommentCharFound = true;
					break;
				}

				if (_IsSeparatorChar(pszStr[i]))
				{
					if (bInsideQuotedBlock)
						continue;
					if (TokenStart != -1)
						break;
				}
				else
				{
					if (TokenStart == -1)
						TokenStart = i;
				}
			}

			if (TokenStart == -1)
			{
				*pLength = 0;
				return -1;
			}

			if ((pszStr[TokenStart] == '\"') || (_TreatSingleQuotesAsDoubleQuotes && (pszStr[TokenStart] == '\'')))
				TokenStart++;

			if (i && (LastQuoteSymbolOffset == (i - 1)))
				i--;

			ASSERT(i >= TokenStart);

			*pLength = (i - TokenStart);

			if (i == TokenStart)
				return -1;
			
			return TokenStart;
		}

		template <class _CharType, bool _TreatSingleQuotesAsDoubleQuotes> static inline int ParseConfigLine(_CharType *pszStr,
			_CharType **ppBuffer,
			size_t MaxCnt,
			_CharType CommentChar)
		{
			if (!pszStr || !ppBuffer || !MaxCnt)
				return 0;
			
			size_t remainingLen = std::char_traits<_CharType>::length(pszStr);
			
			size_t TokenStart = 0, TokenLength;
			_CharType *pCurrent = pszStr;

			size_t done = 0;

			for (;;)
			{
				bool commentCharFound = false;
				TokenStart = FindNextTokenStartAndEnd<_CharType, &IsSeparatorChar<_CharType>, _TreatSingleQuotesAsDoubleQuotes>(pCurrent, remainingLen, &TokenLength, CommentChar, &commentCharFound);
				if (TokenStart == -1)
					return done;

				ppBuffer[done] = pCurrent + TokenStart;
				pCurrent[TokenStart + TokenLength] = 0;
				if (++done >= MaxCnt)
					return done;

				if (commentCharFound)
					return done;

				size_t increment = (TokenStart + TokenLength + 1);
				
				if (remainingLen <= increment)
					return done;

				pCurrent += increment;
				remainingLen -= increment;
			}
		}

		template <bool _TreatSingleQuotesAsDoubleQuotes, class _StringClass> static inline _StringClass GetNextToken(const _StringClass &Source, size_t *pContext, typename _StringClass::_Elem CommentChar = 0)
		{
			ASSERT(pContext);
			size_t off = *pContext, len = 0;

			if (off >= Source.length())
			{
				*pContext = -1;
				return Source.substr(0, 0);
			}

			bool commentCharUsed = false;

			size_t tokenStart = FindNextTokenStartAndEnd<typename _StringClass::_Elem, &IsSeparatorChar<typename _StringClass::_Elem>, _TreatSingleQuotesAsDoubleQuotes>(Source.GetConstBuffer() + off,
																								 Source.length() - off,
																								 &len,
																								 CommentChar,
																								 &commentCharUsed);
			if (commentCharUsed)
				*pContext = Source.length();
			else
				*pContext += (tokenStart + len + 1);

			return Source.substr(tokenStart + off, len);
		}

		template <size_t _MaxTokenCount, class _StringClass, bool _TreatSingleQuotesAsDoubleQuotes = false> class _SplitConfigLineT
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
			_SplitConfigLineT(const _StringClass &Str, typename _StringClass::_Elem CommentChar = 0)
				: m_OriginalString(Str)
			{
				size_t off = 0;
				bool commentCharUsed = false;
				for (m_TokensValid = 0; m_TokensValid < _MaxTokenCount; m_TokensValid++)
				{
					off_t tokenStart = FindNextTokenStartAndEnd<typename _StringClass::_Elem, &IsSeparatorChar<typename _StringClass::_Elem>, _TreatSingleQuotesAsDoubleQuotes>(Str.GetConstBuffer() + off,
						Str.length() - off,
						&m_TokenBounds[m_TokensValid].Len,
						CommentChar,
						&commentCharUsed);

					if (tokenStart == -1)
						break;

					m_TokenBounds[m_TokensValid].Start = off + tokenStart;
					
					off += (tokenStart + m_TokenBounds[m_TokensValid].Len + 1);

					if (commentCharUsed || (off >= Str.length()))
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

		};

		template <class _StringType, bool _TreatSingleQuotesAsDoubleQuotes = false> class _ConfigLineSplitter
		{
		private:
			const _StringType &m_StringRef;
			typename _StringType::_Elem m_CommentChar;

		public:
			class iterator
			{
			private:
				const _ConfigLineSplitter &m_SplitterRef;
				size_t m_Context;
				_StringType m_ThisSubstring;

			public:
				iterator (const _ConfigLineSplitter &splitter, off_t context = 0)
					: m_SplitterRef(splitter)
					, m_Context(context)
					, m_ThisSubstring(m_SplitterRef.m_StringRef.substr(0,0))
				{
					operator++();
				}

				_StringType operator*()
				{
					return m_ThisSubstring;
				}

				void operator++()
				{
					m_ThisSubstring = GetNextToken<_TreatSingleQuotesAsDoubleQuotes>(m_SplitterRef.m_StringRef, &m_Context, m_SplitterRef.m_CommentChar);
				}

				void operator++(int)
				{
					operator++();
				}

				bool operator!=(const iterator &it)
				{
					ASSERT(&it.m_SplitterRef == &m_SplitterRef);
					return (m_Context != it.m_Context);
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
			_ConfigLineSplitter(const _StringType &str, typename _StringType::_Elem CommentChar = 0)
				: m_StringRef(str)
				, m_CommentChar(CommentChar)
			{
			}

		};

	}
}
