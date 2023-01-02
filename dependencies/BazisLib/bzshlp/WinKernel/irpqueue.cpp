#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "irpqueue.h"

using namespace BazisLib::DDK;

IrpQueue::IrpQueue() :
	m_Semaphore(0),
	m_bShutdown(false),
	m_bLastPacketDequeued(false),

	m_pFirstIrp(NULL),
	m_pLastIrp(NULL)
{
	ASSERT(MmIsNonPagedSystemAddressValid(this));
#ifdef _DEBUG
	m_PacketCount = 0;
#endif
}

IrpQueue::~IrpQueue()
{
}

bool IrpQueue::EnqueuePacket(PIRP pIrp, void *pContext)
{
#ifdef TRACK_QUEUE_STATE
	DbgPrint("[Q%08X]: Enqueueing packet: %08X...\n", this, pIrp);
#endif
	if (!pIrp)
		return false;
	if (m_bShutdown)
		return false;

	//Add IRP to queue
	m_QueueSpinLock.Lock();
	if (m_bShutdown || m_bLastPacketDequeued)
	{
		m_QueueSpinLock.Unlock();
		return false;
	}

	if (m_pLastIrp)
	{
		ASSERT(MmIsNonPagedSystemAddressValid(m_pLastIrp));
		m_pLastIrp->Tail.Overlay.DriverContext[0] = pIrp;
	}

#ifdef _DEBUG
	m_PacketCount++;
#endif

	pIrp->Tail.Overlay.DriverContext[0] = NULL;
	pIrp->Tail.Overlay.DriverContext[1] = pContext;
	m_pLastIrp = pIrp;
	if (!m_pFirstIrp)
		m_pFirstIrp = pIrp;
	m_QueueSpinLock.Unlock();

#ifdef TRACK_QUEUE_STATE
	DbgPrint("[Q%08X]: enq()-> %d packets in queue.\n", this, m_PacketCount);
#endif
	//Signal the semaphore
	m_Semaphore.Signal();
	return true;
}

PIRP IrpQueue::DequeuePacket(void **ppContext)
{
	if (m_bLastPacketDequeued)
		return NULL;
	if (m_Semaphore.WaitEx() != STATUS_WAIT_0)
		return NULL;
	PIRP pReturnedIrp;
	m_QueueSpinLock.Lock();
#ifdef _DEBUG
	m_PacketCount--;
#endif

	pReturnedIrp = m_pFirstIrp;
	if (m_pFirstIrp)
	{
		ASSERT(MmIsNonPagedSystemAddressValid(m_pFirstIrp));
		if (m_pFirstIrp == m_pLastIrp)
		{
			//Last IRP was dequeued. Clear first and last pointers. If the IRP's 'next' pointer contains a valid address,
			//somebody's altered our pointer and it is not valid.
			ASSERT(!m_pFirstIrp->Tail.Overlay.DriverContext[0]);
			m_pLastIrp = NULL;
			m_pFirstIrp = NULL;
		}
		else
		{
			m_pFirstIrp = (PIRP)m_pFirstIrp->Tail.Overlay.DriverContext[0];
			ASSERT(m_pFirstIrp);
			ASSERT(m_pLastIrp);
		}
	}
	else
	{
		ASSERT(!m_pLastIrp);
	}

	if (pReturnedIrp)
	{
		if (ppContext)
			*ppContext = pReturnedIrp->Tail.Overlay.DriverContext[1];
	}
	else
		m_bLastPacketDequeued = true;

	m_QueueSpinLock.Unlock();

#ifdef TRACK_QUEUE_STATE
	DbgPrint("[Q%08X]: dequeued %08X; %d packets left in queue.\n", this, pReturnedIrp, m_PacketCount);
#endif

	ASSERT(pReturnedIrp || m_bShutdown);
	return pReturnedIrp;
}
#endif