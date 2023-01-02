//
// some declarations and definitions required to compile QuickJS with Windows XP WDK into BugChecker.
// This file is included by "inttypes.h" in the "EASTL_compat_includes" folder.
//

#ifndef QUICKJSDECLFILL_H
#define QUICKJSDECLFILL_H

//
// IMPORTANT IMPORTANT IMPORTANT ----------------------------------------------------------------------------------
// 
// This definition needs to be commented out in quickjs.h, if upgrading to a newer version of QuickJSPP:
// 
// #define JS_VALUE_GET_PTR(v)  ((void *)((intptr_t)(v) & 0x0000FFFFFFFFFFFFull))
// 
// This is a version that works with 48-bit virtual addresses with the 47th bit set to 1: (NB: a more correct
// version of this code would sign extend the 47th bit, thus being compatible with any type of address.
// However, the whole JS_STRICT_NAN_BOXING breaks in this case: https://en.wikipedia.org/wiki/Intel_5-level_paging)
// 
#ifdef _AMD64_
#define JS_VALUE_GET_PTR(v)  ((void *)(((intptr_t)(v) & 0x0000FFFFFFFFFFFFull) | 0xFFFF000000000000ull))
#endif
// 
// ----------------------------------------------------------------------------------------------------------------
//

#include <sys/stat.h> // we include these files here in order to safely #redefine API names later in this file.
#include <io.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <sys/utime.h>

#ifndef NO_WINDOWS_H
#include <windows.h>
#else
typedef unsigned long DWORD;
typedef int BOOL;
typedef void* LPSYSTEMTIME;
typedef void* LPFILETIME;
typedef void* PCONSOLE_SCREEN_BUFFER_INFO;
typedef void* LPTIME_ZONE_INFORMATION;
#endif

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int_least8_t;
typedef short              int_least16_t;
typedef int                int_least32_t;
typedef long long          int_least64_t;
typedef unsigned char      uint_least8_t;
typedef unsigned short     uint_least16_t;
typedef unsigned int       uint_least32_t;
typedef unsigned long long uint_least64_t;

typedef signed char        int_fast8_t;
typedef int                int_fast16_t;
typedef int                int_fast32_t;
typedef long long          int_fast64_t;
typedef unsigned char      uint_fast8_t;
typedef unsigned int       uint_fast16_t;
typedef unsigned int       uint_fast32_t;
typedef unsigned long long uint_fast64_t;

typedef long long          intmax_t;
typedef unsigned long long uintmax_t;

#define INT8_MIN         (-127i8 - 1)
#define INT16_MIN        (-32767i16 - 1)
#define INT32_MIN        (-2147483647i32 - 1)
#define INT64_MIN        (-9223372036854775807i64 - 1)
#define INT8_MAX         127i8
#define INT16_MAX        32767i16
#define INT32_MAX        2147483647i32
#define INT64_MAX        9223372036854775807i64
#define UINT8_MAX        0xffui8
#define UINT16_MAX       0xffffui16
#define UINT32_MAX       0xffffffffui32
#define UINT64_MAX       0xffffffffffffffffui64

#define INT_LEAST8_MIN   INT8_MIN
#define INT_LEAST16_MIN  INT16_MIN
#define INT_LEAST32_MIN  INT32_MIN
#define INT_LEAST64_MIN  INT64_MIN
#define INT_LEAST8_MAX   INT8_MAX
#define INT_LEAST16_MAX  INT16_MAX
#define INT_LEAST32_MAX  INT32_MAX
#define INT_LEAST64_MAX  INT64_MAX
#define UINT_LEAST8_MAX  UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX

#define INT_FAST8_MIN    INT8_MIN
#define INT_FAST16_MIN   INT32_MIN
#define INT_FAST32_MIN   INT32_MIN
#define INT_FAST64_MIN   INT64_MIN
#define INT_FAST8_MAX    INT8_MAX
#define INT_FAST16_MAX   INT32_MAX
#define INT_FAST32_MAX   INT32_MAX
#define INT_FAST64_MAX   INT64_MAX
#define UINT_FAST8_MAX   UINT8_MAX
#define UINT_FAST16_MAX  UINT32_MAX
#define UINT_FAST32_MAX  UINT32_MAX
#define UINT_FAST64_MAX  UINT64_MAX

#ifdef _WIN64
#define INTPTR_MIN   INT64_MIN
#define INTPTR_MAX   INT64_MAX
#define UINTPTR_MAX  UINT64_MAX
#else
#define INTPTR_MIN   INT32_MIN
#define INTPTR_MAX   INT32_MAX
#define UINTPTR_MAX  UINT32_MAX
#endif

#define INTMAX_MIN       INT64_MIN
#define INTMAX_MAX       INT64_MAX
#define UINTMAX_MAX      UINT64_MAX

#define PTRDIFF_MIN      INTPTR_MIN
#define PTRDIFF_MAX      INTPTR_MAX

#ifndef SIZE_MAX
#ifdef _WIN64
#define SIZE_MAX 0xffffffffffffffffui64
#else
#define SIZE_MAX 0xffffffffui32
#endif
#endif

#define SIG_ATOMIC_MIN   INT32_MIN
#define SIG_ATOMIC_MAX   INT32_MAX

#define WCHAR_MIN        0x0000
#define WCHAR_MAX        0xffff

#define WINT_MIN         0x0000
#define WINT_MAX         0xffff

#define INT8_C(x)    (x)
#define INT16_C(x)   (x)
#define INT32_C(x)   (x)
#define INT64_C(x)   (x ## LL)

#define UINT8_C(x)   (x)
#define UINT16_C(x)  (x)
#define UINT32_C(x)  (x ## U)
#define UINT64_C(x)  (x ## ULL)

#define INTMAX_C(x)  INT64_C(x)
#define UINTMAX_C(x) UINT64_C(x)

struct DIR {
	uint8_t empty;
};
typedef struct DIR DIR;

struct dirent {
	char d_name[261];
};
typedef struct dirent dirent;

#define _S_IFIFO  0x1000 // Pipe

#if !defined(S_IFIFO)
#   define S_IFIFO _S_IFIFO
#endif

#if !defined(S_IFBLK)
#   define S_IFBLK 0
#endif

#define PRId64       "I64d" // "lld" doesn't work with Win XP's kernel printf.

#ifndef _HUGE_ENUF
#define _HUGE_ENUF  1e+300  // _HUGE_ENUF*_HUGE_ENUF must overflow
#endif

#define INFINITY   ((float)(_HUGE_ENUF * _HUGE_ENUF))

#define NAN        ((float)(INFINITY * 0.0F))

#define FE_TONEAREST  _RC_NEAR

#define _RC_NEAR        0x00000000              //     near

#define _In_

#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

double    __stdcall _bc_acosh(_In_ double _X);
double    __stdcall _bc_asinh(_In_ double _X);
double    __stdcall _bc_atanh(_In_ double _X);
double    __stdcall _bc_expm1(_In_ double _X);
double    __stdcall _bc_log1p(_In_ double _X);
double    __stdcall _bc_log2(_In_ double _X);
double    __stdcall _bc_cbrt(_In_ double _X);
double    __stdcall _bc_trunc(_In_ double _X);
double    __stdcall _bc_round(_In_ double _X);
int       __stdcall _bc_isnan(_In_ double _X);
int       __stdcall _bc_isfinite(_In_ double _X);
int       __stdcall _bc_fesetround(_In_ int round);
double    __stdcall _bc_fmin(_In_ double _A, _In_ double _B);
double    __stdcall _bc_fmax(_In_ double _A, _In_ double _B);
int       __stdcall _bc_signbit(_In_ double _X);

#define acosh _bc_acosh
#define asinh _bc_asinh
#define atanh _bc_atanh
#define expm1 _bc_expm1
#define log1p _bc_log1p
#define log2 _bc_log2
#define cbrt _bc_cbrt
#define trunc _bc_trunc
#define round _bc_round
#define isnan _bc_isnan
#define isfinite _bc_isfinite
#define fesetround _bc_fesetround
#define fmin _bc_fmin
#define fmax _bc_fmax
#define signbit _bc_signbit

double __stdcall _bc_fabs(__in double _X);
double __stdcall _bc_sqrt(__in double _X);
double __stdcall _bc_acos(__in double _X);
double __stdcall _bc_asin(__in double _X);
double __stdcall _bc_atan(__in double _X);
double __stdcall _bc_atan2(__in double _Y, __in double _X);
double __stdcall _bc_cos(__in double _X);
double __stdcall _bc_exp(__in double _X);
double __stdcall _bc_log(__in double _X);
double __stdcall _bc_sin(__in double _X);
double __stdcall _bc_tan(__in double _X);
double __stdcall _bc_cosh(__in double _X);
double __stdcall _bc_sinh(__in double _X);
double __stdcall _bc_tanh(__in double _X);
double __stdcall _bc_log10(__in double _X);

#define fabs _bc_fabs
#define sqrt _bc_sqrt
#define acos _bc_acos
#define asin _bc_asin
#define atan _bc_atan
#define atan2 _bc_atan2
#define cos _bc_cos
#define exp _bc_exp
#define log _bc_log
#define sin _bc_sin
#define tan _bc_tan
#define cosh _bc_cosh
#define sinh _bc_sinh
#define tanh _bc_tanh
#define log10 _bc_log10

size_t    __stdcall _bc__msize(void* ptr);

#define _msize _bc__msize

#define assert(x)

struct _bc_stat
{
    _dev_t     st_dev;
    _ino_t     st_ino;
    unsigned short st_mode;
    short      st_nlink;
    short      st_uid;
    short      st_gid;
    _dev_t     st_rdev;
    _off_t     st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
};

int       __stdcall _bc_printf(__in_z __format_string const char* _Format, ...);
int       __stdcall _bc_snprintf(char* s, size_t n, const char* format, ...);
int       __stdcall _bc_stat(const char* restrict path, struct _bc_stat* restrict buf);
int       __stdcall _bc_rmdir(const char* path);
char*     __stdcall _bc_getcwd(char* buffer, size_t size);
int       __stdcall _bc_chdir(const char* path);
int       __stdcall _bc_mkdir(const char* path);
DIR*      __stdcall _bc_opendir(const char* dirname);
struct dirent* __stdcall _bc_readdir(DIR* dirp);
int       __stdcall _bc_closedir(DIR* dirp);

#define printf _bc_printf
#define snprintf _bc_snprintf
#define stat _bc_stat
#define rmdir _bc_rmdir
#define getcwd _bc_getcwd
#define chdir _bc_chdir
#define mkdir _bc_mkdir
#define opendir _bc_opendir
#define readdir _bc_readdir
#define closedir _bc_closedir

void* __stdcall malloc(__in size_t _Size); // we can declare our own malloc/free/realloc since _CRT_ALLOCATION_DEFINED is defined in the project.
void __stdcall free(__inout_opt void* _Memory);
void* __stdcall realloc(__in_opt void* _Memory, __in size_t _NewSize);

int __stdcall _bc_vsnprintf(__out_ecount(_MaxCount) char* _DstBuf, __in size_t _MaxCount, __in_z __format_string const char* _Format, va_list _ArgList);
int __stdcall _bc_snprintf(char* s, size_t n, const char* format, ...);

#define vsnprintf _bc_vsnprintf
#define snprintf _bc_snprintf

void(__stdcall* __stdcall _bc_signal(__in int _SigNum, __in_opt void(__stdcall* _Func)(int)))(int);

#define signal _bc_signal

#undef SIG_DFL
#undef SIG_IGN

#define SIG_DFL ((void (__stdcall *)(int))0)
#define SIG_IGN ((void (__stdcall *)(int))1)

extern FILE _bc__iob[];
extern char** _bc__environ;
extern int* __stdcall _bc__errno(void);

#define _iob _bc__iob
#define _environ _bc__environ
#define _errno _bc__errno

int __stdcall _bc_putchar(__in int _Ch);
void __stdcall _bc_exit(__in int _Code);
char* __stdcall _bc_getenv(__in_z const char* _VarName);
char* __stdcall _bc__fullpath(__out_ecount_z_opt(_SizeInBytes) char* _FullPath, __in_z const char* _Path, __in size_t _SizeInBytes);
int __stdcall _bc__putenv(__in_z const char* _EnvString);
void __stdcall _bc_clearerr(__inout FILE* _File);
int __stdcall _bc_fclose(__inout FILE* _File);
int __stdcall _bc_feof(__in FILE* _File);
int __stdcall _bc_ferror(__in FILE* _File);
int __stdcall _bc_fgetc(__inout FILE* _File);
FILE* __stdcall _bc_fopen(__in_z const char* _Filename, __in_z const char* _Mode);
int __stdcall _bc_fputc(__in int _Ch, __inout FILE* _File);
size_t __stdcall _bc_fread(__out_bcount(_ElementSize* _Count) void* _DstBuf, __in size_t _ElementSize, __in size_t _Count, __inout FILE* _File);
int __stdcall _bc_fseek(__inout FILE* _File, __in long _Offset, __in int _Origin);
long __stdcall _bc_ftell(__inout FILE* _File);
size_t __stdcall _bc_fwrite(__in_ecount(_Size* _Count) const void* _Str, __in size_t _Size, __in size_t _Count, __inout FILE* _File);
int __stdcall _bc__pclose(__inout FILE* _File);
FILE* __stdcall _bc__popen(__in_z const char* _Command, __in_z const char* _Mode);
int __stdcall _bc_rename(__in_z const char* _OldFilename, __in_z const char* _NewFilename);
int __stdcall _bc_unlink(__in_z const char* _Filename);
FILE* __stdcall _bc_tmpfile(void);
FILE* __stdcall _bc_fdopen(__in int _FileHandle, __in_z __format_string const char* _Format);
int __stdcall _bc_fileno(__in FILE* _File);
char* __stdcall _bc_strerror(__in int _param_);
int __stdcall _bc__setmode(__in int _FileHandle, __in int _Mode);
intptr_t __stdcall _bc__get_osfhandle(__in int _FileHandle);
int __stdcall _bc_close(__in int _FileHandle);
int __stdcall _bc_isatty(__in int _FileHandle);
long __stdcall _bc_lseek(__in int _FileHandle, __in long _Offset, __in int _Origin);
int __stdcall _bc_open(__in_z const char* _Filename, __in int _OpenFlag, ...);
int __stdcall _bc_write(__in int _Filehandle, __in_bcount(_MaxCharCount) const void* _Buf, __in unsigned int _MaxCharCount);
int __stdcall _bc__utime(__in_z const char* _Filename, __in_opt struct _utimbuf* _Time);
double __stdcall _bc_hypot(__in double _X, __in double _Y);
double __stdcall _bc_strtod(__in_z const char* _Str, __deref_opt_out_z char** _EndPtr);

#define putchar _bc_putchar
#define exit _bc_exit
#define getenv _bc_getenv
#define _fullpath _bc__fullpath
#define _putenv _bc__putenv
#define clearerr _bc_clearerr
#define fclose _bc_fclose
#define feof _bc_feof
#define ferror _bc_ferror
#define fgetc _bc_fgetc
#define fopen _bc_fopen
#define fputc _bc_fputc
#define fread _bc_fread
#define fseek _bc_fseek
#define ftell _bc_ftell
#define fwrite _bc_fwrite
#define _pclose _bc__pclose
#define _popen _bc__popen
#define rename _bc_rename
#define unlink _bc_unlink
#define tmpfile _bc_tmpfile
#define fdopen _bc_fdopen
#define fileno _bc_fileno
#define strerror _bc_strerror
#define _setmode _bc__setmode
#define _get_osfhandle _bc__get_osfhandle
#define close _bc_close
#define isatty _bc_isatty
#define lseek _bc_lseek
#define open _bc_open
#define write _bc_write
#define _utime _bc__utime
#define hypot _bc_hypot
#define strtod _bc_strtod

VOID __stdcall _bc_GetSystemTime(__out LPSYSTEMTIME lpSystemTime);
BOOL __stdcall _bc_SystemTimeToFileTime(__in const LPSYSTEMTIME lpSystemTime, __out LPFILETIME lpFileTime);
DWORD __stdcall _bc_WaitForSingleObject(__in HANDLE hHandle, __in DWORD dwMilliseconds);
VOID __stdcall _bc_Sleep(__in DWORD dwMilliseconds);
BOOL __stdcall _bc_GetConsoleScreenBufferInfo(__in HANDLE hConsoleOutput, __out PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo);
BOOL __stdcall _bc_SetConsoleMode(__in HANDLE hConsoleHandle, __in DWORD dwMode);
DWORD __stdcall _bc_GetTimeZoneInformation(__out LPTIME_ZONE_INFORMATION lpTimeZoneInformation);

#define GetSystemTime _bc_GetSystemTime
#define SystemTimeToFileTime _bc_SystemTimeToFileTime
#define WaitForSingleObject _bc_WaitForSingleObject
#define Sleep _bc_Sleep
#define GetConsoleScreenBufferInfo _bc_GetConsoleScreenBufferInfo
#define SetConsoleMode _bc_SetConsoleMode
#define GetTimeZoneInformation _bc_GetTimeZoneInformation

int __stdcall _bc_fprintf(__inout FILE* _File, __in_z __format_string const char* _Format, ...);
void __stdcall _bc_abort(void);
int __stdcall _bc_fflush(__inout_opt FILE* _File);
int __stdcall _bc_read(int _FileHandle, __out_bcount(_MaxCharCount) void* _DstBuf, __in unsigned int _MaxCharCount);
void __stdcall _bc__assert(__in_z const char* _Message, __in_z const char* _File, __in unsigned _Line);
long __stdcall _bc_lrint(_In_ double _X);

#define fprintf _bc_fprintf
#define abort _bc_abort
#define fflush _bc_fflush
#define read _bc_read
#define _assert _bc__assert
#define lrint _bc_lrint

#endif
