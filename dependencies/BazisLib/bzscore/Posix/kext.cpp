#include "stdafx.h"
#ifdef BZSLIB_KEXT

#include <string>	//Includes STLPort headers containing malloc() definition

__private_extern__ void * _realloc(void * _Memory, size_t _Size, size_t _OldSize)
{
	if (!_Memory)
		return malloc(_Size);
	if (!_Size)
	{
		free(_Memory);
		return NULL;
	}
	void *pMem = malloc(_Size);
	if (!pMem)
	{
		free(_Memory);
		return NULL;
	}
	size_t todo =  _Size;
	if (_OldSize < todo)
		todo = _OldSize;

	memcpy(pMem, _Memory, todo);
	free(_Memory);
	return pMem;
}

/*

static wchar_t wtolower(wchar_t ch)
{
if (ch >= 'A' && ch <= 'Z')
ch |= 0x20;

return ch;
}

__private_extern__ int wcscasecmp(const wchar_t *s1, const wchar_t *s2)
{
	for (int i = 0; ; i++)
	{
		wchar_t c1 = wtolower(s1[i]);
		wchar_t c2 = wtolower(s2[i]);
		if (!c1 || (c1 != c2))
			return c1 - c2;
	}
}
*/

#include "../assert.h"

#endif