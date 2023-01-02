#ifndef QUICKJSCINTERFACE_H
#define QUICKJSCINTERFACE_H

#ifdef __cplusplus
#define QJSCI_EXTERN extern "C"
#else
#define QJSCI_EXTERN extern
#endif

QJSCI_EXTERN void NOT_IMPLEMENTED(const char* psz);
QJSCI_EXTERN FILE _bc__iob[_IOB_ENTRIES];

typedef int BOOL;

QJSCI_EXTERN int QJS_Eval(const char* src, char* asyncArgs, int asyncArgsMaxSize, unsigned __int64* retValU64, BOOL quiet);
QJSCI_EXTERN void QJS_SetGlobalStringProp(const char* name, const char* value);

#define QJS_EVAL_ERROR_INIT 1
#define QJS_EVAL_SUCCESS_RETVALU64 2

#endif
