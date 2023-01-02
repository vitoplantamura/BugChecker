#include "stdafx.h"
#include "TestPrint.h"
#include <bzscore/cfgstore.h>

using namespace BazisLib;

bool TestConfigurationStorageAPI()
{
#ifdef BZSLIB_WINKERNEL
	ConfigurationPath root = ConfigurationPath::FromNTRegistryPath(_T("\\Registry\\Machine\\SOFTWARE\\Sysprogs_BazisLibTest"));
#elif defined(WIN32)
	ConfigurationPath root = ConfigurationPath::FromWin32RegistryPath(HKEY_CURRENT_USER, _T("SOFTWARE\\Sysprogs\\BazisLibTest"));
#elif defined (BZSLIB_KEXT)
	ConfigurationPath root = ConfigurationPath::FromFSPath("/tmp/cfgtest.plist");
#else
	ConfigurationPath root = ConfigurationPath::FromFSPath("/tmp/cfgtest");
#endif
	ConfigurationKey rootKey(root, true);
	TEST_ASSERT(rootKey.Valid());

	rootKey[_T("key1")] = _T("val1");
	rootKey[_T("key2")] = 2;

	TEST_ASSERT((String)rootKey[_T("key1")] == _T("val1"));
	TEST_ASSERT((int)rootKey[_T("key2")] == 2);

	ConfigurationKey subkey = rootKey[_T("subkey1")];
	TEST_ASSERT(subkey.Valid());

	TEST_ASSERT((int)subkey[_T("DoesNotExist")] == 0);
	//TEST_ASSERT((int)subkey[_T("key2")] == 0);
	
	subkey[_T("key2")] = 7;
	TEST_ASSERT((int)subkey[_T("key2")] == 7);

	std::vector<String> subkeys = rootKey.GetAllSubkeys();
	TEST_ASSERT(subkeys.size() == 1);
	TEST_ASSERT(subkeys[0] == _T("subkey1"));

#ifdef BZSLIB_KEXT
	TEST_ASSERT(rootKey.Save().Successful());
#endif

	return true;
}
