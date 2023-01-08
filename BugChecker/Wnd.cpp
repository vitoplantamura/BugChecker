#include "Wnd.h"

#include "Root.h"
#include "Glyph.h"

#include <EASTL/unique_ptr.h>

BYTE Wnd::hlpClr = 0x30; // default help bar color

void Wnd::DrawAll_Start()
{
	// check if the dimension of the screen changed.

	auto dim = Glyph::GetScreenDim();

	if (dim.first && dim.second &&
		(dim.first != Root::I->fbWidth || dim.second != Root::I->fbHeight))
	{
		Root::I->fbWidth = dim.first;
		Root::I->fbHeight = dim.second;
		Root::I->fbStride = dim.first * 4;

		if (!CheckWndWidthHeight(Root::I->WndWidth, Root::I->WndHeight))
		{
			Root::I->WndWidth = 80;
			Root::I->WndHeight = 25;
		}
	}

	// check if the sizes of the windows are ok.

	CheckDivLineYs();

	// save the overwritten area of the framebuffer.

	SaveOrRestoreFrameBuffer(FALSE);

	::memset(Root::I->FrontBuffer, 0, sizeof(Root::I->FrontBuffer));

	// set the position of each window.

	auto resetDest = [](Wnd* w) {

		w->destX0 = 1;
		w->destY0 = 1;
		w->destX1 = Root::I->WndWidth - 2;
		w->destY1 = 3;
	};

	ULONG nextY = 1;

	if (Root::I->RegsDisasmDivLineY < 0)
		resetDest(&Root::I->RegsWindow);
	else
	{
		Root::I->RegsWindow.destX0 = 1;
		Root::I->RegsWindow.destY0 = nextY;
		Root::I->RegsWindow.destX1 = Root::I->WndWidth - 2;
		Root::I->RegsWindow.destY1 = Root::I->RegsDisasmDivLineY - 1;

		nextY = Root::I->RegsDisasmDivLineY + 1;
	}

	if (Root::I->DisasmCodeDivLineY < 0)
		resetDest(&Root::I->DisasmWindow);
	else
	{
		Root::I->DisasmWindow.destX0 = 1;
		Root::I->DisasmWindow.destY0 = nextY;
		Root::I->DisasmWindow.destX1 = Root::I->WndWidth - 2;
		Root::I->DisasmWindow.destY1 = Root::I->DisasmCodeDivLineY - 1;

		nextY = Root::I->DisasmCodeDivLineY + 1;
	}

	if (Root::I->CodeLogDivLineY < 0)
		resetDest(&Root::I->CodeWindow);
	else
	{
		Root::I->CodeWindow.destX0 = 1;
		Root::I->CodeWindow.destY0 = nextY;
		Root::I->CodeWindow.destX1 = Root::I->WndWidth - 2;
		Root::I->CodeWindow.destY1 = Root::I->CodeLogDivLineY - 1;

		nextY = Root::I->CodeLogDivLineY + 1;
	}

	Root::I->LogWindow.destX0 = 1;
	Root::I->LogWindow.destY0 = nextY;
	Root::I->LogWindow.destX1 = Root::I->WndWidth - 2;
	Root::I->LogWindow.destY1 = Root::I->WndHeight - 4;

	// check if there is a pending GoToEnd in the log window.

	if (Root::I->LogWindow.GoToEnd_Pending)
	{
		Root::I->LogWindow.GoToEnd_Pending = FALSE;
		Root::I->LogWindow.GoToEnd();
	}
}

void Wnd::Draw_InputLine(BOOLEAN eraseBackground)
{
	ULONG y = Root::I->WndHeight - 3;

	if (eraseBackground)
		for (ULONG x = 1; x <= Root::I->WndWidth - 2; x++)
			Root::I->BackBuffer[y * Root::I->WndWidth + x] = 0x0720;

	DrawString(":", 1, y);

	if (Root::I->InputLine.offset < Root::I->InputLine.text.size())
		DrawString(Root::I->InputLine.text.c_str() + Root::I->InputLine.offset, 2, y, 0x07, Root::I->WndWidth - 3);
}

void Wnd::DrawAll_End()
{
	// draw the window "frame" in the back buffer.

	USHORT* p = Root::I->BackBuffer;

	for (ULONG y = 0; y < Root::I->WndHeight; y++)
		for (ULONG x = 0; x < Root::I->WndWidth; x++)
		{
			USHORT c = 0x0720;

			if (x == 0 || x == Root::I->WndWidth - 1)
				c = 0x03B3;

			if (y == 0)
			{
				if (x == 0)
					c = 0x03DA;
				else if (x == Root::I->WndWidth - 1)
					c = 0x03BF;
				else
					c = 0x03C4;
			}
			else if (y == Root::I->WndHeight - 1)
			{
				if (x == 0)
					c = 0x03C0;
				else if (x == Root::I->WndWidth - 1)
					c = 0x03D9;
				else
					c = 0x03C4;
			}
			else if (y == Root::I->WndHeight - 2)
			{
				if (c == 0x0720)
					c = (hlpClr << 8) + (c & 0xff);
			}
			else if (y == Root::I->RegsDisasmDivLineY || y == Root::I->DisasmCodeDivLineY || y == Root::I->CodeLogDivLineY)
			{
				if (c == 0x0720)
					c = 0x02C4;
			}

			*p++ = c;
		}

	// draw the colon and the input line.

	Draw_InputLine(FALSE);

	// draw the windows.

	if (Root::I->RegsDisasmDivLineY >= 0)
		Root::I->RegsWindow.Draw();

	if (Root::I->DisasmCodeDivLineY >= 0)
		Root::I->DisasmWindow.Draw();

	if (Root::I->CodeLogDivLineY >= 0)
		Root::I->CodeWindow.Draw();

	Root::I->LogWindow.Draw();

	// print the help text and current image file name.

	eastl::string helpText;

	if (Root::I->CursorFocus == CursorFocus::Log)
	{
		if (!Root::I->InputLine.helpText.size())
			helpText = "     Enter a command";
		else
			helpText = Root::I->InputLine.helpText;
	}
	else if (Root::I->CursorFocus == CursorFocus::Code)
	{
		helpText = "     ESC=switch windows   F1=evaluate script";
	}

	DrawString(helpText.c_str(), 1, Root::I->WndHeight - 2, hlpClr, Root::I->WndWidth - 2);

	DrawString(Root::I->CurrentImageFileName.c_str(), Root::I->WndWidth - Root::I->CurrentImageFileName.size() - 2, Root::I->WndHeight - 2, hlpClr);

	// compose and draw the entire IRQL + "posInModules" string.

	BYTE posClr = 0x03;
	ULONG posY = 0;

	if (Root::I->CodeLogDivLineY >= 0)
	{
		posClr = 0x02;
		posY = Root::I->CodeLogDivLineY;
	}
	else if (Root::I->DisasmCodeDivLineY >= 0)
	{
		posClr = 0x02;
		posY = Root::I->DisasmCodeDivLineY;
	}
	else if (Root::I->RegsDisasmDivLineY >= 0)
	{
		posClr = 0x02;
		posY = Root::I->RegsDisasmDivLineY;
	}

	eastl::unique_ptr<CHAR[]> buffer(new CHAR[128]); // <=== 128 chars is more than enough here...

	::strcpy(buffer.get(), "IRQL=");

	if (Root::I->CurrentIrql >= 0 && Root::I->CurrentIrql <= 31)
	{
		switch (Root::I->CurrentIrql)
		{
		case 00: ::strcat(buffer.get(), "(PASSIVE)"); break;
		case 01: ::strcat(buffer.get(), "(APC)"); break;
		case 02: ::strcat(buffer.get(), "(DISPATCH)"); break;
		case 05: ::strcat(buffer.get(), "(CMCI)"); break;
		case 27: ::strcat(buffer.get(), "(PROFILE)"); break;
		case 28: ::strcat(buffer.get(), "(CLOCK)"); break;
		case 29: ::strcat(buffer.get(), "(IPI)"); break;
		case 30: ::strcat(buffer.get(), "(POWER)"); break;
		case 31: ::strcat(buffer.get(), "(HIGH)"); break;
		default: ::sprintf(buffer.get() + ::strlen(buffer.get()), "%02X", Root::I->CurrentIrql); break;
		}
	}
	else
	{
		::strcat(buffer.get(), "???");
	}

	::sprintf(buffer.get() + ::strlen(buffer.get()),
		" KTHREAD(" _6432_("%016llX", "%08X") ") CPU(%i/%i)",
		_6432_((ULONG64), (ULONG32)) Root::I->StateChange.Thread,
		Root::I->StateChange.Processor, Root::I->StateChange.NumberProcessors);

	DrawString((eastl::string(buffer.get()) + "     " + Root::I->DisasmWindow.posInModules).c_str(),
		1, posY, posClr, Root::I->WndWidth - 2, 0xC4);

	// print the header text.

	BYTE htClr = 0;
	ULONG htY = 0;

	if (Root::I->RegsDisasmDivLineY >= 0)
	{
		htClr = 0x02;
		htY = Root::I->RegsDisasmDivLineY;
	}
	else if (Root::I->DisasmCodeDivLineY >= 0)
	{
		htClr = 0x03;
		htY = 0;
	}

	if (htClr && htY != posY)
		DrawString(Root::I->DisasmWindow.headerText, 6, htY, htClr, Root::I->WndWidth - 6 - 1);

	// call the input line callback.

	Root::I->InputLine.RedrawCallback();
}

void Wnd::DrawAll_Final()
{
	if (!CheckWndWidthHeight(Root::I->WndWidth, Root::I->WndHeight))
		return;

	ULONG centerX = (Root::I->fbWidth - (Root::I->WndWidth * Root::I->GlyphWidth)) / 2;
	ULONG centerY = (Root::I->fbHeight - (Root::I->WndHeight * Root::I->GlyphHeight)) / 2;

	USHORT* front = Root::I->FrontBuffer;
	USHORT* back = Root::I->BackBuffer;

	for (ULONG y = 0; y < Root::I->WndHeight; y++)
		for (ULONG x = 0; x < Root::I->WndWidth; x++)
		{
			if (*front != *back)
			{
				*front = *back;

				Glyph::Draw(*front & 0xFF, *front >> 8, x, y, Root::I->VideoAddr, centerX, centerY, x == Root::I->CursorDisplayX && y == Root::I->CursorDisplayY);
			}

			front++;
			back++;
		}

	UpdateScreen();
}

void Wnd::Draw()
{
	for (ULONG y = destY0; y <= destY1; y++)
	{
		ULONG index = (y - destY0) + posY;
		const CHAR* ptr = index < contents.size() ? GetLineToDraw(index).c_str() : NULL;

		BYTE clr = 0x07;

		if (ptr)
			for (ULONG i = 0; i < posX; i++)
				if (*ptr)
				{
					while (*ptr == '\n' && IsHex(*(ptr + 1)) && IsHex(*(ptr + 2)))
					{
						clr = (ToHex(*(ptr + 1)) << 4) | ToHex(*(ptr + 2));
						ptr += 3;
					}

					if (*ptr)
						ptr++;
				}

		for (ULONG x = destX0; x <= destX1; x++)
		{
			USHORT c = 0x20 | (clr << 8);

			if (ptr && *ptr)
			{
				while (*ptr == '\n' && IsHex(*(ptr + 1)) && IsHex(*(ptr + 2)))
				{
					clr = (ToHex(*(ptr + 1)) << 4) | ToHex(*(ptr + 2));
					ptr += 3;
				}

				if (*ptr)
				{
					c = ((BYTE)*ptr) | (clr << 8);
					ptr++;
				}
			}

			Root::I->BackBuffer[y * Root::I->WndWidth + x] = c;
		}
	}
}

ULONG Wnd::StrLen(const CHAR* ptr, eastl::string* pOut /*= NULL*/)
{
	ULONG ret = 0;

	while (TRUE)
	{
		while (*ptr == '\n' && IsHex(*(ptr + 1)) && IsHex(*(ptr + 2)))
			ptr += 3;

		if (*ptr)
		{
			if (pOut)
				(*pOut) += *ptr;

			ptr++;
			ret++;
		}
		else
			break;
	}

	return ret;
}

const CHAR* Wnd::StrCount(const CHAR* ptr, ULONG c)
{
	for (ULONG i = 0; i < c; i++)
	{
		while (*ptr == '\n' && IsHex(*(ptr + 1)) && IsHex(*(ptr + 2)))
			ptr += 3;

		if (*ptr)
			ptr++;
		else
			return NULL;
	}

	return ptr;
}

BOOLEAN Wnd::IsHex(CHAR c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

BYTE Wnd::ToHex(CHAR c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'A' && c <= 'F')
		return (c - 'A') + 10;
	else
		return 0;
}

void Wnd::DrawString(const CHAR* psz, ULONG x, ULONG y, BYTE clr /*= 0x07*/, ULONG limit /*= 0*/, BYTE space /*= 0x20*/)
{
	USHORT* p = &Root::I->BackBuffer[y * Root::I->WndWidth + x];

	size_t len = ::strlen(psz);

	if (!limit) limit = len;

	for (ULONG i = 0; i < len && i < limit; i++)
	{
		BYTE b = (BYTE)psz[i];
		if (b == 0x20) b = space;
		*p++ = b | (clr << 8);
	}
}

eastl::string Wnd::GetLineToDraw(ULONG index)
{
	return contents[index];
}

eastl::string Wnd::HighlightHexNumber(eastl::string& pattern, LONG index, LONG* numOfItems)
{
	if (numOfItems) *numOfItems = 0;

	eastl::string text = pattern;

	if (!text.size())
		return "";

	for (int i = 0; i < text.size(); i++)
	{
		if (text[i] >= 'a' && text[i] <= 'f')
			text[i] -= 'a' - 'A';

		if (!IsHex(text[i]))
			return "";
	}

	class Match
	{
	public:
		ULONG y, x0, x1;
	};

	eastl::vector<Match> matches;

	USHORT* back = Root::I->BackBuffer;

	for (ULONG y = 0; y < Root::I->WndHeight; y++)
	{
		int pos = 0;

		for (ULONG x = 0; x < Root::I->WndWidth; x++, back++)
		{
			if (y == Root::I->WndHeight - 3) continue; // skip input line.

			auto c = (CHAR)(*back & 0xFF);

			if (!IsHex(c))
				pos = 0;
			else
			{
				if (c == text[pos])
					pos++;
				else
				{
					x -= pos;
					back -= pos;

					pos = 0;
				}

				if (pos == text.size())
				{
					pos = 0;

					ULONG x2;
					USHORT* back2 = back + 1;
					for (x2 = x + 1; x2 < Root::I->WndWidth; x2++, back2++)
					{
						auto c2 = (CHAR)(*back2 & 0xFF);
						if (!IsHex(c2)) break;
					}

					LONG x3;
					USHORT* back3 = back - text.size();
					for (x3 = (LONG)x - text.size(); x3 >= 0; x3--, back3--)
					{
						auto c3 = (CHAR)(*back3 & 0xFF);
						if (!IsHex(c3)) break;
					}

					if (x2 != x + 1 || x3 != (LONG)x - text.size())
						if (x3 + 1 >= 0 && x2 - 1 < Root::I->WndWidth)
						{
							matches.emplace_back(y, x3 + 1, x2 - 1);

							ULONG diff = x2 - 1 - x;

							x += diff;
							back += diff;
						}
				}
			}
		}
	}

	if (!matches.size())
		return "";

	eastl::string retVal;

	for (int i = 0; i < matches.size(); i++)
	{
		auto& m = matches[i];

		BOOLEAN selected = i == index;

		ULONG y = m.y;

		for (ULONG x = m.x0; x <= m.x1; x++)
		{
			USHORT* p = &Root::I->BackBuffer[y * Root::I->WndWidth + x];

			*p = (*p & 0xFF) | (selected ? 0xCF00 : 0x4700);

			if (selected)
				retVal += (CHAR)(*p & 0xFF);
		}
	}

	if (numOfItems) *numOfItems = matches.size();
	
	return "0x" + retVal;
}

VOID Wnd::ChangeCursorX(LONG newX, LONG newY /*= -1*/)
{
	Root::I->FrontBuffer[Root::I->CursorY * Root::I->WndWidth + Root::I->CursorX] = 0; // old pos, cursor must be already visible.

	Root::I->CursorX = newX;
	if (Root::I->CursorDisplayX >= 0) Root::I->CursorDisplayX = Root::I->CursorX;

	if (newY >= 0)
	{
		Root::I->CursorY = newY;
		if (Root::I->CursorDisplayY >= 0) Root::I->CursorDisplayY = Root::I->CursorY;
	}

	Root::I->FrontBuffer[Root::I->CursorY * Root::I->WndWidth + Root::I->CursorX] = 0; // new pos, cursor must be visible.
}

VOID Wnd::SwitchBetweenLogCode()
{
	if (Root::I->CodeLogDivLineY < 0)
	{
		Root::I->CodeLogDivLineY = -Root::I->CodeLogDivLineY;

		DrawAll_Start();
	}

	if (Root::I->CursorFocus == CursorFocus::Log)
	{
		Root::I->LogCursorX = Root::I->CursorX;
		Root::I->LogCursorY = Root::I->CursorY;

		ChangeCursorX(Root::I->CodeCursorX, Root::I->CodeCursorY);

		Root::I->CursorFocus = CursorFocus::Code;
	}
	else if (Root::I->CursorFocus == CursorFocus::Code)
	{
		Root::I->CodeCursorX = Root::I->CursorX;
		Root::I->CodeCursorY = Root::I->CursorY;

		ChangeCursorX(Root::I->LogCursorX, Root::I->LogCursorY);

		Root::I->CursorFocus = CursorFocus::Log;
	}
}

VOID Wnd::SaveOrRestoreFrameBuffer(BOOLEAN direction)
{
	if (!CheckWndWidthHeight(Root::I->WndWidth, Root::I->WndHeight))
		return;

	Root::I->VideoRestoreBufferTimer = 0;

	ULONG width = Root::I->WndWidth * Root::I->GlyphWidth;
	ULONG height = Root::I->WndHeight * Root::I->GlyphHeight;

	if (width * height * (32 / 8) > sizeof(Root::I->VideoRestoreBuffer))
		return;

	if (!direction) // save
	{
		if (Root::I->VideoRestoreBufferState)
			return;
		else
			Root::I->VideoRestoreBufferState = TRUE;
	}
	else // restore
	{
		if (!Root::I->VideoRestoreBufferState)
			return;
		else
			Root::I->VideoRestoreBufferState = FALSE;
	}

	ULONG centerX = (Root::I->fbWidth - width) / 2;
	ULONG centerY = (Root::I->fbHeight - height) / 2;

	ULONG videoX = centerX;
	ULONG videoY = centerY;

	DWORD* dst = (DWORD*)Root::I->VideoRestoreBuffer;

	for (ULONG y = 0; y < height; y++, videoY++)
	{
		BYTE* lineStart = (BYTE*)Root::I->VideoAddr +
			videoY * Root::I->fbStride +
			videoX * (32 / 8);

		DWORD* src = (DWORD*)lineStart;

		if (!direction)
			for (ULONG x = 0; x < width; x++)
				*dst++ = *src++;
		else
			for (ULONG x = 0; x < width; x++)
				*src++ = *dst++;
	}

	if (direction)
		UpdateScreen();
}

BOOLEAN Wnd::CheckWndWidthHeight(ULONG widthChr, ULONG heightChr)
{
	if (Root::I->fbWidth * Root::I->fbHeight * 4 > Root::I->fbMapSize)
		return FALSE;

	ULONG sizeChr = widthChr * heightChr;

	if (sizeChr > Root::I->TextBuffersDim)
		return FALSE;

	ULONG sizePx = sizeChr * Root::I->GlyphWidth * Root::I->GlyphHeight;

	if (sizePx * (32 / 8) > sizeof(Root::I->VideoRestoreBuffer))
		return FALSE;

	if (widthChr * Root::I->GlyphWidth > Root::I->fbWidth)
		return FALSE;
	if (heightChr * Root::I->GlyphHeight > Root::I->fbHeight)
		return FALSE;

	return TRUE;
}

eastl::vector<DivLine> Wnd::GetVisibleDivLines(LONG& first, LONG& last)
{
	eastl::vector<DivLine> v;

	auto getHeight = [&v](LONG y) -> LONG
	{
		LONG h = y - *v.back().py - 1;
		if (h < 0)
			h = 0;
		return h;
	};

	first = 0;
	last = Root::I->WndHeight - 3;

	v.emplace_back(&first, FALSE, 0);

	if (Root::I->RegsDisasmDivLineY >= 0)
		v.emplace_back(&Root::I->RegsDisasmDivLineY, FALSE, getHeight(Root::I->RegsDisasmDivLineY));

	if (Root::I->DisasmCodeDivLineY >= 0)
		v.emplace_back(&Root::I->DisasmCodeDivLineY, TRUE, getHeight(Root::I->DisasmCodeDivLineY));

	if (Root::I->CodeLogDivLineY >= 0)
		v.emplace_back(&Root::I->CodeLogDivLineY, TRUE, getHeight(Root::I->CodeLogDivLineY));

	v.emplace_back(&last, FALSE, getHeight(last));

	return v;
}

VOID Wnd::CheckDivLineYs(LONG* keep /*= NULL*/)
{
	// get the visible "DivLines".

	LONG first = 0;
	LONG last = 0;
	eastl::vector<DivLine> v = GetVisibleDivLines(first, last);

	// make sure that the various "DivLines" are properly arranged.

	for (int i = 0; i < v.size(); i++)
		if (v[i].canBeMoved && *v[i].py - *v[i - 1].py < 4)
			*v[i].py = *v[i - 1].py + 4;

	for (int i = v.size() - 1; i >= 0; i--)
		if (v[i].canBeMoved && *v[i + 1].py - *v[i].py < 4)
			*v[i].py = *v[i + 1].py - 4;

	if (keep)
	{
		int index = 0;

		for (int i = 0; i < v.size(); i++)
			if (v[i].canBeMoved && v[i].py == keep)
			{
				index = i;
				break;
			}

		if (index > 0)
		{
			auto addLine = [&v, &index]() -> BOOLEAN {

				int offset = 0;
				BOOLEAN done = FALSE;

				do
				{
					offset = 0;
					done = FALSE;

					while (v[index + offset].canBeMoved)
					{
						if (*v[index + offset + 1].py - *v[index + offset].py > 4)
						{
							*v[index + offset].py += 1;
							done = TRUE;
							break;
						}
						offset++;
					}
				} while (done && offset != 0);

				if (!done)
				{
					do
					{
						offset = -1;
						done = FALSE;

						while (v[index + offset].canBeMoved)
						{
							if (*v[index + offset].py - *v[index + offset - 1].py > 4)
							{
								*v[index + offset].py -= 1;
								done = TRUE;
								break;
							}
							offset--;
						}
					} while (done && offset != -1);
				}

				return done;
			};

			while (*v[index].py - *v[index - 1].py < v[index].height + 1)
				if (!addLine())
					break;
		}
	}

	// is the layout different?

	BOOLEAN layoutChanged = FALSE;

	if (Root::I->PrevWndWidth != Root::I->WndWidth ||
		Root::I->PrevWndHeight != Root::I->WndHeight ||
		Root::I->PrevLayout.size() != v.size())
	{
		layoutChanged = TRUE;
	}
	else
	{
		for (int i = 0; i < v.size(); i++)
			if (*v[i].py != Root::I->PrevLayout[i]) // WARNING: we assume here that Root::I->PrevLayout.size() == v.size().
			{
				layoutChanged = TRUE;
				break;
			}
	}

	// reset the cursor data, if necessary.

	if (layoutChanged)
	{
		Root::I->CursorDisplayX = -1;
		Root::I->CursorDisplayY = -1;

		Root::I->CursorX = 2;
		Root::I->CursorY = Root::I->WndHeight - 3;

		Root::I->LogCursorX = 2;
		Root::I->LogCursorY = Root::I->WndHeight - 3;

		LONG CodeCursorY = 0;

		for (int i = 0; i < v.size(); i++)
			if (v[i].py == &Root::I->CodeLogDivLineY)
			{
				CodeCursorY = *v[i - 1].py + 1;
				break;
			}

		Root::I->CodeCursorX = 2;
		Root::I->CodeCursorY = CodeCursorY;

		Root::I->LogWindow.GoToEnd_Pending = TRUE;
	}

	// save the current layout.

	Root::I->PrevWndWidth = Root::I->WndWidth;
	Root::I->PrevWndHeight = Root::I->WndHeight;

	Root::I->PrevLayout.clear();
	for (auto& dl : v)
		Root::I->PrevLayout.push_back(*dl.py);
}

VOID Wnd::UpdateScreen()
{
	ULONG width = Root::I->WndWidth * Root::I->GlyphWidth;
	ULONG height = Root::I->WndHeight * Root::I->GlyphHeight;

	ULONG centerX = (Root::I->fbWidth - width) / 2;
	ULONG centerY = (Root::I->fbHeight - height) / 2;

	Glyph::UpdateScreen(centerX, centerY, width, height);
}
