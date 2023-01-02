#pragma once

void TestPrint(const char *pFormat, ...);

#ifdef _MSC_VER
#define TEST_ASSERT(x) if (!(x)) {TestPrint("Test failed: %s [%s:%d]\n", #x, __FILE__, __LINE__); __debugbreak(); return false; }
#else
#define TEST_ASSERT(x) if (!(x)) {TestPrint("Test failed: %s [%s:%d]\n", #x, __FILE__, __LINE__); asm("int3"); return false; }
#endif