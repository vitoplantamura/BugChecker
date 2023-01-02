#pragma once
#include <CoreFoundation/CoreFoundation.h>

namespace BazisLib
{
	namespace MacOS
	{
		template <class _Type = CFTypeRef> class CFHandle
		{
		private:
			_Type _TypeRef;

		public:
			CFHandle(_Type obj = 0)
				: _TypeRef(obj)
			{
			}

			~CFHandle()
			{
				if (_TypeRef)
					CFRelease(_TypeRef);
			}

			CFHandle(const CFHandle &handle)
				: _TypeRef(handle._TypeRef)
			{
				if (_TypeRef)
					CFRetain(_TypeRef);
			}

			_Type get() const {return _TypeRef;}
			operator bool() {return _TypeRef != 0;}

			CFHandle &operator=(_Type obj)
			{
				if (_TypeRef)
					CFRelease(_TypeRef);
				_TypeRef = obj;
				if (_TypeRef)
					CFRetain(_TypeRef);
				return *this;
			}

			CFHandle &operator=(CFHandle &handle)
			{
				if (_TypeRef)
					CFRelease(_TypeRef);
				_TypeRef = handle._TypeRef;
				if (_TypeRef)
					CFRetain(_TypeRef);
			}

		};
	}
}