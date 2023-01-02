#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"

class Cmd_BD : public Cmd
{
public:

	virtual const CHAR* GetId() { return "BD"; }
	virtual const CHAR* GetDesc() { return "Disable one or more breakpoints."; }
	virtual const CHAR* GetSyntax() { return "BD list|*"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto nums = ParseListOfDecsArgs("BD", params.cmd, Root::I->BreakPoints.size());

		if (!nums.size())
			co_return;

		// disable the specified breakpoints.

		for (auto n : nums)
		{
			if (n >= Root::I->BreakPoints.size()) // should never happen.
				break;

			BreakPoint& bp = *(Root::I->BreakPoints.begin() + n);

			bp.skip = TRUE;
		}

		// return and ask to refresh the code window.

		params.result = CmdParamsResult::RefreshCodeAndRegsWindows;

		co_return;
	}
};

REGISTER_COMMAND(Cmd_BD)
