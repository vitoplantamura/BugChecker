//
// Device and Driver Installation API requires to be called from an architecture native EXE,
// i.e. Wow64 calls are rejected. For this reason, the calls to this API were moved here,
// in this command line utility.
//

#define IGNORE_BC_MODIFICATIONS_TO_EASTL
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#include <windows.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <devguid.h>

#include <EASTL/functional.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>

//
// >>> we use EASTL instead of the default MS STL, because the latter doesn't work
// on Windows XP. For example, std::vector<int> requires InitializeCriticalSectionEx
// which is not available on XP.
// 
// We only use the templates from EASTL, without any implementation file:
// 

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	return ::operator new(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	return ::operator new(size);
}

namespace eastl
{
	EASTL_API void AssertionFailure(const char* pExpression)
	{
	}

	EASTL_API allocator gDefaultAllocator;

	EASTL_API allocator* GetDefaultAllocator()
	{
		return &gDefaultAllocator;
	}
}

// <<<

//
// Quick and simple scope guard class.
//

class scope_guard {
public:
	template<class Callable>
	scope_guard(Callable&& undo_func) : f(std::forward<Callable>(undo_func)) { }

	~scope_guard() {
		if (f) f();
	}

	scope_guard() = delete;
	scope_guard(const scope_guard&) = delete;
	scope_guard& operator = (const scope_guard&) = delete;
	scope_guard(scope_guard&&) = delete;
	scope_guard& operator = (scope_guard&&) = delete;

private:
	eastl::function<void()> f;
};

//
// WStringToAscii function.
//

eastl::string WStringToAscii(const eastl::wstring& str)
{
	eastl::string ret;

	eastl::transform(str.begin(), str.end(), eastl::back_inserter(ret), [](const wchar_t c) {
		if (static_cast<USHORT>(c) & 0b1111111110000000)
			return '?';
		else
			return (CHAR)c;
	});

	return ret;
}

//
// PrintDisplayMemRanges function.
//

void PrintDisplayMemRanges(FILE* file)
{
	// create a "device information set".

	HDEVINFO hDevInfoSet = ::SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, NULL, NULL, DIGCF_PRESENT);
	if (hDevInfoSet == INVALID_HANDLE_VALUE)
		return;

	scope_guard sg01([&]() {

		// free the "device information set".

		::SetupDiDestroyDeviceInfoList(hDevInfoSet);

	});

	// do the enumeration.

	int nIndex = 0;

	while (TRUE)
	{
		// get the current device.

		SP_DEVINFO_DATA devInfo{};
		devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
		if (!::SetupDiEnumDeviceInfo(hDevInfoSet, nIndex, &devInfo))
			break;
		else
			++nIndex;

		// get the device name.

		DWORD dwType = 0;
		DWORD dwSize = 0;

		if (!::SetupDiGetDeviceRegistryPropertyW(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType, NULL, 0, &dwSize) && ::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			continue;

		eastl::vector<BYTE> nameBuffer(dwSize);

		if (!::SetupDiGetDeviceRegistryPropertyW(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType, nameBuffer.data(), dwSize, &dwSize) || dwType != REG_SZ)
			continue;

		eastl::wstring name = reinterpret_cast<const wchar_t*>(nameBuffer.data());

		::fprintf(file, "%s\n", ::WStringToAscii(name).c_str());

		// get the first logical configuration.

		LOG_CONF firstLogConf;
		CONFIGRET cmRet;

		cmRet = ::CM_Get_First_Log_Conf(&firstLogConf, devInfo.DevInst, ALLOC_LOG_CONF);
		if (cmRet != CR_SUCCESS)
		{
			cmRet = ::CM_Get_First_Log_Conf(&firstLogConf, devInfo.DevInst, BOOT_LOG_CONF);
			if (cmRet != CR_SUCCESS)
				continue;
		};

		// iterate through all the resource descriptors.

		eastl::vector<RES_DES> resDesList;
		resDesList.push_back(firstLogConf);

		scope_guard sg02([&]() {

			// free the resource descriptors.
			for (size_t i = 1; i < resDesList.size(); i++)
				::CM_Free_Res_Des_Handle(resDesList[i]);

			// free the first logical config.
			::CM_Free_Log_Conf_Handle(resDesList[0]);

		});

		while (TRUE)
		{
			// get the next resource descriptor.

			RES_DES resDes;

			cmRet = ::CM_Get_Next_Res_Des(&resDes,
				resDesList.back(),
				ResType_Mem,
				0,
				0);

			if (cmRet != CR_SUCCESS)
				break;
			else
				resDesList.push_back(resDes);

			// read the resource descriptor data.

			ULONG ulSize = 0;

			cmRet = ::CM_Get_Res_Des_Data_Size(&ulSize, resDes, 0);
			if (cmRet != CR_SUCCESS || !ulSize)
				break;

			eastl::vector<BYTE> rdBuffer(ulSize);

			cmRet = ::CM_Get_Res_Des_Data(resDes, rdBuffer.data(), ulSize, 0);
			if (cmRet != CR_SUCCESS)
				break;

			// print the data.

			MEM_DES* pMemDes = reinterpret_cast<MEM_DES*>(rdBuffer.data());

			::fprintf(file, "range:%016llX-%016llX\n", pMemDes->MD_Alloc_Base, pMemDes->MD_Alloc_End);
		}
	}
}

int EnableAllDisplayDevices(BOOL enable)
{
	int retVal = 0;

	// create a "device information set".

	HDEVINFO hDevInfoSet = ::SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, NULL, NULL, DIGCF_PRESENT);
	if (hDevInfoSet == INVALID_HANDLE_VALUE)
		return retVal;

	scope_guard sg01([&]() {

		// free the "device information set".

		::SetupDiDestroyDeviceInfoList(hDevInfoSet);

	});

	// do the enumeration.

	int nIndex = 0;

	while (TRUE)
	{
		// get the current device.

		SP_DEVINFO_DATA devInfo{};
		devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
		if (!::SetupDiEnumDeviceInfo(hDevInfoSet, nIndex, &devInfo))
			break;
		else
			++nIndex;

		// set the class install parameter for the device.

		SP_PROPCHANGE_PARAMS spPropChangeParams = {};

		spPropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
		spPropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
		spPropChangeParams.Scope = DICS_FLAG_GLOBAL;
		spPropChangeParams.StateChange = enable ? DICS_ENABLE : DICS_DISABLE;

		if (::SetupDiSetClassInstallParamsW(hDevInfoSet, &devInfo,
			(SP_CLASSINSTALL_HEADER*)&spPropChangeParams, sizeof(spPropChangeParams)))
		{
			if (::SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfoSet, &devInfo))
			{
				// check if a restart is required.

				SP_DEVINSTALL_PARAMS_W spDevInstallParamsW = {};
				spDevInstallParamsW.cbSize = sizeof(spDevInstallParamsW);

				if (::SetupDiGetDeviceInstallParamsW(hDevInfoSet, &devInfo, &spDevInstallParamsW))
				{
					if ((spDevInstallParamsW.Flags & DI_NEEDREBOOT) || (spDevInstallParamsW.Flags & DI_NEEDRESTART))
						retVal = 1;
				}
			}
		}
	}

	// return.

	return retVal;
}

//
// main function.
//

int wmain(int argc, wchar_t** argv)
{
	if (argc != 2)
		return 0;

	if (!::wcscmp(argv[1], L"-save-display-resources"))
	{
		::printf("Saving all display physical address resources in \"resources.txt\".");

		auto file = ::fopen("resources.txt", "wt");
		if (file)
		{
			::PrintDisplayMemRanges(file);
			::fclose(file);
		}
	}
	else if (!::wcscmp(argv[1], L"-enable-all-displays"))
	{
		::printf("Enabling all display devices.");

		auto ret = ::EnableAllDisplayDevices(TRUE);

		if (ret == 1) ::printf("System reboot required.");

		return ret;
	}
	else if (!::wcscmp(argv[1], L"-disable-all-displays"))
	{
		::printf("Disabling all display devices.");

		auto ret = ::EnableAllDisplayDevices(FALSE);

		if (ret == 1) ::printf("System reboot required.");

		return ret;
	}

	return 0;
}
