#pragma once
#include "cmndef.h"

namespace BazisLib
{
	//The xxxStream interfaces are obsolette. When possible, use AIFile interface and BasicFileBase class
	//to implement custom files. Using a limited number of methods in a single interface produces less
	//overhead (less space is required for vtable and vcast pointers), while unused and unimplemented
	//AIFile methods produce completely no overhead.
	enum {STREAM_SIZE_UNKNOWN = -1LL};

	class IInStream
	{
	public:
		virtual size_t Read(void *pBuffer, size_t size)=0;
		virtual LONGLONG GetSize()=0;
		virtual ~IInStream(){}
	};

	class IOutStream
	{
	public:
		virtual size_t Write(const void *pBuffer, size_t size)=0;
		virtual ~IOutStream(){}
	};

	class IPipe : public IInStream, public IOutStream
	{
	public:
		virtual bool SetTimeout(unsigned ReadTimeout, unsigned WriteTimeout)=0;
		virtual void Close()=0;
		virtual bool Valid()=0;
		virtual ~IPipe(){}
	};

}
