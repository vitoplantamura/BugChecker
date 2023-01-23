#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "Utils.h"

class Cmd_BPX : public Cmd
{
public:

	virtual const CHAR* GetId() { return "BPX"; }
	virtual const CHAR* GetDesc() { return "Set a breakpoint on execution."; }
	virtual const CHAR* GetSyntax() { return "BPX address [-t|-p|-kt thread|-kp process] [WHEN js-expression]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		// parse the arguments.

		ULONG64 address = 0;
		ULONG64 eprocess = 0;
		ULONG64 ethread = 0;
		eastl::string when = "";

		auto args = TokenizeArgs(params.cmd, "BPX", "-t", "-p", "WHEN", "-kt", "-kp");

		if (args.size() < 2)
		{
			Print("Too few arguments.");
			co_return;
		}
		else
		{
			int whenCount = 0;

			for (int i = 1; i < args.size(); i++)
			{
				if (Utils::AreStringsEqualI(args[i].c_str(), "-t"))
				{
					if (ethread)
					{
						Print("Syntax error.");
						co_return;
					}

					ethread = Root::I->StateChange.Thread;
				}
				else if (Utils::AreStringsEqualI(args[i].c_str(), "-p"))
				{
					if (eprocess)
					{
						Print("Syntax error.");
						co_return;
					}

					co_await BcAwaiter_Join{ Platform::GetCurrentEprocess(eprocess) };
					if (!eprocess)
					{
						Print("Unable to get current eprocess.");
						co_return;
					}
				}
				else if (Utils::AreStringsEqualI(args[i].c_str(), "-kt") || Utils::AreStringsEqualI(args[i].c_str(), "-kp"))
				{
					ULONG64& value = Utils::AreStringsEqualI(args[i].c_str(), "-kt") ? ethread : eprocess;

					if (i == args.size() - 1 || value)
					{
						Print("Syntax error.");
						co_return;
					}

					i++;

					eastl::pair<BOOLEAN, ULONG64> res;
					co_await BcAwaiter_Join{ ResolveArg(res, args[i].c_str(), params.context, params.contextLen, params.is32bitCompat) };

					if (!res.first)
						co_return;
					else
						value = res.second;
				}
				else if (Utils::AreStringsEqualI(args[i].c_str(), "WHEN"))
				{
					if (++whenCount > 1 || i == args.size() - 1)
					{
						Print("Syntax error.");
						co_return;
					}
				}
				else
				{
					if (Utils::AreStringsEqualI(args[i - 1].c_str(), "WHEN"))
					{
						when = args[i];
					}
					else
					{
						eastl::pair<BOOLEAN, ULONG64> res;
						co_await BcAwaiter_Join{ ResolveArg(res, args[i].c_str(), params.context, params.contextLen, params.is32bitCompat) };

						if (!res.first)
							co_return;
						else
							address = res.second;
					}
				}
			}

			if (eprocess && ethread)
			{
				Print("-t/-kt and -p/-kp cannot be specified at the same time.");
				co_return;
			}
		}

		// is bp already set at this address?

		for (BreakPoint& bp : Root::I->BreakPoints)
			if (bp.address == address)
			{
				Print("Breakpoint already set at this address.");
				co_return;
			}

		// read the byte to overwrite with the breakpoint instruction.

		VOID* ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)address, sizeof(BYTE) };
		if (!ptr)
		{
			Print("Unable to read memory.");
			co_return;
		}

		BYTE prev = *(BYTE*)ptr;

		// set the breakpoint.

		ULONG BreakPointHandle = 0;

		if (!co_await BcAwaiter_StateManipulate{
		[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

			pState->ApiNumber = DbgKdWriteBreakPointApi;

			pState->ReturnStatus = STATUS_PENDING;

			pState->u.WriteBreakPoint.BreakPointHandle = 0;
			pState->u.WriteBreakPoint.BreakPointAddress = address;
		},
		[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

			if (pState->ApiNumber == DbgKdWriteBreakPointApi)
			{
				if (NT_SUCCESS(pState->ReturnStatus) &&
					pState->u.WriteBreakPoint.BreakPointHandle)
				{
					BreakPointHandle = pState->u.WriteBreakPoint.BreakPointHandle;
					return TRUE;
				}
			}

			return FALSE;
		} })
		{
			Print("Error setting breakpoint.");
			co_return;
		}

		// add the new breakpoint to the vector.

		BreakPoint bp;

		bp.address = address;
		bp.handle = BreakPointHandle;
		bp.prevByte = prev;
		bp.eprocess = eprocess;
		bp.ethread = ethread;
		bp.when = when;

		bp.cmd =
			"BPX 0x" +
			Utils::HexToString(address, address >> 32 ? sizeof(ULONG64) : sizeof(ULONG32)) +
			" (" + params.cmd + ")";

		Root::I->BreakPoints.push_back(eastl::move(bp));

		// return to the caller.

		params.result = CmdParamsResult::RefreshCodeAndRegsWindows;

		co_return;
	}
};

REGISTER_COMMAND(Cmd_BPX)
