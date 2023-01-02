#include "stdafx.h"

#ifdef WIN32
#ifdef _NTDDK_
#include "../WinKernel/security.h"

typedef bool BOOL;

#else
#include "security.h"
#endif

#ifndef UNDER_CE

using namespace BazisLib;

#ifdef _NTDDK_
using namespace BazisLib::DDK::Security;
#else
using namespace BazisLib::Win32::Security;
#endif

#ifdef _NTDDK_
Sid::Sid( LPCTSTR lpAccountName, LPCTSTR lpSystemName /*= NULL*/, String *pDomainName /*= NULL*/, ActionStatus *pStatus /*= NULL*/ )
{
	SID_NAME_USE use;
	UNICODE_STRING AccountName;
	RtlInitUnicodeString(&AccountName, lpAccountName);
	ULONG bufSize = sizeof(m_Buffer), domainSize = 0;
	NTSTATUS st = SecLookupAccountName(&AccountName, &bufSize, (PSID)m_Buffer, &use, &domainSize, NULL);
	if (NT_SUCCESS(st) && !pDomainName)
	{
		ASSIGN_STATUS(pStatus, Success);
		return;
	}
	ASSERT(bufSize <= sizeof(m_Buffer));
	if (!bufSize)
	{
		memset(m_Buffer, 0, sizeof(m_Buffer));
		ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(st));
		return;
	}


	bufSize = sizeof(m_Buffer);	//Workaround for bug in SecLookupAccountName
	if (pDomainName)
	{
		st = SecLookupAccountName(&AccountName, &bufSize, (PSID)m_Buffer, &use, &domainSize, pDomainName->ToNTString(domainSize));
		pDomainName->UpdateLengthFromNTString();
	}
	else
	{
		String tmp;
		st = SecLookupAccountName(&AccountName, &bufSize, (PSID)m_Buffer, &use, &domainSize, tmp.ToNTString(domainSize));
	}

	if (!NT_SUCCESS(st))
	{
		memset(m_Buffer, 0, sizeof(m_Buffer));
		ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(st));
		return;
	}

	ASSIGN_STATUS(pStatus, ActionStatus::FromNTStatus(st));
}
#else 
Sid::Sid( LPCTSTR lpAccountName, LPCTSTR lpSystemName /*= NULL*/, String *pDomainName /*= NULL*/, ActionStatus *pStatus /*= NULL*/ )
{
	DWORD bufSize = sizeof(m_Buffer), domainSize = 0;
	SID_NAME_USE use;
	BOOL success = LookupAccountName(lpSystemName, lpAccountName, (PSID)m_Buffer, &bufSize, NULL, &domainSize, &use);
	if (success && !pDomainName)
	{
		ASSIGN_STATUS(pStatus, Success);
		return;
	}
	ASSERT(bufSize <= sizeof(m_Buffer));
	if (!bufSize)
	{
		ASSIGN_STATUS(pStatus, ActionStatus::FailFromLastError());
		memset(m_Buffer, 0, sizeof(m_Buffer));
		return;
	}
	if (pDomainName)
	{
		LookupAccountName(lpSystemName, lpAccountName, (PSID)m_Buffer, &bufSize, pDomainName->PreAllocate(domainSize, false), &domainSize, &use);
		pDomainName->SetLength(domainSize);
	}
	else
	{
		TCHAR *pDomain = new TCHAR[domainSize + 1];
		LookupAccountName(lpSystemName, lpAccountName, (PSID)m_Buffer, &bufSize, pDomain, &domainSize, &use);
		delete pDomain;
	}
	ASSIGN_STATUS(pStatus, ActionStatus::FromLastError());
}
#endif

#ifdef _NTDDK_
BazisLib::ActionStatus Sid::QuerySidInformation( String *pAccountName, String *pDomainName /*= NULL*/, LPCTSTR lpSystemName /*= NULL*/, SID_NAME_USE *pNameUse /*= NULL*/ ) const
{
	ULONG accNameSize = 0, domNameSize = 0;
	SID_NAME_USE use;
	NTSTATUS st = SecLookupAccountSid((PSID)m_Buffer, &accNameSize, NULL, &domNameSize, NULL, &use);
	if (!accNameSize || !domNameSize)
		return MAKE_STATUS(ActionStatus::FromNTStatus(st));

	String tmp1, tmp2;
	if (!pAccountName)
		pAccountName = &tmp1;
	if (!pDomainName)
		pDomainName = &tmp1;

	st = SecLookupAccountSid((PSID)m_Buffer, &accNameSize, pAccountName->ToNTString(accNameSize), &domNameSize, pDomainName->ToNTString(domNameSize), &use);
	pAccountName->UpdateLengthFromNTString();
	pDomainName->UpdateLengthFromNTString();

	return MAKE_STATUS(ActionStatus::FromNTStatus(st));
}
#else
BazisLib::ActionStatus Sid::QuerySidInformation( String *pAccountName, String *pDomainName /*= NULL*/, LPCTSTR lpSystemName /*= NULL*/, SID_NAME_USE *pNameUse /*= NULL*/ ) const
{
	DWORD accNameSize = 0, domNameSize = 0;
	SID_NAME_USE use;
	LookupAccountSid(lpSystemName, (PSID)m_Buffer, NULL, &accNameSize, NULL, &domNameSize, pNameUse ? pNameUse : &use);
	if (!accNameSize || !domNameSize)
		return MAKE_STATUS(ActionStatus::FailFromLastError());
	TCHAR *pwszAccount = pAccountName ? pAccountName->PreAllocate(accNameSize, false) : new TCHAR[accNameSize];
	TCHAR *pwszDomain  = pDomainName  ? pDomainName->PreAllocate(accNameSize, false) : new TCHAR[domNameSize];
	if (!LookupAccountSid(lpSystemName,
		(PSID)m_Buffer,
		pwszAccount,
		&accNameSize,
		pwszDomain,
		&domNameSize,
		pNameUse ? pNameUse : &use))
		return MAKE_STATUS(ActionStatus::FailFromLastError());

	if (pAccountName)
		pAccountName->SetLength(accNameSize);
	else
		delete pwszAccount;

	if (pDomainName)
		pDomainName->SetLength(domNameSize);
	else
		delete pwszDomain;
	return MAKE_STATUS(Success);
}
#endif

//-------------------------------------------------------------------------------------------------------------------------------------

TranslatedAce::TranslatedAce( PVOID pAce )
{
	PACE_HEADER pHdr = (PACE_HEADER)pAce;
	switch (pHdr->AceType)
	{
	case ACCESS_ALLOWED_ACE_TYPE:
		m_AccessAllowedACE = *(ACCESS_ALLOWED_ACE *)pAce;
		m_Sid = (PSID)&(((ACCESS_ALLOWED_ACE *)pAce)->SidStart);
		break;
	case ACCESS_DENIED_ACE_TYPE:
		m_AccessDeniedACE = *(ACCESS_DENIED_ACE *)pAce;
		m_Sid = (PSID)&(((ACCESS_DENIED_ACE *)pAce)->SidStart);
		break;
	case SYSTEM_AUDIT_ACE_TYPE:
		m_SystemAuditACE = *(SYSTEM_AUDIT_ACE *)pAce;
		m_Sid = (PSID)&(((SYSTEM_AUDIT_ACE *)pAce)->SidStart);
		break;
	default:
		memset(&m_AccessAllowedACE, 0, sizeof(m_AccessAllowedACE));
		break;
	}
}

TranslatedAce::TranslatedAce( const Sid &sid, ACCESS_MASK Mask, BYTE AceType /*= ACCESS_ALLOWED_ACE_TYPE*/, BYTE AceFlags /*= CONTAINER_INHERIT_ACE*/ ) : m_Sid(sid)
{
	m_AccessAllowedACE.Header.AceType = AceType;
	m_AccessAllowedACE.Header.AceFlags = AceFlags;
	switch (AceType)
	{
	case ACCESS_ALLOWED_ACE_TYPE:
		m_AccessAllowedACE.Mask = Mask;
		break;
	case ACCESS_DENIED_ACE_TYPE:
		m_AccessDeniedACE.Mask = Mask;
		break;
	case SYSTEM_AUDIT_ACE_TYPE:
		m_SystemAuditACE.Mask = Mask;
		break;
	default:
		memset(&m_AccessAllowedACE, 0, sizeof(m_AccessAllowedACE));
		break;
	}
}

size_t TranslatedAce::Serialize( PVOID pBuffer, size_t MaxSize ) const
{
	size_t baseSize = 0;
	size_t sidLength = m_Sid.GetLength();
	switch (m_AccessAllowedACE.Header.AceType)
	{
	case ACCESS_ALLOWED_ACE_TYPE:
		if (MaxSize < sizeof(m_AccessAllowedACE))
			return 0;
		baseSize = sizeof(m_AccessAllowedACE) - sizeof(m_AccessAllowedACE.SidStart);
		memcpy(pBuffer, &m_AccessAllowedACE, sizeof(m_AccessAllowedACE));
		((ACCESS_ALLOWED_ACE *)pBuffer)->Header.AceSize = (WORD)(baseSize + sidLength);
		break;
	case ACCESS_DENIED_ACE_TYPE:
		if (MaxSize < sizeof(m_AccessDeniedACE))
			return 0;
		baseSize = sizeof(m_AccessDeniedACE) - sizeof(m_AccessAllowedACE.SidStart);
		memcpy(pBuffer, &m_AccessDeniedACE, sizeof(m_AccessDeniedACE));
		((ACCESS_DENIED_ACE *)pBuffer)->Header.AceSize = (WORD)(baseSize + sidLength);
		break;
	case SYSTEM_AUDIT_ACE_TYPE:
		if (MaxSize < sizeof(m_SystemAuditACE))
			return 0;
		baseSize = sizeof(m_SystemAuditACE) - sizeof(m_AccessAllowedACE.SidStart);
		memcpy(pBuffer, &m_SystemAuditACE, sizeof(m_SystemAuditACE));
		((SYSTEM_AUDIT_ACE *)pBuffer)->Header.AceSize = (WORD)(baseSize + sidLength);
		break;
	default:
		return 0;
	}
	MaxSize -= baseSize;
	if (sidLength > MaxSize)
		return 0;
	if (!m_Sid.CopyTo(((char *)pBuffer) + baseSize, sidLength))
		return 0;
	return baseSize + sidLength;
}

#ifdef _NTDDK_
static inline void GetSecurityDescriptorControl (
    __in  PSECURITY_DESCRIPTOR pSecurityDescriptor,
    __out PSECURITY_DESCRIPTOR_CONTROL pControl,
    __out PULONG lpdwRevision
    )
{
	struct _SECURITY_DESCRIPTOR {
		UCHAR Revision;
		UCHAR Sbz1;
		SECURITY_DESCRIPTOR_CONTROL Control;
	};

	*lpdwRevision = ((_SECURITY_DESCRIPTOR*)pSecurityDescriptor)->Revision;
	*pControl = ((_SECURITY_DESCRIPTOR*)pSecurityDescriptor)->Control;
}
#endif


//-------------------------------------------------------------------------------------------------------------------------------------

TranslatedSecurityDescriptor::TranslatedSecurityDescriptor( PSECURITY_DESCRIPTOR pDescriptor /*= NULL*/ ) : DACLPresent(false)
, SACLPresent(false)
, m_ControlBits(0)
{
	if (!pDescriptor)
		return;
#ifndef _NTDDK_
	BOOL def = FALSE, present = FALSE;
#else
	BOOLEAN def = FALSE, present = FALSE;
#endif
	PSID pSid = NULL;
#ifdef _NTDDK_
	if (NT_SUCCESS(RtlGetOwnerSecurityDescriptor(pDescriptor, &pSid, &def)))
#else
	if (GetSecurityDescriptorOwner(pDescriptor, &pSid, &def))
#endif
	{
		if (pSid)
			Owner = pSid;
		OwnerDefaulted = (def != FALSE);
	}
	pSid = NULL;
	def = FALSE;
#ifdef _NTDDK_
	if (NT_SUCCESS(RtlGetGroupSecurityDescriptor(pDescriptor, &pSid, &def)))
#else
	if (GetSecurityDescriptorGroup(pDescriptor, &pSid, &def))
#endif
	{
		if (pSid)
			Group = pSid;
		GroupDefaulted = (def != FALSE);
	}
	DWORD dwRev = 0;
	GetSecurityDescriptorControl(pDescriptor, &m_ControlBits, &dwRev);
	def = FALSE;
	PACL pAcl = NULL;
#ifdef _NTDDK_
	if (NT_SUCCESS(RtlGetDaclSecurityDescriptor(pDescriptor, &present, &pAcl, &def)))
#else
	if (GetSecurityDescriptorDacl(pDescriptor, &present, &pAcl, &def))
#endif
	{
		DACLPresent = (present != FALSE);
		if (pAcl)
			DACL = pAcl;
	}
	pAcl = NULL;
	present = FALSE;
#ifdef _NTDDK_
	if (NT_SUCCESS(RtlGetSaclSecurityDescriptor(pDescriptor, &present, &pAcl, &def)))
#else
	if (GetSecurityDescriptorSacl(pDescriptor, &present, &pAcl, &def))
#endif
	{
		SACLPresent = (present != FALSE);
		if (pAcl)
			SACL = pAcl;
	}

	DWORD dwSize = 0, dwSize1 = 0, dwSize2 = 0, dwSize3 = 0, dwSize4 = 0;
#ifdef _NTDDK_
	RtlSelfRelativeToAbsoluteSD(pDescriptor, NULL, &dwSize, NULL, &dwSize1, NULL, &dwSize2, NULL, &dwSize3, NULL, &dwSize4);
#else
	MakeAbsoluteSD(pDescriptor, NULL, &dwSize, NULL, &dwSize1, NULL, &dwSize2, NULL, &dwSize3, NULL, &dwSize4);
#endif
	if (dwSize)
	{
		m_pOriginalDescriptor.EnsureSizeAndSetUsed(dwSize + dwSize1 + dwSize2 + dwSize3 + dwSize4);
		void *pMain = m_pOriginalDescriptor.GetData();
		void *pDACL = ((char *)pMain) + dwSize;
		void *pSACL = ((char *)pDACL) + dwSize1;
		void *pOwner = ((char *)pSACL) + dwSize2;
		void *pGroup = ((char *)pOwner) + dwSize3;
#ifdef _NTDDK_
		RtlSelfRelativeToAbsoluteSD(pDescriptor, pMain, &dwSize, (PACL)pDACL, &dwSize1, (PACL)pSACL, &dwSize2, pOwner, &dwSize3, pGroup, &dwSize4);
#else
		MakeAbsoluteSD(pDescriptor, pMain, &dwSize, (PACL)pDACL, &dwSize1, (PACL)pSACL, &dwSize2, pOwner, &dwSize3, pGroup, &dwSize4);
#endif
	}
}

PSECURITY_DESCRIPTOR TranslatedSecurityDescriptor::BuildNTSecurityDescriptor()
{
	if (!m_pOriginalDescriptor)
	{
#ifdef _NTDDK_
		m_pOriginalDescriptor.EnsureSizeAndSetUsed(sizeof(_SECURITY_DESCRIPTOR));
		RtlCreateSecurityDescriptor(m_pOriginalDescriptor.GetData(), SECURITY_DESCRIPTOR_REVISION);
#else
		m_pOriginalDescriptor.EnsureSizeAndSetUsed(SECURITY_DESCRIPTOR_MIN_LENGTH);
		InitializeSecurityDescriptor(m_pOriginalDescriptor.GetData(), SECURITY_DESCRIPTOR_REVISION);
#endif
	}
	PSECURITY_DESCRIPTOR pDesc = m_pOriginalDescriptor.GetData();

#ifdef _NTDDK_
	RtlSetOwnerSecurityDescriptor(pDesc, Owner.Valid() ? (PSID)Owner : NULL, OwnerDefaulted);
	RtlSetGroupSecurityDescriptor(pDesc, Group.Valid() ? (PSID)Group : NULL, GroupDefaulted);

	if (DACLPresent)
		RtlSetDaclSecurityDescriptor(pDesc, TRUE, DACL.BuildNTAcl(), FALSE);
	else
		RtlSetDaclSecurityDescriptor(pDesc, FALSE, NULL, TRUE);

	if (SACLPresent)
		RtlSetSaclSecurityDescriptor(pDesc, TRUE, SACL.BuildNTAcl(), FALSE);
	else
		RtlSetSaclSecurityDescriptor(pDesc, FALSE, NULL, TRUE);

#else
	SetSecurityDescriptorOwner(pDesc, Owner.Valid() ? (PSID)Owner : NULL, OwnerDefaulted);
	SetSecurityDescriptorGroup(pDesc, Group.Valid() ? (PSID)Group : NULL, GroupDefaulted);

	if (DACLPresent)
		SetSecurityDescriptorDacl(pDesc, TRUE, DACL.BuildNTAcl(), FALSE);
	else
		SetSecurityDescriptorDacl(pDesc, FALSE, NULL, TRUE);

	if (SACLPresent)
		SetSecurityDescriptorSacl(pDesc, TRUE, SACL.BuildNTAcl(), FALSE);
	else
		SetSecurityDescriptorSacl(pDesc, FALSE, NULL, TRUE);
#endif
	
	return pDesc;
}

#endif
#endif