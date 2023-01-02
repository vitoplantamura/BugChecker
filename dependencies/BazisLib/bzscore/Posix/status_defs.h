#pragma once
#ifdef BZS_KEXT
#include <sys/errno.h>
#else
#include <errno.h>
#endif

namespace BazisLib
{
	enum CommonErrorType
	{
		Success				= 0,
		UnknownError		= ENODEV,
		InvalidParameter	= EINVAL,
		InvalidPointer		= EFAULT,
		InvalidHandle		= EBADF,
		FileNotFound		= ENOENT,
		PathNotFound		= ENOENT,
		AccessDenied		= EACCES,
		AlreadyExists		= EEXIST,
		NoMemory			= ENOMEM,
		NotImplemented		= ENOSYS,
		NotSupported		= EPERM,
		NotInitialized		= ENXIO,
		InvalidState		= ENXIO,
// 		Pending				= __HRESULT_FROM_WIN32(ERROR_IO_PENDING),
// 		NoMoreData			= __HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS),
		SeekError			= EIO,
		ReadError			= EIO,
		WriteError			= EIO,
		OperationAborted	= ECANCELED,
		TimeoutExpired		= ETIME,

// 		ConnectionDropped  = __HRESULT_FROM_WIN32(ERROR_CONNECTION_ABORTED),
// 		CannotConnect		= __HRESULT_FROM_WIN32(ERROR_CONNECTION_REFUSED),
//		ProtocolError		= RPC_E_INVALID_DATAPACKET,
//		RemoteFailure		= RPC_E_CALL_REJECTED,

//		ParsingFailed		= __HRESULT_FROM_WIN32(ERROR_INTERNAL_DB_CORRUPTION),
//		EndOfFile			= __HRESULT_FROM_WIN32(ERROR_HANDLE_EOF),
	};

	class PlatformStatusPolicy
	{
	public:
		static inline bool IsSuccessfulStatusCode(CommonErrorType code)
		{
			return ((int)code) == 0;
		}
	};


}
