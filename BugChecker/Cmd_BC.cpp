#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"

#include "EASTL/sort.h"

class Cmd_BC : public Cmd
{
public:

	virtual const CHAR* GetId() { return "BC"; }
	virtual const CHAR* GetDesc() { return "Clear one or more breakpoints."; }
	virtual const CHAR* GetSyntax() { return "BC list|*"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto nums = ParseListOfDecsArgs("BC", params.cmd, Root::I->BreakPoints.size());

		if (!nums.size())
			co_return;

		// remove the specified breakpoints, in reverse order.

		eastl::sort(nums.begin(), nums.end(), eastl::greater<>());

		for (auto n : nums)
		{
			if (n >= Root::I->BreakPoints.size()) // should never happen.
				break;

			BreakPoint& bp = *(Root::I->BreakPoints.begin() + n);

			if (bp.handle)
				co_await BcAwaiter_StateManipulate{
				[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

					pState->ApiNumber = DbgKdRestoreBreakPointApi;

					pState->ReturnStatus = STATUS_PENDING;

					pState->u.RestoreBreakPoint.BreakPointHandle = bp.handle;
				},
				[](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

					return pState->ApiNumber == DbgKdRestoreBreakPointApi && NT_SUCCESS(pState->ReturnStatus);
				} };

			Root::I->BreakPoints.erase(&bp);
		}

		// return and ask to refresh the code window.

		params.result = CmdParamsResult::RefreshCodeAndRegsWindows;

		co_return;
	}
};

REGISTER_COMMAND(Cmd_BC)
