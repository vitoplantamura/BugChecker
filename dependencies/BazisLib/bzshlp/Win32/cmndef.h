#pragma once

#if !defined(_WINDOWS_) && !defined(__WINDOWS__) && !defined(_DDK_)
#error You should include WINDOWS.H before including BZSWIN headers
#endif

namespace BazisLib
{
	namespace Win32
	{
		using namespace BazisLib;
	}
}

#include "../../bzscore/cmndef.h"

#ifdef UNDER_CE
#define _sntprintf_s	_sntprintf
#define _stscanf_s		_stscanf
#define _sntprintf_s	_sntprintf
#define _tcsncat_s		_tcsncat
#endif