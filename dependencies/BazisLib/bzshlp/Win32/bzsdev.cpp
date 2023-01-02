#include "stdafx.h"

#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "bzsdev.h"

#pragma comment(lib, "setupapi")

using namespace BazisLib;

/*namespace
{
	typedef bool (*PDEVENUMCB)();

	void IterateDevices(const GUID *pguidInterfaceType, PDEVENUMCB pCB, void *pContext)
	{
	}
}*/

std::list<String> BazisLib::EnumerateDevicesByInterface(const GUID *pguidInterfaceType, PDEVICE_SN_FILTER Filter, void *pContext)
{
	std::list<String> devices;
	if (!pguidInterfaceType)
		return devices;
	DeviceInformationSet devs = DeviceInformationSet::GetLocalDevices(pguidInterfaceType);
	for (DeviceInformationSet::iterator it = devs.begin(); it != devs.end(); it++)
	{
		String str(it.GetInstanceId());
		size_t idx = str.rfind('\\');
		bool bSkip = false;
		if (idx != -1)
			str = str.substr(idx + 1);
		else
			str = _T("");

		if (Filter)
			if (!Filter(str, pContext))
				bSkip = true;

		if (!bSkip && it.GetDevicePath())
			devices.push_back(it.GetDevicePath());
	}
	return devices;
}

std::list<String> BazisLib::EnumerateDevicesByHardwareID(const String &ID)
{
	std::list<String> devices;
	DeviceInformationSet devs = DeviceInformationSet::GetLocalDevices();
	for (DeviceInformationSet::iterator it = devs.begin(); it != devs.end(); it++)
	{
		if (it.GetDeviceRegistryProperty(SPDRP_HARDWAREID) == ID)
			devices.push_back(it.GetInstanceId());
	}
	return devices;
}

namespace
{
	class DevinfoReleaser
	{
		HDEVINFO _hDevInfo;

	public:
		DevinfoReleaser(HDEVINFO hInfo)
			: _hDevInfo(hInfo)
		{
		}

		~DevinfoReleaser()
		{
			SetupDiDestroyDeviceInfoList(_hDevInfo);
		}
	};
}

ActionStatus BazisLib::AddRootEnumeratedNode(const INFClass &Class, LPCTSTR HardwareId)
{
	if (!HardwareId)
		return MAKE_STATUS(InvalidParameter);
    HDEVINFO deviceInfoSet = SetupDiCreateDeviceInfoList(Class.GetGuid(),0);

    SP_DEVINFO_DATA deviceInfoData;
    if(deviceInfoSet == INVALID_HANDLE_VALUE) 
		return MAKE_STATUS(ActionStatus::FailFromLastError());

	DevinfoReleaser releaser(deviceInfoSet);

    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(deviceInfoSet,
        Class.GetName(),
        Class.GetGuid(),
        NULL,
        0,
        DICD_GENERATE_ID,
        &deviceInfoData))
		return MAKE_STATUS(ActionStatus::FailFromLastError());

	TCHAR tszHardwareId[MAX_DEVICE_ID_LEN] = {0,};
	_tcsncpy(tszHardwareId, HardwareId, (sizeof(tszHardwareId)/sizeof(tszHardwareId[0])) - 2);

    if(!SetupDiSetDeviceRegistryProperty(deviceInfoSet,
        &deviceInfoData,
        SPDRP_HARDWAREID,
        (LPBYTE)tszHardwareId,
        (DWORD)((_tcslen(tszHardwareId)+1+1)*sizeof(TCHAR)))) 
		return MAKE_STATUS(ActionStatus::FailFromLastError());

    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
        deviceInfoSet,
        &deviceInfoData))
		return MAKE_STATUS(ActionStatus::FailFromLastError());
	
	return MAKE_STATUS(Success);
}
#endif