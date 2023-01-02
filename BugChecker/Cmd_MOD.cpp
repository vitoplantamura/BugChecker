#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "Utils.h"

class Cmd_MOD : public Cmd
{
public:

	virtual const CHAR* GetId() { return "MOD"; }
	virtual const CHAR* GetDesc() { return "Display module information."; }
	virtual const CHAR* GetSyntax() { return "MOD [-u|-s] [search-string]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		// check the arguments.

		auto args = TokenizeArgs(params.cmd, "MOD", "-u", "-s");

		eastl::string search;
		BOOLEAN user = FALSE;
		BOOLEAN system = FALSE;

		for (int i = 1; i < args.size(); i++)
		{
			if (Utils::AreStringsEqualI(args[i].c_str(), "-u"))
				user = TRUE;
			else if (Utils::AreStringsEqualI(args[i].c_str(), "-s"))
				system = TRUE;
			else if (!search.size())
				search = args[i];
			else
			{
				Print("Syntax error.");
				co_return;
			}
		}

		if (user && system)
		{
			Print("-u and -s cannot be specified at the same time.");
			co_return;
		}

		if (!user && !system)
		{
			user = TRUE;
			system = TRUE;
		}

		// print the header.

		auto printLine = [](const eastl::string& modName, const eastl::string& arch, const eastl::string& dbg, const eastl::string& base, const eastl::string& size) {

			const int nameMaxLen = 32;
			eastl::string name = modName.substr(0, nameMaxLen - 1);

			const int baseMaxLen = _6432_(16, 8) + 1;

			eastl::string line =
				name + eastl::string(_MAX_(1, nameMaxLen - (int)name.size()), ' ') +
				arch + eastl::string(_MAX_(1, 5 - (int)arch.size()), ' ') +
				dbg + eastl::string(_MAX_(1, 4 - (int)dbg.size()), ' ') +
				base + eastl::string(_MAX_(1, baseMaxLen - (int)base.size()), ' ') +
				size;

			Print(line.c_str());
		};

		printLine("NAME", "ARCH", "DBG", "BASE", "SIZE");

		// print the list.

		eastl::vector<ULONG64> eprocs;

		if (system)
		{
			co_await BcAwaiter_Join{ Platform::RefreshNtModulesForProc0() };

			eprocs.push_back(0); // kernel space.
		}

		if (user)
		{
			ULONG64 currentEproc = 0;
			co_await BcAwaiter_Join{ Platform::GetCurrentEprocess(currentEproc) };

			if (currentEproc)
				eprocs.push_back(currentEproc); // user space.
		}

		for (ULONG64 eproc : eprocs)
		{
			auto fif = eastl::find_if(Root::I->NtModules->begin(), Root::I->NtModules->end(),
				[&](const eastl::pair<ULONG64, eastl::vector<NtModule>>& e) { return e.first == eproc; });

			if (fif != Root::I->NtModules->end())
			{
				for (auto& mod : fif->second)
				{
					const CHAR* name = mod.dllName;
					const CHAR* arch = mod.arch == 32 ? "32" : mod.arch == 64 ? "64" : "???";
					const CHAR* dbg = "no";
					eastl::string base = Utils::HexToString(mod.dllBase, _6432_(sizeof(ULONG64), sizeof(ULONG32)));
					eastl::string size = Utils::HexToString(mod.sizeOfImage, sizeof(ULONG32));

					for (BYTE b : mod.pdbGuid)
						if (b)
						{
							dbg = "yes";
							break;
						}

					if (!search.size() ||
						Utils::stristr(name, search.c_str()) ||
						Utils::stristr(arch, search.c_str()) ||
						Utils::stristr(dbg, search.c_str()) ||
						Utils::stristr(base.c_str(), search.c_str()) ||
						Utils::stristr(size.c_str(), search.c_str()))
					{
						printLine(name, arch, dbg, base, size);
					}
				}
			}
		}

		// return to the caller.

		co_return;
	}
};

REGISTER_COMMAND(Cmd_MOD)
