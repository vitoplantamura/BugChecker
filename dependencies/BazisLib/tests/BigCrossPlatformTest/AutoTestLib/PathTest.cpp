#include "stdafx.h"
#include "TestPrint.h"
#include <bzscore/path.h>

using namespace BazisLib;

bool TestPathAPI()
{
#ifdef BZSLIB_UNIXWORLD
	TEST_ASSERT(Path::Combine(ConstString(_T("/tmp/")), ConstString(_T("fn"))) == _T("/tmp/fn"));
	TEST_ASSERT(Path::Combine(ConstString(_T("/tmp")), ConstString(_T("fn"))) == _T("/tmp/fn"));
#else
	TEST_ASSERT(Path::Combine(ConstString(_T("c:\\temp\\")), ConstString(_T("fn"))) == _T("c:\\temp\\fn"));
	TEST_ASSERT(Path::Combine(ConstString(_T("c:\\temp")), ConstString(_T("fn"))) == _T("c:\\temp\\fn"));
#endif

	TEST_ASSERT(Path::GetPathWithoutExtension(ConstString(_T("/tmp/test"))) == _T("/tmp/test"));
	TEST_ASSERT(Path::GetPathWithoutExtension(ConstString(_T("/tmp/test.dat"))) == _T("/tmp/test"));
	TEST_ASSERT(Path::GetPathWithoutExtension(ConstString(_T("/tmp.x/test.dat"))) == _T("/tmp.x/test"));
	TEST_ASSERT(Path::GetPathWithoutExtension(ConstString(_T("/tmp.x/test"))) == _T("/tmp.x/test"));

	TEST_ASSERT(Path::GetDirectoryName(ConstString(_T("xxx"))) == _T(""));
	TEST_ASSERT(Path::GetDirectoryName(ConstString(_T("/tmp/zzz/xxx"))) == _T("/tmp/zzz"));

	TEST_ASSERT(Path::GetExtensionExcludingDot(ConstString(_T("xxx"))) == _T(""));
	TEST_ASSERT(Path::GetExtensionExcludingDot(ConstString(_T("/tmp/zzz/xxx"))) == _T(""));
	TEST_ASSERT(Path::GetExtensionExcludingDot(ConstString(_T("/tmp/zzz.x/xxx"))) == _T(""));
	TEST_ASSERT(Path::GetExtensionExcludingDot(ConstString(_T("/tmp/zzz.x/xxx.y"))) == _T("y"));
#ifndef BZSLIB_UNIXWORLD
	TEST_ASSERT(Path::GetExtensionExcludingDot(ConstString(_T("c:\\tmp\\zzz.x\\xxx.y"))) == _T("y"));
#endif

	TEST_ASSERT(Path::GetFileName(ConstString(_T("xxx"))) == _T("xxx"));
	TEST_ASSERT(Path::GetFileName(ConstString(_T("/tmp/zzz/xxx"))) == _T("xxx"));
	TEST_ASSERT(Path::GetFileName(ConstString(_T("/tmp/zzz.x/xxx"))) == _T("xxx"));
	TEST_ASSERT(Path::GetFileName(ConstString(_T("/tmp/zzz.x/xxx.y"))) == _T("xxx.y"));

	TEST_ASSERT(Path::GetFileNameWithoutExtension(ConstString(_T("xxx"))) == _T("xxx"));
	TEST_ASSERT(Path::GetFileNameWithoutExtension(ConstString(_T("/tmp/zzz/xxx"))) == _T("xxx"));
	TEST_ASSERT(Path::GetFileNameWithoutExtension(ConstString(_T("/tmp/zzz.x/xxx"))) == _T("xxx"));
	TEST_ASSERT(Path::GetFileNameWithoutExtension(ConstString(_T("/tmp/zzz.x/xxx.y"))) == _T("xxx"));

#ifdef BZSLIB_UNIXWORLD
	TEST_ASSERT(Path::IsAbsolute(ConstString(_T("/tmp"))));
	TEST_ASSERT(!Path::IsAbsolute(ConstString(_T("tmp"))));
#else
	TEST_ASSERT(Path::IsAbsolute(ConstString(_T("c:\\temp"))));
	TEST_ASSERT(!Path::IsAbsolute(ConstString(_T("temp"))));
	TEST_ASSERT(!Path::IsAbsolute(ConstString(_T("\\temp"))));
	TEST_ASSERT(Path::IsAbsolute(ConstString(_T("\\\\temp"))));
#endif
	return true;
}