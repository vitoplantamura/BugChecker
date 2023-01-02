#pragma once

namespace BazisLib
{
	enum CommonErrorType
	{
			Success			= STATUS_SUCCESS,
			UnknownError		= STATUS_UNSUCCESSFUL,
			InvalidParameter	= STATUS_INVALID_PARAMETER,
			InvalidPointer		= STATUS_ACCESS_VIOLATION,
			InvalidHandle		= STATUS_INVALID_HANDLE,
			FileNotFound		= STATUS_OBJECT_NAME_NOT_FOUND,
			PathNotFound		= STATUS_OBJECT_PATH_NOT_FOUND,
			AccessDenied		= STATUS_ACCESS_DENIED,
			AlreadyExists		= STATUS_OBJECT_NAME_COLLISION,
			NoMemory			= STATUS_NO_MEMORY,
			NotImplemented		= STATUS_NOT_IMPLEMENTED,
			NotSupported		= STATUS_NOT_SUPPORTED,
			NotInitialized		= STATUS_REINITIALIZATION_NEEDED,
			InvalidState		= STATUS_INVALID_DEVICE_STATE,
			Pending			= STATUS_PENDING,
			NoMoreData			= STATUS_NO_MORE_ENTRIES,
			SeekError			= STATUS_UNEXPECTED_IO_ERROR,
		    ReadError			= STATUS_UNEXPECTED_IO_ERROR,
			WriteError			= STATUS_UNEXPECTED_IO_ERROR,
			OperationAborted	= STATUS_ABANDONED,
			TimeoutExpired		= STATUS_TIMEOUT,
			
			ConnectionDropped	= STATUS_CONNECTION_ABORTED,
			CannotConnect		= STATUS_CONNECTION_REFUSED,
			ProtocolError		= STATUS_REMOTE_DISCONNECT,
			RemoteFailure		= STATUS_REMOTE_DISCONNECT,

			ParsingFailed		= STATUS_COULD_NOT_INTERPRET,
			EndOfFile			= STATUS_END_OF_FILE,
		};

	class PlatformStatusPolicy
	{
	public:
		static inline bool IsSuccessfulStatusCode(CommonErrorType code)
		{
			return NT_SUCCESS((NTSTATUS)code);
		}
	};

}
