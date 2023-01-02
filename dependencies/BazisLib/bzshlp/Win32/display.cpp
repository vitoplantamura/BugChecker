#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "display.h"
#include "../algorithms/memalgs.h"

using namespace BazisLib;
using namespace BazisLib::Win32;
using namespace BazisLib::Algorithms;

std::vector<Display> BazisLib::Win32::Display::EnumerateDiplays(DisplayEnumFilterKind filter /*= dfAll*/)
{
	std::vector<Display> result;
	for (DWORD i = 0; ; i++)
	{
		DISPLAY_DEVICE devInfo = {sizeof(DISPLAY_DEVICE), };
		if (!EnumDisplayDevices(NULL, i, &devInfo, 0))
			break;

		if (filter != dfAll)
		{
			if (filter & dfPrimaryOnly)
			{
				if (!(devInfo.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
					continue;
			}
			else
			{
				if (!(filter & dfConnected) && (devInfo.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
					continue;
				if (!(filter & dfDisconnected) && !(devInfo.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
					continue;
				if (!(filter & dfMirrors) && (devInfo.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
					continue;
				if (!(filter & dfNonMirrors) && !(devInfo.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
					continue;
			}
		}
		result.push_back(Display(devInfo));
	}

	return result;
}

BazisLib::Win32::Display::ModeCollection BazisLib::Win32::Display::GetAllSupportedModes()
{
	ModeCollection modes;
	for (DWORD i = 0; ; i++)
	{
		Mode mode(_Device.DeviceName, i);
		if (!mode.Valid())
			break;
		modes.push_back(mode);
	}

	return modes;
}

BazisLib::ActionStatus BazisLib::Win32::Display::Disconnect()
{
	DEVMODE mode = {0,};
	mode.dmSize = sizeof(mode);
	mode.dmDisplayFlags =  DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_POSITION | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;
	int result = ChangeDisplaySettingsEx(_Device.DeviceName, &mode, NULL, CDS_UPDATEREGISTRY, NULL);
	if (result == DISP_CHANGE_SUCCESSFUL)
		return MAKE_STATUS(Success);
	else
		return MAKE_STATUS(UnknownError);
}

BazisLib::Win32::Display::Mode::Mode(LPCTSTR lpDevName, DWORD iModeNum)
{
	_DevMode.dmSize = sizeof(_DevMode);
	_DevMode.dmDriverExtra = 0;
	ZeroStruct(_DevMode);

	if (!EnumDisplaySettings(lpDevName, iModeNum, &_DevMode))
		_DevMode.dmSize = 0;
}
#endif