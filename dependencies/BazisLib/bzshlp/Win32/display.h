#pragma once
#include <vector>
#include "../../bzscore/status.h"
#include "../../bzscore/buffer.h"

namespace BazisLib
{
	namespace Win32
	{
		class Display
		{
		public:
			class Mode
			{
			private:
				DEVMODE _DevMode;

			private:
				Mode(LPCTSTR lpDevName, DWORD iModeNum);
				friend class Display;

			public:
				PDEVMODE operator->() {return &_DevMode;}
				const DEVMODE *operator->() const {return &_DevMode;}

				bool Valid() {return _DevMode.dmSize != 0;}
			};

		private:
			DISPLAY_DEVICE _Device;

		public:
			Display(LPCTSTR pszDeviceName, ActionStatus *pStatus = NULL)
			{
				if (EnumDisplayDevices(pszDeviceName, 0, &_Device, 0))
					ASSIGN_STATUS(pStatus, Success);
				else
					ASSIGN_STATUS(pStatus, ActionStatus::FailFromLastError());
			}

			Display(const DISPLAY_DEVICE &dev)
				: _Device(dev)
			{
			}

			typedef std::vector<Display> DisplayCollection;

		private:
			Display()
			{
				memset(&_Device, 0, sizeof(_Device));
			}

			friend class DisplayCollection;

		public:
			enum DisplayEnumFilterKind
			{
				dfAll			= 0,
				dfMirrors		= 0x01,
				dfNonMirrors	= 0x02,
				dfConnected		= 0x04,
				dfDisconnected	= 0x08,
				dfPrimaryOnly	= 0x80000000
			};

			static DisplayCollection EnumerateDiplays(DisplayEnumFilterKind filter = dfAll);

			Mode GetCurrentMode() {return Mode(_Device.DeviceName, ENUM_CURRENT_SETTINGS);}

			typedef std::vector<Mode> ModeCollection;
			ModeCollection GetAllSupportedModes();

			PDISPLAY_DEVICE operator->(){return &_Device;}
			bool Primary() {return !!(_Device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE);}
			bool Connected() {return !!(_Device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP);}

			bool MirroringDriver() {return !!(_Device.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER);}

			ActionStatus Disconnect();
		};

	}
}