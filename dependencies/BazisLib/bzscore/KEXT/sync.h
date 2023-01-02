#pragma once

#include <IOKit/IOLocks.h>
#include <libkern/OSAtomic.h>
#include "../autolock.h"

#include "../Posix/SyncImpl.h"

namespace BazisLib
{
	namespace KEXT
	{
		class NonRecursiveLock
		{
		private:
			IOLock *m_pLock;

		public:
			NonRecursiveLock()
			{
				m_pLock = IOLockAlloc();
			}

			~NonRecursiveLock()
			{
				if (m_pLock)
					IOLockFree(m_pLock);
			}

			void Lock()
			{
				IOLockLock(m_pLock);
			}

			bool TryLock()
			{
				return IOTryLock(m_pLock);
			}

			void Unlock()
			{
				IOLockUnlock(m_pLock);
			}

		public:	//MacOS-specific functionality
			wait_result_t SleepWhenLocked(void *pEvent, wait_interrupt_t interruptMode = THREAD_UNINT)
			{
				return IOLockSleep(m_pLock, pEvent, interruptMode);
			}

			wait_result_t SleepWithDeadlineWhenLocked(void *pEvent, AbsoluteTime deadline, wait_interrupt_t interruptMode = THREAD_UNINT)
			{
				return IOLockSleepDeadline(m_pLock, pEvent, deadline, interruptMode);
			}

			void Wakeup(void *pEvent, bool oneThread = false)
			{
				IOLockWakeup(m_pLock, pEvent, oneThread);
			}

		};

		class RecursiveLock
		{
		private:
			IORecursiveLock *m_pLock;

		public:
			RecursiveLock()
			{
				m_pLock = IORecursiveLockAlloc();
			}

			~RecursiveLock()
			{
				if (m_pLock)
					IORecursiveLockFree(m_pLock);
			}

			void Lock()
			{
				IORecursiveLockLock(m_pLock);
			}

			bool TryLock()
			{
				return IORecursiveLockTryLock(m_pLock);
			}

			void Unlock()
			{
				IORecursiveLockUnlock(m_pLock);
			}
		};

		class SpinLock
		{
		private:
			IOSimpleLock *m_pLock;

		public:
			SpinLock()
			{
				m_pLock = IOSimpleLockAlloc();
			}

			~SpinLock()
			{
				if (m_pLock)
					IOSimpleLockFree(m_pLock);
			}

			void Lock()
			{
				IOSimpleLockLock(m_pLock);
			}

			bool TryLock()
			{
				return IOSimpleLockTryLock(m_pLock);
			}

			void Unlock()
			{
				IOSimpleLockUnlock(m_pLock);
			}
		};

		class ConditionVariableWithMutex : public NonRecursiveLock
		{
		public:
			void WaitWhenLocked()
			{
				NonRecursiveLock::SleepWhenLocked(this);
			}

			typedef uint64_t _Deadline;

			static _Deadline MsecToDeadline(unsigned timeoutInMilliseconds)
			{
				uint64_t deadline;
				clock_interval_to_deadline(timeoutInMilliseconds, kMillisecondScale, &deadline);
				return deadline;
			}

			bool TryWaitWhenLocked(_Deadline deadline)
			{
				return SleepWithDeadlineWhenLocked(this, deadline, THREAD_UNINT) == THREAD_AWAKENED;
			}

			void Signal(bool allThreads = true)
			{
				Wakeup(this, !allThreads);
			}
		};

		/*class Event
		{
		private:
			bool _State;
			NonRecursiveLock _Lock;

		public:
			Event(bool initialState = false, EventType eventType = kManualResetEvent)
			{
				_State = initialState;
			}

			~Event()
			{
			}

			void Wait()
			{
				FastLocker<NonRecursiveLock> lck(_Lock);
				if (_State)
					return;
				_Lock.SleepWhenLocked(&_State, THREAD_UNINT);
			}

			bool TryWait(unsigned timeoutInMilliseconds)
			{
				FastLocker<NonRecursiveLock> lck(_Lock);
				if (_State)
					return true;

				uint64_t deadline;
				clock_interval_to_deadline(timeoutInMilliseconds, kMillisecondScale, &deadline);
				return _Lock.SleepWithDeadlineWhenLocked(&_State, deadline, THREAD_UNINT) == THREAD_AWAKENED;
			}

			void Set()
			{
				FastLocker<NonRecursiveLock> lck(_Lock);
				_State = true;
				_Lock.Wakeup(&_State);
			}

			void Reset()
			{
				FastLocker<NonRecursiveLock> lck(_Lock);
				_State = false;
			}
		};*/

		typedef _PosixSyncImpl::PosixBasedEvent<ConditionVariableWithMutex> Event;

		class Semaphore
		{
		private:
			semaphore_t m_Semaphore;

		public:
			Semaphore(int InitialCount = 0)
			{
				semaphore_create(current_task(), &m_Semaphore, SYNC_POLICY_FIFO, InitialCount);
			}

			~Semaphore()
			{
				semaphore_destroy(current_task(), m_Semaphore);
			}

			void Wait()
			{
				semaphore_wait(m_Semaphore);
			}

			bool TryWait(unsigned timeoutInMilliseconds = 0)
			{
				ConditionVariableWithMutex::_Deadline deadline = ConditionVariableWithMutex::MsecToDeadline(timeoutInMilliseconds);
				return !semaphore_wait_deadline(m_Semaphore, deadline);
			}

			void Signal()
			{
				semaphore_signal(m_Semaphore);
			}
		};

		class RWLock
		{
		private:
			IORWLock *m_pLock;

		public:
			RWLock()
			{
				m_pLock = IORWLockAlloc();
			}

			~RWLock()
			{
				if (m_pLock)
					IORWLockFree(m_pLock);
			}

			void LockRead()
			{
				IORWLockRead(m_pLock);
			}

			void LockWrite()
			{
				IORWLockWrite(m_pLock);
			}

			void UnlockRead()
			{
				IORWUnlock(m_pLock);
			}

			void UnlockWrite()
			{
				IORWUnlock(m_pLock);
			}
		};
	}

 	typedef KEXT::RecursiveLock Mutex, FastMutex;
	typedef KEXT::SpinLock SpinLock;
 	typedef KEXT::Semaphore Semaphore;
 	typedef KEXT::Event Event;
 	typedef KEXT::RWLock RWLock;
}


