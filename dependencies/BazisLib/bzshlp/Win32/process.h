#pragma once
#include "../../bzscore/status.h"
#include <shellapi.h>

namespace BazisLib
{
	namespace Win32
	{
		class Process
		{
		public:
			static ActionStatus RunCommandLineSynchronously(LPTSTR lpszCmdLine, DWORD *pExitCode = NULL, WORD wShowCmd = SW_SHOW)
			{
				STARTUPINFO startupInfo = {0, };
				startupInfo.cb = sizeof(startupInfo);
				startupInfo.wShowWindow = wShowCmd;
				PROCESS_INFORMATION processInfo = {0, };
				if (!CreateProcess(NULL, lpszCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
					return MAKE_STATUS(ActionStatus::FailFromLastError());
				WaitForSingleObject(processInfo.hProcess, INFINITE);
				if (pExitCode)
					GetExitCodeProcess(processInfo.hProcess, pExitCode);
				CloseHandle(processInfo.hProcess);
				CloseHandle(processInfo.hThread);
				return MAKE_STATUS(Success);
			}
		private:
			PROCESS_INFORMATION m_ProcessInfo;

		private:
			void Create(LPCTSTR lpCommandLine, WORD wShowCmd, STARTUPINFO *pStartupInfo = NULL, bool InheritHandles = false, ActionStatus *pStatus = NULL)
			{
				memset(&m_ProcessInfo, 0, sizeof(m_ProcessInfo));
				m_ProcessInfo.hProcess = m_ProcessInfo.hThread = INVALID_HANDLE_VALUE;

				STARTUPINFO startupInfo = {0, };
				startupInfo.cb = sizeof(startupInfo);
				startupInfo.wShowWindow = wShowCmd;

				DynamicString cmdLineCopy = lpCommandLine;
				if (!CreateProcess(NULL, const_cast<TCHAR *>(cmdLineCopy.c_str()), NULL, NULL, InheritHandles, 0, NULL, NULL, pStartupInfo ? pStartupInfo : &startupInfo, &m_ProcessInfo))
					ASSIGN_STATUS(pStatus, ActionStatus::FailFromLastError());
				else
					ASSIGN_STATUS(pStatus, Success);

				if (!m_ProcessInfo.hProcess)
					m_ProcessInfo.hProcess = INVALID_HANDLE_VALUE;

				if (!m_ProcessInfo.hThread)
					m_ProcessInfo.hThread = INVALID_HANDLE_VALUE;
			}

			void CreateUsingShellExecute(LPCTSTR lpCommandLine, WORD wShowCmd, LPCTSTR lpVerb = _T("open"), ActionStatus *pStatus = NULL)
			{
				memset(&m_ProcessInfo, 0, sizeof(m_ProcessInfo));
				m_ProcessInfo.hProcess = m_ProcessInfo.hThread = INVALID_HANDLE_VALUE;

				String exeFile;
				LPCTSTR pArgs = NULL;
				if (lpCommandLine[0] == '\"')
				{
					const TCHAR *p = _tcschr(lpCommandLine + 1, '\"');
					if (p)
					{
						if (p[1] == ' ')
							pArgs = p + 2;
						else
							pArgs = NULL;
						exeFile = DynamicString(lpCommandLine + 1, p - lpCommandLine - 1);
					}
					else
					{
						pArgs = NULL;
						exeFile = DynamicString(lpCommandLine + 1);
					}
				}
				else
				{
					const TCHAR *p = _tcschr(lpCommandLine + 1, ' ');
					if (p)
					{
						pArgs = p + 1;
						exeFile = DynamicString(lpCommandLine, p - lpCommandLine);
					}
					else
					{
						pArgs = NULL;
						exeFile = DynamicString(lpCommandLine);
					}
				}

				SHELLEXECUTEINFO execInfo = {sizeof(SHELLEXECUTEINFO), };
				execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
				execInfo.lpFile = exeFile.c_str();
				execInfo.lpParameters = pArgs;
				execInfo.lpVerb = lpVerb;
				execInfo.nShow = wShowCmd;

				if (!ShellExecuteEx(&execInfo))
					ASSIGN_STATUS(pStatus, ActionStatus::FailFromLastError());
				else
				{
					m_ProcessInfo.hProcess = execInfo.hProcess ? execInfo.hProcess : INVALID_HANDLE_VALUE;
					ASSIGN_STATUS(pStatus, Success);
				}
			}

		public:
			Process(LPCTSTR lpCommandLine, WORD wShowCmd, ActionStatus *pStatus = NULL)
			{
				Create(lpCommandLine, wShowCmd, NULL, false, pStatus);
			}

			Process(LPCTSTR lpCommandLine, LPSTARTUPINFO pStartupInfo, bool InheritHandles, ActionStatus *pStatus = NULL)
			{
				Create(lpCommandLine, pStartupInfo ? pStartupInfo->wShowWindow : SW_SHOW, pStartupInfo, InheritHandles, pStatus);
			}

			Process(LPCTSTR lpCommandLine, ActionStatus *pStatus = NULL)
			{
				Create(lpCommandLine, SW_SHOW, NULL, false, pStatus);
			}

			Process(LPCTSTR lpCommandLine, TCHAR *pShellExecuteVerb = NULL, ActionStatus *pStatus = NULL, WORD wCmdShow = SW_SHOW)
			{
				if (pShellExecuteVerb )
					CreateUsingShellExecute(lpCommandLine, wCmdShow, pShellExecuteVerb, pStatus);
				else
					Create(lpCommandLine, wCmdShow, NULL, false, pStatus);
			}

			~Process()
			{
				if (m_ProcessInfo.hThread != INVALID_HANDLE_VALUE)
					CloseHandle(m_ProcessInfo.hThread);
				if (m_ProcessInfo.hProcess != INVALID_HANDLE_VALUE)
					CloseHandle(m_ProcessInfo.hProcess);
			}

			unsigned ProcessID() { return m_ProcessInfo.dwProcessId; }
			unsigned MainThreadID() { return m_ProcessInfo.dwThreadId; }

			HANDLE GetProcessHandle() { return m_ProcessInfo.hProcess; }
			HANDLE GetMainThreadHandle() { return m_ProcessInfo.hThread; }

			HANDLE DetachProcessHandle() { HANDLE h = m_ProcessInfo.hProcess; m_ProcessInfo.hProcess = INVALID_HANDLE_VALUE; return h; }
			HANDLE DetachMainThreadHandle() { HANDLE h = m_ProcessInfo.hThread; m_ProcessInfo.hThread = INVALID_HANDLE_VALUE; return h; }

			bool Wait(unsigned Timeout = INFINITE)
			{
				return WaitForSingleObject(m_ProcessInfo.hProcess, Timeout) == WAIT_OBJECT_0;
			}

			bool IsRunning() {return !Wait(0);}

			unsigned GetExitCode()
			{
				DWORD dwCode = -1;
				GetExitCodeProcess(m_ProcessInfo.hProcess, &dwCode);
				return dwCode;
			}
		};
	}
}