#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "Utils.h"
#include "Main.h"

class Cmd_P : public Cmd
{
public:

	virtual const CHAR* GetId() { return "P"; }
	virtual const CHAR* GetDesc() { return "Execute one program step."; }
	virtual const CHAR* GetSyntax() { return "P [RET]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		// parse the arguments.

		auto args = TokenizeArgs(params.cmd, "P", "RET");

		BOOLEAN ret = FALSE;

		if (args.size() > 2)
		{
			Print("Too many arguments.");
			co_return;
		}
		else if (args.size() == 2)
		{
			if (Utils::AreStringsEqualI(args[1].c_str(), "RET"))
			{
				ret = TRUE;
			}
			else
			{
				Print("Invalid argument.");
				co_return;
			}
		}

		// step over the instruction.

		co_await BcAwaiter_Join{ StepOver(params.context, params.thisInstr) };

		// should we enter in step out mode ?

		if (ret && !IsRet(params.thisInstr->mnemonic))
		{
			Root::I->StepOutThread = Root::I->StateChange.Thread;
		}

		// return to the caller.

		params.result = CmdParamsResult::Continue;
		co_return;
	}
};

REGISTER_COMMAND(Cmd_P)
