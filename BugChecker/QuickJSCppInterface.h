#ifndef QUICKJSCPPINTERFACE_H
#define QUICKJSCPPINTERFACE_H

#ifdef __cplusplus
class NullableU64
{
public:
	ULONG64 value = 0;
	BOOLEAN isNull = TRUE;
};

class QuickJSCppInterface
{
public:

	static BcCoroutine Eval(const CHAR* psz, BYTE* context, ULONG contextLen, BOOLEAN is32bitCompat, NullableU64* pRetVal = NULL, BOOLEAN quiet = FALSE, BOOLEAN wrapInAsyncFn = TRUE) noexcept;
};
#endif

#ifdef __cplusplus
#define QJSCPPI_EXTERN extern "C"
#else
#define QJSCPPI_EXTERN extern
#endif

QJSCPPI_EXTERN void __stdcall _bc_resetstdout();

QJSCPPI_EXTERN const char* QJSStartupScript;

#define QJS_EVAL_RET_OBJ_NAME "_bc_eval_"
#define QJS_EVAL_TAG_PROP_NAME "_bc_eval_object_"
#define QJS_EVAL_RET_PROP_NAME "ret"
#define QJS_EVAL_RET_STR_PROP_NAME "retStr"
#define QJS_EVAL_RET_U64_PROP_NAME "retU64"
#define QJS_EVAL_ASYNC_ARGS_PROP_NAME "async_args"

#endif
