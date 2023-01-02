#pragma once

#include "BugChecker.h"

#include "BcCoroutine.h"
#include "Cmd.h"
#include "X86Step.h"

#include "EASTL/string.h"

class Main
{
public:

	static BcCoroutine EntryPoint() noexcept;

	static BcCoroutine SetStepBp(ULONG64 addr) noexcept;

private:

	static Cmd* GetCmd(const CHAR* id);
	static BcCoroutine ProcessCmd(const eastl::pair<eastl::string, Cmd*>& cmd, BOOLEAN& exit, BYTE* Context, ULONG ContextLen, BOOLEAN is32bitCompat, ULONG64 jumpDest, ZydisDecodedInstruction* thisInstr, ZydisDecodedOperand* thisOps) noexcept;
	static BcCoroutine DisasmCurrInstr(ZydisDecodedInstruction& thisInstr, ZydisDecodedOperand* thisOps, JumpInstrType& jumpType, ULONG64& stepAddr, BYTE* Context, BOOLEAN is32bitCompat) noexcept;
	static BcCoroutine DrawInterface(BYTE* Context, BOOLEAN is32bitCompat, ULONG64 jumpDest) noexcept;
};
