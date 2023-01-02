#pragma once
namespace BazisLib
{
	namespace Algorithms
	{
		template <class _Structure> void ZeroStruct(_Structure &object)
		{
			memset(&object, 0, sizeof(_Structure));
		}

		template <size_t _ArraySize> void CopyArray(char (&dest)[_ArraySize], const char (&source)[_ArraySize])
		{
			memcpy(dest, source, _ArraySize);
		}
	}
}