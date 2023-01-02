#pragma once
#include "cmndef.h"

namespace BazisLib
{
	template <class _T> class FastLocker
	{
	private:
		_T *m_pLockable;

	public:
		FastLocker(_T &pLockable) : m_pLockable(&pLockable) {if (m_pLockable) m_pLockable->Lock();}
		~FastLocker() {if (m_pLockable) m_pLockable->Unlock();}
	};

	template <class _T> class FastUnlocker
	{
	private:
		_T *m_pLockable;

	public:
		FastUnlocker(_T &pLockable) : m_pLockable(&pLockable) {if (m_pLockable) m_pLockable->Unlock();}
		~FastUnlocker() {if (m_pLockable) m_pLockable->Lock();}
	};

	template <class _T> class _ReadLocker
	{
	private:
		_T *m_pLockable;

	public:
		_ReadLocker(_T &pLockable) : m_pLockable(&pLockable) {if (m_pLockable) m_pLockable->LockRead();}
		~_ReadLocker() {if (m_pLockable) m_pLockable->UnlockRead();}
	};

	template <class _T> class _WriteLocker
	{
	private:
		_T *m_pLockable;

	public:
		_WriteLocker(_T &pLockable) : m_pLockable(&pLockable) {if (m_pLockable) m_pLockable->LockWrite();}
		~_WriteLocker() {if (m_pLockable) m_pLockable->UnlockWrite();}
	};

	template <class _T> class _WriteFromReadLocker
	{
	private:
		_T *m_pLockable;

	public:
		_WriteFromReadLocker(_T &pLockable) : m_pLockable(&pLockable) {if (m_pLockable) {m_pLockable->UnlockRead(); m_pLockable->LockWrite();}}
		~_WriteFromReadLocker() {if (m_pLockable) {m_pLockable->UnlockWrite(); m_pLockable->LockRead();}}
	};
}