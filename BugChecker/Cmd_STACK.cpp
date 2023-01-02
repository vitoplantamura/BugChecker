#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"

class Cmd_STACK : public Cmd
{
public:

	virtual const CHAR* GetId() { return "STACK"; }
	virtual const CHAR* GetDesc() { return "Scan the stack searching for return addresses."; }
	virtual const CHAR* GetSyntax() { return "STACK (no parameters)"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept // >>>> N.B. <<<< we need to scan the stack because the RUNTIME_FUNCTION tables are typically paged out!
	{
		auto args = TokenizeArgs(params.cmd, "STACK");

		if (args.size() != 1)
		{
			Print("Too many arguments.");
			co_return;
		}

		// scan the stack searching for return addresses.

		BOOLEAN res = FALSE;
		eastl::vector<eastl::pair<ULONG64, ULONG64>> addrs;
		BOOLEAN is64 = FALSE;

		co_await BcAwaiter_Join{ ScanStackForRets(res, addrs, is64, params.context, params.is32bitCompat, 999999) };

		if (!res)
			co_return;

		// print the results.

		LONG lineNum = 0;

		for (auto& addr : addrs)
		{
			// try to convert the address to a symbol name.

			eastl::string sym;

			co_await BcAwaiter_Join{ AddressToSymbol(sym, addr.second, is64) };

			if (!sym.size())
				sym = "???";

			// print the line.

			if (!lineNum)
				Print(!is64 ? "     RET-POS  RET-ADDR" : "     RET-POS          RET-ADDR");

			CHAR header[64];

			::sprintf(header, "%03d) ", lineNum++);

			ULONG64 a = addr.first;
			::sprintf(header + ::strlen(header), _6432_(!is64 ? "%08X" : "%016llX", "%08X"), _6432_(!is64 ? (ULONG32)a : a, (ULONG32)a));

			::strcat(header, " ");

			Print((eastl::string(header) + sym).c_str());
		}

		// return to the caller.

		co_return;
	}
};

REGISTER_COMMAND(Cmd_STACK)
