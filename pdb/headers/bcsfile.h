#pragma once

typedef unsigned char BYTE;
typedef unsigned long ULONG;

#define BCSFILE_HEADER_SIGNATURE		0x00534342
#define BCSFILE_HEADER_VERSION			2

typedef struct
{
	ULONG		magic;
	ULONG		version;

	BYTE		guid[16];
	ULONG		age;

	ULONG		publicSymbols;
	ULONG		publicSymbolsSize;

	ULONG		datatypes;
	ULONG		datatypesSize;

	ULONG		datatypeMembers;
	ULONG		datatypeMembersSize;

	// write the names at the end of the file, to preserve the alignment of the structs.
	ULONG		names;
	ULONG		namesSize;

} BCSFILE_HEADER;

typedef struct
{
	ULONG		name;
	ULONG		address;
	ULONG		length;

} BCSFILE_PUBLIC_SYMBOL;

typedef struct
{
	ULONG		name;
	ULONG		kind;
	ULONG		length;
	ULONG		pdbIndex;
	ULONG		firstMember;
	ULONG		numOfMembers;

} BCSFILE_DATATYPE;

typedef struct
{
	ULONG		name;
	ULONG		datatype;
	ULONG		offset;
	ULONG		pdbDatatypeIndex;

} BCSFILE_DATATYPE_MEMBER;
