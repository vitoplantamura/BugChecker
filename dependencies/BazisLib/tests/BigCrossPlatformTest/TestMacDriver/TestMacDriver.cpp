#include "TestMacDriver.h"
#include <IOKit/IOLib.h>

#include <bzscore/sync.h>
#include <bzscore/status.h>

// This required macro defines the class's constructor, destructor,
// and several other methods I/O Kit requires.
OSDefineMetaClassAndStructors(com_sysprogs_TestMacDriver, IOService)

bool AutoTestEntry(bool runTestsCausingMemoryLeaks, const char *pTempDir);

void TestPrint(const char *pFormat, ...)
{
	va_list lst;
	va_start(lst, pFormat);
	vprintf(pFormat, lst);
	va_end(lst);
} 

void sWorkerThreadBody( void *pArg, wait_result_t )
{
	IOLog("Starting tests...\n");
	bool result = AutoTestEntry(false, "/tmp");
	IOLog("Tests %s...\n", result ? "succeeded" : "failed");

	thread_terminate(current_thread());
}

bool com_sysprogs_TestMacDriver::init( OSDictionary *dictionary /*= 0*/ )
{
 	thread_t _WorkerThread;
 	kernel_thread_start(sWorkerThreadBody, this, &_WorkerThread);
 	thread_deallocate(_WorkerThread);

	return super::init(dictionary);
}

void com_sysprogs_TestMacDriver::free( void )
{
	super::free();
}

IOService * com_sysprogs_TestMacDriver::probe( IOService *provider, SInt32 *score )
{
	return super::probe(provider, score);
}

bool com_sysprogs_TestMacDriver::start( IOService *provider )
{
	if (!super::start(provider))
		return false;

	registerService(kIOServiceSynchronous);
	return true;
}

void com_sysprogs_TestMacDriver::stop( IOService *provider )
{
	super::stop(provider);
}
