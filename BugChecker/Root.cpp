#include "Root.h"

#include "Utils.h"
#include "CrtFill.h"

#include <EASTL/algorithm.h>
#include <EASTL/string.h>

Root* Root::I = NULL;

SymbolFile* Root::GetSymbolFileByGuidAndAge(const BYTE* guid, const ULONG age)
{
	auto f = eastl::find_if(SymbolFiles.begin(), SymbolFiles.end(), [&guid, &age](SymbolFile& sf) {

		auto h = sf.GetHeader();
		if (h && !::memcmp(h->guid, guid, 16) && h->age == age)
			return TRUE;

		return FALSE;
	});

	return f == SymbolFiles.end() ? NULL : f;
}

Root::Root()
{
	// calculate the cursor blink time.

	CalculateRdtscTimeouts();

	// get the list of the symbol files to load.

	eastl::vector<eastl::string> ret;

	ULONG size = 0;
	auto file = Utils::LoadFileAsByteArray(L"\\SystemRoot\\BugChecker\\BugChecker.dat", &size);

	if (file)
	{
		// enumerate the symbol files.

		static CHAR line[1024];

		CHAR* p = Utils::ParseStructuredFile((BYTE*)file, size, 0, "symbols", NULL, &line);

		if (p)
		{
			while (1)
			{
				p = Utils::ParseStructuredFile((BYTE*)file, size, 1, NULL, p, &line);
				if (!p)
					break;

				ret.push_back(line + 1); // "+1" skips the tab
			}
		}

		// read other configuration options.

		auto read = [&](const CHAR* l0, const CHAR* l1, const CHAR* def) -> const CHAR* {

			p = Utils::ParseStructuredFile((BYTE*)file, size, 0, "settings", NULL, &line);
			if (!p) return def;

			p = Utils::ParseStructuredFile((BYTE*)file, size, 1, l0, p, &line);
			if (!p) return def;

			p = Utils::ParseStructuredFile((BYTE*)file, size, 2, l1, p, &line);
			if (!p) return def;

			p = Utils::ParseStructuredFile((BYTE*)file, size, 3, NULL, p, &line);
			if (!p) return def;

			return line + 3; // "+3" skips the tabs
		};

		fbWidth = (ULONG)::BC_strtoui64(read("framebuffer", "width", "0"), NULL, 10);
		fbHeight = (ULONG)::BC_strtoui64(read("framebuffer", "height", "0"), NULL, 10);
		fbAddress = ::BC_strtoui64(read("framebuffer", "address", "0"), NULL, 16);
		fbStride = (ULONG)::BC_strtoui64(read("framebuffer", "stride", "0"), NULL, 10);

		bcDebug = ::BC_strtoui64(read("framebuffer", "debug", "0"), NULL, 10) != 0;

		auto vms = Utils::Tokenize(read("framebuffer", "vm_screen", "0"), ",");
		if (vms.size() == 3)
		{
			for (auto& s : vms)
				s.trim();

			VmFifoPhys = ::BC_strtoui64(vms[0].c_str(), NULL, 16);
			VmFifoSize = (ULONG)(::BC_strtoui64(vms[1].c_str(), NULL, 16) - VmFifoPhys + 1);
			VmIoPort = (ULONG) ::BC_strtoui64(vms[2].c_str(), NULL, 16);
		}

		auto hook = read("kdcom", "hook", "");

		kdcomHook = KdcomHook::None;

		if (!::strcmp(hook, "callback")) kdcomHook = KdcomHook::Callback;
		else if (!::strcmp(hook, "patch")) kdcomHook = KdcomHook::Patch;

		// free the file contents.

		delete[] file;
	}

	// load each symbol file and add it to the vector.

	for (auto& s : ret)
	{
		SymbolFile sf{ s.c_str() };

		if (sf.IsValid())
			SymbolFiles.push_back(eastl::move(sf));
	}
}

VOID Root::CalculateRdtscTimeouts() // =fixfix= RDTSC is not ideal/reliable here, but blinking the cursor is not a mission critical operation (ref: https://docs.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps)
{
	LARGE_INTEGER freq = {};
	LARGE_INTEGER start = ::KeQueryPerformanceCounter(&freq);

	ULONG64 rdtscStart = __rdtsc();

	while (TRUE)
	{
		auto end = ::KeQueryPerformanceCounter(NULL);
		if (end.QuadPart - start.QuadPart > freq.QuadPart / 10)
			break;
	}

	ULONG64 diff = __rdtsc() - rdtscStart;

	CursorBlinkTime = diff * 3;
	VideoRestoreBufferTimeOut = diff / 2;
}

BpTrace& BpTrace::Get()
{
	int index = -1;

	for (auto& bpt : Root::I->BpTraces)
		if (bpt.first == Root::I->StateChange.Thread)
		{
			index = &bpt - Root::I->BpTraces.begin();
			break;
		}

	if (index < 0) // not found: insert new item at position 0.
	{
		Root::I->BpTraces.insert(Root::I->BpTraces.begin(),
			eastl::pair<ULONG64, BpTrace>(Root::I->StateChange.Thread, BpTrace()));

		size_t dim = 3 * _MAX_(Root::I->StateChange.Processor + 1, Root::I->StateChange.NumberProcessors);

		Root::I->BpTraces.resize(dim);
	}
	else if (index > 0) // found at a position != 0: move it to position 0. NOTE that for "index == 0", we do nothing.
	{
		eastl::pair<ULONG64, BpTrace> el = Root::I->BpTraces[index];

		Root::I->BpTraces.erase(Root::I->BpTraces.begin() + index);

		Root::I->BpTraces.insert(Root::I->BpTraces.begin(), el);
	}

	return Root::I->BpTraces[0].second;
}
