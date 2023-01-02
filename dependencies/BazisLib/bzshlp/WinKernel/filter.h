#pragma once
#include "pnpdevice.h"
#include "devapi.h"

namespace BazisLib
{
	namespace DDK
	{
		/*  The Filter class is subclassed from PNPDevice because PnP IRPs are processed in filter-like manner.
			That allows using existing PNPDevice code for handling PnP events.
		*/
		class Filter : protected PNPDevice
		{
		private:
			ExternalDeviceObjectReference m_BaseDeviceReference;

		public:
			Filter(LPCWSTR pwszBaseDeviceName,
			       bool bDeleteThisAfterRemoveRequest = false,
				   LPCWSTR pwszFilterDeviceName = NULL,
				   DEVICE_TYPE FilterDeviceType = FILE_DEVICE_UNKNOWN,
				   ULONG DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN,
				   bool bExclusive = FALSE,
				   ULONG AdditionalDeviceFlags = DO_POWER_PAGABLE);

			~Filter();

			bool Valid() {return __super::Valid();}

			NTSTATUS Register(Driver *pDriver, const GUID *pInterfaceGuid = NULL);

		};
	}
}