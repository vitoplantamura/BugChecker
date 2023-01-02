#pragma once
#include "cmndef.h"
#include "../../bzscore/string.h"
#include "../../bzscore/assert.h"
#include "../../bzscore/algorithms/memalgs.h"
#include "../../bzscore/status.h"

namespace BazisLib
{
	namespace DDK
	{
		class Guid
		{
		private:
			GUID m_Guid;
			enum {StringFormLength = 36};

		private:
			static bool ScanGuid(const wchar_t *pStringForm, GUID *guid)
			{
			/*	return (swscanf(pStringForm, L"%8x-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x",
					&guid->Data1, &guid->Data2, &guid->Data3,
					&guid->Data4[0], &guid->Data4[1], &guid->Data4[2], &guid->Data4[3],
					&guid->Data4[4], &guid->Data4[5], &guid->Data4[6], &guid->Data4[7]) == 12);*/
			}

		public:
			Guid()
			{
				memset(&m_Guid, 0, sizeof(m_Guid));
			}

			Guid(const GUID &guid)
			{
				m_Guid = guid;
			}

			/*Guid(const String &str, ActionStatus *pStatus = NULL)
			{
				if (str.length() < StringFormLength)
				{
					memset(&m_Guid, 0, sizeof(m_Guid));
					ASSIGN_STATUS(pStatus, InvalidParameter);
					return;
				}
				if (str[0] == '{')
				{
					if (!ScanGuid(str.c_str(), &m_Guid))
					{
						memset(&m_Guid, 0, sizeof(m_Guid));
						ASSIGN_STATUS(pStatus, InvalidParameter);
						return;
					}
				}
				else
				{
					if (!ScanGuid(str.c_str(), &m_Guid))
					{
						memset(&m_Guid, 0, sizeof(m_Guid));
						ASSIGN_STATUS(pStatus, InvalidParameter);
						return;
					}
				}
				ASSIGN_STATUS(pStatus, Success);
			}*/

			bool Valid() const
			{
				for (size_t i = 0; i < sizeof(m_Guid); i++)
					if (((char *)&m_Guid)[i])
						return true;
				return false;
			}

			static Guid Generate()
			{
				UUID uuid;
				if (!NT_SUCCESS(ExUuidCreate(&uuid)))
					return Guid();
				return Guid(*((GUID *)&uuid));
			}

			operator GUID &()
			{
				return m_Guid;
			}

			String ToString(bool IncludeCurlyBrackets = false) const
			{
				wchar_t wsz[128];
				swprintf(wsz, IncludeCurlyBrackets ? L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}" : L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
					m_Guid.Data1, m_Guid.Data2, m_Guid.Data3,
					m_Guid.Data4[0], m_Guid.Data4[1], m_Guid.Data4[2], m_Guid.Data4[3],
					m_Guid.Data4[4], m_Guid.Data4[5], m_Guid.Data4[6], m_Guid.Data4[7]);
				return wsz;
			}
		};
	}
}
