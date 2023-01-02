#pragma once

#include "BugChecker.h"

#include "Symbols.h"

#include <EASTL/unique_ptr.h>

class SymbolFile
{
public:

	SymbolFile(const char* filename);

	BOOLEAN IsValid();
	BCSFILE_HEADER* GetHeader();

private:

	eastl::unique_ptr<BYTE[]> m_contents;
};
