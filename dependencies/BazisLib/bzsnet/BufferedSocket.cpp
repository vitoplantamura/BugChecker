#include "StdAfx.h"
#include "BufferedSocket.h"


size_t BazisLib::Network::BufferedSocketBase::Recv( void *pBuffer, size_t length, bool waitUntilEntireBuffer /*= false*/ )
{
	size_t available = m_Buffer.GetSize() - m_BufferOffset;
	char *pData = (char *)m_Buffer.GetData();
	size_t todoFromBuffer = 0;

	//First, try to read data from the buffer
	if (available)
	{
		todoFromBuffer = min(available, length);
		memcpy(pBuffer, pData + m_BufferOffset, todoFromBuffer);
		m_BufferOffset += todoFromBuffer;
		if (!waitUntilEntireBuffer || (todoFromBuffer == length))
			return todoFromBuffer;

		length -= todoFromBuffer;
		pBuffer = ((char *)pBuffer) + todoFromBuffer;
	}

	//At this point the internal buffer should be empty.
	//We got here either because the buffer was originally empty, or because waitUntilEntireBuffer is set and we have more space in the buffer.
	//We simply call the RawRecv() to receive the deta to the rest of the buffer.
	ASSERT(m_BufferOffset == m_Buffer.GetSize());
	m_BufferOffset = 0;
	m_Buffer.SetSize(0);

	size_t rawDone = RawRecv(pBuffer, length, waitUntilEntireBuffer);
	if (rawDone == 0)
		m_LastRecvError = kSocketClosed;
	else if (rawDone == -1)
		m_LastRecvError = kInternalError;

	if (!todoFromBuffer)
		return rawDone;

	if (rawDone == -1 || !rawDone)
		return todoFromBuffer;

	return todoFromBuffer + rawDone;
}

void *BazisLib::Network::BufferedSocketBase::PeekRel( size_t increment, OUT size_t *pTotalSize, bool waitForAllData )
{
	ASSERT(m_BufferOffset <= m_Buffer.GetSize());
	size_t available = m_Buffer.GetSize() - m_BufferOffset;
	if (increment)
	{

		//Align the data to the beginning of the buffer if needed
		if (m_BufferOffset)
		{
			char *p = (char *)m_Buffer.GetData();
			memmove(p, p + m_BufferOffset, available);
			m_Buffer.SetSize(available);
			m_BufferOffset = 0;
		}

		if (!m_Buffer.EnsureSize(available + increment))
			return NULL;

		size_t done = RawRecv(m_Buffer.GetData(available), increment, waitForAllData);
		if(!done)
			m_LastRecvError = kSocketClosed;
		else if (done == -1)
			m_LastRecvError = kInternalError, done = 0;

		available += done;
		m_Buffer.SetSize(available);
	}

	*pTotalSize = available;
	return m_Buffer.GetData(m_BufferOffset);
}


void *BazisLib::Network::BufferedSocketBase::PeekAbs(size_t minimumDataLength, OUT size_t *pTotalSize)
{
	size_t available = m_Buffer.GetSize() - m_BufferOffset;
	if (available >= minimumDataLength)
		return PeekRel(0, pTotalSize);
	else
		return PeekRel(minimumDataLength - available, pTotalSize, true);
}

void BazisLib::Network::BufferedSocketBase::Discard( void *pData, size_t size )
{
	ASSERT(pData == m_Buffer.GetData(m_BufferOffset));
	ASSERT(size <= m_Buffer.GetSize());
	if (size > m_Buffer.GetSize())
		size = m_Buffer.GetSize();

	m_BufferOffset += size;
}

//Returns -1 if not found
static size_t FindMarker(const void *pBuffer, size_t bufferSize, const void *pMarker, size_t markerSize)
{
	if (!markerSize)
		return -1;
	if (markerSize > bufferSize)
		return -1;
	char *p = (char *)pBuffer;
	char firstChar = ((char *)pMarker)[0];

	size_t lengthToSearch = bufferSize - markerSize + 1;
	for (size_t i = 0; i < lengthToSearch; i++)
	{
		char *pSeq = (char *)memchr(p + i, firstChar, lengthToSearch - i);
		if (!pSeq)
			return -1;
		i = pSeq - p;

		if (!memcmp(pSeq, pMarker, markerSize))
			return i;
	}

	return -1;
}

size_t BazisLib::Network::BufferedSocketBase::RecvToMarker( void *pBuffer, size_t length, const void *pMarker, size_t markerSize )
{
	if (!markerSize)
	{
		m_LastRecvError = kInvalidArgument;
		return 0;
	}

	//1. Try searching for the marker in the internal buffer
	//2. If failed, copy data to external buffer and try receiving more until the buffer is full
	//3. After every iteration try finding the marker
	//4. If failed, return 0
	//   If found, put what's left to the internal buffer

	size_t availableInternally = m_Buffer.GetSize() - m_BufferOffset;
	size_t off = FindMarker(m_Buffer.GetData(m_BufferOffset), min(availableInternally, length), pMarker, markerSize);
	if (off != -1)
	{
		ASSERT((off + markerSize) <= length);
		memcpy(pBuffer, m_Buffer.GetData(m_BufferOffset), off + markerSize);
		m_BufferOffset += (off + markerSize);
		m_LastRecvError = kSuccess;
		return off + markerSize;
	}
	//The marker was not found in the buffer. Try to fetch more

	if (availableInternally >= length)
	{
		m_LastRecvError = kBufferFull;
		return 0;
	}

	memcpy(pBuffer, m_Buffer.GetData(m_BufferOffset), availableInternally);
	//At this point the internal buffer has been completely copied to the user buffer. We will copy it back if we go to the fallback routine.

	size_t bufOff = availableInternally, prevOff = 0;
	while (bufOff < length)
	{
		size_t todo = length - bufOff;
		size_t done = RawRecv(((char *)pBuffer) + bufOff, todo);
		if (done == 0)
		{
			m_LastRecvError = kSocketClosed;
			return 0;
		}
		else if (done == -1)
		{
			m_LastRecvError = kSocketClosed;
			return 0;
		}

		off = FindMarker(((char *)pBuffer) + prevOff, bufOff + done - prevOff, pMarker, markerSize);
		if (off != -1)
		{
			off += prevOff;	//Adjust 'off' so that it contains the offset from the beginning of buffer
			off += markerSize;

			//Off now points to the end of the data we are returning
			size_t writeBackSize = bufOff + done - off;
			if (!m_Buffer.EnsureSize(writeBackSize))
			{
				m_LastRecvError = kOutOfMemoryAndLostData;
				return 0;
			}

			m_BufferOffset = 0;
			memcpy(m_Buffer.GetData(), ((char *)pBuffer) + off, writeBackSize);
			m_Buffer.SetSize(writeBackSize);
			
			return off;
		}

		bufOff += done;
		if (bufOff > markerSize)
			prevOff = bufOff - markerSize + 1;
		else
			prevOff = 0;
	}

	//Fallback case. We cannot go further as the user-supplied buffer has been filled completely.
	//This should happen relatively rarely, so copying the data back to our internal buffer
	//should not be a performance problem here.

	//The data from the internal buffer has been copied to the user buffer, as we assumed that we
	//are going to return it. Now we are copying it back.
	ASSERT(bufOff == length);

	//An optimization is possible here: if the original m_BufferOffset was 0, we do not need to
	//copy back the first part of the buffer. However, it's probably impractical to implement.

	if (!m_Buffer.EnsureSize(bufOff))
	{
		m_LastRecvError = kOutOfMemoryAndLostData;
		return 0;
	}

	m_BufferOffset = 0;
	memcpy(m_Buffer.GetData(), pBuffer, bufOff);
	m_Buffer.SetSize(bufOff);

	m_LastRecvError = kBufferFull;
	return 0;
}

bool BazisLib::Network::BufferedSocketBase::RecvLine( DynamicStringA *pLine, size_t bufferSize /*= 1024 * 1024*/ )
{
	char *pBuf = pLine->PreAllocate(bufferSize, false);
	if (!pBuf)
		return false;

	size_t done = RecvToMarker(pBuf, bufferSize, "\r\n", 2);
	if (!done)
		return false;
	ASSERT(done >= 2 && pBuf[done - 2] == '\r' && pBuf[done - 1] == '\n');
	if (done >= 2)
		done -= 2;
	else
		done = 0;

	pLine->SetLength(done);
	return true;
}
