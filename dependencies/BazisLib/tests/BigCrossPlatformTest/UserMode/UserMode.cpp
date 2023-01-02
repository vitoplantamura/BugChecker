// UserMode.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <bzscore/bzscore.h>
#include <bzscore/status.h>
#include <bzscore/atomic.h>
#include <bzscore/sync.h>
#include <bzscore/objmgr.h>
#include <bzscore/datetime.h>

using namespace BazisLib;

bool AutoTestEntry(bool runTestsCausingMemoryLeaks, const TCHAR *pTempDir);

void TestPrint(const char *pFormat, ...)
{
	va_list lst;
	va_start(lst, pFormat);
	vprintf(pFormat, lst);
	va_end(lst);
} 

#include <bzscore/path.h>

int main(int argc, char* argv[])
{
	String tmp = Path::GetSpecialDirectoryLocation(dirTemp);
	if (!AutoTestEntry(false, tmp.c_str()))
	{
		printf("Automatic tests failed!\n");
		return 1;
	}
	
	printf("Automatic tests succeeded!\n");
	return 0;
}
