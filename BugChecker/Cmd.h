#pragma once

#include "BugChecker.h"

#include "BcCoroutine.h"
#include "Symbols.h"

#include "EASTL/string.h"
#include "EASTL/vector.h"

enum class CmdParamsResult
{
	None,
	Continue,
	Unassemble,
	RefreshCodeAndRegsWindows,
	RedrawUi
};

class CmdParams
{
public:

	eastl::string cmd;

	BYTE* context;
	ULONG contextLen;
	BOOLEAN is32bitCompat;

	ULONG64 jumpDest;

	ZydisDecodedInstruction* thisInstr = NULL;
	ZydisDecodedOperand* thisOps = NULL;

	CmdParamsResult result = CmdParamsResult::None;
	ULONG64 resultUL64 = 0;
};

class Cmd
{
public:

	virtual ~Cmd() {}

	virtual const CHAR* GetId() = 0;
	virtual const CHAR* GetDesc() = 0;
	virtual const CHAR* GetSyntax() = 0;

	virtual BcCoroutine Execute(CmdParams& params) noexcept = 0;

public:

	template <typename ... Args>
	static eastl::vector<eastl::string> TokenizeArgs(const eastl::string& str, Args ... args) // NOTE: workaround since std::initializer_list doesn't work.
	{
		const CHAR* vec[] = { args... };

		eastl::vector<eastl::string> delimiters;
		for (auto v : vec)
			delimiters.push_back(v);

		return TokenizeVec(str, delimiters);
	}

	static VOID Print(const CHAR* psz);
	static const BCSFILE_DATATYPE* GetNtDatatype(const CHAR* typeName, VOID** symbols);
	static const BCSFILE_PUBLIC_SYMBOL* GetNtPublicByName(const CHAR* name);
	static eastl::vector<eastl::string> TokenizeStr(const eastl::string& str, const eastl::string& delimiter);
	static eastl::vector<eastl::string> TokenizeVec(const eastl::string& str, eastl::vector<eastl::string>& delimiters);
	static BcCoroutine WriteMemory(ULONG64 dest, VOID* src, ULONG count, BOOLEAN& res) noexcept;
	static BcCoroutine ResolveArg(eastl::pair<BOOLEAN, ULONG64>& res, const CHAR* arg, BYTE* context, ULONG contextLen, BOOLEAN is32bitCompat) noexcept;
	static BcCoroutine PageIn(ULONG_PTR attachVal, ULONG_PTR pageInVal, BOOLEAN& res) noexcept;
	static eastl::vector<ULONG> ParseListOfDecsArgs(const CHAR* cmdId, const eastl::string& cmd, ULONG size);
	static BcCoroutine ScanStackForRets(BOOLEAN& res, eastl::vector<eastl::pair<ULONG64, ULONG64>>& addrs, BOOLEAN& is64, BYTE* context, BOOLEAN is32bitCompat, LONG depth) noexcept;
	static BcCoroutine AddressToSymbol(eastl::string& retVal, ULONG64 l, BOOLEAN is64) noexcept;
	static BOOLEAN IsRet(ZydisMnemonic mnemonic);
	static BcCoroutine StepOver(BYTE* context, ZydisDecodedInstruction* thisInstr) noexcept;
};

typedef Cmd* (*PfnCreateCommand)();

class Cmd_Startup
{
public:

	static PfnCreateCommand* Cmds;
	static LONG CmdsNum;

	static VOID RegisterCommand(PfnCreateCommand pfnCreateCommand);
	static VOID Free();
	static VOID CreateInstances();
};

#define REGISTER_COMMAND(n) \
	static class __register__##n \
	{ \
	public: \
		__register__##n() \
		{ \
			Cmd_Startup::RegisterCommand([]() -> Cmd* { return new n(); }); \
		} \
	} __register__##n;
