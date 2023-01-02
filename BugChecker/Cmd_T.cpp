#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"

class Cmd_T : public Cmd
{
public:

	virtual const CHAR* GetId() { return "T"; }
	virtual const CHAR* GetDesc() { return "Trace one instruction."; }
	virtual const CHAR* GetSyntax() { return "T (no parameters)"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto args = TokenizeArgs(params.cmd, "T");

		if (args.size() != 1)
		{
			Print("Too many arguments.");
			co_return;
		}

		Root::I->Trace = TRUE;
		BpTrace::Get().nextAddr = 0;

		params.result = CmdParamsResult::Continue;
		co_return;
	}
};

REGISTER_COMMAND(Cmd_T)
