#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "Utils.h"

static ULONG64 address = 0;
static ULONG64 length = 16 * 4;

template<typename T>
static BcCoroutine D(const CHAR* cmdId, CmdParams& params) noexcept
{
	// check the number of arguments.

	auto args = Cmd::TokenizeArgs(params.cmd, cmdId, "-l");

	if (args.size() > 4)
	{
		Cmd::Print("Too many arguments.");
		co_return;
	}

	// parse the arguments.

	BOOLEAN addressSet = FALSE;

	for (int i = 1; i < args.size(); i++)
	{
		if (!Utils::AreStringsEqualI(args[i].c_str(), "-l"))
		{
			eastl::pair<BOOLEAN, ULONG64> res;
			co_await BcAwaiter_Join{ Cmd::ResolveArg(res, args[i].c_str(), params.context, params.contextLen, params.is32bitCompat) };

			if (!res.first)
				co_return;
			else
			{
				if (Utils::AreStringsEqualI(args[i - 1].c_str(), "-l"))
				{
					length = res.second;
				}
				else
				{
					if (addressSet)
					{
						Cmd::Print("Syntax error.");
						co_return;
					}

					address = res.second;
					addressSet = TRUE;
				}
			}
		}
	}

	// read the memory.

	if (!length)
	{
		Cmd::Print("\"len-in-bytes\" cannot be 0.");
		co_return;
	}
	else if (length > 8 * 1024)
	{
		Cmd::Print("\"len-in-bytes\" too large.");
		co_return;
	}

	length = (length / 16) * 16 + (length % 16 ? 16 : 0);

	T* ptr = (T*)co_await BcAwaiter_ReadMemory{ (ULONG_PTR)address, (size_t)length };
	if (!ptr)
	{
		Cmd::Print("Unable to read memory.");
		co_return;
	}

	// print the memory contents.

	for (int i = 0; i < length / 16; i++, address += 16)
	{
		BYTE* pb = (BYTE*)ptr;

		eastl::string line =
			Utils::HexToString(((CONTEXT*)params.context)->SegDs, sizeof(WORD)) + ":" +
			Utils::HexToString(address, sizeof(VOID*)) + "  ";

		for (int j = 0; j < 16; j += sizeof(T), ptr++)
			line += Utils::HexToString(*ptr, sizeof(T)) + (j + sizeof(T) == 8 ? "-" : " ");

		line += " ";

		for (int j = 0; j < 16; j++, pb++)
			if (*pb < 32 || *pb >= 128)
				line += ".";
			else
				line += (CHAR)*pb;

		Cmd::Print(line.c_str());
	}

	// return to the caller.

	co_return;
}

class Cmd_DB : public Cmd
{
public:

	virtual const CHAR* GetId() { return "DB"; }
	virtual const CHAR* GetDesc() { return "Display memory as 8-bit values."; }
	virtual const CHAR* GetSyntax() { return "DB [address] [-l len-in-bytes]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		co_await BcAwaiter_Join{ D<BYTE>("DB", params)};
	}
};

class Cmd_DW : public Cmd
{
public:

	virtual const CHAR* GetId() { return "DW"; }
	virtual const CHAR* GetDesc() { return "Display memory as 16-bit values."; }
	virtual const CHAR* GetSyntax() { return "DW [address] [-l len-in-bytes]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		co_await BcAwaiter_Join{ D<WORD>("DW", params)};
	}
};

class Cmd_DD : public Cmd
{
public:

	virtual const CHAR* GetId() { return "DD"; }
	virtual const CHAR* GetDesc() { return "Display memory as 32-bit values."; }
	virtual const CHAR* GetSyntax() { return "DD [address] [-l len-in-bytes]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		co_await BcAwaiter_Join{ D<DWORD>("DD", params)};
	}
};

class Cmd_DQ : public Cmd
{
public:

	virtual const CHAR* GetId() { return "DQ"; }
	virtual const CHAR* GetDesc() { return "Display memory as 64-bit values."; }
	virtual const CHAR* GetSyntax() { return "DQ [address] [-l len-in-bytes]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		co_await BcAwaiter_Join{ D<QWORD>("DQ", params)};
	}
};

REGISTER_COMMAND(Cmd_DB)
REGISTER_COMMAND(Cmd_DW)
REGISTER_COMMAND(Cmd_DD)
REGISTER_COMMAND(Cmd_DQ)
