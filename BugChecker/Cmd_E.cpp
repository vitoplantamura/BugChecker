#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "Utils.h"

template<typename T>
static BcCoroutine E(const CHAR* cmdId, CmdParams& params) noexcept
{
	// check the number of arguments.

	auto args = Cmd::TokenizeArgs(params.cmd, cmdId, "-v");

	if (args.size() < 4)
	{
		Cmd::Print("Too few arguments.");
		co_return;
	}
	else if (args.size() > 4)
	{
		Cmd::Print("Too many arguments.");
		co_return;
	}
	else if (!Utils::AreStringsEqualI(args[2].c_str(), "-v"))
	{
		Cmd::Print("Syntax error.");
		co_return;
	}

	auto dataList = Cmd::TokenizeStr(args[3], " ");

	if (!dataList.size())
	{
		Cmd::Print("Syntax error.");
		co_return;
	}

	// parse the address.

	ULONG64 address = 0;

	{
		eastl::pair<BOOLEAN, ULONG64> res;
		co_await BcAwaiter_Join{ Cmd::ResolveArg(res, args[1].c_str(), params.context, params.contextLen, params.is32bitCompat) };

		if (!res.first)
			co_return;
		else
			address = res.second;
	}

	// parse the "data-list".

	BOOLEAN trunc = FALSE;

	eastl::vector<T> data;

	for (int i = 0; i < dataList.size(); i++)
	{
		ULONG64 value64 = 0;

		eastl::pair<BOOLEAN, ULONG64> res;
		co_await BcAwaiter_Join{ Cmd::ResolveArg(res, dataList[i].c_str(), params.context, params.contextLen, params.is32bitCompat) };

		if (!res.first)
			co_return;
		else
			value64 = res.second;

		T v = (T)value64;

		if ((ULONG64)v != value64)
			trunc = TRUE;

		data.push_back(v);
	}

	if (trunc)
		Cmd::Print("WARNING: one or more arguments in \"data-list\" have been truncated.");

	// write the data.

	if (!data.size())
		co_return;

	BOOLEAN res = FALSE;

	co_await BcAwaiter_Join{ Cmd::WriteMemory(address, data.data(), data.size() * sizeof(T), res) };

	if (!res)
	{
		Cmd::Print("Error writing memory.");
		co_return;
	}

	// return to the caller.

	params.result = CmdParamsResult::RefreshCodeAndRegsWindows;

	co_return;
}

class Cmd_EB : public Cmd
{
public:

	virtual const CHAR* GetId() { return "EB"; }
	virtual const CHAR* GetDesc() { return "Edit memory as 8-bit values."; }
	virtual const CHAR* GetSyntax() { return "EB address -v space-separated-values"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		co_await BcAwaiter_Join{ E<BYTE>("EB", params)};
	}
};

class Cmd_EW : public Cmd
{
public:

	virtual const CHAR* GetId() { return "EW"; }
	virtual const CHAR* GetDesc() { return "Edit memory as 16-bit values."; }
	virtual const CHAR* GetSyntax() { return "EW address -v space-separated-values"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		co_await BcAwaiter_Join{ E<WORD>("EW", params)};
	}
};

class Cmd_ED : public Cmd
{
public:

	virtual const CHAR* GetId() { return "ED"; }
	virtual const CHAR* GetDesc() { return "Edit memory as 32-bit values."; }
	virtual const CHAR* GetSyntax() { return "ED address -v space-separated-values"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		co_await BcAwaiter_Join{ E<DWORD>("ED", params)};
	}
};

class Cmd_EQ : public Cmd
{
public:

	virtual const CHAR* GetId() { return "EQ"; }
	virtual const CHAR* GetDesc() { return "Edit memory as 64-bit values."; }
	virtual const CHAR* GetSyntax() { return "EQ address -v space-separated-values"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		co_await BcAwaiter_Join{ E<QWORD>("EQ", params)};
	}
};

REGISTER_COMMAND(Cmd_EB)
REGISTER_COMMAND(Cmd_EW)
REGISTER_COMMAND(Cmd_ED)
REGISTER_COMMAND(Cmd_EQ)
