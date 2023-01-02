#include "Ps2Keyb.h"

#ifdef _AMD64_
extern "C" BYTE Amd64_ReadKeyb();
#endif

const CHAR* Ps2Keyb::Layout = "EN";

BYTE Ps2Keyb::Read()
{
#ifndef _AMD64_
	BYTE	bKeybPortByte;

	__asm
	{
		push	eax
		pushf

		in		al, 0x64
		test	al, 1
		jz		_ReturnZero

		mov		ah, al
		in		al, 0x60

		test	ah, 0x20	// (100000b) is "second PS/2 port output buffer full" ? Ref: https://wiki.osdev.org/%228042%22_PS/2_Controller
		jnz		_ReturnZero

		jmp		_Exit

		_ReturnZero :

		mov		al, 0

			_Exit :

			mov		bKeybPortByte, al

			popf
			pop		eax
	}

	return bKeybPortByte;
#else
	return Amd64_ReadKeyb();
#endif
}

static WORD g_vwScancodeToAsciiMap_EN[] = // === NumLock NOT pressed. === // from the first BugChecker.
{
	(MACRO_SCANCODE_1 << 8) | '1',
	(MACRO_SCANCODE_2 << 8) | '2',
	(MACRO_SCANCODE_3 << 8) | '3',
	(MACRO_SCANCODE_4 << 8) | '4',
	(MACRO_SCANCODE_5 << 8) | '5',
	(MACRO_SCANCODE_6 << 8) | '6',
	(MACRO_SCANCODE_7 << 8) | '7',
	(MACRO_SCANCODE_8 << 8) | '8',
	(MACRO_SCANCODE_9 << 8) | '9',
	(MACRO_SCANCODE_0 << 8) | '0',
	(MACRO_SCANCODE_Minus << 8) | '-',
	(MACRO_SCANCODE_Equal << 8) | '=',
	(MACRO_SCANCODE_Q << 8) | 'q',
	(MACRO_SCANCODE_W << 8) | 'w',
	(MACRO_SCANCODE_E << 8) | 'e',
	(MACRO_SCANCODE_R << 8) | 'r',
	(MACRO_SCANCODE_T << 8) | 't',
	(MACRO_SCANCODE_Y << 8) | 'y',
	(MACRO_SCANCODE_U << 8) | 'u',
	(MACRO_SCANCODE_I << 8) | 'i',
	(MACRO_SCANCODE_O << 8) | 'o',
	(MACRO_SCANCODE_P << 8) | 'p',
	(MACRO_SCANCODE_OpBrac << 8) | '[',
	(MACRO_SCANCODE_ClBrac << 8) | ']',
	(MACRO_SCANCODE_A << 8) | 'a',
	(MACRO_SCANCODE_S << 8) | 's',
	(MACRO_SCANCODE_D << 8) | 'd',
	(MACRO_SCANCODE_F << 8) | 'f',
	(MACRO_SCANCODE_G << 8) | 'g',
	(MACRO_SCANCODE_H << 8) | 'h',
	(MACRO_SCANCODE_J << 8) | 'j',
	(MACRO_SCANCODE_K << 8) | 'k',
	(MACRO_SCANCODE_L << 8) | 'l',
	(MACRO_SCANCODE_SemiCl << 8) | ';',
	(MACRO_SCANCODE_Acc0 << 8) | '\'',
	(MACRO_SCANCODE_Acc1 << 8) | '`',
	(MACRO_SCANCODE_BSlash << 8) | '\\',
	(MACRO_SCANCODE_Z << 8) | 'z',
	(MACRO_SCANCODE_X << 8) | 'x',
	(MACRO_SCANCODE_C << 8) | 'c',
	(MACRO_SCANCODE_V << 8) | 'v',
	(MACRO_SCANCODE_B << 8) | 'b',
	(MACRO_SCANCODE_N << 8) | 'n',
	(MACRO_SCANCODE_M << 8) | 'm',
	(MACRO_SCANCODE_Comma << 8) | ',',
	(MACRO_SCANCODE_Dot << 8) | '.',
	(MACRO_SCANCODE_Slash << 8) | '/',
	(MACRO_SCANCODE_AstrP << 8) | '*',
	(MACRO_SCANCODE_Space << 8) | ' ',
	(MACRO_SCANCODE_MinusP << 8) | '-',
	(MACRO_SCANCODE_PlusP << 8) | '+',

	0x0000 // END
};

static WORD g_vwScancodeToAsciiMap_ShiftPressed_EN[] = // from the first BugChecker.
{
	(MACRO_SCANCODE_1 << 8) | '!',
	(MACRO_SCANCODE_2 << 8) | '@',
	(MACRO_SCANCODE_3 << 8) | '#',
	(MACRO_SCANCODE_4 << 8) | '$',
	(MACRO_SCANCODE_5 << 8) | '%',
	(MACRO_SCANCODE_6 << 8) | '^',
	(MACRO_SCANCODE_7 << 8) | '&',
	(MACRO_SCANCODE_8 << 8) | '*',
	(MACRO_SCANCODE_9 << 8) | '(',
	(MACRO_SCANCODE_0 << 8) | ')',
	(MACRO_SCANCODE_Minus << 8) | '_',
	(MACRO_SCANCODE_Equal << 8) | '+',
	(MACRO_SCANCODE_Q << 8) | 'Q',
	(MACRO_SCANCODE_W << 8) | 'W',
	(MACRO_SCANCODE_E << 8) | 'E',
	(MACRO_SCANCODE_R << 8) | 'R',
	(MACRO_SCANCODE_T << 8) | 'T',
	(MACRO_SCANCODE_Y << 8) | 'Y',
	(MACRO_SCANCODE_U << 8) | 'U',
	(MACRO_SCANCODE_I << 8) | 'I',
	(MACRO_SCANCODE_O << 8) | 'O',
	(MACRO_SCANCODE_P << 8) | 'P',
	(MACRO_SCANCODE_OpBrac << 8) | '{',
	(MACRO_SCANCODE_ClBrac << 8) | '}',
	(MACRO_SCANCODE_A << 8) | 'A',
	(MACRO_SCANCODE_S << 8) | 'S',
	(MACRO_SCANCODE_D << 8) | 'D',
	(MACRO_SCANCODE_F << 8) | 'F',
	(MACRO_SCANCODE_G << 8) | 'G',
	(MACRO_SCANCODE_H << 8) | 'H',
	(MACRO_SCANCODE_J << 8) | 'J',
	(MACRO_SCANCODE_K << 8) | 'K',
	(MACRO_SCANCODE_L << 8) | 'L',
	(MACRO_SCANCODE_SemiCl << 8) | ':',
	(MACRO_SCANCODE_Acc0 << 8) | '"',
	(MACRO_SCANCODE_BSlash << 8) | '|',
	(MACRO_SCANCODE_Z << 8) | 'Z',
	(MACRO_SCANCODE_X << 8) | 'X',
	(MACRO_SCANCODE_C << 8) | 'C',
	(MACRO_SCANCODE_V << 8) | 'V',
	(MACRO_SCANCODE_B << 8) | 'B',
	(MACRO_SCANCODE_N << 8) | 'N',
	(MACRO_SCANCODE_M << 8) | 'M',
	(MACRO_SCANCODE_Comma << 8) | '<',
	(MACRO_SCANCODE_Dot << 8) | '>',
	(MACRO_SCANCODE_Slash << 8) | '?',

	0x0000 // END
};

static WORD g_vwScancodeToAsciiMap_AltPressed_EN[] =
{
	0x0000 // END
};

static WORD g_vwScancodeToAsciiMap_ShiftAltPressed_EN[] =
{
	0x0000 // END
};

static WORD g_vwScancodeToAsciiMap_IT[] = // === NumLock NOT pressed. ===
{
	(MACRO_SCANCODE_1 << 8) | '1',
	(MACRO_SCANCODE_2 << 8) | '2',
	(MACRO_SCANCODE_3 << 8) | '3',
	(MACRO_SCANCODE_4 << 8) | '4',
	(MACRO_SCANCODE_5 << 8) | '5',
	(MACRO_SCANCODE_6 << 8) | '6',
	(MACRO_SCANCODE_7 << 8) | '7',
	(MACRO_SCANCODE_8 << 8) | '8',
	(MACRO_SCANCODE_9 << 8) | '9',
	(MACRO_SCANCODE_0 << 8) | '0',
	(MACRO_SCANCODE_Minus << 8) | '\'',
	//(MACRO_SCANCODE_Equal << 8) | 'ì',
	(MACRO_SCANCODE_Q << 8) | 'q',
	(MACRO_SCANCODE_W << 8) | 'w',
	(MACRO_SCANCODE_E << 8) | 'e',
	(MACRO_SCANCODE_R << 8) | 'r',
	(MACRO_SCANCODE_T << 8) | 't',
	(MACRO_SCANCODE_Y << 8) | 'y',
	(MACRO_SCANCODE_U << 8) | 'u',
	(MACRO_SCANCODE_I << 8) | 'i',
	(MACRO_SCANCODE_O << 8) | 'o',
	(MACRO_SCANCODE_P << 8) | 'p',
	//(MACRO_SCANCODE_OpBrac << 8) | 'è',
	(MACRO_SCANCODE_ClBrac << 8) | '+',
	(MACRO_SCANCODE_A << 8) | 'a',
	(MACRO_SCANCODE_S << 8) | 's',
	(MACRO_SCANCODE_D << 8) | 'd',
	(MACRO_SCANCODE_F << 8) | 'f',
	(MACRO_SCANCODE_G << 8) | 'g',
	(MACRO_SCANCODE_H << 8) | 'h',
	(MACRO_SCANCODE_J << 8) | 'j',
	(MACRO_SCANCODE_K << 8) | 'k',
	(MACRO_SCANCODE_L << 8) | 'l',
	//(MACRO_SCANCODE_SemiCl << 8) | 'ò',
	//(MACRO_SCANCODE_Acc0 << 8) | 'à',
	//(MACRO_SCANCODE_BSlash << 8) | 'ù',
	(MACRO_SCANCODE_Acc1 << 8) | '\\',
	(0x56 << 8) | '<',
	(MACRO_SCANCODE_Z << 8) | 'z',
	(MACRO_SCANCODE_X << 8) | 'x',
	(MACRO_SCANCODE_C << 8) | 'c',
	(MACRO_SCANCODE_V << 8) | 'v',
	(MACRO_SCANCODE_B << 8) | 'b',
	(MACRO_SCANCODE_N << 8) | 'n',
	(MACRO_SCANCODE_M << 8) | 'm',
	(MACRO_SCANCODE_Comma << 8) | ',',
	(MACRO_SCANCODE_Dot << 8) | '.',
	(MACRO_SCANCODE_Slash << 8) | '-',
	(MACRO_SCANCODE_AstrP << 8) | '*',
	(MACRO_SCANCODE_Space << 8) | ' ',
	(MACRO_SCANCODE_MinusP << 8) | '-',
	(MACRO_SCANCODE_PlusP << 8) | '+',

	0x0000 // END
};

static WORD g_vwScancodeToAsciiMap_ShiftPressed_IT[] =
{
	(MACRO_SCANCODE_1 << 8) | '!',
	(MACRO_SCANCODE_2 << 8) | '"',
	//(MACRO_SCANCODE_3 << 8) | '£',
	(MACRO_SCANCODE_4 << 8) | '$',
	(MACRO_SCANCODE_5 << 8) | '%',
	(MACRO_SCANCODE_6 << 8) | '&',
	(MACRO_SCANCODE_7 << 8) | '/',
	(MACRO_SCANCODE_8 << 8) | '(',
	(MACRO_SCANCODE_9 << 8) | ')',
	(MACRO_SCANCODE_0 << 8) | '=',
	(MACRO_SCANCODE_Minus << 8) | '?',
	(MACRO_SCANCODE_Equal << 8) | '^',
	(MACRO_SCANCODE_Q << 8) | 'Q',
	(MACRO_SCANCODE_W << 8) | 'W',
	(MACRO_SCANCODE_E << 8) | 'E',
	(MACRO_SCANCODE_R << 8) | 'R',
	(MACRO_SCANCODE_T << 8) | 'T',
	(MACRO_SCANCODE_Y << 8) | 'Y',
	(MACRO_SCANCODE_U << 8) | 'U',
	(MACRO_SCANCODE_I << 8) | 'I',
	(MACRO_SCANCODE_O << 8) | 'O',
	(MACRO_SCANCODE_P << 8) | 'P',
	//(MACRO_SCANCODE_OpBrac << 8) | 'é',
	(MACRO_SCANCODE_ClBrac << 8) | '*',
	(MACRO_SCANCODE_A << 8) | 'A',
	(MACRO_SCANCODE_S << 8) | 'S',
	(MACRO_SCANCODE_D << 8) | 'D',
	(MACRO_SCANCODE_F << 8) | 'F',
	(MACRO_SCANCODE_G << 8) | 'G',
	(MACRO_SCANCODE_H << 8) | 'H',
	(MACRO_SCANCODE_J << 8) | 'J',
	(MACRO_SCANCODE_K << 8) | 'K',
	(MACRO_SCANCODE_L << 8) | 'L',
	//(MACRO_SCANCODE_SemiCl << 8) | 'ç',
	//(MACRO_SCANCODE_Acc0 << 8) | '°',
	//(MACRO_SCANCODE_BSlash << 8) | '§',
	(MACRO_SCANCODE_Acc1 << 8) | '|',
	(0x56 << 8) | '>',
	(MACRO_SCANCODE_Z << 8) | 'Z',
	(MACRO_SCANCODE_X << 8) | 'X',
	(MACRO_SCANCODE_C << 8) | 'C',
	(MACRO_SCANCODE_V << 8) | 'V',
	(MACRO_SCANCODE_B << 8) | 'B',
	(MACRO_SCANCODE_N << 8) | 'N',
	(MACRO_SCANCODE_M << 8) | 'M',
	(MACRO_SCANCODE_Comma << 8) | ';',
	(MACRO_SCANCODE_Dot << 8) | ':',
	(MACRO_SCANCODE_Slash << 8) | '_',

	0x0000 // END
};

static WORD g_vwScancodeToAsciiMap_AltPressed_IT[] =
{
	(MACRO_SCANCODE_OpBrac << 8) | '[',
	(MACRO_SCANCODE_ClBrac << 8) | ']',
	(MACRO_SCANCODE_SemiCl << 8) | '@',
	(MACRO_SCANCODE_Acc0 << 8) | '#',

	0x0000 // END
};

static WORD g_vwScancodeToAsciiMap_ShiftAltPressed_IT[] =
{
	(MACRO_SCANCODE_OpBrac << 8) | '{',
	(MACRO_SCANCODE_ClBrac << 8) | '}',

	0x0000 // END
};

USHORT Ps2Keyb::ReadAscii(BOOLEAN& shiftState, BOOLEAN& ctrlState, BOOLEAN& altState)
{
	auto s = Read();

	if (s == MACRO_SCANCODE_LShift || s == MACRO_SCANCODE_RShift)
	{
		shiftState = TRUE;
		return 0;
	}

	if (s == (MACRO_SCANCODE_LShift | 0x80) || s == (MACRO_SCANCODE_RShift | 0x80))
	{
		shiftState = FALSE;
		return 0;
	}

	if (s == MACRO_SCANCODE_CTRL)
	{
		ctrlState = TRUE;
		return 0;
	}

	if (s == (MACRO_SCANCODE_CTRL | 0x80))
	{
		ctrlState = FALSE;
		return 0;
	}

	if (s == MACRO_SCANCODE_Alt)
	{
		altState = TRUE;
		return 0;
	}

	if (s == (MACRO_SCANCODE_Alt | 0x80))
	{
		altState = FALSE;
		return 0;
	}

	if (FALSE && s == 0xE0) // disabled.
	{
		s = Read();
		if (s == 0 || s >= 0x80)
			return 0;
		else
			s |= 0x80;
	}
	else if (s == 0 || s >= 0x80)
		return 0;

	WORD* map = g_vwScancodeToAsciiMap_EN;
	WORD* mapShift = g_vwScancodeToAsciiMap_ShiftPressed_EN;
	WORD* mapAlt = g_vwScancodeToAsciiMap_AltPressed_EN;
	WORD* mapShiftAlt = g_vwScancodeToAsciiMap_ShiftAltPressed_EN;

	if (Layout)
	{
		if (!::strcmp(Layout, "IT"))
		{
			map = g_vwScancodeToAsciiMap_IT;
			mapShift = g_vwScancodeToAsciiMap_ShiftPressed_IT;
			mapAlt = g_vwScancodeToAsciiMap_AltPressed_IT;
			mapShiftAlt = g_vwScancodeToAsciiMap_ShiftAltPressed_IT;
		}
	}

	WORD* p = map;

	if (shiftState)
	{
		if (altState)
			p = mapShiftAlt;
		else
			p = mapShift;
	}
	else if (altState)
	{
		p = mapAlt;
	}

	while (*p)
		if ((*p >> 8) == s)
			return *p;
		else
			p++;

	return ((s & 0xFF) << 8) | MACRO_ASCIICONVERSION_RETVAL_CHECK_SCANCODE;
}
