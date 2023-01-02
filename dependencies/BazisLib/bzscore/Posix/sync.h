#pragma once

#include <pthread.h>
#include <semaphore.h>
#include "datetime.h"
#include "../autolock.h"
#include "SyncImpl.h"

namespace BazisLib
{
	namespace Posix
	{
		class Mutex
		{
		protected:
			pthread_mutex_t m_Mutex;
			
		public:
			Mutex()
			{
				pthread_mutexattr_t attr;
				pthread_mutexattr_init(&attr);
				pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
				pthread_mutex_init(&m_Mutex, &attr);
				pthread_mutexattr_destroy(&attr);
			}

			~Mutex()
			{
				pthread_mutex_destroy(&m_Mutex);
			}

			void Lock()
			{
				pthread_mutex_lock(&m_Mutex);
			}

			bool TryLock()
			{
				return pthread_mutex_trylock(&m_Mutex) == 0;
			}

			void Unlock()
			{
				pthread_mutex_unlock(&m_Mutex);
			}
		};

		class ConditionVariableWithMutex : public Mutex
		{
		private:
			pthread_cond_t m_Condition;
			
		public:
			ConditionVariableWithMutex()
			{
				pthread_cond_init(&m_Condition, NULL);
			}

			~ConditionVariableWithMutex()
			{
				pthread_cond_destroy(&m_Condition);
			}

			void WaitWhenLocked()
			{
				pthread_cond_wait(&m_Condition, &m_Mutex);
			}

			typedef timespec _Deadline;

			static _Deadline MsecToDeadline(unsigned msec)
			{
				timeval tv = DateTime::Now().ToTimeval();

				//1. ts = tv
				timespec ts;
				ts.tv_sec  = tv.tv_sec;
				ts.tv_nsec = tv.tv_usec * 1000;

				//2. ts += msec
				int sec = msec / 1000;
				int nsec = (msec % 1000) * 1000000;

				ts.tv_sec += sec;
				ts.tv_nsec += nsec;

				if (ts.tv_nsec >= 1000000000)
				{
					ts.tv_sec++;
					ts.tv_nsec -= 1000000000;
				}
				return ts;
			}

			bool TryWaitWhenLocked(_Deadline deadline)
			{
				return !pthread_cond_timedwait(&m_Condition, &m_Mutex, &deadline);
			}

			void Signal(bool allThreads = true)
			{
				if (allThreads)
					pthread_cond_broadcast(&m_Condition);
				else
					pthread_cond_signal(&m_Condition);
			}
		};

		
		typedef _PosixSyncImpl::PosixBasedEvent<ConditionVariableWithMutex> Event;

		class Semaphore
		{
		private:
			sem_t m_Semaphore;
		
		public:
			Semaphore(int InitialValue = 0)
			{
				sem_init(&m_Semaphore, 0, InitialValue);
			}

			~Semaphore()
			{
				sem_destroy(&m_Semaphore);
			}

			void Wait()
			{
				sem_wait(&m_Semaphore);
			}

#ifndef __APPLE__
			bool TryWait(unsigned timeout = 0)
			{
				timespec deadline;
				clock_gettime(CLOCK_REALTIME, &deadline);
			
				deadline.tv_sec += timeout / 1000;
				deadline.tv_nsec += (timeout % 1000) * 10000000LL;
				deadline.tv_sec += (deadline.tv_nsec / 1000000000);
				deadline.tv_nsec %= 1000000000;
				return !sem_timedwait(&m_Semaphore, &deadline);
			}
#endif

			void Signal()
			{
				sem_post(&m_Semaphore);
			}
		};

		class RWLock
		{
		private:
			pthread_rwlock_t m_Lock;

		public:
			RWLock()
			{
				pthread_rwlock_init(&m_Lock, NULL);
			}

			~RWLock()
			{
				pthread_rwlock_destroy(&m_Lock);
			}

			void LockRead()
			{
				pthread_rwlock_rdlock(&m_Lock);
			}

			void LockWrite()
			{
				pthread_rwlock_wrlock(&m_Lock);
			}

			void UnlockRead()
			{
				pthread_rwlock_unlock(&m_Lock);
			}

			void UnlockWrite()
			{
				pthread_rwlock_unlock(&m_Lock);
			}
		};

	}

	typedef Posix::Mutex Mutex, SpinLock, FastMutex;
#ifdef __APPLE__	//MacOS POSIX semaphore does not support TryWait() call
	typedef _PosixSyncImpl::PosixBasedSemaphore<Posix::ConditionVariableWithMutex> Semaphore;
#else
	typedef Posix::Semaphore Semaphore;
#endif

	typedef Posix::Event Event;
	typedef Posix::RWLock RWLock;
}


