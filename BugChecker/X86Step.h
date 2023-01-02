#pragma once

#include "BugChecker.h"

enum class JumpInstrType
{
	None,
	NotJumping,
	Jumping
};

class X86Step
{
public:

	static VOID* ZydisRegToCtxRegValuePtr(
		__in CONTEXT* ctx,
		__in ZydisRegister reg,
		__out size_t* psz);

	static ZydisRegister StringToZydisReg(__in const CHAR* str);

	static JumpInstrType CalculateStepDestination(
		__in CONTEXT* ctx,
		__in ZydisDecodedInstruction& thisInstr,
		__in ZydisDecodedOperand* thisOps,
		__in BOOLEAN is32bitCompat,
		__out ULONG64& stepAddrImm,
		__out ULONG64& stepAddrPtr,
		__out ULONG64& stepAddrPtrSize);
};
