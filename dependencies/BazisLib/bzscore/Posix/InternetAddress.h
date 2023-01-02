#pragma once
#ifdef WIN32
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

#ifdef BZSLIB_UNIXWORLD
#define IP_FROM_SOCKADDR(in) ((in).sin_addr.s_addr)
#else
#define IP_FROM_SOCKADDR(in) ((in).sin_addr.S_un.S_addr)
#endif

#include "../string.h"

namespace BazisLib
{
	namespace Network
	{
		namespace Posix
		{
			class IP4Address;
			class IP6Address;

			class InternetAddress
			{
			protected:
				union
				{
					sockaddr_in in4;
#ifndef BZSLIB_NO_IPV6
					sockaddr_in6 in6;
#endif
					sockaddr hdr;
				} m_Addr;

			public:
				InternetAddress()
				{
					memset(&m_Addr, 0, sizeof(m_Addr));
				}

				InternetAddress(const sockaddr_in &addr)
				{
					m_Addr.in4 = addr;
				}

				InternetAddress(const sockaddr &addr)
				{
					if (addr.sa_family == AF_INET)
						memcpy(&m_Addr.in4, &addr, sizeof(sockaddr_in));
#ifndef BZSLIB_NO_IPV6
					else if (addr.sa_family == AF_INET6)
						memcpy(&m_Addr.in6, &addr, sizeof(sockaddr_in6));
#endif
					else
						memset(&m_Addr, 0, sizeof(m_Addr));
				}

				operator const sockaddr_in *() const
				{
					return &m_Addr.in4;
				}

				bool IsIPv4() const
				{
					return m_Addr.hdr.sa_family == AF_INET;
				}

				bool operator==(const InternetAddress &addr) const
				{
					if (addr.m_Addr.hdr.sa_family != m_Addr.hdr.sa_family)
						return false;
					switch(m_Addr.hdr.sa_family)
					{
					case AF_INET:
						return !memcmp(&addr.m_Addr, &m_Addr, sizeof(sockaddr_in));
					case AF_INET6:
						return !memcmp(&addr.m_Addr, &m_Addr, sizeof(sockaddr_in6));
					default:
						return false;
					}
				}

#ifndef BZSLIB_NO_IPV6
				InternetAddress(const sockaddr_in6 &addr)
				{
					m_Addr.in6 = addr;
				}

				operator const sockaddr_in6 *() const
				{
					return &m_Addr.in6;
				}

				bool IsIPv6() const
				{
					return m_Addr.hdr.sa_family == AF_INET6;
				}
#endif

				operator const sockaddr *() const
				{
					return &m_Addr.hdr;
				}

				bool Valid() const
				{
					C_ASSERT(AF_UNSPEC == 0);
					return m_Addr.hdr.sa_family != AF_UNSPEC;
				}

				unsigned short GetPort() const
				{
					return ntohs(m_Addr.in4.sin_port);
				}

				InternetAddress &SetPort(unsigned short port)
				{
					m_Addr.in4.sin_port = htons(port);
					return *this;
				}

			public:
				size_t GetRawSize() const
				{
					switch(m_Addr.hdr.sa_family)
					{
					case AF_INET:
						return sizeof(sockaddr_in);
#ifndef BZSLIB_NO_IPV6
					case AF_INET6:
						return sizeof(sockaddr_in6);
#endif
					default:
						return 0;
					}
				}
			};

			class IP4Address : public InternetAddress
			{
			public:
				IP4Address(unsigned IP, unsigned short Port)
				{
					memset(&m_Addr, 0, sizeof(m_Addr));
#ifdef BZSLIB_KEXT
					m_Addr.in4.sin_len = sizeof(sockaddr_in);
#endif
					m_Addr.in4.sin_family = AF_INET;
					m_Addr.in4.sin_port = htons(Port);
					IP_FROM_SOCKADDR(m_Addr.in4) = IP;
				}

				IP4Address(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short Port)
				{
					memset(&m_Addr, 0, sizeof(m_Addr));
#ifdef BZSLIB_KEXT
					m_Addr.in4.sin_len = sizeof(sockaddr_in);
#endif
					m_Addr.in4.sin_family = AF_INET;
					m_Addr.in4.sin_port = htons(Port);
					IP_FROM_SOCKADDR(m_Addr.in4) = ((d << 24) | (c << 16) | (b << 8) | a);
				}

				IP4Address(const sockaddr_in &addr)
					: InternetAddress(addr)
				{
					ASSERT(m_Addr.in4.sin_family == AF_INET);
#ifdef BZSLIB_KEXT
					m_Addr.in4.sin_len = sizeof(sockaddr_in);
#endif
				}

				IP4Address(const InternetAddress &addr)
				{
					if (addr.IsIPv4())
						m_Addr.in4 = *(const sockaddr_in *)addr;
				}

				unsigned GetIP() const
				{
					return ntohl(IP_FROM_SOCKADDR(m_Addr.in4));
				}

				String ToString() const
				{
					unsigned IP = GetIP();

					return String::sFormat(_T("%d.%d.%d.%d:%d"), 
						IP & 0xFF,
						(IP >> 8) & 0xFF,
						(IP >> 16) & 0xFF,
						(IP >> 24) & 0xFF,
						GetPort());
				}

				bool Valid()
				{
					return IsIPv4();
				}
			};

#ifndef BZSLIB_NO_IPV6
			class IP6Address : public InternetAddress
			{
			public:
				IP6Address(const sockaddr_in6 &addr)
					: InternetAddress(addr)
				{
					ASSERT(m_Addr.in6.sin6_family == AF_INET6);
				}

				bool Valid()
				{
					return IsIPv6();
				}
			};
#endif
		}

		using Posix::InternetAddress;
		using Posix::IP4Address;
#ifndef BZSLIB_NO_IPV6
		using Posix::IP6Address;
#endif
	}
}
