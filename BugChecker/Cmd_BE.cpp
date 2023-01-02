#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"

class Cmd_BE : public Cmd
{
public:

	virtual const CHAR* GetId() { return "BE"; }
	virtual const CHAR* GetDesc() { return "Enable one or more breakpoints."; }
	virtual const CHAR* GetSyntax() { return "BE list|*"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto nums = ParseListOfDecsArgs("BE", params.cmd, Root::I->BreakPoints.size());

		if (!nums.size())
			co_return;

		// enable the specified breakpoints.

		for (auto n : nums)
		{
			if (n >= Root::I->BreakPoints.size()) // should never happen.
				break;

			BreakPoint& bp = *(Root::I->BreakPoints.begin() + n);

			bp.skip = FALSE;
		}

		// return and ask to refresh the code window.

		params.result = CmdParamsResult::RefreshCodeAndRegsWindows;

		co_return;
	}
};

REGISTER_COMMAND(Cmd_BE)
