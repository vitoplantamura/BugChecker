#pragma once

#include "BugChecker.h"

class ProcAddress
{
public:
	static PVOID GetModuleBase(const CHAR* pModuleName);
	static PVOID GetProcAddress(PVOID ModuleBase, const CHAR* pFunctionName);
};
