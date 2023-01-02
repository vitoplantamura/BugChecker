#pragma once

#include "../cmndef.h"

#include "../assert.h"
#include "../status.h"

#ifndef _WINSOCKAPI_
#error Please include <winsock.h> before including socket.h
#endif

#pragma comment (lib, "WS2_32")
#define BZSLIB_PLATFORM_SPECIFIC_SOCKETS_DEFINED
#include "../Posix/DNS.h"
#include "../atomic.h"

#ifdef _DEBUG
#define BZSLIB_SOCKET_LOGGING_SUPPORT
#endif

#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
#include "../file.h"
#endif


namespace BazisLib
{
	namespace Win32
	{
		class WinsockLoader
		{
		private:
			static AtomicInt32 m_WinsockLoaded;
		public:
			WinsockLoader();
		};


		class TCPSocket : WinsockLoader
		{
		protected:
			SOCKET m_Socket;
#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
			BazisLib::File *m_pLogFile;
#endif

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
					closesocket(m_Socket);
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
			void Close() {closesocket(m_Socket); m_Socket = INVALID_SOCKET;}

			size_t Send(const void *pBuffer, size_t length, int rawFlags = 0);
			size_t Recv(void *pBuffer, size_t length, bool waitUntilEntireBuffer = false, int rawFlags = 0);
			
			bool SetNoDelay(bool noDelay);
			bool IsNoDelay();

			BazisLib::Network::InternetAddress GetRemoteAddress();
		};

		class TCPListener : public WinsockLoader
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
					closesocket(m_Socket);
				m_Socket = INVALID_SOCKET;
			}

			TCPListener();
			~TCPListener();

			ActionStatus Accept(TCPSocket *pSocket, Network::InternetAddress *pAddress = NULL);
			bool Valid() const {return (m_Socket != INVALID_SOCKET);}
		};

		class UDPSocket : public WinsockLoader
		{
		private:
			SOCKET m_Socket;

		public:
			UDPSocket(SOCKET hSocket = INVALID_SOCKET) : m_Socket(hSocket) {}

			~UDPSocket()
			{
				if (m_Socket != INVALID_SOCKET)
					closesocket(m_Socket);
			}

			ActionStatus Bind(unsigned Port = 0, unsigned IP = 0)
			{
				if (m_Socket == INVALID_SOCKET)
					m_Socket = socket(AF_INET, SOCK_DGRAM, 0);

				sockaddr_in sin = {0,};
				sin.sin_family = AF_INET;
				sin.sin_port = htons(Port);
				sin.sin_addr.s_addr = IP;
				if (!bind(m_Socket, (sockaddr *)&sin, sizeof(sin)))
					return MAKE_STATUS(Success);
				else
					return MAKE_STATUS(ActionStatus::FromLastError(UnknownError));
			}

			ActionStatus Bind(const BazisLib::Network::InternetAddress &addr)
			{
				const sockaddr *pRawAddr = addr;

				if (m_Socket == INVALID_SOCKET)
					m_Socket = socket(pRawAddr->sa_family, SOCK_DGRAM, 0);

				if (!bind(m_Socket, addr, (int)addr.GetRawSize()))
					return MAKE_STATUS(Success);
				else
					return MAKE_STATUS(ActionStatus::FailFromLastError());
			}

			size_t Sendto(const BazisLib::Network::InternetAddress &addr, const void *pBuffer, size_t size)
			{
				if (m_Socket == INVALID_SOCKET)
					m_Socket = socket(((const sockaddr *)addr)->sa_family, SOCK_DGRAM, 0);

				return sendto(m_Socket, (const char *)pBuffer, (int)size, 0, (const sockaddr *)addr, (int)addr.GetRawSize());
			}

			size_t Recvfrom(const void *pBuffer, size_t size, BazisLib::Network::InternetAddress *pAddr = NULL)
			{
				union
				{
					sockaddr_in in4;
					sockaddr_in6 in6;
					sockaddr hdr;
				} rawaddr;

				int fromlen = sizeof(rawaddr);
				size_t ret = recvfrom(m_Socket, (char *)pBuffer, (int)size, 0, &rawaddr.hdr, &fromlen);
				if (pAddr)
					*pAddr = rawaddr.hdr;
				return ret;
			}

			bool Valid()
			{
				return m_Socket != INVALID_SOCKET;
			}

			BazisLib::Network::InternetAddress GetLocalAddr()
			{
				union
				{
					sockaddr_in in4;
					sockaddr_in6 in6;
					sockaddr hdr;
				} rawaddr;

				int fromlen = sizeof(rawaddr);
				if (getsockname(m_Socket, &rawaddr.hdr, &fromlen))
					return BazisLib::Network::InternetAddress();
				return rawaddr.hdr;
			}
		};

	}

	namespace Network
	{
		using BazisLib::Win32::TCPSocket;
		using BazisLib::Win32::UDPSocket;
		using BazisLib::Win32::TCPListener;
	}
}