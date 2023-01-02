#pragma once

#include "framework.h"

#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/algorithm.h>

class StructuredFile
{
public:

	class Entry
	{
	public:
		eastl::string name;
		eastl::vector<Entry> children;

		Entry* get(const CHAR* name, int* pindex = NULL)
		{
			if (pindex) *pindex = -1;

			auto el = eastl::find_if(children.begin(), children.end(), [name](const Entry& e) { return e.name == name; });

			if (el == eastl::end(children))
				return NULL;
			else
			{
				if (pindex) *pindex = el - children.begin();

				return el;
			}
		}

		void AddSetting(const CHAR* n, const CHAR* v)
		{
			Entry e{ n };
			e.children.push_back(Entry{ v });
			this->children.push_back(eastl::move(e));
		}

		Entry* GetSetting(const CHAR* n)
		{
			auto e = get(n);
			if (!e || e->children.size() != 1) return NULL;
			return e->children.begin();
		}

		void SetSetting(const CHAR* n, const CHAR* v)
		{
			auto e = GetOrAdd(n);
			e->children.clear();
			e->children.push_back(Entry{ v });
		}

		Entry* GetOrAdd(const CHAR* name)
		{
			auto e = get(name);
			if (e)
				return e;
			this->children.push_back(Entry{ name });
			return &this->children.back();
		}
	};

	static CHAR* Parse(IN BYTE* pbFile, IN ULONG ulSize, IN ULONG ulTabsNum, IN const CHAR* pszString, IN CHAR* pszStart, OUT CHAR(*paOutput)[1024]); // this function is from the first BugChecker.

	static Entry* LoadConfig();
	static BOOL SaveConfig(Entry* config);
};
