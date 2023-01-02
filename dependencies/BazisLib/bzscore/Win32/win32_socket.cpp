#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "socket.h"

using namespace BazisLib;
using namespace BazisLib::Win32;
using namespace BazisLib::Network;

BazisLib::AtomicInt32 WinsockLoader::m_WinsockLoaded = 0;

WinsockLoader::WinsockLoader()
{
	if (!m_WinsockLoaded.CompareAndExchange(0, 1))
		return;
	WSADATA data;
	WSAStartup(MAKEWORD(2,0), &data);
}

TCPSocket::TCPSocket(SOCKET hSocket)
	: m_Socket(hSocket)
{
#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
	m_pLogFile = NULL;
#endif
}

TCPSocket::TCPSocket(const char *pszAddr, unsigned port, const InternetAddress &addressToBind) :
	m_Socket(INVALID_SOCKET)
{
#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
	m_pLogFile = NULL;
#endif
	Connect(DNS::GetHostByName(pszAddr).SetPort(port), addressToBind);
}

TCPSocket::TCPSocket(const BazisLib::Network::InternetAddress &addr) :
	m_Socket(INVALID_SOCKET)
{
#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
	m_pLogFile = NULL;
#endif
	Connect(addr);
}

TCPSocket::TCPSocket(class TCPListener &listener) :
	m_Socket(listener.Accept())
{
#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
	m_pLogFile = NULL;
#endif
}

TCPSocket::~TCPSocket()
{
	Close();
#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
	delete m_pLogFile;
#endif
}

size_t TCPSocket::Send(const void *pBuffer, size_t length, int rawFlags)
{
	if ((m_Socket == INVALID_SOCKET) || !pBuffer)
		return 0;
#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
	if (m_pLogFile)
	{
		m_pLogFile->Write("W", 1);
		m_pLogFile->Write(&length, 4);
		if (length != -1)
			m_pLogFile->Write(pBuffer, length);
	}
#endif
	return send(m_Socket, (const char *)pBuffer, (int)length, rawFlags);
}

size_t TCPSocket::Recv(void *pBuffer, size_t length, bool waitUntilEntireBuffer, int rawFlags)
{
	if ((m_Socket == INVALID_SOCKET) || !pBuffer || !length)
		return 0;
	if (waitUntilEntireBuffer)
		rawFlags |= MSG_WAITALL;
	size_t done = recv(m_Socket, (char *)pBuffer, (int)length, rawFlags);
#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
	if (m_pLogFile)
	{
		m_pLogFile->Write("R", 1);
		m_pLogFile->Write(&done, 4);
		if (done != -1)
			m_pLogFile->Write(pBuffer, done);
	}
#endif
	return done;
}

ActionStatus TCPSocket::Connect(const BazisLib::Network::InternetAddress &address, const BazisLib::Network::InternetAddress &addressToBind)
{
	if (m_Socket == INVALID_SOCKET)
	{
		m_Socket = socket(AF_INET, SOCK_STREAM, 0);
		if (m_Socket == INVALID_SOCKET)
			return MAKE_STATUS(ActionStatus::FromWin32Error(WSAGetLastError()));
	}
	if (addressToBind.Valid())
	{
		int err = bind(m_Socket, addressToBind, (int)addressToBind.GetRawSize());
		if (err)
			return MAKE_STATUS(ActionStatus::FromWin32Error(err));
	}

	int err = connect(m_Socket, address, (int)address.GetRawSize());
	if (err)
		return MAKE_STATUS(ActionStatus::FromWin32Error(err));

	return MAKE_STATUS(Success);
}

BazisLib::ActionStatus BazisLib::Win32::TCPSocket::SetTimeouts( unsigned receiveTimeoutInMsec, unsigned sendTimeoutInMsec /*= 0*/ )
{
	if (m_Socket == INVALID_SOCKET)
		return MAKE_STATUS(NotInitialized);

	if (receiveTimeoutInMsec != -1)
	{
		int err = setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&receiveTimeoutInMsec, sizeof(unsigned));
		if (err)
			return MAKE_STATUS(ActionStatus::FromWin32Error(err));
	}
	if (sendTimeoutInMsec != -1)
	{
		int err = setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&sendTimeoutInMsec, sizeof(unsigned));
		if (err)
			return MAKE_STATUS(ActionStatus::FromWin32Error(err));
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
	if (getpeername(m_Socket, &rawaddr.hdr, &fromlen))
		return BazisLib::Network::InternetAddress();
	return BazisLib::Network::InternetAddress(rawaddr.hdr);
}

//----------------------------------------------------------------------------------------------------------------

TCPListener::TCPListener() :
	m_Socket(INVALID_SOCKET)
{
}

ActionStatus TCPListener::Listen(unsigned Port)
{
	m_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_Socket == INVALID_SOCKET)
		return MAKE_STATUS(ActionStatus::FromWin32Error(WSAGetLastError()));
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(Port);
	int err = bind(m_Socket, (sockaddr *)&addr, sizeof(addr));
	if (err)
		return MAKE_STATUS(ActionStatus::FromWin32Error(err));
	err = listen(m_Socket, SOMAXCONN);
	if (err)
		return MAKE_STATUS(ActionStatus::FromWin32Error(err));
	return MAKE_STATUS(Success);
}

TCPListener::~TCPListener()
{
	if (m_Socket != INVALID_SOCKET)
		closesocket(m_Socket);
}


BazisLib::ActionStatus BazisLib::Win32::TCPListener::Accept( TCPSocket *pSocket, Network::InternetAddress *pAddress /*= NULL*/ )
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

	SOCKET sock = accept(m_Socket, &rawaddr.hdr, &fromlen);
	if (sock == INVALID_SOCKET)
		return MAKE_STATUS(ActionStatus::FromWin32Error(WSAGetLastError()));
	if (pAddress)
		*pAddress = BazisLib::Network::InternetAddress(rawaddr.hdr);
	pSocket->AttachSocket(sock);
	return MAKE_STATUS(Success);
}

SOCKET TCPListener::Accept()
{
	return accept(m_Socket, NULL, 0);
}


bool TCPSocket::SetNoDelay( bool noDelay )
{
	int flag = noDelay;
	int result = setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	return !result;
}

bool TCPSocket::IsNoDelay()
{
	int flag = 0;
	int len = sizeof(int);
	int result = getsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, &len);
	if (!result || !len)
		return false;
	return flag != 0;
}


#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
void TCPSocket::StartLogging( const TCHAR *pLogFile )
{
	delete m_pLogFile;
	m_pLogFile = new BazisLib::File(pLogFile, FileModes::CreateOrTruncateRW);
}
#endif

#endif
