#pragma once
#include <queue>
#include <BazisLib/bzscore/sync.h>
#include <BazisLib/bzscore/objmgr.h>

namespace BazisLib
{
	namespace DDK
	{
		//! A queue that owns enqueued items and frees them when aborted
		class AlertableQueue
		{
		public:
			class Item
			{
			public:
				virtual ~Item(){}
			};

		private:
			Semaphore m_Semaphore;
			
			std::queue<Item *> m_Queue;
			FastMutex m_QueueMutex;
			bool _Shutdown;

		private:
			//! Enqueues the item or destroys it if the queue is already shut down
			void DoEnqueueItem(Item *pItem);

		public:
			AlertableQueue()
				: _Shutdown(false)
			{
			}

			~AlertableQueue();

			void EnqueueItem(Item *pItem)
			{
				if (!pItem)
					return;
				FastMutexLocker lck(m_QueueMutex);
				m_Queue.push(pItem);
				m_Semaphore.Signal();
			}

			//! Returns one of 3: STATUS_SUCCESS, STATUS_USER_APC, STATUS_CANCELLED
			NTSTATUS WaitAndDequeue(Item **ppItem, ULONG Timeout = (ULONG)-1L);

			void Shutdown()
			{
				_Shutdown = true;
				m_Semaphore.Signal();
			}
		};

		template <class _Item> class AlertableQueueEx : public AlertableQueue
		{
		public:
			void EnqeueItem(_Item *pItem)
			{
				__super::EnqueueItem(pItem);
			}

			NTSTATUS WaitAndDequeue(_Item **ppItem, ULONG Timeout = (ULONG)-1L)
			{
				Item *p = NULL;
				NTSTATUS st = __super::WaitAndDequeue(&p, Timeout);
				*ppItem = static_cast<_Item *>(p);
				return st;
			}
		};
	}
}