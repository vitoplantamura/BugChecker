#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "Utils.h"

class Cmd_U : public Cmd
{
public:

	virtual const CHAR* GetId() { return "U"; }
	virtual const CHAR* GetDesc() { return "Unassemble instructions."; }
	virtual const CHAR* GetSyntax() { return "U address|DEST"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		// parse the arguments.

		ULONG64 address = 0;

		auto args = TokenizeArgs(params.cmd, "U", "DEST");

		if (args.size() > 2)
		{
			Print("Too many arguments.");
			co_return;
		}
		else if (args.size() < 2)
		{
			Print("Too few arguments.");
			co_return;
		}
		else
		{
			if (::strlen(args[1].c_str()) == 4 && Utils::stristr(args[1].c_str(), "DEST"))
			{
				if (!params.jumpDest)
				{
					Print("Instruction at PC is not a jump instruction.");
					co_return;
				}
				else if (params.jumpDest == -1)
				{
					Print("Condition for jump instruction at PC is false (ie not jumping).");
					co_return;
				}
				else
					address = params.jumpDest;
			}
			else
			{
				eastl::pair<BOOLEAN, ULONG64> res;
				co_await BcAwaiter_Join{ ResolveArg(res, args[1].c_str(), params.context, params.contextLen, params.is32bitCompat) };

				if (!res.first)
					co_return;
				else
					address = res.second;
			}
		}

		// forward the request to the caller.

		params.result = CmdParamsResult::Unassemble;
		params.resultUL64 = address;

		co_return;
	}
};

REGISTER_COMMAND(Cmd_U)
