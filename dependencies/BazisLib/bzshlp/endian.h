#pragma once
#include "cmndef.h"

namespace BazisLib
{
	static inline unsigned short SwapByteOrder16(unsigned short val)
	{
		return ((val & 0xFF00) >> 8) | ((val & 0x00FF) << 8);
	}

	static inline unsigned SwapByteOrder32(unsigned val)
	{
		return ((val & 0xFF000000) >> 24) | ((val & 0x00FF0000) >> 8) | ((val & 0x0000FF00) << 8) | ((val & 0x000000FF) << 24);
	}

	static inline ULONGLONG SwapByteOrder64(ULONGLONG val)
	{
		return (((ULONGLONG)SwapByteOrder32((unsigned)val)) << 32) | SwapByteOrder32((unsigned)(val >> 32));
	}

	static inline short SwapByteOrder(const unsigned short *pVal) {return SwapByteOrder16(*pVal);}
	static inline unsigned SwapByteOrder(const unsigned *pVal) {return SwapByteOrder32(*pVal);}
	static inline unsigned long SwapByteOrder(const unsigned long *pVal) {return SwapByteOrder32(*pVal);}
	static inline ULONGLONG SwapByteOrder(const ULONGLONG *pVal) {return SwapByteOrder64(*pVal);}
}