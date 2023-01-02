#pragma once
#include <IOKit/IOService.h>
#include <kern/thread.h>

class com_sysprogs_TestMacDriver : public IOService
{
	OSDeclareDefaultStructors(com_sysprogs_TestMacDriver);
private:
	typedef IOService super;

public:
	virtual bool init(OSDictionary *dictionary = 0);
	virtual void free(void);
	virtual IOService *probe(IOService *provider, SInt32 *score);
	virtual bool start(IOService *provider);
	virtual void stop(IOService *provider);
};