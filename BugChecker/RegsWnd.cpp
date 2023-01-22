#include "RegsWnd.h"
#include "Utils.h"

#define MACRO_OF_MASK				( 1 << 11 )  // from the first BugChecker.
#define MACRO_DF_MASK				( 1 << 10 )
#define MACRO_IF_MASK				( 1 << 9 )
#define MACRO_SF_MASK				( 1 << 7 )
#define MACRO_ZF_MASK				( 1 << 6 )
#define MACRO_AF_MASK				( 1 << 4 )
#define MACRO_PF_MASK				( 1 << 2 )
#define MACRO_CF_MASK				( 1 << 0 )

VOID RegsWnd::DrawRegs(BYTE* context, BOOLEAN is32bitCompat)
{
	contents.clear();

	eastl::string line = "";

	const ULONG lineDim = destX1 - destX0 + 1;

	const BOOLEAN automaticAddLine = _6432_(is32bitCompat ? FALSE : TRUE, FALSE); // preserve the look and feel of the original SoftICE/BugChecker.

	// define some lambdas.

	auto addLine = [&]() {

		contents.push_back(line);

		line = "";
	};

	auto drawReg = [&](const CHAR* name, ULONG64 value, size_t size, ULONG64 oldValue) {

		CHAR buffer[64];

		BOOLEAN highlight = FALSE;

		if (size == 2)
		{
			::sprintf(buffer, "%04X", (USHORT)value);
			highlight = (USHORT)value != (USHORT)oldValue;
		}
		else if (size == 4)
		{
			::sprintf(buffer, "%08X", (ULONG32)value);
			highlight = (ULONG32)value != (ULONG32)oldValue;
		}
		else
		{
			::sprintf(buffer, "%016llX", (ULONG64)value);
			highlight = (ULONG64)value != (ULONG64)oldValue;
		}

		auto str = eastl::string(name) + "=" + buffer;

		if (automaticAddLine && StrLen(line.c_str()) + str.size() > lineDim)
			addLine();

		line +=
			(oldCtxValid && highlight ? (Wnd::nrmClr != 0x0B ? "\n0B" : "\n" + Utils::HexToString(Utils::NegateByte(0x0B), sizeof(BYTE))) : "") +
			str +
			"\n" + Utils::HexToString(Wnd::nrmClr, sizeof(BYTE)) + "   " +
			(!::strcmp(name, "R8") || !::strcmp(name, "R9") ? " " : "");
	};

	auto drawFlag = [&](ULONG64 mask, const CHAR idUppercase, ULONG64 value, ULONG64 oldValue, eastl::string& str) {

		if (oldCtxValid && (value & mask) != (oldValue & mask))
			str += "\n" + (Wnd::nrmClr != 0x0B? "0B" : Utils::HexToString(Utils::NegateByte(0x0B), sizeof(BYTE)));

		if (value & mask)
			str += idUppercase;
		else
			str += idUppercase + 32;

		str += "\n" + Utils::HexToString(Wnd::nrmClr, sizeof(BYTE)) + " ";
	};

	auto drawFlags = [&](ULONG64 value, ULONG64 oldValue) {

		eastl::string str;

		drawFlag(MACRO_OF_MASK, 'O', value, oldValue, str);
		drawFlag(MACRO_DF_MASK, 'D', value, oldValue, str);
		drawFlag(MACRO_IF_MASK, 'I', value, oldValue, str);
		drawFlag(MACRO_SF_MASK, 'S', value, oldValue, str);
		drawFlag(MACRO_ZF_MASK, 'Z', value, oldValue, str);
		drawFlag(MACRO_AF_MASK, 'A', value, oldValue, str);
		drawFlag(MACRO_PF_MASK, 'P', value, oldValue, str);
		drawFlag(MACRO_CF_MASK, 'C', value, oldValue, str);

		str += "  ";

		if (automaticAddLine && StrLen(line.c_str()) + (8 * 2 - 1) > lineDim)
			addLine();

		line += str;
	};

	// add the 32 bit or 64 bit registers.

	auto ctx = (CONTEXT*)context;

#ifndef _AMD64_

	drawReg("EAX", ctx->Eax, 4, oldCtx.Eax);
	drawReg("EBX", ctx->Ebx, 4, oldCtx.Ebx);
	drawReg("ECX", ctx->Ecx, 4, oldCtx.Ecx);
	drawReg("EDX", ctx->Edx, 4, oldCtx.Edx);
	drawReg("ESI", ctx->Esi, 4, oldCtx.Esi);

	addLine();

	drawReg("EDI", ctx->Edi, 4, oldCtx.Edi);
	drawReg("EBP", ctx->Ebp, 4, oldCtx.Ebp);
	drawReg("ESP", ctx->Esp, 4, oldCtx.Esp);
	drawReg("EIP", ctx->Eip, 4, oldCtx.Eip);

	drawFlags(ctx->EFlags, oldCtx.EFlags);

	addLine();

	drawReg("CS", ctx->SegCs, 2, oldCtx.SegCs);
	drawReg("DS", ctx->SegDs, 2, oldCtx.SegDs);
	drawReg("SS", ctx->SegSs, 2, oldCtx.SegSs);
	drawReg("ES", ctx->SegEs, 2, oldCtx.SegEs);
	drawReg("FS", ctx->SegFs, 2, oldCtx.SegFs);
	drawReg("GS", ctx->SegGs, 2, oldCtx.SegGs);

	addLine();

#else

	if (is32bitCompat)
	{
		drawReg("EAX", ctx->Rax, 4, oldCtx.Rax);
		drawReg("EBX", ctx->Rbx, 4, oldCtx.Rbx);
		drawReg("ECX", ctx->Rcx, 4, oldCtx.Rcx);
		drawReg("EDX", ctx->Rdx, 4, oldCtx.Rdx);
		drawReg("ESI", ctx->Rsi, 4, oldCtx.Rsi);

		addLine();

		drawReg("EDI", ctx->Rdi, 4, oldCtx.Rdi);
		drawReg("EBP", ctx->Rbp, 4, oldCtx.Rbp);
		drawReg("ESP", ctx->Rsp, 4, oldCtx.Rsp);
		drawReg("EIP", ctx->Rip, 4, oldCtx.Rip);
	}
	else
	{
		drawReg("RAX", ctx->Rax, 8, oldCtx.Rax);
		drawReg("RBX", ctx->Rbx, 8, oldCtx.Rbx);
		drawReg("RCX", ctx->Rcx, 8, oldCtx.Rcx);
		drawReg("RDX", ctx->Rdx, 8, oldCtx.Rdx);
		drawReg("RSI", ctx->Rsi, 8, oldCtx.Rsi);
		drawReg("RDI", ctx->Rdi, 8, oldCtx.Rdi);
		drawReg("RBP", ctx->Rbp, 8, oldCtx.Rbp);
		drawReg("RSP", ctx->Rsp, 8, oldCtx.Rsp);
		drawReg("R8", ctx->R8, 8, oldCtx.R8);
		drawReg("R9", ctx->R9, 8, oldCtx.R9);
		drawReg("R10", ctx->R10, 8, oldCtx.R10);
		drawReg("R11", ctx->R11, 8, oldCtx.R11);
		drawReg("R12", ctx->R12, 8, oldCtx.R12);
		drawReg("R13", ctx->R13, 8, oldCtx.R13);
		drawReg("R14", ctx->R14, 8, oldCtx.R14);
		drawReg("R15", ctx->R15, 8, oldCtx.R15);
		drawReg("RIP", ctx->Rip, 8, oldCtx.Rip);
	}

	drawFlags(ctx->EFlags, oldCtx.EFlags);

	addLine();

	drawReg("CS", ctx->SegCs, 2, oldCtx.SegCs);
	drawReg("DS", ctx->SegDs, 2, oldCtx.SegDs);
	drawReg("SS", ctx->SegSs, 2, oldCtx.SegSs);
	drawReg("ES", ctx->SegEs, 2, oldCtx.SegEs);
	drawReg("FS", ctx->SegFs, 2, oldCtx.SegFs);
	drawReg("GS", ctx->SegGs, 2, oldCtx.SegGs);

	addLine();

#endif

	// save the current context.

	oldCtx = *ctx;
	oldCtxValid = TRUE;
}
