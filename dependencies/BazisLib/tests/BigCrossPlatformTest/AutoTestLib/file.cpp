#include "stdafx.h"
#include <bzscore/autofile.h>
#include <bzscore/datetime.h>
#include "TestPrint.h"

using namespace BazisLib;

bool TestFileAPI(const BazisLib::String &tempPath)
{
	BazisLib::String testFile = Path::Combine(tempPath, ConstString(_T("bzs1.tmp")));

	//1. Test native file API
	{
		ActionStatus openStatus;
		File f1(testFile, FileModes::CreateOrOpenRW, &openStatus);
		TEST_ASSERT(openStatus.Successful());

		TEST_ASSERT(f1.WriteString("Hello, file system") == 18);
	}

	{
		ActionStatus openStatus;
		File f1(testFile, FileModes::OpenReadWrite, &openStatus);
		TEST_ASSERT(openStatus.Successful());

		char sz[512];
		TEST_ASSERT(f1.Read(sz, sizeof(sz)) == 18);
		TEST_ASSERT(f1.GetSize() == 18);
		TEST_ASSERT(!memcmp(sz, "Hello, file system", 18));
		
		TEST_ASSERT(f1.Seek(-6, FileFlags::FileCurrent) == (18 - 6));
		TEST_ASSERT(f1.Read(sz, sizeof(sz)) == 6);
		TEST_ASSERT(!memcmp(sz, "system", 6));

		DateTime modificationTime;
		TEST_ASSERT(f1.GetFileTimes(NULL, &modificationTime, NULL).Successful());
		TimeSpan elapsed = DateTime::Now() - modificationTime;
		ULONGLONG msec = elapsed.GetTotalMilliseconds();
		TEST_ASSERT(msec < 10000);
	}

#ifndef BZSLIB_KEXT
	TEST_ASSERT(File::Delete(testFile.c_str()).Successful());

	{
		ActionStatus openStatus;
		File f1(testFile, FileModes::OpenReadWrite, &openStatus);
		TEST_ASSERT(openStatus.GetErrorCode() == FileNotFound);;
	}
#endif

	//2. Test auto file class
	ActionStatus openStatus;
	ManagedPointer<AIFile> pFile = new ACFile(testFile, FileModes::CreateOrOpenRW, &openStatus);
	TEST_ASSERT(openStatus.Successful());
	TEST_ASSERT(pFile->Write("Hello, file system", 18) == 18);
	TEST_ASSERT(pFile->GetSize() == 18);
	TEST_ASSERT(pFile->GetPosition() == 18);
	pFile = NULL;

	pFile = new ACFile(testFile, FileModes::OpenReadOnly, &openStatus);
	TEST_ASSERT(pFile->GetSize() == 18);
	TEST_ASSERT(openStatus.Successful());
	pFile = NULL;
#ifndef BZSLIB_KEXT
	TEST_ASSERT(File::Delete(testFile.c_str()).Successful());
	pFile = new ACFile(testFile, FileModes::OpenReadOnly, &openStatus);
	TEST_ASSERT(openStatus.GetErrorCode() == FileNotFound);
#endif

	//3. Test directory
#ifndef BZSLIB_KEXT
	String testDir = Path::Combine(tempPath, ConstString(_T("bzsdir.tmp")));
	ActionStatus st = Directory::Create(testDir);
	TEST_ASSERT(st.Successful() || st.GetErrorCode() == AlreadyExists);

	File(Path::Combine(testDir, ConstString(_T("file1"))), FileModes::CreateOrTruncateRW).Write("123", 3);
	ManagedPointer<AIDirectory> pDir = new ACDirectory(testDir);
	pFile = pDir->CreateFile(_T("file2"), FileFlags::ReadWriteAccess, FileFlags::CreateAlways);
	TEST_ASSERT(pFile && pFile->Valid());
	TEST_ASSERT(pFile->Write("1234", 4) == 4);
	pFile = NULL;

	ManagedPointer<AIDirectory> pSubDir = pDir->CreateDirectory(_T("subdir1"));

	bool file1Found = false, file2Found = false, subdirFound = false;	
	for (ManagedPointer<AIDirectoryIterator> pIter = pDir->FindFirstFile(); pIter->Valid(); pIter->FindNextFile())
	{
		if (pIter->IsAPseudoEntry())
			continue;
		TEST_ASSERT(Path::GetDirectoryName(pIter->GetFullPath()) == testDir);
		TEST_ASSERT(Path::GetFileName(pIter->GetFullPath()) == pIter->GetRelativePath());
		
		if (pIter->GetRelativePath() == _T("file1"))
		{
			file1Found = true;
			TEST_ASSERT(!pIter->IsDirectory());
			TEST_ASSERT(pIter->GetSize() == 3);
		}
		else if (pIter->GetRelativePath() == _T("file2"))
		{
			file2Found = true;
			TEST_ASSERT(!pIter->IsDirectory());
			TEST_ASSERT(pIter->GetSize() == 4);
		}
		else if (pIter->GetRelativePath() == _T("subdir1"))
		{
			subdirFound = true;
			TEST_ASSERT(pIter->IsDirectory());
		}
		else
		{
			TEST_ASSERT(false);
		}
	}

	TEST_ASSERT(file1Found && file2Found && subdirFound);
	
	TEST_ASSERT(Directory::Exists(tempPath));
	TEST_ASSERT(File::Exists(Path::Combine(testDir, _T("file1"))));
	TEST_ASSERT(!File::Exists(Path::Combine(testDir, _T("fileX"))));

	TEST_ASSERT(pDir->DeleteFile(_T("file2")).Successful());
	TEST_ASSERT(!File::Exists(Path::Combine(testDir, _T("file2"))));

	TEST_ASSERT(File::Delete(Path::Combine(testDir, _T("file1"))).Successful());
	TEST_ASSERT(!File::Exists(Path::Combine(testDir, _T("file1"))));

	TEST_ASSERT(Directory::Exists(Path::Combine(testDir, _T("subdir1"))));
	TEST_ASSERT(pDir->RemoveDirectory(_T("subdir1")).Successful());
	TEST_ASSERT(!Directory::Exists(Path::Combine(testDir, _T("subdir1"))));

	TEST_ASSERT(Directory::Remove(testDir).Successful());
	TEST_ASSERT(!Directory::Exists(testDir));
#endif
	return true;
}
