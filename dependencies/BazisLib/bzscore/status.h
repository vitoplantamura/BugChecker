#pragma once

#include "cmndef.h"
#include <list>
#include <vector>
#include <stdio.h>

#if defined(_DEBUG) && defined(_NDEBUG)
#error Both _DEBUG and _NDEBUG cannot be defined at the same time!
#endif

#if defined(BZSLIB_UNIXWORLD)
#include "Posix/status_defs.h"
#elif defined (BZSLIB_WINKERNEL)
#include "WinKernel/status_defs.h"
#elif defined (_WIN32)
#include "Win32/status_defs.h"
#else
#error Status code mapping is not defined for current platform.
#endif

#include "string.h"
#include "assert.h"

//#define VERBOSE_FUNCTION_DUMPING

namespace BazisLib
{
	static inline class ActionStatus _MAKE_STATUS(CommonErrorType ErrorType, const String::value_type *pszFile, unsigned Line, const String::value_type *pszFunction);
	static inline void _ASSIGN_STATUS(ActionStatus *pStatus, CommonErrorType Error, const String::value_type *pszFile, unsigned Line, const String::value_type *pszFunction);
	static inline class ActionStatus _COPY_STATUS(ActionStatus &Status, const String::value_type *pszFile, unsigned Line, const String::value_type *pszFunction);

	//! Represents an action status (error code & auxiliary info)
	/*! This class is an abstraction for such things as HRESULT in Win32, NTSTATUS in DDK and errno in Unix.
		If a method wants to return the status of its call, it can do it in two ways:
		<ul>
			<li>Specify ActionStatus as its return value type.</li>
			<li>Take an additional argument of type ActionStatus *, defaulted to NULL</li>
		</ul>
		The main feature of ActionStatus is that in the debug build of the library it supports storing full
		information about file names and line numbers that caused the errors. However, in the release build
		the ActionStatus object is represented as a simple DWORD that fits in a single register. That way,
		it produces almost no overhead in the release build (if you use it as a return value, up to 4 additional
		instructions will be required to transfer it via stack due to calling convention, however, in case of inline
		calls, the optimizer will put the status to a register).
		You cannot create initialized ActionStatus objects directly to return a status code. Instead, use the
		ASSIGN_STATUS() and MAKE_STATUS() macros. That macros ensure that the debug version will put current file
		name & line number into the error object. Here is an example: <pre>

ActionStatus Test1()
{
	...
	return MAKE_STATUS(UnknownError);
}

void *Test2(ActionStatus *pStatus = NULL)
{
	if (...)
	{
		ASSIGN_STATUS(pStatus, FileNotFound);
		return NULL;
	}
	ActionStatus status = Test1();
	if (!Test1())
	{
		ASSIGN_STATUS(pStatus, status);
		// --- OR ---
		return NULL;
	}
	ASSIGN_STATUS(pStatus, Success);
	return ...;
}
		</pre>
		As a result, you may simply call the ActionStatus::ProduceDebugDump() method to get a dump like this: <pre>
The parameter is incorrect. in: CreateSomething (at filetest.cpp, line 104)
Invalid pointer in: SomeOtherFunction (at filetest.cpp, line 111)
	</pre>
	Simply include such output in your error messages in the debug version and save plenty of time in finding errors.
	*/
	class ActionStatus
	{
	private:
		friend inline ActionStatus _MAKE_STATUS(CommonErrorType ErrorType, const String::value_type *pszFile, unsigned Line, const String::value_type *pszFunction);
		friend inline void _ASSIGN_STATUS(ActionStatus *pStatus, CommonErrorType Error, const String::value_type *pszFile, unsigned Line, const String::value_type *pszFunction);
		friend inline ActionStatus _COPY_STATUS(ActionStatus &Status, const String::value_type *pszFile, unsigned Line, const String::value_type *pszFunction);

	private:
		CommonErrorType m_Result;

#ifdef _DEBUG
		class LocationRecord
		{
		public:
			String File;
			unsigned Line;
			String Function;

			LocationRecord(const String::value_type *file, unsigned line, const String::value_type *function) :
				File(file),
				Line(line),
				Function(function)
			{
			}
		};

		std::list<LocationRecord> m_ErrorRecords;
#endif

	private:

#if _DEBUG
		void AttachLocation(const String::value_type *file, unsigned line, const String::value_type *Function)
		{
			m_ErrorRecords.push_back(LocationRecord(file, line, Function));
		}
#endif

		ActionStatus(CommonErrorType Result, const String::value_type *file, unsigned line, const String::value_type *Function) :
			m_Result(Result)
		{
#ifdef _DEBUG
			m_ErrorRecords.push_back(LocationRecord(file, line, Function));
#endif
		}

		static String FormatErrorCode(CommonErrorType code);

	public:

		ActionStatus() :
			m_Result(NotInitialized)
		{
		}

		CommonErrorType GetErrorCode()
		{
			return m_Result;
		}

		bool Successful()
		{
			return PlatformStatusPolicy::IsSuccessfulStatusCode(m_Result);
		}

		bool IsCustomError()
		{
#ifdef BZSLIB_CUSTOM_STATUS_CODES_SUPPORTED
			return PlatformStatusPolicy::IsCustomStatusCode(m_Result);
#else
			return false;
#endif
		}

		bool operator==(CommonErrorType error)
		{
			return m_Result == error;
		}

		bool operator!=(CommonErrorType error)
		{
			return m_Result == error;
		}

		String GetErrorText()
		{
			return FormatErrorCode(m_Result);
		}

#ifdef _DEBUG
		String ProduceDebugDump()
		{
			String result = FormatErrorCode(m_Result);
			size_t pos = result.rfind(_T("\r\n"));
			if (pos != String::npos)
				result.replace(pos, 2, _T(""));
			result += _T("\r\n");
			char szNum[32] = {0,};
			String::value_type chr;
			for (std::list<LocationRecord>::iterator it = m_ErrorRecords.begin(); it != m_ErrorRecords.end(); it++)
			{
				result += _T("In ");
				result += it->Function;
				result += _T(" (at ");
				result += it->File;
				result += _T(", line ");
#if defined (_WIN32) && !defined (BZSLIB_WINKERNEL) && !defined(UNDER_CE)
				_snprintf_s(szNum, sizeof(szNum - 1), _TRUNCATE, "%d", it->Line);
#else
#ifdef BZSLIB_UNIXWORLD
				snprintf(szNum, sizeof(szNum - 1), "%d", it->Line);
#else
				_snprintf(szNum, sizeof(szNum - 1), "%d", it->Line);
#endif
#endif
				for (int i = 0; szNum[i]; i++)
				{
					chr = szNum[i];
					result.append(&chr, 1);
				}
				result += _T(")\n");
			}
			return result;
		}
#endif

		String GetMostInformativeText()
		{
#ifdef _DEBUG
			return ProduceDebugDump();
#else
			return GetErrorText();
#endif
		}

#if defined (_WIN32) && !defined(BZSLIB_WINKERNEL)
		HRESULT ConvertToHResult()
		{
			return (HRESULT)m_Result;
		}

		static CommonErrorType FromHResult(HRESULT Result)
		{
			return (CommonErrorType)Result;
		}

		static CommonErrorType FromWin32Error(DWORD Error)
		{
			return (CommonErrorType)HRESULT_FROM_WIN32(Error);
		}

		static CommonErrorType FromLastError()
		{
			return (CommonErrorType)HRESULT_FROM_WIN32(GetLastError());
		}

		static CommonErrorType FromLastError(CommonErrorType CodeOnSuccess)
		{
			DWORD err = GetLastError();
			return err ? ((CommonErrorType)HRESULT_FROM_WIN32(err)) : CodeOnSuccess;
		}

		static CommonErrorType FailFromLastError()
		{
			DWORD err = GetLastError();
			return err ? ((CommonErrorType)HRESULT_FROM_WIN32(err)) : UnknownError;
		}
#endif

#ifdef BZSLIB_POSIX
		static CommonErrorType FromLastError(CommonErrorType defaultError)
		{
			int err = errno;
			if (err == 0)
				return defaultError;
			return (CommonErrorType)errno;
		}
#endif

#ifdef BZSLIB_UNIXWORLD
		static CommonErrorType FromUnixError(int err)
		{
			return (CommonErrorType)err;
		}

		static CommonErrorType FailFromUnixError(int err)
		{
			if (!err)
				return UnknownError;
			return (CommonErrorType)err;
		}
#endif

#ifdef BZSLIB_WINKERNEL
		NTSTATUS ConvertToNTStatus()
		{
			return (NTSTATUS)m_Result;
		}

		static CommonErrorType FromNTStatus(NTSTATUS Status)
		{
			return (CommonErrorType)Status;
		}
#endif

	};

	static inline ActionStatus _MAKE_STATUS(CommonErrorType ErrorType, const String::value_type *pszFile, unsigned Line, const String::value_type *pszFunction)
	{
		return ActionStatus(ErrorType, pszFile, Line, pszFunction);
	}

#ifdef _DEBUG

	static inline void _ASSIGN_STATUS(ActionStatus *pStatus, CommonErrorType Error, const String::value_type *pszFile, unsigned Line, const String::value_type *pszFunction)
	{
		if (pStatus) *pStatus = ActionStatus(Error, pszFile, Line, pszFunction);
	}

	static inline ActionStatus _COPY_STATUS(ActionStatus &Status, const String::value_type *pszFile, unsigned Line, const String::value_type *pszFunction)
	{
		ActionStatus st = Status;
		st.AttachLocation(pszFile, Line, pszFunction);
		return st;
	}

#define COPY_STATUS(status)	_COPY_STATUS(status, _T(__FILE__), __LINE__, _T(__DUMPED_FUNCTION__))

#else

#define COPY_STATUS(status)	(status)

	static inline void _ASSIGN_STATUS(ActionStatus *pStatus, CommonErrorType Error, const String::value_type *pszFile, unsigned Line, const String::value_type *pszFunction)
	{
		if (pStatus) *pStatus = ActionStatus(Error, NULL, 0, NULL);
	}

	static inline void DIRECT_COPY_STATUS(ActionStatus *pStatus, ActionStatus &baseStatus)
	{
		if (pStatus) *pStatus = baseStatus;
	}

#endif

#ifdef VERBOSE_FUNCTION_DUMPING
#define __DUMPED_FUNCTION__ __FUNCSIG__
#else
#define __DUMPED_FUNCTION__ __FUNCTION__
#endif

//! Sets pStatus value to the requested value. Always replaces the previous one.
#define ASSIGN_STATUS(pointer, value) BazisLib::_ASSIGN_STATUS(pointer, value, _T(__FILE__), __LINE__, _T(__DUMPED_FUNCTION__))

//! Copies the source status to destination status and adds a detailed status code.
#define MAKE_STATUS(value)	BazisLib::_MAKE_STATUS(value, _T(__FILE__), __LINE__, _T(__DUMPED_FUNCTION__))

#ifdef BZSLIB_CUSTOM_STATUS_CODES_SUPPORTED

	__declspec(selectany) std::vector<class CustomStatus *> _CustomStatusObjectsInternal;

	class CustomStatus
	{
	private:
		const char *_StatusName;
		const TCHAR *_StatusText;

		CommonErrorType _ErrorType;

	public:
		operator CommonErrorType() {return _ErrorType;}

		CustomStatus(const char *pName, const TCHAR *pMessage)
			: _StatusName(pName)
			, _StatusText(pMessage)
		{
			size_t idx = _CustomStatusObjectsInternal.size();
			_ErrorType = PlatformStatusPolicy::CustomErrorFromIndex((int)idx);
			_CustomStatusObjectsInternal.push_back(this);
		}

		const TCHAR *GetStatusText() { return _StatusText; }
	};
#define DECLARE_CUSTOM_STATUS(name, text) __declspec(selectany) CustomStatus name(#name, text)
#endif

}
