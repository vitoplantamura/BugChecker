#pragma once

#include "BugChecker.h"

#include "Wnd.h"

class RegsWnd : public Wnd
{
public:

	VOID DrawRegs(BYTE* context, BOOLEAN is32bitCompat);

	VOID InvalidateOldContext()
	{
		oldCtxValid = FALSE;
	}

private:

	CONTEXT oldCtx;
	BOOLEAN oldCtxValid = FALSE;
};
