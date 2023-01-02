#include "Utils.h"

BYTE* Utils::LoadFileAsByteArray(IN PCWSTR pszFileName, OUT ULONG* pulSize) // this function is from the first BugChecker.
{
	BYTE*						retval = NULL;
	NTSTATUS					ntStatus;
	HANDLE						handle = NULL;
	OBJECT_ATTRIBUTES			attrs;
	UNICODE_STRING				unicode_fn;
	IO_STATUS_BLOCK				iosb;
	FILE_STANDARD_INFORMATION	info;
	ULONG						size = 0;
	BYTE*						mem;
	LARGE_INTEGER				zeropos;

	memset(&zeropos, 0, sizeof(zeropos));

	// Load the File.

	RtlInitUnicodeString(&unicode_fn, pszFileName);

	InitializeObjectAttributes(&attrs,
		&unicode_fn,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	ntStatus = ZwCreateFile(&handle,
		FILE_READ_DATA | GENERIC_READ | SYNCHRONIZE,
		&attrs,
		&iosb,
		0,
		FILE_ATTRIBUTE_NORMAL,
		0,
		FILE_OPEN,
		FILE_NON_DIRECTORY_FILE | FILE_RANDOM_ACCESS | FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);

	if (ntStatus == STATUS_SUCCESS && handle)
	{
		ntStatus = ZwQueryInformationFile(
			handle,
			&iosb,
			&info,
			sizeof(info),
			FileStandardInformation);

		if (ntStatus == STATUS_SUCCESS)
		{
			size = info.EndOfFile.LowPart;

			mem = new BYTE[size];

			if (mem)
			{
				ntStatus = ZwReadFile(
					handle,
					NULL,
					NULL,
					NULL,
					&iosb,
					mem,
					size,
					&zeropos,
					NULL);

				if (ntStatus != STATUS_SUCCESS || iosb.Information != size)
				{
					delete[] mem;
				}
				else
				{
					retval = mem;
				}
			}
		}

		ZwClose(handle);
	}

	// Return.

	if (pulSize && retval)
		*pulSize = size;

	return retval;
}

CHAR* Utils::ParseStructuredFile(IN BYTE* pbFile, IN ULONG ulSize, IN ULONG ulTabsNum, IN const CHAR* pszString, IN CHAR* pszStart, OUT CHAR (*paOutput)[1024]) // this function is from the first BugChecker.
{
	CHAR* pszEnd;
	CHAR* p;
	BOOLEAN bEndOfLine = FALSE, bEndOfFile = FALSE;
	int i, x, y;
	CHAR* retval = NULL;
	size_t nNumOfTabs;
	char* pszLine;
	CHAR* pszLineStart;
	BOOLEAN bTNReached = FALSE;
	BOOLEAN bFirstLine = TRUE;

	if (pbFile == NULL || ulSize == 0)
		return NULL;
	if (ulTabsNum == 0 && pszString == NULL)
		return NULL;

	pszEnd = (CHAR*)pbFile + ulSize - 1; // -> LAST VALID CHAR <-

	if (pszStart == NULL)
		pszStart = (CHAR*)pbFile;

	// Parse Each Line.

	p = pszStart;

	do
	{
		bEndOfLine = FALSE;

		pszLineStart = p;

		for (i = 0; i < sizeof((*paOutput)) / sizeof(CHAR); i++)
		{
			if (p > pszEnd)
			{
				bEndOfLine = TRUE;
				bEndOfFile = TRUE;

				(*paOutput)[i] = '\0';
				break;
			}
			else if (*p == '\r' || *p == '\0')
			{
				if ((p + 1) <= pszEnd &&
					*(p + 1) == '\n')
					p++;

				bEndOfLine = TRUE;

				(*paOutput)[i] = '\0';
				break;
			}
			else
			{
				(*paOutput)[i] = *p++;
			}
		}

		p++;

		if (bEndOfLine == FALSE) // LINE TOO LONG
			break;
		else
		{
			// Calculate the number of tabulations at the left of the line.

			nNumOfTabs = 0;
			for (x = 0, y = strlen((*paOutput)); x < y; x++)
				if ((*paOutput)[x] == '\t')
					nNumOfTabs++;
				else
					break;

			// Make controls on the Tabs Num.

			if (
				((pszString == NULL && bFirstLine == FALSE) || bTNReached) &&
				nNumOfTabs < ulTabsNum
				)
				break;

			if (nNumOfTabs >= ulTabsNum)
				bTNReached = TRUE;

			// The resulting string must be non-null.

			pszLine = &(*paOutput)[nNumOfTabs];
			if (strlen(pszLine) != 0)
			{
				// Check if there is a Match.

				if (ulTabsNum == nNumOfTabs &&
					(pszString == NULL || ::strcmp(pszString, pszLine) == 0))
				{
					retval = pszLineStart + nNumOfTabs;
					break;
				}
			}
		}

		bFirstLine = FALSE;

	} while (bEndOfFile == FALSE);

	// Return.

	return retval;
}

eastl::string Utils::GuidToString(BYTE* guid)
{
	CHAR buffer[128];

	GUID* g = (GUID*)guid;

	::sprintf(buffer, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
		g->Data1, g->Data2, g->Data3,
		g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
		g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

	return buffer;
}

eastl::string Utils::Dword32ToString(DWORD dw)
{
	CHAR buffer[64];

	::sprintf(buffer, "%X", dw);

	return buffer;
}

CHAR* Utils::stristr(IN CONST CHAR* string, IN CONST CHAR* strCharSet) // from the first BugChecker.
{
	if (strlen(strCharSet) <= strlen(string))
	{
		size_t		size = strlen(string);
		size_t		sizeCharSet = strlen(strCharSet);
		ULONG		i;

		for (i = 0; i <= size - sizeCharSet; i++, string++)
			if (!Utils::memicmp(string, strCharSet, sizeCharSet))
				return (CHAR*)string;
	}

	return NULL;
}

#define _TOLOWER(c) ( ((c) >= 'A') && ((c) <= 'Z') ? ((c) - 'A' + 'a') :\
              (c) )

int Utils::memicmp ( // from the first BugChecker.
	const void* first,
	const void* last,
	unsigned int count
	)
{
	int f = 0;
	int l = 0;

	while (count--)
	{
		if ((*(unsigned char*)first == *(unsigned char*)last) ||
			((f = _TOLOWER(*(unsigned char*)first)) ==
				(l = _TOLOWER(*(unsigned char*)last))))
		{
			first = (char*)first + 1;
			last = (char*)last + 1;
		}
		else
			break;
	}

	return (f - l);
}

BOOLEAN Utils::AreStringsEqualI(const CHAR* str1, const CHAR* str2)
{
	if (::strlen(str1) != ::strlen(str2)) return FALSE;

	if (!stristr(str1, str2)) return FALSE;

	return TRUE;
}

eastl::string Utils::HexToString(ULONG64 hex, size_t size)
{
	CHAR buffer[64] = "";

	switch (size)
	{
		case sizeof(BYTE):
			::sprintf(buffer, "%02X", (BYTE)hex);
			break;
		case sizeof(WORD):
			::sprintf(buffer, "%04X", (WORD)hex);
			break;
		case sizeof(DWORD):
			::sprintf(buffer, "%08X", (DWORD)hex);
			break;
		case sizeof(QWORD):
			::sprintf(buffer, "%08X%08X", (DWORD)(hex >> 32), (DWORD)hex);
			break;
	}

	return buffer;
}

eastl::string Utils::I64ToString(LONG64 i64)
{
	CHAR buffer[80] = "";

	::sprintf(buffer, "%I64d", i64);

	return buffer;
}

eastl::vector<eastl::string> Utils::Tokenize(const eastl::string& str, const eastl::string& delimiter)
{
	eastl::vector<eastl::string> retVal;

	eastl::string s = str;
	size_t pos = 0;

	while ((pos = s.find(delimiter)) != eastl::string::npos)
	{
		eastl::string token = s.substr(0, pos);
		retVal.push_back(eastl::move(token));

		s.erase(0, pos + delimiter.length());
	}

	retVal.push_back(eastl::move(s));

	return retVal;
}

VOID Utils::ToLower(eastl::string& s)
{
	for (CHAR& c : s)
		if (c >= 'A' && c <= 'Z')
			c = c - 'A' + 'a';
}
