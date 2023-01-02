#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "Utils.h"
#include "CrtFill.h"
#include "Wnd.h"

static VOID CommandImpl(CmdParams& params, BOOLEAN isWidth)
{
	// parse the argument.

	auto args = Cmd::TokenizeArgs(params.cmd, isWidth ? "WIDTH" : "LINES");

	if (args.size() == 1)
	{
		Cmd::Print(("Current number of display " + eastl::string(isWidth ? "columns" : "rows") + " is " +
			Utils::I64ToString(isWidth ? Root::I->WndWidth : Root::I->WndHeight) + ".").c_str());
		return;
	}
	else if (args.size() != 2)
	{
		Cmd::Print("Too many arguments.");
		return;
	}

	eastl::string v = args[1];

	v.trim();

	for (CHAR c : v)
		if (!(c >= '0' && c <= '9'))
		{
			Cmd::Print("Invalid argument.");
			return;
		}

	ULONG newValue = (ULONG)::BC_strtoui64(v.c_str(), NULL, 10);

	ULONG minValue = isWidth ? 80 : 25;

	if (newValue < minValue)
	{
		Cmd::Print(("Argument must be greater than or equal to " + Utils::I64ToString(minValue) + ".").c_str());
		return;
	}
	else if (newValue > 160)
	{
		Cmd::Print("Argument must be less than or equal to 160.");
		return;
	}

	// set the new value.

	ULONG widthChr = isWidth ? newValue : Root::I->WndWidth;
	ULONG heightChr = !isWidth ? newValue : Root::I->WndHeight;

	while (!Wnd::CheckWndWidthHeight(widthChr, heightChr))
		if (isWidth)
			widthChr--;
		else
			heightChr--;

	Wnd::SaveOrRestoreFrameBuffer(TRUE);

	Root::I->WndWidth = widthChr;
	Root::I->WndHeight = heightChr;

	Wnd::CheckDivLineYs();

	// redraw the ui.

	params.result = CmdParamsResult::RedrawUi;
}

class Cmd_WIDTH : public Cmd
{
public:

	virtual const CHAR* GetId() { return "WIDTH"; }
	virtual const CHAR* GetDesc() { return "Display or set current display columns."; }
	virtual const CHAR* GetSyntax() { return "WIDTH [columns-num]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		CommandImpl(params, TRUE);
		co_return;
	}
};

class Cmd_LINES : public Cmd
{
public:

	virtual const CHAR* GetId() { return "LINES"; }
	virtual const CHAR* GetDesc() { return "Display or set current display rows."; }
	virtual const CHAR* GetSyntax() { return "LINES [rows-num]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		CommandImpl(params, FALSE);
		co_return;
	}
};

REGISTER_COMMAND(Cmd_WIDTH)
REGISTER_COMMAND(Cmd_LINES)
