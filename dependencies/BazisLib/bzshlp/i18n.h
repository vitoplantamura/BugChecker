#pragma once
#include "cmndef.h"

namespace BazisLib
{
	struct StringIDRecord
	{
		unsigned ID;
		const TCHAR *pString;
	};

	static inline const TCHAR *LookupLocalizedString(unsigned ID);

#ifndef _DEBUG
#ifndef BAZISLIB_DISABLE_LOCALIZATION
#define BAZISLIB_LOCALIZATION_ENABLED
#endif
#endif


#ifdef BAZISLIB_LOCALIZATION_ENABLED

	static inline const TCHAR * _tr_helper(unsigned ID, const TCHAR *pDefault)
	{
		const TCHAR *pStr = LookupLocalizedString(ID);
		return pStr ? pStr : pDefault;
	}

#define _TR(id, str) BazisLib::_tr_helper((id), _T(str))

#else

#define _TR(id, str) _T(str)
#define LOCALIZE_DIALOG(strid, dlgid, hwnd)
#define LOCALIZE_DLGITEM(strid, hwnd, item)

#endif
}