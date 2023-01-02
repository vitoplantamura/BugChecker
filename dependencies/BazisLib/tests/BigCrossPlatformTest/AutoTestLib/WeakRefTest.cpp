#include "stdafx.h"
#include <bzscore/objmgr.h>
#include "TestPrint.h"

using namespace BazisLib;

class AutoObj1 : AUTO_OBJECT
{
public:
	void Test(){}
};


bool TestWeakLinks()
{
	ManagedPointer<AutoObj1> pObj = new AutoObj1();

	WeakManagedPointer<AutoObj1> pWeak = pObj;

	ManagedPointer<AutoObj1> pObj2 = pWeak;
	if (pObj2 != pObj)
	{
		TestPrint("Opening weak reference failed!\n");
		return false;
	}
	
	pObj = NULL;
	pObj2 = NULL;

	pObj2 = pWeak;
	if (pObj2)
	{
		TestPrint("Weak-referenced object was not deleted!\n");
		return false;
	}

	pWeak = NULL;

	pObj = new AutoObj1();
	pWeak = pObj;
	WeakManagedPointer<AutoObj1> pWeak2 = pWeak;
	pWeak = NULL;
	pWeak2 = NULL;
	pObj = NULL;

	pObj = new AutoObj1();
	pWeak = pObj;
	pWeak = NULL;
	pWeak = pObj;
	pWeak2 = pWeak;
	pWeak = NULL;
	pObj2 = pWeak2;

	if (pObj2 != pObj)
	{
		TestPrint("Opening weak reference failed!\n");
		return false;
	}

	return true;
}