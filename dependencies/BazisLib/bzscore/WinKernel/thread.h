#pragma once

#pragma region Imported API
extern "C" NTSYSAPI NTSTATUS NTAPI ZwWaitForSingleObject(
					  __in HANDLE Handle,
					  __in BOOLEAN Alertable,
					  __in_opt PLARGE_INTEGER Timeout);

typedef struct _THREAD_BASIC_INFORMATION {
	NTSTATUS                ExitStatus;
	PVOID                   TebBaseAddress;
	CLIENT_ID               ClientId;
	KAFFINITY               AffinityMask;
	KPRIORITY               Priority;
	KPRIORITY               BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

extern "C" NTSTATUS NTAPI ZwQueryInformationThread(HANDLE ThreadHandle,
													THREADINFOCLASS ThreadInformationClass,
													PVOID ThreadInformation,
													ULONG ThreadInformationLength,
													PULONG ReturnLength);
#pragma endregion

namespace BazisLib
{
	namespace _ThreadPrivate
	{
		//! Represents a kernel thread.
		/*! The _BasicThread template class represents a single kernel thread and contains methods for controlling it.
			The thread body is defined through the template parameter (DescendantClass::ThreadStarter static function
			is called, passing <b>this</b> as a parameter). That allows declaring VT-less thread classes. However,
			as VT is not a huge overhead, a classical pure method-based BazisLib::Win32::Thread class is provided.
		*/
		template <class DescendantClass> class _BasicThread
		{
		private:
			HANDLE m_hThread;
			CLIENT_ID m_ID;

		protected:
			typedef void _ThreadBodyReturnType;
			static inline _ThreadBodyReturnType _ReturnFromThread(void *pArg, ULONG retcode)
			{
				PsTerminateSystemThread((NTSTATUS)retcode);
			}

		public:
			//! Initializes the thread object, but does not start the thread
			/*! This constructor does not create actual Win32 thread. To create the thread and to start it, call
				the Start() method.
			*/
			_BasicThread() :
			  m_hThread(NULL)
			{
				m_ID.UniqueProcess = m_ID.UniqueThread = 0;
			}

			bool Start(HANDLE hProcessToInject = NULL)
			{
				if (m_hThread)
					return false;
				OBJECT_ATTRIBUTES threadAttr;
				InitializeObjectAttributes(&threadAttr, NULL, OBJ_KERNEL_HANDLE, 0, NULL);
				NTSTATUS st = PsCreateSystemThread(&m_hThread, THREAD_ALL_ACCESS, &threadAttr, hProcessToInject, &m_ID, DescendantClass::ThreadStarter, this);
				if (!NT_SUCCESS(st))
					return false;
				return true;
			}

			/*bool Terminate()
			{
				return false;
			}*/

			//! Waits for the thread to complete
			bool Join()
			{
				if (!m_hThread)
					return true;
				if (PsGetCurrentThreadId() == m_ID.UniqueThread)
					return true;
				ZwWaitForSingleObject(m_hThread, FALSE, NULL);
				return true;
			}

			bool Join(unsigned timeoutInMsec)
			{
				if (!m_hThread)
					return true;
				if (PsGetCurrentThreadId() == m_ID.UniqueThread)
					return true;

				LARGE_INTEGER interval;
				interval.QuadPart = (ULONGLONG)(-((LONGLONG)timeoutInMsec) * 10000);

				NTSTATUS st = ZwWaitForSingleObject(m_hThread, FALSE, &interval);
				return st == STATUS_SUCCESS;
			}

			bool IsRunning()
			{
				if (!m_hThread)
					return false;
				LARGE_INTEGER zero = {0,};
				return ZwWaitForSingleObject(m_hThread, FALSE, &zero) == STATUS_TIMEOUT;
			}

			void Reset()
			{
				Join();
				m_hThread = NULL;
				memset(&m_ID, 0, sizeof(m_ID));
			}

/*			bool Suspend()
			{
			}

			bool Resume()
			{
			}*/

			int GetReturnCode(bool *pbSuccess = NULL)
			{
				if (!m_hThread)
				{
					if (pbSuccess)
						*pbSuccess = false;
					return -1;
				}
				THREAD_BASIC_INFORMATION info;
				NTSTATUS status = ZwQueryInformationThread(m_hThread, ThreadBasicInformation, &info, sizeof(info), NULL);
				if (!NT_SUCCESS(status))
				{
					if (pbSuccess)
						*pbSuccess = false;
					return -1;
				}
				if (pbSuccess)
					*pbSuccess = true;
				return info.ExitStatus;
			}

			unsigned GetThreadID()
			{
				return (unsigned)m_ID.UniqueThread;
			}

			~_BasicThread()
			{
				Join();
				if (m_hThread)
					ZwClose(m_hThread);
			}

		public:
			static void Sleep(unsigned Millisecs)
			{
				LARGE_INTEGER interval;
				interval.QuadPart = (ULONGLONG)(-((LONGLONG)Millisecs) * 10000);
				KeDelayExecutionThread(KernelMode, FALSE, &interval);
			}
		};
	}
}
