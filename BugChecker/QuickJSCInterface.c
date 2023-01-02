#define NO_WINDOWS_H

#include <ntddk.h>
#include <stdio.h>

#include <stdarg.h>

#include "CrtFill.h"

#include "QuickJS\quickjs-libc.h"

#include "QuickJSCInterface.h"
#include "QuickJSCppInterface.h"

static CHAR QJSNotImplLog[256] = "";

// --------- QJS_Eval function definition. ---------

static JSRuntime* rt = NULL;
static JSContext* ctx = NULL;

static LONG QJS_InitState = 0;

static const char* TagToStr(int64_t tag)
{
	switch (tag)
	{
	case JS_TAG_INT:               return "int";
	case JS_TAG_BOOL:              return "bool";
	case JS_TAG_NULL:              return "null";
	case JS_TAG_UNDEFINED:         return "undefined";
	case JS_TAG_CATCH_OFFSET:      return "catch offset";
	case JS_TAG_EXCEPTION:         return "exception";
	case JS_TAG_FLOAT64:           return "float64";
	case JS_TAG_MODULE:            return "module";
	case JS_TAG_OBJECT:            return "object";
	case JS_TAG_STRING:            return "string";
	case JS_TAG_BIG_INT:           return "big int";
	case JS_TAG_BIG_FLOAT:         return "big float";
	case JS_TAG_SYMBOL:            return "symbol";
	case JS_TAG_FUNCTION_BYTECODE: return "function bytecode";
	default:                       return "unknown";
	}
}

void QJS_SetGlobalStringProp(const char* name, const char* value)
{
	JSValue global_obj;

	global_obj = JS_GetGlobalObject(ctx);

	JS_SetPropertyStr(ctx, global_obj, name, JS_NewString(ctx, value));

	JS_FreeValue(ctx, global_obj);
}

int QJS_Eval(const char* src, char* asyncArgs, int asyncArgsMaxSize, unsigned __int64* retValU64, BOOL quiet)
{
	int retVal;
	BOOL readRet;
	JSValue vexp;
	BOOL quietBcAsync = FALSE;

	retVal = 0;
	*retValU64 = 0;

	// initialize QuickJS.

	if (!QJS_InitState)
	{
		QJS_InitState = 1;

		rt = JS_NewRuntime();
		if (rt)
		{
			ctx = JS_NewContext(rt);
			if (ctx)
			{
				js_std_add_helpers(ctx, 0, NULL);

				JS_AddIntrinsicBigFloat(ctx);
				JS_AddIntrinsicBigDecimal(ctx);
				JS_AddIntrinsicOperators(ctx);
				JS_EnableBignumExt(ctx, TRUE);

				JS_FreeValue(ctx, JS_Eval(ctx, QJSStartupScript, strlen(QJSStartupScript), "", JS_EVAL_TYPE_GLOBAL));

				QJS_InitState = 2;
			}
		}
	}

	if (QJS_InitState != 2)
		return QJS_EVAL_ERROR_INIT;

	// evaluate the expression.

	readRet = TRUE;

	vexp = JS_Eval(ctx, src, strlen(src), "", JS_EVAL_TYPE_GLOBAL);

	_bc_resetstdout();

	if (JS_IsException(vexp))
	{
		js_std_dump_error(ctx);
		readRet = FALSE;
	}

	JS_FreeValue(ctx, vexp);

	// execute async operations in QJS (like Promises).

	js_std_loop(ctx);

	// get the return value object.

	if (readRet)
	{
		JSValue val = JS_UNDEFINED;
		JSValue ret = JS_UNDEFINED;
		JSValue tag = JS_UNDEFINED;
		JSValue asy = JS_UNDEFINED;
		JSValue retStr = JS_UNDEFINED;
		JSValue retU64 = JS_UNDEFINED;

		val = JS_Eval(ctx, QJS_EVAL_RET_OBJ_NAME, strlen(QJS_EVAL_RET_OBJ_NAME), "", JS_EVAL_TYPE_GLOBAL);

		_bc_resetstdout();

		// parse the eval object.

		if (JS_IsObject(val))
		{
			tag = JS_GetPropertyStr(ctx, val, QJS_EVAL_TAG_PROP_NAME);

			if (JS_IsBool(tag))
			{
				ret = JS_GetPropertyStr(ctx, val, QJS_EVAL_RET_PROP_NAME);

				retStr = JS_GetPropertyStr(ctx, val, QJS_EVAL_RET_STR_PROP_NAME);

				asy = JS_GetPropertyStr(ctx, val, QJS_EVAL_ASYNC_ARGS_PROP_NAME);

				retU64 = JS_GetPropertyStr(ctx, val, QJS_EVAL_RET_U64_PROP_NAME);

				if (JS_IsString(asy))
				{
					const char* str = JS_ToCString(ctx, asy);

					if (str)
					{
						snprintf(asyncArgs, asyncArgsMaxSize, "%s", str);
						quietBcAsync = TRUE;
						JS_FreeCString(ctx, str);
					}
				}

				if (!JS_IsString(retU64))
					quiet = FALSE;
				else
				{
					const char* str = JS_ToCString(ctx, retU64);

					if (!str)
						quiet = FALSE;
					else
					{
						*retValU64 = BC_strtoui64(str, NULL, 10);
						retVal |= QJS_EVAL_SUCCESS_RETVALU64;
						JS_FreeCString(ctx, str);
					}
				}
			}
		}

		// print the result.

		if (!quietBcAsync && !quiet)
		{
			const char* str;

			str = JS_ToCString(ctx, JS_IsString(retStr) ? retStr : !JS_IsUndefined(ret) ? ret : val);

			if (str)
			{
				if (strstr(str, "Throw: ") == str)
					printf("%s", str);
				else
					printf("%s: %s\n", TagToStr(JS_VALUE_GET_TAG(!JS_IsUndefined(ret) ? ret : val)), str);

				JS_FreeCString(ctx, str);
			}
			else
			{
				printf("[exception]\n");
			}
		}

		// free the memory.

		JS_FreeValue(ctx, val);
		JS_FreeValue(ctx, tag);
		JS_FreeValue(ctx, ret);
		JS_FreeValue(ctx, asy);
		JS_FreeValue(ctx, retStr);
		JS_FreeValue(ctx, retU64);
	}

	// run the GC.

	JS_RunGC(rt);

	// did QJS call any not implemented function?

	_bc_resetstdout();

	if (strlen(QJSNotImplLog))
	{
		printf("=== WARNING === QuickJS called the following not implemented functions: %s\n", QJSNotImplLog);

		strcpy(QJSNotImplLog, "");
	}

	// return to the caller.

	return retVal;
}

// --------- NOT_IMPLEMENTED function definition: we need to log any not implemented function invoked by QJS. ---------

void NOT_IMPLEMENTED(const char* psz)
{
	if (strstr(QJSNotImplLog, psz))
		return;

	if (strlen(QJSNotImplLog) + strlen(psz) > sizeof(QJSNotImplLog) - 16)
		return;

	if (strlen(QJSNotImplLog))
		strcat(QJSNotImplLog, ",");

	strcat(QJSNotImplLog, psz);

	return;
}

// --------- Functions imported by QuickJS and implemented: (the others are in "QuickJSCppInterface.cpp") ---------

#define _DENORM		(-2)	/* C9X only */
#define _FINITE		(-1)
#define _INFCODE	1
#define _NANCODE	2
#define _D0	3	/* little-endian, small long doubles */
#define _D1	2
#define _D2	1
#define _D3	0
#define _DOFF	4
#define _DFRAC	((unsigned short)((1 << _DOFF) - 1))
#define _DMASK	((unsigned short)(0x7fff & ~_DFRAC))
#define _DMAX	((unsigned short)((1 << (15 - _DOFF)) - 1))
#define _DSIGN	((unsigned short)0x8000)

static short _Dtest(double* px) // from VS2012 CRT sources.
{	/* categorize *px */
	unsigned short* ps = (unsigned short*)(char*)px;

	if ((ps[_D0] & _DMASK) == _DMAX << _DOFF)
		return ((short)((ps[_D0] & _DFRAC) != 0 || ps[_D1] != 0
			|| ps[_D2] != 0 || ps[_D3] != 0 ? _NANCODE : _INFCODE));
	else if ((ps[_D0] & ~_DSIGN) != 0 || ps[_D1] != 0
		|| ps[_D2] != 0 || ps[_D3] != 0)
		return ((ps[_D0] & _DMASK) == 0 ? _DENORM : _FINITE);
	else
		return (0);
}

int __stdcall _bc_isnan(_In_ double _X)
{
	return _Dtest(&_X) == _NANCODE;
}

int __stdcall _bc_isfinite(_In_ double _X)
{
	return _Dtest(&_X) <= 0;
}

size_t __stdcall _bc__msize(void* ptr)
{
	return 0;
}

VOID __stdcall _bc_GetSystemTime(__out LPSYSTEMTIME lpSystemTime)
{
	memset(lpSystemTime, 0, 16); // 8 * sizeof(WORD)
	return;
}

BOOL __stdcall _bc_SystemTimeToFileTime(__in const LPSYSTEMTIME lpSystemTime, __out LPFILETIME lpFileTime)
{
	*(ULONG64*)lpFileTime = 0;
	return 0; // returns error.
}

// --------- Definitions for data imported by QuickJS: ---------

static char* bc_nullEnviron = NULL;

char** _bc__environ = &bc_nullEnviron;

FILE _bc__iob[_IOB_ENTRIES];

static int bc_errno = 0;

int* __stdcall _bc__errno(void)
{
	return &bc_errno;
}

// --------- Functions imported by QuickJS and NOT implemented: ---------

void(__stdcall* __stdcall _bc_signal(__in int _SigNum, __in_opt void(__stdcall* _Func)(int)))(int)
{
	NOT_IMPLEMENTED("signal");
	return 0;
}

double __stdcall _bc_sqrt(__in double _X)
{
	NOT_IMPLEMENTED("sqrt");
	return 0;
}

double __stdcall _bc_acos(__in double _X)
{
	NOT_IMPLEMENTED("acos");
	return 0;
}

double __stdcall _bc_asin(__in double _X)
{
	NOT_IMPLEMENTED("asin");
	return 0;
}

double __stdcall _bc_atan(__in double _X)
{
	NOT_IMPLEMENTED("atan");
	return 0;
}

double __stdcall _bc_atan2(__in double _Y, __in double _X)
{
	NOT_IMPLEMENTED("atan2");
	return 0;
}

double __stdcall _bc_cos(__in double _X)
{
	NOT_IMPLEMENTED("cos");
	return 0;
}

double __stdcall _bc_exp(__in double _X)
{
	NOT_IMPLEMENTED("exp");
	return 0;
}

double __stdcall _bc_log(__in double _X)
{
	NOT_IMPLEMENTED("log");
	return 0;
}

double __stdcall _bc_sin(__in double _X)
{
	NOT_IMPLEMENTED("sin");
	return 0;
}

double __stdcall _bc_tan(__in double _X)
{
	NOT_IMPLEMENTED("tan");
	return 0;
}

double __stdcall _bc_cosh(__in double _X)
{
	NOT_IMPLEMENTED("cosh");
	return 0;
}

double __stdcall _bc_sinh(__in double _X)
{
	NOT_IMPLEMENTED("sinh");
	return 0;
}

double __stdcall _bc_tanh(__in double _X)
{
	NOT_IMPLEMENTED("tanh");
	return 0;
}

double __stdcall _bc_log10(__in double _X)
{
	NOT_IMPLEMENTED("log10");
	return 0;
}

double __stdcall _bc_acosh(_In_ double _X)
{
	NOT_IMPLEMENTED("acosh");
	return 0;
}

double __stdcall _bc_asinh(_In_ double _X)
{
	NOT_IMPLEMENTED("asinh");
	return 0;
}

double __stdcall _bc_atanh(_In_ double _X)
{
	NOT_IMPLEMENTED("atanh");
	return 0;
}

double __stdcall _bc_expm1(_In_ double _X)
{
	NOT_IMPLEMENTED("expm1");
	return 0;
}

double __stdcall _bc_log1p(_In_ double _X)
{
	NOT_IMPLEMENTED("log1p");
	return 0;
}

double __stdcall _bc_log2(_In_ double _X)
{
	NOT_IMPLEMENTED("log2");
	return 0;
}

double __stdcall _bc_cbrt(_In_ double _X)
{
	NOT_IMPLEMENTED("cbrt");
	return 0;
}

double __stdcall _bc_round(_In_ double _X)
{
	NOT_IMPLEMENTED("round");
	return 0;
}

int __stdcall _bc_fesetround(_In_ int round)
{
	NOT_IMPLEMENTED("fesetround");
	return 0;
}

double __stdcall _bc_fmin(_In_ double _A, _In_ double _B)
{
	NOT_IMPLEMENTED("fmin");
	return 0;
}

double __stdcall _bc_fmax(_In_ double _A, _In_ double _B)
{
	NOT_IMPLEMENTED("fmax");
	return 0;
}

int __stdcall _bc_signbit(_In_ double _X)
{
	NOT_IMPLEMENTED("signbit");
	return 0;
}

long __stdcall _bc_lrint(_In_ double _X)
{
	NOT_IMPLEMENTED("lrint");
	return 0;
}

void __stdcall _bc_abort(void)
{
	NOT_IMPLEMENTED("abort");
	return;
}

int __stdcall _bc_fflush(__inout_opt FILE* _File)
{
	NOT_IMPLEMENTED("fflush");
	return 0;
}

int __stdcall _bc_read(int _FileHandle, __out_bcount(_MaxCharCount) void* _DstBuf, __in unsigned int _MaxCharCount)
{
	NOT_IMPLEMENTED("read");
	return 0;
}

void __stdcall _bc__assert(__in_z const char* _Message, __in_z const char* _File, __in unsigned _Line)
{
	NOT_IMPLEMENTED("_assert");
	return;
}

int __stdcall _bc_stat(const char* restrict path, struct _bc_stat* restrict buf)
{
	NOT_IMPLEMENTED("stat");
	return 0;
}

void __stdcall _bc_exit(__in int _Code)
{
	NOT_IMPLEMENTED("exit");
	return;
}

char* __stdcall _bc_getenv(__in_z const char* _VarName)
{
	NOT_IMPLEMENTED("getenv");
	return 0;
}

char* __stdcall _bc__fullpath(__out_ecount_z_opt(_SizeInBytes) char* _FullPath, __in_z const char* _Path, __in size_t _SizeInBytes)
{
	NOT_IMPLEMENTED("_fullpath");
	return 0;
}

int __stdcall _bc__putenv(__in_z const char* _EnvString)
{
	NOT_IMPLEMENTED("_putenv");
	return 0;
}

void __stdcall _bc_clearerr(__inout FILE* _File)
{
	NOT_IMPLEMENTED("clearerr");
	return;
}

int __stdcall _bc_fclose(__inout FILE* _File)
{
	NOT_IMPLEMENTED("fclose");
	return 0;
}

int __stdcall _bc_feof(__in FILE* _File)
{
	NOT_IMPLEMENTED("feof");
	return 0;
}

int __stdcall _bc_ferror(__in FILE* _File)
{
	NOT_IMPLEMENTED("ferror");
	return 0;
}

int __stdcall _bc_fgetc(__inout FILE* _File)
{
	NOT_IMPLEMENTED("fgetc");
	return 0;
}

FILE* __stdcall _bc_fopen(__in_z const char* _Filename, __in_z const char* _Mode)
{
	NOT_IMPLEMENTED("fopen");
	return 0;
}

int __stdcall _bc_fputc(__in int _Ch, __inout FILE* _File)
{
	NOT_IMPLEMENTED("fputc");
	return 0;
}

size_t __stdcall _bc_fread(__out_bcount(_ElementSize* _Count) void* _DstBuf, __in size_t _ElementSize, __in size_t _Count, __inout FILE* _File)
{
	NOT_IMPLEMENTED("fread");
	return 0;
}

int __stdcall _bc_fseek(__inout FILE* _File, __in long _Offset, __in int _Origin)
{
	NOT_IMPLEMENTED("fseek");
	return 0;
}

long __stdcall _bc_ftell(__inout FILE* _File)
{
	NOT_IMPLEMENTED("ftell");
	return 0;
}

int __stdcall _bc__pclose(__inout FILE* _File)
{
	NOT_IMPLEMENTED("_pclose");
	return 0;
}

FILE* __stdcall _bc__popen(__in_z const char* _Command, __in_z const char* _Mode)
{
	NOT_IMPLEMENTED("_popen");
	return 0;
}

int __stdcall _bc_rename(__in_z const char* _OldFilename, __in_z const char* _NewFilename)
{
	NOT_IMPLEMENTED("rename");
	return 0;
}

int __stdcall _bc_unlink(__in_z const char* _Filename)
{
	NOT_IMPLEMENTED("unlink");
	return 0;
}

FILE* __stdcall _bc_tmpfile(void)
{
	NOT_IMPLEMENTED("tmpfile");
	return 0;
}

FILE* __stdcall _bc_fdopen(__in int _FileHandle, __in_z __format_string const char* _Format)
{
	NOT_IMPLEMENTED("fdopen");
	return 0;
}

int __stdcall _bc_fileno(__in FILE* _File)
{
	NOT_IMPLEMENTED("fileno");
	return 0;
}

char* __stdcall _bc_strerror(__in int _param_)
{
	NOT_IMPLEMENTED("strerror");
	return 0;
}

int __stdcall _bc__setmode(__in int _FileHandle, __in int _Mode)
{
	NOT_IMPLEMENTED("_setmode");
	return 0;
}

intptr_t __stdcall _bc__get_osfhandle(__in int _FileHandle)
{
	NOT_IMPLEMENTED("_get_osfhandle");
	return 0;
}

int __stdcall _bc_close(__in int _FileHandle)
{
	NOT_IMPLEMENTED("close");
	return 0;
}

int __stdcall _bc_isatty(__in int _FileHandle)
{
	NOT_IMPLEMENTED("isatty");
	return 0;
}

long __stdcall _bc_lseek(__in int _FileHandle, __in long _Offset, __in int _Origin)
{
	NOT_IMPLEMENTED("lseek");
	return 0;
}

int __stdcall _bc_open(__in_z const char* _Filename, __in int _OpenFlag, ...)
{
	NOT_IMPLEMENTED("open");
	return 0;
}

int __stdcall _bc_write(__in int _Filehandle, __in_bcount(_MaxCharCount) const void* _Buf, __in unsigned int _MaxCharCount)
{
	NOT_IMPLEMENTED("write");
	return 0;
}

int __stdcall _bc__utime(__in_z const char* _Filename, __in_opt struct _utimbuf* _Time)
{
	NOT_IMPLEMENTED("_utime");
	return 0;
}

double __stdcall _bc_hypot(__in double _X, __in double _Y)
{
	NOT_IMPLEMENTED("hypot");
	return 0;
}

int __stdcall _bc_rmdir(const char* path)
{
	NOT_IMPLEMENTED("rmdir");
	return 0;
}

char* __stdcall _bc_getcwd(char* buffer, size_t size)
{
	NOT_IMPLEMENTED("getcwd");
	return 0;
}

int __stdcall _bc_chdir(const char* path)
{
	NOT_IMPLEMENTED("chdir");
	return 0;
}

int __stdcall _bc_mkdir(const char* path)
{
	NOT_IMPLEMENTED("mkdir");
	return 0;
}

DIR* __stdcall _bc_opendir(const char* dirname)
{
	NOT_IMPLEMENTED("opendir");
	return 0;
}

struct dirent* __stdcall _bc_readdir(DIR* dirp)
{
	NOT_IMPLEMENTED("readdir");
	return 0;
}

int __stdcall _bc_closedir(DIR* dirp)
{
	NOT_IMPLEMENTED("closedir");
	return 0;
}

// --------- Windows APIs: ---------

DWORD __stdcall _bc_WaitForSingleObject(__in HANDLE hHandle, __in DWORD dwMilliseconds)
{
	NOT_IMPLEMENTED("WaitForSingleObject");
	return 0;
}

VOID __stdcall _bc_Sleep(__in DWORD dwMilliseconds)
{
	NOT_IMPLEMENTED("Sleep");
	return;
}

BOOL __stdcall _bc_GetConsoleScreenBufferInfo(__in HANDLE hConsoleOutput, __out PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo)
{
	NOT_IMPLEMENTED("GetConsoleScreenBufferInfo");
	return 0;
}

BOOL __stdcall _bc_SetConsoleMode(__in HANDLE hConsoleHandle, __in DWORD dwMode)
{
	NOT_IMPLEMENTED("SetConsoleMode");
	return 0;
}

DWORD __stdcall _bc_GetTimeZoneInformation(__out LPTIME_ZONE_INFORMATION lpTimeZoneInformation)
{
	NOT_IMPLEMENTED("GetTimeZoneInformation");
	return 0;
}
