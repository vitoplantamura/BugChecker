#include "stdafx.h"
#ifdef BZSLIB_POSIX
#include "../status.h"

using namespace BazisLib;

#ifdef _DEBUG
static struct {int code; const TCHAR *pError; const TCHAR *pHint;} s_ErrnoTable[] =
{
	{1, _T("EPERM"), _T("Operation not permitted")},
	{2, _T("ENOENT"), _T("No such file or directory")},
	{3, _T("ESRCH"), _T("No such process")},
	{4, _T("EINTR"), _T("Interrupted system call")},
	{5, _T("EIO"), _T("Input/output error")},
	{6, _T("ENXIO"), _T("Device not configured")},
	{7, _T("E2BIG"), _T("Argument list too long")},
	{8, _T("ENOEXEC"), _T("Exec format error")},
	{9, _T("EBADF"), _T("Bad file descriptor")},
	{10, _T("ECHILD"), _T("No child processes")},
	{11, _T("EDEADLK"), _T("Resource deadlock avoided")},
	{12, _T("ENOMEM"), _T("Cannot allocate memory")},
	{13, _T("EACCES"), _T("Permission denied")},
	{14, _T("EFAULT"), _T("Bad address")},
	{15, _T("ENOTBLK"), _T("Block device required")},
	{16, _T("EBUSY"), _T("Device / Resource busy")},
	{17, _T("EEXIST"), _T("File exists")},
	{18, _T("EXDEV"), _T("Cross-device link")},
	{19, _T("ENODEV"), _T("Operation not supported by device")},
	{20, _T("ENOTDIR"), _T("Not a directory")},
	{21, _T("EISDIR"), _T("Is a directory")},
	{22, _T("EINVAL"), _T("Invalid argument")},
	{23, _T("ENFILE"), _T("Too many open files in system")},
	{24, _T("EMFILE"), _T("Too many open files")},
	{25, _T("ENOTTY"), _T("Inappropriate ioctl for device")},
	{26, _T("ETXTBSY"), _T("Text file busy")},
	{27, _T("EFBIG"), _T("File too large")},
	{28, _T("ENOSPC"), _T("No space left on device")},
	{29, _T("ESPIPE"), _T("Illegal seek")},
	{30, _T("EROFS"), _T("Read-only file system")},
	{31, _T("EMLINK"), _T("Too many links")},
	{32, _T("EPIPE"), _T("Broken pipe")},
	{33, _T("EDOM"), _T("Numerical argument out of domain")},
	{34, _T("ERANGE"), _T("Result too large")},
	{35, _T("EAGAIN"), _T("Resource temporarily unavailable")},
	{36, _T("EINPROGRESS"), _T("Operation now in progress")},
	{37, _T("EALREADY"), _T("Operation already in progress")},
	{38, _T("ENOTSOCK"), _T("Socket operation on non-socket")},
	{39, _T("EDESTADDRREQ"), _T("Destination address required")},
	{40, _T("EMSGSIZE"), _T("Message too long")},
	{41, _T("EPROTOTYPE"), _T("Protocol wrong type for socket")},
	{42, _T("ENOPROTOOPT"), _T("Protocol not available")},
	{43, _T("EPROTONOSUPPORT"), _T("Protocol not supported")},
	{44, _T("ESOCKTNOSUPPORT"), _T("Socket type not supported")},
	{45, _T("ENOTSUP"), _T("Operation not supported")},
	{46, _T("EPFNOSUPPORT"), _T("Protocol family not supported")},
	{47, _T("EAFNOSUPPORT"), _T("Address family not supported by protocol family")},
	{48, _T("EADDRINUSE"), _T("Address already in use")},
	{49, _T("EADDRNOTAVAIL"), _T("Can't assign requested address")},
	{50, _T("ENETDOWN"), _T("Network is down")},
	{51, _T("ENETUNREACH"), _T("Network is unreachable")},
	{52, _T("ENETRESET"), _T("Network dropped connection on reset")},
	{53, _T("ECONNABORTED"), _T("Software caused connection abort")},
	{54, _T("ECONNRESET"), _T("Connection reset by peer")},
	{55, _T("ENOBUFS"), _T("No buffer space available")},
	{56, _T("EISCONN"), _T("Socket is already connected")},
	{57, _T("ENOTCONN"), _T("Socket is not connected")},
	{58, _T("ESHUTDOWN"), _T("Can't send after socket shutdown")},
	{59, _T("ETOOMANYREFS"), _T("Too many references: can't splice")},
	{60, _T("ETIMEDOUT"), _T("Operation timed out")},
	{61, _T("ECONNREFUSED"), _T("Connection refused")},
	{62, _T("ELOOP"), _T("Too many levels of symbolic links")},
	{63, _T("ENAMETOOLONG"), _T("File name too long")},
	{64, _T("EHOSTDOWN"), _T("Host is down")},
	{65, _T("EHOSTUNREACH"), _T("No route to host")},
	{66, _T("ENOTEMPTY"), _T("Directory not empty")},
	{67, _T("EPROCLIM"), _T("Too many processes")},
	{68, _T("EUSERS"), _T("Too many users")},
	{69, _T("EDQUOT"), _T("Disc quota exceeded")},
	{70, _T("ESTALE"), _T("Stale NFS file handle")},
	{71, _T("EREMOTE"), _T("Too many levels of remote in path")},
	{72, _T("EBADRPC"), _T("RPC struct is bad")},
	{73, _T("ERPCMISMATCH"), _T("RPC version wrong")},
	{74, _T("EPROGUNAVAIL"), _T("RPC prog. not avail")},
	{75, _T("EPROGMISMATCH"), _T("Program version wrong")},
	{76, _T("EPROCUNAVAIL"), _T("Bad procedure for program")},
	{77, _T("ENOLCK"), _T("No locks available")},
	{78, _T("ENOSYS"), _T("Function not implemented")},
	{79, _T("EFTYPE"), _T("Inappropriate file type or format")},
	{80, _T("EAUTH"), _T("Authentication error")},
	{81, _T("ENEEDAUTH"), _T("Need authenticator")},
	{82, _T("EPWROFF"), _T("Device power is off")},
	{83, _T("EDEVERR"), _T("Device error, e.g. paper out")},
	{84, _T("EOVERFLOW"), _T("Value too large to be stored in data type")},
	{85, _T("EBADEXEC"), _T("Bad executable")},
	{86, _T("EBADARCH"), _T("Bad CPU type in executable")},
	{87, _T("ESHLIBVERS"), _T("Shared library version mismatch")},
	{88, _T("EBADMACHO"), _T("Malformed Macho file")},
	{89, _T("ECANCELED"), _T("Operation canceled")},
	{90, _T("EIDRM"), _T("Identifier removed")},
	{91, _T("ENOMSG"), _T("No message of desired type")},   
	{92, _T("EILSEQ"), _T("Illegal byte sequence")},
	{93, _T("ENOATTR"), _T("Attribute not found")},
	{94, _T("EBADMSG"), _T("Bad message")},
	{95, _T("EMULTIHOP"), _T("Reserved")},
	{96, _T("ENODATA"), _T("No message available on STREAM")},
	{97, _T("ENOLINK"), _T("Reserved")},
	{98, _T("ENOSR"), _T("No STREAM resources")},
	{99, _T("ENOSTR"), _T("Not a STREAM")},
	{100, _T("EPROTO"), _T("Protocol error")},
	{101, _T("ETIME"), _T("STREAM ioctl timeout")},
	{102, _T("EOPNOTSUPP"), _T("Operation not supported on socket")},
	{103, _T("ENOPOLICY"), _T("No such policy registered")},
	{104, _T("ENOTRECOVERABLE"), _T("State not recoverable")},
	{105, _T("EOWNERDEAD"), _T("Previous owner died")}
};

C_ASSERT(sizeof(s_ErrnoTable) / sizeof(s_ErrnoTable[0]) == 105);

#endif

String ActionStatus::FormatErrorCode(CommonErrorType code)
{
#ifdef _DEBUG
	if (!code)
		return _T("SUCCESSFUL");

	if ((int)code <= __countof(s_ErrnoTable))
		if (s_ErrnoTable[code - 1].code == code)
			return String::sFormat(_T("%s (%s)"), s_ErrnoTable[code - 1].pError, s_ErrnoTable[code - 1].pHint);
#endif
	return String::sFormat(_T("errno=%d"), code);	
}

#endif
