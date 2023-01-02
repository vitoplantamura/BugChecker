#pragma once

#include "BugChecker.h"

#include "Cmd.h"

#include "EASTL/string.h"

class InputLine
{
public:

	eastl::string text;

	ULONG offset = 0;

	LONG historyPos = -1;

	eastl::string helpText;

public:

	VOID AddChar(CHAR c);
	eastl::pair<eastl::string, Cmd*> Enter();
	VOID BackSpace();
	VOID Left();
	VOID Right();
	VOID Up();
	VOID Down();
	VOID Del();
	VOID Home();
	VOID End();
	VOID Tab(BOOLEAN modifierKeyState);

	VOID Finalize(BOOLEAN redrawWnds = TRUE);

	VOID RedrawCallback();

private:

	class TabData
	{
	public:
		eastl::string text;
		ULONG offset;
		LONG CursorX;
		LONG index;
		LONG numOfItems;

		eastl::string* t; // only used when enumerating symbols.
		eastl::string* r; // only used when enumerating symbols.
	};

	TabData tabData;
};
