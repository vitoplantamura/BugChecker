#pragma once

#include "BugChecker.h"

#include <EASTL/string.h>
#include <EASTL/vector.h>

class Utils
{
public:
	static BYTE* LoadFileAsByteArray(IN PCWSTR pszFileName, OUT ULONG* pulSize);
	static CHAR* ParseStructuredFile(IN BYTE* pbFile, IN ULONG ulSize, IN ULONG ulTabsNum, IN const CHAR* pszString, IN CHAR* pszStart, OUT CHAR(*paOutput)[1024]);

	static eastl::string GuidToString(BYTE* guid);
	static eastl::string Dword32ToString(DWORD dw);

	static CHAR* stristr(IN CONST CHAR* string, IN CONST CHAR* strCharSet);
	static int memicmp(const void* first, const void* last, unsigned int count);
	static BOOLEAN AreStringsEqualI(const CHAR* str1, const CHAR* str2);
	static eastl::string HexToString(ULONG64 hex, size_t size);
	static eastl::string I64ToString(LONG64 i64);
	static eastl::vector<eastl::string> Tokenize(const eastl::string& str, const eastl::string& delimiter);
	static VOID ToLower(eastl::string& s);
	static BYTE NegateByte(BYTE b);
};
