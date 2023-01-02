#include "SymbolFile.h"

#include "Utils.h"
#include "Symbols.h"

#include <EASTL/string.h>

SymbolFile::SymbolFile(const char* filename)
{
	eastl::wstring ws = L"\\SystemRoot\\BugChecker\\";
	eastl::transform(filename, filename + ::strlen(filename), eastl::back_inserter(ws), [](const char c) { // BCS file names in "BugChecker.dat" are always in ASCII format.
		if (static_cast<BYTE>(c) & 0x80)
			return L'?';
		else
			return (wchar_t)c;
	});

	m_contents.reset(Utils::LoadFileAsByteArray(ws.c_str(), NULL));
}

BOOLEAN SymbolFile::IsValid()
{
	if (!m_contents)
		return FALSE;

	BCSFILE_HEADER* header = (BCSFILE_HEADER*)m_contents.get();

	return Symbols::IsBcsValid(header);
}

BCSFILE_HEADER* SymbolFile::GetHeader()
{
	if (!IsValid())
		return NULL;

	return (BCSFILE_HEADER*)m_contents.get();
}
