#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "X86Step.h"
#include "Utils.h"

class Cmd_R : public Cmd
{
public:

	virtual const CHAR* GetId() { return "R"; }
	virtual const CHAR* GetDesc() { return "Change a register value."; }
	virtual const CHAR* GetSyntax() { return "R register-name -v value"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		// check the number of arguments.

		auto args = TokenizeArgs(params.cmd, "R", "-v");

		if (args.size() < 4)
		{
			Print("Too few arguments.");
			co_return;
		}
		else if (args.size() > 4)
		{
			Print("Too many arguments.");
			co_return;
		}
		else if (!Utils::AreStringsEqualI(args[2].c_str(), "-v"))
		{
			Print("Syntax error.");
			co_return;
		}

		// parse the "register-name".

		VOID* preg = NULL;
		size_t sz = 0;

		{
			ZydisRegister reg = X86Step::StringToZydisReg(args[1].c_str());

			if (reg != ZYDIS_REGISTER_NONE)
			{
				preg = X86Step::ZydisRegToCtxRegValuePtr((CONTEXT*)params.context, reg, &sz);
			}

			if (!preg || (sz != 4 && sz != 8))
			{
				Print("Invalid register name.");
				co_return;
			}
		}

		if (_6432_(TRUE, FALSE) && params.is32bitCompat && sz == 8)
		{
			Print("Unable to change 64-bit register in 32-bit mode.");
			co_return;
		}

		// parse the "value".

		ULONG64 val = 0;

		{
			eastl::pair<BOOLEAN, ULONG64> res;
			co_await BcAwaiter_Join{ ResolveArg(res, args[3].c_str(), params.context, params.contextLen, params.is32bitCompat) };

			if (!res.first)
				co_return;
			else
				val = res.second;
		}

		// set the new value in the context.

		if (sz == 8)
			*(ULONG64*)preg = val;
		else
			*(ULONG32*)preg = (ULONG32)val;

		// set the new context.

		if (!co_await BcAwaiter_StateManipulate{
		[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

			if (SecondBuffer->pData != NULL && params.contextLen > 0 && SecondBuffer->MaxLength >= params.contextLen)
			{
				pState->ApiNumber = DbgKdSetContextApi;

				::memcpy(SecondBuffer->pData, params.context, params.contextLen);
				SecondBuffer->Length = params.contextLen;
			}
		},
		[](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

			return pState->ApiNumber == DbgKdSetContextApi && NT_SUCCESS(pState->ReturnStatus);
		} })
		{
			Print("Unable to set context.");
			co_return;
		}

		// return to the caller.

		Root::I->RegsWindow.InvalidateOldContext();

		params.result = CmdParamsResult::RefreshCodeAndRegsWindows;

		co_return;
	}
};

REGISTER_COMMAND(Cmd_R)
