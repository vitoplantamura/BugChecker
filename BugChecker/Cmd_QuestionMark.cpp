#include "BugChecker.h"

#include "Cmd.h"
#include "QuickJSCppInterface.h"

class Cmd_QuestionMark : public Cmd
{
public:

	virtual const CHAR* GetId() { return "?"; }
	virtual const CHAR* GetDesc() { return "Evaluate a javascript expression."; }
	virtual const CHAR* GetSyntax() { return "? javascript-expression"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		eastl::string s = params.cmd;

		s.trim();

		if (!s.size() || s[0] != '?') // should never happen.
			co_return;

		s = s.substr(1);

		if (!s.size())
		{
			Print("Too few arguments.");
			co_return;
		}

		co_await BcAwaiter_Join{ QuickJSCppInterface::Eval(s.c_str(), params.context, params.contextLen, params.is32bitCompat) };

		co_return;
	}
};

REGISTER_COMMAND(Cmd_QuestionMark)
