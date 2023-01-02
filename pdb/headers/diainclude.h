#pragma once

#ifndef WINDOWSXP
#include "dia2.h"
#include "diacreate.h"
#include "cvconst.h"
#else
// binds to the MSDIA100.DLL, that ships with VS 2010, and that is the last version supported by Windows XP.
#include "..\..\dependencies\DiaSdk10Include\dia2.h"
#include "..\..\dependencies\DiaSdk10Include\diacreate.h"
#include "..\..\dependencies\DiaSdk10Include\cvconst.h"
#endif
