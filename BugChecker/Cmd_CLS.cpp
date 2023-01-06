#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"

class Cmd_CLS : public Cmd
{
public:

	virtual const CHAR* GetId() { return "CLS"; }
	virtual const CHAR* GetDesc() { return "Clear log window."; }
	virtual const CHAR* GetSyntax() { return "CLS (no parameters)"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto args = TokenizeArgs(params.cmd, "CLS");

		if (args.size() != 1)
		{
			Print("Too many arguments.");
			co_return;
		}

		Root::I->LogWindow.Clear();

		Wnd::DrawAll_End();
		Wnd::DrawAll_Final();
		co_return;
	}
};

REGISTER_COMMAND(Cmd_CLS)
