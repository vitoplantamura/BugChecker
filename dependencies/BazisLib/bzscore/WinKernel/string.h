#pragma once

#include <Bazislib/bzscore/string.h>

namespace BazisLib
{
	namespace DDK
	{
		class StaticString
		{
		private:
			UNICODE_STRING m_unicodeStr;

		private:
			void Initialize(const wchar_t *pwsz, unsigned allocLength = 0)
			{
				if (!pwsz)
					m_unicodeStr.Length = 0;
				else
					m_unicodeStr.Length = (USHORT)(wcslen(pwsz) * sizeof(wchar_t));
	
				m_unicodeStr.MaximumLength =(USHORT)(allocLength ? (allocLength * sizeof(wchar_t)) : (m_unicodeStr.Length + sizeof(wchar_t)));
				m_unicodeStr.Buffer = new wchar_t[m_unicodeStr.MaximumLength];
				if (m_unicodeStr.Length)
					memcpy(m_unicodeStr.Buffer, pwsz, (m_unicodeStr.Length + 1) * sizeof(wchar_t));
			}

		public:
			StaticString(const wchar_t *pwsz, unsigned allocLength = 0)
			{
				Initialize(pwsz, allocLength);
			}

			StaticString(const String &str)
			{
				Initialize(str.c_str());
			}

			~StaticString()
			{
				if (m_unicodeStr.Buffer)
					delete m_unicodeStr.Buffer;
			}

			operator PUNICODE_STRING()
			{
				return &m_unicodeStr;
			}
		};
	}
}