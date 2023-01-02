#pragma once
#include "i18n.h"
#include <map>
#include "../../bzscore/sync.h"

namespace BazisLib
{
	namespace TranslationSystem
	{
		class LNGManager
		{
		private:
			struct LanguageFileRecord
			{
				InstalledLanguage Lang;
				String FullLNGFilePath;
				StringTable *pLoadedTable;

				LanguageFileRecord()
					: pLoadedTable(NULL)
				{
				}
			};

			std::map<unsigned, LanguageFileRecord> m_InstalledLanguages;
			Mutex m_LanguageListLock;

		private:
			static StringTable *ParseLNGFile(const String &fp, unsigned MinID, unsigned IDCount, const std::map<TempStrPointerWrapperW, unsigned> &StringIDMap);
			static ActionStatus ReadLNGFileHeaders(const String &fp, InstalledLanguage *pLangInfo);

		public:
			StringTable *LoadStringTableForLanguage(unsigned LangID, StringIDRecord *pIDTable);
			
			LNGManager(Directory::FindFileParams fileList);
			
			~LNGManager()
			{
				MutexLocker lck(m_LanguageListLock);
				for each(const std::pair<unsigned, LanguageFileRecord> &rec in m_InstalledLanguages)
					delete rec.second.pLoadedTable;
			}

			std::vector<InstalledLanguage> GetInstalledLanguageList()
			{
				std::vector<InstalledLanguage> ret;
				MutexLocker lck(m_LanguageListLock);
				ret.reserve(m_InstalledLanguages.size());
				for each(const std::pair<unsigned, LanguageFileRecord> &kv in m_InstalledLanguages)
					ret.push_back(kv.second.Lang);
				return ret;
			}
		};
	}
}