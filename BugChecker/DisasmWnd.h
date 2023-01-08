#pragma once

#include "BugChecker.h"

#include "Wnd.h"
#include "BcCoroutine.h"
#include "SymbolFile.h"
#include "Platform.h"

class DisasmWnd : public Wnd
{
public:

	BcCoroutine Disasm(BYTE* context, BOOLEAN is32bitCompat, ULONG64 jumpDest, LONG navigate = 0) noexcept;

	VOID SetPos(ULONG64 address)
	{
		firstAddressShown = address;
		lastAddressShown = 0;

		isFirstAddressShownASymName = FALSE;
		isLastAddressShownASymName = FALSE;

		symNameLinesNum = 0;
	}

public:

	const static LONG NAVIGATE_PRESERVE_POS = 999999;

	LONG symNameLinesNum = 0;

	CHAR posInModules[350] = { 0 };
	CHAR headerText[350] = { 0 };

	BOOLEAN current32bitCompat = FALSE;

	static BYTE bldClr;
	static BYTE rvrClr;

private:

	ULONG64 firstAddressShown = 0;
	ULONG64 lastAddressShown = 0;

	BOOLEAN isFirstAddressShownASymName = FALSE;
	BOOLEAN isLastAddressShownASymName = FALSE;

private:

	ZydisFormatterFunc defaultPrintAddressAbsolute = NULL;

	static ZyanStatus ZydisFormatterPrintAddressAbsolute(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context);

	SymbolFile* symF = NULL;
	ImageDebugInfo debugInfo = {};
	eastl::string moduleName = "";

	eastl::string symbolPrologue = "", symbolEpilogue = "";
};
