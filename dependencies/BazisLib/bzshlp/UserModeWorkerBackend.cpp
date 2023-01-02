#include "stdafx.h"
#if defined(BZSLIB_WINKERNEL)

#include "UserModeWorkerBackend.h"

using namespace BazisLib;
using namespace BazisLib::DDK;

bool BazisLib::UserModeWorkerBackend::ProcessRequest(CBuffer &inputBuffer, CBuffer &outputBuffer)
{
	if (_Cancelling)
		return false;
	MutexLocker lck1(_RequestProcessMutex);

	{
		MutexLocker lck2(_RequestFieldChangeMutex);
		m_pActiveInBuffer = &inputBuffer;
		m_pActiveOutBuffer = &outputBuffer;
		_RequestReadyEvent.Set();
		ASSERT(!_RequestCompletedEvent.IsSet());
		_RequestCompletedEvent.Reset();
	}

	_RequestCompletedEvent.Wait();
	if (_Cancelling)
		return false;

	{
		MutexLocker lck2(_RequestFieldChangeMutex);
		m_pActiveInBuffer = NULL;
		m_pActiveOutBuffer = NULL;
		_RequestCompletedEvent.Reset();
	}

	return true;
}

BazisLib::ActionStatus BazisLib::UserModeWorkerBackend::GetNextPendingRequest(void *pBuffer, IN OUT size_t *pBufferSize)
{
	if (_Cancelling)
		return MAKE_STATUS(OperationAborted);
	MutexLocker lck2(_RequestFieldChangeMutex);
	if (!m_pActiveInBuffer)
		return MAKE_STATUS(InvalidParameter);
	if (m_pActiveInBuffer->GetSize() > *pBufferSize)
		return MAKE_STATUS(InvalidParameter);

	*pBufferSize = m_pActiveInBuffer->GetSize();
	memcpy(pBuffer, m_pActiveInBuffer->GetData(), m_pActiveInBuffer->GetSize());
	_RequestReadyEvent.Clear();
	return MAKE_STATUS(Success);
}

BazisLib::ActionStatus BazisLib::UserModeWorkerBackend::SubmitReplyToRequest(const void *pBuffer, size_t replySize)
{
	if (_Cancelling)
		return MAKE_STATUS(OperationAborted);
	MutexLocker lck2(_RequestFieldChangeMutex);
	if (!m_pActiveOutBuffer)
		return MAKE_STATUS(InvalidParameter);
	if (!m_pActiveOutBuffer->EnsureSize(replySize))
		return MAKE_STATUS(NoMemory);
	m_pActiveOutBuffer->SetSize(replySize);
	memcpy(m_pActiveOutBuffer->GetData(), pBuffer, replySize);
	_RequestCompletedEvent.Set();
	return MAKE_STATUS(Success);
}

size_t BazisLib::UserModeWorkerBackend::GetPendingRequestSize()
{
	if (_Cancelling)
		return 0;
	MutexLocker lck2(_RequestFieldChangeMutex);
	if (!m_pActiveInBuffer || !m_pActiveOutBuffer)
		return 0;
	return max(m_pActiveInBuffer->GetSize(), m_pActiveOutBuffer->GetSize());
}

#endif