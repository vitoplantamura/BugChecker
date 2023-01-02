#pragma once
#include "../assert.h"

#ifdef __APPLE__
#include <libkern/OSAtomic.h>
#endif

#ifdef __linux__
#include <ext/atomicity.h>
#endif

namespace BazisLib
{
	namespace Posix
	{
#ifdef __APPLE__
		template<class _BaseType> class _AtomicInt32
		{
		private:
			int32_t m_Value;

		public:
			typedef _BaseType _UnderlyingType;

			_AtomicInt32(_BaseType val) {m_Value = (int32_t)val; C_ASSERT(sizeof(m_Value) == sizeof(_BaseType));}
			_AtomicInt32(){}
			operator _BaseType() {return (_BaseType)m_Value;}
			_BaseType operator++() {return OSAtomicIncrement32(&m_Value);}
			_BaseType operator++(int) {return OSAtomicIncrement32(&m_Value) - 1;}
			_BaseType operator--() {return OSAtomicDecrement32(&m_Value);}
			_BaseType operator--(int) {return OSAtomicDecrement32(&m_Value) + 1;}
			_BaseType operator+=(_BaseType v) {return OSAtomicAdd32(v, &m_Value);}
			_BaseType operator-=(_BaseType v) {return OSAtomicAdd32(-v, &m_Value);}

			_BaseType operator|=(_BaseType v) {return OSAtomicOr32(v, (uint32_t *)&m_Value);}
			_BaseType operator&=(_BaseType v) {return OSAtomicAnd32(v, (uint32_t *)&m_Value);}
			_BaseType operator^=(_BaseType v) {return OSAtomicXor32(v, (uint32_t *)&m_Value);}

			_BaseType GetAndOr(_BaseType v) {return OSAtomicOr32Orig(v, (uint32_t *)&m_Value);}
			_BaseType GetAndAnd(_BaseType v) {return OSAtomicAnd32Orig(v, (uint32_t *)&m_Value);}
			_BaseType GetAndXor(_BaseType v) {return OSAtomicXor32Orig(v, (uint32_t *)&m_Value);}

			bool CompareAndExchange(_BaseType oldVal, _BaseType newVal) {return OSAtomicCompareAndSwap32(oldVal, newVal, &m_Value);}
		};

		template<class _BaseType> class _AtomicInt64
		{
		private:
			int64_t m_Value;

		public:
			typedef _BaseType _UnderlyingType;

			_AtomicInt64(_BaseType val) {m_Value = (int64_t)val; C_ASSERT(sizeof(m_Value) == sizeof(_BaseType));}
			_AtomicInt64(){}
			operator _BaseType() {return (_BaseType)m_Value;}
			_BaseType operator++() {return OSAtomicIncrement64(&m_Value);}
			_BaseType operator++(int) {return OSAtomicIncrement64(&m_Value) - 1;}
			_BaseType operator--() {return OSAtomicDecrement64(&m_Value);}
			_BaseType operator--(int) {return OSAtomicDecrement64(&m_Value) + 1;}
			_BaseType operator+=(_BaseType v) {return OSAtomicAdd64(v, &m_Value);}
			_BaseType operator-=(_BaseType v) {return OSAtomicAdd64(-v, &m_Value);}

			bool CompareAndExchange(_BaseType oldVal, _BaseType newVal) {return OSAtomicCompareAndSwap64(oldVal, newVal, &m_Value);}
		};

#define BZSLIB_NO_64BIT_ATOMIC_BITWISE_OPS

#elif defined (__linux__)
		template<class _BaseType> class _AtomicInt32
		{
		private:
			_Atomic_word m_Value;

		public:
			typedef _BaseType _UnderlyingType;

			_AtomicInt32(_BaseType val) {m_Value = val; C_ASSERT(sizeof(m_Value) == sizeof(_BaseType));}
			_AtomicInt32(){}
			operator _BaseType() {return m_Value;}
			_BaseType operator++() {return __sync_fetch_and_add(&m_Value, 1) + 1;}
			_BaseType operator++(int) {return __sync_fetch_and_add(&m_Value, 1);}
			_BaseType operator--() {return __sync_fetch_and_sub(&m_Value, 1) - 1;}
			_BaseType operator--(int) {return __sync_fetch_and_sub(&m_Value, 1);}
			_BaseType operator+=(_BaseType v) {return __sync_fetch_and_add(&m_Value, v) + v;}
			_BaseType operator-=(_BaseType v) {return __sync_fetch_and_sub(&m_Value, v) - v;}

			bool CompareAndExchange(_BaseType oldVal, _BaseType newVal) {return __sync_val_compare_and_swap(&m_Value, oldVal, newVal) == oldVal;}

			_BaseType operator|=(_BaseType v) {return __sync_fetch_and_or(&m_Value, v) | v;}
			_BaseType operator&=(_BaseType v) {return __sync_fetch_and_and(&m_Value, v) & v;}
			_BaseType operator^=(_BaseType v) {return __sync_fetch_and_xor(&m_Value, v) ^ v;}

			_BaseType GetAndOr(_BaseType v) {return __sync_fetch_and_or(&m_Value, v);}
			_BaseType GetAndAnd(_BaseType v) {return __sync_fetch_and_and(&m_Value, v);}
			_BaseType GetAndXor(_BaseType v) {return __sync_fetch_and_xor(&m_Value, v);}
		};

#define BZSLIB_NO_64BIT_ATOMIC_OPS


#else
#error Atomic types not available on your platform
#endif
	}
	
	typedef Posix::_AtomicInt32<int32_t> AtomicInt32;
	typedef Posix::_AtomicInt32<uint32_t> AtomicUInt32;

#ifndef BZSLIB_NO_64BIT_ATOMIC_OPS
	typedef Posix::_AtomicInt64<int64_t> AtomicInt64;
	typedef Posix::_AtomicInt64<uint64_t> AtomicUInt64;
#endif
}

#define BAZISLIB_ATOMIC_DEFINED
