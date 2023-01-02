#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"

class Cmd_PAGEIN : public Cmd
{
public:

	virtual const CHAR* GetId() { return "PAGEIN"; }
	virtual const CHAR* GetDesc() { return "Force a page of memory to be paged in (returns control to OS)."; }
	virtual const CHAR* GetSyntax() { return "PAGEIN address"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		// parse the arguments.

		ULONG64 address = 0;

		auto args = TokenizeArgs(params.cmd, "PAGEIN");

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
				address = res.second;
		}

		// do the requested operation.

		ULONG64 eproc = 0;

		co_await BcAwaiter_Join{ Platform::GetCurrentEprocess(eproc) };

		if (!eproc)
		{
			Print("Unable to get current eprocess.");
			co_return;
		}

		BOOLEAN res;

		co_await BcAwaiter_Join{ PageIn(eproc, address, res) };

		if (!res)
			co_return;

		// return, returning control to the OS.

		params.result = CmdParamsResult::Continue;

		co_return;
	}
};

REGISTER_COMMAND(Cmd_PAGEIN)
