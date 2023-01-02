#include "InputLine.h"

#include "Root.h"
#include "Utils.h"
#include "Wnd.h"

#include "EASTL/sort.h"

VOID InputLine::AddChar(CHAR c)
{
	LONG pos = (Root::I->CursorX - 2) + offset;
	if (pos > text.size())
		return;

	text.insert(pos, 1, c);

	if (Root::I->CursorX < Root::I->WndWidth - 2)
		Wnd::ChangeCursorX(Root::I->CursorX + 1);
	else
		offset++;

	Finalize();
}

eastl::pair<eastl::string, Cmd*> InputLine::Enter()
{
	eastl::string ret1 = text;
	Cmd* ret2 = NULL;

	ret1.trim();

	if (ret1.size())
	{
		// try to determine which Cmd should be executed.

		Root::I->LogWindow.AddUserCmd(ret1.c_str());

		auto cmd = ret1.substr(0, ret1.find_first_of(' '));

		for (auto& c : Root::I->Cmds)
		{
			if (cmd.size() == ::strlen(c.get()->GetId()) &&
				Utils::stristr(cmd.c_str(), c.get()->GetId()))
			{
				ret2 = c.get();
				break;
			}
		}

		if (!ret2)
			Root::I->LogWindow.AddString("Unrecognized command.");
	}

	text = "";
	offset = 0;
	historyPos = -1;

	Wnd::ChangeCursorX(2);

	Finalize();

	return eastl::pair<eastl::string, Cmd*>(eastl::move(ret1), ret2);
}

VOID InputLine::BackSpace()
{
	if (!text.size())
		return;

	LONG pos = (Root::I->CursorX - 2) + offset - 1;
	if (pos < 0)
		return;

	text.erase(pos, 1);

	if (Root::I->CursorX > 2)
		Wnd::ChangeCursorX(Root::I->CursorX - 1);
	else if (offset > 0)
		offset--;

	Finalize();
}

VOID InputLine::Left()
{
	if (Root::I->CursorX > 2)
		Wnd::ChangeCursorX(Root::I->CursorX - 1);
	else if (offset > 0)
		offset--;

	Finalize();
}

VOID InputLine::Right()
{
	LONG pos = (Root::I->CursorX - 2) + offset;
	if (pos >= text.size())
		return;

	if (Root::I->CursorX < Root::I->WndWidth - 2)
		Wnd::ChangeCursorX(Root::I->CursorX + 1);
	else
		offset++;

	Finalize();
}

VOID InputLine::Del()
{
	LONG pos = (Root::I->CursorX - 2) + offset;
	if (pos >= text.size())
		return;

	text.erase(pos, 1);

	Finalize();
}

VOID InputLine::Home()
{
	offset = 0;

	Wnd::ChangeCursorX(2);

	Finalize();
}

VOID InputLine::End()
{
	offset = 0;

	LONG x = text.size() + 2;

	if (x > Root::I->WndWidth - 2)
	{
		x = Root::I->WndWidth - 2;
		offset = text.size() - (Root::I->WndWidth - 4);
	}

	Wnd::ChangeCursorX(x);

	Finalize();
}

VOID InputLine::Up()
{
	LONG i = historyPos;

	if (i < 0 || i >= Root::I->LogWindow.contents.size())
		i = Root::I->LogWindow.contents.size();

	for (i--; i >= 0; i--)
	{
		auto& s = Root::I->LogWindow.contents[i];
		if (s.size() > 2 && s[0] == '\r' && s[1] == ':')
		{
			text = s.substr(2);
			End();
			historyPos = i;
			return;
		}
	}
}

VOID InputLine::Down()
{
	LONG i = historyPos;

	if (i < 0 || i >= Root::I->LogWindow.contents.size())
		return;

	for (i++; i < Root::I->LogWindow.contents.size(); i++)
	{
		auto& s = Root::I->LogWindow.contents[i];
		if (s.size() > 2 && s[0] == '\r' && s[1] == ':')
		{
			text = s.substr(2);
			End();
			historyPos = i;
			return;
		}
	}
}

VOID InputLine::Tab(BOOLEAN modifierKeyState)
{
	// is this the first TAB press?

	if (!tabData.text.size())
	{
		tabData.text = text;
		tabData.offset = offset;
		tabData.CursorX = Root::I->CursorX;
		tabData.index = 0;
		tabData.numOfItems = 0;
	}
	else
	{
		if (!tabData.numOfItems)
			return;

		text = tabData.text;
		offset = tabData.offset;
		Wnd::ChangeCursorX(tabData.CursorX);

		tabData.index += modifierKeyState ? -1 : +1;

		if (tabData.index < 0)
			tabData.index = tabData.numOfItems - 1;
		else if (tabData.index >= tabData.numOfItems)
			tabData.index = 0;
	}

	// can we continue?

	if (!text.size())
		return;

	LONG pos = (Root::I->CursorX - 2) + offset;
	if (pos > text.size())
		return;

	if (!pos)
		return;
	else if (pos < text.size() && text[pos] != ' ')
		return;

	// get the user word to search and complete.

	auto p = text.find_last_of(' ', pos - 1) + 1;

	auto l = pos - p;

	auto t = text.substr(p, l);

	if (!t.size())
		return;

	// get the complete word to replace.

	tabData.numOfItems = 0;

	eastl::string r = Wnd::HighlightHexNumber(t, tabData.index, &tabData.numOfItems);

	{
		tabData.t = &t;
		tabData.r = &r;

		for (auto& symF : Root::I->SymbolFiles)
		{
			Symbols::EnumPublics(symF.GetHeader(), &tabData, [](VOID* context, const CHAR* name, ULONG address, ULONG length) -> BOOLEAN
			{
				if (::strchr(name, ' ')) return TRUE; // skip symbols with spaces in them.

				TabData& tabData = *(TabData*)context;

				if (Utils::stristr(name, tabData.t->c_str()))
				{
					if (tabData.index == tabData.numOfItems)
						*tabData.r = name;

					tabData.numOfItems++;
				}

				return TRUE;
			});
		}
	}

	// complete word must be longer or same size than previous text.

	if (r.size() < t.size())
		return;

	// replace the text.

	text.erase(p, l);
	text.insert(p, r);

	// set cursor pos.

	auto d = r.size() - t.size();

	LONG x = Root::I->CursorX + d;

	if (x > Root::I->WndWidth - 2)
	{
		offset += x - (Root::I->WndWidth - 2);
		x = Root::I->WndWidth - 2;
	}

	Wnd::ChangeCursorX(x);

	// final draw.

	Wnd::Draw_InputLine(TRUE);
	Finalize(FALSE);
}

VOID InputLine::RedrawCallback()
{
	tabData.text = "";
}

VOID InputLine::Finalize(BOOLEAN redrawWnds /*= TRUE*/)
{
	// set the help text.

	helpText = "";

	eastl::string t = text;

	t.ltrim();

	if (t.size())
	{
		auto cmd = t.substr(0, t.find_first_of(' '));

		eastl::vector<eastl::string> hints;

		for (auto& c : Root::I->Cmds)
		{
			if (cmd.size() == ::strlen(c.get()->GetId()) &&
				Utils::stristr(cmd.c_str(), c.get()->GetId()))
			{
				if (t.size() == cmd.size())
					helpText = c.get()->GetDesc();
				else
					helpText = c.get()->GetSyntax();

				break;
			}
			else if (Utils::stristr(c.get()->GetId(), t.c_str()))
			{
				hints.push_back(c.get()->GetId());
			}
		}

		if (!helpText.size() && hints.size())
		{
			eastl::sort(hints.begin(), hints.end());

			for (int i = 0; i < hints.size(); i++)
			{
				helpText += hints[i];

				if (i != hints.size() - 1)
					helpText += ", ";
			}
		}
	}

	// final draw.

	if (redrawWnds) Wnd::DrawAll_End();
	Wnd::DrawAll_Final();
}
