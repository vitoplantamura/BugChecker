#pragma once
#include "IORegistry.h"
#include "IOConnect.h"
#include <bzscore/string.h>
#include <list>

namespace BazisLib
{
	namespace MacOS
	{
		class IOServiceWrapper : public _IORegistryObjectWrapperT<io_service_t>
		{
		protected:
			typedef _IORegistryObjectWrapperT<io_service_t> super;

		public:
			IOServiceWrapper(io_service_t obj)
				: super(obj)
			{
			}

			IOServiceWrapper()
			{
			}

			IOConnectWrapper OpenService()
			{
				io_connect_t result = 0;
				IOServiceOpen(get(), mach_task_self(), 0, &result);
				return result;
			}

			IOServiceWrapper GetParentService()
			{
				return GetParent();
			}

			IOServiceWrapper FindFirstMatchingChildRecursively(const char *pClassName);
			void FindAllMatchingChildrenRecursively(const char *pClassName, std::list<IOServiceWrapper> &allServices, bool lookThroughChildrenOfMatched = false);
		};

		typedef _IOIteratorWrapper<IOServiceWrapper> IOServiceIteratorWrapper;

		inline BazisLib::MacOS::IOServiceWrapper IOServiceWrapper::FindFirstMatchingChildRecursively( const char *pClassName )
		{
			for (IOServiceIteratorWrapper iter(GetChildren()); iter.Valid(); iter.Next())
			{
				if (iter->IsAnInstanceOf(pClassName))
					return *iter;
				IOServiceWrapper result = iter->FindFirstMatchingChildRecursively(pClassName);
				if (result.Valid())
					return result;
			}
			return NULL;
		}

		inline void IOServiceWrapper::FindAllMatchingChildrenRecursively( const char *pClassName, std::list<IOServiceWrapper> &allServices, bool lookThroughChildrenOfMatched )
		{
			for (IOServiceIteratorWrapper iter(GetChildren()); iter.Valid(); iter.Next())
			{
				if (iter->IsAnInstanceOf(pClassName))
				{
					allServices.push_back(*iter);
					if (!lookThroughChildrenOfMatched)
						continue;
				}
				iter->FindAllMatchingChildrenRecursively(pClassName, allServices, lookThroughChildrenOfMatched);
			}
		}

	}
}