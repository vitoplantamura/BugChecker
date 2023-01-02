#include "DisasmWnd.h"

#include "Glyph.h"
#include "Root.h"
#include "Utils.h"

BcCoroutine DisasmWnd::Disasm(BYTE* context, BOOLEAN is32bitCompat, ULONG64 jumpDest, LONG navigate /*= 0*/) noexcept
{
	current32bitCompat = is32bitCompat;

	// reset the "posInModules" string and the header text.

	::strcpy(posInModules, "");
	::strcpy(headerText, "");

	if (!navigate)
	{
		symNameLinesNum = 0;

		isFirstAddressShownASymName = FALSE;
		isLastAddressShownASymName = FALSE;
	}

	// initialize decoder context.

	ZydisDecoder decoder;
	::ZydisDecoderInit(&decoder,
		_6432_(is32bitCompat ? ZYDIS_MACHINE_MODE_LONG_COMPAT_32 : ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_MACHINE_MODE_LEGACY_32),
		_6432_(is32bitCompat ? ZYDIS_STACK_WIDTH_32 : ZYDIS_STACK_WIDTH_64, ZYDIS_STACK_WIDTH_32));

	// initialize formatter.

	ZydisFormatter formatter;
	::ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

	::ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SEGMENT, ZYAN_TRUE);
	::ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);
	::ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_UPPERCASE_PREFIXES, ZYAN_TRUE);
	::ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_UPPERCASE_MNEMONIC, ZYAN_TRUE);
	::ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_UPPERCASE_REGISTERS, ZYAN_TRUE);
	::ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_UPPERCASE_TYPECASTS, ZYAN_TRUE);
	::ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_UPPERCASE_DECORATORS, ZYAN_TRUE);
	::ZydisFormatterSetProperty(&formatter, ZYDIS_FORMATTER_PROP_HEX_UPPERCASE, ZYAN_TRUE);

	defaultPrintAddressAbsolute = (ZydisFormatterFunc)&ZydisFormatterPrintAddressAbsolute;

	::ZydisFormatterSetHook(&formatter, ZYDIS_FORMATTER_FUNC_PRINT_ADDRESS_ABS, (const void**)&defaultPrintAddressAbsolute);

	// initialize several consts before starting.

	if (isFirstAddressShownASymName && navigate == 1)
		navigate = NAVIGATE_PRESERVE_POS;
	else
		isFirstAddressShownASymName = FALSE;

	if (navigate != -1)
		isLastAddressShownASymName = FALSE;

	const ULONG contentsDim = destY1 - destY0 + 1;
	const ULONG lineDim = destX1 - destX0 + 1 + posX;

	const ULONG64 pc = ((CONTEXT*)context)->_6432_(Rip, Eip);

	const ULONG64 start =
		navigate ? firstAddressShown :
		pc >= firstAddressShown && pc <= lastAddressShown ? firstAddressShown :
		pc;

	if (navigate == NAVIGATE_PRESERVE_POS) navigate = 0;

	// read the memory to disassemble.

	BYTE asmBytes[1024];

	const ZyanU64 asmBytesAddress = start - (navigate < 0 ? sizeof(asmBytes) / 2 : 0);
	const size_t asmBytesSize = navigate < 0 ? sizeof(asmBytes) : sizeof(asmBytes) / 2;

	auto ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)asmBytesAddress, asmBytesSize };
	if (!ptr)
	{
		contents.clear();

		CHAR text[128];
		::sprintf(text, _6432_(is32bitCompat ? "%08X" : "%016llX", "%08X"), _6432_(is32bitCompat ? (ULONG32)start : start, (ULONG32)start));
		::strcat(text, " ???");

		contents.push_back(text);

		co_return;
	}

	::memcpy(asmBytes, ptr, asmBytesSize);

	// hide our breakpoint instructions.

	for (BreakPoint& bp : Root::I->BreakPoints)
		if (bp.address >= asmBytesAddress && bp.address < asmBytesAddress + asmBytesSize)
			asmBytes[bp.address - asmBytesAddress] = bp.prevByte;

	// disassemble backwards or forward.

	ZydisDecodedInstruction instruction;
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];

	ZyanUSize offset = 0;
	ZyanU64 runtimeAddress = asmBytesAddress;

	eastl::vector<ULONG64> addresses;

	while (TRUE)
	{
		// fill the "addresses" list. Each item represents a line in the disassembler window.

		addresses.push_back(runtimeAddress);

		if (navigate >= 0)
		{
			if (addresses.size() >= contentsDim + navigate)
			{
				addresses.erase(addresses.begin(), addresses.begin() + navigate);
				break;
			}
		}
		else
		{
			if (runtimeAddress > lastAddressShown)
				co_return; // unaligned: unable to disassemble backwards.
			else if (runtimeAddress == lastAddressShown)
			{
				if (addresses.size() < contentsDim + (-navigate) + (isLastAddressShownASymName ? 1 : 0)) co_return; // should never happen.
				addresses.erase(addresses.end() - (-navigate) + (isLastAddressShownASymName ? 1 : 0), addresses.end());
				addresses.erase(addresses.begin(), addresses.end() - contentsDim + (isLastAddressShownASymName ? -1 : 0));
				break;
			}
		}

		// disassemble.

		ULONG instrLength;

		if (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, asmBytes + offset, asmBytesSize - offset,
			&instruction, operands, ZYDIS_MAX_OPERAND_COUNT_VISIBLE,
			ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY)))
			instrLength = instruction.length;
		else
			instrLength = 1;

		offset += instrLength;
		runtimeAddress += instrLength;
	}

	if (addresses.size() != contentsDim + (isLastAddressShownASymName ? 1 : 0)) co_return; // should never happen.

	// determine in which module eip/rip is.

	debugInfo = {};

	if (addresses.size())
		co_await BcAwaiter_DiscoverPosInModules{ posInModules, &debugInfo, (ULONG_PTR)addresses[0] };

	symF = NULL;

	if (debugInfo.startAddr)
		symF = Root::I->GetSymbolFileByGuidAndAge(debugInfo.guid, debugInfo.age);

	::sprintf(g_debugStr, "age:%X,guid:%s -> %s", debugInfo.age, Utils::GuidToString(debugInfo.guid).c_str(), symF == NULL ? "***IS_NULL***" : "***FOUND***");

	moduleName = "";

	if (::strlen(debugInfo.szPdb))
	{
		CHAR sz0[sizeof(debugInfo.szPdb)];
		::strcpy(sz0, debugInfo.szPdb);

		CHAR* psz = &sz0[::strlen(sz0) - 1];

		for (; psz >= sz0; psz--)
			if (*psz == '.')
				*psz = '\0';
			else if (*psz == '\\' || *psz == '/')
				break;

		psz++;

		if (::strlen(psz))
			moduleName = psz;
	}

	// disassemble, filling the "contents" vector.

	symNameLinesNum = 0;

	contents.clear();

	for (int j = 0; j < addresses.size(); j++)
	{
		runtimeAddress = addresses[j];
		offset = runtimeAddress - asmBytesAddress;

		// disassemble.

		BOOLEAN error = FALSE;

		ULONG instrLength;

		if (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, asmBytes + offset, asmBytesSize - offset,
			&instruction, operands, ZYDIS_MAX_OPERAND_COUNT_VISIBLE,
			ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY)))
		{
			instrLength = instruction.length;
		}
		else
		{
			instrLength = 1;
			error = TRUE;
		}

		// is a breakpoint set here ?

		BOOLEAN hasBp = FALSE;

		for (BreakPoint& bp : Root::I->BreakPoints)
			if (!bp.skip && bp.address == runtimeAddress)
			{
				hasBp = TRUE;
				break;
			}

		// is this the PC line ?

		CHAR line[512];

		if (runtimeAddress == pc)
			::strcpy(line, "\n71");
		else
			::strcpy(line, !hasBp ? "\n07" : "\n0B");

		// print current instruction pointer and instruction bytes.

		if (runtimeAddress == jumpDest)
		{
			::strcat(line, runtimeAddress == pc ? "\n7B" : "\n0B");
			::strcat(line, "===>");
			::strcat(line, runtimeAddress == pc ? "\n71" : !hasBp ? "\n07" : "\n0B");
		}
		else
			::sprintf(line + ::strlen(line), "%04X", ((CONTEXT*)context)->SegCs & 0xFFFF);

		::strcat(line, ":");

		::sprintf(line + ::strlen(line), _6432_(is32bitCompat ? "%08X" : "%016llX", "%08X"), _6432_(is32bitCompat ? (ULONG32)runtimeAddress : runtimeAddress, (ULONG32)runtimeAddress));

		*(USHORT*)(line + ::strlen(line)) = 0x00B3;

		::strcat(line, " ");

		for (int i = 0; i < 10; i++)
		{
			if (i < instrLength)
				::sprintf(line + ::strlen(line), "%02X", *(asmBytes + offset + i));
			else
				::strcat(line, "  ");
		}

		*(USHORT*)(line + ::strlen(line)) = 0x00B3;

		::strcat(line, " ");

		// format & print the binary instruction structure to human readable format.

		if (error)
		{
			::strcat(line, "???");
		}
		else
		{
			symbolPrologue = runtimeAddress == pc ? "" : "\n0E";
			symbolEpilogue = runtimeAddress == pc ? "" : !hasBp ? "\n07" : "\n0B";

			auto len = ::strlen(line);

			::ZydisFormatterFormatInstruction(&formatter, &instruction, operands,
				instruction.operand_count_visible,
				line + len, sizeof(line) - len,
				runtimeAddress);
		}

		// add the JUMP/NO JUMP tag.

		if (runtimeAddress == pc && jumpDest)
		{
			for (ULONG i = StrLen(line); i < lineDim; i++)
				::strcat(line, " ");

			auto ptr = const_cast<CHAR*>(StrCount(line, lineDim - 9));
			if (ptr)
			{
				if (jumpDest == -1)
					::strcpy(ptr, "\n70(NO JUMP)");
				else
				{
					::strcpy(ptr + 1, "\n70(JUMP X)");
					*(ptr + 10) = jumpDest < pc ? 0x18 : 0x19; // up/down arrow.
				}
			}
		}

		// check if there is a match for this line in the symbol file.

		if (!(isFirstAddressShownASymName && !j))
			if (symF && runtimeAddress >= (ULONG64)debugInfo.startAddr)
			{
				ULONG64 rva = runtimeAddress - (ULONG64)debugInfo.startAddr;
				if (rva < debugInfo.length)
				{
					BCSFILE_PUBLIC_SYMBOL* sym = NULL;
					auto symName = Symbols::GetNameOfPublic(symF->GetHeader(), (ULONG)rva, &sym);

					if (symName && sym && sym->address == rva)
					{
						// add the line with the symbol name.

						contents.push_back((runtimeAddress == pc ? "\n71" : !hasBp ? "\n07" : "\n0B") + moduleName + "!" + symName);

						// add the address of the new line.

						addresses.insert(addresses.begin() + j, runtimeAddress);

						j++;

						symNameLinesNum++;
					}
				}
			}

		// add the line.

		contents.push_back(line);
	}

	// remove the lines in excess, after the insertion of the "symbol name lines".

	ULONG64 posInModulesOffset = 0;

	if (navigate >= 0)
	{
		addresses.erase(addresses.end() - symNameLinesNum, addresses.end());
		contents.erase(contents.end() - symNameLinesNum, contents.end());
	}
	else
	{
		posInModulesOffset = addresses[symNameLinesNum] - addresses[0];

		addresses.erase(addresses.begin(), addresses.begin() + symNameLinesNum);
		contents.erase(contents.begin(), contents.begin() + symNameLinesNum);
	}

	// save info about our position.

	firstAddressShown = addresses[0];
	lastAddressShown = addresses.back();

	if (addresses.size() < 2)
	{
		isFirstAddressShownASymName = FALSE;
		isLastAddressShownASymName = FALSE;
	}
	else
	{
		isFirstAddressShownASymName = addresses[0] == addresses[1];
		isLastAddressShownASymName = addresses.size() == contentsDim && addresses.back() == addresses[addresses.size() - 2];
	}

	// adjust the "posInModules" string: we cannot call "DiscoverPosInModules" again because it's too expensive.

	if (::strlen(posInModules) && posInModulesOffset)
	{
		ULONG64 addr = 0;
		LONG shift = 0;

		CHAR* psz = &posInModules[::strlen(posInModules) - 1];

		for (; psz >= posInModules; psz--)
			if (IsHex(*psz))
			{
				addr |= ToHex(*psz) << shift;
				shift += 4;
			}
			else
			{
				if (*psz != '+') addr = 0;
				break;
			}

		psz++;

		if (::strlen(psz) && psz > posInModules && addr)
		{
			::sprintf(psz, "%.8X", (ULONG32)(addr + posInModulesOffset));
		}
	}

	// set the header text.

	if (symF && firstAddressShown >= (ULONG64)debugInfo.startAddr)
	{
		ULONG64 rva = firstAddressShown - (ULONG64)debugInfo.startAddr;
		if (rva < debugInfo.length)
		{
			BCSFILE_PUBLIC_SYMBOL* sym = NULL;
			auto symName = Symbols::GetNameOfPublic(symF->GetHeader(), (ULONG)rva, &sym);

			if (symName && sym && moduleName.size() + ::strlen(symName) < sizeof(headerText) - 32)
			{
				::strcpy(headerText, moduleName.c_str());
				::strcat(headerText, "!");
				::strcat(headerText, symName);

				if (rva > sym->address)
					::sprintf(headerText + ::strlen(headerText), "+%04X", (ULONG)(rva - sym->address));
			}
		}
	}
}

ZyanStatus DisasmWnd::ZydisFormatterPrintAddressAbsolute(const ZydisFormatter* formatter, ZydisFormatterBuffer* buffer, ZydisFormatterContext* context)
{
	auto I = &Root::I->DisasmWindow;

	// check the pointer size.

	if (context->instruction->stack_width != 32 && context->instruction->stack_width != 64)
		return I->defaultPrintAddressAbsolute(formatter, buffer, context);

	// get the address.

	ZyanU64 address;
	ZYAN_CHECK(::ZydisCalcAbsoluteAddress(context->instruction, context->operand, context->runtime_address, &address));

	// check if there is a corresponding symbol.

	if (I->symF && address >= (ULONG64)I->debugInfo.startAddr)
	{
		ULONG64 rva = address - (ULONG64)I->debugInfo.startAddr;
		if (rva < I->debugInfo.length)
		{
			BCSFILE_PUBLIC_SYMBOL* sym = NULL;
			auto symName = Symbols::GetNameOfPublic(I->symF->GetHeader(), (ULONG)rva, &sym);

			if (symName && sym)
			{
				// compose the symbol string.

				ZYAN_CHECK(::ZydisFormatterBufferAppend(buffer, ZYDIS_TOKEN_SYMBOL));

				ZyanString* string;
				ZYAN_CHECK(::ZydisFormatterBufferGetString(buffer, &string));

				ZyanStringView view = {};

				ZYAN_CHECK(::ZyanStringViewInsideBuffer(&view, I->symbolPrologue.c_str()));
				ZYAN_CHECK(::ZyanStringAppend(string, &view));

				ZYAN_CHECK(::ZyanStringViewInsideBuffer(&view, I->moduleName.c_str()));
				ZYAN_CHECK(::ZyanStringAppend(string, &view));

				ZYAN_CHECK(::ZyanStringViewInsideBuffer(&view, "!"));
				ZYAN_CHECK(::ZyanStringAppend(string, &view));

				ZYAN_CHECK(::ZyanStringViewInsideBuffer(&view, symName));
				ZYAN_CHECK(::ZyanStringAppend(string, &view));

				CHAR buffer[64];

				if (rva > sym->address)
				{
					::sprintf(buffer, "+%04X", (ULONG)(rva - sym->address));

					ZYAN_CHECK(::ZyanStringViewInsideBuffer(&view, buffer));
					ZYAN_CHECK(::ZyanStringAppend(string, &view));
				}

				if (context->instruction->stack_width == 32)
					::sprintf(buffer, " @ 0x%08X", (ULONG32)address);
				else
					::sprintf(buffer, " @ 0x%016llX", (ULONG64)address);

				ZYAN_CHECK(::ZyanStringViewInsideBuffer(&view, buffer));
				ZYAN_CHECK(::ZyanStringAppend(string, &view));

				ZYAN_CHECK(::ZyanStringViewInsideBuffer(&view, I->symbolEpilogue.c_str()));
				ZYAN_CHECK(::ZyanStringAppend(string, &view));

				return ZYAN_STATUS_SUCCESS;
			}
		}
	}

	// do nothing: call the default implementation.

	return I->defaultPrintAddressAbsolute(formatter, buffer, context);
}
