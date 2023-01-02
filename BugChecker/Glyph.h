#pragma once

#include "BugChecker.h"

class Glyph
{
public:
	static void Draw(IN BYTE bTextChar, IN BYTE bTextColor, IN ULONG ulX, IN ULONG ulY, IN PVOID pvPrimary, ULONG ulTargetStartX = 0, ULONG ulTargetStartY = 0, BOOLEAN hasCursor = FALSE);
	static void DrawStr(IN const CHAR* pcStr, IN BYTE bTextColor, IN ULONG ulX, IN ULONG ulY, IN PVOID pvPrimary);

	static VOID UpdateScreen(ULONG x, ULONG y, ULONG w, ULONG h);
	static eastl::pair<ULONG, ULONG> GetScreenDim();
};
