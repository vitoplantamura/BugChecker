#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "volmgr.h"
#include "../alg.h"
#include <winioctl.h>
#include "bzsdev.h"

using namespace BazisLib;
using namespace BazisLib::Win32;

#ifndef UNDER_CE

VolumeManager::VolumeManager(void)
{
}

VolumeManager::~VolumeManager(void)
{
}

BazisLib::String VolumeManager::GetVolumeNameForGuid(const Guid &guid)
{
	TCHAR tszVolume[MAX_PATH];
	HANDLE hVol = FindFirstVolume(tszVolume, __countof(tszVolume));
	if (hVol == INVALID_HANDLE_VALUE)
		return _T("");
	String strGuid = guid.ToString();
	const TCHAR *ptszGuid = strGuid.c_str();
	do 
	{
		if (_tcsstr(tszVolume, ptszGuid))
			return tszVolume;
	} while (FindNextVolume(hVol, tszVolume, __countof(tszVolume)));
	FindVolumeClose(hVol);
	return _T("");
}

BazisLib::String BazisLib::Win32::VolumeManager::GetVolumeMountPointForGuid( const Guid &guid )
{
	String volName = GetVolumeNameForGuid(guid);
	if (volName.empty())
		return volName;
	TCHAR tszMountPoint[MAX_PATH];
	DWORD dwCount = 0;
	if (!GetVolumePathNamesForVolumeName(volName.c_str(), tszMountPoint, __countof(tszMountPoint), &dwCount))
		return _T("");
	return tszMountPoint;
}

TCHAR BazisLib::Win32::VolumeManager::GetDriveLetterForGuid( const Guid &guid )
{
	TCHAR tszRoot[] = _T("C:\\");
	TCHAR tszVolName[MAX_PATH];
	String strGuid = guid.ToString();
	const TCHAR *ptszGuid = strGuid.c_str();
	for (TCHAR c = 'C'; c <= 'Z'; c++)
	{
		tszRoot[0] = c;
		if (GetVolumeNameForVolumeMountPoint(tszRoot, tszVolName, __countof(tszVolName)))
			if (_tcsstr(tszVolName, ptszGuid))
				return c;
	}
	return 0;
}

BazisLib::ActionStatus BazisLib::Win32::VolumeManager::ChangeVolumeLetter( TCHAR oldLetter, TCHAR newLetter, bool bForce )
{
	TCHAR tszRoot[] = _T("C:\\");
	tszRoot[0] = oldLetter;
	TCHAR tszVolName[MAX_PATH];
	if (GetVolumeNameForVolumeMountPoint(tszRoot, tszVolName, __countof(tszVolName)))
	{
		if (!DeleteVolumeMountPoint(tszRoot))
			return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
	}
	else
	{
		if (bForce)
			return MAKE_STATUS(FileNotFound);
	}
	tszRoot[0] = newLetter;
	if (!SetVolumeMountPoint(tszRoot, tszVolName))
		return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
	return MAKE_STATUS(Success);
}

BazisLib::ActionStatus BazisLib::Win32::VolumeManager::DismountVolume( TCHAR driveLetter )
{
	TCHAR tszRoot[] = _T("\\\\.\\C:");
	tszRoot[4] = driveLetter;
	ActionStatus status;
//	File file(tszRoot, FastFileFlags::OpenReadWrite, &status);
	File file(tszRoot, FileModes::OpenReadWrite.ShareAll(), &status);
	if (!status.Successful())
		return status;
	file.DeviceIoControl(FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &status);
	return status;
}

#include <set>

std::vector<VolumeManager::MountedVolume> BazisLib::Win32::VolumeManager::GetAllMountedVolumes()
{
	std::vector<MountedVolume> result;
	TCHAR tszVolume[MAX_PATH];
	HANDLE hVol = FindFirstVolume(tszVolume, __countof(tszVolume));
	if (hVol == INVALID_HANDLE_VALUE)
		return result;

	String volMountPoint;
	const size_t maxSize = 4096;
	do 
	{
		String volNameWithoutPrefix(tszVolume + 4, _tcslen(tszVolume) - 5);
		MountedVolume volObj;
		volObj.MainVolumeName = tszVolume;

		size_t devNameLen = QueryDosDevice(volNameWithoutPrefix.c_str(), volObj.DeviceName.PreAllocate(maxSize, false), maxSize + 1);
		if (devNameLen)
			volObj.DeviceName.SetLength(_tcslen(volObj.DeviceName.GetConstBuffer()));
		
		DWORD bufSize = 0;
		GetVolumePathNamesForVolumeName(tszVolume, NULL, 0, &bufSize);
		if (!bufSize)
			continue;
		SolidVector<TCHAR> pName(bufSize);
		if (!GetVolumePathNamesForVolumeName(tszVolume, pName.GetData(), (DWORD)pName.GetAllocated(), &bufSize))
			continue;

		for(size_t off = 0, len; len = _tcslen(pName.GetData(off)); off += (len + 1))
			volObj.MountPoints.push_back(TempStrPointerWrapper(pName.GetData(off), len));
		result.push_back(volObj);

	} while (FindNextVolume(hVol, tszVolume, __countof(tszVolume)));
	FindVolumeClose(hVol);
	return result;
}

#endif
#endif