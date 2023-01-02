#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "LNGManager.h"
#include "../textfile.h"
#include <WinNls.h>

#ifdef UNICODE

using namespace BazisLib;
using namespace BazisLib::TranslationSystem;

struct LNGKeyAndValue
{
	TempStringW Key;
	TempStringW Value;
	bool Valid;

	LNGKeyAndValue(const TempStringW &line)
		: Key(line.substr(0,0))
		, Value(line.substr(0,0))
		, Valid(false)
	{
		size_t spacingOff = line.find_first_of(L" \t");
		if ((spacingOff == -1) || !spacingOff)
			return;
		Key = line.substr(0, spacingOff);

		size_t stringOff = line.find_first_not_of(L" \t", spacingOff);
		if (stringOff == -1)
			return;

		Value = line.substr(stringOff);
		Valid = true;
	}
};

static void SubstituteEscapeChars(TCHAR *pStr)
{
	size_t r,w;
	for(r = 0, w = 0; pStr[r]; r++)
	{
		TCHAR ch = pStr[r];
		if (ch == '\\')
		{
			switch(pStr[++r])
			{
			case 'r':
				ch = '\r';
				break;
			case 'n':
				ch = '\n';
				break;
			case 't':
				ch = '\t';
				break;
			case 'b':
				ch = '\b';
				break;
			case '0':
				ch = '\0';
				break;
			case '\"':
				ch = '\"';
				break;
			case '\'':
				ch = '\'';
				break;
			case '\\':
				ch = '\\';
				break;
			default:
				r--;
				break;
			}
		}
		pStr[w++] = ch;
	}
	pStr[w] = 0;
}


BazisLib::TranslationSystem::StringTable * BazisLib::TranslationSystem::LNGManager::ParseLNGFile(const String &fp, unsigned MinID, unsigned IDCount, const std::map<TempStrPointerWrapperW, unsigned> &StringIDMap)
{
	ActionStatus st;
	ManagedPointer<AIFile> pFile = new ACFile(fp, FileModes::OpenReadOnly, &st);
	if (!pFile->Valid())
		return NULL;

	size_t stringCount = 0;
	size_t totalStringSize = 0;
	size_t stringBufferUsed = 0;

	StringTable *pTable = NULL;


	for (int pass = 0; pass < 2; pass++)
	{
		pFile->Seek(0);

		ManagedPointer<TextUnicodeFileReader> pRdr = new TextUnicodeFileReader(pFile);
		if (!pRdr->Valid())
			return NULL;

		bool InsideStringsSection = false;

		while (!pRdr->IsEOF())
		{
			DynamicStringW str = pRdr->ReadLine();
			if (str.empty() || (str[0] == ';'))
				continue;

			if (str[0] == '[')
				InsideStringsSection = !str.icompare(L"[strings]");
			else
			{
				if (!InsideStringsSection)
					continue;
				LNGKeyAndValue kv(str);
				if (!kv.Valid)
					continue;

				if (!pass)
				{
					stringCount++;
					totalStringSize += (kv.Value.length() + 1);
				}
				else
				{
					ASSERT(pTable);
					std::map<TempStrPointerWrapperW, unsigned>::const_iterator it = StringIDMap.find(TempStrPointerWrapperW(kv.Key.GetConstBuffer(), kv.Key.length()));
					if (it != StringIDMap.end())
					{
						unsigned ID = it->second;
						if (ID < MinID)
						{
							ASSERT(FALSE);
							continue;
						}
						ID -= MinID;
						if (ID >= IDCount)
						{
							ASSERT(FALSE);
							continue;
						}

						size_t thisLen = kv.Value.length() + 1;

						if ((totalStringSize - stringBufferUsed) < thisLen)
						{
							ASSERT(FALSE);
							continue;
						}

						wchar_t *thisPtr = pTable->GetStringBuffer() + stringBufferUsed;
						memcpy(thisPtr, kv.Value.GetConstBuffer(), sizeof(wchar_t) * thisLen);
						stringBufferUsed += thisLen;

						SubstituteEscapeChars(thisPtr);
						pTable->ppStrings[ID] = thisPtr;
					}					
				}
			}
		}
		
		if (!pass)
		{
			pTable = new StringTable(new wchar_t[totalStringSize], new wchar_t *[IDCount], IDCount, MinID);
			if (!pTable)
				return NULL;
			memset(pTable->ppStrings, 0, sizeof(wchar_t *) * IDCount);
		}
	}

	return pTable;
}

BazisLib::ActionStatus BazisLib::TranslationSystem::LNGManager::ReadLNGFileHeaders( const String &fp, InstalledLanguage *pLangInfo )
{
	ActionStatus st;
	ManagedPointer<TextUnicodeFileReader> pRdr = new TextUnicodeFileReader(new ACFile(fp, FileModes::OpenReadOnly, &st));
	if (!pRdr->Valid())
		return st;

	bool InsideSettingsSection = false;

	pLangInfo->LangID = 0;
	pLangInfo->LangName.clear();
	pLangInfo->LangNameEng.clear();

	unsigned doneFlags = 0;

	while (!pRdr->IsEOF())
	{
		DynamicStringW str = pRdr->ReadLine();
		if (str.empty() || (str[0] == ';'))
			continue;

		if (str[0] == '[')
			InsideSettingsSection = !str.icompare(L"[settings]");
		else
		{
			if (InsideSettingsSection)
			{
				LNGKeyAndValue kv(str);
				if (!kv.Valid)
					continue;
				if (!kv.Key.icompare(L"Language"))
					pLangInfo->LangName = kv.Value, doneFlags |= 0x01;
				else if (!kv.Key.icompare(L"LanguageENG"))
					pLangInfo->LangNameEng = kv.Value, doneFlags |= 0x02;
				else if (!kv.Key.icompare(L"LANGID"))
					pLangInfo->LangID = _wtoi(kv.Value.GetConstBuffer()), doneFlags |= 0x04;

				if (doneFlags == 0x07)	//All 3 values fetched
					return MAKE_STATUS(Success);
			}
		}
	}
	
	return MAKE_STATUS(ParsingFailed);
}

BazisLib::TranslationSystem::LNGManager::LNGManager(Directory::FindFileParams fileList)
{
	for (Directory::enumerator iterator = fileList; iterator.Valid(); iterator.Next())
	{
		LanguageFileRecord rec;
		ActionStatus st = ReadLNGFileHeaders(iterator.GetFullPath(), &rec.Lang);
		if (st.Successful())
		{
			rec.pLoadedTable = NULL;
			rec.FullLNGFilePath = iterator.GetFullPath();
			m_InstalledLanguages[rec.Lang.LangID] = rec;
		}
	}
}

StringTable * BazisLib::TranslationSystem::LNGManager::LoadStringTableForLanguage( unsigned LangID, StringIDRecord *pIDTable )
{
	if (!pIDTable)
		return NULL;

	MutexLocker lck(m_LanguageListLock);
	std::map<unsigned, LanguageFileRecord>::iterator it = m_InstalledLanguages.find(LangID);
	if (it == m_InstalledLanguages.end())
		return NULL;
	if (!it->second.pLoadedTable)
	{
		std::map<TempStrPointerWrapperW, unsigned> StringIDMap;
		unsigned FirstID = -1;
		unsigned LastID = 0;

		for (StringIDRecord *pRec = pIDTable; pRec->ID && pRec->pString; pRec++)
		{
			if (pRec->ID < FirstID)
				FirstID = pRec->ID;
			if (pRec->ID > LastID)
				LastID = pRec->ID;
			StringIDMap[pRec->pString] = pRec->ID;
		}

		it->second.pLoadedTable = ParseLNGFile(it->second.FullLNGFilePath, FirstID, (LastID - FirstID) + 1, StringIDMap);
		if (it->second.pLoadedTable)
			it->second.pLoadedTable->LangID = LangID;
	}
	return it->second.pLoadedTable;
}

#endif
#endif