#pragma once

#include <IOKit/IOKitLib.h>
#include <IOKit/scsi/SCSITaskLib.h>
#include "CFHandle.h"
#include "IOService.h"
#include "IOObject.h"
#include "../../bzscore/string.h"
#include <CoreFoundation/CFString.h>

namespace BazisLib
{
	namespace MacOS
	{
		template <class _ObjectWrapper = IOObjectWrapper> class _IOIteratorWrapper : public IOObjectWrapper
		{
		private:
			_ObjectWrapper _Object;

		private:
			_IOIteratorWrapper(const _IOIteratorWrapper &it)
			{
			}

			void operator=(const _IOIteratorWrapper &it)
			{
			}

		public:
			_IOIteratorWrapper(io_iterator_t referencedIterator)
				: IOObjectWrapper(referencedIterator)
			{
				if (!get())
					return;
				Next();
			}

			void Next()
			{
				_Object = IOIteratorNext(get());
			}

			bool Valid()
			{
				return _Object.Valid();
			}

			typename _ObjectWrapper::_RawObjectType *getObject()
			{
				return _Object.getObject();
			}

			_ObjectWrapper operator*()
			{
				return _Object;
			}

			_ObjectWrapper *operator->()
			{
				return &_Object;
			}

		};

		template <typename _ObjType> class _IORegistryObjectWrapperT : public _IOObjectWrapperT<_ObjType>
		{
		private:
			typedef _IOObjectWrapperT<_ObjType> super;
		public:
			_IORegistryObjectWrapperT(_ObjType obj)
				: super(obj)
			{
			}

			_IORegistryObjectWrapperT()
			{
			}

			io_registry_entry_t GetParent(io_name_t plane = (char *)kIOServicePlane)
			{
				io_registry_entry_t parent = NULL;
				kern_return_t err = IORegistryEntryGetParentEntry(super::get(), plane, &parent);
				if (err != KERN_SUCCESS)
					return 0;
				return parent;
			}

			BazisLib::String GetRegistryPath(io_name_t plane = (char *)kIOServicePlane)
			{
				if (!super::Valid())
					return BazisLib::String();
				io_string_t path;
				kern_return_t result = IORegistryEntryGetPath(super::get(), plane, path);
				if (result != KERN_SUCCESS)
					return BazisLib::String();
				return BazisLib::String(path);
			}

			io_iterator_t GetChildren(io_name_t plane = (char *)kIOServicePlane)
			{
				io_iterator_t iter = 0;
				kern_return_t err = IORegistryEntryGetChildIterator(super::get(), plane, &iter);
				if (err != KERN_SUCCESS)
					return 0;
				return iter;
			}

			bool IsAnInstanceOf(const char *pName)
			{
				return IOObjectConformsTo(super::get(), pName) != 0;
			}

			BazisLib::String GetPropertyValue(const char *pPropertyName)
			{
				CFTypeRef val = IORegistryEntryCreateCFProperty(super::get(), __CFStringMakeConstantString(pPropertyName), kCFAllocatorDefault, 0);
				if (!val)
					return BazisLib::String();
				if (CFGetTypeID(val) != CFStringGetTypeID())
					return BazisLib::String();

				int len = CFStringGetLength((CFStringRef)val);
				BazisLib::String result;
				if (!CFStringGetCString((CFStringRef)val, result.PreAllocate(len, false), len + 1, kCFStringEncodingASCII))
					return BazisLib::String();
				result.SetLength(len);
				
				CFRelease(val);
				return result;
			}

			template <class _ValueType> _ValueType GetNumericPropertyValue(const char *pPropertyName, _ValueType defaultVal = _ValueType())
			{
				_ValueType result = defaultVal;

				CFTypeRef val = IORegistryEntryCreateCFProperty(super::get(), __CFStringMakeConstantString(pPropertyName), kCFAllocatorDefault, 0);
				if (!val)
					return result;
				CFNumberType type = CFNumberGetType((CFNumberRef)val);
				if (CFGetTypeID(val) != CFNumberGetTypeID())
					return result;

				if (CFNumberGetByteSize((CFNumberRef)val) != sizeof(result))
					return result;

				if (!CFNumberGetValue((CFNumberRef)val, type, &result))
					return defaultVal;
				CFRelease(val);
				return result;
			}

			uint64_t GetEntryID()
			{
				uint64_t ID = 0;
				IORegistryEntryGetRegistryEntryID(super::get(), &ID);
				return ID;
			}
		};
	}
}