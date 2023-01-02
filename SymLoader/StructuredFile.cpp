#include "StructuredFile.h"

#include "SymLoader.h"

CHAR* StructuredFile::Parse(IN BYTE* pbFile, IN ULONG ulSize, IN ULONG ulTabsNum, IN const CHAR* pszString, IN CHAR* pszStart, OUT CHAR(*paOutput)[1024]) // this function is from the first BugChecker.
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

StructuredFile::Entry* StructuredFile::LoadConfig()
{
	Entry* ret = NULL;

	// open and read the file.

	auto fn = SymLoader::BcDir + L"\\BugChecker.dat";

	auto f = ::_wfopen(fn.c_str(), L"rb");
	if (f)
	{
		::fseek(f, 0L, SEEK_END);
		auto filesize = ::ftell(f);
		::fseek(f, 0L, SEEK_SET);

		auto buffer = new BYTE[filesize];
		if (::fread(buffer, 1, filesize, f) != filesize)
			filesize = 0;
		::fclose(f);

		if (filesize > 0)
		{
			// now we can create the final object.

			ret = new Entry();

			// parse the file.

			CHAR line[1024];

			auto add = [&](const CHAR* name) {

				CHAR* p = StructuredFile::Parse(buffer, filesize, 0, name, NULL, &line);
				if (p)
				{
					eastl::function<void(Entry&, ULONG)> iterate = [&](Entry& e, ULONG num) {

						while (1)
						{
							CHAR* p2 = StructuredFile::Parse(buffer, filesize, num, NULL, p, &line);
							if (!p2)
								break;
							else
								p = p2;

							e.children.push_back(Entry{ line + num }); // "+num" skips the tab(s)
							iterate(e.children.back(), num + 1);
						}
					};

					ret->children.push_back(Entry{ name });
					iterate(ret->children.back(), 1);
				}
			};

			add("symbols");
			add("settings");
		}

		// free the file contents memory.

		delete[] buffer;
	}

	// return.

	return ret;
}

BOOL StructuredFile::SaveConfig(Entry* config)
{
	BOOL ret = TRUE;

	// open the file for writing.

	auto fn = SymLoader::BcDir + L"\\BugChecker.dat";

	auto f = ::_wfopen(fn.c_str(), L"wt");
	if (!f)
		ret = FALSE;
	else
	{
		// write the header from the first BugChecker, just for nostalgic reasons (even if the file name is different...).

		::fprintf(f,
			"+---------------------------------------------------------------------------------+\n"
			"| bugchk.dat file -> VPC Technologies's BugChecker Structured Configuration File. |\n"
			"| !! WARNING !! DO NOT EDIT directly: use Symbol Loader application instead...    |\n"
			"| Format Note: the number of TABULATIONS determines the hierarchy of the data.    |\n"
			"+---------------------------------------------------------------------------------+\n"
			"\n");

		// write the file.

		eastl::function<void(Entry&,ULONG)> iterate = [&](Entry& t, ULONG num)
		{
			for (auto& e : t.children)
			{
				if (::fprintf(f, "%s%s\n", eastl::string(num, '\t').c_str(), e.name.c_str()) < 0)
					ret = FALSE;
				else
					iterate(e, num + 1);
			}
		};

		iterate(*config, 0);

		// close the file.

		::fclose(f);
	}

	// return and eventually delete the file.

	if (!ret)
		::DeleteFileW(fn.c_str());

	return ret;
}
