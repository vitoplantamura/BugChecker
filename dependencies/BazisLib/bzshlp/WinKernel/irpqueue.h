#pragma once

#include "../../bzscore/sync.h"

namespace BazisLib
{
	namespace DDK
	{
		/*! This class represents a thread-safe IRP queue. It is internally used by the Device class. Use the
			Device::EnqueuePacket() method to add an IRP to device queue.
			This class is used instead of native IoStartPacket()/StartIo() mechanism because the latter does not
			ensure that the IRP started with IoStartPacket() will be processed from the dispatcher thread, that
			can be vital in some cases (for example, in virtual disk drivers when FSD call in the same thread
			that dispatches a call from FSD may cause infrequent crashes caused by TLS-like IoSetTopLevelIrp().

			\remarks This class uses IRP::Tail::Overlay::DriverContext for storing pointers in the following way:
						[0] - Next queued IRP pointer
						[1] - IRP context. Usually contains the IncomingIrp flags.
			\remarks DO NOT create any instances of this class in paged memory. Always use new() operator.
			\remarks See comments in the beginning of IRP.H for use example.
		*/
		class IrpQueue
		{
		private:
			PIRP m_pFirstIrp;
			PIRP m_pLastIrp;

			bool m_bShutdown;
			bool m_bLastPacketDequeued;
			KSemaphore m_Semaphore;
			KSpinLock m_QueueSpinLock;

#ifdef _DEBUG
			int m_PacketCount;
#endif

		public:
			IrpQueue();
			~IrpQueue();

			/*! Adds a new packet to the end of the queue and signals the semaphore.
				\return The method returns \b false if the queue was shut down using Shutdown() method. In other case
						the method returns \b true.
			*/
			bool EnqueuePacket(PIRP pIrp, void *pContext);

			/*! Dequeues a packet from the queue. If the queue is empty, waits for a new packet to be enqueued.
				\return The method returns \b NULL only if the queue was shut down AND there are no more enqueued
						packets left. That way, shutdown operation does not cause any packet to be lost.
			*/
			PIRP DequeuePacket(void **ppContext);

			/*! Shuts down the IRP queue. A queue that is shut down fails to enqueue packets
			*/
			void Shutdown() {m_bShutdown = true; m_Semaphore.Signal();}

			bool IsShutDown() {return m_bShutdown;}

#ifdef _DEBUG
			int GetPacketCount() {return m_PacketCount;}
#endif

		public:
			void *operator new(size_t size)
			{
				return npagednew(size);
			}
		};
	}
}
