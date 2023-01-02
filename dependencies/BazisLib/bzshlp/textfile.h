#pragma once
#include "cmndef.h"
#include "../bzscore/autofile.h"
#include "../bzscore/buffer.h"
#include "../bzscore/string.h"
#include "../bzscore/platform.h"

namespace BazisLib
{
	namespace _Core
	{
		template <class _CharType> static inline _CharType *_FindSubstring(_CharType *String, _CharType *Substring)
		{
			ASSERT(FALSE);
			return NULL;
		}

		template<> static inline char *_FindSubstring<char>(char *String, char *Substring)
		{
			return strstr(String, Substring);
		}

		template<> static inline wchar_t *_FindSubstring<wchar_t>(wchar_t *String, wchar_t *Substring)
		{
			return wcsstr(String, Substring);
		}

		template <class _CharType, unsigned _LineEnding, unsigned _LineEndingCharCount> class _TextFile : AUTO_OBJECT
		{
		private:
			TypedBuffer<_CharType> m_ZeroTerminatedBuffer;
			unsigned m_PendingLine;
			bool m_bEOF;

			enum {ReadChunkSize = 50};

			DECLARE_REFERENCE(BazisLib::AIFile, m_pFile);
	
		public:
			_TextFile(const ManagedPointer<BazisLib::AIFile> &pFile) :
				INIT_REFERENCE(m_pFile, pFile),
				m_PendingLine(0),
				m_bEOF(false)
			{
				if (m_pFile && (sizeof(_CharType) == sizeof(wchar_t)))
				{
					m_ZeroTerminatedBuffer.EnsureSize(sizeof(wchar_t) * 2);
					if (m_pFile->Read(m_ZeroTerminatedBuffer.GetData(), sizeof(wchar_t)) == sizeof(wchar_t))
					{
						if (*((wchar_t *)m_ZeroTerminatedBuffer.GetData()) != 0xFEFF)
							m_ZeroTerminatedBuffer.SetSize(sizeof(wchar_t));
					}
				}

				_CharType term = _CharType();
				m_ZeroTerminatedBuffer.AppendData(&term, sizeof(term));
			}

			bool IsEOF()
			{
				return m_bEOF && m_ZeroTerminatedBuffer.GetSize() <= sizeof(_CharType);
			}

			bool HasMoreData()
			{
				return !IsEOF();
			}

			bool Valid()
			{
				return m_pFile && m_pFile->Valid();
			}

			_DynamicStringT<_CharType> ReadLine()
			{
				if (!m_pFile)
					return _DynamicStringT<_CharType>();
				for (;;)
				{
					if (m_ZeroTerminatedBuffer.GetSize() >= sizeof(_CharType))
					{
						unsigned LineEnding[2] = { _LineEnding, 0 };
						_CharType *pEnd = _FindSubstring((_CharType *)m_ZeroTerminatedBuffer.GetData(), (_CharType *)LineEnding);
						if (pEnd >= m_ZeroTerminatedBuffer.GetDataAfterEndOfUsedSpace())
						{
							ASSERT(false);
							pEnd = NULL;
						}

						if (pEnd)
						{
							_DynamicStringT<_CharType> str((_CharType *)m_ZeroTerminatedBuffer.GetData(), pEnd - (_CharType *)m_ZeroTerminatedBuffer.GetData());
							pEnd += _LineEndingCharCount;
							size_t startOfNextLineInChars = (size_t)(pEnd - (_CharType *)m_ZeroTerminatedBuffer.GetData());
							size_t newSizeInChars = (m_ZeroTerminatedBuffer.GetSize() / sizeof(_CharType)) - startOfNextLineInChars;
							memmove(m_ZeroTerminatedBuffer.GetData(), pEnd, newSizeInChars * sizeof(_CharType));	//The moved area includes a null-terminating character
							if (newSizeInChars < 1)
								newSizeInChars = 1;

							m_ZeroTerminatedBuffer.SetSize(newSizeInChars * sizeof(_CharType));
							m_ZeroTerminatedBuffer[(unsigned int)newSizeInChars - 1] = 0;
							return str;
						}
					}

					m_ZeroTerminatedBuffer.EnsureSize(m_ZeroTerminatedBuffer.GetSize() + (ReadChunkSize + 1) * sizeof(_CharType));
					size_t done = m_pFile->Read(((char *)m_ZeroTerminatedBuffer.GetData()) + m_ZeroTerminatedBuffer.GetSize() - sizeof(_CharType), ReadChunkSize * sizeof(_CharType));
					if (!done)
					{
						m_bEOF = true;
						_DynamicStringT<_CharType> ret = ((_CharType *)m_ZeroTerminatedBuffer.GetData());
						m_ZeroTerminatedBuffer.SetSize(sizeof(_CharType));
						m_ZeroTerminatedBuffer[(size_t)0] = 0;
						return ret;
					}
					m_ZeroTerminatedBuffer.SetSize(m_ZeroTerminatedBuffer.GetSize() + done);
					m_ZeroTerminatedBuffer[(unsigned int)m_ZeroTerminatedBuffer.GetSize() / sizeof(_CharType) - 1] = 0;
				}
			}
		};
	}

	typedef _Core::_TextFile<char, LINE_ENDING_SEQUENCE_A, LINE_ENDING_SEQUENCE_LEN> TextANSIFileReader;
	typedef _Core::_TextFile<wchar_t, LINE_ENDING_SEQUENCE_W, LINE_ENDING_SEQUENCE_LEN> TextUnicodeFileReader;

	typedef _Core::_TextFile<char, '\n', 1> TextANSIUNIXFileReader;
	typedef _Core::_TextFile<wchar_t, '\n\0', 1> TextUnicodeUNIXFileReader;
}
