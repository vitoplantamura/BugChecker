#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "Utils.h"

class Cmd_THREAD : public Cmd
{
public:

	virtual const CHAR* GetId() { return "THREAD"; }
	virtual const CHAR* GetDesc() { return "Display thread information."; }
	virtual const CHAR* GetSyntax() { return "THREAD [-kt thread|-kp process]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		// parse the arguments.

		ULONG64 eprocess = 0;
		ULONG64 ethread = 0;

		auto args = TokenizeArgs(params.cmd, "THREAD", "-kt", "-kp");

		for (int i = 1; i < args.size(); i++)
		{
			if (Utils::AreStringsEqualI(args[i].c_str(), "-kt") || Utils::AreStringsEqualI(args[i].c_str(), "-kp"))
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
			else
			{
				Print("Syntax error.");
				co_return;
			}
		}

		if (eprocess && ethread)
		{
			Print("-kt and -kp cannot be specified at the same time.");
			co_return;
		}
		else if (!eprocess && !ethread)
		{
			// get a pointer to the current eprocess.

			co_await BcAwaiter_Join{ Platform::GetCurrentEprocess(eprocess) };

			if (!eprocess)
			{
				Print("Unable to get current eprocess.");
				co_return;
			}
		}

		// show info about all the threads in a process or detailed info for a particular thread.

		auto GetOffset = [](LONG& offset, const CHAR* typeName, const CHAR* memberName, const CHAR* expectedType) -> BOOLEAN {

			offset = GetNtDatatypeMemberOffset(typeName, memberName, expectedType);

			if (offset == -1)
			{
				Print((eastl::string("Unable to determine ") + memberName + " offset in " + typeName + ".").c_str());
				return FALSE;
			}

			return TRUE;
		};

		LONG offsetTlh, offsetTle, offsetState, offsetKs, offsetCid, offsetUt, offsetTeb, offsetUtFinal, offsetCtxS, offsetW32T;

		if (!GetOffset(offsetTlh, "_KPROCESS", "ThreadListHead", "_LIST_ENTRY")) co_return;
		if (!GetOffset(offsetTle, "_KTHREAD", "ThreadListEntry", "_LIST_ENTRY")) co_return;
		if (!GetOffset(offsetState, "_KTHREAD", "State", "uchar")) co_return;
		if (!GetOffset(offsetKs, "_KTHREAD", "KernelStack", "void *")) co_return;
		if (!GetOffset(offsetCid, "_ETHREAD", "Cid", "_CLIENT_ID")) co_return;
		if (!GetOffset(offsetUt, "_CLIENT_ID", "UniqueThread", "void *")) co_return;
		if (!GetOffset(offsetTeb, "_KTHREAD", "Teb", "void *")) co_return;
		if (!GetOffset(offsetCtxS, "_KTHREAD", "ContextSwitches", "ulong")) co_return;
		if (!GetOffset(offsetW32T, "_KTHREAD", "Win32Thread", "void *")) co_return;

		offsetUtFinal = offsetCid + offsetUt;

		LONG* offsets[] = {&offsetState, &offsetKs, &offsetUtFinal, &offsetTeb, &offsetCtxS, &offsetW32T};

		size_t structSz = 0;
		for (LONG* ptrLong : offsets)
			if (*ptrLong > structSz)
				structSz = *ptrLong;

		structSz += 16;

		auto printLineP = [](const eastl::string& utStr, const eastl::string& thread, const eastl::string& ks, const eastl::string& stateStr, const eastl::string& teb) {

			eastl::string line =
				utStr + eastl::string(_MAX_(1, 8 - (int)utStr.size()), ' ') +
				thread + eastl::string(_MAX_(1, _6432_(16, 8) + 1 - (int)thread.size()), ' ') +
				ks + eastl::string(_MAX_(1, _6432_(16, 8) + 1 - (int)ks.size()), ' ') +
				stateStr + eastl::string(_MAX_(1, 14 - (int)stateStr.size()), ' ') +
				teb;

			Print(line.c_str());
		};

		auto printLineT = [](const eastl::string& str1, const eastl::string& str2) {

			eastl::string line =
				str1 + eastl::string(_MAX_(1, 32 - (int)str1.size()), ' ') +
				str2;

			Print(line.c_str());
		};

		auto dumpThreadInfo = [&](ULONG_PTR threadPtr, ULONG_PTR kThread) {

			BYTE state = *(BYTE*)(threadPtr + offsetState);
			ULONG_PTR ks = *(ULONG_PTR*)(threadPtr + offsetKs);
			ULONG_PTR ut = *(ULONG_PTR*)(threadPtr + offsetUtFinal);
			ULONG_PTR teb = *(ULONG_PTR*)(threadPtr + offsetTeb);
			DWORD ctxs = *(DWORD*)(threadPtr + offsetCtxS);
			ULONG_PTR w32t = *(ULONG_PTR*)(threadPtr + offsetW32T);

			// get a string representation of the thread state.

			eastl::string stateStr = Utils::HexToString(state, sizeof(BYTE));

			switch (state) // from the _KTHREAD_STATE type.
			{
			case 0: stateStr = "Initialized"; break;
			case 1: stateStr = "Ready"; break;
			case 2: stateStr = "Running"; break;
			case 3: stateStr = "Standby"; break;
			case 4: stateStr = "Terminated"; break;
			case 5: stateStr = "Waiting"; break;
			case 6: stateStr = "Transition"; break;
			case 7: stateStr = "DeferredReady"; break;
			case 8: stateStr = "GateWait"; break;
			}

			// print the line.

			CHAR utStr[64];
			::sprintf(utStr, "%X", (DWORD)ut);

			if (eprocess)
			{
				printLineP(
					utStr,
					Utils::HexToString(kThread, sizeof(ULONG_PTR)),
					Utils::HexToString(ks, sizeof(ULONG_PTR)),
					stateStr,
					!teb ? "None" : Utils::HexToString(teb, sizeof(ULONG_PTR)));
			}
			else
			{
				printLineT(
					"Thread ID: " + eastl::string(utStr),
					"KThread: " + Utils::HexToString(kThread, sizeof(ULONG_PTR))
				);

				printLineT(
					"Kernel Stack: " + Utils::HexToString(ks, sizeof(ULONG_PTR)),
					"State: " + stateStr
				);

				printLineT(
					"Teb: " + (!teb ? "None" : Utils::HexToString(teb, sizeof(ULONG_PTR))),
					"Context Switch Count: " + Utils::HexToString(ctxs, sizeof(DWORD))
				);

				printLineT(
					"Win32Thread: " + (!w32t ? "None" : Utils::HexToString(w32t, sizeof(ULONG_PTR))),
					""
				);
			}
		};

		if (eprocess)
		{
			printLineP("TID", "KTHREAD", "KSTACK", "STATE", "TEB");

			// iterate through the threads.

			ULONG64 head = eprocess + offsetTlh;

			ULONG64 Flink = head;

			for (int i = 0; i < 1024; i++)
			{
				// read the pointer to the next entry.

				VOID* ptr;

				ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)Flink, sizeof(VOID*) };
				if (!ptr)
				{
					Print("Unable to read memory.");
					co_return;
				}

				Flink = *(ULONG_PTR*)ptr;

				if (!Flink || Flink == head)
					break;

				// read the thread structure.

				ULONG64 thread = Flink - offsetTle;

				ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)thread, structSz };
				if (!ptr)
					continue;

				dumpThreadInfo((ULONG_PTR)ptr, (ULONG_PTR)thread);
			}
		}
		else if (ethread)
		{
			// dump the thread info.

			VOID* ptr;

			ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)ethread, structSz };
			if (!ptr)
			{
				Print("Unable to read memory.");
				co_return;
			}

			dumpThreadInfo((ULONG_PTR)ptr, (ULONG_PTR)ethread);

			ULONG64 sp = *(ULONG_PTR*)((ULONG_PTR)ptr + offsetKs); // since we are scanning the stack, we don't care about including the context switch frame here as well!
			BOOLEAN is32bc = FALSE;

			// scan the stack searching for return addresses.

			BOOLEAN res = FALSE;
			eastl::vector<eastl::pair<ULONG64, ULONG64>> addrs;
			BOOLEAN is64 = FALSE;

			co_await BcAwaiter_Join{ ScanStackForRets(res, addrs, is64, sp, is32bc, 999999) };

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
		}

		// return to the caller.

		co_return;
	}
};

REGISTER_COMMAND(Cmd_THREAD)
