#pragma once

#include "cmndef.h"
#ifdef BZS_KEXT
#include <kern/assert.h>
#else
#include <assert.h>
#endif

#ifndef ASSERT
#define ASSERT assert
#endif

#ifdef _AMD64_
#define ASSERT_32BIT(a) ASSERT(!((a) & 0xFFFFFFFF00000000))
#else
#define ASSERT_32BIT(a)
#endif

#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__ [(e)?1:-1]
#define C_ASSERT2(e) typedef char __C_ASSERT2__ [(e)?1:-1]
#define C_ASSERT3(e) typedef char __C_ASSERT3__ [(e)?1:-1]
#endif