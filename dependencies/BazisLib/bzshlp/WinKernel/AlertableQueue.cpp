#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL

#include "AlertableQueue.h"

using namespace BazisLib::DDK;

AlertableQueue::~AlertableQueue()
{
	//This auto-object is destroyed when no other references are available and thus no other method can be called
	while (!m_Queue.empty())
	{
		delete m_Queue.front();
		m_Queue.pop();
	}
}

NTSTATUS BazisLib::DDK::AlertableQueue::WaitAndDequeue( Item **ppItem, ULONG Timeout)
{
	NTSTATUS status;
	if (Timeout == ((ULONG)-1L))
		status = m_Semaphore.WaitEx(UserRequest, UserMode);
	else
		status = m_Semaphore.WaitWithTimeoutEx(((ULONGLONG)Timeout) * 10000, UserRequest, UserMode);

	if (status == STATUS_SUCCESS)
	{
		if (_Shutdown)
			return STATUS_CANCELLED;

		FastMutexLocker lck(m_QueueMutex);
		ASSERT(!m_Queue.empty());
		*ppItem = m_Queue.front();
		m_Queue.pop();
		return STATUS_SUCCESS;
	}
	ASSERT(status == STATUS_USER_APC);
	return status;
}
#endif