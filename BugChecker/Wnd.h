#pragma once

#include "BugChecker.h"

#include <EASTL/vector.h>
#include <EASTL/string.h>

class DivLine
{
public:
	LONG* py;
	BOOLEAN canBeMoved;
	LONG height;
};

class Wnd
{
public:

	static void DrawAll_Start();
	static void DrawAll_End();
	static void DrawAll_Final();

	static void Draw_InputLine(BOOLEAN eraseBackground);

	virtual void Draw();
	virtual eastl::string GetLineToDraw(ULONG index);

	ULONG destX0 = 0; // set in DrawAll_Start
	ULONG destY0 = 0; //  "  "
	ULONG destX1 = 0; //  "  "
	ULONG destY1 = 0; //  "  "

	ULONG posX = 0;
	ULONG posY = 0;

	static BYTE hlpClr; // help bar color
	static BYTE hrzClr; // title bar color

	eastl::vector<eastl::string> contents;

public:

	static ULONG StrLen(const CHAR* ptr, eastl::string* pOut = NULL);
	static const CHAR* StrCount(const CHAR* ptr, ULONG c);
	static void DrawString(const CHAR* psz, ULONG x, ULONG y, BYTE clr = 0x07, ULONG limit = 0, BYTE space = 0x20);

	static eastl::string HighlightHexNumber(eastl::string& pattern, LONG index, LONG* numOfItems);

	static VOID ChangeCursorX(LONG newX, LONG newY = -1);
	static VOID SwitchBetweenLogCode();
	static VOID SaveOrRestoreFrameBuffer(BOOLEAN direction);
	static BOOLEAN CheckWndWidthHeight(ULONG widthChr, ULONG heightChr);
	static VOID CheckDivLineYs(LONG* keep = NULL);
	static eastl::vector<DivLine> GetVisibleDivLines(LONG& first, LONG& last);

protected:

	static BOOLEAN IsHex(CHAR c);
	static BYTE ToHex(CHAR c);
	static VOID UpdateScreen();
};
