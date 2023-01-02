#pragma once

#ifdef _DEBUG

namespace BazisLib
{
	namespace DDK
	{
		const wchar_t *MapNTStatus(NTSTATUS Status);
	}
};
#endif