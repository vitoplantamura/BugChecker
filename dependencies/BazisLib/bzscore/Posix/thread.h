#pragma once
#include <pthread.h>
#include <unistd.h>
#include "../sync.h"

namespace BazisLib
{
	namespace _ThreadPrivate
	{
		template <class DescendantClass> class _BasicThread
		{
		private:
			pthread_t m_Thread;
			Event m_ThreadExitedEvent;

			bool m_Created;

			bool m_JoinSucceeded;
			void *m_ExitCode;

		private:
			_BasicThread(const _BasicThread<DescendantClass> &thr)
			{
				ASSERT(FALSE);
			}

		protected:
			typedef void *_ThreadBodyReturnType;

			static inline _ThreadBodyReturnType _ReturnFromThread(void *pArg, int retcode)
			{
				((_BasicThread *)pArg)->m_ThreadExitedEvent.Set();
				pthread_exit((void *)retcode);
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
				int result = pthread_create(&m_Thread, NULL, DescendantClass::ThreadStarter, this);
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
				if (pthread_equal(m_Thread, pthread_self()))
					return true;

				if (m_JoinSucceeded)
					return true;

				int result = pthread_join(m_Thread, &m_ExitCode);

				if (result == 0)
					m_JoinSucceeded = true;

				return (result == 0);
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
				if (pthread_equal(m_Thread, pthread_self()))
					return true;
				return !m_ThreadExitedEvent.TryWait(0);
			}

			int GetReturnCode(bool *pbSuccess = NULL)
			{
				if (m_JoinSucceeded)
					return (int)(intptr_t)m_ExitCode;

				if (pbSuccess)
					*pbSuccess = false;
				if (!Join(0))
					return -1;
				if (pbSuccess)
					*pbSuccess = true;
				return (int)(intptr_t)m_ExitCode;
			}


			unsigned GetThreadID()
			{
				return m_Thread;
			}

			~_BasicThread()
			{
				Join();
				if (m_Created)
					pthread_detach(m_Thread);
			}

		public:
			static void Sleep(unsigned Millisecs)
			{
				usleep(Millisecs * 1000);
			}


		};
	}
}
