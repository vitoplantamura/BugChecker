// CommonFunctionalityTests.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <bzsnet/BufferedSocket.h>
#include <queue>
#include <string>

using namespace BazisLib;
using namespace BazisLib::Network;
using namespace std;

class TestBufferedSocket : public BufferedSocketBase
{
private:
	queue<string> m_EnqueuedData;

protected:
	virtual size_t RawRecv(void *pBuffer, size_t length, bool waitUntilEntireBuffer = false)
	{
		if (m_EnqueuedData.empty())
			return 0;
		string str = m_EnqueuedData.front();
		m_EnqueuedData.pop();
		if (length < str.length())
			__asm int 3;	//Not expected
		if (waitUntilEntireBuffer)
			__asm int 3;	//Not supported

		memcpy(pBuffer, str.c_str(), str.length());
		return str.length();
	}

public:
	void EnqueueResponse(char *pResponse, int len = -1)
	{
		if (len == -1)
			len = strlen(pResponse);
		m_EnqueuedData.push(string(pResponse, len));
	}
};

#define TEST_ASSERT(x) {if (!(x)) {__debugbreak(); return false;}}

static bool BufferContainsString(const void *pBuf, size_t len, const char *str)
{
	int strLen = strlen(str);
	if (len != strLen)
		return false;
	return !memcmp(pBuf, str, strLen);
}

static bool DeterministicTest()
{
	TestBufferedSocket sock;
	sock.EnqueueResponse("123");
	sock.EnqueueResponse("4567");
	sock.EnqueueResponse("8\r");
	sock.EnqueueResponse("\r\nxxx\r\nzzz\r\nAB");

	char buf[512];
	int done = sock.RecvToMarker(buf, sizeof(buf), "\r\n", 2);
	TEST_ASSERT(BufferContainsString(buf, done, "12345678\r\r\n"));
	done = sock.RecvToMarker(buf, sizeof(buf), "\r\n", 2);
	TEST_ASSERT(BufferContainsString(buf, done, "xxx\r\n"));
	done = sock.RecvToMarker(buf, sizeof(buf), "\r\n", 2);
	TEST_ASSERT(BufferContainsString(buf, done, "zzz\r\n"));
	done = sock.Recv(buf, 1);
	TEST_ASSERT(done == 1 && buf[0] == 'A');
	done = sock.Recv(buf, 5);
	TEST_ASSERT(done == 1 && buf[0] == 'B');
	done = sock.Recv(buf, 7);
	TEST_ASSERT(done == 0);
	TEST_ASSERT(sock.GetLastRecvError() == BufferedSocketBase::kSocketClosed);
	return true;
}

static bool DeterministicFailTest()
{
	TestBufferedSocket sock;
	sock.EnqueueResponse("zzx123");
	sock.EnqueueResponse("4567");
	sock.EnqueueResponse("89");

	char buf[512];
	int done;

	done = sock.RecvToMarker(buf, sizeof(buf), "x", 1);
	TEST_ASSERT(done == 3);
	TEST_ASSERT(BufferContainsString(buf, done, "zzx"));

	done = sock.RecvToMarker(buf, 9, "x", 1);
	TEST_ASSERT(done == 0);
	TEST_ASSERT(sock.GetLastRecvError() == BufferedSocketBase::kBufferFull);
	done = sock.RecvToMarker(buf, 9, "x", 1);
	TEST_ASSERT(done == 0);
	TEST_ASSERT(sock.GetLastRecvError() == BufferedSocketBase::kBufferFull);

	done = sock.Recv(buf, sizeof(buf));
	TEST_ASSERT(done == 9);
	TEST_ASSERT(BufferContainsString(buf, done, "123456789"));

	return true;
}

static bool DeterministicPeekTest()
{
	TestBufferedSocket sock;
	sock.EnqueueResponse("zzx123");
	sock.EnqueueResponse("4567");

	size_t size;
	void *p = sock.Peek(0, &size);
	TEST_ASSERT(size == 0);
	p = sock.Peek(1024, &size);
	TEST_ASSERT(size == 6 && p != NULL);
	TEST_ASSERT(!memcmp(p, "zzx123", 6));

	p = sock.Peek(0, &size);
	TEST_ASSERT(size == 6 && p != NULL);
	TEST_ASSERT(!memcmp(p, "zzx123", 6));

	sock.Discard(p, 2);

	p = sock.Peek(0, &size);
	TEST_ASSERT(size == 4 && p != NULL);
	TEST_ASSERT(!memcmp(p, "x123", 4));

	p = sock.Peek(1024, &size);
	TEST_ASSERT(size == 8 && p != NULL);
	TEST_ASSERT(!memcmp(p, "x1234567", 4));

	sock.Discard(p, 1);
	p = sock.Peek(1024, &size);
	TEST_ASSERT(size == 7 && p != NULL);
	TEST_ASSERT(!memcmp(p, "1234567", 4));

	return true;
}

static bool RandomizedTest(int iterations = 10000)
{
	char szData[] = "this is line 1\r\nthis is line 2\r\r\n this is the end";
	srand(123);

	for (int iter = 0; iter < iterations; iter++)
//	for (volatile int iter = 0; ; iter++)
	{
		TestBufferedSocket sock;
		for (int i = 0; i < (sizeof(szData) - 1);)
		{
			int rem = sizeof(szData) - 1 - i;
			int todo  = (rem == 1) ? 1 : (1 + rand() % (rem - 1));
			sock.EnqueueResponse(szData + i, todo);
			i += todo;
		}

		char buf[512];
		int done = sock.RecvToMarker(buf, sizeof(buf), "\r\n", 2);
		TEST_ASSERT(BufferContainsString(buf, done, "this is line 1\r\n"));
		done = sock.RecvToMarker(buf, sizeof(buf), "\r\n", 2);
		TEST_ASSERT(BufferContainsString(buf, done, "this is line 2\r\r\n"));
		done = sock.RecvToMarker(buf, sizeof(buf), "\r\n", 2);
		TEST_ASSERT(done == 0);
	}

	return true;
}

#include <vector>

static bool ComplexRandomizedTest(int iterations = 100000)
{
	srand(123);

	for (int iter = 0; iter < iterations; iter++)
//	for (volatile ULONGLONG iter = 0; ; iter++)
	{
		int lines = 3 + rand() % 10;
		string allData;
		vector<string> allLines;

		for (int i = 0; i < lines; i++)
		{
			int len = rand() % 10;
			string line;
			for (int j = 0; j < len; j++)
				line.append(1, 'a' + (rand() % ('z' - 'a')));

			allLines.push_back(line);
			allData += line + "\r\n";
		}

		TestBufferedSocket sock;
		for (int i = 0; i < allData.length();)
		{
			int rem = allData.length() - i;
			int todo  = (rem == 1) ? 1 : (1 + rand() % (rem - 1));
			sock.EnqueueResponse((char *)allData.c_str() + i, todo);
			i += todo;
		}

		char buf[512];
		for (int i = 0; i < lines; i++)
		{
			int done = sock.RecvToMarker(buf, sizeof(buf), "\r\n", 2);
			TEST_ASSERT(BufferContainsString(buf, done, (allLines[i] + "\r\n").c_str()));
		}
		int done = sock.RecvToMarker(buf, sizeof(buf), "\r\n", 2);
		TEST_ASSERT(done == 0);
	}


	return true;
}

bool TestBufferedSockets()
{
	if (!DeterministicTest())
		return false;
	if (!DeterministicFailTest())
		return false;
	if (!DeterministicPeekTest())
		return false;
	if (!RandomizedTest())
		return false;
	if (!ComplexRandomizedTest())
		return false;
	return true;
}

