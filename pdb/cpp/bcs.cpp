#define _CRT_SECURE_NO_WARNINGS

#include "bcs.h"
#include "bcsfile.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <algorithm>

class Name
{
public:
	std::string name;
	ULONG pos;
};

class PublicSymbol
{
public:
	std::size_t name;
	ULONG address;
	ULONG length;
};

class DataType
{
public:
	std::size_t name;
	std::size_t kind;
	ULONG length;
	DWORD pdbIndex;
	ULONG firstMember;
	ULONG numOfMembers;
};

class DataTypeMember
{
public:
	std::size_t name;
	std::size_t datatype;
	ULONG offset;
	DWORD pdbDatatypeIndex;
};

static std::vector<PublicSymbol> PublicSymbols;
static std::vector<Name> Names;
static std::vector<DataType> DataTypes;
static std::vector<DataTypeMember> DataTypeMembers;

static GUID Guid;
static DWORD Age;

void BCS_SavePdbInfo(GUID& guid, DWORD age)
{
	Guid = guid;
	Age = age;
}

static std::size_t AddName(const char* name)
{
	std::size_t nameIndex;

	auto it = std::find_if(Names.begin(), Names.end(), [&name](const Name& n) { return n.name == name; });
	if (it != Names.end())
	{
		nameIndex = it - Names.begin();
	}
	else
	{
		nameIndex = Names.size();
		Names.push_back(Name{ name, 0 });
	}

	return nameIndex;
}

void BCS_AddPublicSymbol(const char* name, ULONG address, ULONG length)
{
	if (::strlen(name) == 0)
		return;

	// save the symbol name avoiding duplicates.

	auto nameIndex = ::AddName(name);

	// save the symbol data.

	PublicSymbols.push_back(PublicSymbol{ nameIndex, address, length });
}

BOOL BCS_AddDataType(const char* name, const char* kind, ULONG length, DWORD index)
{
	if (::strlen(name) == 0 || ::strlen(kind) == 0)
		return FALSE;

	// save the symbol name avoiding duplicates.

	auto nameIndex = ::AddName(name);

	auto kindIndex = ::AddName(kind);

	// save the type.

	DataTypes.push_back(DataType{ nameIndex, kindIndex, length, index, static_cast<ULONG>(DataTypeMembers.size()), 0 });

	return TRUE;
}

void BCS_AddDataTypeMember(const char* name, const char* datatype, ULONG offset, DWORD datatypeIndex)
{
	if (::strlen(name) == 0 || ::strlen(datatype) == 0)
		return;

	// save the symbol name avoiding duplicates.

	auto nameIndex = ::AddName(name);

	auto datatypeNameIndex = ::AddName(datatype);

	// save the member.

	DataTypeMembers.push_back(DataTypeMember{ nameIndex, datatypeNameIndex, offset, datatypeIndex });

	// increase the number of members.

	DataTypes.back().numOfMembers++;

	return;
}

void BCS_WriteFile()
{
	// sort the symbols by RVA.

	std::sort(PublicSymbols.begin(), PublicSymbols.end(), [](const auto& ls, const auto& rs)
	{
		return ls.address < rs.address;
	});

	// sort the datatypes by name.

	std::sort(DataTypes.begin(), DataTypes.end(), [](const auto& ls, const auto& rs)
	{
		return Names[ls.name].name < Names[rs.name].name;
	});

	// compose the final byte buffer of symbol names.

	std::vector<std::byte> namesBuff;

	for (auto& name : Names)
	{
		name.pos = (ULONG)namesBuff.size();

		for (const char c : name.name)
			namesBuff.push_back((std::byte)c);
		namesBuff.push_back((std::byte)0);
	}

	// write the file.

	auto fp = ::fopen("output.bcs", "wb"); // fstream doesn't work on XP (it doesn't find InitializeCriticalSectionEx).

	auto file_write = [&fp](const char* buff, std::size_t size) { ::fwrite(buff, 1, size, fp); };

	if (fp)
	{
		// write the file header.

		BCSFILE_HEADER header;

		header.magic = BCSFILE_HEADER_SIGNATURE;
		header.version = BCSFILE_HEADER_VERSION;

		*(GUID*)header.guid = Guid;
		header.age = Age;

		header.publicSymbols = sizeof(header);
		header.publicSymbolsSize = (ULONG)(sizeof(BCSFILE_PUBLIC_SYMBOL) * PublicSymbols.size());

		header.datatypes = header.publicSymbols + header.publicSymbolsSize;
		header.datatypesSize = (ULONG)(sizeof(BCSFILE_DATATYPE) * DataTypes.size());

		header.datatypeMembers = header.datatypes + header.datatypesSize;
		header.datatypeMembersSize = (ULONG)(sizeof(BCSFILE_DATATYPE_MEMBER) * DataTypeMembers.size());

		header.names = header.datatypeMembers + header.datatypeMembersSize;
		header.namesSize = (ULONG)namesBuff.size();

		file_write(reinterpret_cast<char*>(&header), sizeof(header));

		// write the public symbols.

		for (const auto& s : PublicSymbols)
		{
			BCSFILE_PUBLIC_SYMBOL symbol;

			symbol.name = Names[s.name].pos;
			symbol.address = s.address;
			symbol.length = s.length;

			file_write(reinterpret_cast<char*>(&symbol), sizeof(symbol));
		}

		// write the datatypes.

		for (const auto& t : DataTypes)
		{
			BCSFILE_DATATYPE datatype;

			datatype.name = Names[t.name].pos;
			datatype.kind = Names[t.kind].pos;
			datatype.length = t.length;
			datatype.pdbIndex = t.pdbIndex;
			datatype.firstMember = t.firstMember;
			datatype.numOfMembers = t.numOfMembers;

			file_write(reinterpret_cast<char*>(&datatype), sizeof(datatype));
		}

		// write the datatype members.

		for (const auto& m : DataTypeMembers)
		{
			BCSFILE_DATATYPE_MEMBER member;

			member.name = Names[m.name].pos;
			member.datatype = Names[m.datatype].pos;
			member.offset = m.offset;
			member.pdbDatatypeIndex = m.pdbDatatypeIndex;

			file_write(reinterpret_cast<char*>(&member), sizeof(member));
		}

		// write the names.

		file_write(reinterpret_cast<char*>(namesBuff.data()), namesBuff.size());

		// close the file.

		::fclose(fp);
	}
}
