#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"

extern CHAR BcVersion[256];

class Cmd_VER : public Cmd
{
public:

	virtual const CHAR* GetId() { return "VER"; }
	virtual const CHAR* GetDesc() { return "Display version information."; }
	virtual const CHAR* GetSyntax() { return "VER (no parameters)"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto args = TokenizeArgs(params.cmd, "VER");

		if (args.size() != 1)
		{
			Print("Too many arguments.");
			co_return;
		}

		// print version and diagnostic info.

		Print(BcVersion);

		CHAR map[39 + 1];
		Allocator::GetPoolMap(map, sizeof(map) - 1);

		eastl::string msg = "Internal pool map: ";
		msg += map;
		Print(msg.c_str());

		co_return;
	}
};

REGISTER_COMMAND(Cmd_VER)
