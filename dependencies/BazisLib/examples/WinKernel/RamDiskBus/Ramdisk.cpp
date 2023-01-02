#include "stdafx.h"
#include "ramdisk.h"

using namespace BazisLib;

enum { kBufSize = 1024 * 1024 };

RamDisk::RamDisk(unsigned MegabyteCount)
{
	__debugbreak();
	int buffers = MegabyteCount / (kBufSize / 1024 / 1024);
	for (int i = 0; i < buffers; i++)
	{
		PHYSICAL_ADDRESS start, end;
		start.QuadPart = 0;
		end.QuadPart = -1;
		char *p = (char *)MmAllocateContiguousMemorySpecifyCache(kBufSize, start, end, start, MmCached);
		if (!p)
			break;
		m_Buffers.push_back(p);
	}

	buffers = m_Buffers.size();
	m_TotalSize = buffers * (ULONGLONG)kBufSize;
	m_SectorCount = m_TotalSize  / RAMDISK_SECTOR_SIZE;
}

RamDisk::~RamDisk()
{
	for (char *p : m_Buffers)
		MmFreeContiguousMemory(p);
}

ULONGLONG RamDisk::GetSectorCount()
{
	return m_SectorCount;
}

unsigned RamDisk::Read(ULONGLONG ByteOffset, void *pBuffer, unsigned Length)
{
	unsigned done = 0;
	while (done < Length)
	{
		unsigned cdone = DoTransfer(ByteOffset + done, (char*)pBuffer + done, Length - done, true);
		if (!cdone)
			break;
		done += cdone;
	}

	return done;
}

unsigned RamDisk::Write(ULONGLONG ByteOffset, const void *pBuffer, unsigned Length)
{
	unsigned done = 0;
	while (done < Length)
	{
		unsigned cdone = DoTransfer(ByteOffset + done, (char*)pBuffer + done, Length - done, false);
		if (!cdone)
			break;
		done += cdone;
	}

	return done;
}

unsigned BazisLib::RamDisk::DoTransfer(ULONGLONG ByteOffset, void *pBuffer, unsigned Length, bool read)
{
	unsigned bufNum = (unsigned)(ByteOffset / kBufSize);
	if (bufNum >= m_Buffers.size() || !m_Buffers[bufNum])
		return 0;
	unsigned bufOff = (ByteOffset % kBufSize);

	unsigned todo = min(Length, kBufSize - bufOff);
	if (!read)
		memcpy(m_Buffers[bufNum] + bufOff, pBuffer, todo);
	else
		memcpy(pBuffer, m_Buffers[bufNum] + bufOff, todo);
	return todo;
}

