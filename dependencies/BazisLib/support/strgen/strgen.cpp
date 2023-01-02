// strgen.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <bzscore/string.h>
#include <list>
#include <bzscore/file.h>
#include <bzshlp/algorithms/filealgs.h>
#include "StringManager.h"

int DisplayHelp()
{
	printf("STRGEN.EXE - Generates string ID lists for localizable BazisLib-based apps\n");
	printf("Usage: strgen [-srcfiles:file1.cpp,file2.h,...]\n");
	printf("              [-rcfiles:file1.rc,file2.rc,...]\n");
	printf("              [-outhdr:strings.h]\n");
	printf("              [-outtbl:strings.cpp]\n");
	printf("              [-outlng:default.lng]\n");
	return 0;
}

using namespace BazisLib;

struct CommandLineParameters
{
	std::list<String> SourceFileList;
	std::list<String> RcFileList;

	String GeneratedHeaderFile;
	String GeneratedLNGFile;
	String GeneratedTableFile;
};

#include <bzscore/strfast.h>

int _tmain(int argc, _TCHAR* argv[])
{
	if ((argc == 2) && (!_tcsicmp(argv[1], _T("/?"))))
		return DisplayHelp();

	CommandLineParameters params;

	//Parse command-line args
	for (int i = 1; i < argc; i++)
	{
		String arg(argv[i]);
		_FixedCharacterSplitString<2, TempString> split(arg, ':');
		if (split.count() != 2)
			continue;
		TempString key = split[0];
		if (key.empty())
			continue;
		if ((key[0] != '-') && (key[0] != '/'))
			continue;
		key = key.substr(1);
		if (!key.icompare(_T("srcfiles")))
		{
			for each(const TempString &fp in split[1].SplitByMarker(','))
				params.SourceFileList.push_back(fp);
		}
		else if (!key.icompare(_T("rcfiles")))
		{
			for each(const TempString &fp in split[1].SplitByMarker(','))
				params.RcFileList.push_back(fp);
		}
		else if (!key.icompare(_T("outhdr")))
			params.GeneratedHeaderFile = split[1];
		else if (!key.icompare(_T("outlng")))
			params.GeneratedLNGFile = split[1];
		else if (!key.icompare(_T("outtbl")))
			params.GeneratedTableFile = split[1];
	}

	//Provide defaults for unspecified params
	String currentPath = BazisLib::Path::GetCurrentPath();
	
	if (params.GeneratedHeaderFile.empty())
		params.GeneratedHeaderFile = Path::GetFileNameWithoutExtension(currentPath) + _T("_lng.h");
	if (params.GeneratedLNGFile.empty())
		params.GeneratedLNGFile = Path::GetFileNameWithoutExtension(currentPath) + _T(".lng");
	if (params.GeneratedTableFile.empty())
		params.GeneratedTableFile = Path::GetFileNameWithoutExtension(currentPath) + _T("_lng.cpp");
	
	if (params.RcFileList.empty())
		params.RcFileList = Algorithms::EnumerateFilesRecursively<std::list<String>>(currentPath, _T("*.rc"));

	if (params.SourceFileList.empty())
	{
		Algorithms::DoEnumerateFilesRecursively(currentPath, &params.SourceFileList, _T("*.h"));
		Algorithms::DoEnumerateFilesRecursively(currentPath, &params.SourceFileList, _T("*.c"));
		Algorithms::DoEnumerateFilesRecursively(currentPath, &params.SourceFileList, _T("*.cpp"));
	}

	LocalizableStringManager stringMgr;

	printf("Parsing resource files...\n");
	for each(const String &fp in params.RcFileList)
		if (!stringMgr.ParseResourceFile(fp))
		{
			printf("Failed to parse resource files\n");
			return 1;
		}
	printf("Parsing source files...\n");
	for each(const String &fp in params.SourceFileList)
		if (!stringMgr.ParseSourceFile(fp))
		{
			printf("Failed to parse source files\n");
			return 1;
		}
	printf("Checking coverage...\n");
	stringMgr.FindAndDisplayStructuralWarnings();

	_tprintf(_T("Generating %s...\n"), params.GeneratedHeaderFile.c_str());
	if (!stringMgr.ProduceHeaderFile(params.GeneratedHeaderFile))
		return 1;

	_tprintf(_T("Generating %s...\n"), params.GeneratedLNGFile.c_str());
	if (!stringMgr.ProduceLanguageFile(params.GeneratedLNGFile))
		return 1;
	
	_tprintf(_T("Generating %s...\n"), params.GeneratedTableFile.c_str());
	if (!stringMgr.ProduceIDTableFile(params.GeneratedTableFile, Path::GetFileName(params.GeneratedHeaderFile)))
		return 1;

	printf("Finished generating localization files.\n");

	return 0;
}

