#pragma once
#include "../string.h"
#define BAZISLIB_NO_STRUCTURED_TIME

#define gettimeofday(a,b) microtime(a)
#include "../Posix/datetime.h"