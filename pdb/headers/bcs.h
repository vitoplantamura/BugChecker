#pragma once

#include "diainclude.h"

void BCS_AddPublicSymbol(const char* name, ULONG address, ULONG length);
void BCS_WriteFile();
void BCS_SavePdbInfo(GUID& guid, DWORD age);
BOOL BCS_AddDataType(const char* name, const char* kind, ULONG length, DWORD index);
void BCS_AddDataTypeMember(const char* name, const char* datatype, ULONG offset, DWORD datatypeIndex);
