#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"

class Cmd_ADDR : public Cmd
{
public:

	virtual const CHAR* GetId() { return "ADDR"; }
	virtual const CHAR* GetDesc() { return "Switch to process context (returns control to OS)."; }
	virtual const CHAR* GetSyntax() { return "ADDR eprocess"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		// parse the arguments.

		ULONG64 eprocess = 0;

		auto args = TokenizeArgs(params.cmd, "ADDR");

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
			eastl::pair<BOOLEAN, ULONG64> res;
			co_await BcAwaiter_Join{ ResolveArg(res, args[1].c_str(), params.context, params.contextLen, params.is32bitCompat) };

			if (!res.first)
				co_return;
			else
				eprocess = res.second;
		}

		// do the requested operation.

		BOOLEAN res;

		co_await BcAwaiter_Join{ PageIn(eprocess, 0, res) };

		if (!res)
			co_return;

		// return, returning control to the OS.

		params.result = CmdParamsResult::Continue;

		co_return;
	}
};

REGISTER_COMMAND(Cmd_ADDR)
