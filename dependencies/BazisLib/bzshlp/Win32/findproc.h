#pragma once
#include "cmndef.h"
#include <tlhelp32.h>

#ifdef UNDER_CE
#pragma comment(lib, "toolhelp")
#else
#define CloseToolhelp32Snapshot CloseHandle
#endif

#pragma region Unsupported template class
//The following template class is NOT processed correctly by VS2008 optimizer.
//That way, it is not supported by now
/*
template <class _Struc,
		  BOOL (WINAPI *_FindFirst)(HANDLE, _Struc *),
		  BOOL (WINAPI *_FindNext)(HANDLE, _Struc *),
		  DWORD dwCreationFlags,
		  bool AllowZeroProcess> 
	class _ToolhelpList
	{
	private:
		unsigned m_ProcessID;
		DWORD m_Flags;

	public:
		_ToolhelpList(unsigned PID = 0, DWORD Flags = dwCreationFlags)
			: m_ProcessID(PID)
			, m_Flags(Flags)
		{
		}

	public:
		class iterator
		{
		private:
			friend class _ToolhelpList;
			HANDLE m_hSnapshot;
			_Struc m_Entry;

		private:
			iterator(_ToolhelpList &list) 
				: m_hSnapshot(CreateToolhelp32Snapshot(list.m_Flags, list.m_ProcessID))
			{
				if (!_FindFirst(m_hSnapshot, &m_Entry))
				{
					CloseToolhelp32Snapshot(m_hSnapshot);
					m_hSnapshot = INVALID_HANDLE_VALUE;
				}
			}

			iterator()
				: m_hSnapshot(INVALID_HANDLE_VALUE)
			{
			}

			void operator=(const iterator &it)
			{
				ASSERT(FALSE);
			}

		public:
			iterator(iterator &it)
				: m_hSnapshot(it)
			{
				it.m_hSnapshot = INVALID_HANDLE_VALUE;
				m_Entry = it.m_Entry;
			}

			~iterator()
			{
				if (m_hSnapshot != INVALID_HANDLE_VALUE)
					CloseToolhelp32Snapshot(m_hSnapshot);
			}

			bool operator!=(const iterator &it)
			{
				return (m_hSnapshot != it.m_hSnapshot);
			}

			const _Struc &operator*()
			{
				return m_Entry;
			}

			iterator &operator++()
			{
				if (!_FindNext(m_hSnapshot, &m_Entry))
				{
					CloseToolhelp32Snapshot(m_hSnapshot);
					m_hSnapshot = INVALID_HANDLE_VALUE;
				}
				return *this;
			}
		};

	public:
		iterator begin()
		{
			return iterator(*this);
		}

		iterator end()
		{
			return iterator();
		}
	};
	*/
#pragma endregion

class ToolhelpSnapshot
{
private:
	HANDLE m_hSnapshot;

public:
	ToolhelpSnapshot(bool SnapProcesses = true, bool SnapThreads = true)
		: m_hSnapshot(CreateToolhelp32Snapshot((SnapProcesses ? TH32CS_SNAPPROCESS : 0) | (SnapThreads ? TH32CS_SNAPTHREAD : 0), 0))
	{
	}

	~ToolhelpSnapshot()
	{
		CloseToolhelp32Snapshot(m_hSnapshot);
	}
	
#pragma region Process enumeration functions
	bool FindFirstProcess(PROCESSENTRY32 *pEntry, LPCTSTR lpExeName = NULL)
	{
		if (!pEntry)
			return false;
		pEntry->dwSize = sizeof(PROCESSENTRY32);
		if (!Process32First(m_hSnapshot, pEntry))
			return false;
		while (lpExeName && _tcsicmp(lpExeName, pEntry->szExeFile))
		{
			if (!Process32Next(m_hSnapshot, pEntry))
				return false;
		}
		return true;
	}

	bool FindNextProcess(PROCESSENTRY32 *pEntry, LPCTSTR lpExeName = NULL)
	{
		pEntry->dwSize = sizeof(PROCESSENTRY32);
		do
		{
			if (!Process32Next(m_hSnapshot, pEntry))
				return false;
		} while (lpExeName && _tcsicmp(lpExeName, pEntry->szExeFile));
		return true;
	}

	unsigned FindFirstProcess(LPCTSTR lpExeName = NULL)
	{
		PROCESSENTRY32 entry;
		if (!FindFirstProcess(&entry, lpExeName))
			return 0;
		return entry.th32ProcessID;
	}

	unsigned FindNextProcess(LPCTSTR lpExeName = NULL)
	{
		PROCESSENTRY32 entry;
		if (!FindNextProcess(&entry, lpExeName))
			return 0;
		return entry.th32ProcessID;
	}
#pragma endregion
};