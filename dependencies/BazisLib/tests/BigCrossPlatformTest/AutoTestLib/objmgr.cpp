// objmgr.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <bzscore/atomic.h>
#include "TestPrint.h"

static int s_AssertCount = 0;

int _inc_assert_count(const char *expr);

int _inc_assert_count(const char *expr)
{
	TestPrint(">>>>>ASSERTION FAILED<<<<< (%s)\n\n", expr);
	s_AssertCount++;
	return 0;
}

static bool s_error = false;

#ifdef _MSC_VER
#define __dbg_break_point()	{s_error = true; __debugbreak();}
#else
#define __dbg_break_point()	{s_error = true; asm("int3");}
#endif

#undef ASSERT
#define ASSERT(expr) ((expr) || _inc_assert_count(#expr))

#include <bzscore/objmgr.h>
//#include <bzscore/Win32/memdbg.h>

static BazisLib::AtomicInt32 s_ObjectCount;

class CountedObject : AUTO_OBJECT
{
public:
	CountedObject() {s_ObjectCount++;}
	~CountedObject() {s_ObjectCount--;}
};

/*
	This is a test project for object manager subsystem.
	See the SAMPLES\COMMON\OBJMGR for basic object manager sample.
*/

using namespace BazisLib;

static unsigned s_DiskInstances = 0;

class AIDiskDrive : AUTO_INTERFACE
{
public:
	virtual unsigned ReadSector(unsigned Number)=0;
};

class SomeDisk : public CountedObject, public AIDiskDrive
{
public:
	virtual unsigned ReadSector(unsigned Number)
	{
		TestPrint("SomeDisk: Reading sector 0x%X\n", Number);
		return 512;
	}

	SomeDisk()
	{
		s_DiskInstances++;
	}

	~SomeDisk()
	{
		TestPrint("SomeDisk: destructor\n");
		s_DiskInstances--;
	}

};

class SomeFileSystem : public CountedObject
{
private:
	DECLARE_REFERENCE(AIDiskDrive, m_pDisk);
public:
	SomeFileSystem(ManagedPointer<AIDiskDrive> pDisk) :
	  INIT_REFERENCE(m_pDisk, pDisk)
	{
	}

	unsigned ReadFile(const char *pFile)
	{
		if (!m_pDisk)
		{
			TestPrint("SomeFileSystem: No underlying disk!", pFile);
			return 0;
		}
		TestPrint("SomeFileSystem: Reading file \"%s\"\n", pFile);
		return m_pDisk->ReadSector(pFile[0]);
	}

	~SomeFileSystem()
	{
		TestPrint("SomeFileSystem: destructor\n");
	}
};

class SomeUI : public CountedObject
{
private:
	DECLARE_REFERENCE(AIDiskDrive, m_pDisk);
	DECLARE_REFERENCE(SomeFileSystem, m_pFS);

public:
	SomeUI() :
	  INIT_REFERENCE(m_pDisk, new SomeDisk()),
	  INIT_REFERENCE(m_pFS, new SomeFileSystem(m_pDisk))
	{
	}

    ManagedPointer<SomeFileSystem> GetFileSystem()
	{
		return m_pFS;
	}

    ManagedPointer<AIDiskDrive> GetDisk()
	{
		return m_pDisk;
	}

	ManagedPointer<SomeFileSystem> GetFileSystemAlt1()
	{
		ManagedPointer<SomeFileSystem> ptr = m_pFS;
		return ptr;
	}

    ManagedPointer<SomeFileSystem> GetFileSystemAlt2()
	{
		ManagedPointer<SomeFileSystem> ptr = NULL;
		ptr = m_pFS;
		return ptr;
	}

    ManagedPointer<SomeFileSystem> GetFileSystemAlt3()
	{
		ManagedPointer<SomeFileSystem> ptr = m_pFS;
		ptr = m_pFS;
		return ptr;
	}

	ManagedPointer<SomeFileSystem> CreateAnotherFileSystem()
	{
		return new SomeFileSystem(m_pDisk);
	}

	void DoSomething()
	{
		m_pFS->ReadFile("Nth file");
		m_pDisk->ReadSector(3);
	}

	virtual ~SomeUI()
	{
		TestPrint("SomeUI: destructor\n");
	}
};

class BadReferringObject : public CountedObject
{
private:
	ManagedPointer<AIDiskDrive> m_pDisk;

public:
	BadReferringObject() :
		m_pDisk(new SomeDisk())
	{
	}
};

class SelfReferringObject : public CountedObject
{
private:
	DECLARE_REFERENCE(AIDiskDrive, m_pDisk);

public:
	DECLARE_REFERENCE(SelfReferringObject, m_pAnother);

public:
	SelfReferringObject(ManagedPointer<AIDiskDrive> pDisk, ManagedPointer<SelfReferringObject> pObj = NULL) :
	  INIT_REFERENCE(m_pAnother, pObj),
	  INIT_REFERENCE(m_pDisk, pDisk)
	{
	}
};

void Read3Files(ManagedPointer <SomeFileSystem> pFS)
{
	pFS->ReadFile("1st file");
	pFS->ReadFile("2nd file");
	pFS->ReadFile("3rd file");
}

bool TestWeakLinks();
static bool MainTestBody();
bool ErrorDetectionTests();

bool ObjectManagerTestEntry(bool runTestsCausingMemoryLeaks)
{
	TestPrint("Preparing tests...\n");
	{
		//MemoryLeakDetector detector;
		if (!MainTestBody())
			return false;

		if (s_ObjectCount)
		{
			TestPrint("ERROR: %d objects were not deleted!", (int)s_ObjectCount);
			return false;
		}
	}

	if (runTestsCausingMemoryLeaks)
		if (!ErrorDetectionTests())
			return false;

	return true;
}

static bool MainTestBody()
{
	TestPrint("Testing weak links...\n");
	if (!TestWeakLinks())
	{
		TestPrint("Failed!\n");
		return false;
	}
	TestPrint("Passed\n");
	{
		ManagedPointer<SomeUI> pUI = new SomeUI();

		ManagedPointer<SomeFileSystem> pFS = pUI->GetFileSystem();

		Read3Files(pFS);

		pFS = pUI->CreateAnotherFileSystem();
		Read3Files(pFS);

		pUI->DoSomething();

		ManagedPointer<SomeFileSystem> pFS2 = NULL;
		pFS2 = pFS;
		
		ManagedPointer<SomeFileSystem> pFS3 = pFS2;
		pFS = pFS3;
		pFS2 = pFS;
		
		Read3Files(pUI->GetFileSystemAlt1());
		Read3Files(pUI->GetFileSystemAlt2());
		Read3Files(pUI->GetFileSystemAlt3());

		Read3Files(pFS2);

		//We create a complex object structure without circular references. This should not be a problem.
		ManagedPointer<SelfReferringObject> pObj1 = new SelfReferringObject(pUI->GetDisk());
		ManagedPointer<SelfReferringObject> pObj2 = new SelfReferringObject(pUI->GetDisk(), pObj1);
		ManagedPointer<SelfReferringObject> pObj3 = new SelfReferringObject(pUI->GetDisk(), pObj2);
		ManagedPointer<SelfReferringObject> pObj4 = new SelfReferringObject(pUI->GetDisk(), pObj3);
		ManagedPointer<SelfReferringObject> pObj5 = new SelfReferringObject(pUI->GetDisk(), pObj4);
		ManagedPointer<SelfReferringObject> pObj6 = new SelfReferringObject(pUI->GetDisk(), pObj5);

		pObj3->m_pAnother = pObj1;
		pObj4->m_pAnother = pObj3;
		pObj5->m_pAnother = pObj1;
		pObj2->m_pAnother = pObj5;

		ManagedPointer<AIDiskDrive> pDisk = pUI->GetDisk();

		pFS = new SomeFileSystem(pDisk);
		pFS2 = new SomeFileSystem(new SomeDisk());

		//Very, very exotic. However, one can try to use this
		SomeFileSystem *puFS1 = pFS.RetainAndGet();
		SomeFileSystem *puFS2 = pFS.RetainAndGet();

		//A copy operator for parent service reference should be invoked.
		//You can check this by setting breakpoint on ParentServiceReference::Assign() method.
		*puFS1 = *puFS2;

		puFS1->Release();
		puFS2->Release();
	}
	if (s_DiskInstances)
	{
		TestPrint("!ERROR! Disk instance is not freed. There is probably a bug in Object Manager!\n");
		__dbg_break_point();
		return false;
	}
	return true;
}

bool ErrorDetectionTests()
{
	TestPrint("---------------------------------------------------\n");
	TestPrint("Object freeing test PASSED!\nNow we'll test debug checks.");
#ifndef BZSLIB_KEXT
	{
		TestPrint("Trying to create a managed object in stack... Normally, 4 errors should be detected.\n");
		s_AssertCount = 0;
		SomeUI UI;
	}
	if (s_AssertCount != 4)
		__dbg_break_point();
#endif
	s_AssertCount = 0;

	TestPrint("Testing bad placed managed references... 1 error should pop up\n");

	BadReferringObject *pObj = new BadReferringObject();
	pObj->Release();

	if (s_AssertCount != 1)
		__dbg_break_point();
	s_AssertCount = 0;

	TestPrint("Attempting premature object deletion... Another 1 error should pop up\n");

	SomeDisk *pDisk = new SomeDisk();
	delete pDisk;

	if (s_AssertCount != 1)
		__dbg_break_point();
	s_AssertCount = 0;

	TestPrint("Attempting to create a circular reference. 2 errors are expected\n");
	{
		ManagedPointer<SomeUI> pUI = new SomeUI();
		ManagedPointer<SelfReferringObject> pObj1 = new SelfReferringObject(pUI->GetDisk());
		ManagedPointer<SelfReferringObject> pObj2 = new SelfReferringObject(pUI->GetDisk(), pObj1);
		ManagedPointer<SelfReferringObject> pObj3 = new SelfReferringObject(pUI->GetDisk(), pObj2);
		ManagedPointer<SelfReferringObject> pObj4 = new SelfReferringObject(pUI->GetDisk(), pObj3);
		ManagedPointer<SelfReferringObject> pObj5 = new SelfReferringObject(pUI->GetDisk(), pObj4);
		ManagedPointer<SelfReferringObject> pObj6 = new SelfReferringObject(pUI->GetDisk(), pObj5);

		pObj1->m_pAnother = pObj6;
	}

	if (s_AssertCount != 2)
		__dbg_break_point();
	s_AssertCount = 0;

	TestPrint("Another circular reference test. 2 errors are expected\n");
	{
		ManagedPointer<SomeUI> pUI = new SomeUI();
		ManagedPointer<SelfReferringObject> pObj1 = new SelfReferringObject(pUI->GetDisk());
		ManagedPointer<SelfReferringObject> pObj2 = new SelfReferringObject(pUI->GetDisk(), pObj1);
		ManagedPointer<SelfReferringObject> pObj3 = new SelfReferringObject(pUI->GetDisk(), pObj2);
		ManagedPointer<SelfReferringObject> pObj4 = new SelfReferringObject(pUI->GetDisk(), pObj3);
		ManagedPointer<SelfReferringObject> pObj5 = new SelfReferringObject(pUI->GetDisk(), pObj4);
		ManagedPointer<SelfReferringObject> pObj6 = new SelfReferringObject(pUI->GetDisk(), pObj5);

		pObj3->m_pAnother = pObj1;
		pObj4->m_pAnother = pObj3;
		pObj5->m_pAnother = pObj1;
		pObj2->m_pAnother = pObj4;
		pObj3->m_pAnother = pObj2;
	}

	if (s_AssertCount != 2)
		__dbg_break_point();
	s_AssertCount = 0;

	TestPrint("---------------------------------------------------\n");
	if (s_error)
		TestPrint("An error was detected during debug assertion test!\nThere may be a bug in Object Manager!!!\n");
	else
	{
		TestPrint("All tests done.\n");
		TestPrint("You can also try uncommenting a piece of code at the end of the\n");
		TestPrint("\"objmgr.cpp\" file to test some compile-time checks\n");
	}

	//The following code should not compile
	/*

	{
		ManagedPointer<SomeUI> pUI = new SomeUI;
		SomeUI *pUnmanaged = pUI;	//Instead use pUnmanaged = pUI.RetainAndGet()
		ParentServiceReference<SomeUI> Reference;	//Parent references should only be declared using DECLARE_REFERENCE
		ParentServiceReference<SomeUI> Reference2 = new SomeUI;	//The same thing

		SelfReferringObject *pObj = new SelfReferringObject(NULL);
		SelfReferringObject *pObj2 = new SelfReferringObject(*pObj);	//Copy constructor should fail, as we did not use
																		//COPY_REFERENCE macro
	}
	
	//*/
	return true;
}
