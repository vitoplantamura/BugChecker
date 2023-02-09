#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "Utils.h"

class Cmd_COLOR : public Cmd
{
public:

	virtual const CHAR* GetId() { return "COLOR"; }
	virtual const CHAR* GetDesc() { return "Display, set or reset the screen colors"; }
	virtual const CHAR* GetSyntax() { return "COLOR [normal bold reverse help line]|[reset]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		auto args = TokenizeArgs(params.cmd, "COLOR");

		if (args.size() == 1)
		{
			CHAR currentColors[64];
			::sprintf(currentColors, "Colors are %02X %02X %02X %02X %02X", Wnd::nrmClr, Wnd::bldClr, Wnd::rvrClr, Wnd::hlpClr, Wnd::hrzClr);
			Print(currentColors);
			co_return;
		}
		else if (args.size() == 2)
		{
			eastl::vector<BYTE> colors;
			if (Utils::AreStringsEqualI(args[1].c_str(), "reset"))
			{
				colors.push_back(0x07);
				colors.push_back(0x70);
				colors.push_back(0x71);
				colors.push_back(0x30);
				colors.push_back(0x02);
			}
			else
			{
				colors = ParseListOfBytesArgs("COLOR", params.cmd);
			}
			if (colors.size() != 5)
			{
				Print("Arguments number mismatch.");
				co_return;
			}
			Wnd::nrmClr = colors[0];
			Wnd::bldClr = colors[1];
			Wnd::rvrClr = colors[2];
			Wnd::hlpClr = colors[3];
			Wnd::hrzClr = colors[4];
			Root::I->CodeWindow.SyntaxColorAll();
			params.result = CmdParamsResult::RedrawUi;
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
