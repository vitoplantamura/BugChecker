#pragma once

#include "framework.h"

#include <EASTL/string.h>

#define BC_DRIVER_NAME L"BugChecker"

class DriverControl
{
public:

	static eastl::string Start();
	static eastl::string Stop();
};
