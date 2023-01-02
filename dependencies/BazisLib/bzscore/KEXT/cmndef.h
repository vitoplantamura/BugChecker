#pragma once

#include <machine/types.h>
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
#define TRUE 1
#define FALSE 0
#define NULL 0

#include "../Posix/compat.h"
#include <sys/types.h>
#include <uuid/uuid.h>
#include <stdint.h>

typedef uint16_t wchar16_t;

typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;

typedef uuid_t GUID;

__private_extern__ void* _realloc(void * _Memory, size_t _Size, size_t _OldSize);

#define EXTENDED_REALLOC