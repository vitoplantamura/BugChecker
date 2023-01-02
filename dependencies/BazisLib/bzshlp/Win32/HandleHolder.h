#pragma once

namespace BazisLib
{
	namespace Win32
	{
		class HandleHolder : public _HandleHolderT<HANDLE, INVALID_HANDLE_VALUE, BOOL (_stdcall *)(HANDLE), &CloseHandle>
		{
		public:	
			explicit HandleHolder(HANDLE hInit = INVALID_HANDLE_VALUE) : _HandleHolderT(hInit) {}

			HANDLE Duplicate(HANDLE hDestProcess = GetCurrentProcess(),
				bool bInheritable = true,
				DWORD options = DUPLICATE_SAME_ACCESS,
				DWORD newAccess = 0)
			{
				HANDLE hRes = INVALID_HANDLE_VALUE;
				DuplicateHandle(GetCurrentProcess(), m_Handle, hDestProcess, &hRes, newAccess, bInheritable, options);
				return hRes;
			}

			_HandleHolderT &operator=(HANDLE hNew) { return __super::operator=(hNew); }

		};
	}


}