#pragma once

#ifdef _WIN32
#define LINE_ENDING_SEQUENCE_A		'\r\n'
#define LINE_ENDING_SEQUENCE_W		'\r\0\n\0'
#define LINE_ENDING_SEQUENCE_LEN	2	
#define LINE_ENDING_STR_T			_T("\r\n")

#define LINE_ENDING_STR_A			"\r\n"
#define LINE_ENDING_STR_W			L"\r\n"

#define MAX_INTEGER_TEXT_REPRESENTATION_LEN		11	//Longest 32-bit integer: "-2147483648"


#else
#define LINE_ENDING_STR_T			_T("\n")
#define LINE_ENDING_STR_A			"\n"
#define LINE_ENDING_STR_W			L"\n"
#endif

#if defined(_AMD64_) || defined (_WIN64)
#define BAZISLIB_X64
#endif

#ifdef BZSLIB_WINKERNEL
#define EXTENDED_REALLOC
#endif
