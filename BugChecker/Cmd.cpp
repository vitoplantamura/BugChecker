#include "Cmd.h"

#include "Root.h"
#include "X86Step.h"
#include "CrtFill.h"
#include "Main.h"
#include "Utils.h"
#include "QuickJSCppInterface.h"

PfnCreateCommand* Cmd_Startup::Cmds = NULL;
LONG Cmd_Startup::CmdsNum = 0;

VOID Cmd_Startup::RegisterCommand(PfnCreateCommand pfnCreateCommand)
{
	PfnCreateCommand* prev = Cmds;
	auto prevn = CmdsNum;

	CmdsNum++;
	Cmds = (PfnCreateCommand*)::ExAllocatePool(NonPagedPool, CmdsNum * sizeof(PfnCreateCommand));

	Cmds[CmdsNum - 1] = pfnCreateCommand;

	for (int i = 0; i < prevn; i++)
		Cmds[i] = prev[i];

	if (prev)
		::ExFreePool(prev);
}

VOID Cmd_Startup::Free()
{
	if (Cmds)
	{
		::ExFreePool(Cmds);
		Cmds = NULL;
		CmdsNum = 0;
	}
}

VOID Cmd_Startup::CreateInstances()
{
	for (int i = 0; i < CmdsNum; i++)
		Root::I->Cmds.emplace_back(Cmds[i]());
}

VOID Cmd::Print(const CHAR* psz)
{
	Root::I->LogWindow.AddString(psz);

	Wnd::DrawAll_End();
	Wnd::DrawAll_Final();
}

const BCSFILE_DATATYPE* Cmd::GetNtDatatype(const CHAR* typeName, VOID** symbols)
{
	if (symbols) *symbols = NULL;

	auto symF = Root::I->GetSymbolFileByGuidAndAge(Platform::KernelDebugInfo.guid, Platform::KernelDebugInfo.age);
	if (!symF) return NULL;

	auto symH = symF->GetHeader();
	if (!symH) return NULL;

	auto type = Symbols::GetDatatype(symH, typeName);
	if (!type) return NULL;

	if (symbols) *symbols = symH;

	return type;
}

const BCSFILE_PUBLIC_SYMBOL* Cmd::GetNtPublicByName(const CHAR* name)
{
	auto symF = Root::I->GetSymbolFileByGuidAndAge(Platform::KernelDebugInfo.guid, Platform::KernelDebugInfo.age);
	if (!symF) return NULL;

	auto symH = symF->GetHeader();
	if (!symH) return NULL;

	return Symbols::GetPublicByName(symH, name);
}

eastl::vector<eastl::string> Cmd::TokenizeStr(const eastl::string& str, const eastl::string& delimiter)
{
	eastl::vector<eastl::string> retVal;

	eastl::string s = str;
	size_t pos = 0;

	s.trim();

	while ((pos = s.find(delimiter)) != eastl::string::npos)
	{
		eastl::string token = s.substr(0, pos);
		if (token.size())
			retVal.push_back(eastl::move(token));

		s.erase(0, pos + delimiter.length());
	}

	if (s.size())
		retVal.push_back(eastl::move(s));

	return retVal;
}

eastl::vector<eastl::string> Cmd::TokenizeVec(const eastl::string& str, eastl::vector<eastl::string>& delimiters)
{
	// define a lambda for searching in the string.

	for (auto& del : delimiters)
		Utils::ToLower(del);

	auto find = [&delimiters](eastl::string& thisStr, eastl::string& matchingDel) -> size_t {

		eastl::string sL = thisStr;
		Utils::ToLower(sL);

		size_t ret = eastl::string::npos;
		matchingDel = "";
		auto matchingDelPtr = delimiters.end();

		for (auto del = delimiters.begin(); del != delimiters.end(); del++)
		{
			size_t f = sL.find(*del);

			if (f != eastl::string::npos)
				if (ret == eastl::string::npos || f < ret)
				{
					if (
						(f == 0 || sL[f - 1] == ' ') &&
						(f + del->size() >= sL.size() || sL[f + del->size()] == ' ')
						)
					{
						ret = f;
						matchingDel = *del;
						matchingDelPtr = del;
					}
				}
		}

		if (matchingDelPtr != delimiters.end())
			delimiters.erase(matchingDelPtr);

		return ret;
	};

	// search the "delimiters" in the string and split it accordingly.

	eastl::vector<eastl::string> retVal;

	eastl::string s = str;
	s.trim();

	size_t pos = 0;
	eastl::string delimiter;

	while ((pos = find(s, delimiter)) != eastl::string::npos)
	{
		eastl::string token = s.substr(0, pos);
		token.trim();
		if (token.size())
			retVal.push_back(eastl::move(token));

		s.erase(0, pos + delimiter.length());

		retVal.push_back(eastl::move(delimiter));
	}

	s.trim();
	if (s.size())
		retVal.push_back(eastl::move(s));

	// return the result to the caller.

	return retVal;
}

BcCoroutine Cmd::WriteMemory(ULONG64 dest, VOID* src, ULONG count, BOOLEAN& res) noexcept
{
	res = co_await BcAwaiter_StateManipulate{
	[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

		SecondBuffer->Length = !SecondBuffer->pData ? 0 : _MIN_(SecondBuffer->MaxLength, count);
		if (SecondBuffer->pData)
			::memcpy(SecondBuffer->pData, src, SecondBuffer->Length);

		pState->ApiNumber = DbgKdWriteVirtualMemoryApi;

		pState->ReturnStatus = STATUS_PENDING;

		pState->u.WriteMemory.TargetBaseAddress = dest;
		pState->u.WriteMemory.TransferCount = count;
		pState->u.WriteMemory.ActualBytesWritten = 0;
	},
	[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

		if (pState->ApiNumber == DbgKdWriteVirtualMemoryApi)
		{
			if (NT_SUCCESS(pState->ReturnStatus) &&
				pState->u.WriteMemory.ActualBytesWritten == count)
			{
				return TRUE;
			}
		}

		return FALSE;
	} };

	co_return;
}

BcCoroutine Cmd::ResolveArg(eastl::pair<BOOLEAN, ULONG64>& res, const CHAR* arg, BYTE* context, ULONG contextLen, BOOLEAN is32bitCompat) noexcept
{
	res.first = FALSE;
	res.second = 0;

	// is it a register?

	ZydisRegister reg = X86Step::StringToZydisReg(arg);

	if (reg != ZYDIS_REGISTER_NONE)
	{
		size_t sz = 0;
		VOID* pval = X86Step::ZydisRegToCtxRegValuePtr((CONTEXT*)context, reg, &sz);

		if (pval && (sz == 8 || sz == 4))
		{
			res.first = TRUE;
			res.second = sz == 8 ? *(ULONG64*)pval : *(ULONG32*)pval;
			co_return;
		}
	}

	// is it an hex value?

	{
		const CHAR* ptr = arg;

		if (*ptr == '0' && *(ptr + 1) == 'x')
			ptr += 2;

		auto len = ::strlen(ptr);

		if (len > 0 && len <= 16)
		{
			int i;
			for (i = 0; i < len; i++)
			{
				CHAR c = ptr[i];

				if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
					continue;
				else
					break;
			}

			if (i == len)
			{
				res.first = TRUE;
				res.second = ::BC_strtoui64(ptr, NULL, 16);
				co_return;
			}
		}
	}

	// is it a symbol name?

	class Data
	{
	public:
		const CHAR* arg;
		ULONG address;
		ULONG length;
		BCSFILE_HEADER* header;
	};

	Data data{ arg, 0, 0, NULL };

	for (auto& symF : Root::I->SymbolFiles)
	{
		Symbols::EnumPublics(symF.GetHeader(), &data, [](VOID* context, const CHAR* name, ULONG address, ULONG length) -> BOOLEAN
		{
			Data& data = *(Data*)context;

			if (!::strcmp(name, data.arg) && address && length)
			{
				data.address = address;
				data.length = length;
				return FALSE;
			}

			return TRUE;
		});

		if (data.address && data.length)
		{
			data.header = symF.GetHeader();
			break;
		}
	}

	if (data.address && data.length && data.header)
	{
		co_await BcAwaiter_Join{ Platform::RefreshNtModulesForProc0() };

		ULONG64 currentEproc = 0;
		co_await BcAwaiter_Join{ Platform::GetCurrentEprocess(currentEproc) };

		eastl::vector<ULONG64> eprocs;

		if (currentEproc)
			eprocs.push_back(currentEproc); // user space.

		eprocs.push_back(0); // kernel space.

		for (ULONG64 eproc : eprocs)
		{
			auto fif = eastl::find_if(Root::I->NtModules->begin(), Root::I->NtModules->end(),
				[&](const eastl::pair<ULONG64, eastl::vector<NtModule>>& e) { return e.first == eproc; });

			if (fif != Root::I->NtModules->end())
			{
				for (auto& mod : fif->second)
				{
					if (mod.pdbAge == data.header->age &&
						!::memcmp(mod.pdbGuid, data.header->guid, 16))
					{
						res.first = TRUE;
						res.second = mod.dllBase + data.address;
						co_return;
					}
				}
			}
		}

		// print a specific error message.

		Print("Symbol found but not loaded in current address space. Try changing process context (ADDR command).");

		co_return;
	}

	// try with QuickJS.

	NullableU64 nu64;

	co_await BcAwaiter_Join{ QuickJSCppInterface::Eval(arg, context, contextLen, is32bitCompat, &nu64, TRUE) };

	if (!nu64.isNull)
	{
		res.first = TRUE;
		res.second = nu64.value;
		co_return;
	}
	else
	{
		Print("Unable to convert javascript result to ULONG64.");
	}

	co_return;
}

BcCoroutine Cmd::PageIn(ULONG_PTR attachVal, ULONG_PTR pageInVal, BOOLEAN& fnRes) noexcept
{
	fnRes = FALSE;

	// set the workitem for the KD worker thread.

	auto expDebuggerWork = GetNtPublicByName("ExpDebuggerWork");
	auto expDebuggerProcessAttach = GetNtPublicByName("ExpDebuggerProcessAttach");
	auto expDebuggerPageIn = GetNtPublicByName("ExpDebuggerPageIn");

	if (!expDebuggerWork || !expDebuggerProcessAttach || !expDebuggerPageIn)
	{
		expDebuggerWork = GetNtPublicByName("_ExpDebuggerWork");
		expDebuggerProcessAttach = GetNtPublicByName("_ExpDebuggerProcessAttach");
		expDebuggerPageIn = GetNtPublicByName("_ExpDebuggerPageIn");

		if (!expDebuggerWork || !expDebuggerProcessAttach || !expDebuggerPageIn)
		{
			Print("KD worker thread symbols not found.");
			co_return;
		}
	}

	VOID* ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)Platform::KernelDebugInfo.startAddr + expDebuggerWork->address, sizeof(ULONG) };

	if (!ptr)
	{
		Print("Unable to read memory.");
		co_return;
	}
	else if (*(ULONG*)ptr > 1)
	{
		Print("KD worker thread has a pending command.");
		co_return;
	}

	BOOLEAN res;

	co_await BcAwaiter_Join{ WriteMemory((ULONG64)(ULONG_PTR)Platform::KernelDebugInfo.startAddr + expDebuggerProcessAttach->address, &attachVal, sizeof(attachVal), res) };

	if (!res)
	{
		Print("Unable to write memory.");
		co_return;
	}

	co_await BcAwaiter_Join{ WriteMemory((ULONG64)(ULONG_PTR)Platform::KernelDebugInfo.startAddr + expDebuggerPageIn->address, &pageInVal, sizeof(pageInVal), res) };

	if (!res)
	{
		Print("Unable to write memory.");
		co_return;
	}

	ULONG workVal = 1;
	co_await BcAwaiter_Join{ WriteMemory((ULONG64)(ULONG_PTR)Platform::KernelDebugInfo.startAddr + expDebuggerWork->address, &workVal, sizeof(workVal), res) };

	if (!res)
	{
		Print("Unable to write memory.");
		co_return;
	}

	// return to the caller.

	fnRes = TRUE;

	co_return;
}

eastl::vector<ULONG> Cmd::ParseListOfDecsArgs(const CHAR* cmdId, const eastl::string& cmd, ULONG size)
{
	eastl::vector<ULONG> nums;

	auto argsCmd = TokenizeArgs(cmd, cmdId);

	if (argsCmd.size() < 2)
	{
		Print("Too few arguments.");
		return eastl::vector<ULONG>();
	}
	else if (argsCmd.size() > 2)
	{
		Print("Too many arguments.");
		return eastl::vector<ULONG>();
	}

	auto argsStr = TokenizeStr(argsCmd[1], " ");

	if (argsStr.size() == 0)
	{
		Print("Syntax error.");
		return eastl::vector<ULONG>();
	}

	BOOLEAN all = FALSE;

	for (int i = 0; i < argsStr.size(); i++)
	{
		auto& arg = argsStr[i];

		if (arg == "*")
		{
			all = TRUE;
		}
		else
		{
			BOOLEAN dec = FALSE;
			for (CHAR c : arg)
				if (c >= '0' && c <= '9')
				{
					dec = TRUE;
				}
				else
				{
					dec = FALSE;
					break;
				}

			if (!dec)
			{
				Print("Invalid argument.");
				return eastl::vector<ULONG>();
			}

			ULONG n = (ULONG)::BC_strtoui64(arg.c_str(), NULL, 10);

			if (n >= size)
			{
				Print("One or more arguments are out of range.");
				return eastl::vector<ULONG>();
			}

			nums.push_back(n);
		}
	}

	if (all)
	{
		if (nums.size())
		{
			Print("Specify * or a list of indexes, not both.");
			return eastl::vector<ULONG>();
		}

		for (ULONG i = 0; i < size; i++)
			nums.push_back(i);
	}

	return nums;
}

eastl::vector<BYTE> Cmd::ParseListOfBytesArgs(const CHAR* cmdId, const eastl::string& cmd)
{
	eastl::vector<BYTE> bytes;

	auto argsCmd = TokenizeArgs(cmd, cmdId);

	if (argsCmd.size() < 2)
	{
		Print("Too few arguments.");
		return eastl::vector<BYTE>();
	}
	else if (argsCmd.size() > 2)
	{
		Print("Too many arguments.");
		return eastl::vector<BYTE>();
	}

	auto argsStr = TokenizeStr(argsCmd[1], " ");

	if (argsStr.size() == 0)
	{
		Print("Syntax error.");
		return eastl::vector<BYTE>();
	}

	for (int i = 0; i < argsStr.size(); i++)
	{
		auto& arg = argsStr[i];

		if (arg.length() > 2)
		{
			Print("One or more arguments are out of range.");
			return eastl::vector<BYTE>();
		}
		else
		{
			BOOLEAN byte = FALSE;
			for (CHAR c : arg)
			{
				if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || c >= 'a' && c <= 'f')
				{
					byte = TRUE;
				}
				else
				{
					byte = FALSE;
					break;
				}
			}

			if (!byte)
			{
				Print("Invalid argument.");
				return eastl::vector<BYTE>();
			}

			BYTE n = (BYTE)::BC_strtoui64(arg.c_str(), NULL, 16);

			bytes.push_back(n);
		}
	}

	return bytes;
}

BcCoroutine Cmd::ScanStackForRets(BOOLEAN& res, eastl::vector<eastl::pair<ULONG64, ULONG64>>& addrs, BOOLEAN& is64, ULONG64 sp, BOOLEAN is32bitCompat, LONG depth) noexcept
{
	res = FALSE;
	addrs.clear();
	is64 = FALSE;

	// compose a list of "valid" ranges of memory, so we can exclude as many potential return addresses as possible.

	eastl::vector<eastl::pair<ULONG64, ULONG64>> ranges;

	co_await BcAwaiter_Join{ Platform::RefreshNtModulesForProc0() };

	ULONG64 currentEproc = 0;
	co_await BcAwaiter_Join{ Platform::GetCurrentEprocess(currentEproc) };

	eastl::vector<ULONG64> eprocs;

	if (currentEproc)
		eprocs.push_back(currentEproc); // user space.

	eprocs.push_back(0); // kernel space.

	for (ULONG64 eproc : eprocs)
	{
		auto fif = eastl::find_if(Root::I->NtModules->begin(), Root::I->NtModules->end(),
			[&](const eastl::pair<ULONG64, eastl::vector<NtModule>>& e) { return e.first == eproc; });

		if (fif != Root::I->NtModules->end())
		{
			for (auto& mod : fif->second)
			{
				ranges.emplace_back(mod.dllBase, mod.dllBase + mod.sizeOfImage);
			}
		}
	}

	// get and validate the stack pointer.

	is64 = _6432_(TRUE, FALSE) && !is32bitCompat;

	if (sp % (is64 ? 8 : 4))
	{
		Print("Stack pointer is not aligned.");
		co_return;
	}

	// init the zydis decoder.

	ZydisDecoder decoder;
	::ZydisDecoderInit(&decoder,
		!is64 ? _6432_(ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_MACHINE_MODE_LEGACY_32) : ZYDIS_MACHINE_MODE_LONG_64,
		!is64 ? ZYDIS_STACK_WIDTH_32 : ZYDIS_STACK_WIDTH_64);

	// scan the entire stack searching for return addresses.

	ULONG64 start = sp;
	ULONG64 next = (start & 0xFFFFFFFFFFFFF000) + 0x1000;

	while (depth > 0)
	{
		// fetch the next memory page in the stack.

		BYTE page[0x1000];

		ULONG64 len = next - start;
		if (len > sizeof(page)) // should never happen.
			break;

		VOID* ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)start, (size_t)len };
		if (!ptr)
			break;

		::memcpy(page, ptr, len);

		// scan the memory page searching for return addresses.

		for (ULONG64 i = 0; i < len; i += (is64 ? 8 : 4))
		{
			ULONG64 l = is64 ? *(ULONG64*)(page + i) : *(ULONG32*)(page + i);

			if (!l)
				continue;

			// is this address inside one of the "valid" memory ranges?

			BOOLEAN inRange = FALSE;

			for (auto& range : ranges)
				if (l >= range.first && l < range.second)
				{
					inRange = TRUE;
					break;
				}

			if (!inRange)
				continue;

			// fetch the memory at the POTENTIAL return address.

			BYTE asmb[32 + 32];

			ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)l - sizeof(asmb) / 2, sizeof(asmb) };
			if (!ptr)
				continue;

			::memcpy(asmb, ptr, sizeof(asmb));

			// is the instruction BEFORE the return address a valid CALL?

			BOOLEAN isCall = FALSE;

			for (LONG offset = 0; offset < sizeof(asmb) / 2 - 2; offset++)
			{
				ZydisDecodedInstruction instruction;
				ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];

				if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, asmb + offset, sizeof(asmb) - offset,
					&instruction, operands, ZYDIS_MAX_OPERAND_COUNT_VISIBLE,
					ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY)))
					continue;

				if (!(
					instruction.mnemonic == ZYDIS_MNEMONIC_CALL &&
					offset + instruction.length == sizeof(asmb) / 2 &&
					instruction.operand_count_visible == 1 &&
					(operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE ||
						operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER ||
						(operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY && operands[0].mem.type == ZYDIS_MEMOP_TYPE_MEM))
					))
					continue;

				isCall = TRUE;
				break;
			}

			if (!isCall)
				continue;

			// can add this address.

			addrs.push_back(eastl::pair<ULONG64, ULONG64>(start + i, l));

			if (--depth <= 0)
				break;
		}

		// time to scan the next memory page.

		start = next;
		next += 0x1000;
	}

	// return to the caller.

	res = TRUE;

	co_return;
}

BcCoroutine Cmd::AddressToSymbol(eastl::string& retVal, ULONG64 l, BOOLEAN is64) noexcept
{
	// get the symbol name, if possible.

	CHAR posInModules[350] = { 0 };
	ImageDebugInfo debugInfo = {};

	co_await BcAwaiter_DiscoverPosInModules{ posInModules, &debugInfo, (ULONG_PTR)l };

	SymbolFile* symF = NULL;

	if (debugInfo.startAddr)
		symF = Root::I->GetSymbolFileByGuidAndAge(debugInfo.guid, debugInfo.age);

	if (symF && l >= (ULONG64)debugInfo.startAddr)
	{
		ULONG64 rva = l - (ULONG64)debugInfo.startAddr;
		if (rva < debugInfo.length)
		{
			eastl::string moduleName = "";

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

			BCSFILE_PUBLIC_SYMBOL* sym = NULL;
			auto symName = Symbols::GetNameOfPublic(symF->GetHeader(), (ULONG)rva, &sym);

			if (symName && sym && moduleName.size() + ::strlen(symName) < sizeof(posInModules) - 32)
			{
				::strcpy(posInModules, moduleName.c_str());
				::strcat(posInModules, "!");
				::strcat(posInModules, symName);

				if (rva > sym->address)
					::sprintf(posInModules + ::strlen(posInModules), "+%04X", (ULONG)(rva - sym->address));
			}
		}
	}

	if (::strlen(posInModules) < sizeof(posInModules) - 32)
	{
		if (::strlen(posInModules))
			::strcat(posInModules, " @ ");

		::strcat(posInModules, "0x");

		::sprintf(posInModules + ::strlen(posInModules), _6432_(!is64 ? "%08X" : "%016llX", "%08X"), _6432_(!is64 ? (ULONG32)l : l, (ULONG32)l));
	}

	// return to the caller.

	retVal = posInModules;

	co_return;
}

BOOLEAN Cmd::IsRet(ZydisMnemonic mnemonic)
{
	return mnemonic == ZYDIS_MNEMONIC_RET ||
		mnemonic == ZYDIS_MNEMONIC_IRET ||
		mnemonic == ZYDIS_MNEMONIC_IRETD ||
		mnemonic == ZYDIS_MNEMONIC_IRETQ ||
		mnemonic == ZYDIS_MNEMONIC_SYSEXIT ||
		mnemonic == ZYDIS_MNEMONIC_SYSRET;
}

BcCoroutine Cmd::StepOver(BYTE* context, ZydisDecodedInstruction* thisInstr) noexcept
{
	// check the type of the next instruction.

	if (
		thisInstr->mnemonic == ZYDIS_MNEMONIC_CALL ||
		thisInstr->mnemonic == ZYDIS_MNEMONIC_INT ||
		thisInstr->mnemonic == ZYDIS_MNEMONIC_SYSENTER ||
		thisInstr->mnemonic == ZYDIS_MNEMONIC_SYSCALL ||
		thisInstr->mnemonic == ZYDIS_MNEMONIC_LOOP ||
		thisInstr->mnemonic == ZYDIS_MNEMONIC_LOOPE ||
		thisInstr->mnemonic == ZYDIS_MNEMONIC_LOOPNE ||
		(thisInstr->attributes & ZYDIS_ATTRIB_HAS_REP) ||
		(thisInstr->attributes & ZYDIS_ATTRIB_HAS_REPE) ||
		(thisInstr->attributes & ZYDIS_ATTRIB_HAS_REPNE)
		)
	{
		// step over the instruction with a breakpoint.

		ULONG64 pc = ((CONTEXT*)context)->_6432_(Rip, Eip);

		co_await BcAwaiter_Join{ Main::SetStepBp(pc + thisInstr->length) };
	}
	else
	{
		// normal single step.

		Root::I->Trace = TRUE;
		BpTrace::Get().nextAddr = 0;
	}

	// return to the caller.

	co_return;
}

LONG Cmd::GetNtDatatypeMemberOffset(const CHAR* typeName, const CHAR* memberName, const CHAR* expectedType)
{
	LONG offset = -1;

	VOID* symH = NULL;
	auto type = GetNtDatatype(typeName, &symH);

	if (type && symH)
	{
		auto member = Symbols::GetDatatypeMember(symH, type, memberName, expectedType);

		if (member)
		{
			offset = member->offset;
		}
	}

	return offset;
}
