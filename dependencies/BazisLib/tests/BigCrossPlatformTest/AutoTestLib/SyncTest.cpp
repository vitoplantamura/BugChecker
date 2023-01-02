#include "stdafx.h"
#include "TestPrint.h"
#include <bzscore/sync.h>
#include <bzscore/thread.h>
#include "MethodVerifier.h"

using namespace BazisLib;

class TwoThreadBase
{
protected:
	virtual int Thread1Body() {return 0;}
	virtual int Thread2Body() {return 0;}

protected:
	MemberThread _Thread1, _Thread2;

protected:
	TwoThreadBase()
		: _Thread1(this, &TwoThreadBase::Thread1Body)
		, _Thread2(this, &TwoThreadBase::Thread2Body)
	{
	}

	~TwoThreadBase()
	{
	}
};

ENSURE_MEMBER_DECL(Event, Wait, void);
ENSURE_MEMBER_DECL(Event, TryWait, bool, unsigned);
ENSURE_MEMBER_DECL(Event, Set, void);
ENSURE_MEMBER_DECL(Event, Reset, void);
ENSURE_CONSTRUCTOR_DECL(Event, bool());
ENSURE_CONSTRUCTOR_DECL(Event, bool(), EventType());

/*
	Event testing sequence:

	1. Test creating set and unset events
	2. Test waiting with 0/1 timeout for both set and unset
	3. Test waiting with infinite timeout for set
	4. Set/unset events, repeat tests

	5. Start thread, that will wait for an unset event and then set another event
	6. Ensure that waiting for an unset event freezes the thread (i.e. second event is not set)
	7. Set first event, ensure that the second one gets set as well
*/
class BasicEventTest : public TwoThreadBase
{
private:
	Event _EventSetByMainThread, _EventSetByTestThread;

protected:
	virtual int Thread1Body() override
	{
		_EventSetByMainThread.Wait();
		_EventSetByTestThread.Set();
		return 0;
	}

private:
	bool TestEventsWithinOneThread(Event &setEvent, Event &unsetEvent)
	{
		TEST_ASSERT(setEvent.TryWait(0));
		TEST_ASSERT(setEvent.TryWait(1));
		setEvent.Wait();
		TEST_ASSERT(!unsetEvent.TryWait(0));
		TEST_ASSERT(!unsetEvent.TryWait(1));
		return true;
	}

public:
	bool Test()
	{
		Event e1, e2(true);
		if (!TestEventsWithinOneThread(e2, e1))
			return false;
		e1.Set();
		e2.Reset();
		if (!TestEventsWithinOneThread(e1, e2))
			return false;

		//Test that infinite wait works
		_Thread1.Start();
		TEST_ASSERT(!_EventSetByTestThread.TryWait(100));
		_EventSetByMainThread.Set();
		TEST_ASSERT(_EventSetByTestThread.TryWait(100));

		TEST_ASSERT(_Thread1.Join(100));
		return true;
	}
};

/*
	Have 2 threads waiting for the event. Ensure that auto-reset event and manual-reset event behave differently.
*/

class TwoThreadEventTest
{
private:
	class TestThread : public Thread
	{
		Event &_In, &_Out;

	public:
		TestThread(Event &evtIn, Event &evtOut)
			: _In(evtIn)
			, _Out(evtOut)
		{
			Start();
		}

	private:
		virtual int ThreadBody() override
		{
			_In.Wait();
			_Out.Set();
			return 0;
		}
	};

private:
	bool TestAutoReset()
	{
		Event autoReset(false, kAutoResetEvent), done1, done2;
		TestThread thr1(autoReset, done1);
		TestThread thr2(autoReset, done2);

		TEST_ASSERT(!done1.TryWait(100));
		TEST_ASSERT(!done2.TryWait(10));
		autoReset.Set();

		bool f1 = done1.TryWait(100);
		bool f2 = done2.TryWait(100);

		TEST_ASSERT(f1 || f2);
		TEST_ASSERT(!f1 || !f2);
		TEST_ASSERT(!autoReset.TryWait(0));
		autoReset.Set();

		TEST_ASSERT(done1.TryWait(100));
		TEST_ASSERT(done2.TryWait(100));
		return true;
	}

	bool TestManualReset()
	{
		Event manualReset, done1, done2;
		TestThread thr1(manualReset, done1);
		TestThread thr2(manualReset, done2);

		TEST_ASSERT(!done1.TryWait(100));
		TEST_ASSERT(!done2.TryWait(10));

		manualReset.Set();

		TEST_ASSERT(done1.TryWait(100));
		TEST_ASSERT(done2.TryWait(100));
		return true;
	}

public:

	bool Test()
	{
		if (!TestAutoReset())
			return false;
		if (!TestManualReset())
			return false;
		return true;
	}
};
ENSURE_MEMBER_DECL(Mutex, Lock, void);
ENSURE_MEMBER_DECL(Mutex, Unlock, void);
#ifdef BZSLIB_WINKERNEL
ENSURE_MEMBER_DECL(Mutex, TryLock, SafeBool);
#else
ENSURE_MEMBER_DECL(Mutex, TryLock, bool);
#endif
ENSURE_CONSTRUCTOR_DECL(Mutex);

/*
	Mutex testing sequence:
	
	1. Main thread: try take mutex, release immediately
	2. Thread 1: Recursive take/release twice, set event when done
	3. Thread 1: Take mutex, set MutexTaken1, wait for MutexReleaseTrigger
	4. Main thread: try take mutex, ensure a failure
	5. Thread 2: Take mutex, set event MutexTaken2
	6. Main thread: wait for MutexTaken1, ensure a failed wait for MutexTaken2
	7. Set MutexReleaseTrigger
	8. Wait for MutexTaken2
*/

class MutexTest : public TwoThreadBase
{
private:
	Event _RecursiveTestDone, _MutexTaken1, _MutexTaken2, _MutexReleaseTrigger;
	Mutex _Mutex;

protected:
	virtual int Thread1Body() override
	{
		_Mutex.Lock();
		_Mutex.Lock();
		_Mutex.Unlock();
		_Mutex.Unlock();
		_RecursiveTestDone.Set();

		_Mutex.Lock();
		_MutexTaken1.Set();

		_MutexReleaseTrigger.Wait();
		_Mutex.Unlock();

		return 0;
	}

	virtual int Thread2Body() override
	{
		_Mutex.Lock();
		_MutexTaken2.Set();
		_Mutex.Unlock();

		return 0;
	}

public:
	bool Test()
	{
		TEST_ASSERT(_Mutex.TryLock());
		_Mutex.Unlock();

		_Thread1.Start();
		TEST_ASSERT(_RecursiveTestDone.TryWait(100));

		TEST_ASSERT(_MutexTaken1.TryWait(100));
		_Thread2.Start();
		TEST_ASSERT(!_MutexTaken2.TryWait(100));
		_MutexReleaseTrigger.Set();
		TEST_ASSERT(_MutexTaken2.TryWait(100));

		TEST_ASSERT(_Thread1.Join(100));
		TEST_ASSERT(_Thread2.Join(100));

		return true;
	}
};

ENSURE_MEMBER_DECL(Semaphore, Wait, void);
ENSURE_MEMBER_DECL(Semaphore, Signal, void);
ENSURE_MEMBER_DECL(Semaphore, TryWait, bool, unsigned);
ENSURE_CONSTRUCTOR_DECL(Semaphore);
ENSURE_CONSTRUCTOR_DECL(Semaphore, int());

/*
	Semaphore testing sequence:
	1. Create empty, try taking, fail.
	2. Signal k times, take k times.
	3. Create a worker thread that will do this in a loop:
		a. Wait for semaphore 1
		b. When signaled, increase a counter and signal semaphore 2
		c. If aborting, exit

	3. Repeat the following sequence with N from 1 to 10
		a. Signal semaphore N times
		b. Wait for the final semaphore N times
		c. Check counter value

	4. Set aborted flag to 1, signal semaphore
	5. Wait till thread has exited
*/

class SemaphoreTest : public TwoThreadBase
{
private:
	Semaphore _InSemaphore, _OutSemaphore;
	int _Counter;
	bool _Abort;

protected:
	virtual int Thread1Body() override
	{
		while(!_Abort)
		{
			_InSemaphore.Wait();
			_Counter++;
			_OutSemaphore.Signal();
		}

		return 0;
	}

	
public:
	bool Test()
	{
		_Counter = 0;
		_Abort = false;

		TEST_ASSERT(!_InSemaphore.TryWait());
		TEST_ASSERT(!_InSemaphore.TryWait(100));

		const int semTestIter = 3;
		for (int i = 0; i < semTestIter; i++)
			_InSemaphore.Signal();

		for (int i = 0; i < semTestIter; i++)
			TEST_ASSERT(_InSemaphore.TryWait());

		Semaphore sem2(semTestIter);

		for (int i = 0; i < semTestIter; i++)
			TEST_ASSERT(sem2.TryWait());

		int expectedCounter = 0;

		_Thread1.Start();

		for (int i = 1; i <= 10; i++)
		{
			expectedCounter += i;
			for (int j = 0; j < i; j++)
				_InSemaphore.Signal();

			for (int j = 0; j < i; j++)
				TEST_ASSERT(_OutSemaphore.TryWait(100));

			TEST_ASSERT(_Counter == expectedCounter);
		}

		_Abort = true;
		_InSemaphore.Signal();
		TEST_ASSERT(_Thread1.Join(100));
		
		return true;
	}
};

/* 
	R/W lock testing sequence:

	Utilizes 2 threads controlled via events from the main thread.

	1. Lock read from thread A
	2. Lock read from thread B
	3. Unlock read from thread A, lock write from thread A
	4. Ensure that thread A waits
	5. Unlock read from thread B. Ensure that thread A continued.
	6. Lock write from thread B. Ensure that it waits.
	7. Unlock write from thread A and exit thread.
	8. Ensure that thread B resumed.
*/

class RWLockTest
{
private:
	enum ThreadCommand
	{
		kLockRead,
		kLockWrite,
		kUnlockRead,
		kUnlockWrite,
		kExitThread,
	};

	class CommandThread : public Thread
	{
		Semaphore _Semaphore;
		ThreadCommand _Cmd;
		Event _Done;

		RWLock &_Lock;

	public:
		CommandThread(RWLock &lck)
			: _Lock(lck)
		{
			Start();
		}

	private:
		virtual int ThreadBody() override
		{
			for (;;)
			{
				_Semaphore.Wait();
				switch(_Cmd)
				{
				case kLockRead:
					_Lock.LockRead();
					break;
				case kLockWrite:
					_Lock.LockWrite();
					break;
				case kUnlockRead:
					_Lock.UnlockRead();
					break;
				case kUnlockWrite:
					_Lock.UnlockWrite();
					break;
				case kExitThread:
					return 0;				
				}

				_Done.Set();
			}
		}

	public:
		bool Wait()
		{
			return _Done.TryWait(100);
		}

		bool Exec(ThreadCommand cmd, bool wait)
		{
			_Done.Reset();
			_Cmd = cmd;
			_Semaphore.Signal();
			if (wait)
				return Wait();
			else
				return true;
		}
	};


	RWLock _Lock;
	CommandThread _A, _B;

public:
	RWLockTest()
		: _A(_Lock)
		, _B(_Lock)
	{
	}

public:
	bool Test()
	{
		TEST_ASSERT(_A.Exec(kLockRead, true));
		TEST_ASSERT(_B.Exec(kLockRead, true));

		TEST_ASSERT(_A.Exec(kUnlockRead, true));
		TEST_ASSERT(!_A.Exec(kLockWrite, true));

		TEST_ASSERT(_B.Exec(kUnlockRead, true));
		TEST_ASSERT(_A.Wait());

		TEST_ASSERT(!_B.Exec(kLockWrite, true));
		TEST_ASSERT(_A.Exec(kUnlockWrite, true));
		TEST_ASSERT(_B.Wait());

		TEST_ASSERT(_A.Exec(kExitThread, false));
		TEST_ASSERT(_B.Exec(kExitThread, false));

		TEST_ASSERT(_A.Join(100));
		TEST_ASSERT(_B.Join(100));

		return true;
	}
};

/*class AutoResetAtomicityEvaluator : public TwoThreadBase
{
	Event _T1Done, _T2Done;
	Event _AutoReset1, _AutoReset2;

public:
	AutoResetAtomicityEvaluator()
		: _T1Done(0, kAutoResetEvent)
		, _T2Done(0, kAutoResetEvent)
		, _AutoReset1(0, kAutoResetEvent)
		, _AutoReset2(0, kAutoResetEvent)
	{
	}

	virtual int Thread1Body()
	{
		for (;;)
		{
			_AutoReset1.Wait();
			//TestPrint("[1.1]");
			_T1Done.Set();
			_AutoReset2.Wait();
			//TestPrint("[1.2]");
			_T1Done.Set();
		}
		return 0;
	}

	virtual int Thread2Body()
	{
		for (;;)
		{
			_AutoReset1.Wait();
			//TestPrint("[2.1]");
			_T2Done.Set();
			_AutoReset2.Wait();
			//TestPrint("[2.2]");
			_T2Done.Set();
		}
		return 0;
	}

public:
	bool Test()
	{
		_Thread1.Start();
		_Thread2.Start();

		Thread::Sleep(100);

		for (int i = 0; ; i++)
		{
			_AutoReset1.Set();
			_AutoReset1.Reset();
			//Thread::Sleep(100);
			_AutoReset1.Set();
			_AutoReset1.Reset();

			TEST_ASSERT(_T1Done.TryWait(100));
			TEST_ASSERT(_T2Done.TryWait(100));

			_AutoReset2.Set();
			//Thread::Sleep(100);
			_AutoReset2.Set();

			TEST_ASSERT(_T1Done.TryWait(100));
			TEST_ASSERT(_T2Done.TryWait(100));
		}
		return true;
	}

};
*/

bool TestSyncAPI()
{
	TestPrint("Testing synchronization primitives...\nBazisLib::Event");
	if (!BasicEventTest().Test())
		return false;

	if (!TwoThreadEventTest().Test())
		return false;

	//AutoResetAtomicityEvaluator().Test();

	TestPrint(" BazisLib::Mutex");
	if (!MutexTest().Test())
		return false;

	TestPrint(" BazisLib::Semaphore");
	if (!SemaphoreTest().Test())
		return false;

	TestPrint(" BazisLib::RWLock");
	if (!RWLockTest().Test())
		return false;

	TestPrint("\nSynchronization primitive tests succeeded.\n");
	return true;
}