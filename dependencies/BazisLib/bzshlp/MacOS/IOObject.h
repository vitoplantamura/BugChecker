#pragma once
#include <IOKit/IOKitLib.h>

namespace BazisLib
{
	namespace MacOS
	{
		class IOObjectWrapper
		{
		private:
			io_object_t _Obj;

		public:
			IOObjectWrapper(io_object_t obj = 0)
				: _Obj(obj)
			{
			}

			~IOObjectWrapper()
			{
				if (_Obj)
					IOObjectRelease(_Obj);
			}

			IOObjectWrapper(const IOObjectWrapper &obj)
				: _Obj(obj._Obj)
			{
				if (_Obj)
					IOObjectRetain(_Obj);
			}

			io_object_t get() const {return _Obj;}
			io_object_t RetainAndGet() {IOObjectRetain(_Obj); return _Obj;}

			io_object_t *MakeWritablePointer()
			{
				if (_Obj)
				{
					IOObjectRelease(_Obj);
					_Obj = NULL;
				}
				return &_Obj;
			}

			bool Valid() { return  _Obj != 0;}
			operator bool() {return _Obj != 0;}

			IOObjectWrapper &operator=(IOObjectWrapper obj)
			{
				if (_Obj)
					IOObjectRelease(_Obj);
				_Obj = obj._Obj;
				if (_Obj)
					IOObjectRetain(_Obj);
				return *this;
			}

			IOObjectWrapper &operator=(io_object_t &obj)
			{
				if (_Obj)
					IOObjectRelease(_Obj);
				_Obj = obj;
				if (_Obj)
					IOObjectRetain(_Obj);
			}

		};

		template <typename _ObjType> class _IOObjectWrapperT : public IOObjectWrapper
		{
		public:
			_IOObjectWrapperT(_ObjType obj)
				: IOObjectWrapper(obj)
			{
			}

			_IOObjectWrapperT()
			{
			}

			_ObjType get()
			{
				return static_cast<_ObjType>(IOObjectWrapper::get());
			}

			typedef _ObjType _RawObjectType;
		};
	}
}