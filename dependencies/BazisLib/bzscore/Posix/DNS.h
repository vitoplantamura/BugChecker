#pragma once

#include "InternetAddress.h"
#include <vector>

namespace BazisLib
{
	namespace Network
	{
		namespace Posix
		{
			class DNS
			{
			public:
				static Network::InternetAddress GetHostByName(const char *pHostName);

				enum AddrTypeFilter {kIPv4 = 0x01, kIPv6 = 0x02, kIPvAll = kIPv4 | kIPv6};
				static std::vector<Network::InternetAddress> GetAllHostAddresses(const char *pHostName, AddrTypeFilter filter = kIPvAll);
			};
		}

		using Posix::DNS;
	}
}
