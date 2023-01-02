#include "stdafx.h"
#include "TestPrint.h"
#define BZSLIB_DISABLE_NO_SOCKET_ERROR
#include <bzscore/socket.h>

#if defined(BZSLIB_SOCKETS_AVAILABLE) && !defined(BZSLIB_KEXT)

using namespace BazisLib::Network;


bool TestDNS()
{
	InternetAddress addr = DNS::GetHostByName("localhost");
	TEST_ASSERT(addr.Valid());
	TEST_ASSERT(addr.IsIPv4());
	IP4Address addr4 = addr;
	TEST_ASSERT(addr4.GetIP() == 0x7F000001);	//127.0.0.1

	std::vector<InternetAddress> allAddr = DNS::GetAllHostAddresses("localhost");
	bool found = false;
	for (size_t i = 0; i < allAddr.size(); i++)
		if (allAddr[i] == addr)
			found = true;

	TEST_ASSERT(found);

	return true;
}


bool TestTCP()
{
	TCPSocket sock1, sock2;
	TCPListener listener;

	enum {kPort = 47162};
	TEST_ASSERT(listener.Listen(kPort).Successful());

	TEST_ASSERT(sock1.Connect(DNS::GetHostByName("localhost").SetPort(kPort)).Successful());
	TEST_ASSERT(listener.Accept(&sock2).Successful());

	char buf1[] = "12345678";
	char buf2[sizeof(buf1)];

	TEST_ASSERT(sock1.Send(buf1, sizeof(buf1)) == sizeof(buf1));

	enum {kPart1 = 4};

	TEST_ASSERT(sock2.Recv(buf2, kPart1, true) == kPart1);
	TEST_ASSERT(!memcmp(buf2, buf1, kPart1));
	TEST_ASSERT(sock2.Recv(buf2, sizeof(buf1) - kPart1, true) == (sizeof(buf1) - kPart1));
	
	TEST_ASSERT(!memcmp(buf2, buf1 + kPart1, sizeof(buf1) - kPart1));

	sock2.Close();
	TEST_ASSERT(sock1.Recv(buf1, 1) == 0);
	return true;
}


bool TestUDP()
{
	UDPSocket sock1, sock2;
	TEST_ASSERT(sock1.Bind().Successful());

	char buf1[] = "message";
	char buf2[sizeof(buf1) + 7];

	InternetAddress addr = DNS::GetHostByName("localhost").SetPort(sock1.GetLocalAddr().GetPort());

	TEST_ASSERT(sock2.Sendto(addr, buf1, sizeof(buf1)) == sizeof(buf1));
	TEST_ASSERT(sock1.Recvfrom(buf2, sizeof(buf2)) == sizeof(buf1));
	TEST_ASSERT(!memcmp(buf1, buf2, sizeof(buf1)));

	return true;
}

using namespace BazisLib;
bool TestSocketAPI()
{
	TestPrint("Testing socket API...\n");
	if (!TestDNS())
		return false;
	if (!TestTCP())
		return false;
	if (!TestUDP())
		return false;

	return true;
}

#else

bool TestSocketAPI()
{
	TestPrint("Sockets are not supported for this target. Skipping tests...\n");
	return true;
}

#endif
