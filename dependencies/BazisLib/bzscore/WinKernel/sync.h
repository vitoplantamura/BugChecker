#pragma once

#include "kmemory.h"
#include "../autolock.h"

#pragma region API definition
extern "C" 
{
	NTSTATUS NTAPI
		ZwCreateEvent(
		OUT PHANDLE  EventHandle,
		IN ACCESS_MASK  DesiredAccess,
		IN POBJECT_ATTRIBUTES  ObjectAttributes OPTIONAL,
		IN EVENT_TYPE  EventType,
		IN BOOLEAN  InitialState
		);

	NTSTATUS NTAPI
		ZwSetEvent(
		IN HANDLE  EventHandle,
		OUT PLONG  PreviousState OPTIONAL
		);


	NTSTATUS NTAPI
		ZwClearEvent(
		IN HANDLE  EventHandle);

	NTSTATUS
		NTAPI
		ZwDuplicateObject(
		__in HANDLE SourceProcessHandle,
		__in HANDLE SourceHandle,
		__in_opt HANDLE TargetProcessHandle,
		__out_opt PHANDLE TargetHandle,
		__in ACCESS_MASK DesiredAccess,
		__in ULONG HandleAttributes,
		__in ULONG Options
		);

	NTSYSAPI
		NTSTATUS
		NTAPI
		ZwWaitForSingleObject(
		__in HANDLE Handle,
		__in BOOLEAN Alertable,
		__in_opt PLARGE_INTEGER Timeout
		);
}
#pragma endregion


namespace BazisLib
{
#ifdef _DEBUG
	class SafeBool
	{
	private:
		bool _Value;

	public:
		SafeBool(bool val)
			: _Value(val)
		{
		}

		operator bool() {return _Value;}
		bool operator!() {return !_Value;}

	private:
		operator int() {ASSERT(FALSE); return 0;}
		operator NTSTATUS() {ASSERT(FALSE); return STATUS_NOT_SUPPORTED;}
	};
#else
	typedef bool SafeBool;
#endif


	namespace DDK
	{
		//! Represents the kernel KEVENT object.
		class KEvent
		{
		private:
			KEVENT *m_pEvent;

		public:
			/*! Initializes the KEvent object. The arguments are similar to KeInitializeEvent() routine.
				\param Type Specifies event type. Can be \b NotificationEvent or \b SynchronizationEvent.
							\b SynchronizationEvent is automatically reset after successful wait,
							\b NotificationEvent is not.
				\param State Specifies initial event state.

				\remarks The real KEVENT object is always allocated from non-paged memory, so the KEvent
						 object can be safely allocated in stack.
			    \remarks Always check whether the event initialization completed using Valid() method before
						 using it. The event initialization may fail if the system is out of non-paged memory.
			*/

			KEvent(EVENT_TYPE Type, bool State = false)
			{
				m_pEvent = (KEVENT *)npagednew(sizeof(KEVENT));
				KeInitializeEvent(m_pEvent, Type, State);
			}


		public:
			KEvent(bool initialState = false, EventType eventType = kManualResetEvent)
			{
				m_pEvent = (KEVENT *)npagednew(sizeof(KEVENT));
				C_ASSERT(NotificationEvent == kManualResetEvent);
				C_ASSERT(SynchronizationEvent == kAutoResetEvent);
				KeInitializeEvent(m_pEvent, (EVENT_TYPE)eventType, initialState);
			}

			~KEvent()
			{
				delete m_pEvent;
			}

			bool Valid()
			{
				return (m_pEvent != NULL);
			}

			void Set()
			{
				SetEx();
			}

			void Reset()
			{
				ASSERT(Valid());
				KeClearEvent(m_pEvent);
			}

			bool IsSet()
			{
				ASSERT(Valid());
				return (KeReadStateEvent(m_pEvent) != FALSE);
			}

			void Wait()
			{
				WaitEx();
			}

			bool TryWait(unsigned timeoutInMilliseconds = 0)
			{
				return WaitWithTimeoutEx(((LONGLONG)timeoutInMilliseconds) * 10000) == STATUS_SUCCESS;
			}

		public:	//NT-specific methods
			
			bool SetEx(KPRIORITY Increment = IO_NO_INCREMENT, bool NextCallIsWaitXXX = false)
			{
				ASSERT(Valid());
				return (KeSetEvent(m_pEvent, Increment, NextCallIsWaitXXX) != FALSE);
			}

			/*! Waits for the event to become signaled. The wait operation is not time-limited and
				will not end by timeout.
			*/
			NTSTATUS WaitEx(KWAIT_REASON WaitReason = Executive,
						  KPROCESSOR_MODE WaitMode = KernelMode,
						  bool Alertable = false)
			{
				ASSERT(Valid());
				ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				return KeWaitForSingleObject(m_pEvent, WaitReason, WaitMode, Alertable, NULL);
			}

			/*! Waits for the event to become signaled. The wait operation is always time-limited.
				\param Timeout Specifies the wait timeout in 100-nanosecond units. If this parameter
							   has the value of 0, the method returns immediately.
			*/
			NTSTATUS WaitWithTimeoutEx(LONGLONG Timeout,
									 KWAIT_REASON WaitReason = Executive,
									 KPROCESSOR_MODE WaitMode = KernelMode,
									 bool Alertable = false)
			{
				ASSERT(Valid());
				LARGE_INTEGER liTimeout;
				liTimeout.QuadPart = -Timeout;
				return KeWaitForSingleObject(m_pEvent, WaitReason, WaitMode, Alertable, &liTimeout);
			}

			/*! Allows converting KEvent class to KEVENT kernel structure. Can be passed to various routines
				such as IoBuildSynchronousFsdRequest().
				\remarks Note that the KEVENT structure will automatically be deleted together with the KEvent
						 object. That way, avoid situations when the KEVENT structure will be accessed after
						 KEvent object deletion. Note that the code patterns like the following are bad:
						 <PRE>
	NTSTATUS StartXXX()
	{
		KEvent evt;
		StartOperation((PKEVENT)evt, ...);
		return STATUS_PENDING;					//BAD!!! The evt object will be deleted now!
    }
						 </PRE>
						 In this example, the operation started by StartOperation() routine may end AFTER the evt
						 is deleted, and its call to KeSetEvent() will cause the system to crash.
			*/
			operator PKEVENT()
			{
				ASSERT(Valid());
				return m_pEvent;
			}
		};


		//! Represents a fast mutex. See ExInitializeFastMutex() for details.			
		class FastMutex
		{
		private:
			PFAST_MUTEX m_pMutex;

		public:
			FastMutex()
			{
				m_pMutex = (PFAST_MUTEX)npagednew(sizeof(FAST_MUTEX));
				if (m_pMutex)
					ExInitializeFastMutex(m_pMutex);
			}
			
			~FastMutex()
			{
				delete m_pMutex;
			}

			bool Valid()
			{
				return (m_pMutex != NULL);
			}

			void Lock()
			{
				ASSERT(Valid());
				ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
				ExAcquireFastMutex(m_pMutex);
			}

			bool TryLock()
			{
				ASSERT(Valid());
				ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
				return ExTryToAcquireFastMutex(m_pMutex) != FALSE;
			}

			void Unlock()
			{
				ASSERT(Valid());
				ExReleaseFastMutex(m_pMutex);
			}
		};

		//! Represents a standard NT mutex. See ExInitializeFastMutex() for details.			
		class KMutex
		{
		private:
			PKMUTEX m_pMutex;

		public:
			KMutex()
			{
				m_pMutex = (PKMUTEX)npagednew(sizeof(KMUTEX));
				ASSERT(m_pMutex);
				if (m_pMutex)
					KeInitializeMutex(m_pMutex, 0);
			}
			
			~KMutex()
			{
				delete m_pMutex;
			}

			bool Valid()
			{
				return (m_pMutex != NULL);
			}

			void Lock()
			{
				LockEx();
			}

			SafeBool TryLock()
			{
				return TryLockEx() == STATUS_SUCCESS;
			}

			void Unlock()
			{
				return Unlock(false);
			}

		public: //NT-specific methods

			NTSTATUS LockEx(KWAIT_REASON WaitReason = Executive,
						  KPROCESSOR_MODE WaitMode = KernelMode,
						  bool Alertable = false)
			{
				ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				return KeWaitForMutexObject(m_pMutex, WaitReason, WaitMode, Alertable, 0);
			}

			NTSTATUS TryLockEx(LONGLONG Timeout = 0,
							 KWAIT_REASON WaitReason = Executive,
						     KPROCESSOR_MODE WaitMode = KernelMode,
						     bool Alertable = false)
			{
				ASSERT(Valid());
				ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				LARGE_INTEGER liTimeout;
				liTimeout.QuadPart = Timeout;
				return KeWaitForMutexObject(m_pMutex, WaitReason, WaitMode, Alertable, &liTimeout);
			}

			void Unlock(bool NextCallIsWaitXXX)
			{
				ASSERT(Valid());
				KeReleaseMutex(m_pMutex, NextCallIsWaitXXX);
			}
		};

		//! Represents a standard NT semaphore. See KeInitializeSemaphore() for details.			
		/*! This class represents a semaphore. A semaphore is set to a signaled state when its value is non-zero.
			Each KSemaphore::Wait() call decreases the value by 1 and each KSemaphore::Signal() call increases it.
			See the IrpQueue class for usage example.
		*/
		class KSemaphore
		{
		private:
			PKSEMAPHORE m_pSemaphore;

		public:
			/*! Initializes the KSemaphore object. See KeInitializeSemaphore() for details.
				\param InitialCount Specifies the initial count of the semaphore.
				\param Limit Specifies the maximum value of the semaphore.
			*/
			KSemaphore(int InitialCount = 0, int Limit = 0x7FFFFFFF)
			{
				ASSERT(InitialCount >= 0);
				ASSERT(Limit > 0);
				m_pSemaphore = (PKSEMAPHORE)npagednew(sizeof(KSEMAPHORE));
				if (m_pSemaphore)
					KeInitializeSemaphore(m_pSemaphore, InitialCount, Limit);
			}
			
			~KSemaphore()
			{
				delete m_pSemaphore;
			}

			bool Valid()
			{
				return (m_pSemaphore != NULL);
			}

			void Wait()
			{
				WaitEx();
			}

			void Signal()
			{
				return SignalEx();
			}

			bool TryWait(unsigned timeoutInMsec = 0)
			{
				return WaitWithTimeoutEx(((LONGLONG)timeoutInMsec) * 10000) == STATUS_SUCCESS;
			}

		public:	//NT-specific methods

			NTSTATUS WaitEx(KWAIT_REASON WaitReason = Executive,
						  KPROCESSOR_MODE WaitMode = KernelMode,
						  bool Alertable = false)
			{
				ASSERT(Valid());
				ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				return KeWaitForSingleObject(m_pSemaphore, WaitReason, WaitMode, Alertable, NULL);
			}

			NTSTATUS WaitWithTimeoutEx(LONGLONG Timeout = 0,
				KWAIT_REASON WaitReason = Executive,
				KPROCESSOR_MODE WaitMode = KernelMode,
				bool Alertable = false)
			{
				ASSERT(Valid());
				ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				LARGE_INTEGER liTimeout;
				liTimeout.QuadPart = -Timeout;
				return KeWaitForSingleObject(m_pSemaphore, WaitReason, WaitMode, Alertable, &liTimeout);
			}

			void SignalEx(int Count = 1, KPRIORITY Increment = IO_NO_INCREMENT, bool NextCallIsWaitXXX = false)
			{
				ASSERT(Count > 0);
				ASSERT(Valid());
				KeReleaseSemaphore(m_pSemaphore, Increment, Count, NextCallIsWaitXXX);
			}
		};

		//! Represents a spin lock. See KeInitializeSpinLock() for details.
		/*! This class represents a spin lock. See KeInitializeSpinLock() for details.
			A spin lock is, in fact, a very fast mutex-like synchronization object that has several restrictions:
				1. When a spin lock is held, DO NOT call any routines that may cause your thread to start waiting.
				2. Avoid performing complex operations that require much time when a spin lock is held.
			\attention As the spin lock unlocking code runs at a raised IRQL, it should be allocated from non-paged pool.
					   This class checks it in the debug version.
		*/
		class KSpinLock
		{
		private:
			KSPIN_LOCK m_SpinLock;
			KIRQL m_OwnerIrql;

#ifdef _DEBUG
			bool m_bHeld;
#endif

		public:
			KSpinLock()
			{
				ASSERT(MmIsNonPagedSystemAddressValid(this)); 
				KeInitializeSpinLock(&m_SpinLock);
#ifdef _DEBUG
				m_bHeld = false;
#endif
			}
			
			void Lock()
			{
				KeAcquireSpinLock(&m_SpinLock, &m_OwnerIrql);
#ifdef _DEBUG
				ASSERT(!m_bHeld);
				m_bHeld = true;
#endif
			}

			void Unlock()
			{
#ifdef _DEBUG
				ASSERT(m_bHeld);
				m_bHeld = false;
#endif
				KeReleaseSpinLock(&m_SpinLock, m_OwnerIrql);
			}

		public:
			void *operator new(size_t size)
			{
				return npagednew(size);
			}
		};

		//! Represents a read-write lockable resource
		class KEResource
		{
		private:
			ERESOURCE m_Resource;

		public:
			KEResource()
			{
				ExInitializeResourceLite(&m_Resource);
			}

			~KEResource()
			{
				ExDeleteResourceLite(&m_Resource);
			}

			void LockRead()
			{
				ExAcquireResourceSharedLite(&m_Resource, TRUE);
			}

			void LockWrite()
			{
				ExAcquireResourceExclusiveLite(&m_Resource, TRUE);
			}

			void UnlockRead()
			{
				ExReleaseResourceLite(&m_Resource);
			}

			void UnlockWrite()
			{
				ExReleaseResourceLite(&m_Resource);
			}
		};

		class EventWithHandle
		{
			HANDLE m_hEvent;

		public:
			EventWithHandle(EVENT_TYPE type = NotificationEvent, bool initialState = false)
				: m_hEvent(NULL)
			{
				OBJECT_ATTRIBUTES attr;
				InitializeObjectAttributes(&attr, NULL, OBJ_KERNEL_HANDLE, 0, NULL);
				ZwCreateEvent(&m_hEvent, EVENT_ALL_ACCESS, &attr, type, initialState);
			}

			void Set()
			{
				ZwSetEvent(m_hEvent, NULL);
			}

			void Clear()
			{
				ZwClearEvent(m_hEvent);
			}

			HANDLE ExportHandleToCurrentProcess(ACCESS_MASK access = SYNCHRONIZE)
			{
				HANDLE hNew = 0;
				NTSTATUS status = ZwDuplicateObject(NtCurrentProcess(), m_hEvent, NtCurrentProcess(), &hNew, access, 0, 0);
				UNREFERENCED_PARAMETER(status);
				return hNew;
			}

			bool Wait(bool Alertable = false)
			{
				ASSERT(m_hEvent != 0);
				return ZwWaitForSingleObject(m_hEvent, Alertable, NULL) == STATUS_SUCCESS;
			}

			//HANDLE GetHandle() {return m_hEvent;}
		};

	}

	typedef DDK::KMutex	Mutex;	//FastMutex objects may cause problems with DPCs
	typedef DDK::KSpinLock SpinLock;
	typedef DDK::KEResource RWLock;
	typedef DDK::FastMutex FastMutex;

#define BZSLIB_SPINLOCK_AVAILABLE

	typedef DDK::KEvent Event;
	typedef DDK::KSemaphore Semaphore;

	using DDK::EventWithHandle;

	typedef FastLocker<SpinLock> SpinLockLocker;
	typedef FastUnlocker<SpinLock> SpinLockUnlocker;

	typedef FastLocker<FastMutex> FastMutexLocker;
	typedef FastUnlocker<FastMutex> FastMutexUnlocker;
}