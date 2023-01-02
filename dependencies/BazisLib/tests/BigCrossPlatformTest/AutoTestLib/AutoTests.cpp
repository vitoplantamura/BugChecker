#include "stdafx.h"
#include <bzscore/string.h>

bool ObjectManagerTestEntry(bool runTestsCausingMemoryLeaks);
bool TestPathAPI();
bool TestFileAPI(const BazisLib::String &tempPath);
bool TestThreadAPI();
bool TestConfigurationStorageAPI();

bool TestAtomicAPI();
bool TestSyncAPI();

bool TestSocketAPI();

bool AutoTestEntry(bool runTestsCausingMemoryLeaks, const TCHAR *pTempDir)
{
	if (!TestPathAPI())
		return false;
	if (!TestAtomicAPI())
		return false;
	if (!ObjectManagerTestEntry(runTestsCausingMemoryLeaks))
		return false;
	if (!TestConfigurationStorageAPI())
		return false;
	if (!TestFileAPI(pTempDir))
		return false;
	if (!TestThreadAPI())
		return false;
	if (!TestSyncAPI())
		return false;
	if (!TestSocketAPI())
		return false;

	return true;
}