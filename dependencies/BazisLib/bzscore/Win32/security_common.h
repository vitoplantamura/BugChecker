#pragma once

#ifndef UNDER_CE

#include "../status.h"
#include <list>
#include "../buffer.h"
#include <sddl.h>

#ifndef _NTDDK_
#include <AccCtrl.h>
#include <AclApi.h>
#endif

namespace BazisLib
{
#ifdef _NTDDK_
#define CopySid RtlCopySid
#define EqualSid RtlEqualSid
#define IsValidSid RtlValidSid

	namespace DDK
#else
	namespace Win32
#endif
	{
		//! Contains classes simplifying security operations
		/*! \section overview Overview
			This namespace contains several classes that can be used to perform security-related operations under Win32. This includes most operations
			related to SECURITY_DESCRIPTOR and SECURITY_ATTRIBUTES structures, as well as operations related to token privilege adjustment.
			An example of security class usage can be found under <b>samples/windows/security</b>.
			\section typical_cases Typical Use Cases
			Here is a short reference on using various security-related classes to perform various common tasks:
			<ul>
				<li>To modify security properties of a file (change DACL or take ownership) without opening it, use the NamedSecurityAccessor class</li>
				<li>To create a new object, such as a file, directory, named pipe, or similar, use the TranslatedSecurityDescriptor class to build
				a security descriptor and finally call the TranslatedSecurityDescriptor::BuildNTSecurityDescriptor() or
				TranslatedSecurityDescriptor::BuildSecurityAttributes() method.</li>
				<li>To modify security attributes of an open file, use the security-related methods of Win32::File class. <b>Note, that by-handle security
				information can be incomplete due to Windows limitations and it is recommended to use NamedSecurityAccessor instead.</b></li>
				<li>To adjust the privileges of the current token (such as SE_DEBUG_NAME), use either EnablePrivilege()/DisablePrivilege(), the
				CallerContextToken class, or the PrivilegeHolder class if you want to temporarily enable a privilege.</li>
			</ul>
			\section sd_classes Security Descriptor-related classes
			Windows security model is quite complex. Changing access rights of a file using Win32 API functions could be a complex task involving retrieving existing
			ACL, allocating and building another one with modified items, sorting it and setting it back. To simplify this operations, BazisLib represents  an access
			control list (ACL) using the TranslatedAcl class, that is a subclass of an STL list of TranslatedAce objects. Each access control entry (ACE) is represented
			by a TranslatedAce class, that contains the original ACE and an instance of Sid class. This requires more memory than a traditional variable-length ACE including
			the variable-length SID, but provides more flexibility.
			All contents of a TranslatedAce, including the SID, can be easily accessed or changed, and all items inside a TranslatedAcl object can be easily manipulated using
			normal STL list methods. Finally, before passing the ACL back to Win32 API functions, the TranslatedAcl::BuildNTAcl() method should be called.
			
			A security descriptor is represented using the TranslatedSecurityDescriptor class, that contains owner and group SIDs and two TranslatedAcl objects for DACL and SACL.
			\remarks
				Note that a newly created security descriptor has the DACLPresent and SACLPresent fields set to false, indicating an absent SACL and DACL. If you want to create
				a security descriptor with a valid DACL, be sure to set the TranslatedSecurityDescriptor::DACLPresent field to true before calling 
				TranslatedSecurityDescriptor::BuildNTSecurityDescriptor().
		*/
		namespace Security
		{
			//Information taken from here: http://www.winzero.ca/WEllKnownSIDs.htm
			enum StandardAccount
			{
				accAdministrator = 500,
				accGuest = 501,
			};

			//! Represents a Security Identifier (SID)
			/*! This class represents a Windows Security Identifier that, in turn, represents a user account or a group.
				The Sid class includes a buffer big enough to fit a SID of a maximum length and does not use the heap,
				that makes various inline optimizations possible. Normally, a Sid object is obtained by calling one of 
				the static methods:
				<ul>
					<li>Sid::WellKnownSid()</li>
					<li>Sid::FromAccountName()</li>
					<li>Sid::CurrentUserSid()</li>
				</ul>
				The Sid::Valid() method can be used to test whether a given Sid is valid.
			*/
			class Sid
			{
			private:
				char m_Buffer[SECURITY_MAX_SID_SIZE];
			
			public:
				//! Creates an empty (invalid) Sid object
				Sid()
				{
					//SID_IDENTIFIER_AUTHORITY nullAuthority = SECURITY_NULL_SID_AUTHORITY;
					//InitializeSid((PSID)m_Buffer, &nullAuthority, 0);
					memset(m_Buffer, 0, sizeof(m_Buffer));
				}

				//! Creates a copy of an existing SID structure
				Sid(PSID pSid)
				{
					CopySid(sizeof(m_Buffer), (PSID)m_Buffer, pSid);
				}

				//! Creates a Sid representing a specified user account
				Sid(LPCTSTR lpAccountName, LPCTSTR lpSystemName = NULL, String *pDomainName = NULL, ActionStatus *pStatus = NULL);

				//! Creates a well-known Sid (such as WinBuiltinAdministratorsSid or WinWorldSid)
				Sid(WELL_KNOWN_SID_TYPE sidType, PSID pDomain = NULL)
				{
#ifdef _NTDDK_
					ULONG sidSize = 0;
					SecLookupWellKnownSid(sidType, (PSID)m_Buffer, sizeof(m_Buffer), &sidSize);
#else
					DWORD dwBufSize = sizeof(m_Buffer);
					CreateWellKnownSid(sidType, pDomain, (PSID)m_Buffer, &dwBufSize);
#endif
				}

				~Sid()
				{
				}

				Sid(const Sid &sid)
				{
					CopySid(sizeof(m_Buffer), (PSID)m_Buffer, (PSID)sid.m_Buffer);
				}

				Sid &operator=(const Sid &sid)
				{
					CopySid(sizeof(m_Buffer), (PSID)m_Buffer, (PSID)sid.m_Buffer);
					return *this;
				}

				Sid &operator=(PSID pSid)
				{
					CopySid(sizeof(m_Buffer), (PSID)m_Buffer, pSid);
					return *this;
				}

				//! Compares two SIDs
				bool operator==(const Sid &sid) const
				{
					return (EqualSid((PSID)m_Buffer, (PSID)sid.m_Buffer) != 0);
				}

				//! Compares two SIDs
				bool operator!=(const Sid &sid) const
				{
					return (EqualSid((PSID)m_Buffer, (PSID)sid.m_Buffer) == 0);
				}

				//! Returns a pointer to a SID stored in a Sid object
				operator PSID()
				{
					return (PSID)m_Buffer;
				}

				//! Returns a constant pointer to a SID stored in a Sid object
				operator const PSID() const
				{
					return (PSID)m_Buffer;
				}

				bool Valid() const
				{
					return (IsValidSid((PSID)m_Buffer) != 0);
				}

			public:
				//! Returns various SID-related information
				BazisLib::ActionStatus QuerySidInformation(String *pAccountName,
														   String *pDomainName = NULL,
														   LPCTSTR lpSystemName = NULL,
														   SID_NAME_USE *pNameUse = NULL) const;

				PSID_IDENTIFIER_AUTHORITY GetIdentifierAuthority() const
				{
#ifdef _NTDDK_
					return RtlIdentifierAuthoritySid((PSID)m_Buffer);
#else
					return GetSidIdentifierAuthority((PSID)m_Buffer);
#endif
				}

				size_t GetSubAuthorityCount() const
				{
#ifdef _NTDDK_
					PUCHAR pCount = RtlSubAuthorityCountSid((PSID)m_Buffer);
#else
					PUCHAR pCount = GetSidSubAuthorityCount((PSID)m_Buffer);
#endif
					if (!pCount)
						return 0;
					return *pCount;
				}

				PULONG GetSubAuthority(size_t index) const
				{
#ifdef _NTDDK_
					return RtlSubAuthoritySid((PSID)m_Buffer, (ULONG)index);
#else
					return GetSidSubAuthority((PSID)m_Buffer, (ULONG)index);
#endif
				}

				//! Converts a SID to a string form (such as S-1-5-20)
				String ToString() const
				{
#ifdef _NTDDK_
					UNICODE_STRING str = {0,};
					NTSTATUS st = RtlConvertSidToUnicodeString(&str, (PSID)m_Buffer, TRUE);
					if (!NT_SUCCESS(st))
						return String();
					String ret;
					ret = &str;
					RtlFreeUnicodeString(&str);
					return ret;
#else
					LPTSTR lpStr = NULL;
					ConvertSidToStringSid((PSID)m_Buffer, &lpStr);
					if (!lpStr)
						return _T("");
					String str = lpStr;
					LocalFree(lpStr);
					return str;
#endif
				}

				//! Returns SID account name (or a string form SID if the name 
				String GetAccountName(bool ReturnStringFormIfNameNotFound = true) const
				{
					String ret;
					if (!QuerySidInformation(&ret).Successful() || ret.empty())
						return ReturnStringFormIfNameNotFound ? ToString() : String();
					return ret;
				}

				//! Returns SID length that can be used to copy the SID
				size_t GetLength() const
				{
#ifdef _NTDDK_
					return RtlLengthSid((PSID)m_Buffer);
#else
					return GetLengthSid((PSID)m_Buffer);
#endif
				}

				//! Copies a SID to another buffer
				bool CopyTo(PSID pDestSid, size_t maxSize) const
				{
					return CopySid((DWORD)maxSize, pDestSid, (PSID)m_Buffer) != 0;
				}

				//! Checks whether a SID represents a standard built-in account (such as Administrator or Guest)
				bool IsStandardAccount(StandardAccount account)
				{
					SID *pSid = (SID *)m_Buffer;
					return (pSid->SubAuthority[pSid->SubAuthorityCount - 1] == account);
				}

			public:
				
				//! Creates a well-known Sid (such as WinBuiltinAdministratorsSid or WinWorldSid)
				static Sid WellKnownSid(WELL_KNOWN_SID_TYPE sidType, PSID pDomain = NULL)
				{
					return Sid(sidType, pDomain);
				}

				static Sid Everyone()
				{
					struct  
					{
						SID sid;
						ULONG subAuthorities[1];
					} everyone;
					memset(&everyone, 0, sizeof(everyone));
					everyone.sid.Revision = SID_REVISION;
					everyone.sid.IdentifierAuthority.Value[__countof(everyone.sid.IdentifierAuthority.Value) - 1] = 1;
					everyone.sid.SubAuthorityCount = 1;
					everyone.sid.SubAuthority[0] = 0;
					return Sid((PSID)&everyone);
				}

				//! Creates a Sid for a specified user account
				static Sid FromAccountName(LPCTSTR lpAccountName, LPCTSTR lpSystemName = NULL, String *pDomainName = NULL, ActionStatus *pStatus = NULL)
				{
					return Sid(lpAccountName, lpSystemName, pDomainName, pStatus);
				}

				//! Returns the Sid of the user running the calling program
				static Sid CurrentUserSid()
				{
					HANDLE hToken;
#ifdef _NTDDK_
					if (!NT_SUCCESS(ZwOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, FALSE, &hToken)))
						if (!NT_SUCCESS(ZwOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY, &hToken)))
							return Sid();
#else
					if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
						if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
							return Sid();
#endif
					TypedBuffer<TOKEN_USER> pUserInfo;
					DWORD dwSize = 0;
#ifdef _NTDDK_
					ZwQueryInformationToken(hToken, TokenUser, NULL, 0, &dwSize);
#else
					GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
#endif
					pUserInfo.EnsureSizeAndSetUsed(dwSize);

#ifdef _NTDDK_
					if (!NT_SUCCESS(ZwQueryInformationToken(hToken, TokenUser, pUserInfo, (DWORD)pUserInfo.GetAllocated(), &dwSize)))
					{
						ZwClose(hToken);
						return Sid();
					}
					ZwClose(hToken);
#else
					if (!GetTokenInformation(hToken, TokenUser, pUserInfo, (DWORD)pUserInfo.GetAllocated(), &dwSize))
					{
						CloseHandle(hToken);
						return Sid();
					}
					CloseHandle(hToken);
#endif
					return Sid(pUserInfo->User.Sid);
				}

			};

			//! Represents an access control entry (ACE)
			/*! The TranslatedAce class represents an access control entry (ACE). The class includes a public m_Sid member that can be accessed or changed at any time.
				This makes the TranslatedAce class more flexible, than the raw Win32 ACEs, having variable size dependent on the SID size and not allowing to change the SID
				to a differently sized SID without reallocating memory.
				To convert a TranslatedAce to a raw ACE structure, the Serialize() method should be used (TranslatedAcl class automatically uses both Serialize() and
				GetTotalACESize() methods to build raw ACLs).
				\remarks
					Normally, TranslatedAce objects are used only as a part of a TranslatedAcl list. However, if you want to add special entries to a TranslatedAcl,
					you can create a TranslatedAce object and push it to the list.
			*/
			class TranslatedAce
			{
			private:
				union
				{
					ACCESS_ALLOWED_ACE m_AccessAllowedACE;
					ACCESS_DENIED_ACE m_AccessDeniedACE;
					SYSTEM_AUDIT_ACE m_SystemAuditACE;
				};
			public:
				//! Represents the SID of the ACE
				Sid m_Sid;
				
			public:
				TranslatedAce()
				{
					memset(&m_AccessAllowedACE, 0, sizeof(m_AccessAllowedACE));
				}

				TranslatedAce(PVOID pAce);
				TranslatedAce(const Sid &sid, ACCESS_MASK Mask, BYTE AceType = ACCESS_ALLOWED_ACE_TYPE, BYTE AceFlags = CONTAINER_INHERIT_ACE);

#pragma region Methods for querying entry structure pointers
				ACCESS_ALLOWED_ACE *GetAllowedACE()
				{
					return &m_AccessAllowedACE;
				}

				ACCESS_DENIED_ACE *GetDeniedACE()
				{
					return &m_AccessDeniedACE;
				}

				SYSTEM_AUDIT_ACE *GetAuditACE()
				{
					return &m_SystemAuditACE;
				}

				const ACCESS_ALLOWED_ACE *GetAllowedACE() const
				{
					return &m_AccessAllowedACE;
				}

				const ACCESS_DENIED_ACE *GetDeniedACE() const
				{
					return &m_AccessDeniedACE;
				}

				const SYSTEM_AUDIT_ACE *GetAuditACE() const
				{
					return &m_SystemAuditACE;
				}
#pragma endregion

				//! Gets access mask of an entry taking its type into consideration
				ACCESS_MASK GetAccessMask() const
				{
					switch (m_AccessAllowedACE.Header.AceType)
					{
					case ACCESS_ALLOWED_ACE_TYPE:
						return m_AccessAllowedACE.Mask;
					case ACCESS_DENIED_ACE_TYPE:
						return m_AccessDeniedACE.Mask;
					case SYSTEM_AUDIT_ACE_TYPE:
						return m_SystemAuditACE.Mask;
					default:
						return 0;
					}
				}

				//! Sets access mask of an entry taking its type into consideration
				void SetAccessMask(ACCESS_MASK newMask)
				{
					switch (m_AccessAllowedACE.Header.AceType)
					{
					case ACCESS_ALLOWED_ACE_TYPE:
						m_AccessAllowedACE.Mask = newMask;
						break;
					case ACCESS_DENIED_ACE_TYPE:
						m_AccessDeniedACE.Mask = newMask;
						break;
					case SYSTEM_AUDIT_ACE_TYPE:
						m_SystemAuditACE.Mask = newMask;
						break;
					}
				}

				//! Returns an ACE header that is  common for all ACE types
				PACE_HEADER GetHeader()
				{
					return &m_AccessAllowedACE.Header;
				}

				//! Returns a constant pointer to ACE header that is common for all ACE types
				const ACE_HEADER *GetHeader() const
				{
					return &m_AccessAllowedACE.Header;
				}

				//! Returns ACCESS_ALLOWED_ACE_TYPE, ACCESS_DENIED_ACE_TYPE or SYSTEM_AUDIT_ACE_TYPE
				int GetType() const
				{
					return m_AccessAllowedACE.Header.AceType;
				}

				//! Returns the pointer to the m_Sid member
				Sid *GetSid()
				{
					return &m_Sid;
				}

				//! Returns the constant pointer to the m_Sid member
				const Sid *GetSid() const
				{
					return &m_Sid;
				}

				//! Compares two ACEs to ensure correct order, when the raw ACL list is being built.
				bool operator<(const TranslatedAce &_Right) const
				{
					if ((m_AccessAllowedACE.Header.AceType == ACCESS_DENIED_ACE_TYPE) != (_Right.m_AccessAllowedACE.Header.AceType == ACCESS_DENIED_ACE_TYPE))
						return (m_AccessAllowedACE.Header.AceType == ACCESS_DENIED_ACE_TYPE);
					if ((m_AccessAllowedACE.Header.AceFlags & INHERITED_ACE) != (_Right.m_AccessAllowedACE.Header.AceFlags & INHERITED_ACE))
						return (m_AccessAllowedACE.Header.AceFlags & INHERITED_ACE) == 0;
					return (this < &_Right);	//Order does not matter in that case
				}

			public:

				//! Returns the total size of a raw ACE corresponding to current TranslatedAce object
				size_t GetTotalACESize() const
				{
					size_t baseSize = 0;
					switch (m_AccessAllowedACE.Header.AceType)
					{
					case ACCESS_ALLOWED_ACE_TYPE:
						baseSize = sizeof(m_AccessAllowedACE);
						break;
					case ACCESS_DENIED_ACE_TYPE:
						baseSize = sizeof(m_AccessDeniedACE);
						break;
					case SYSTEM_AUDIT_ACE_TYPE:
						baseSize = sizeof(m_SystemAuditACE);
						break;
					default:
						return 0;
					}
					return baseSize + m_Sid.GetLength() - sizeof(m_AccessAllowedACE.SidStart);
				}

				//! Converts the TranslatedAce object into a raw ACE suitable for including in raw ACLs
				size_t Serialize(PVOID pBuffer, size_t MaxSize) const;
			};

			//! Represents an "Access Allowed" ACE providing a convenient constructor.
			class AllowingAce : public TranslatedAce
			{
			public:
				AllowingAce(ACCESS_MASK Access, const Sid &sid = Sid::WellKnownSid(WinWorldSid), BYTE AceFlags = CONTAINER_INHERIT_ACE)
					: TranslatedAce(sid, Access, ACCESS_ALLOWED_ACE_TYPE, AceFlags)
				{
				}
			};

			//! Represents an "Access Denied" ACE providing a convenient constructor.
			class DenyingAce : public TranslatedAce
			{
			public:
				DenyingAce(ACCESS_MASK Access, const Sid &sid = Sid::WellKnownSid(WinWorldSid), BYTE AceFlags = CONTAINER_INHERIT_ACE)
					: TranslatedAce(sid, Access, ACCESS_DENIED_ACE_TYPE, AceFlags)
				{
				}
			};

			//! Represents an access control list (ACL)
			/*! The TranslatedAcl class represents an access control list (ACL). To simplify reading and changing individual ACEs inside an ACL,
				this class is derived from a STL list of TranslatedAce objects. That way, once a TranslatedAcl is created (or obtained from a 
				raw ACL returned by system), it is possible to access, modify or add its entries using standard STL operators and methods.
				Additionally, you can use the MS-specific "for each" operator for iterating over ACEs in a TranslatedAcl.
			*/
			class TranslatedAcl : public std::list<TranslatedAce>
			{
			private:
				TypedBuffer<PACL> m_CombinedACL;

			public:
				//! Creates an empty ACL, or an ACL based on an existing raw ACL structure
				/*! This constructor creates a TranslatedAcl object based on a raw ACL structure provided by Win32 API.
					Note that the created TranslatedAcl copies all information from the original ACL and does not need
					it after the constructor has returned.
				*/
				TranslatedAcl(PACL pRawAcl = NULL)
				{
					if (!pRawAcl)
						return;
					PACE_HEADER pAce = (PACE_HEADER)(pRawAcl + 1);
					int aceProcessed = 0;
					while (aceProcessed++ < pRawAcl->AceCount)
					{
						push_back(TranslatedAce(pAce));
						pAce = (PACE_HEADER)(((char *)pAce) + pAce->AceSize);
					}
				}

				//! Produces raw ACL suitable for passing to Win32 API functions
				/*! The BuildNTAcl method sorts the access control entries of an ACE and produces a raw ACL suitable for
					passing to Win32 API functions, such as SetNamedSecurityInfo(). Note that the produced ACL pointer is
					managed by the TranslatedAcl automatically and does not need to be freed afterwards.
				*/
				PACL BuildNTAcl()
				{
					size_t totalSize = sizeof(ACL);
					for (const TranslatedAce &ace : *this)
						totalSize += ace.GetTotalACESize();

					sort();

					m_CombinedACL.EnsureSizeAndSetUsed(totalSize);
#ifdef _NTDDK_
					RtlCreateAcl((PACL)m_CombinedACL.GetData(), (DWORD)totalSize, ACL_REVISION);
#else
					InitializeAcl((PACL)m_CombinedACL.GetData(), (DWORD)totalSize, ACL_REVISION);
#endif
					void *pAceBuf = ((char *)m_CombinedACL.GetData()) + sizeof(ACL);
					totalSize -= sizeof(ACL);
					size_t done = 0;
					for (const TranslatedAce &ace : *this)
					{
						size_t aceSize = ace.Serialize(pAceBuf, totalSize);
						totalSize -= aceSize;
						pAceBuf = ((char *)pAceBuf) + aceSize;
						done++;
					}
					((PACL)m_CombinedACL.GetData())->AceCount = (WORD)done;
					return (PACL)m_CombinedACL.GetData();
				}

			public:
				//! Adds a new "Access Allowed" entry to an ACL
				void AddAllowingAce(ACCESS_MASK AccessMask, const Sid &sid = Sid::WellKnownSid(WinWorldSid), BYTE AceFlags = CONTAINER_INHERIT_ACE)
				{
					push_back(AllowingAce(AccessMask, sid, AceFlags));
				}

				//! Adds a new "Access Denied" entry to an ACL
				void AddDenyingAce(ACCESS_MASK AccessMask, const Sid &sid = Sid::WellKnownSid(WinWorldSid), BYTE AceFlags = CONTAINER_INHERIT_ACE)
				{
					push_back(DenyingAce(AccessMask, sid, AceFlags));
				}

				//! Removes all entries referencing a given SID
				void RemoveAcesForSid(const Sid &sid)
				{
					for (iterator it = begin(); it != end(); )
					{
						if (it->m_Sid == sid)
							erase(it++);
						else
							it++;
					}
				}
			};

			//! Represents a Security Descriptor
			/*! The TranslatedSecurityDescriptor class represents a Windows security descriptor. This class can be created either empty, or
				from an existing security descriptor. In the last case, the supplied descriptor is copied to the TranslatedSecurityDescriptor class.
				The class contains several public fields, that can be freely accessed or modified.
				<br/>To obtain a pointer to a NT SECURITY_DESCRIPTOR structure, or a SECURITY_ATTRIBUTES structure, the BuildNTSecurityDescriptor() or
				BuildSecurityAttributes() methods should be used.
				\remarks If you are creating a new security descriptor and adding some entries to its DACL, do not forget to set the DACLPresent field 
				to true, as otherwise the DACL will be ignored.
			*/
			class TranslatedSecurityDescriptor
			{
			public:
				TranslatedAcl SACL;
				TranslatedAcl DACL;
				Sid Owner;
				Sid Group;
				bool DACLPresent;
				bool SACLPresent;
				bool OwnerDefaulted;
				bool GroupDefaulted;

			private:
				TypedBuffer<SECURITY_DESCRIPTOR> m_pOriginalDescriptor;
				SECURITY_DESCRIPTOR_CONTROL m_ControlBits;
#ifndef _NTDDK_
				SECURITY_ATTRIBUTES m_AttributesStructure;
#endif

			public:
				TranslatedSecurityDescriptor(PSECURITY_DESCRIPTOR pDescriptor = NULL);

				//! Builds a raw SECURITY_DESCRIPTOR using an automatically managed buffer
				PSECURITY_DESCRIPTOR BuildNTSecurityDescriptor();
				
#ifndef _NTDDK_
				//! Builds a raw SECURITY_ATTRIBUTES structure using an automatically managed buffer
				SECURITY_ATTRIBUTES *BuildSecurityAttributes(bool bInheritHandles = false)
				{
					m_AttributesStructure.nLength = sizeof(m_AttributesStructure);
					m_AttributesStructure.bInheritHandle = bInheritHandles;
					m_AttributesStructure.lpSecurityDescriptor = BuildNTSecurityDescriptor();
					return &m_AttributesStructure;
				}

				//! Converts security descriptor to a string format suitable for serializing
				String ToString()
				{
					LPTSTR lpStr = NULL;
					ConvertSecurityDescriptorToStringSecurityDescriptor(BuildNTSecurityDescriptor(), SDDL_REVISION_1, -1, &lpStr, NULL);
					if (!lpStr)
						return _T("");
					String ret = lpStr;
					LocalFree(lpStr);
					return ret;
				}
#endif
			};
		}
	}
}
#endif