#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"

class Cmd_X : public Cmd
{
public:

	virtual const CHAR* GetId() { return "X"; }
	virtual const CHAR* GetDesc() { return "Exit from the BugChecker screen."; }
	virtual const CHAR* GetSyntax() { return "X (no parameters)"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto args = TokenizeArgs(params.cmd, "X");

		if (args.size() != 1)
		{
			Print("Too many arguments.");
			co_return;
		}

		params.result = CmdParamsResult::Continue;
		co_return;
	}
};

REGISTER_COMMAND(Cmd_X)
