#include "BugChecker.h"

#include "Cmd.h"
#include "Root.h"
#include "Utils.h"

class Cmd_PROC : public Cmd
{
public:

	virtual const CHAR* GetId() { return "PROC"; }
	virtual const CHAR* GetDesc() { return "Display process information."; }
	virtual const CHAR* GetSyntax() { return "PROC [search-string]"; }

	virtual BcCoroutine Execute(CmdParams& params) noexcept
	{
		// check the arguments.

		eastl::string search;

		auto args = TokenizeArgs(params.cmd, "PROC");

		if (args.size() > 2)
		{
			Print("Too many arguments.");
			co_return;
		}
		else if (args.size() == 2)
			search = args[1];

		// get offset and size of the ImageFileName field.

		LONG offsetFn = -1;
		LONG sizeFn = 0;

		VOID* symH = NULL;
		auto type = GetNtDatatype("_EPROCESS", &symH);

		if (type && symH)
		{
			auto member15 = Symbols::GetDatatypeMember(symH, type, "ImageFileName", "uchar[15]");
			auto member16 = Symbols::GetDatatypeMember(symH, type, "ImageFileName", "uchar[16]");

			if (member15)
			{
				offsetFn = member15->offset;
				sizeFn = 15;
			}
			else if (member16)
			{
				offsetFn = member16->offset;
				sizeFn = 16;
			}
		}

		// get offset and size of the UniqueProcessId field.

		LONG offsetId = -1;
		LONG sizeId = 0;

		if (type && symH)
		{
			auto member = Symbols::GetDatatypeMember(symH, type, "UniqueProcessId", "void *");

			if (member)
			{
				offsetId = member->offset;
				sizeId = sizeof(void*);
			}
		}

		// print the header.

		auto printLine = [](const eastl::string& name, const eastl::string& id, const eastl::string& eproc) {

			eastl::string line =
				name + eastl::string(_MAX_(1, 20 - (int)name.size()), ' ') +
				id + eastl::string(_MAX_(1, 8 - (int)id.size()), ' ') +
				eproc;

			Print(line.c_str());
		};

		printLine("NAME", "PID", "EPROCESS");

		// iterate through all the processes.

		for (auto& proc : *Root::I->NtModules)
		{
			// get the process name.

			eastl::string name;

			if (offsetFn >= 0 && sizeFn)
			{
				VOID* ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)proc.first + offsetFn, (size_t)sizeFn };
				if (ptr)
				{
					CHAR buffer[32] = { 0 };

					::memcpy(buffer, ptr, sizeFn);

					name = buffer;
				}
			}

			if (!name.size())
				name = "???";

			// get the process id.

			eastl::string id;

			if (offsetId >= 0 && sizeId)
			{
				VOID* ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)proc.first + offsetId, (size_t)sizeId };
				if (ptr)
				{
					CHAR buffer[64];

					::sprintf(buffer, _6432_("%llu", "%u"), *(ULONG_PTR*)ptr);

					id = buffer;
				}
			}

			if (!id.size())
				id = "???";

			// get the EPROCESS string.

			eastl::string eproc;

			{
				CHAR buffer[64];

				::sprintf(buffer, _6432_("%016llX", "%08X"), _6432_(proc.first, (ULONG32)proc.first));

				eproc = buffer;
			}

			// print the line.

			if (!search.size() ||
				Utils::stristr(name.c_str(), search.c_str()) ||
				Utils::stristr(id.c_str(), search.c_str()) ||
				Utils::stristr(eproc.c_str(), search.c_str()))
			{
				printLine(name, id, eproc);
			}
		}

		co_return;
	}
};

REGISTER_COMMAND(Cmd_PROC)
