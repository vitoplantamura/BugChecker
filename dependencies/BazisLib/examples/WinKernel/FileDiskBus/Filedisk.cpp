#include "stdafx.h"
#include "filedisk.h"

using namespace BazisLib;

FileDisk::FileDisk(ManagedPointer<AIFile> &pFile, ULONGLONG FileOffset, unsigned SectorSize, unsigned minMBCount) :
	m_pFile(pFile),
	m_FileOffset(FileOffset),
	m_SectorSize(SectorSize),
	m_MinMBCount(minMBCount)
{
	if (!m_pFile)
		return;
	m_SectorCount = m_pFile->GetSize() / SectorSize;
}

FileDisk::~FileDisk()
{
}

unsigned FileDisk::GetSectorSize()
{
	return m_SectorSize;
}

ULONGLONG FileDisk::GetSectorCount()
{
	return m_SectorCount;
}

unsigned FileDisk::Read(ULONGLONG ByteOffset, void *pBuffer, unsigned Length)
{
	if (!m_pFile || !pBuffer)
		return 0;
	m_pFile->Seek(m_FileOffset + ByteOffset, FileFlags::FileBegin);
	return m_pFile->Read(pBuffer, Length);
}

unsigned FileDisk::Write(ULONGLONG ByteOffset, const void *pBuffer, unsigned Length)
{
	if (!m_pFile || !pBuffer)
		return 0;
	LONGLONG soff = m_pFile->Seek(m_FileOffset + ByteOffset, FileFlags::FileBegin);
	return m_pFile->Write(pBuffer, Length);
}

void FileDisk::Dispose()
{
	m_pFile = NULL;
}

BazisLib::ActionStatus BazisLib::FileDisk::Initialize()
{
	if (m_pFile->GetSize() < (m_MinMBCount * 1024 * 1024))
	{
		DbgPrint("Preparing filedisk file...");
		BasicBuffer buf(1024 * 1024);
		buf.Fill();
		for(unsigned i = 0; i < m_MinMBCount; i++)
			m_pFile->Write(buf.GetData(), buf.GetAllocated());
		DbgPrint("done\n");
	}

	m_SectorCount = m_pFile->GetSize() / m_SectorSize;
	return MAKE_STATUS(Success);
}
