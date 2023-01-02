#pragma once
#include "cmndef.h"

#ifndef UNDER_CE
namespace BazisLib
{
	class WOW64APIProvider
	{
	private:
		typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
		typedef BOOL (WINAPI *LPFN_WOW64DISABLEREDIR) (PVOID *);
		typedef BOOL (WINAPI *LPFN_WOW64REVERTREDIR) (PVOID);
		
		LPFN_ISWOW64PROCESS m_fnIsWow64Process;
		LPFN_WOW64DISABLEREDIR m_fnDisableRedirection;
		LPFN_WOW64REVERTREDIR  m_fnResumeRedirection;

	public:
		WOW64APIProvider();
		~WOW64APIProvider()
		{
		}

		bool IsWow64Process(unsigned PID = 0);
		bool DisableFSRedirection(PVOID *pContext);
		bool ResumeFSRedirection(PVOID pContext);

	public:
		static bool	sIsWow64Process(unsigned PID = 0)
		{
			WOW64APIProvider prov;
			return prov.IsWow64Process(PID);
		}

	};

	class WOW64FSRedirHolder
	{
	private:
		WOW64APIProvider m_Provider;
		PVOID m_pContext;

	public:
		WOW64FSRedirHolder()
		{
			m_Provider.DisableFSRedirection(&m_pContext);
		}

		~WOW64FSRedirHolder()
		{
			m_Provider.ResumeFSRedirection(m_pContext);
		}
	};
}
#endif