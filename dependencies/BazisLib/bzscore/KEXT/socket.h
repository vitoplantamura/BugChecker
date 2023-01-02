#pragma once
#include <sys/kpi_socket.h>
#include "../Posix/InternetAddress.h"
#include "../status.h"

namespace BazisLib
{
	namespace KEXT
	{
		typedef socket_t SOCKET;
#define INVALID_SOCKET 0

		class TCPSocket
		{
		public:

		protected:
			SOCKET m_Socket;

		private:
			TCPSocket(const TCPSocket &socket)
			{
				//Socket instance cannot be copied
				ASSERT(FALSE);
			}

			void operator=(const TCPSocket &socket)
			{
				//Socket instance cannot be copied
				ASSERT(FALSE);
			}

			friend class TCPListener;

			void AttachSocket(SOCKET socket)
			{
				if (m_Socket != INVALID_SOCKET)
					sock_close(m_Socket);
				m_Socket = socket;
			}

		public:
			SOCKET GetSocketForSingleUse() const {return m_Socket;}
			SOCKET DetachSocket()
			{
				SOCKET sock = m_Socket;
				m_Socket = INVALID_SOCKET;
				return sock;
			}

		public:
			TCPSocket(SOCKET hSocket = INVALID_SOCKET);

			TCPSocket(const char *pszAddr, unsigned port, const BazisLib::Network::InternetAddress &addressToBind = BazisLib::Network::InternetAddress());
			TCPSocket(const BazisLib::Network::InternetAddress &addr);
			TCPSocket(class TCPListener &listener);
			~TCPSocket();

#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
			void StartLogging(const TCHAR *pLogFile);
#endif

			ActionStatus Connect(const BazisLib::Network::InternetAddress &address, const BazisLib::Network::InternetAddress &addressToBind = BazisLib::Network::InternetAddress());
			ActionStatus SetTimeouts(unsigned receiveTimeoutInMsec, unsigned sendTimeoutInMsec = 0);

			bool Valid() const {return (m_Socket != INVALID_SOCKET);}
			void Close() {sock_close(m_Socket); m_Socket = INVALID_SOCKET;}

			size_t Send(const void *pBuffer, size_t length, int rawFlags = 0);
			size_t Recv(void *pBuffer, size_t length, bool waitUntilEntireBuffer = false, int rawFlags = 0);

			BazisLib::Network::InternetAddress GetRemoteAddress();
		};

		class TCPListener
		{
		private:
			SOCKET m_Socket;

			TCPListener(const TCPListener &socket)
			{
				//Listener instance cannot be copied
				ASSERT(FALSE);
			}

			void operator=(const TCPListener &socket)
			{
				//Listener instance cannot be copied
				ASSERT(FALSE);
			}

		private:
			SOCKET Accept();
			friend class TCPSocket;

		public:
			SOCKET GetSocketForSingleUse() const {return m_Socket;}
			SOCKET DetachSocket()
			{
				SOCKET sock = m_Socket;
				m_Socket = INVALID_SOCKET;
				return sock;
			}

		public:
			//! Starts listening for incoming connections, but does not wait for anything
			ActionStatus Listen(unsigned Port);
			ActionStatus Listen(const Network::InternetAddress &bindAddr);

			void Abort()
			{
				if (m_Socket != INVALID_SOCKET)
					sock_close(m_Socket);
				m_Socket = INVALID_SOCKET;
			}

			TCPListener();
			~TCPListener();

			ActionStatus Accept(TCPSocket *pSocket, Network::InternetAddress *pAddress = NULL);
			bool Valid() const {return (m_Socket != INVALID_SOCKET);}
			void Close() {sock_close(m_Socket); m_Socket = INVALID_SOCKET;}
		};
	}

	namespace Network
	{
		using BazisLib::KEXT::TCPSocket;
	}
}