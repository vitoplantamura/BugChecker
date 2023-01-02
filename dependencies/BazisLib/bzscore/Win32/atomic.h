#pragma once

#ifndef BZSLIB_WINKERNEL
#include <intrin.h>
#endif

namespace BazisLib
{
	namespace Win32
	{
		//! Provides atomic operation support for 32-bit integers
		/*! The _AtomicInt32 template class provides support for atomic operations (such as addition and subtraction)
			on 32-bit integers. To use such class, simply declare a variable and apply standard operators. For example,
			the following code ensures atomic operation:
			<pre>
class Test
{
public:
	AtomicInt32 m_RefCount;

	unsigned Retain()
	{
		return ++m_RefCount;
	}

	unsigned Release()
	{
		register int r = --m_RefCount;
		if (!r)
			delete this;
		return r;
	}
};
			</pre>
			Avoid using the _AtomicInt32 template directly. Instead use the AtomicInt32 and AtomicUInt32 typedefs.
		*/
		template<class _BaseType> class _AtomicInt32
		{
		private:
			long m_Value;

		public:
			typedef _BaseType _UnderlyingType;

			_AtomicInt32(_BaseType val) {m_Value = val; C_ASSERT(sizeof(m_Value) == sizeof(_BaseType));}
			_AtomicInt32(){}
			operator _BaseType() {return m_Value;}

			_BaseType operator++() {return _InterlockedIncrement(&m_Value);}
			_BaseType operator++(int) {return _InterlockedIncrement(&m_Value) - 1;}
			_BaseType operator--() {return _InterlockedDecrement(&m_Value);}
			_BaseType operator--(int) {return _InterlockedDecrement(&m_Value) + 1;}
			_BaseType operator+=(_BaseType v) {return _InterlockedExchangeAdd(&m_Value, v) + v;}
			_BaseType operator-=(_BaseType v) {return _InterlockedExchangeAdd(&m_Value, -v) - v;}

			_BaseType operator|=(_BaseType v) {return _InterlockedOr(&m_Value, v) | v;}
			_BaseType operator&=(_BaseType v) {return _InterlockedAnd(&m_Value, v) & v;}
			_BaseType operator^=(_BaseType v) {return _InterlockedXor(&m_Value, v) ^ v;}

			_BaseType GetAndOr(_BaseType v) {return _InterlockedOr(&m_Value, v);}
			_BaseType GetAndAnd(_BaseType v) {return _InterlockedAnd(&m_Value, v);}
			_BaseType GetAndXor(_BaseType v) {return _InterlockedXor(&m_Value, v);}

			bool CompareAndExchange(_BaseType oldVal, _BaseType newVal) {return _InterlockedCompareExchange(&m_Value, newVal, oldVal) == oldVal;}
		};

#ifdef BZSLIB64
		template<class _BaseType> class _AtomicInt64
		{
		private:
			LONGLONG m_Value;

		public:
			typedef _BaseType _UnderlyingType;

			_AtomicInt64(_BaseType val) {m_Value = val; C_ASSERT(sizeof(m_Value) == sizeof(_BaseType));}
			_AtomicInt64(){}
			operator _BaseType() {return m_Value;}

			_BaseType operator++() {return _InterlockedIncrement64(&m_Value);}
			_BaseType operator++(int) {return _InterlockedIncrement64(&m_Value) - 1;}
			_BaseType operator--() {return _InterlockedDecrement64(&m_Value);}
			_BaseType operator--(int) {return _InterlockedDecrement64(&m_Value) + 1;}
			_BaseType operator+=(_BaseType v) {return _InterlockedExchangeAdd64(&m_Value, v) + v;}
			_BaseType operator-=(_BaseType v) {return _InterlockedExchangeAdd64(&m_Value, -v) - v;}

			_BaseType operator|=(_BaseType v) {return _InterlockedOr64(&m_Value, v) | v;}
			_BaseType operator&=(_BaseType v) {return _InterlockedAnd64(&m_Value, v) & v;}
			_BaseType operator^=(_BaseType v) {return _InterlockedXor64(&m_Value, v) ^ v;}

			_BaseType GetAndOr(_BaseType v) {return _InterlockedOr64(&m_Value, v);}
			_BaseType GetAndAnd(_BaseType v) {return _InterlockedAnd64(&m_Value, v);}
			_BaseType GetAndXor(_BaseType v) {return _InterlockedXor64(&m_Value, v);}

			bool CompareAndExchange(_BaseType oldVal, _BaseType newVal) {return _InterlockedCompareExchange64(&m_Value, newVal, oldVal) == oldVal;}
		};
#else
#define BZSLIB_NO_64BIT_ATOMIC_OPS
#endif
	}

	typedef Win32::_AtomicInt32<int> AtomicInt32;
	typedef Win32::_AtomicInt32<unsigned> AtomicUInt32;

#ifdef BZSLIB64
	typedef Win32::_AtomicInt64<LONGLONG> AtomicInt64;
	typedef Win32::_AtomicInt64<ULONGLONG> AtomicUInt64;
#endif
}

#define BAZISLIB_ATOMIC_DEFINED
