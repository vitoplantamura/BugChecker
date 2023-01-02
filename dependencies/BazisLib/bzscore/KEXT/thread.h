#pragma once
#include "../sync.h"
#include <IOKit/IOLib.h>

#define THREAD_STARTER_ADDITIONAL_ARGS , wait_result_t

namespace BazisLib
{
	namespace _ThreadPrivate
	{
		template <class DescendantClass> class _BasicThread
		{
		private:
			thread_t m_Thread;
			Event m_ThreadExitedEvent;

			bool m_Created;

			bool m_JoinSucceeded;

			void *m_ExplicitExitCodeNotSupportedByOS;

		private:
			_BasicThread(const _BasicThread<DescendantClass> &thr)
			{
				ASSERT(FALSE);
			}

		protected:
			typedef void _ThreadBodyReturnType;

			static inline _ThreadBodyReturnType _ReturnFromThread(void *pArg, int retcode)
			{
				((_BasicThread *)pArg)->m_ThreadExitedEvent.Set();
				((_BasicThread *)pArg)->m_ExplicitExitCodeNotSupportedByOS = (void *)retcode;
				thread_terminate(current_thread());
			}

		private:
			//! Wait until a given thread completely terminates, or a given timeout expires.
			/*!
				\remarks The function is based on undocumented and unsupported assumption regarding the location and meaning
						 of the state field inside the XNU thread structure.
						 As there is no other documented way to wait for a thread to terminate, we use this one.
			*/
			static void DoLoopUntilKernelThreadTerminates(thread_t thread)
			{
				struct thread_head
				{
					queue_chain_t	links;
					processor_t		runq;
					void *wait_queue;
					event64_t		wait_event;
					integer_t		options;

					uintptr_t sched_lock;
					uintptr_t wake_lock;
					boolean_t			wake_active;
					int					at_safe_point;
					uint32_t				reason;
					wait_result_t		wait_result;
					thread_continue_t	continuation;
					void				*parameter;

					struct funnel_lock	*funnel_lock;
					int				    funnel_state;
					vm_offset_t     	kernel_stack;
					vm_offset_t			reserved_stack;
					int					state;
				};

				thread_head *pHead = (thread_head *)thread;
				int iter = 0;
				const int kMaxIterations = 10;

				while (!(pHead->state & 0x20))	//TH_TERMINATE2
				{
					IOSleep(1);
					if (iter++ >= kMaxIterations)
						break;	//Something's wrong, probably using a debug kernel that has different offset of thread::state.
				}
			}

		public:
			//! Initializes the thread object, but does not start the thread
			/*! This constructor does not create actual Win32 thread. To create the thread and to start it, call
				the Start() method.
			*/
			_BasicThread()
				: m_Created(false)
				, m_JoinSucceeded(false)
			{
			}

			bool Start()
			{
				if (m_Created)
					return false;
				int result = kernel_thread_start(DescendantClass::ThreadStarter, this, &m_Thread);
				if (result != 0)
					return false;
				m_Created = true;
				return true;
			}

/*			bool Terminate()
			{
				return false;	//WARNING! If added, ensure that the m_ThreadExitedEvent is set
			}*/

			//! Waits for the thread to complete
			bool Join()
			{
				if (!m_Created)
					return false;
				if (m_Thread == current_thread())
					return true;

				if (m_JoinSucceeded)
					return true;

				m_ThreadExitedEvent.Wait();
				DoLoopUntilKernelThreadTerminates(m_Thread);
				m_JoinSucceeded = true;

				return true;
			}

			bool Join(unsigned timeoutInMsec)
			{
				if (timeoutInMsec == -1)
					return Join();

				if (!m_ThreadExitedEvent.TryWait(timeoutInMsec))
					return false;

				return Join();
			}

			bool IsRunning()
			{
				if (!m_Created || m_JoinSucceeded)
					return false;
				if (m_Thread == current_thread())
					return true;
				return !m_ThreadExitedEvent.TryWait(0);
			}

			int GetReturnCode(bool *pbSuccess = NULL)
			{
				if (m_JoinSucceeded)
					return (int)(intptr_t)m_ExplicitExitCodeNotSupportedByOS;

				if (pbSuccess)
					*pbSuccess = false;
				if (!Join(0))
					return -1;
				if (pbSuccess)
					*pbSuccess = true;
				return (int)(intptr_t)m_ExplicitExitCodeNotSupportedByOS;
			}

			unsigned GetThreadID()
			{
				return m_Thread;
			}

			~_BasicThread()
			{
				Join();
				if (m_Created)
					thread_deallocate(m_Thread);
			}

		public:
			static void Sleep(unsigned Millisecs)
			{
				IOSleep(Millisecs);
			}


		};
	}
}
