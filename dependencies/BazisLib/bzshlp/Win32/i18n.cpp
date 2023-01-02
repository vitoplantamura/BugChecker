#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "i18n.h"
#include "LNGManager.h"

using namespace BazisLib;
using namespace BazisLib::TranslationSystem;

extern StringIDRecord g_StringIDs[];

static LNGManager *s_pLNGManager = NULL;

StringTable *BazisLib::TranslationSystem::g_pCurrentLanguageStringTable;

//Not reentrant
BazisLib::ActionStatus BazisLib::InitializeLNGBasedTranslationEngine(LANGID PreferredLangID, const String &LNGDirectory /*= _T("langfiles")*/ , HMODULE hMainModule)
{
	if (s_pLNGManager)
		return MAKE_STATUS(InvalidState);
	Directory::FindFileParams iter;
	if (Path::IsAbsolute(LNGDirectory))
		iter = Directory(LNGDirectory).FindFirstFile(_T("*.lng"));
	else
	{
		TCHAR tsz[MAX_PATH] = { 0, };
		GetModuleFileName(hMainModule, tsz, __countof(tsz));
		iter = Directory(Path::Combine(Path::GetDirectoryName(TempStrPointerWrapper(tsz)), LNGDirectory)).FindFirstFile(_T("*.lng"));
	}

	s_pLNGManager = new LNGManager(iter);
	SelectLanguage(PreferredLangID ? PreferredLangID : GetUserDefaultLangID());
	return MAKE_STATUS(Success);
}

//Not reentrant
void BazisLib::ShutdownTranslationEngine()
{
	g_pCurrentLanguageStringTable = NULL;
	delete s_pLNGManager;
	s_pLNGManager = NULL;
}

//Reentrant due to LNGManager synchronization
BazisLib::ActionStatus BazisLib::SelectLanguage( LANGID LangID )
{
	if (!s_pLNGManager)
		return MAKE_STATUS(NotInitialized);
	g_pCurrentLanguageStringTable = s_pLNGManager->LoadStringTableForLanguage(LangID, g_StringIDs);
	return MAKE_STATUS(Success);
}

//Reentrant due to LNGManager synchronization
std::vector<InstalledLanguage> BazisLib::GetInstalledLanguageList()
{
	if (!s_pLNGManager)
		return std::vector<InstalledLanguage>();
	return s_pLNGManager->GetInstalledLanguageList();
}

//Reentrant
LANGID BazisLib::GetCurrentLanguageId()
{
	TranslationSystem::StringTable *pTable = TranslationSystem::g_pCurrentLanguageStringTable;
	if (!pTable)
		return 0;
	return pTable->LangID;
}
#endif