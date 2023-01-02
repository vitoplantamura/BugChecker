#pragma once
#include "../assert.h"
#include <libkern/OSAtomic.h>

namespace BazisLib
{
	namespace KEXT
	{
		template<class _BaseType> class _AtomicInt32
		{
		private:
			int32_t m_Value;

		public:
			typedef _BaseType _UnderlyingType;

			_AtomicInt32(_BaseType val) {m_Value = (int32_t)val; C_ASSERT(sizeof(m_Value) == sizeof(_BaseType));}
			_AtomicInt32(){}
			operator _BaseType() {return (_BaseType)m_Value;}
			_BaseType operator++() {return OSIncrementAtomic(&m_Value) + 1;}
			_BaseType operator++(int) {return OSIncrementAtomic(&m_Value);}
			_BaseType operator--() {return OSDecrementAtomic(&m_Value) - 1;}
			_BaseType operator--(int) {return OSDecrementAtomic(&m_Value);}
			_BaseType operator+=(_BaseType v) {return OSAddAtomic(v, &m_Value) + v;}
			_BaseType operator-=(_BaseType v) {return OSAddAtomic(-v, &m_Value) - v;}

			_BaseType operator|=(_BaseType v) {return OSBitOrAtomic(v, (uint32_t *)&m_Value) | v;}
			_BaseType operator&=(_BaseType v) {return OSBitAndAtomic(v, (uint32_t *)&m_Value) & v;}
			_BaseType operator^=(_BaseType v) {return OSBitXorAtomic(v, (uint32_t *)&m_Value) ^ v;}

			_BaseType GetAndOr(_BaseType v) {return OSBitOrAtomic(v, (uint32_t *)&m_Value);}
			_BaseType GetAndAnd(_BaseType v) {return OSBitAndAtomic(v, (uint32_t *)&m_Value);}
			_BaseType GetAndXor(_BaseType v) {return OSBitXorAtomic(v, (uint32_t *)&m_Value);}

			bool CompareAndExchange(_BaseType oldVal, _BaseType newVal) {return OSCompareAndSwap(oldVal, newVal, &m_Value);}
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
			_BaseType operator++() {return OSIncrementAtomic64(&m_Value) + 1;}
			_BaseType operator++(int) {return OSIncrementAtomic64(&m_Value);}
			_BaseType operator--() {return OSDecrementAtomic64(&m_Value) - 1;}
			_BaseType operator--(int) {return OSDecrementAtomic64(&m_Value);}
			_BaseType operator+=(_BaseType v) {return OSAddAtomic64(v, &m_Value) + v;}
			_BaseType operator-=(_BaseType v) {return OSAddAtomic64(-v, &m_Value) - v;}

			bool CompareAndExchange(_BaseType oldVal, _BaseType newVal) {return OSCompareAndSwap64(oldVal, newVal, &m_Value);}
		};
	}

	typedef KEXT::_AtomicInt32<int32_t> AtomicInt32;
	typedef KEXT::_AtomicInt32<uint32_t> AtomicUInt32;

	typedef KEXT::_AtomicInt64<int64_t> AtomicInt64;
	typedef KEXT::_AtomicInt64<uint64_t> AtomicUInt64;
}

#define BAZISLIB_ATOMIC_DEFINED
#define BZSLIB_NO_64BIT_ATOMIC_BITWISE_OPS