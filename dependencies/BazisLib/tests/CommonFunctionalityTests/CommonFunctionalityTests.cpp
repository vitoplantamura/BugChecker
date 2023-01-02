// CommonFunctionalityTests.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

bool TestBufferedSockets();

int _tmain(int argc, _TCHAR* argv[])
{
	if (!TestBufferedSockets())
		__asm int 3;


	return 0;
}

