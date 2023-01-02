#pragma once

namespace BazisLib
{
	template <typename _Ty> const _Ty &Min(const _Ty &a, const _Ty &b)
	{
		if (a < b)
			return a;
		else
			return b;
	}

	template <typename _Ty> const _Ty &Max(const _Ty &a, const _Ty &b)
	{
		if (a > b)
			return a;
		else
			return b;
	}
}