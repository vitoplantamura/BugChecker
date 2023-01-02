#pragma once

namespace BazisLib
{
	namespace Win32
	{
		template<typename _R, typename _T1, typename _T2> class NewAPICaller2
		{
		private:
			typedef _R (_stdcall *FUNCPTR)(_T1, _T2);

			FUNCPTR m_pFunc;

		public:
			NewAPICaller2(const TCHAR *pDLLName, const char *pFuncName)
				: m_pFunc(NULL)
			{
				HMODULE hDll = GetModuleHandle(pDLLName);
				if (!hDll)
					return;
				m_pFunc = (FUNCPTR)GetProcAddress(hDll, pFuncName);
			}

			bool Valid()
			{
				return (m_pFunc != NULL);
			}

			_R operator()(_T1 p1, _T2 p2)
			{
				//Valid() should be called before
				ASSERT(m_pFunc);
				return m_pFunc(p1, p2);
			}
		};

		template<typename _R, typename _T1, typename _T2, typename _T3, typename _T4> class NewAPICaller4
		{
		private:
			typedef _R (_stdcall *FUNCPTR)(_T1, _T2, _T3, _T4);

			FUNCPTR m_pFunc;

		public:
			NewAPICaller4(const TCHAR *pDLLName, const char *pFuncName)
				: m_pFunc(NULL)
			{
				HMODULE hDll = GetModuleHandle(pDLLName);
				if (!hDll)
					return;
				m_pFunc = (FUNCPTR)GetProcAddress(hDll, pFuncName);
			}

			bool Valid()
			{
				return (m_pFunc != NULL);
			}

			_R operator()(_T1 p1, _T2 p2, _T3 p3, _T4 p4)
			{
				//Valid() should be called before
				ASSERT(m_pFunc);
				return m_pFunc(p1, p2, p3, p4);
			}
		};

	}
}