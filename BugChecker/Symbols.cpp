#include "Symbols.h"

BOOLEAN Symbols::IsBcsValid(BCSFILE_HEADER* header)
{
	return
		header &&
		header->magic == BCSFILE_HEADER_SIGNATURE &&
		header->version == BCSFILE_HEADER_VERSION;
}

template<typename T, typename P>
static T* BinarySearch(T* table, int num, const P& pred)
{
	int start_index = 0;
	int end_index = num - 1;

	while (start_index <= end_index)
	{
		int middle = start_index + (end_index - start_index) / 2;

		auto& s = table[middle];

		auto comp = pred(s);
		if (!comp)
			return &s;
		else if (comp > 0)
			start_index = middle + 1;
		else
			end_index = middle - 1;
	}

	return NULL;
}

const CHAR* Symbols::GetNameOfPublic(VOID* symbols, ULONG rva, BCSFILE_PUBLIC_SYMBOL** symbolPtrPtr /*= NULL*/)
{
	if (symbolPtrPtr) *symbolPtrPtr = NULL;

	// verify the integrity of the symbols file.

	BCSFILE_HEADER* header = (BCSFILE_HEADER*)symbols;

	if (!IsBcsValid(header) || header->publicSymbolsSize == 0)
		return NULL;

	BCSFILE_PUBLIC_SYMBOL* pubs = (BCSFILE_PUBLIC_SYMBOL*)((BYTE*)symbols + header->publicSymbols);
	int pubsNum = header->publicSymbolsSize / sizeof(BCSFILE_PUBLIC_SYMBOL);

	CHAR* names = (CHAR*)((BYTE*)symbols + header->names);

	// binary searches the public symbol.

	auto search = ::BinarySearch(pubs, pubsNum, [rva](const auto& c) {
		if (rva >= c.address && rva < c.address + c.length)
			return 0;
		else if (rva > c.address)
			return 1;
		else
			return -1;
	});

	if (!search)
		return NULL;
	else
	{
		if (symbolPtrPtr) *symbolPtrPtr = search;
		return names + search->name;
	}
}

const BCSFILE_DATATYPE* Symbols::GetDatatype(VOID* symbols, const char* typeName)
{
	// verify the integrity of the symbols file.

	BCSFILE_HEADER* header = (BCSFILE_HEADER*)symbols;

	if (!IsBcsValid(header) || header->datatypesSize == 0 || header->datatypeMembersSize == 0)
		return NULL;

	auto dts = (BCSFILE_DATATYPE*)((BYTE*)symbols + header->datatypes);
	int dtsNum = header->datatypesSize / sizeof(BCSFILE_DATATYPE);

	CHAR* names = (CHAR*)((BYTE*)symbols + header->names);

	// binary searches the datatype.

	auto search = ::BinarySearch(dts, dtsNum, [&typeName, &names](const auto& c) {
		return ::strcmp(typeName, names + c.name);
	});

	if (!search)
		return NULL;

	// multiple types with the same name can be present: return the most complete type.

	int i = search - dts - 1;

	while (i >= 0)
		if (::strcmp(typeName, names + dts[i].name))
			break;
		else
			i--;

	for (i++; i < dtsNum && !::strcmp(typeName, names + dts[i].name); i++)
		if (dts[i].length > search->length)
			search = &dts[i];

	return search;
}

const BCSFILE_DATATYPE_MEMBER* Symbols::GetDatatypeMember(VOID* symbols, const BCSFILE_DATATYPE* datatype, const char* memberName, const char* expectedType)
{
	if (!datatype)
		return NULL;

	// verify the integrity of the symbols file.

	BCSFILE_HEADER* header = (BCSFILE_HEADER*)symbols;

	if (!IsBcsValid(header) || header->datatypesSize == 0 || header->datatypeMembersSize == 0)
		return NULL;

	auto mbs = (BCSFILE_DATATYPE_MEMBER*)((BYTE*)symbols + header->datatypeMembers);
	int mbsNum = header->datatypeMembersSize / sizeof(BCSFILE_DATATYPE_MEMBER);

	CHAR* names = (CHAR*)((BYTE*)symbols + header->names);

	// search the member.

	mbs += datatype->firstMember;

	for (int i = 0; i < datatype->numOfMembers; i++)
		if (!::strcmp(memberName, names + mbs[i].name))
		{
			if (expectedType && ::strcmp(expectedType, names + mbs[i].datatype))
				continue;

			return mbs + i;
		}

	return NULL;
}

const CHAR* Symbols::GetNameFromOffset(VOID* symbols, ULONG offset)
{
	BCSFILE_HEADER* header = (BCSFILE_HEADER*)symbols;

	CHAR* names = (CHAR*)((BYTE*)symbols + header->names);

	return names + offset;
}

const BCSFILE_PUBLIC_SYMBOL* Symbols::GetPublicByName(VOID* symbols, const char* name)
{
	// verify the integrity of the symbols file.

	BCSFILE_HEADER* header = (BCSFILE_HEADER*)symbols;

	if (!IsBcsValid(header) || header->publicSymbolsSize == 0)
		return NULL;

	BCSFILE_PUBLIC_SYMBOL* pubs = (BCSFILE_PUBLIC_SYMBOL*)((BYTE*)symbols + header->publicSymbols);
	int pubsNum = header->publicSymbolsSize / sizeof(BCSFILE_PUBLIC_SYMBOL);

	CHAR* names = (CHAR*)((BYTE*)symbols + header->names);

	// search for the public symbol.

	for (int i = 0; i < pubsNum; i++)
	{
		const CHAR* n = names + pubs[i].name;

		if (!::strcmp(n, name))
			return &pubs[i];
	}

	return NULL;
}

VOID Symbols::EnumPublics(VOID* symbols, VOID* context, BOOLEAN(*callback)(VOID* context, const CHAR* name, ULONG address, ULONG length))
{
	// verify the integrity of the symbols file.

	BCSFILE_HEADER* header = (BCSFILE_HEADER*)symbols;

	if (!IsBcsValid(header) || header->publicSymbolsSize == 0)
		return;

	BCSFILE_PUBLIC_SYMBOL* pubs = (BCSFILE_PUBLIC_SYMBOL*)((BYTE*)symbols + header->publicSymbols);
	int pubsNum = header->publicSymbolsSize / sizeof(BCSFILE_PUBLIC_SYMBOL);

	CHAR* names = (CHAR*)((BYTE*)symbols + header->names);

	// enumerate.

	for (int i = 0; i < pubsNum; i++)
	{
		const CHAR* n = names + pubs[i].name;

		if (!callback(context, n, pubs[i].address, pubs[i].length))
			return;
	}

	return;
}
