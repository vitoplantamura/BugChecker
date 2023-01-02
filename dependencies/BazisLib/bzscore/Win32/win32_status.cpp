#include "stdafx.h"
#ifdef BZSLIB_WINUSER
#include "../status.h"

using namespace BazisLib;

String ActionStatus::FormatErrorCode(CommonErrorType code)
{
	if (HRESULT_FACILITY(code) == FACILITY_ITF)
	{
		size_t idx = code & 0xFFFF;
		if (idx < _CustomStatusObjectsInternal.size())
			return _CustomStatusObjectsInternal[idx]->GetStatusText();
	}

	LPTSTR lpBuffer = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				  NULL,
				  (DWORD)code,
				  0,
				  (LPTSTR)&lpBuffer,
				  0,
				  NULL);
	if (!lpBuffer)
	{
		TCHAR tsz[512];
#ifdef UNDER_CE
		_sntprintf(tsz, sizeof(tsz)/sizeof(tsz[0]), _T("HRESULT code 0x%08X"), code);
#else
		_sntprintf_s(tsz, sizeof(tsz)/sizeof(tsz[0]), _TRUNCATE, _T("HRESULT code 0x%08X"), code);
#endif
		return tsz;
	}
	else
	{
		LPTSTR lpCrlf = _tcsstr(lpBuffer, _T("\r\n"));
		if (lpCrlf)
			lpCrlf[0] = 0;
	}
	String text = lpBuffer;
	LocalFree(lpBuffer);
	return text;
}
#endif