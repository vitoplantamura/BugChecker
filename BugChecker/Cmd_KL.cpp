#include "BugChecker.h"

#include "Cmd.h"
#include "Ps2Keyb.h"
#include "Utils.h"

class Cmd_KL : public Cmd
{
public:

	virtual const CHAR* GetId() { return "KL"; }
	virtual const CHAR* GetDesc() { return "Set keyboard layout."; }
	virtual const CHAR* GetSyntax() { return "KL EN|IT"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto args = TokenizeArgs(params.cmd, "KL");

		if (args.size() < 2)
		{
			Print("Too few arguments.");
			co_return;
		}
		else if (args.size() > 2)
		{
			Print("Too many arguments.");
			co_return;
		}

		auto& layout = args[1];

		if (Utils::AreStringsEqualI(layout.c_str(), "EN"))
			Ps2Keyb::Layout = "EN";
		else if (Utils::AreStringsEqualI(layout.c_str(), "IT"))
			Ps2Keyb::Layout = "IT";
		else
			Print("Unrecognized keyboard layout identifier.");

		co_return;
	}
};

REGISTER_COMMAND(Cmd_KL)
