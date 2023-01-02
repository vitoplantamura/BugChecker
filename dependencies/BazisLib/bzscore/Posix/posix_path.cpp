#include "stdafx.h"
#ifdef BZSLIB_UNIXWORLD
#include "../path.h"

using namespace BazisLib;

String Path::GetSpecialDirectoryLocation(SpecialDirectoryType dir)
{
	switch(dir)
	{
	case dirTemp:
		return _T("/tmp");
	default:
		return _T("");
	}
}

#endif
