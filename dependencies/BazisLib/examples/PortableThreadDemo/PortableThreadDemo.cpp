#include "stdafx.h"
#include <bzscore/thread.h>
#include <bzscore/string.h>
#include <bzscore/sync.h>
#include <bzscore/datetime.h>
#include <queue>

/*
	PortableThreadDemo
	
	This sample demonstrates the use of thread and synchronization API of BazisLib.
	It contains configurations for building on Win32 and Posix (Linux & MacOS) targets.
	The sample creates 10 "work items" and puts them to the input queue where they are taken and "processed" by 2 worker threads.
	The main thread then collects the processed items from the result queue.
*/

template <class _Item> class SignaledQueue
{
private:
	std::queue<_Item> m_InnerQueue;
	BazisLib::Mutex m_Mutex;
	BazisLib::Semaphore m_Semaphore;
	bool m_Abort;

public:
	SignaledQueue()
		: m_Abort(false)
	{
	}

	void Enqueue(const _Item &item)
	{
		BazisLib::MutexLocker locker(m_Mutex);
		m_InnerQueue.push(item);
		m_Semaphore.Signal();
	}

	void Abort()
	{
		m_Abort = true;
		m_Semaphore.Signal();
	}

	bool WaitAndDequeue(_Item *pResult)
	{
		if (m_Abort)
			return false;

		m_Semaphore.Wait();
		if (m_Abort)
		{
			m_Semaphore.Signal();
			return false;
		}

		BazisLib::MutexLocker locker(m_Mutex);
		if (m_InnerQueue.empty())
		{
			m_Semaphore.Signal();
			return false;
		}

		*pResult = m_InnerQueue.front();
		m_InnerQueue.pop();
		return true;
	}
	
};

class QueueDemo
{
private:
	struct WorkItem
	{
		int Number;
		bool Processed;

		WorkItem(int num = -1)
			: Number(num)
			, Processed(false)
		{
		}
		
		void Process()
		{
			BazisLib::Thread::Sleep(200);
			Processed = true;
		}
	};

	SignaledQueue<WorkItem> m_InputQueue, m_ResultQueue;
	BazisLib::DateTime m_StartTime;

public:
	int WorkerThreadBody()
	{
		WorkItem item;
		while(m_InputQueue.WaitAndDequeue(&item))
		{
			item.Process();
			printf("[+%4d msec] Processed item %d\n", (int)m_StartTime.MillisecondsElapsed(), item.Number);
			m_ResultQueue.Enqueue(item);
		}
		return 0;
	}

	void RunTest()
	{
		m_StartTime = BazisLib::DateTime::Now();

		BazisLib::MemberThread thread1(this, &QueueDemo::WorkerThreadBody);
		BazisLib::MemberThread thread2(this, &QueueDemo::WorkerThreadBody);
		thread1.Start();
		thread2.Start();
		
		int itemCounter = 0;
		for (int i = 0; i < 10; i++)
			m_InputQueue.Enqueue(WorkItem(itemCounter++));

		for (int i = 0; i < 10; i++)
		{
			WorkItem result;
			if (!m_ResultQueue.WaitAndDequeue(&result))
				return;

			printf("[+%4d msec] Received item %d, processed = %d\n", (int)m_StartTime.MillisecondsElapsed(), result.Number, result.Processed);
		}

		m_InputQueue.Abort();
		thread1.Join();
		thread2.Join();
	}

};

int main(int argc, char* argv[])
{
	QueueDemo demo;
	demo.RunTest();
	printf("Press ENTER to exit...\n");
	getchar();
	return 0;
}

