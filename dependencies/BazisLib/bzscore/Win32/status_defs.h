#pragma once

#ifndef _WINDOWS_
#error Please include windows.h first
#endif

#define BZSLIB_CUSTOM_STATUS_CODES_SUPPORTED

namespace BazisLib
{
	enum CommonErrorType
	{
			Success				= S_OK,
			UnknownError		= __HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR),
			InvalidParameter	= __HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER),
			InvalidPointer		= E_POINTER,
			InvalidHandle		= __HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE),
			FileNotFound		= __HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
			PathNotFound		= __HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND),
			AccessDenied		= __HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED),
			AlreadyExists		= __HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS),
			NoMemory			= __HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY),
			NotImplemented		= __HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED),
			NotSupported		= __HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED),
			NotInitialized		= __HRESULT_FROM_WIN32(ERROR_INVALID_STATE),
			InvalidState		= __HRESULT_FROM_WIN32(ERROR_INVALID_STATE),
			Pending				= __HRESULT_FROM_WIN32(ERROR_IO_PENDING),
			NoMoreData			= __HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS),
			SeekError			= __HRESULT_FROM_WIN32(ERROR_SEEK),
			ReadError			= __HRESULT_FROM_WIN32(ERROR_READ_FAULT),
			WriteError			= __HRESULT_FROM_WIN32(ERROR_WRITE_FAULT),
			OperationAborted	= __HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED),
			TimeoutExpired		= __HRESULT_FROM_WIN32(ERROR_TIMEOUT),

			ConnectionDropped  = __HRESULT_FROM_WIN32(ERROR_CONNECTION_ABORTED),
			CannotConnect		= __HRESULT_FROM_WIN32(ERROR_CONNECTION_REFUSED),
			ProtocolError		= RPC_E_INVALID_DATAPACKET,
			RemoteFailure		= RPC_E_CALL_REJECTED,

			ParsingFailed		= __HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION),
			EndOfFile			= __HRESULT_FROM_WIN32(ERROR_HANDLE_EOF),
	};

	class PlatformStatusPolicy
	{
	public:
		static inline bool IsSuccessfulStatusCode(CommonErrorType code)
		{
			return SUCCEEDED((HRESULT)code);
		}

		static inline bool IsCustomStatusCode(CommonErrorType code) {return (HRESULT_FACILITY(code) == FACILITY_ITF);}
		static inline CommonErrorType CustomErrorFromIndex(int idx)
		{
			return (CommonErrorType)((HRESULT) (((idx) & 0x0000FFFF) | (FACILITY_ITF << 16) | 0x80000000));
		}

		static inline int CustomErrorIndexFromErrorCode(CommonErrorType code)
		{
			return HRESULT_CODE(code);
		}
	};


}
