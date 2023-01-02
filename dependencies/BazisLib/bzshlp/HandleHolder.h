#pragma once
#include "cmndef.h"

namespace BazisLib
{
	template <class _HandleType, _HandleType _ErrorValue, typename _CloseHandleFunc, _CloseHandleFunc _CloseHandle> class _HandleHolderT
	{
	protected:
		_HandleType m_Handle;

	private:
		_HandleHolderT &operator=(const _HandleHolderT &newHolder)
		{
			ASSERT(0);
		}

		_HandleHolderT(const _HandleHolderT &holder)
		{
			ASSERT(0)
		}

	public:
		explicit _HandleHolderT(_HandleType hInit = _ErrorValue) : m_Handle(hInit) {}

		_HandleType *operator&()
		{
			ASSERT(m_Handle == _ErrorValue);
			return &m_Handle;
		}

		~_HandleHolderT ()
		{
			if (m_Handle != _ErrorValue)
				_CloseHandle(m_Handle);
		}

		_HandleType Detach()
		{
			_HandleType r = m_Handle;
			m_Handle = _ErrorValue;
			return r;
		}

		_HandleType GetForSingleUse() const
		{
			return m_Handle;
		}

		_HandleHolderT &operator=(_HandleType hNew)
		{
			if (m_Handle == _ErrorValue)
				_CloseHandle(m_Handle);
			m_Handle = hNew;
			return *this;
		}

		bool Valid() const
		{
			return m_Handle != _ErrorValue;
		}

	};

}

#if defined (BZSLIB_WINKERNEL)
#include "WinKernel/HandleHolder.h"
#elif defined (_WIN32)
#include "Win32/HandleHolder.h"
#else
#error Handle holder not available on your platform
#endif
