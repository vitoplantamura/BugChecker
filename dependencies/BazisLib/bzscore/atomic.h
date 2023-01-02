#pragma once
#include "cmndef.h"

#ifdef BZSLIB_KEXT
#include "KEXT/atomic.h"
#elif defined (BZSLIB_POSIX)
#include "Posix/atomic.h"
#elif defined (BZSLIB_WINKERNEL)
#include "WinKernel/atomic.h"
#elif defined (_WIN32)
#include "Win32/atomic.h"
#else
#error No atomic types defined for current platform
#endif

namespace BazisLib
{
#if defined(BZSLIB_NO_64BIT_ATOMIC_OPS) && defined(BZSLIB64)
	//No AtomicPointer available
#else
	class AtomicPointer
	{
	protected:
#ifdef BZSLIB64
		AtomicUInt64 _Value;
		typedef AtomicUInt64::_UnderlyingType _UnderlyingType;
#else
		AtomicUInt32 _Value;
		typedef AtomicUInt32::_UnderlyingType _UnderlyingType;
#endif

		C_ASSERT(sizeof(_UnderlyingType) == sizeof(void *));

	private:
		void operator=(void *p)
		{
			_Value = (_UnderlyingType)p;
		}

	public:
		AtomicPointer(void *pVal = NULL)
			: _Value((_UnderlyingType)pVal)
		{
		}

		bool AssignIfNULL(void *pNewVal)
		{
			return _Value.CompareAndExchange((_UnderlyingType)NULL, (_UnderlyingType)pNewVal);
		}

		operator void *()
		{
			return (void *)(_UnderlyingType)_Value;
		}
	};

	template <class _Ty> class _AtomicPointerT
	{
	private:
		AtomicPointer _Pointer;

		void operator=(_Ty *p)
		{
			_Pointer = (void *)p;
		}
		
	public:
		_AtomicPointerT(_Ty *p = NULL)
			: _Pointer(p)
		{
		}

		bool AssignIfNULL(_Ty *pNewVal)
		{
			return _Pointer.AssignIfNULL(pNewVal);
		}

		void AssignIfNULLOrDeleteIfNot(_Ty *pNewVal)
		{
			if (!AssignIfNULL(pNewVal))
				delete pNewVal;
		}

		operator _Ty *()
		{
			return (_Ty *)(void *)_Pointer;
		}

		_Ty * operator->()
		{
			return (_Ty *)(void *)_Pointer;
		}

		operator bool()
		{
			return (void *)_Pointer != 0;
		}
	};
#endif
}