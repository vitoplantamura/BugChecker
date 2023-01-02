#pragma once

#define _BZSDDK_SECURITY_INCLUDED_

#ifndef SID_IDENTIFIER_AUTHORITY_DEFINED
#define SID_IDENTIFIER_AUTHORITY_DEFINED
typedef struct _SID_IDENTIFIER_AUTHORITY {
	UCHAR Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;
#endif


#ifndef SID_DEFINED
#define SID_DEFINED
typedef struct _SID {
	UCHAR Revision;
	UCHAR SubAuthorityCount;
	SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
#ifdef MIDL_PASS
	[size_is(SubAuthorityCount)] ULONG SubAuthority[*];
#else // MIDL_PASS
	ULONG SubAuthority[ANYSIZE_ARRAY];
#endif // MIDL_PASS
} SID, *PISID;
#endif

#define SID_REVISION                     (1)    // Current revision level
#define SID_MAX_SUB_AUTHORITIES          (15)
#define SID_RECOMMENDED_SUB_AUTHORITIES  (1)    // Will change to around 6

//Reference security-related functions from ntifs.h, as ntifs.h is incomatible with ntddk.h
#define SECURITY_MAX_SID_SIZE  \
	(sizeof(SID) - sizeof(ULONG) + (SID_MAX_SUB_AUTHORITIES * sizeof(ULONG)))


typedef enum _SID_NAME_USE {
	SidTypeUser = 1,
	SidTypeGroup,
	SidTypeDomain,
	SidTypeAlias,
	SidTypeWellKnownGroup,
	SidTypeDeletedAccount,
	SidTypeInvalid,
	SidTypeUnknown,
	SidTypeComputer,
	SidTypeLabel
} SID_NAME_USE, *PSID_NAME_USE;


#define KSECDDDECLSPEC
#define SEC_ENTRY __stdcall

extern "C" 
{
	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlCopySid (
		__in ULONG DestinationSidLength,
		__in_bcount(DestinationSidLength) PSID DestinationSid,
		__in PSID SourceSid
		);

	NTSYSAPI
		BOOLEAN
		NTAPI
		RtlEqualSid (
		__in PSID Sid1,
		__in PSID Sid2
		);

	NTSYSAPI
		BOOLEAN
		NTAPI
		RtlValidSid (
		__in PSID Sid
		);

	KSECDDDECLSPEC
		NTSTATUS
		SEC_ENTRY
		SecLookupAccountSid(
		__in      PSID Sid,
		__out     PULONG NameSize,
		__inout   PUNICODE_STRING NameBuffer,
		__out     PULONG DomainSize OPTIONAL,
		__out_opt PUNICODE_STRING DomainBuffer OPTIONAL,
		__out     PSID_NAME_USE NameUse
		);

	KSECDDDECLSPEC
		NTSTATUS
		SEC_ENTRY
		SecLookupAccountName(
		__in        PUNICODE_STRING Name,
		__inout     PULONG SidSize,
		__out       PSID Sid,
		__out       PSID_NAME_USE NameUse,
		__out       PULONG DomainSize OPTIONAL,
		__inout_opt PUNICODE_STRING ReferencedDomain OPTIONAL
		);

	KSECDDDECLSPEC
		NTSTATUS
		SEC_ENTRY
		SecLookupWellKnownSid(
		__in        WELL_KNOWN_SID_TYPE SidType,
		__out       PSID Sid,
		__in        ULONG SidBufferSize,
		__inout_opt PULONG SidSize OPTIONAL
		);

	NTSYSAPI
		PSID_IDENTIFIER_AUTHORITY
		NTAPI
		RtlIdentifierAuthoritySid (
		__in PSID Sid
		);

	NTSYSAPI
		PUCHAR
		NTAPI
		RtlSubAuthorityCountSid (
		__in PSID Sid
		);

	NTSYSAPI
		PULONG
		NTAPI
		RtlSubAuthoritySid (
		__in PSID Sid,
		__in ULONG SubAuthority
		);

	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlConvertSidToUnicodeString(
		__inout PUNICODE_STRING UnicodeString,
		__in PSID Sid,
		__in BOOLEAN AllocateDestinationString
		);

	NTSYSAPI
		ULONG
		NTAPI
		RtlLengthSid (
		__in PSID Sid
		);

	NTSYSCALLAPI
		NTSTATUS
		NTAPI
		ZwOpenThreadToken(
		__in HANDLE ThreadHandle,
		__in ACCESS_MASK DesiredAccess,
		__in BOOLEAN OpenAsSelf,
		__out PHANDLE TokenHandle
		);

	NTSYSCALLAPI
		NTSTATUS
		NTAPI
		ZwOpenProcessToken(
		__in HANDLE ProcessHandle,
		__in ACCESS_MASK DesiredAccess,
		__out PHANDLE TokenHandle
		);

	typedef struct _SID_AND_ATTRIBUTES {
#ifdef MIDL_PASS
		PISID Sid;
#else // MIDL_PASS
		PSID Sid;
#endif // MIDL_PASS
		ULONG Attributes;
	} SID_AND_ATTRIBUTES, * PSID_AND_ATTRIBUTES;

	typedef struct _TOKEN_USER {
		SID_AND_ATTRIBUTES User;
	} TOKEN_USER, *PTOKEN_USER;

	typedef enum _TOKEN_INFORMATION_CLASS {
		TokenUser = 1,
	} TOKEN_INFORMATION_CLASS;

	NTSYSCALLAPI
		NTSTATUS
		NTAPI
		ZwQueryInformationToken (
		__in HANDLE TokenHandle,
		__in TOKEN_INFORMATION_CLASS TokenInformationClass,
		__out_bcount_part_opt(TokenInformationLength, *ReturnLength) PVOID TokenInformation,
		__in ULONG TokenInformationLength,
		__out PULONG ReturnLength
		);

#define TOKEN_QUERY             (0x0008)

	typedef unsigned long DWORD, *PDWORD;
	C_ASSERT(sizeof(DWORD) == 4);

	typedef struct _ACE_HEADER {
		UCHAR AceType;
		UCHAR AceFlags;
		USHORT AceSize;
	} ACE_HEADER;
	typedef ACE_HEADER *PACE_HEADER;

	typedef struct _ACCESS_ALLOWED_ACE {
		ACE_HEADER Header;
		ACCESS_MASK Mask;
		ULONG SidStart;
	} ACCESS_ALLOWED_ACE;

	typedef ACCESS_ALLOWED_ACE *PACCESS_ALLOWED_ACE;

	typedef struct _ACCESS_DENIED_ACE {
		ACE_HEADER Header;
		ACCESS_MASK Mask;
		ULONG SidStart;
	} ACCESS_DENIED_ACE;
	typedef ACCESS_DENIED_ACE *PACCESS_DENIED_ACE;

	typedef struct _SYSTEM_AUDIT_ACE {
		ACE_HEADER Header;
		ACCESS_MASK Mask;
		ULONG SidStart;
	} SYSTEM_AUDIT_ACE;
	typedef SYSTEM_AUDIT_ACE *PSYSTEM_AUDIT_ACE;

	typedef UCHAR BYTE;
	typedef USHORT WORD;

	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlCreateAcl (
		__out_bcount(AclLength) PACL Acl,
		__in ULONG AclLength,
		__in ULONG AclRevision
		);

	struct SECURITY_DESCRIPTOR;
	typedef WORD   SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;


#define ACCESS_MIN_MS_ACE_TYPE                  (0x0)
#define ACCESS_ALLOWED_ACE_TYPE                 (0x0)
#define ACCESS_DENIED_ACE_TYPE                  (0x1)
#define SYSTEM_AUDIT_ACE_TYPE                   (0x2)


#define OBJECT_INHERIT_ACE                (0x1)
#define CONTAINER_INHERIT_ACE             (0x2)
#define NO_PROPAGATE_INHERIT_ACE          (0x4)
#define INHERIT_ONLY_ACE                  (0x8)
#define INHERITED_ACE                     (0x10)
#define VALID_INHERIT_FLAGS               (0x1F)


	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlGetOwnerSecurityDescriptor (
		__in PSECURITY_DESCRIPTOR SecurityDescriptor,
		__out PSID *Owner,
		__out PBOOLEAN OwnerDefaulted
		);

	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlGetGroupSecurityDescriptor (
		__in PSECURITY_DESCRIPTOR SecurityDescriptor,
		__out PSID *Group,
		__out PBOOLEAN GroupDefaulted
		);

	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlSetOwnerSecurityDescriptor (
		__inout PSECURITY_DESCRIPTOR SecurityDescriptor,
		__in_opt PSID Owner,
		__in_opt BOOLEAN OwnerDefaulted
		);

	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlSetGroupSecurityDescriptor (
		__inout PSECURITY_DESCRIPTOR SecurityDescriptor,
		__in_opt PSID Group,
		__in_opt BOOLEAN GroupDefaulted
		);

	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlGetDaclSecurityDescriptor (
		__in PSECURITY_DESCRIPTOR SecurityDescriptor,
		__out PBOOLEAN DaclPresent,
		__out PACL *Dacl,
		__out PBOOLEAN DaclDefaulted
		);

	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlGetSaclSecurityDescriptor (
		__in PSECURITY_DESCRIPTOR SecurityDescriptor,
		__out PBOOLEAN DaclPresent,
		__out PACL *Dacl,
		__out PBOOLEAN DaclDefaulted
		);

	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlSelfRelativeToAbsoluteSD (
		__in PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
		__out_bcount_part_opt(*AbsoluteSecurityDescriptorSize, *AbsoluteSecurityDescriptorSize) PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
		__inout PULONG AbsoluteSecurityDescriptorSize,
		__out_bcount_part_opt(*DaclSize, *DaclSize) PACL Dacl,
		__inout PULONG DaclSize,
		__out_bcount_part_opt(*SaclSize, *SaclSize) PACL Sacl,
		__inout PULONG SaclSize,
		__out_bcount_part_opt(*OwnerSize, *OwnerSize) PSID Owner,
		__inout PULONG OwnerSize,
		__out_bcount_part_opt(*PrimaryGroupSize, *PrimaryGroupSize) PSID PrimaryGroup,
		__inout PULONG PrimaryGroupSize
		);

	struct _SECURITY_DESCRIPTOR {
		UCHAR Revision;
		UCHAR Sbz1;
		SECURITY_DESCRIPTOR_CONTROL Control;
		PSID Owner;
		PSID Group;
		PACL Sacl;
		PACL Dacl;
	};

	NTSYSAPI
		NTSTATUS
		NTAPI
		RtlSetSaclSecurityDescriptor (
		__inout PSECURITY_DESCRIPTOR SecurityDescriptor,
		__in BOOLEAN DaclPresent,
		__in_opt PACL Dacl,
		__in_opt BOOLEAN DaclDefaulted
		);

	NTSYSAPI
		NTSTATUS
		NTAPI
		ZwSetSecurityObject(
		__in HANDLE Handle,
		__in SECURITY_INFORMATION SecurityInformation,
		__in PSECURITY_DESCRIPTOR SecurityDescriptor
		);

	NTSYSAPI
		NTSTATUS
		NTAPI
		ZwQuerySecurityObject (
		__in HANDLE Handle,
		__in SECURITY_INFORMATION SecurityInformation,
		__out_bcount_opt(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
		__in ULONG Length,
		__out PULONG LengthNeeded
		);
};


#include "../Win32/security_common.h"

