#pragma once
#include "cmndef.h"
#include "../../bzscore/string.h"
#include "guid.h"
#include <vector>

#ifndef UNDER_CE

namespace BazisLib
{
	namespace Win32
	{
		class VolumeManager
		{
		public:
			VolumeManager(void);
			~VolumeManager(void);

		public:
			static String GetVolumeNameForGuid(const Guid &guid);
			static String GetVolumeMountPointForGuid(const Guid &guid);
			static TCHAR GetDriveLetterForGuid(const Guid &guid);
			static ActionStatus ChangeVolumeLetter(TCHAR oldLetter, TCHAR newLetter, bool bForce);

			static ActionStatus DismountVolume(TCHAR driveLetter);

			struct MountedVolume
			{
				String DeviceName;
				String MainVolumeName;
				std::vector<String> MountPoints;
			};

			static std::vector<MountedVolume> GetAllMountedVolumes();
		};
	}
}

#endif