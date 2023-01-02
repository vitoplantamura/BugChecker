#include "stdafx.h"
#include <bzscore/thread.h>
#include <bzscore/sync.h>
#include <bzscore/datetime.h>
#include "TestPrint.h"

using namespace BazisLib;

class TestThread1 : public Thread
{
private:
	Event _Event;

public:
	TestThread1()
	{
	}

	virtual int ThreadBody() override
	{
		_Event.Set();
		return 123;
	}

	bool Test()
	{
		TEST_ASSERT(Start());
		_Event.Wait();
		TEST_ASSERT(Join());
		TEST_ASSERT(GetReturnCode() == 123);

		return true;
	}
};

class TestThread2 : public Thread
{
private:
	Event _Event;

public:
	virtual int ThreadBody() override
	{
		_Event.Set();
		Thread::Sleep(10000);
		return 123;
	}

	bool Test()
	{
#if !defined(BZSLIB_WINKERNEL) && !defined(BZSLIB_POSIX) && !defined(BZSLIB_KEXT)
		TEST_ASSERT(Start());
		_Event.Wait();
		TEST_ASSERT(IsRunning());
		TEST_ASSERT(Terminate());
		TEST_ASSERT(Join());
		TEST_ASSERT(!IsRunning());
#endif
		
		return true;
	}
};

#ifndef BZSLIB_WINKERNEL
class TestThread3 : public Thread
{
private:
	Event _ThreadStarted, _EndThreadNow;
	Mutex _ThreadMutex;

private:
	virtual int ThreadBody() override
	{
		MutexLocker lck(_ThreadMutex);
		_ThreadStarted.Set();
		_EndThreadNow.Wait();
		Thread::Sleep(10);
		return 0;
	}

public:
	bool Test()
	{
		_ThreadMutex.Lock();
		_ThreadMutex.Unlock();

		TEST_ASSERT(Start());
		
		_ThreadStarted.Wait();
		TEST_ASSERT(!_ThreadMutex.TryLock());
		DateTime dtStart = DateTime::Now();
		TEST_ASSERT(!Join(100));
		int elapsed = (int)dtStart.MillisecondsElapsed();
		TEST_ASSERT(abs(elapsed - 100) < 20);

		_EndThreadNow.Set();
		TEST_ASSERT(Join(100));
		TEST_ASSERT(_ThreadMutex.TryLock());
		_ThreadMutex.Unlock();

		return true;
	}
};
#endif

bool TestThreadAPI()
{
	TestPrint("Testing threads... In case of bugs, the program might hang here.\n");
	TEST_ASSERT(TestThread1().Test());

	DateTime dt = DateTime::Now();
#ifndef BZSLIB_WINKERNEL	//Can be inaccurate when we run inside a VM
	Thread::Sleep(100);
	int elapsed = (int)dt.MillisecondsElapsed();
	TEST_ASSERT(abs(elapsed - 100) < 20);
#endif

	TEST_ASSERT(TestThread2().Test());
#ifndef BZSLIB_WINKERNEL
	TEST_ASSERT(TestThread3().Test());
#endif

	return true;
}
