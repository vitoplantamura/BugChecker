#pragma once
#include "../bzscore/socket.h"
#include "../bzscore/atomic.h"
#include "../bzscore/sync.h"
#include "../bzscore/thread.h"

namespace BazisLib
{
	namespace Network
	{
		class BasicTCPServer
		{
		private:
			TCPListener m_Listener;
			AtomicUInt32 m_ActiveThreadCount;
			Event m_LastThreadDone;

		private:
			class WorkerThread : public BazisLib::AutoDeletingThread
			{
			private:
				BasicTCPServer *m_pServer;
				
			protected:
				virtual int ThreadBody() override;

			public:
				WorkerThread(BasicTCPServer *pServer) :
				  m_pServer(pServer)
				{
				}

			};

		protected:
			void WaitForWorkerThreads();

		public:
			BasicTCPServer();
			~BasicTCPServer();

			//! Starts the server. If a server was already stopped, it cannot be restarted.
			ActionStatus Start(unsigned PortNumber);

			//! Stops the server. Once it is stopped, it cannot be started any more.
			void Stop(bool WaitForWorkerThreads = true);

			bool Valid() {return m_Listener.Valid();}

			void WaitForTermination() {WaitForWorkerThreads();}

		protected:
			//! An overrideable connection handler that is called from a separate thread for each connection
			virtual void ConnectionHandler(TCPSocket &socket, const InternetAddress &addr)=0;
		};
	}
}