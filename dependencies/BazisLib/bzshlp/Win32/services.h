#pragma once
#ifndef UNDER_CE

#include "cmndef.h"
#include "../../bzscore/buffer.h"
#include "../../bzscore/status.h"
#include "../../bzscore/file.h"
#include "../../bzscore/cmndef.h"
#include "../../bzscore/string.h"
#include "../../bzscore/sync.h"

namespace BazisLib
{
	namespace Win32
	{
		//! Represents a Windows Service Handle, allowing to start, stop and control a Windows Service
		class Service
		{
		private:
			SC_HANDLE m_hServiceHandle;
			ActionStatus m_Status;

		private:
			void Initialize(SC_HANDLE hSCManager, LPCTSTR lpServiceName, DWORD dwAccess = SERVICE_ALL_ACCESS, ActionStatus *pStatus = NULL)
			{
				m_hServiceHandle = OpenService(hSCManager, lpServiceName, dwAccess);
				if (m_hServiceHandle)
					m_Status = MAKE_STATUS(Success);
				else
					m_Status = MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				if (pStatus)
					*pStatus = COPY_STATUS(m_Status);
			}

			Service(const Service &srv)
			{
				ASSERT(FALSE);
			}

			void operator=(const Service &svc)
			{
				ASSERT(FALSE);
			}

		private:
			template <int _InfoType, class _InfoStruct> ActionStatus DoQueryConfig2(TypedBuffer<_InfoStruct> *pBuf)
			{
				if (!pBuf)
					return MAKE_STATUS(InvalidPointer);
				if (!m_Status.Successful())
					return COPY_STATUS(m_Status);
				ASSERT(m_hServiceHandle);
				DWORD dwBytesNeeded = 0;
				QueryServiceConfig2(m_hServiceHandle, _InfoType, (LPBYTE)pBuf->GetData(), (DWORD)pBuf->GetAllocated(), &dwBytesNeeded);
				if (!dwBytesNeeded)
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				if (pBuf->GetAllocated() >= dwBytesNeeded)
				{
					pBuf->SetSize(dwBytesNeeded);
					return MAKE_STATUS(Success);
				}
				pBuf->EnsureSizeAndSetUsed(dwBytesNeeded);
				if (!QueryServiceConfig2(m_hServiceHandle, _InfoType, (LPBYTE)pBuf->GetData(), dwBytesNeeded, &dwBytesNeeded))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			void AttachAnotherServiceHandle(SC_HANDLE hService)
			{
				if (m_hServiceHandle)
					CloseServiceHandle(m_hServiceHandle);
				m_hServiceHandle = hService;
				m_Status = MAKE_STATUS(Success);
			}

			friend class ServiceControlManager;

		public:

			Service(SC_HANDLE hSCManager, LPCTSTR lpServiceName, DWORD dwAccess = SERVICE_ALL_ACCESS, ActionStatus *pStatus = NULL)
			{
				Initialize(hSCManager, lpServiceName, dwAccess, pStatus);
			}

			Service(ActionStatus OpenError)
				: m_Status(COPY_STATUS(OpenError))
				, m_hServiceHandle(0)
			{
			}

			Service()
				: m_hServiceHandle(0)
			{
			}

			inline Service(class ServiceControlManager *pMgr, LPCTSTR ServiceName, DWORD dwAccess = SERVICE_ALL_ACCESS, ActionStatus *pStatus = NULL);
			inline ActionStatus OpenAnotherService(class ServiceControlManager *pMgr, LPCTSTR ServiceName, DWORD dwAccess = SERVICE_ALL_ACCESS, ActionStatus *pStatus = NULL);

			~Service()
			{
				if (m_hServiceHandle)
					CloseServiceHandle(m_hServiceHandle);
			}

		public:
			ActionStatus QueryStatus(SERVICE_STATUS_PROCESS *pProcess)
			{
				if (!m_Status.Successful())
					return COPY_STATUS(m_Status);
				ASSERT(m_hServiceHandle);
				if (!pProcess)
					return MAKE_STATUS(InvalidPointer);
				DWORD dwNeeded = 0;
				if (!QueryServiceStatusEx(m_hServiceHandle, SC_STATUS_PROCESS_INFO, (LPBYTE)pProcess, sizeof(SERVICE_STATUS_PROCESS), &dwNeeded))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				ASSERT(dwNeeded == sizeof(SERVICE_STATUS_PROCESS));
				return MAKE_STATUS(Success);
			}

			//! Returns current service state (SERVICE_RUNNING/etc)
			/*! This method returns the current service state.
				\return If the method fails, a value of -1 is returned.
						If the method succeeds, it returns a Win32 service status value,
						such as SERVICE_RUNNING, SERVICE_STOPPED, etc.
			*/
			DWORD QueryState(ActionStatus *pStatus = NULL)
			{
				SERVICE_STATUS_PROCESS status;
				ActionStatus st = QueryStatus(&status);
				if (!st.Successful())
				{
					if (pStatus)
						*pStatus = COPY_STATUS(st);
					return -1;
				}
				ASSIGN_STATUS(pStatus, Success);
				return status.dwCurrentState;
			}

			ActionStatus Start(unsigned argc = 0, LPCTSTR *argv = NULL)
			{
				if (!m_Status.Successful())
					return COPY_STATUS(m_Status);
				ASSERT(m_hServiceHandle);
				if (!StartService(m_hServiceHandle, argc, argv))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			ActionStatus Control(DWORD dwControl, LPSERVICE_STATUS pServiceStatus = NULL)
			{
				if (!m_Status.Successful())
					return COPY_STATUS(m_Status);
				ASSERT(m_hServiceHandle);
				SERVICE_STATUS srvStatus;
				if (!ControlService(m_hServiceHandle, dwControl, pServiceStatus ? pServiceStatus : &srvStatus))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			ActionStatus Continue(LPSERVICE_STATUS pServiceStatus = NULL) {return Control(SERVICE_CONTROL_CONTINUE, pServiceStatus); }
			ActionStatus Interrogate(LPSERVICE_STATUS pServiceStatus = NULL) {return Control(SERVICE_CONTROL_INTERROGATE, pServiceStatus); }
			ActionStatus Pause(LPSERVICE_STATUS pServiceStatus = NULL) {return Control(SERVICE_CONTROL_PAUSE, pServiceStatus); }
			ActionStatus Stop(LPSERVICE_STATUS pServiceStatus = NULL) {return Control(SERVICE_CONTROL_STOP, pServiceStatus); }

			ActionStatus QueryConfig(TypedBuffer<QUERY_SERVICE_CONFIG> *pConfig)
			{
				if (!pConfig)
					return MAKE_STATUS(InvalidPointer);
				if (!m_Status.Successful())
					return COPY_STATUS(m_Status);
				ASSERT(m_hServiceHandle);
				DWORD dwBytesNeeded = 0;
				QueryServiceConfig(m_hServiceHandle, NULL, 0, &dwBytesNeeded);
				if (!dwBytesNeeded)
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				pConfig->EnsureSizeAndSetUsed(dwBytesNeeded);
				if (!QueryServiceConfig(m_hServiceHandle, *pConfig, dwBytesNeeded, &dwBytesNeeded))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			ActionStatus ChangeConfig(DWORD dwServiceType			= SERVICE_NO_CHANGE,
									  DWORD dwStartType				= SERVICE_NO_CHANGE,
									  DWORD dwErrorControl			= SERVICE_NO_CHANGE,
									  LPCTSTR lpBinaryPathName		= NULL,
									  LPCTSTR lpLoadOrderGroup		= NULL,
									  LPDWORD lpdwTagId				= NULL,
									  LPCTSTR lpDependencies		= NULL,
									  LPCTSTR lpServiceStartName	= NULL,
									  LPCTSTR lpPassword			= NULL,
									  LPCTSTR lpDisplayName			= NULL)
			{
				if (!m_Status.Successful())
					return COPY_STATUS(m_Status);
				ASSERT(m_hServiceHandle);
				if (!ChangeServiceConfig(m_hServiceHandle, dwServiceType, dwStartType, dwErrorControl, lpBinaryPathName, lpLoadOrderGroup, lpdwTagId, lpDependencies, lpServiceStartName, lpPassword, lpDisplayName))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}


			ActionStatus QueryConfig2(TypedBuffer<SERVICE_DESCRIPTION> *pBuffer) {return DoQueryConfig2<SERVICE_CONFIG_DESCRIPTION>(pBuffer); }
			ActionStatus QueryConfig2(TypedBuffer<SERVICE_FAILURE_ACTIONS> *pBuffer) {return DoQueryConfig2<SERVICE_CONFIG_FAILURE_ACTIONS>(pBuffer); }

#ifdef SERVICE_CONFIG_DELAYED_AUTO_START_INFO
			ActionStatus QueryConfig2(TypedBuffer<SERVICE_DELAYED_AUTO_START_INFO> *pBuffer) {return DoQueryConfig2<SERVICE_CONFIG_DELAYED_AUTO_START_INFO>(pBuffer); }
			ActionStatus QueryConfig2(TypedBuffer<SERVICE_FAILURE_ACTIONS_FLAG > *pBuffer) {return DoQueryConfig2<SERVICE_CONFIG_FAILURE_ACTIONS_FLAG>(pBuffer); }
			ActionStatus QueryConfig2(TypedBuffer<SERVICE_PRESHUTDOWN_INFO> *pBuffer) {return DoQueryConfig2<SERVICE_CONFIG_PRESHUTDOWN_INFO>(pBuffer); }
			ActionStatus QueryConfig2(TypedBuffer<SERVICE_REQUIRED_PRIVILEGES_INFO> *pBuffer) {return DoQueryConfig2<SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO>(pBuffer); }
			ActionStatus QueryConfig2(TypedBuffer<SERVICE_SID_INFO > *pBuffer) {return DoQueryConfig2<SERVICE_CONFIG_SERVICE_SID_INFO>(pBuffer); }
#endif

		public:

			ActionStatus ChangeConfig2(LPVOID lpData, size_t dataSize, unsigned dataType)
			{
				if (!lpData)
					return MAKE_STATUS(InvalidPointer);
				if (!m_Status.Successful())
					return COPY_STATUS(m_Status);
				(void)dataSize;
				if (!ChangeServiceConfig2(m_hServiceHandle, dataType, lpData))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			ActionStatus SetDescription(LPCTSTR lpDescription)
			{
				SERVICE_DESCRIPTION desc = {(LPTSTR)lpDescription};
				return ChangeConfig2(&desc, sizeof(desc), SERVICE_CONFIG_DESCRIPTION);
			}

			ActionStatus Delete()
			{
				if (!m_Status.Successful())
					return COPY_STATUS(m_Status);
				if (!DeleteService(m_hServiceHandle))
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				return MAKE_STATUS(Success);
			}

			ActionStatus GetOpenStatus()
			{
				return m_Status;
			}

		public:
			static const TCHAR *GetStateName(DWORD dwState);
			static const TCHAR *GetServiceTypeName(DWORD DriverType);
			static const TCHAR *GetStartTypeName(DWORD DriverType);
			static String TranslateDriverPath(LPCTSTR lpReportedPath);
			static String Win32PathToDriverPath(LPCTSTR lpWin32Path, bool AddSystemRootPrefix = false, bool DoNotAddDosDevicesPrefix = false);
			//static FilePath GetServiceBinaryPath(LPCTSTR lpReportedPath, bool ChopCommandLineArgs = true, bool ProbeWin32Paths = true);

		};

		//! Represents a Service Control Manager allowing to create, delete and enumerate services
		class ServiceControlManager
		{
		private:
			SC_HANDLE m_hSCManager;
			enum {INVALID_CONTINUATION_HANDLE_VALUE = -1};

			friend class Service;

		public:
			class _iterator_init
			{
			private:
				DWORD m_dwServiceType;
				DWORD m_dwServiceState;
				DWORD m_dwContinueEnumHandle;
				const ServiceControlManager *m_pManager;
				DWORD m_dwThisEntry;

			private:
				friend class ServiceControlManager;
				friend class iterator;
				
				_iterator_init(const ServiceControlManager *pManager, DWORD dwContHandle, DWORD dwType, DWORD dwState)
					: m_pManager(pManager)
					, m_dwContinueEnumHandle(dwContHandle)
					, m_dwServiceType(dwType)
					, m_dwServiceState(dwState)
					, m_dwThisEntry(0)
				{
				}
			};

			class iterator : private _iterator_init
			{
			private:
				TypedBuffer<ENUM_SERVICE_STATUS_PROCESS> m_pData;
				ActionStatus m_DataRetrieveStatus;
				DWORD m_dwTotalEntries;

			public:
				iterator(const _iterator_init &init)
					: _iterator_init(init)
					, m_DataRetrieveStatus(MAKE_STATUS(NotInitialized))
					, m_dwTotalEntries(0)
				{
					ASSERT(m_pManager);
				}

				friend class ServiceControlManager;

			private:
				void ProvideData()
				{
					if (m_dwTotalEntries)
						return;
					if (m_dwContinueEnumHandle == INVALID_CONTINUATION_HANDLE_VALUE)
						return;
					ASSERT(m_pManager);
					DWORD dwNeeded = 0;
					m_dwThisEntry = 0;
					for (;;)
					{
						m_dwTotalEntries = 0;
						EnumServicesStatusEx(m_pManager->m_hSCManager,
							SC_ENUM_PROCESS_INFO,
							m_dwServiceType,
							m_dwServiceState,
							(LPBYTE)m_pData.GetData(),
							(DWORD)m_pData.GetAllocated(),
							&dwNeeded,
							&m_dwTotalEntries,
							&m_dwContinueEnumHandle,
							NULL);
						if (m_dwTotalEntries || (GetLastError() != ERROR_MORE_DATA))
							break;
						m_pData.EnsureSizeAndSetUsed(dwNeeded);
					}
					if (!m_dwTotalEntries)
						m_dwContinueEnumHandle = INVALID_CONTINUATION_HANDLE_VALUE;
					m_DataRetrieveStatus = MAKE_STATUS(ActionStatus::FromLastError());
				}

			public:
				operator ENUM_SERVICE_STATUS_PROCESS *()
				{
					ProvideData();
					ASSERT(m_dwThisEntry < m_dwTotalEntries);
					return &m_pData[m_dwThisEntry];
				}

				ENUM_SERVICE_STATUS_PROCESS *operator->()
				{
					return *this;
				}

				bool Valid()
				{
					ProvideData();
					return m_dwTotalEntries && (m_dwContinueEnumHandle != INVALID_CONTINUATION_HANDLE_VALUE);
				}

				bool operator !=(const _iterator_init &it)
				{
					if ((it.m_dwContinueEnumHandle == INVALID_CONTINUATION_HANDLE_VALUE) || (m_dwContinueEnumHandle == INVALID_CONTINUATION_HANDLE_VALUE))
						return m_dwContinueEnumHandle != INVALID_CONTINUATION_HANDLE_VALUE;
					return (m_dwContinueEnumHandle != it.m_dwContinueEnumHandle) || (m_dwThisEntry != it.m_dwThisEntry);
				}

				bool operator ==(const _iterator_init &it)
				{
					return !(*this == it);
				}

				iterator &operator++()
				{
					ProvideData();
					if (m_dwThisEntry < (m_dwTotalEntries - 1))
						m_dwThisEntry++;
					else
					{
						if (m_dwTotalEntries && !m_dwContinueEnumHandle)
							m_dwContinueEnumHandle = INVALID_CONTINUATION_HANDLE_VALUE;
						m_dwTotalEntries = 0;
						m_dwThisEntry = 0;
					}
					return *this;
				}

			public:

				DWORD QueryState(ActionStatus *pStatus = NULL)
				{
					if (!Valid())
					{
						if (pStatus)
							*pStatus = COPY_STATUS(m_DataRetrieveStatus);
						return -1;
					}
					return Service(m_pManager->m_hSCManager, m_pData[m_dwThisEntry].lpServiceName, SERVICE_QUERY_STATUS).QueryState(pStatus);
				}

				ActionStatus Start(unsigned argc = 0, LPCTSTR *argv = NULL)
				{
					if (!Valid())
						return COPY_STATUS(m_DataRetrieveStatus);
					return Service(m_pManager->m_hSCManager, m_pData[m_dwThisEntry].lpServiceName, SERVICE_START).Start(argc, argv);
				}

				ActionStatus Control(DWORD dwControl, LPSERVICE_STATUS pServiceStatus = NULL, DWORD Access = SERVICE_PAUSE_CONTINUE | SERVICE_INTERROGATE | SERVICE_START | SERVICE_STOP | SERVICE_USER_DEFINED_CONTROL)
				{
					if (!Valid())
						return COPY_STATUS(m_DataRetrieveStatus);
					return Service(m_pManager->m_hSCManager, m_pData[m_dwThisEntry].lpServiceName, SERVICE_START).Control(dwControl, pServiceStatus);
				}

				ActionStatus Continue(LPSERVICE_STATUS pServiceStatus = NULL) {return Control(SERVICE_CONTROL_CONTINUE, pServiceStatus, SERVICE_PAUSE_CONTINUE); }
				ActionStatus Interrogate(LPSERVICE_STATUS pServiceStatus = NULL) {return Control(SERVICE_CONTROL_INTERROGATE, pServiceStatus, SERVICE_INTERROGATE); }
				ActionStatus Pause(LPSERVICE_STATUS pServiceStatus = NULL) {return Control(SERVICE_CONTROL_PAUSE, pServiceStatus, SERVICE_PAUSE_CONTINUE); }
				ActionStatus Stop(LPSERVICE_STATUS pServiceStatus = NULL) {return Control(SERVICE_CONTROL_STOP, pServiceStatus, SERVICE_STOP); }

			};
		public:
			_iterator_init begin(DWORD dwType = SERVICE_DRIVER | SERVICE_WIN32, DWORD dwState = SERVICE_STATE_ALL)
			{
				return _iterator_init(this, 0, dwType, dwState);
			}

			_iterator_init end()
			{
				return _iterator_init(this, INVALID_CONTINUATION_HANDLE_VALUE, 0, 0);
			}

		public:
			ServiceControlManager(DWORD dwAccess = SC_MANAGER_ALL_ACCESS, LPCTSTR lpMachineName = NULL, ActionStatus *pStatus = NULL)
			{
				m_hSCManager = OpenSCManager(lpMachineName, NULL, dwAccess);
				if (m_hSCManager)
					ASSIGN_STATUS(pStatus, Success);
				else
					ASSIGN_STATUS(pStatus, ActionStatus::FromLastError(UnknownError));
			}

			~ServiceControlManager()
			{
				if (m_hSCManager)
					CloseServiceHandle(m_hSCManager);
			}

			ActionStatus CreateService(OUT Service *pCreatedService,
										LPCTSTR lpServiceName,
										LPCTSTR lpDisplayName = NULL,
										DWORD dwDesiredAccess = SERVICE_ALL_ACCESS,
										DWORD dwServiceType = SERVICE_KERNEL_DRIVER,
										DWORD dwStartType = SERVICE_DEMAND_START,
										DWORD dwErrorControl = SERVICE_ERROR_NORMAL,
										LPCTSTR lpBinaryPathName = NULL,
										LPCTSTR lpLoadOrderGroup = NULL,
										LPDWORD lpdwTagId = NULL,
										LPCTSTR lpDependencies = NULL,
										LPCTSTR lpServiceStartName = NULL,
										LPCTSTR lpPassword = NULL)
			{
				if (!m_hSCManager)
					return MAKE_STATUS(NotInitialized);
				SC_HANDLE hServ = ::CreateService(m_hSCManager, 
												 lpServiceName,
												 lpDisplayName,
												 dwDesiredAccess,
												 dwServiceType,
												 dwStartType,
												 dwErrorControl,
												 lpBinaryPathName,
												 lpLoadOrderGroup,
												 lpdwTagId,
												 lpDependencies,
												 lpServiceStartName,
												 lpPassword);
				if (!hServ)
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
				if (pCreatedService)
					pCreatedService->AttachAnotherServiceHandle(hServ);
				else
					CloseServiceHandle(hServ);
				return MAKE_STATUS(Success);
			}

		};

		inline Service::Service(class ServiceControlManager *pMgr, LPCTSTR ServiceName, DWORD dwAccess, ActionStatus *pStatus)
		{
			Initialize(pMgr->m_hSCManager, ServiceName, dwAccess, pStatus);
		}

		inline ActionStatus Service::OpenAnotherService(class ServiceControlManager *pMgr, LPCTSTR ServiceName, DWORD dwAccess, ActionStatus *pStatus)
		{
			if (m_hServiceHandle)
				CloseServiceHandle(m_hServiceHandle);
			m_hServiceHandle = NULL;
			Initialize(pMgr->m_hSCManager, ServiceName, dwAccess, pStatus);
			return m_Status;
		}

		class OwnProcessServiceBase
		{
		private:
			String m_ServiceName;
			SERVICE_STATUS_HANDLE m_hThisService;
			bool m_bInteractive;

		private:
			static OwnProcessServiceBase *s_pService;
			static VOID WINAPI sServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
			static DWORD WINAPI sHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

		public:
			OwnProcessServiceBase(LPCTSTR lpServiceName, bool Interactive = true);
			~OwnProcessServiceBase();

			int EntryPoint();

		protected:
			ActionStatus ReportState(DWORD dwState = SERVICE_RUNNING, DWORD dwControlsAccepted = SERVICE_ACCEPT_STOP, DWORD dwExitCode = 0, DWORD dwSpecificError = 0, DWORD dwCheckPoint = 0, DWORD dwWaitHint = 0);

		protected:
			virtual ActionStatus OnStartService(int argc, LPTSTR *argv)=0;
			virtual ActionStatus OnCommand(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData)=0;
		};

		class OwnProcessServiceWithWorkerThread : public OwnProcessServiceBase
		{
		private:
			Event m_StopEvent;
			bool m_bReportRunningAutomatically;

		public:
			OwnProcessServiceWithWorkerThread(LPCTSTR lpServiceName, bool Interactive = true, bool ReportRunningAutomatically = true)
				: OwnProcessServiceBase(lpServiceName, Interactive)
				, m_bReportRunningAutomatically(ReportRunningAutomatically)
			{
			}

		protected:
			virtual ActionStatus OnStartService(int argc, LPTSTR *argv) override;
			virtual ActionStatus OnCommand(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData) override;

		protected:
			virtual ActionStatus WorkerThreadBody(HANDLE hStopEvent)=0;

		};
	}
}
#endif

