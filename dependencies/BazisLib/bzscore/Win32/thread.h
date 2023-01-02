#pragma once

namespace BazisLib
{
	namespace _ThreadPrivate
	{
		//! Represents a Win32 thread.
		/*! The _BasicThread template class represents a single Win32 thread and contains methods for controlling it.
			The thread body is defined through the template parameter (DescendantClass::ThreadStarter static function
			is called, passing <b>this</b> as a parameter). That allows declaring VT-less thread classes. However,
			as VT is not a huge overhead, a classical pure method-based BazisLib::Win32::Thread class is provided.
		*/
		template <class DescendantClass> class _BasicThread
		{
		private:
			HANDLE m_hThread;
			DWORD m_ThreadID;

		private:
			_BasicThread(const _BasicThread<DescendantClass> &thr)
			{
				ASSERT(FALSE);
			}

		protected:
			typedef ULONG _ThreadBodyReturnType;
			static inline _ThreadBodyReturnType _ReturnFromThread(void *pThreadArg, ULONG retcode)
			{
				return retcode;
			}

		public:
			//! Initializes the thread object, but does not start the thread
			/*! This constructor does not create actual Win32 thread. To create the thread and to start it, call
				the Start() method.
			*/
			_BasicThread() :
			  m_hThread(INVALID_HANDLE_VALUE)
			{
			}

			bool Start()
			{
				if (m_hThread != INVALID_HANDLE_VALUE)
					return false;
				m_hThread = CreateThread(NULL, 0, DescendantClass::ThreadStarter, this, 0, &m_ThreadID);
				if (m_hThread == INVALID_HANDLE_VALUE)
					return false;
				return true;
			}

			bool Terminate()
			{
				if (m_hThread == INVALID_HANDLE_VALUE)
					return false;
				return (TerminateThread(m_hThread, -1) == TRUE);
			}

			//! Waits for the thread to complete
			bool Join(unsigned timeoutInMsec = INFINITE)
			{
				if (m_ThreadID == GetCurrentThreadId())
					return true;
				if (m_hThread == INVALID_HANDLE_VALUE)
					return false;
				DWORD result = WaitForSingleObject(m_hThread, timeoutInMsec);
				return result == WAIT_OBJECT_0;
			}

			bool IsRunning()
			{
				if (m_hThread == INVALID_HANDLE_VALUE)
					return false;
				return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
			}

			bool Suspend()
			{
				if (m_hThread == INVALID_HANDLE_VALUE)
					return false;
				return (SuspendThread(m_hThread) == TRUE);
			}

			bool Resume()
			{
				if (m_hThread == INVALID_HANDLE_VALUE)
					return false;
				return (ResumeThread(m_hThread) == TRUE);
			}

			int GetReturnCode(bool *pbSuccess = NULL)
			{
				if (pbSuccess)
					*pbSuccess = false;
				if (m_hThread == INVALID_HANDLE_VALUE)
					return -1;
				if (WaitForSingleObject(m_hThread, 0) != WAIT_OBJECT_0)
					return -1;
				DWORD code = -1;
				if (!GetExitCodeThread(m_hThread, &code))
					return -1;
				if (pbSuccess)
					*pbSuccess = true;
				return code;
			}

			unsigned GetThreadID()
			{
				return m_ThreadID;
			}

			HANDLE GetHandle()
			{
				return m_hThread;
			}

			~_BasicThread()
			{
				Join();
				if (m_hThread != INVALID_HANDLE_VALUE)
					CloseHandle(m_hThread);
			}

		public:
			static void Sleep(unsigned Millisecs)
			{
				::Sleep(Millisecs);
			}


		};
	}
}
