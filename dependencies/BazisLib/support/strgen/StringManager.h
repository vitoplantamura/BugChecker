#pragma once
#include <map>
#include <bzscore/string.h>
#include <bzscore/file.h>

class LocalizableStringManager
{
private:
	struct DialogMember
	{
		BazisLib::String Name;
		bool Localized;
		bool Skipped;

		DialogMember()
			: Localized(false)
			, Skipped(false)
		{
		}
	};

	struct DialogRecord
	{
		bool Localized;
		BazisLib::String Caption;
		std::map<BazisLib::String, DialogMember> DialogMembers;

		BazisLib::String FileAndLine;
		BazisLib::String LocalizationFileAndLine;

		DialogRecord()
			: Localized(false)
		{
		}
	};

	struct StringRecord
	{
		BazisLib::String Value;
	};


	std::map<BazisLib::String, DialogRecord> m_Dialogs;
	std::map<BazisLib::String, StringRecord> m_Strings;

private:
	BazisLib::String FormatStringASCString(const BazisLib::TempString &str);

public:
	bool ParseResourceFile(const BazisLib::String &fp);
	bool ParseSourceFile(const BazisLib::String &fp);

	void FindAndDisplayStructuralWarnings();

	bool ProduceHeaderFile(const BazisLib::String &fp);
	bool ProduceLanguageFile(const BazisLib::String &fp);

	bool ProduceIDTableFile(const BazisLib::String &fp, const BazisLib::String &HeaderFileName);
};