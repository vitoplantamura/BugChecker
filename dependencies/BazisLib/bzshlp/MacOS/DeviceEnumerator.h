#pragma once

#include <IOKit/IOKitLib.h>
#include <IOKit/scsi/SCSITaskLib.h>
#include "CFHandle.h"
#include "IOService.h"
#include <string>
#include "IORegistry.h"

namespace BazisLib
{
	namespace MacOS
	{
		class DeviceInformationSet
		{
		private:
			CFHandle<CFDictionaryRef> _MatchingDict;

		public:
			enum MatchType
			{
				ServiceClass,
				ServiceName,
				BSDName,
			};

			DeviceInformationSet(const char *pName, MatchType nameType = ServiceClass)
			{
				switch(nameType)
				{
				case ServiceName:
					_MatchingDict = IOServiceNameMatching(pName);
					break;
				case BSDName:
					_MatchingDict = IOBSDNameMatching(kIOMasterPortDefault, 0, pName);
					break;
				case ServiceClass:
				default:
					_MatchingDict = IOServiceMatching(pName);
					break;
				}
			}

			DeviceInformationSet(uint64_t entryID)
			{
				_MatchingDict = IORegistryEntryIDMatching(entryID);
			}

			class iterator : public _IOIteratorWrapper<IOServiceWrapper>
			{
			private:
				typedef _IOIteratorWrapper<IOServiceWrapper> super;

			public:
				iterator(const DeviceInformationSet &devs)
					: super(NULL)
				{
					IOServiceGetMatchingServices(kIOMasterPortDefault, devs._MatchingDict.get(), MakeWritablePointer());
					if (!get())
						return;
					Next();
				}

				BazisLib::String GetServicePath()
				{
					return (*this)->GetRegistryPath();
				}
			};

			IOServiceWrapper GetFirstService()
			{
				return *iterator(*this);
			}
		};
	}
}