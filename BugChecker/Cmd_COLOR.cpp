#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"


class Cmd_COLOR : public Cmd
{
public:

	virtual const CHAR* GetId() { return "COLOR"; }
	virtual const CHAR* GetDesc() { return "Display or set the screen colors"; }
	virtual const CHAR* GetSyntax() { return "COLOR [normal bold reverse help line]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto args = TokenizeArgs(params.cmd, "COLOR");

		if (args.size() == 1)
		{
			Print("TODO: Display current colors");
			co_return;
		}
		else if (args.size() == 2)
		{
			auto colors = ParseListOfBytesArgs("COLOR", params.cmd, 6);
			if (colors.size() != 5)
			{
				Print("Arguments number mismatch.");
				co_return;
			}
			Print("TODO: This command is not fully  implemented");
			Wnd::hlpClr = colors[3];
			Wnd::hrzClr = colors[4];
			Wnd::DrawAll_End();
			Wnd::DrawAll_Final();
			co_return;
		}
		else {
			Print("Arguments number mismatch.");
			co_return;
		}

		co_return;
	}
};

REGISTER_COMMAND(Cmd_COLOR)
