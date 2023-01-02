#pragma once
#include "cmndef.h"
#include "../../bzscore/string.h"
#include "../../bzscore/assert.h"
#include "../algorithms/memalgs.h"
#include "../../bzscore/status.h"
#include <Rpc.h>

#ifndef UNDER_CE

#ifdef UNICODE
typedef RPC_WSTR RPC_TSTR;
#else
typedef RPC_CSTR RPC_TSTR;
#endif


namespace BazisLib
{
	namespace Win32
	{
		class Guid
		{
		private:
			GUID m_Guid;
			enum {StringFormLength = 36};

		public:
			Guid()
			{
				memset(&m_Guid, 0, sizeof(m_Guid));
			}

			Guid(const GUID &guid)
			{
				m_Guid = guid;
			}

			Guid(const String &str, ActionStatus *pStatus = NULL)
			{
				if (str.length() < StringFormLength)
				{
					memset(&m_Guid, 0, sizeof(m_Guid));
					ASSIGN_STATUS(pStatus, InvalidParameter);
					return;
				}
				if (str[0] == '{')
				{
					if (UuidFromString((RPC_TSTR)String(str.substr(1,StringFormLength)).c_str(), (UUID *)&m_Guid) != RPC_S_OK)
					{
						memset(&m_Guid, 0, sizeof(m_Guid));
						ASSIGN_STATUS(pStatus, InvalidParameter);
						return;
					}
				}
				else
				{
					if (UuidFromString((RPC_TSTR)str.c_str(), (UUID *)&m_Guid) != RPC_S_OK)
					{
						memset(&m_Guid, 0, sizeof(m_Guid));
						ASSIGN_STATUS(pStatus, InvalidParameter);
						return;
					}
				}
				ASSIGN_STATUS(pStatus, Success);
			}

			bool Valid() const
			{
				for (size_t i = 0; i < sizeof(m_Guid); i++)
					if (((char *)&m_Guid)[i])
						return true;
				return false;
			}

			static Guid Generate()
			{
				GUID guid;
				UuidCreate((UUID *)&guid);
				return guid;
			}

			operator GUID &()
			{
				return m_Guid;
			}

			String ToString(bool IncludeCurlyBrackets = false) const
			{
				RPC_TSTR pGuid = NULL;
				if ((UuidToString((UUID *)&m_Guid, &pGuid) != RPC_S_OK) || !pGuid)
					return _T("");
				ASSERT(wcslen((wchar_t *)pGuid) == StringFormLength);
				if (IncludeCurlyBrackets)
				{
					TCHAR tszGuid[40];
					_sntprintf(tszGuid, __countof(tszGuid), _T("{%s}"), (TCHAR*)pGuid);
					RpcStringFree(&pGuid);
					return tszGuid;
				}
				else
				{
					String ret = (TCHAR *)pGuid;
					RpcStringFree(&pGuid);
					return ret;
				}
			}
		};
	}
}
#endif