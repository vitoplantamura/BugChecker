#pragma once
#include "../i18n.h"
#include "../../bzscore/status.h"
#include "../../bzscore/file.h"
#include <vector>

/*! \page i18n_sys Internationalization/localization system
	Starting from BazisLib 2.4.0, it includes support for localizing applications. A custom
	localization engine is used instead of Windows string tables to achieve the following goals:
	<ul>
		<li>Minimum effort while developing application logic. Just use _TR(IDS_SOMETHING, "english string")
		in your C++ code and you can compile and run your debug build using default english strings.</li>
		<li>Automated generation and updating of translation files for new languages. A separate tool,
		STRGEN.EXE, generates all required headers, sources and translatable LNG files based on your source
		code containing _TR() statements.</li>
		<li>You do not need a compiler or programming knowledge to make a translation. Just edit a unicode
		LNG file, replace the language name and the translation is ready.</li>
		<li>Potential cross-platform support. The LNG-based engine should be able to localize native Windows
		applications as well as usual Win32 programs.</li>
	</ul>
	To make your BazisLib-based application localizable, use the following steps:
	<ul>
		<li>Replace the _T("Some string") to _TR(IDS_SOMESTRING, "Some string") for all strings requiring to
		be localized</li>. You do not need to define IDS_SOMESTRING and similar macros! In debug builds, they
		are ignored, and for release builds they will be generated automatically.
		<li>Use the LOCALIZE_DIALOG() and LOCALIZE_DLGITEM() macros to localize dialogs. Example:
<pre>
LOCALIZE_DIALOG(IDS_SETTINGSHDR, IDD_SETTINGS, m_hWnd);
LOCALIZE_DLGITEM(IDS_LTRPOLICY, IDC_LTRPOLICY, m_hWnd);
</pre>
		Note that you do not need to specify the dialog item text in those macros. The default values are extracted
		from RC files by STRGEN.EXE.</li>
		<li>Every time you add or remove _TR() or LOCALIZE_xxx() statements in your program and want to make a
		Release build, run the STRGEN.EXE in your source directory. It will produce 3 files:
		<ul>
			<li>A header file containing definitions for IDS_xxx macros referenced in your code.</li>
			<li>A source file defining the map from the "IDS_xxx" strings to corresponding numbers</li>
			<li>A LNG file containing all detected IDS_xxx strings and their default values</li>
		</ul>
		</li>
		<li>Call the InitializeLNGBasedTranslationEngine() method in the beginning of your program</li>
		<li>Call the ShutdownTranslationEngine() method before finishing the program</li>
		<li>Ensure that the program can find the LNG files in the directory specified to InitializeLNGBasedTranslationEngine()</li>
	</ul>
	Note that if a LNG file is not found, or a particular string from it cannot be loaded, the default english string
	specified in the code will be used.
	\section i18n_iface Interface
	The following functions are related to the internationalization/localization engine:
	<ul>
	<li>InitializeLNGBasedTranslationEngine()</li>
	<li>ShutdownTranslationEngine()</li>
	<li>GetInstalledLanguageList()</li>
	<li>GetCurrentLanguageId()</li>
	<li>SelectLanguage()</li>
	</ul>
	\section i18n_porting Porting
	The general interface is not restricted to support only LNG-based translation engine. You can add your own engine, provide an initialization
	function for it and ensure that it is cleant up on ShutdownTranslationEngine() call. Generally, your engine will need to provide a list of
	installed languages, create string tables for any language, maintain the list of previously allocated tables and free the list on engine
	deletion. Consider implementing such an engine in a separate file and modifying bzswin/i18n.cpp to support it.

	\remarks The translation system is fully thread-safe. Every call to SelectLanguage() will leave the old table in memory, keeping
			 the old string pointers valid until ShutdownTranslationEngine() is called. Also note that the _TR() and similar macros will
			 not crash your program if used before translation system initialization. Instead, the default values will be used. However,
			 calling ShutdownTranslationEngine() before all localization-dependent threads are finished, can crash the program, as all
			 previously obtained localized string pointers will become invalid.
*/

namespace BazisLib
{
	//! Initializes the translation engine based on Unicode LNG files.
	/*!	This function should be called from main application thread before running any localized code.
		\remarks Having other localization-dependent threads running while this function is being called
				 will not crash the system, but will not guarantee correct localized string loading until
				 the function returns.
	    \attention Calling more than one instance of this function from different threads may cause unpredictable
				   behavior.
	*/
	ActionStatus InitializeLNGBasedTranslationEngine(LANGID PreferredLangID = 0, const String &LNGDirectory = _T("langfiles"), HMODULE hMainModule = GetModuleHandle(0));
	
	//! Unloads all previously allocated language tables and frees the translation engine
	/*! This function should be called from main application thread when the application is about
		to exit and all other threads using the localized strings have exited.
		\attention Calling this function while another thread using localization API is running
				   may lead to a crash.
	*/
	void ShutdownTranslationEngine();

	struct InstalledLanguage
	{
		String LangName;
		String LangNameEng;
		unsigned LangID;
	};

	std::vector<InstalledLanguage> GetInstalledLanguageList();

	//! Returns currently selected language ID, or zero if default built-in strings are used.
	LANGID GetCurrentLanguageId();


	//! Selects another language from the installed language list
	/*! This function can be called from an arbitrary thread at the arbitrary time.
		\remarks To ensure that the previously obtained string pointers to localized strings
				 remain valid after this function call, the previous language's string table
				 will remain in memory until the ShutdownTranslationEngine() is called.
				 This ensures correct multi-thread operation and eliminates the need of copying
				 localized strings to thread-provided buffers.
	*/
	ActionStatus SelectLanguage(LANGID LangID);

	//! Contains various internal stuff used by localization system
	/*!	See \ref i18n_sys "this page" for more info about the localization system.
	*/
	namespace TranslationSystem
	{
		class StringTable
		{
		private:
			TCHAR *m_pBuffer;

		public:
			size_t IDCount;
			size_t FirstID;
			TCHAR **ppStrings;
			LANGID LangID;

		public:
			StringTable(TCHAR *pBuffer, TCHAR **strings, size_t StringCount, size_t firstID)
				: m_pBuffer(pBuffer)
				, IDCount(StringCount)
				, FirstID(firstID)
				, ppStrings(strings)
				, LangID(0)
			{
			}

			~StringTable()
			{
				delete m_pBuffer;
				delete ppStrings;
			}

			TCHAR *GetStringBuffer()
			{
				return m_pBuffer;
			}
		};

		extern StringTable *g_pCurrentLanguageStringTable;


		static inline void LocalizeDlgItem(unsigned StringID, unsigned ItemID, HWND hWnd)
		{
			const TCHAR *pString = LookupLocalizedString(StringID);
			if (pString)
				SetDlgItemText(hWnd, ItemID, pString);
		}

		static inline void LocalizeDialog(unsigned StringID, unsigned, HWND hWnd)
		{
			const TCHAR *pString = LookupLocalizedString(StringID);
			if (pString)
				SetWindowText(hWnd, pString);
		}
	}

#ifdef BAZISLIB_LOCALIZATION_ENABLED
#define LOCALIZE_DLGITEM(strid, item, hwnd)	BazisLib::TranslationSystem::LocalizeDlgItem(strid, item, hwnd)
#define LOCALIZE_DIALOG(strid, dlgid, hwnd) BazisLib::TranslationSystem::LocalizeDialog(strid, dlgid, hwnd)
#endif

	static inline const TCHAR *LookupLocalizedString(unsigned ID)
	{
		//Even if g_pCurrentLanguageStringTable is changed by SelectLanguage(), the previous table
		//remains present in memory until the translation engine is shut down. This allows safely
		//looking up strings from the table without any further synchronization.
		TranslationSystem::StringTable *pTable = TranslationSystem::g_pCurrentLanguageStringTable;
		if (!pTable)
			return NULL;
		size_t offset = pTable->FirstID;
		if (ID < offset)
			return NULL;
		if ((ID - offset) >= pTable->IDCount)
			return NULL;
		return pTable->ppStrings[ID - offset];
	}

}