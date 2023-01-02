#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "../status.h"
#include "ntstatusdbg.h"

using namespace BazisLib;

String ActionStatus::FormatErrorCode(CommonErrorType code)
{
#ifdef _DEBUG
	const wchar_t *pwszStatus = BazisLib::DDK::MapNTStatus((NTSTATUS)code);
	if (pwszStatus)
		return pwszStatus;
#endif
	wchar_t wsz[512];
	_snwprintf(wsz, sizeof(wsz)/sizeof(wsz[0]), L"NTSTATUS code %08X", code);
	return wsz;
}
#endif