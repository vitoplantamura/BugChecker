#pragma once
#include "IOObject.h"
#include <IOKit/IOCFPlugIn.h>

namespace BazisLib
{
	namespace MacOS
	{
		class CFPluginWrapper
		{
		private:
			IOCFPlugInInterface **_ppInterface;

		private:
			CFPluginWrapper(const CFPluginWrapper &) { }
			void operator=(const CFPluginWrapper &) { }

		public:
			CFPluginWrapper(const IOObjectWrapper &service, CFUUIDRef pluginType, CFUUIDRef interfaceType = kIOCFPlugInInterfaceID)
				: _ppInterface(NULL)
			{
				SInt32 score;
				IOCreatePlugInInterfaceForService(service.get(), pluginType, interfaceType, &_ppInterface, &score);
			}

			bool Valid() const
			{
				return _ppInterface != 0;
			}

			~CFPluginWrapper()
			{
				if (_ppInterface)
					IODestroyPlugInInterface(_ppInterface);
			}

			template <class _Iface> HRESULT QueryInterface(CFUUIDRef iid, _Iface ***ppv) const
			{
				if (!_ppInterface)
					return E_POINTER;
				return (*_ppInterface)->QueryInterface(_ppInterface, CFUUIDGetUUIDBytes(iid), (LPVOID *)ppv);
			}
		};

		template <class _Iface> class CFInterfaceWrapper
		{
		private:
			_Iface **_pIface;

		public:
			_Iface *operator->()
			{
				return *_pIface;
			}

			CFInterfaceWrapper(const CFPluginWrapper &plugin, CFUUIDRef interfaceID)
				: _pIface(NULL)
			{
				if (plugin.Valid())
					plugin.QueryInterface(interfaceID, &_pIface);
			}

			bool Valid() {return _pIface != NULL;}

			CFInterfaceWrapper(_Iface **ppIface = NULL)
				: _pIface(ppIface)
			{
			}

			CFInterfaceWrapper(const CFInterfaceWrapper<_Iface> &wrapper)
				: _pIface(wrapper._pIface)
			{
				if (_pIface)
					(*_pIface)->AddRef(_pIface);
			}

			void operator=(const CFInterfaceWrapper<_Iface> &wrapper)
			{
				if (_pIface)
					(*_pIface)->Release(_pIface);
				_pIface = wrapper._pIface;
				if (_pIface)
					(*_pIface)->AddRef(_pIface);
			}

			operator void*(){return _pIface;}

			~CFInterfaceWrapper()
			{
				if (_pIface)
					(*_pIface)->Release(_pIface);
			}
		};

	}
}