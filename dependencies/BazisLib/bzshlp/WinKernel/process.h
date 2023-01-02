#pragma once
#include <Bazislib/bzscore/status.h>

#pragma region Undocumented API
extern "C" 
{
	NTSTATUS __stdcall ZwQueryInformationProcess(
	__in          HANDLE ProcessHandle,
	__in          PROCESSINFOCLASS ProcessInformationClass,
	__out         PVOID ProcessInformation,
	__in          ULONG ProcessInformationLength,
	__out_opt     PULONG ReturnLength
	);
}
#pragma endregion

namespace BazisLib
{
	namespace DDK
	{
		class ProcessAccessor
		{
		private:
			HANDLE _hProcess;
			PKPROCESS _pProcess;

		public:
			//! This function accepts a process ID, not the process handle!
			ProcessAccessor(HANDLE ProcessID, ActionStatus *pStatus = NULL);
			~ProcessAccessor();

			bool Valid() { return _hProcess != 0; }
			ActionStatus Terminate(NTSTATUS exitCode)
			{
				NTSTATUS status = ZwTerminateProcess(_hProcess, exitCode);
				return MAKE_STATUS(ActionStatus::FromNTStatus(status));
			}

			ActionStatus QueryBasicInfo(PROCESS_BASIC_INFORMATION *pInfo);
			
			ActionStatus ReadMemory(const void *pAddressInProcess, void *pSystemBuffer, size_t size);
			
			template <class _Struct> ActionStatus ReadStruct(const void *pAddressInProcess, _Struct *pStruct)
			{
				return ReadMemory(pAddressInProcess, pStruct, sizeof(_Struct));
			}

			struct StartInfo
			{
				String ImagePath;
				String CommandLine;

				bool Valid() {return !ImagePath.empty();}
			};

			StartInfo QueryStartInfo();

		private:
			ActionStatus DoQueryStartInfoFromProcessContext(PPROCESS_BASIC_INFORMATION pBasicInfo, StartInfo *pInfo);
		};
	}
}