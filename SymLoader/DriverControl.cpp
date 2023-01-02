#include "DriverControl.h"

#include "Utils.h"

eastl::string DriverControl::Start()
{
	// open the Service Control Manager on this computer.

	SC_HANDLE schSCManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (!schSCManager)
		return "Unable to open Service Control Manager.";

	// on scope exit: close the Service Control Manager.

	scope_guard __CloseSCM__([=]() { ::CloseServiceHandle(schSCManager); });

	// STEP 1: the service must be non-existent or stopped.

	{
		// open the service.

		auto schService = ::OpenService(schSCManager,
			BC_DRIVER_NAME,
			SERVICE_ALL_ACCESS
		);

		if (schService)
		{
			// on scope exit: close the service.

			scope_guard __CloseServ__([=]() { ::CloseServiceHandle(schService); });

			// check the current state of the service.

			SERVICE_STATUS_PROCESS ssp;
			DWORD dwBytesNeeded;

			if (!::QueryServiceStatusEx(
				schService,
				SC_STATUS_PROCESS_INFO,
				(LPBYTE)&ssp,
				sizeof(SERVICE_STATUS_PROCESS),
				&dwBytesNeeded))
			{
				return "QueryServiceStatusEx failed.";
			}

			if (ssp.dwCurrentState != SERVICE_STOPPED)
				return "The BugChecker driver is already running.";

			// STEP 2: remove the service.

			if (!::DeleteService(schService))
				return "DeleteService failed.";
		}
	}

	// STEP 3: create the service.

	{
		// create the service: the call may fail because of the service pending deletion.

		eastl::wstring sys = Utils::GetExeFolderWithBackSlash() + L"BugChecker_";

		if (Utils::Is32bitOS())
			sys += L"32bit";
		else
			sys += L"64bit";

		sys += L".sys";

		if (!Utils::FileExists(sys.c_str()))
			return "File not found: \"" + Utils::WStringToAscii(sys) + "\".";

		SC_HANDLE schService = NULL;

		for (int i = 0; i < 4 * 10; i++) // 10 seconds
		{
			schService = ::CreateService(schSCManager,
				BC_DRIVER_NAME,        // name of service
				BC_DRIVER_NAME,        // name to display
				SERVICE_ALL_ACCESS,    // desired access
				SERVICE_KERNEL_DRIVER, // service type
				SERVICE_DEMAND_START,  // start type
				SERVICE_ERROR_NORMAL,  // error control type
				sys.c_str(),           // service's binary
				NULL,                  // no load ordering group
				NULL,                  // no tag identifier
				NULL,                  // no dependencies
				NULL,                  // LocalSystem account
				NULL                   // no password
			);

			if (schService)
				break;
			else
				::Sleep(250);
		}

		if (!schService)
			return "CreateService failed.";

		// on scope exit: close the handle.

		scope_guard __CloseServ__([=]() { ::CloseServiceHandle(schService); });

		// start the driver.

		if (!::StartService(schService, 0, NULL))
			return "StartService failed.\r\n\r\nPlease make sure that Driver Signature Enforcement is disabled. You can disable DSE at boot time pressing F8.";
	}

	// return success.

	return "";
}

eastl::string DriverControl::Stop()
{
	// open the Service Control Manager on this computer.

	SC_HANDLE schSCManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (!schSCManager)
		return "Unable to open Service Control Manager.";

	// on scope exit: close the Service Control Manager.

	scope_guard __CloseSCM__([=]() { ::CloseServiceHandle(schSCManager); });

	// open the service.

	auto schService = ::OpenService(schSCManager,
		BC_DRIVER_NAME,
		SERVICE_ALL_ACCESS
	);

	if (!schService)
		return "Unable to open BugChecker service.";

	// on scope exit: close the service.

	scope_guard __CloseServ__([=]() { ::CloseServiceHandle(schService); });

	// check the current state of the service.

	SERVICE_STATUS_PROCESS ssp;
	DWORD dwBytesNeeded;

	if (!::QueryServiceStatusEx(
		schService,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&ssp,
		sizeof(SERVICE_STATUS_PROCESS),
		&dwBytesNeeded))
	{
		return "QueryServiceStatusEx failed.";
	}

	if (ssp.dwCurrentState != SERVICE_RUNNING)
		return "The BugChecker driver is not running.";

	// send a stop code to the service.

	if (!::ControlService(
		schService,
		SERVICE_CONTROL_STOP,
		(LPSERVICE_STATUS)&ssp))
	{
		return "ControlService failed.";
	}

	// return success.

	return "";
}
