#include "stdafx.h"
#include "StringManager.h"

#include <bzscore/file.h>
#include <bzshlp/textfile.h>
#include <bzscore/strbase.h>

using namespace BazisLib;

BazisLib::String GetFullPath(const BazisLib::String &path)
{
	return path;
}

bool LocalizableStringManager::ParseResourceFile( const BazisLib::String &fp )
{
	ManagedPointer<TextANSIFileReader> pRdr = new TextANSIFileReader(new ACFile(fp, FileModes::OpenReadOnly));
	if (!pRdr->Valid())
	{
		_tprintf(_T("ERROR: cannot open %s\n"), fp.c_str());
		return false;
	}
	_tprintf(_T("  %s\n"), DynamicString(Path::GetFileName((fp))).c_str());

	DynamicString dialogName;
	bool insideDialogDescription = false;
	size_t spacingBeforeDlgitem = -1;

	unsigned lineNum = 0;

	while (!pRdr->IsEOF())
	{
		lineNum++;
		DynamicStringA line = pRdr->ReadLine();
		_FixedCharacterSplitString<2, TempStringA> tokens(line, ' ');
		if (!tokens.count())
			continue;
		if (!tokens[0].icompare("BEGIN"))
			insideDialogDescription = true;
		else if (!tokens[0].icompare("END"))
			dialogName.clear(), insideDialogDescription = false;
		else if (!tokens[1].icompare("DIALOGEX"))
		{
			dialogName = ANSIStringToString(tokens[0]), insideDialogDescription = false;
			m_Dialogs[dialogName].FileAndLine.Format(_T("%s(%d)"), GetFullPath(fp).c_str(), lineNum);
		}
		else if (!line.substr(0, 8).icompare("CAPTION ") && !dialogName.empty() && !insideDialogDescription)
		{
			TempStringA dlgCaption = line.substr(9);
			size_t lastQuote = dlgCaption.find_last_of('\"');
			if (lastQuote != -1)
				dlgCaption = dlgCaption.substr(0, lastQuote);

			DynamicString translatedCaption = ANSIStringToString(dlgCaption);
			for (size_t idx = 0; idx < (translatedCaption.length() - 1); idx++)
			{
				if ((translatedCaption[idx] == '\"') && (translatedCaption[idx + 1] == '\"'))
					translatedCaption.erase(idx, 1);
			}
			m_Dialogs[dialogName].Caption = FormatStringASCString(translatedCaption);
		}
		else if (insideDialogDescription && !dialogName.empty())
		{
			if (spacingBeforeDlgitem == -1)
				spacingBeforeDlgitem = line.find_first_not_of(" \t");
			if (spacingBeforeDlgitem == -1)
				continue;

			if ((line.length() <= spacingBeforeDlgitem) || (line[spacingBeforeDlgitem] == ' ') || (line[spacingBeforeDlgitem] == '\t'))
				continue;

			_FixedSetOfCharsSplitString<1, TempStringA> tokens2(line.substr(spacingBeforeDlgitem), " \t");
			if (tokens2.count() < 1)
				continue;

			if (!tokens2[0].length())
				continue;

			size_t nameStart = tokens2.GetRemainderOffset();
			if (nameStart == -1)
				continue;

			nameStart = line.find_first_not_of(" \t", nameStart + spacingBeforeDlgitem);

			if ((nameStart == -1) || (line[nameStart] != '\"'))
				continue;

			nameStart++;

			char prevChar = 0;
			size_t nameEnd;
			for (nameEnd = nameStart; nameEnd < line.length(); nameEnd++)
			{
				if (line[nameEnd] == '\"')
				{
					if (((nameEnd + 1) < line.length()) && (line[nameEnd + 1] == '\"'))
					{
						nameEnd++;
						continue;
					}
					else
						break;
				}
			}

			DynamicStringA itemText = line.substr(nameStart, nameEnd - nameStart);

			for (size_t idx = 0; (idx + 1) < itemText.length(); idx++)
			{
				if ((itemText[idx] == '\"') && (itemText[idx + 1] == '\"'))
					itemText.erase(idx, 1);
			}


			size_t IDOffset = line.find_first_not_of(" \t,", nameEnd + 1);
			if (IDOffset == -1)
				continue;
			size_t IDEnd = line.find_first_of(" \t,", IDOffset);

			TempStringA itemID = line.substr(IDOffset, (IDEnd == -1) ? -1 : IDEnd - IDOffset);

			m_Dialogs[dialogName].DialogMembers[ANSIStringToString(itemID)].Name = FormatStringASCString(ANSIStringToString(itemText));
			
		}
	}
	return true;
}

static inline bool IsValidCTokenChar(char ch)
{
	if ((ch >= 'a') && (ch <= 'z'))
		return true;
	if ((ch >= 'A') && (ch <= 'Z'))
		return true;
	if ((ch >= '0') && (ch <= '9'))
		return true;
	if (ch == '_')
		return true;
	return false;
}

bool LocalizableStringManager::ParseSourceFile( const BazisLib::String &fp )
{
	ManagedPointer<TextANSIFileReader> pRdr = new TextANSIFileReader(new ACFile(fp, FileModes::OpenReadOnly));
	if (!pRdr->Valid())
	{
		_tprintf(_T("ERROR: cannot open %s\n"), fp.c_str());
		return false;
	}
	_tprintf(_T("  %s\n"), Path::GetFileName(fp).c_str());

	bool insideCCommentBlock = false;
	bool insideStringConstant = false;

	unsigned lineNum = 0;

	DynamicString currentDialogID;

	while (!pRdr->IsEOF())
	{
		size_t LocalizationTokenStart = -1;
		enum
		{
			ltUnknown = 0,
			ltTR,
			ltDIALOG,
			ltDLGITEM,
		} localizationTokenType;

		lineNum++;
		DynamicStringA line = pRdr->ReadLine();

		unsigned backslashCount = 0;
		char prevChar = 0, ch = 0;

		size_t lastStringStart = -1, lastStringLen = 0;

		for (size_t i = 0; i < line.length(); i++, prevChar = ch)
		{
			//Even amount of backslashes followed by quotation mark toggles string constant flag (outside comment block)
			//The "/*" and "*/" sequences outside string block change the comment flag.
			//The "//" sequence skips everything till the end of the string
			//At the end of each line inside the string block there should be a backslash
			ch = line[i];

			bool isLastChar = (i == (line.length() - 1));

			if (insideCCommentBlock)
			{
				if ((ch == '/') && (prevChar == '*'))
					insideCCommentBlock = false;
			}
			else	//Outside comment block - detect string literals
			{
				if (ch == '\\')
					backslashCount++;
				else 
				{
					if (((ch == '\"') || (ch == '\'')) && !(backslashCount % 2))
					{
						if (!insideStringConstant)
							lastStringStart = i + 1, lastStringLen = 0;
						else
							lastStringLen = i - lastStringStart;

						insideStringConstant = !insideStringConstant;
					}
					backslashCount = 0;
				}

				if (!insideStringConstant)	//Outside comment and string blocks - check for comment start
				{
					if ((ch == '*') && (prevChar == '/'))
						insideCCommentBlock = true;
					if ((ch == '/') && (prevChar == '/'))
						break;
				}
				else	//Inside string block
				{
					if (isLastChar && (ch != '\\'))
					{
						_tprintf(_T("%s(%d) : error: Unterminated string"), GetFullPath(fp).c_str(), lineNum);
						return false;
					}
				}
			}

			//Here, both insideCCommentBlock and insideStringConstant reflect the real file context. So, let's find "_TR()" and LOCALIZE_DIALOG()/LOCALIZE_DLGITEM()
			if (!insideCCommentBlock && !insideStringConstant)
			{
				if (!IsValidCTokenChar(prevChar))
				{
					size_t newTokenStart = -1;
					size_t remaining = line.length() - i;
					if ((remaining > 4) && (line.substr(i, 3) == "_TR") && !IsValidCTokenChar(line[i + 3]))
						newTokenStart = i + 3, localizationTokenType = ltTR;
					else if ((remaining > 16) && (line.substr(i, 15) == "LOCALIZE_DIALOG") && !IsValidCTokenChar(line[i + 15]))
						newTokenStart = i + 15, localizationTokenType = ltDIALOG;
					else if ((remaining > 17) && (line.substr(i, 16) == "LOCALIZE_DLGITEM") && !IsValidCTokenChar(line[i + 16]))
						newTokenStart = i + 16, localizationTokenType = ltDLGITEM;

					if (newTokenStart != -1)
					{
						if (LocalizationTokenStart != -1)
						{
							_tprintf(_T("%s(%d) : error: Localization token used recursively"), GetFullPath(fp).c_str(), lineNum);
							return false;
						}
						LocalizationTokenStart = newTokenStart;
						
						lastStringStart = -1;
						lastStringLen = 0;
					}

				}
				if (ch == '}')
					currentDialogID.clear();

				if ((ch == ')') && (LocalizationTokenStart != -1))
				{
					size_t tokenParamsStart = line.find_first_not_of(" \t()", LocalizationTokenStart);
					TempStringA tokenParams = line.substr(tokenParamsStart, i - LocalizationTokenStart);
					if (tokenParams.empty())
					{
						_tprintf(_T("%s(%d) : error: Empty localization macro"), GetFullPath(fp).c_str(), lineNum);
						return false;
					}
					if (localizationTokenType == ltTR)
					{
						if ((lastStringStart == -1) || !lastStringLen)
						{
							_tprintf(_T("%s(%d) : error: Invalid _TR() statement - default string not found"), GetFullPath(fp).c_str(), lineNum);
							return false;
						}
						DynamicString lastString = ANSIStringToString(line.substr(lastStringStart, lastStringLen));

						size_t idEnd = tokenParams.find_first_of(" ,");
						DynamicString id = ANSIStringToString(tokenParams.substr(0, (idEnd == -1) ? 0 : idEnd));
						if (id.empty())
						{
							_tprintf(_T("%s(%d) : error: Invalid _TR() statement - cannot determine string ID"), GetFullPath(fp).c_str(), lineNum);
							return false;
						}

						const DynamicString &existingValue = m_Strings[id].Value;
						if (!existingValue.empty() && (existingValue != lastString))
							_tprintf(_T("%s(%d) : warning: %s redefined with different value\n"), GetFullPath(fp).c_str(), lineNum, id.c_str());

						m_Strings[id].Value = lastString;
					}
					else
					{
						_FixedSetOfCharsSplitString<2, TempStringA> params(tokenParams, ", \t");
						if (params.count() < 2)
						{
							_tprintf(_T("%s(%d) : error: Invalid LOCALIZE_xxx() statement - less than 2 arguments\n"), GetFullPath(fp).c_str(), lineNum);
							return false;
						}

						DynamicString stringId = ANSIStringToString(params[0]);
						if (stringId.empty())
						{
							_tprintf(_T("%s(%d) : error: Invalid LOCALIZE_xxx() statement - empty string ID\n"), GetFullPath(fp).c_str(), lineNum);
							return false;
						}

						if (localizationTokenType == ltDIALOG)
						{
							DynamicString dlgID = ANSIStringToString(params[1]);
							if (m_Dialogs.find(dlgID) == m_Dialogs.end())
							{
								_tprintf(_T("%s(%d) : error: Cannot find dialog %s\n"), GetFullPath(fp).c_str(), lineNum, dlgID.c_str());
								return false;
							}
							m_Dialogs[dlgID].Localized = true;
							m_Dialogs[dlgID].LocalizationFileAndLine.Format(_T("%s(%d)"), GetFullPath(fp).c_str(), lineNum);
							m_Strings[stringId].Value = m_Dialogs[dlgID].Caption;
							currentDialogID = dlgID;
						}
						else if (localizationTokenType == ltDLGITEM)
						{
							if (currentDialogID.empty())
							{
								_tprintf(_T("%s(%d) : error: LOCALIZE_DLGITEM() used outside LOCALIZE_DIALOG() block\n"), GetFullPath(fp).c_str(), lineNum);
								return false;
							}
							DynamicString itemID = ANSIStringToString(params[1]);
							std::map<BazisLib::String, DialogMember>::iterator it = m_Dialogs[currentDialogID].DialogMembers.find(itemID);
							if ((stringId == _T("0")) || !stringId.icompare(_T("NULL")))
							{
								it->second.Skipped = true;
								continue;
							}
							if (it == m_Dialogs[currentDialogID].DialogMembers.end())
							{
								_tprintf(_T("%s(%d) : error: %s is not defined in %s\n"), GetFullPath(fp).c_str(), lineNum, itemID.c_str(), currentDialogID.c_str());
								return false;
							}
							m_Strings[stringId].Value = it->second.Name;
							it->second.Localized = true;
						}

					}
					LocalizationTokenStart = -1;
				}
			}
		}

	}
	return true;
}

void LocalizableStringManager::FindAndDisplayStructuralWarnings()
{
	for each(const std::map<BazisLib::String, DialogRecord>::value_type &pair in m_Dialogs)
	{
		if (!pair.second.Localized)
			_tprintf(_T("%s : warning: Dialog %s was not localized in any of the source files\n"), pair.second.FileAndLine.c_str(), pair.first.c_str());
		else
		{
			for each(const std::map<BazisLib::String, DialogMember>::value_type &pair2 in pair.second.DialogMembers)
				if (!pair2.second.Localized && !pair2.second.Skipped)
					_tprintf(_T("%s : warning: Dialog item %s was not localized. Use LOCALIZE_DLGITEM(NULL, %s) to mark it as non-localizable.\n"), pair.second.LocalizationFileAndLine.c_str(), pair2.first.c_str(), pair2.first.c_str());

		}
	}
}

bool LocalizableStringManager::ProduceHeaderFile( const BazisLib::String &fp )
{
	File file(fp, FileModes::CreateOrTruncateRW);
	if (!file.Valid())
	{
		_tprintf(_T("ERROR: cannot open %s for writing\n"), fp.c_str());
		return false;
	}
	file.WriteLine(DynamicStringA("#pragma once\r\n//Generated by BazisLib STRGEN.EXE, http://bazislib.sysprogs.org/\r\n\r\n#include<bzshlp/Win32/i18n.h>\r\n#ifndef BAZISLIB_FIRST_LOCALIZABLE_STRING_ID\r\n#define BAZISLIB_FIRST_LOCALIZABLE_STRING_ID 0x1000\r\n#endif\r\n"));
	unsigned id = 0;
	DynamicStringA definitionLine;

	unsigned maxIdLen = 0;

	for each(const std::map<BazisLib::String, StringRecord>::value_type &pair in m_Strings)
		if (pair.first.length() > maxIdLen)
			maxIdLen = pair.first.length();

	DynamicStringA maxSpacing;
	maxSpacing.insert(0, maxIdLen, ' ');

	for each(const std::map<BazisLib::String, StringRecord>::value_type &pair in m_Strings)
	{
		const char *pSpacing = maxSpacing.c_str() + pair.first.length();
		definitionLine.Format("#define %s  %s(BAZISLIB_FIRST_LOCALIZABLE_STRING_ID + 0x%04X)", StringToANSIString(pair.first).c_str(), pSpacing, id++);
		file.WriteLine(definitionLine);
	}

	definitionLine.Format("\r\n#define BAZISLIB_LAST_LOCALIZABLE_STRING_ID \t(BAZISLIB_FIRST_LOCALIZABLE_STRING_ID + 0x%04X)", id - 1);
	file.WriteLine(definitionLine);

	return true;
}

bool LocalizableStringManager::ProduceLanguageFile( const BazisLib::String &fp )
{
	File file(fp, FileModes::CreateOrTruncateRW);
	if (!file.Valid())
	{
		_tprintf(_T("ERROR: cannot open %s for writing\n"), fp.c_str());
		return false;
	}
#ifdef UNICODE
	unsigned short unicodeHeader = 0xFEFF;
	file.Write(&unicodeHeader, sizeof(unicodeHeader));
#endif
	//1033 == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)
	file.WriteLine(DynamicString(_T("; Generated by BazisLib STRGEN.EXE, http://bazislib.sysprogs.org/\r\n; Syntax: <ID> <spaces or tabs> <value>\r\n; <value> should be in C/C++ format (\\r, \\n, \\t, \\\", etc.) \r\n; Strings starting with '; ' will be ignored\r\n; WARNING! This file should always be UNICODE!\r\n")));
	file.WriteLine(DynamicString(_T("[settings]\r\nLanguage       English\r\nLANGID         1033\r\nLanguageEng    English\r\nTranslator     SysProgs\r\nE-Mail         support@sysprogs.org\r\n"))); 
	file.WriteLine(DynamicString(_T("[strings]"))); 

	unsigned maxIdLen = 0;

	for each(const std::map<BazisLib::String, StringRecord>::value_type &pair in m_Strings)
		if (pair.first.length() > maxIdLen)
			maxIdLen = pair.first.length();

	DynamicString definitionLine;
	DynamicString maxSpacing;
	maxSpacing.insert(0, maxIdLen, ' ');

	for each(const std::map<BazisLib::String, StringRecord>::value_type &pair in m_Strings)
	{
		const TCHAR *pSpacing = maxSpacing.c_str() + pair.first.length();
		definitionLine.Format(_T("%s%s   %s"), pair.first.c_str(), pSpacing, pair.second.Value.c_str());
		file.WriteLine(definitionLine);
	}

	return true;
}

BazisLib::String LocalizableStringManager::FormatStringASCString( const BazisLib::TempString &str )
{
	BazisLib::String result;
	result.reserve(str.length());
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == '\\')
			result.append(_T("\\\\"));
		else if (str[i] == '\'')
			result.append(_T("\\\'"));
		else if (str[i] == '\"')
			result.append(_T("\\\""));
		else if (str[i] == '\t')
			result.append(_T("\\\t"));
		else if (str[i] == '\r')
			result.append(_T("\\\r"));
		else if (str[i] == '\n')
			result.append(_T("\\\n"));
		else
			result.append(1, str[i]);
	}
	return result;
}

bool LocalizableStringManager::ProduceIDTableFile( const BazisLib::String &fp, const String &HeaderFileName )
{
	File file(fp, FileModes::CreateOrTruncateRW);
	if (!file.Valid())
	{
		_tprintf(_T("ERROR: cannot open %s for writing\n"), fp.c_str());
		return false;
	}
	DynamicStringA definitionLine;

	file.WriteLine(DynamicStringA("#include \"stdafx.h\"\r\n#include <bzshlp/Win32/i18n.h>"));
	definitionLine.Format("#include \"%s\"", StringToANSIString(HeaderFileName).c_str());
	file.WriteLine(definitionLine);
	file.WriteLine(DynamicStringA("//Generated by BazisLib STRGEN.EXE, http://bazislib.sysprogs.org/\r\n"));
	file.WriteLine(DynamicStringA("BazisLib::StringIDRecord g_StringIDs[] = {"));
	unsigned id = 0;

	unsigned maxIdLen = 0;

	for each(const std::map<BazisLib::String, StringRecord>::value_type &pair in m_Strings)
		if (pair.first.length() > maxIdLen)
			maxIdLen = pair.first.length();

	DynamicStringA maxSpacing;
	maxSpacing.insert(0, maxIdLen, ' ');

	for each(const std::map<BazisLib::String, StringRecord>::value_type &pair in m_Strings)
	{
		const char *pSpacing = maxSpacing.c_str() + pair.first.length();
		definitionLine.Format("\t{%s%s, _T(\"%s\")},", StringToANSIString(pair.first).c_str(), pSpacing, StringToANSIString(pair.first).c_str());
		file.WriteLine(definitionLine);
	}

	file.WriteLine(DynamicStringA("\t{0, NULL},\r\n};"));

	return true;
}