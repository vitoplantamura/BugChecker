#pragma once

#include "BugChecker.h"

#include "Wnd.h"

class LogWnd : public Wnd
{
public:

	VOID AddUserCmd(const CHAR* psz);
	VOID AddString(const CHAR* psz, BOOLEAN doAppend = FALSE, BOOLEAN refreshUi = FALSE);
	VOID Clear();

	VOID GoToEnd();

	BOOLEAN GoToEnd_Pending = FALSE;

	VOID Left();
	VOID Right();
	VOID Up();
	VOID Down();
	VOID PgUp();
	VOID PgDn();
	VOID Home();
	VOID End();

	virtual eastl::string GetLineToDraw(ULONG index);

private:

	BOOLEAN newLine = TRUE;

	LONG64 allocSize = 0;
};
