#include "stdafx.h"
#include "TestPrint.h"
#include <bzscore/atomic.h>

#if defined(BZSLIB_NO_64BIT_ATOMIC_OPS) && defined(BZSLIB64)
bool TestAtomicAPI()
{
	return true;
}

#else

using namespace BazisLib;
bool TestAtomicAPI();

//We don't really test atomic API with multiple threads, as most problems with atomic integer implementation come
//from incorrect calls to underlying OS atomic functions and can be easily detected with just one thread.
template<typename _AtomicType> bool DoTestAtomicAPI()
{
	_AtomicType val = 1000;

	TEST_ASSERT(val == 1000);
	TEST_ASSERT(val-- == 1000);
	TEST_ASSERT(val == 999);
	TEST_ASSERT(val++ == 999);
	TEST_ASSERT(val == 1000);

	TEST_ASSERT(--val == 999);
	TEST_ASSERT(val == 999);
	TEST_ASSERT(++val == 1000);
	TEST_ASSERT(val == 1000);

	TEST_ASSERT((val += 2) == 1002);
	TEST_ASSERT(val == 1002);

	TEST_ASSERT((val -= 2) == 1000);
	TEST_ASSERT(val == 1000);

	TEST_ASSERT(!val.CompareAndExchange(999, 1234));
	TEST_ASSERT(val == 1000);
	TEST_ASSERT(val.CompareAndExchange(1000, 1234));
	TEST_ASSERT(val == 1234);
	return true;
}

template<typename _AtomicType> bool DoTestBitwiseAtomicAPI()
{
	_AtomicType val = 1000;

	val = 0x1000;
	TEST_ASSERT(val == 0x1000);

	TEST_ASSERT((val |= 0x1100) == 0x1100);
	TEST_ASSERT(val == 0x1100);

	TEST_ASSERT((val &= ~0x100) == 0x1000);
	TEST_ASSERT(val == 0x1000);

	TEST_ASSERT((val ^= 0x1001) == 1);
	TEST_ASSERT(val == 1);

	val = 0x1000;
	TEST_ASSERT(val == 0x1000);

	TEST_ASSERT((val.GetAndOr(0x1100)) == 0x1000);
	TEST_ASSERT(val == 0x1100);

	TEST_ASSERT((val.GetAndAnd(~0x100)) == 0x1100);
	TEST_ASSERT(val == 0x1000);

	TEST_ASSERT((val.GetAndXor(0x1001)) == 0x1000);
	TEST_ASSERT(val == 1);

	return true;
}

bool TestAtomicAPI()
{
	if (!DoTestAtomicAPI<AtomicInt32>())
		return false;
#ifndef BZSLIB_NO_64BIT_ATOMIC_OPS
	if (!DoTestAtomicAPI<AtomicInt64>())
		return false;
#endif
#ifndef BZSLIB_NO_ATOMIC_BITWISE_OPS
	if (!DoTestBitwiseAtomicAPI<AtomicInt32>())
		return false;
#ifndef BZSLIB_NO_64BIT_ATOMIC_BITWISE_OPS
#ifndef BZSLIB_NO_64BIT_ATOMIC_OPS
	if (!DoTestBitwiseAtomicAPI<AtomicInt64>())
		return false;
#endif
#endif
#endif
	
	int a, b;
	_AtomicPointerT<int> pTest;
	TEST_ASSERT(pTest == NULL);
	TEST_ASSERT(!pTest);
	TEST_ASSERT(pTest.AssignIfNULL(&a));
	TEST_ASSERT(pTest);
	TEST_ASSERT(pTest == &a);
	TEST_ASSERT(!pTest.AssignIfNULL(&b));
	TEST_ASSERT(pTest == &a);

	return true;
}
#endif