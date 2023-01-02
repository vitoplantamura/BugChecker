#pragma once

#pragma once

#include "assert.h"
#include "tls.h"
#include <limits.h>

namespace BazisLib
{
	namespace Win32
	{
		class Event
		{
		private:
			HANDLE m_hEvent;

		private:
			//! Use the constructor accepting EventType instead
			Event(bool InitialState, bool AutoReset)
			{
				m_hEvent = CreateEvent(NULL, !AutoReset, InitialState, NULL);
			}

		public:
			Event(bool InitialState = false, EventType eventType = kManualResetEvent)
			{
				m_hEvent = CreateEvent(NULL, eventType == kManualResetEvent, InitialState, NULL);
			}

			~Event()
			{
				CloseHandle(m_hEvent);
			}

			void Set()
			{
				SetEvent(m_hEvent);
			}

			void Reset()
			{
				ResetEvent(m_hEvent);
			}

			bool IsSet()
			{
				return (WaitForSingleObject(m_hEvent, 0) == WAIT_OBJECT_0);
			}

			void Wait()
			{
				WaitForSingleObject(m_hEvent, INFINITE);
			}

			bool TryWait(unsigned TimeoutInMsec)
			{
				return (WaitForSingleObject(m_hEvent, TimeoutInMsec) == WAIT_OBJECT_0);
			}

			HANDLE GetHandle()
			{
				return m_hEvent;
			}
		};

		class CCriticalSection 
		{
		private:
			CRITICAL_SECTION m_Section;

		public:
			CCriticalSection() {InitializeCriticalSection(&m_Section);}
			~CCriticalSection() {DeleteCriticalSection(&m_Section);}


			 void Lock() {EnterCriticalSection(&m_Section);}
			 void Unlock() {LeaveCriticalSection(&m_Section);}
			 bool TryLock() {return TryEnterCriticalSection(&m_Section) != FALSE;}
		};

		class CSimpleRWLock
		{
		private:
			CCriticalSection m_WriteMutex;
#ifdef _DEBUG
			TLSUInt32 m_tlsCurThreadReadCount;
#endif
			
			volatile unsigned m_TotalReaderCount;
			Event m_ReadyForWrite;

		public:
			CSimpleRWLock() :
			  m_TotalReaderCount(0),
			  m_ReadyForWrite(true, kAutoResetEvent)
			{
			}

			~CSimpleRWLock()
			{
			}

			//! Locks the R/W Lock in read mode
			/*!
				\remarks This function should NOT be called while the lock is held in write mode
			*/
			void LockRead()
			{
				m_WriteMutex.Lock();
				m_TotalReaderCount++;
#ifdef _DEBUG
				m_tlsCurThreadReadCount++;
#endif
				m_ReadyForWrite.Reset();
				m_WriteMutex.Unlock();
			}

			//! Locks the R/W Lock in write mode
			/*!
				\remarks This function should NOT be called while the lock is held in read mode
			*/
			void LockWrite()
			{
#ifdef _DEBUG
				ASSERT(!m_tlsCurThreadReadCount);
#endif
				m_WriteMutex.Lock();
				for (;;)
				{
					if (!m_TotalReaderCount)
						return;
					m_WriteMutex.Unlock();
					m_ReadyForWrite.Wait();
					m_WriteMutex.Lock();
				}
			}

			void UnlockRead()
			{
				m_WriteMutex.Lock();
#ifdef _DEBUG
				int cnt = --m_tlsCurThreadReadCount;
				ASSERT(cnt >= 0);
#endif
				if (!--m_TotalReaderCount)
					m_ReadyForWrite.Set();
				m_WriteMutex.Unlock();
			}

			void UnlockWrite()
			{
				if (!m_TotalReaderCount)
					m_ReadyForWrite.Set();
				m_WriteMutex.Unlock();
			}
		};

#if (_WIN32_WINNT >= 0x0600)
		class CSRWLock
		{
		private:
			SRWLOCK m_Lock;

		public:
			CSRWLock()
			{
				InitializeSRWLock(&m_Lock);
			}

			void LockRead()
			{
				AcquireSRWLockShared(&m_Lock);
			}

			void LockWrite()
			{
				AcquireSRWLockExclusive(&m_Lock);
			}

			void UnlockRead()
			{
				ReleaseSRWLockShared(&m_Lock);
			}

			void UnlockWrite()
			{
				ReleaseSRWLockExclusive(&m_Lock);
			}
		};
#endif

		class Semaphore
		{
		private:
			HANDLE m_hSemaphore;

		public:
			Semaphore(int initialCount = 0, int maxCount = INT_MAX)
			{
				m_hSemaphore = CreateSemaphore(NULL, initialCount, maxCount, NULL);
			}

			~Semaphore()
			{
				CloseHandle(m_hSemaphore);
			}

			void Wait()
			{
				WaitForSingleObject(m_hSemaphore, INFINITE);
			}

			bool TryWait(unsigned timeoutInMsec = 0)
			{
				return (WaitForSingleObject(m_hSemaphore, timeoutInMsec) == WAIT_OBJECT_0);
			}

			void Signal(unsigned count)
			{
				ReleaseSemaphore(m_hSemaphore, count, NULL);
			}

			void Signal()
			{
				ReleaseSemaphore(m_hSemaphore, 1, NULL);
			}

			HANDLE GetHandle()
			{
				return m_hSemaphore;
			}

		};

		class Mutex
		{
		private:
			HANDLE m_hMutex;

		public:
			Mutex(LPCTSTR lpMutexName = NULL) {m_hMutex = CreateMutex(NULL, FALSE, lpMutexName);}
			~Mutex() {CloseHandle(m_hMutex);}

			void Lock() {WaitForSingleObject(m_hMutex, INFINITE);}
			void Unlock() {ReleaseMutex(m_hMutex);}
			bool TryLock() {return WaitForSingleObject(m_hMutex, 0) == WAIT_OBJECT_0;}
		};

	}

	typedef Win32::CCriticalSection Mutex;

	typedef Win32::Event			Event;
	typedef Win32::CSimpleRWLock	RWLock;

	typedef Win32::Semaphore		Semaphore;
}