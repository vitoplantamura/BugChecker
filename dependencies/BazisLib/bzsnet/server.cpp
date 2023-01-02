#include "stdafx.h"
#include "server.h"

using namespace BazisLib;
using namespace BazisLib::Network;

BasicTCPServer::BasicTCPServer() :
	m_ActiveThreadCount(0U),
	m_LastThreadDone(true)
{
}

BasicTCPServer::~BasicTCPServer()
{
	WaitForWorkerThreads();
}

ActionStatus BasicTCPServer::Start(unsigned PortNumber)
{
	if (m_ActiveThreadCount)
		return MAKE_STATUS(InvalidState);
	ActionStatus st = m_Listener.Listen(PortNumber);
	if (!st.Successful())
		return st;
	m_LastThreadDone.Reset();
	m_ActiveThreadCount++;

	WorkerThread *pThread = new WorkerThread(this);
	if (!pThread)
		return MAKE_STATUS(NoMemory);

	if (!pThread->Start())
	{
		delete pThread;
		m_ActiveThreadCount--;
		return MAKE_STATUS(UnknownError);
	}
	return MAKE_STATUS(Success);
}

void BasicTCPServer::Stop(bool WaitForWorkerThreads)
{
	m_Listener.Abort();
	if (WaitForWorkerThreads)
		this->WaitForWorkerThreads();
}

int BasicTCPServer::WorkerThread::ThreadBody()
{
	InternetAddress addr;
	TCPSocket sock;
	ActionStatus st = m_pServer->m_Listener.Accept(&sock, &addr);
	if (st.Successful() && sock.Valid())
	{
		m_pServer->m_ActiveThreadCount++;
		WorkerThread *pThread = new WorkerThread(m_pServer);
		if (!pThread || !pThread->Start())
			delete pThread;
		m_pServer->ConnectionHandler(sock, addr);
	}
	if (!--m_pServer->m_ActiveThreadCount)
		m_pServer->m_LastThreadDone.Set();
	return 0;
}

void BasicTCPServer::WaitForWorkerThreads()
{
	m_LastThreadDone.Wait();
}
