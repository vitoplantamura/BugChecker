#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "crtFill.h"

static VOID CommandImpl(CmdParams& params, LONG* py)
{
	const CHAR* cmdId = py == &Root::I->DisasmCodeDivLineY ? "WD" : py == &Root::I->CodeLogDivLineY ? "WS" : NULL;

	if (!cmdId) return;

	auto args = Cmd::TokenizeArgs(params.cmd, cmdId);

	if (args.size() == 1)
	{
		*py = -*py;

		Wnd::CheckDivLineYs();

		params.result = CmdParamsResult::RedrawUi;

		return;
	}
	else if (args.size() != 2)
	{
		Cmd::Print("Too many arguments.");
		return;
	}

	eastl::string val = args[1];

	val.trim();

	for (CHAR c : val)
		if (!(c >= '0' && c <= '9'))
		{
			Cmd::Print("Invalid argument.");
			return;
		}

	ULONG newValue = (ULONG)::BC_strtoui64(val.c_str(), NULL, 10);

	if (newValue < 3)
	{
		Cmd::Print("Argument must be greater than or equal to 3.");
		return;
	}
	else if (newValue > 160)
	{
		Cmd::Print("Argument must be less than or equal to 160.");
		return;
	}

	if (*py < 0)
	{
		*py = -*py;
		Wnd::CheckDivLineYs();
	}

	LONG first = 0;
	LONG last = 0;
	eastl::vector<DivLine> v = Wnd::GetVisibleDivLines(first, last);

	for (int i = 0; i < v.size(); i++)
	{
		if (v[i].py == py)
		{
			*py = *v[i - 1].py + newValue + 1;

			Wnd::CheckDivLineYs(py);

			params.result = CmdParamsResult::RedrawUi;

			break;
		}
	}
}

class Cmd_WD : public Cmd
{
public:

	virtual const CHAR* GetId() { return "WD"; }
	virtual const CHAR* GetDesc() { return "Toggle the Disassembler window or set its size."; }
	virtual const CHAR* GetSyntax() { return "WD [window-size]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		::CommandImpl(params, &Root::I->DisasmCodeDivLineY);
		co_return;
	}
};

class Cmd_WS : public Cmd
{
public:

	virtual const CHAR* GetId() { return "WS"; }
	virtual const CHAR* GetDesc() { return "Toggle the Script window or set its size."; }
	virtual const CHAR* GetSyntax() { return "WS [window-size]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		::CommandImpl(params, &Root::I->CodeLogDivLineY);
		co_return;
	}
};

class Cmd_WR : public Cmd
{
public:

	virtual const CHAR* GetId() { return "WR"; }
	virtual const CHAR* GetDesc() { return "Toggle the Registers window."; }
	virtual const CHAR* GetSyntax() { return "WR (no parameters)"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto args = TokenizeArgs(params.cmd, "WR");

		if (args.size() != 1)
		{
			Print("Too many arguments.");
			co_return;
		}

		Root::I->RegsDisasmDivLineY = -Root::I->RegsDisasmDivLineY;

		Wnd::CheckDivLineYs();

		params.result = CmdParamsResult::RedrawUi;

		co_return;
	}
};

REGISTER_COMMAND(Cmd_WR)
REGISTER_COMMAND(Cmd_WD)
REGISTER_COMMAND(Cmd_WS)
