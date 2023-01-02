#include "stdafx.h"
#ifdef BZSLIB_KEXT
#include "socket.h"

using namespace BazisLib;
using namespace BazisLib::KEXT;

TCPSocket::TCPSocket( SOCKET hSocket /*= INVALID_SOCKET*/ )
	: m_Socket(hSocket)
{
}

TCPSocket::TCPSocket(const BazisLib::Network::InternetAddress &addr) 
	: m_Socket(INVALID_SOCKET)
{
	Connect(addr);
}

TCPSocket::~TCPSocket()
{
	if (m_Socket != INVALID_SOCKET)
		sock_close(m_Socket);
}

size_t TCPSocket::Send(const void *pBuffer, size_t length, int rawFlags)
{
	if ((m_Socket == INVALID_SOCKET) || !pBuffer)
		return 0;
	size_t done = 0;
	iovec vec = {(void *)pBuffer, length};
	msghdr hdr = {0,};

	hdr.msg_iovlen = 1;
	hdr.msg_iov = &vec;

	sock_send(m_Socket, &hdr, rawFlags, &done);
	return done;
}

size_t TCPSocket::Recv(void *pBuffer, size_t length, bool waitUntilEntireBuffer, int rawFlags)
{
	if ((m_Socket == INVALID_SOCKET) || !pBuffer || !length)
		return 0;
	if (waitUntilEntireBuffer)
		rawFlags |= MSG_WAITALL;

	size_t done = 0;
	iovec vec = {pBuffer, length};
	msghdr hdr = {0,};

	hdr.msg_iovlen = 1;
	hdr.msg_iov = &vec;

	sock_receive(m_Socket, &hdr, rawFlags, &done);
	return done;
}

ActionStatus TCPSocket::Connect(const BazisLib::Network::InternetAddress &address, const BazisLib::Network::InternetAddress &addressToBind)
{
	if (m_Socket == INVALID_SOCKET)
	{
		int err = sock_socket(AF_INET, SOCK_STREAM, 0, NULL, 0, &m_Socket);
		if (m_Socket == INVALID_SOCKET)
			return MAKE_STATUS(ActionStatus::FromUnixError(err));
	}
	if (addressToBind.Valid())
	{
		int err = sock_bind(m_Socket, addressToBind);
		if (err)
			return MAKE_STATUS(ActionStatus::FromUnixError(err));
	}

	int err = sock_connect(m_Socket, address, 0);
	if (err)
		return MAKE_STATUS(ActionStatus::FromUnixError(err));

	return MAKE_STATUS(Success);
}

BazisLib::ActionStatus BazisLib::KEXT::TCPSocket::SetTimeouts( unsigned receiveTimeoutInMsec, unsigned sendTimeoutInMsec /*= 0*/ )
{
	if (m_Socket == INVALID_SOCKET)
		return MAKE_STATUS(NotInitialized);

	if (receiveTimeoutInMsec != -1)
	{
		int err = sock_setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&receiveTimeoutInMsec, sizeof(unsigned));
		if (err)
			return MAKE_STATUS(ActionStatus::FromUnixError(err));
	}
	if (sendTimeoutInMsec != -1)
	{
		int err = sock_setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&sendTimeoutInMsec, sizeof(unsigned));
		if (err)
			return MAKE_STATUS(ActionStatus::FromUnixError(err));
	}
	return MAKE_STATUS(Success);
}

BazisLib::Network::InternetAddress TCPSocket::GetRemoteAddress()
{
	if (!Valid())
		return BazisLib::Network::InternetAddress();
	union
	{
		sockaddr_in in4;
		sockaddr_in6 in6;
		sockaddr hdr;
	} rawaddr;

	int fromlen = sizeof(rawaddr);
	if (sock_getpeername(m_Socket, &rawaddr.hdr, fromlen))
		return BazisLib::Network::InternetAddress();
	return BazisLib::Network::InternetAddress(rawaddr.hdr);
}


//----------------------------------------------------------------------------------------------------------------

TCPListener::TCPListener()
	: m_Socket(INVALID_SOCKET)
{
}

ActionStatus TCPListener::Listen(unsigned Port)
{
	int err = sock_socket(AF_INET, SOCK_STREAM, 0, NULL, 0, &m_Socket);
	if (m_Socket == INVALID_SOCKET)
		return MAKE_STATUS(ActionStatus::FromUnixError(err));

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_len = sizeof(sockaddr_in);
	addr.sin_port = htons(Port);
	err = sock_bind(m_Socket, (sockaddr *)&addr);
	if (err)
		return MAKE_STATUS(ActionStatus::FromUnixError(err));
	err = sock_listen(m_Socket, SOMAXCONN);
	if (err)
		return MAKE_STATUS(ActionStatus::FromUnixError(err));
	return MAKE_STATUS(Success);
}

TCPListener::~TCPListener()
{
	if (m_Socket != INVALID_SOCKET)
		sock_close(m_Socket);
}


BazisLib::ActionStatus TCPListener::Accept( TCPSocket *pSocket, Network::InternetAddress *pAddress /*= NULL*/ )
{
	if (!pSocket)
		return MAKE_STATUS(InvalidParameter);

	union
	{
		sockaddr_in in4;
		sockaddr_in6 in6;
		sockaddr hdr;
	} rawaddr;

	int fromlen = sizeof(rawaddr);

	SOCKET sock = INVALID_SOCKET;
	int err = sock_accept(m_Socket, &rawaddr.hdr, fromlen,0, NULL, 0, &sock);
	if (sock == INVALID_SOCKET)
		return MAKE_STATUS(ActionStatus::FromUnixError(err));
	if (pAddress)
		*pAddress = BazisLib::Network::InternetAddress(rawaddr.hdr);
	pSocket->AttachSocket(sock);
	return MAKE_STATUS(Success);
}

SOCKET TCPListener::Accept()
{
	SOCKET sock;
	if (sock_accept(m_Socket, NULL, 0, 0, NULL, 0, &sock) != 0)
		return INVALID_SOCKET;
	else
		return sock;
}


#endif