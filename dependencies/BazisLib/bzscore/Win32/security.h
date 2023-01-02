#pragma once

#include "security_common.h"

namespace BazisLib
{
	namespace Win32
	{
		namespace Security
		{
			//! Allows performing security operations on a named object
			/*! This class allows performing various security operations on a named object. Here is a typical usage example:
<pre>
NamedSecurityAccessor accessor(_T("SomeExistingFile.dat"));
accessor.TakeOwnership();
TranslatedAcl acl = accessor.GetDACL();
acl.AddAllowingAce(FILE_ALL_ACCESS, Sid::CurrentUserSid());
accessor.SetDACL(acl);
</pre>
			*/
			class NamedSecurityAccessor
			{
			private:
				String m_ObjectName;
				SE_OBJECT_TYPE m_ObjectType;

			public:
				NamedSecurityAccessor(LPCTSTR lpFileName, SE_OBJECT_TYPE ObjType = SE_FILE_OBJECT)
					: m_ObjectName(lpFileName)
					, m_ObjectType(ObjType)
				{
				}

				NamedSecurityAccessor(const TempString &FileName, SE_OBJECT_TYPE ObjType = SE_FILE_OBJECT)
					: m_ObjectName(FileName)
					, m_ObjectType(ObjType)
				{
				}

			public:
				//! Static method returning an owener SID of a given object
				static Sid GetObjectOwner(LPCTSTR lpObjectName, SE_OBJECT_TYPE objectType = SE_FILE_OBJECT)
				{
					Security::Sid sid;
					PSID pOwner = NULL;
					PSECURITY_DESCRIPTOR pDesc = NULL;
					::GetNamedSecurityInfo((LPTSTR)lpObjectName, objectType, OWNER_SECURITY_INFORMATION, &pOwner, NULL, NULL, NULL, &pDesc);
					if (!pDesc)
						return sid;
					sid = pOwner;
					LocalFree(pDesc);
					return sid;
				}

				//! Static method setting an owner of a given object
				static ActionStatus SetObjectOwner(LPCTSTR lpObjectName, const Sid &NewOwner, SE_OBJECT_TYPE objectType = SE_FILE_OBJECT)
				{
					if (!::SetNamedSecurityInfo((LPTSTR)lpObjectName, objectType, OWNER_SECURITY_INFORMATION, NewOwner, NULL, NULL, NULL))
						return MAKE_STATUS(Success);
					else
						return MAKE_STATUS(ActionStatus::FailFromLastError());
				}

				//! Static method returning a DACL of a given object
				static TranslatedAcl GetObjectDACL(LPCTSTR lpObjectName, SE_OBJECT_TYPE objectType = SE_FILE_OBJECT)
				{
					Security::TranslatedAcl acl;
					PACL pAcl = NULL;
					PSECURITY_DESCRIPTOR pDesc = NULL;
					::GetNamedSecurityInfo((LPTSTR)lpObjectName, objectType, DACL_SECURITY_INFORMATION, NULL, NULL, &pAcl, NULL, &pDesc);
					if (!pDesc)
						return acl;
					acl = pAcl;
					LocalFree(pDesc);
					return acl;
				}

				//! Static method setting the DACL of a given object
				static ActionStatus SetObjectDACL(LPCTSTR lpObjectName, TranslatedAcl &NewDACL, SE_OBJECT_TYPE objectType = SE_FILE_OBJECT)
				{
					if (!::SetNamedSecurityInfo((LPTSTR)lpObjectName, objectType, DACL_SECURITY_INFORMATION, NULL, NULL, NewDACL.BuildNTAcl(), NULL))
						return MAKE_STATUS(Success);
					else
						return MAKE_STATUS(ActionStatus::FailFromLastError());
				}

			public:

				Sid GetOwner()
				{
					return GetObjectOwner(m_ObjectName.c_str(), m_ObjectType);
				}

				ActionStatus SetOwner(const Sid &sid)
				{
					return SetObjectOwner(m_ObjectName.c_str(), sid, m_ObjectType);
				}

				//! Sets the owner of an object to the current user
				ActionStatus TakeOwnership()
				{
					return SetOwner(Sid::CurrentUserSid());
				}

				TranslatedAcl GetDACL()
				{
					return GetObjectDACL(m_ObjectName.c_str(), m_ObjectType);
				}

				ActionStatus SetDACL(TranslatedAcl &NewDACL)
				{
					return SetObjectDACL(m_ObjectName.c_str(), NewDACL, m_ObjectType);
				}
			};

			//! Represents a system token
			/*! The Token class is mainly used to adjust the privileges of the running process. To do so, use the
				CallerContextToken class, or EnablePrivilege()/DisablePrivilege() functions.
			*/
			class Token
			{
			private:
				HANDLE m_hToken;

			private:
				Token &operator=(const Token &)
				{
					ASSERT(FALSE);
				}

				Token(const Token &)
				{
					ASSERT(FALSE);
				}

			protected:
				Token(HANDLE hToken)
					: m_hToken(hToken)
				{
				}

				~Token()
				{
					if (m_hToken != INVALID_HANDLE_VALUE)
						CloseHandle(m_hToken);
				}

			public:
				//! Either adds or removes a privilege specified by its LUID
				ActionStatus ModifyPrivilege(const LUID &luid, bool Add)
				{
					TOKEN_PRIVILEGES priv;
					priv.PrivilegeCount = 1;
					priv.Privileges[0].Luid = luid;
					priv.Privileges[0].Attributes = Add ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_REMOVED;

					if (!AdjustTokenPrivileges(m_hToken, FALSE, &priv, sizeof(priv), NULL, NULL))
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					return MAKE_STATUS(Success);
				}

				//! Either adds or removes a privilege specified by its name (such as SE_DEBUG_NAME)
				ActionStatus ModifyPrivilege(LPCTSTR lpName, bool Add)
				{
					LUID luid;
					if (!LookupPrivilegeValue(NULL, lpName, &luid))
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					return ModifyPrivilege(luid, Add);
				}

				ActionStatus AddPrivilege(LPCTSTR lpName)
				{
					return ModifyPrivilege(lpName, true);
				}

				ActionStatus RemovePrivilege(LPCTSTR lpName)
				{
					return ModifyPrivilege(lpName, false);
				}

				//! Checks whether a token has a privilege specified by a LUID enabled
				bool CheckPrivilege(const LUID &luid)
				{
					BOOL result = FALSE;
					PRIVILEGE_SET priv;
					priv.PrivilegeCount = 1;
					priv.Control = PRIVILEGE_SET_ALL_NECESSARY;
					priv.Privilege[0].Attributes = 0;
					priv.Privilege[0].Luid = luid;

					PrivilegeCheck(m_hToken, &priv, &result);
					return result != FALSE;
				}

				//! Checks whether a token has a privilege specified by a name enabled
				bool CheckPrivilege(LPCTSTR lpName)
				{
					LUID luid;
					if (!LookupPrivilegeValue(NULL, lpName, &luid))
						return false;
					return CheckPrivilege(luid);
				}

				bool Valid()
				{
					return m_hToken != INVALID_HANDLE_VALUE;
				}

			public:
				ActionStatus BeginImpersonation()
				{
					if (!ImpersonateLoggedOnUser(m_hToken))
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					return MAKE_STATUS(Success);
				}

				ActionStatus EndImpersonation()
				{
					if (!RevertToSelf())
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					return MAKE_STATUS(Success);
				}
			};

			//! Represents a token of a calling thread or process
			/*! This class allows modifying caller's privileges. Here is a short example:
<pre>
CallerContextToken token;
token.AddPrivilege(SE_DEBUG_NAME);
//Do something
token.RemovePrivilege(SE_DEBUG_NAME);
</pre>
				\remarks See the PrivilegeHolder class for another example.
			*/
			class CallerContextToken : public Token
			{
			private:
				static HANDLE OpenThisThreadOrProcessToken(DWORD dwAccess)
				{
					HANDLE hToken = INVALID_HANDLE_VALUE;
					if (!OpenThreadToken(GetCurrentThread(), dwAccess, TRUE, &hToken))
						if (!OpenProcessToken(GetCurrentProcess(), dwAccess, &hToken))
							return INVALID_HANDLE_VALUE;
					return hToken;
				}

			public:
				CallerContextToken()
					: Token(OpenThisThreadOrProcessToken(TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES))
				{
				}
			};

			//! Represents a token obtained by calling LogonUser()
			class NTUserToken : public Token
			{
			private:
				static HANDLE DoLogonUser(LPCTSTR lpUserName, LPCTSTR lpPassword, LPCTSTR lpDomain, DWORD LogonType, DWORD LogonProvider)
				{
					HANDLE hToken = INVALID_HANDLE_VALUE;
					LogonUser((LPTSTR)lpUserName, (LPTSTR)lpDomain, (LPTSTR)lpPassword, LogonType, LogonProvider, &hToken);
					int er = GetLastError();
					return hToken ? hToken : INVALID_HANDLE_VALUE;
				}
			public:
				NTUserToken(LPCTSTR lpUserName, LPCTSTR lpPassword, LPCTSTR lpDomain = _T("."), DWORD LogonType = LOGON32_LOGON_INTERACTIVE, DWORD LogonProvider = LOGON32_PROVIDER_DEFAULT)
					: Token(DoLogonUser(lpUserName, lpPassword, lpDomain, LogonType, LogonProvider))
				{
				}
			};

			//! Enables a privilege for a caller process token
			static inline ActionStatus EnablePrivilege(LPCTSTR lpPrivName)
			{
				CallerContextToken token;
				return token.AddPrivilege(lpPrivName);
			}

			//! Disables a privilege for a caller process token
			static inline ActionStatus DisablePrivilege(LPCTSTR lpPrivName)
			{
				CallerContextToken token;
				return token.RemovePrivilege(lpPrivName);
			}

			//! Allows temporary enabling a privilege
			/*! This class allows enabling some privilege temporarily to perform an operation requiring this privilege.
				Here is an example:
<pre>
DebuggedProcess *StartDebug(PID)
{
	PrivilegeHolder holder(SE_DEBUG_NAME);	//SE_DEBUG_NAME is enabled
	return new DebuggedProcess(PID);		//SE_DEBUG_NAME is stille nabled
}											//SE_DEBUG_NAME is disabled
</pre>
				\remarks Note that if the privilege was already enabled, it will not be disabled on PrivilegeHolder destruction.
				\attention Note that PrivilegeHolder class is not thread-safe. If you modify process privileges from different threads
				simultaneously, unexpected results can be produced.
			*/
			class PrivilegeHolder
			{
			private:
				LUID m_PrivLuid;
				bool m_bChanged;

				CallerContextToken m_Token;

			public:
				PrivilegeHolder(LPCTSTR lpPrivilegeName, ActionStatus *pStatus = NULL)
					: m_bChanged(false)
				{
					if (!LookupPrivilegeValue(NULL, lpPrivilegeName, &m_PrivLuid))
					{
						ASSIGN_STATUS(pStatus, InvalidParameter);
						return;
					}
					m_bChanged = !m_Token.CheckPrivilege(m_PrivLuid);
					ActionStatus st = MAKE_STATUS(Success);

					if (m_bChanged)
						st = m_Token.ModifyPrivilege(m_PrivLuid, true);

					if (pStatus)
						*pStatus = st;
				}

				~PrivilegeHolder()
				{
					if (m_bChanged)
						m_Token.ModifyPrivilege(m_PrivLuid, false);

				}
			};
		}
	}
}