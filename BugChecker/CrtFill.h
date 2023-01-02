#ifndef CRTFILL_H
#define CRTFILL_H

#ifdef __cplusplus
#define CRTFILL_H_EXTERN extern "C"
#else
#define CRTFILL_H_EXTERN extern
#endif

CRTFILL_H_EXTERN __int64 /*_CRTIMP*/ __cdecl BC_strtoi64(
	const char* nptr,
	char** endptr,
	int ibase
);

CRTFILL_H_EXTERN unsigned __int64 /*_CRTIMP*/ __cdecl BC_strtoui64(
	const char* nptr,
	char** endptr,
	int ibase
);

#endif
