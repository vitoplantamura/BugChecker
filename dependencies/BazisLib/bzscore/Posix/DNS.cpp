#include "stdafx.h"
#if (defined(BZSLIB_UNIXWORLD) && !defined(BZSLIB_KEXT)) || (defined (WIN32) && !defined(_NTDDK_))

#include "DNS.h"

#ifdef WIN32
#include "../Win32/socket.h"
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

using namespace BazisLib;
using namespace BazisLib::Network;

InternetAddress BazisLib::Network::Posix::DNS::GetHostByName( const char *pHostName )
{
#ifdef WIN32
	BazisLib::Win32::WinsockLoader winsock;
#endif

	const char *p = strchr(pHostName, ':');
	int port = 0;
	hostent *pHostEnt;
	if (!p)
		pHostEnt = gethostbyname(pHostName);
	else
	{
		pHostEnt = gethostbyname(DynamicStringA(pHostName, p - pHostName).c_str());
		port = atoi(p + 1);
	}

	if (!pHostEnt)
		return InternetAddress();

	char *pAddr0 = pHostEnt->h_addr_list[0];
	return IP4Address(((unsigned *)pAddr0)[0], (unsigned short)port);
}

std::vector<Network::InternetAddress> BazisLib::Network::Posix::DNS::GetAllHostAddresses( const char *pHostName, AddrTypeFilter filter /*= kIPvAll*/ )
{
	std::vector<Network::InternetAddress> result;

	addrinfo *pFirst = NULL;
	getaddrinfo(pHostName , NULL, NULL, &pFirst);
	if (!pFirst)
		return result;

	int cnt = 0;
	for (addrinfo *pNode = pFirst; pNode ; pNode = pNode->ai_next)
		cnt++;

	result.reserve(cnt);

	for (addrinfo *pNode = pFirst; pNode ; pNode = pNode->ai_next)
	{
		switch(pNode->ai_family)
		{
		case AF_INET:
			if (filter & kIPv4)
				result.push_back(*reinterpret_cast<sockaddr_in *>(pNode->ai_addr));
			break;
		case AF_INET6:
			if (filter & kIPv6)
				result.push_back(*reinterpret_cast<sockaddr_in6 *>(pNode->ai_addr));
			break;
		}
	}

	freeaddrinfo(pFirst);
	return result;
}
#endif