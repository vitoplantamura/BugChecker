#include "X86Step.h"

#include "Utils.h"

VOID* X86Step::ZydisRegToCtxRegValuePtr(
	__in CONTEXT* ctx,
	__in ZydisRegister reg,
	__out size_t* psz)
{
	*psz = 0;

	VOID* pval = NULL;

	switch (reg)
	{
	case ZYDIS_REGISTER_RAX: pval = _6432_(&ctx->Rax, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_RBX: pval = _6432_(&ctx->Rbx, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_RCX: pval = _6432_(&ctx->Rcx, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_RDX: pval = _6432_(&ctx->Rdx, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_RSI: pval = _6432_(&ctx->Rsi, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_RDI: pval = _6432_(&ctx->Rdi, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_RSP: pval = _6432_(&ctx->Rsp, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_RBP: pval = _6432_(&ctx->Rbp, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_R8: pval = _6432_(&ctx->R8, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_R9: pval = _6432_(&ctx->R9, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_R10: pval = _6432_(&ctx->R10, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_R11: pval = _6432_(&ctx->R11, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_R12: pval = _6432_(&ctx->R12, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_R13: pval = _6432_(&ctx->R13, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_R14: pval = _6432_(&ctx->R14, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_R15: pval = _6432_(&ctx->R15, NULL); *psz = 8; break;
	case ZYDIS_REGISTER_RIP: pval = _6432_(&ctx->Rip, NULL); *psz = 8; break;

	case ZYDIS_REGISTER_EAX: pval = _6432_(&ctx->Rax, &ctx->Eax); *psz = 4; break;
	case ZYDIS_REGISTER_EBX: pval = _6432_(&ctx->Rbx, &ctx->Ebx); *psz = 4; break;
	case ZYDIS_REGISTER_ECX: pval = _6432_(&ctx->Rcx, &ctx->Ecx); *psz = 4; break;
	case ZYDIS_REGISTER_EDX: pval = _6432_(&ctx->Rdx, &ctx->Edx); *psz = 4; break;
	case ZYDIS_REGISTER_ESI: pval = _6432_(&ctx->Rsi, &ctx->Esi); *psz = 4; break;
	case ZYDIS_REGISTER_EDI: pval = _6432_(&ctx->Rdi, &ctx->Edi); *psz = 4; break;
	case ZYDIS_REGISTER_ESP: pval = _6432_(&ctx->Rsp, &ctx->Esp); *psz = 4; break;
	case ZYDIS_REGISTER_EBP: pval = _6432_(&ctx->Rbp, &ctx->Ebp); *psz = 4; break;
	case ZYDIS_REGISTER_EIP: pval = _6432_(&ctx->Rip, &ctx->Eip); *psz = 4; break;
	}

	return pval;
}

ZydisRegister X86Step::StringToZydisReg(__in const CHAR* str)
{
	auto compare = [&](const CHAR* regName) {

		return ::strlen(regName) == ::strlen(str) &&
			Utils::stristr(regName, str);
	};

	if (compare("RAX")) return ZYDIS_REGISTER_RAX;
	else if (compare("RBX")) return ZYDIS_REGISTER_RBX;
	else if (compare("RCX")) return ZYDIS_REGISTER_RCX;
	else if (compare("RDX")) return ZYDIS_REGISTER_RDX;
	else if (compare("RSI")) return ZYDIS_REGISTER_RSI;
	else if (compare("RDI")) return ZYDIS_REGISTER_RDI;
	else if (compare("RSP")) return ZYDIS_REGISTER_RSP;
	else if (compare("RBP")) return ZYDIS_REGISTER_RBP;
	else if (compare("R8")) return ZYDIS_REGISTER_R8;
	else if (compare("R9")) return ZYDIS_REGISTER_R9;
	else if (compare("R10")) return ZYDIS_REGISTER_R10;
	else if (compare("R11")) return ZYDIS_REGISTER_R11;
	else if (compare("R12")) return ZYDIS_REGISTER_R12;
	else if (compare("R13")) return ZYDIS_REGISTER_R13;
	else if (compare("R14")) return ZYDIS_REGISTER_R14;
	else if (compare("R15")) return ZYDIS_REGISTER_R15;
	else if (compare("RIP")) return ZYDIS_REGISTER_RIP;
	else if (compare("EAX")) return ZYDIS_REGISTER_EAX;
	else if (compare("EBX")) return ZYDIS_REGISTER_EBX;
	else if (compare("ECX")) return ZYDIS_REGISTER_ECX;
	else if (compare("EDX")) return ZYDIS_REGISTER_EDX;
	else if (compare("ESI")) return ZYDIS_REGISTER_ESI;
	else if (compare("EDI")) return ZYDIS_REGISTER_EDI;
	else if (compare("ESP")) return ZYDIS_REGISTER_ESP;
	else if (compare("EBP")) return ZYDIS_REGISTER_EBP;
	else if (compare("EIP")) return ZYDIS_REGISTER_EIP;
	else return ZYDIS_REGISTER_NONE;
}

JumpInstrType X86Step::CalculateStepDestination(
	__in CONTEXT* ctx,
	__in ZydisDecodedInstruction& thisInstr,
	__in ZydisDecodedOperand* thisOps,
	__in BOOLEAN is32bitCompat,
	__out ULONG64& stepAddrImm,
	__out ULONG64& stepAddrPtr,
	__out ULONG64& stepAddrPtrSize)
{
	stepAddrImm = 0;
	stepAddrPtr = 0;
	stepAddrPtrSize = 0;

	BOOLEAN _not_ = FALSE;

	BOOLEAN isJumpInstr = TRUE;

	switch (thisInstr.mnemonic) // left out: iret, xbegin, xend, xabort
	{
	case ZYDIS_MNEMONIC_RET:
	{
		if (thisInstr.meta.branch_type == ZYDIS_BRANCH_TYPE_NEAR)
		{
			stepAddrPtr = _6432_(is32bitCompat ? (ULONG32)ctx->Rsp : ctx->Rsp, ctx->Esp);
			stepAddrPtrSize = _6432_(is32bitCompat ? 4 : 8, 4);
		}
	}
	break;

	case ZYDIS_MNEMONIC_CALL:
	case ZYDIS_MNEMONIC_JMP:
	{
		if (thisInstr.operand_count_visible == 1)
		{
			if (thisOps[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
			{
				stepAddrImm = ctx->_6432_(Rip, Eip) + thisInstr.length + thisOps[0].imm.value.s;
			}
			else if (thisOps[0].type == ZYDIS_OPERAND_TYPE_REGISTER)
			{
				size_t sz = 0;
				VOID* pval = ZydisRegToCtxRegValuePtr(ctx, thisOps[0].reg.value, &sz);

				if (pval && (sz == 8 || sz == 4))
				{
					stepAddrImm = sz == 8 ? *(ULONG64*)pval : *(ULONG32*)pval;
				}
			}
			else if (thisOps[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
			{
				if (thisOps[0].mem.type == ZYDIS_MEMOP_TYPE_MEM)
				{
					switch (thisOps[0].mem.segment)
					{
					case ZYDIS_REGISTER_CS:
					case ZYDIS_REGISTER_DS:
					case ZYDIS_REGISTER_ES:
					case ZYDIS_REGISTER_SS:
					{
						ZydisRegister base = thisOps[0].mem.base;

						size_t szBase = 0;
						VOID* pBase = ZydisRegToCtxRegValuePtr(ctx, base, &szBase);

						size_t szIndex = 0;
						VOID* pIndex = ZydisRegToCtxRegValuePtr(ctx, thisOps[0].mem.index, &szIndex);

						if (
							(!pBase || (szBase == 8 || szBase == 4)) &&
							(!pIndex || (szIndex == 8 || szIndex == 4))
							)
						{
							ULONG64 v = 0;

							if (pBase)
							{
								v += szBase == 8 ? *(ULONG64*)pBase : *(ULONG32*)pBase;

								if (base == ZYDIS_REGISTER_EIP || base == ZYDIS_REGISTER_RIP)
									v += thisInstr.length;
							}

							if (pIndex)
							{
								v += (szIndex == 8 ? *(ULONG64*)pIndex : *(ULONG32*)pIndex) * thisOps[0].mem.scale;
							}

							if (thisOps[0].mem.disp.has_displacement)
							{
								v += thisOps[0].mem.disp.value;
							}

							//

							if (v)
							{
								stepAddrPtr = _6432_(is32bitCompat ? (ULONG32)v : v, (ULONG32)v);
								stepAddrPtrSize = _6432_(is32bitCompat ? 4 : 8, 4);
							}
						}
					}
					break;
					}
				}
			}
		}
	}
	break;

	case ZYDIS_MNEMONIC_JNB:
	case ZYDIS_MNEMONIC_JNBE:
	case ZYDIS_MNEMONIC_JNL:
	case ZYDIS_MNEMONIC_JNLE:
	case ZYDIS_MNEMONIC_JNO:
	case ZYDIS_MNEMONIC_JNP:
	case ZYDIS_MNEMONIC_JNS:
	case ZYDIS_MNEMONIC_JNZ:
		_not_ = TRUE; // fallthrough
	case ZYDIS_MNEMONIC_JB:
	case ZYDIS_MNEMONIC_JBE:
	case ZYDIS_MNEMONIC_JL:
	case ZYDIS_MNEMONIC_JLE:
	case ZYDIS_MNEMONIC_JO:
	case ZYDIS_MNEMONIC_JP:
	case ZYDIS_MNEMONIC_JS:
	case ZYDIS_MNEMONIC_JZ:
	{
		BOOLEAN jump = FALSE;

		switch (thisInstr.mnemonic)
		{
		case ZYDIS_MNEMONIC_JBE:
			jump = (ctx->EFlags & ZYDIS_CPUFLAG_CF) || (ctx->EFlags & ZYDIS_CPUFLAG_ZF);
			break;

		case ZYDIS_MNEMONIC_JL:
			jump = !(ctx->EFlags & ZYDIS_CPUFLAG_SF) != !(ctx->EFlags & ZYDIS_CPUFLAG_OF);
			break;

		case ZYDIS_MNEMONIC_JNL:
			jump = !(ctx->EFlags & ZYDIS_CPUFLAG_SF) == !(ctx->EFlags & ZYDIS_CPUFLAG_OF);
			break;

		case ZYDIS_MNEMONIC_JLE:
			jump = (ctx->EFlags & ZYDIS_CPUFLAG_ZF) || !(ctx->EFlags & ZYDIS_CPUFLAG_SF) != !(ctx->EFlags & ZYDIS_CPUFLAG_OF);
			break;

		case ZYDIS_MNEMONIC_JNLE:
			jump = !(ctx->EFlags & ZYDIS_CPUFLAG_ZF) && !(ctx->EFlags & ZYDIS_CPUFLAG_SF) == !(ctx->EFlags & ZYDIS_CPUFLAG_OF);
			break;

		default:
		{
			if (thisInstr.cpu_flags && thisInstr.cpu_flags->tested)
			{
				auto cond = ctx->EFlags & thisInstr.cpu_flags->tested;

				if (
					(!_not_ && cond == thisInstr.cpu_flags->tested) ||
					(_not_ && cond == 0)
					)
				{
					jump = TRUE;
				}
			}
		}
		break;
		}

		if (jump &&
			thisOps[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
		{
			stepAddrImm = ctx->_6432_(Rip, Eip) + thisInstr.length + thisOps[0].imm.value.s;
		}
	}
	break;

	case ZYDIS_MNEMONIC_JCXZ:
	case ZYDIS_MNEMONIC_JECXZ:
	case ZYDIS_MNEMONIC_JRCXZ:
	{
		VOID* p = &ctx->_6432_(Rcx, Ecx);

		ULONG64 v = 1;

		switch (thisInstr.mnemonic)
		{
		case ZYDIS_MNEMONIC_JCXZ: v = *(USHORT*)p; break;
		case ZYDIS_MNEMONIC_JECXZ: v = *(ULONG32*)p; break;
		case ZYDIS_MNEMONIC_JRCXZ: v = *(ULONG64*)p; break;
		}

		if (!v &&
			thisOps[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
		{
			stepAddrImm = ctx->_6432_(Rip, Eip) + thisInstr.length + thisOps[0].imm.value.s;
		}
	}
	break;

	case ZYDIS_MNEMONIC_LOOP:
	case ZYDIS_MNEMONIC_LOOPE:
	case ZYDIS_MNEMONIC_LOOPNE:
	{
		BOOLEAN jump = FALSE;

		VOID* p = &ctx->_6432_(Rcx, Ecx);

		ULONG64 v = 1;

		switch (thisInstr.address_width)
		{
		case 16: v = *(USHORT*)p; break;
		case 32: v = *(ULONG32*)p; break;
		case 64: v = *(ULONG64*)p; break;
		}

		switch (thisInstr.mnemonic)
		{
		case ZYDIS_MNEMONIC_LOOP:
			jump = v != 1;
			break;

		case ZYDIS_MNEMONIC_LOOPE:
			jump = v != 1 && (ctx->EFlags & ZYDIS_CPUFLAG_ZF);
			break;

		case ZYDIS_MNEMONIC_LOOPNE:
			jump = v != 1 && !(ctx->EFlags & ZYDIS_CPUFLAG_ZF);
			break;
		}

		if (jump &&
			thisOps[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
		{
			stepAddrImm = ctx->_6432_(Rip, Eip) + thisInstr.length + thisOps[0].imm.value.s;
		}
	}
	break;

	default:
		isJumpInstr = FALSE;
		break;
	}

	//

	if (!stepAddrImm && !stepAddrPtr && !stepAddrPtrSize)
	{
		stepAddrImm = ctx->_6432_(Rip, Eip) + thisInstr.length;

		return isJumpInstr ? JumpInstrType::NotJumping : JumpInstrType::None;
	}

	return isJumpInstr ? JumpInstrType::Jumping : JumpInstrType::None;
}
