#include "CodeWnd.h"

#include "Root.h"
#include "QuickJSCppInterface.h"
#include "Utils.h"

CodeWnd::CodeWnd()
{
}

eastl::string CodeWnd::GetLineToDraw(ULONG index)
{
	eastl::string s = contents[index];

	CHAR c = ':';

	if (index == posY && posY)
		c = 0x1E;
	else if (index == posY + (destY1 - destY0) && index + 1 < contents.size())
		c = 0x1F;

	s = "\n" + Utils::HexToString(Wnd::hrzClr, sizeof(BYTE)) + eastl::string(1, c) + "\n" + Utils::HexToString(Wnd::nrmClr, sizeof(BYTE)) + s;

	return s;
}

eastl::string& CodeWnd::GetLine(ULONG y, BOOLEAN add /*= FALSE*/, eastl::string* deletedStr /*= NULL */)
{
	while (y >= contents.size())
		contents.push_back("");

	if (add)
	{
		y++;
		contents.insert(contents.begin() + y, "");
	}
	else if (deletedStr)
	{
		*deletedStr = contents[y];
		contents.erase(contents.begin() + y);
	}

	auto& text = deletedStr ? *deletedStr : contents[y];

	eastl::string temp;
	StrLen(text.c_str(), &temp); // remove color tags
	text = eastl::move(temp);

	return text;
}

ULONG CodeWnd::GetCurrentLineId()
{
	return posY + (Root::I->CursorY - destY0);
}

eastl::string& CodeWnd::GetCurrentLine()
{
	return GetLine(GetCurrentLineId());
}

VOID CodeWnd::SyntaxColor(ULONG y)
{
	auto& text = GetLine(y);

	eastl::string textUnc = text; // non colored line

	auto indexToPos = [&text](int index) {

		return StrLen(text.substr(0, index).c_str());
	};

	eastl::vector<eastl::pair<int, int>> exclusion;

	auto exAdd = [&](int start, int end) {

		start = indexToPos(start);

		if (end >= 0)
			end = indexToPos(end);

		for (int i = exclusion.size() - 1; i >= 0; i--)
			if (exclusion[i].second < 0)
				exclusion.erase(exclusion.begin() + i);

		exclusion.emplace_back(start, end);
	};

	auto exCheck = [&](int index) -> BOOLEAN {

		index = indexToPos(index);

		for (auto& p : exclusion)
			if (index > p.first && (p.second < 0 || index < p.second))
				return TRUE;

		return FALSE;
	};

	int strIndex = -1;
	int slashCn = 0;

	for (int i = 0; i < text.size(); i++)
	{
		CHAR c = text[i];

		if (c == '/')
		{
			if (strIndex == -1 && i > 0 && text[i - 1] == '/')
			{
				exAdd(i - 1, -1);
				text.insert(i - 1, "\n" + Utils::HexToString(Wnd::hrzClr, sizeof(BYTE)));
				break;
			}
		}
		else if (c == '\'' || c == '"' || c == '`')
		{
			if (strIndex == -1)
			{
				exAdd(i, -1);
				text.insert(i, "\n04");
				i += 3;
				strIndex = i;
			}
			else if (text[strIndex] == c && !(slashCn % 2))
			{
				exAdd(strIndex, i);
				text.insert(i + 1, "\n" + Utils::HexToString(Wnd::nrmClr, sizeof(BYTE)));
				i += 3;
				strIndex = -1;
			}
		}

		if (c == '\\' && strIndex != -1)
			slashCn++;
		else
			slashCn = 0;
	}

	const CHAR* keywords[] = {
		"break","case","catch","continue","debugger","default","delete","do","else",
		"finally","for","function","if","in","instanceof","new","return","switch",
		"this","throw","try","typeof","while","with","class","const","enum","import",
		"export","extends","super","implements","interface","let","package","private",
		"protected","public","static","yield","undefined","null","true","false",
		"Infinity","NaN","eval","arguments","await","async","void","var"
	};

	for (auto keyword : keywords)
	{
		const auto keywordSize = ::strlen(keyword);

		auto isIdChar = [&](size_t pos, int offset) -> BOOLEAN {

			if (pos < 0 || pos >= text.size())
				return FALSE;

			auto posUnc = indexToPos(pos);

			posUnc += offset;

			if (posUnc < 0 || posUnc >= textUnc.size())
				return FALSE;

			CHAR c = textUnc[posUnc];

			return
				(c >= '0' && c <= '9') ||
				(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z');
		};

		size_t pos = 0;

		while ((pos = text.find(keyword, pos)) != eastl::string::npos)
		{
			if (!isIdChar(pos, -1) && !isIdChar(pos, keywordSize) && !exCheck(pos))
			{
				text.insert(pos + keywordSize, "\n" + Utils::HexToString(Wnd::nrmClr, sizeof(BYTE)));
				text.insert(pos, "\n03");

				pos += 3 + 3;
			}

			pos += keywordSize;
		}
	}
}

VOID CodeWnd::Finalize()
{
	SyntaxColor(GetCurrentLineId());

	Wnd::DrawAll_End();
	Wnd::DrawAll_Final();
}

VOID CodeWnd::Add(const eastl::string& str)
{
	auto& text = GetCurrentLine();

	LONG pos = (Root::I->CursorX - 2) + posX;
	if (pos < 0 || pos > text.size())
		return;

	text.insert(pos, str);

	if (Root::I->CursorX < Root::I->WndWidth - 1 - str.size())
		Wnd::ChangeCursorX(Root::I->CursorX + str.size());
	else
		posX += str.size();

	Finalize();
}

VOID CodeWnd::Enter()
{
	auto& text = GetCurrentLine();

	LONG pos = (Root::I->CursorX - 2) + posX;
	if (pos < 0 || pos > text.size())
		return;

	auto text2 = text.substr(pos);
	text.erase(pos);

	auto& next = GetLine(GetCurrentLineId(), TRUE);

	next = eastl::move(text2);

	posX = 0;

	auto cursorY = Root::I->CursorY + 1;

	if (cursorY > destY1)
	{
		cursorY--;
		posY++;
	}

	Wnd::ChangeCursorX(2, cursorY);

	SyntaxColor(GetCurrentLineId() - 1);
	Finalize();
}

VOID CodeWnd::BackSpace()
{
	auto id = GetCurrentLineId();

	LONG pos = (Root::I->CursorX - 2) + posX - 1;

	if (pos >= 0)
	{
		auto& text = GetLine(id);

		if (!text.size())
			return;

		if (pos >= text.size())
			return;

		text.erase(pos, 1);

		if (Root::I->CursorX > 2)
			Wnd::ChangeCursorX(Root::I->CursorX - 1);
		else if (posX > 0)
			posX--;
	}
	else
	{
		if (!id)
			return;

		eastl::string auxStr;
		auto& del = GetLine(id, FALSE, &auxStr);

		auto& text = GetLine(id - 1);
		auto textSize = text.size();

		text += del;

		posX = 0;

		LONG x = textSize + 2;

		if (x > Root::I->WndWidth - 2)
		{
			x = Root::I->WndWidth - 2;
			posX = textSize - (Root::I->WndWidth - 4);
		}

		auto cursorY = Root::I->CursorY - 1;

		if (cursorY < destY0)
		{
			cursorY++;
			posY--;
		}

		Wnd::ChangeCursorX(x, cursorY);
	}

	Finalize();
}

VOID CodeWnd::Del()
{
	auto& text = GetCurrentLine();

	LONG pos = (Root::I->CursorX - 2) + posX;
	if (pos < 0)
		return;

	if (pos < text.size())
	{
		text.erase(pos, 1);
	}
	else
	{
		auto id = GetCurrentLineId() + 1;
		if (id >= contents.size())
			return;

		eastl::string auxStr;
		auto& del = GetLine(id, FALSE, &auxStr);

		text += del;
	}

	Finalize();
}

VOID CodeWnd::Left()
{
	if (Root::I->CursorX > 2)
		Wnd::ChangeCursorX(Root::I->CursorX - 1);
	else if (posX > 0)
		posX--;
	else
	{
		auto id = GetCurrentLineId();
		if (!id)
			return;

		auto& text = GetLine(id - 1);
		auto textSize = text.size();

		posX = 0;

		LONG x = textSize + 2;

		if (x > Root::I->WndWidth - 2)
		{
			x = Root::I->WndWidth - 2;
			posX = textSize - (Root::I->WndWidth - 4);
		}

		auto cursorY = Root::I->CursorY - 1;

		if (cursorY < destY0)
		{
			cursorY++;
			posY--;
		}

		Wnd::ChangeCursorX(x, cursorY);
	}

	Finalize();
}

VOID CodeWnd::Right()
{
	auto id = GetCurrentLineId();
	auto& text = GetLine(id);

	LONG pos = (Root::I->CursorX - 2) + posX;
	if (pos < 0)
		return;

	if (pos < text.size())
	{
		if (Root::I->CursorX < Root::I->WndWidth - 2)
			Wnd::ChangeCursorX(Root::I->CursorX + 1);
		else
			posX++;
	}
	else
	{
		if (id + 1 >= contents.size())
			return;

		posX = 0;

		auto cursorY = Root::I->CursorY + 1;

		if (cursorY > destY1)
		{
			cursorY--;
			posY++;
		}

		Wnd::ChangeCursorX(2, cursorY);

		SyntaxColor(id);
	}

	Finalize();
}

VOID CodeWnd::Up()
{
	auto id = GetCurrentLineId();
	if (!id)
		return;

	auto& text = GetLine(id - 1);
	auto textSize = text.size();

	posX = 0;

	LONG x = _MIN_(textSize + 2, Root::I->CursorX);

	if (x > Root::I->WndWidth - 2)
	{
		x = Root::I->WndWidth - 2;
		posX = textSize - (Root::I->WndWidth - 4);
	}

	auto cursorY = Root::I->CursorY - 1;

	if (cursorY < destY0)
	{
		cursorY++;
		posY--;
	}

	Wnd::ChangeCursorX(x, cursorY);

	Finalize();
}

VOID CodeWnd::Down()
{
	auto id = GetCurrentLineId();
	if (id + 1 >= contents.size())
		return;

	auto& text = GetLine(id + 1);
	auto textSize = text.size();

	posX = 0;

	LONG x = _MIN_(textSize + 2, Root::I->CursorX);

	if (x > Root::I->WndWidth - 2)
	{
		x = Root::I->WndWidth - 2;
		posX = textSize - (Root::I->WndWidth - 4);
	}

	auto cursorY = Root::I->CursorY + 1;

	if (cursorY > destY1)
	{
		cursorY--;
		posY++;
	}

	Wnd::ChangeCursorX(x, cursorY);

	Finalize();
}

VOID CodeWnd::Home()
{
	posX = 0;

	Wnd::ChangeCursorX(2);

	Finalize();
}

VOID CodeWnd::End()
{
	auto& text = GetCurrentLine();
	auto textSize = text.size();

	posX = 0;

	LONG x = textSize + 2;

	if (x > Root::I->WndWidth - 2)
	{
		x = Root::I->WndWidth - 2;
		posX = textSize - (Root::I->WndWidth - 4);
	}

	Wnd::ChangeCursorX(x);

	Finalize();
}

VOID CodeWnd::Tab()
{
	LONG pos = (Root::I->CursorX - 2) + posX;
	pos = 4 - (pos % 4);
	if (pos <= 0 || pos > 4)
		return;

	Add(eastl::string(pos, ' '));
}

VOID CodeWnd::PgUp()
{
	LONG id = (LONG)GetCurrentLineId();
	if (!id)
		return;

	const LONG winDim = destY1 - destY0 + 1;

	id -= winDim;
	if (id < 0)
		id = 0;

	auto& text = GetLine(id);
	auto textSize = text.size();

	posX = 0;

	LONG x = _MIN_(textSize + 2, Root::I->CursorX);

	if (x > Root::I->WndWidth - 2)
	{
		x = Root::I->WndWidth - 2;
		posX = textSize - (Root::I->WndWidth - 4);
	}

	auto cursorY = Root::I->CursorY;

	if (posY >= winDim)
		posY -= winDim;
	else
	{
		cursorY = destY0 + id;
		posY = 0;
	}

	Wnd::ChangeCursorX(x, cursorY);

	Finalize();
}

VOID CodeWnd::PgDn()
{
	LONG id = (LONG)GetCurrentLineId();
	if (id + 1 >= contents.size())
		return;

	const LONG winDim = destY1 - destY0 + 1;

	id += winDim;
	if (id >= contents.size())
		id = contents.size() - 1;

	auto& text = GetLine(id);
	auto textSize = text.size();

	posX = 0;

	LONG x = _MIN_(textSize + 2, Root::I->CursorX);

	if (x > Root::I->WndWidth - 2)
	{
		x = Root::I->WndWidth - 2;
		posX = textSize - (Root::I->WndWidth - 4);
	}

	auto cursorY = Root::I->CursorY;

	if (posY + winDim < contents.size())
		posY += winDim;

	cursorY = destY0 + (id - (LONG)posY);

	Wnd::ChangeCursorX(x, cursorY);

	Finalize();
}

BcCoroutine CodeWnd::Eval(BYTE* context, ULONG contextLen, BOOLEAN is32bitCompat) noexcept
{
	eastl::string s;

	for (auto& line : contents)
	{
		eastl::string temp;
		StrLen(line.c_str(), &temp); // remove color tags
		s += temp + "\n";
	}

	co_await BcAwaiter_Join{ QuickJSCppInterface::Eval(s.c_str(), context, contextLen, is32bitCompat, NULL, FALSE, FALSE) };
}
