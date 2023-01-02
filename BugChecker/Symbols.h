#pragma once

#include "BugChecker.h"

#include "..\pdb\headers\bcsfile.h"

class Symbols
{
public:
	static BOOLEAN IsBcsValid(BCSFILE_HEADER* header);

	static const CHAR* GetNameOfPublic(VOID* symbols, ULONG rva, BCSFILE_PUBLIC_SYMBOL** symbolPtrPtr = NULL);
	static const BCSFILE_DATATYPE* GetDatatype(VOID* symbols, const char* typeName);
	static const BCSFILE_DATATYPE_MEMBER* GetDatatypeMember(VOID* symbols, const BCSFILE_DATATYPE* datatype, const char* memberName, const char* expectedType = NULL);
	static const CHAR* GetNameFromOffset(VOID* symbols, ULONG offset);
	static const BCSFILE_PUBLIC_SYMBOL* GetPublicByName(VOID* symbols, const char* name);
	static VOID EnumPublics(VOID* symbols, VOID* context, BOOLEAN(*callback)(VOID* context, const CHAR* name, ULONG address, ULONG length));
};
