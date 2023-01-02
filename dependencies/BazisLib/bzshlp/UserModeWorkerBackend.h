#pragma once
#include <BazisLib/bzscore/status.h>
#include <BazisLib/bzscore/buffer.h>
#include <BazisLib/bzscore/sync.h>

namespace BazisLib
{
	class UserModeWorkerBackend
	{
	private:
		bool _Cancelling;

		Event _RequestCompletedEvent;
		EventWithHandle _RequestReadyEvent;

		Mutex _RequestProcessMutex;
		Mutex _RequestFieldChangeMutex;
		CBuffer *m_pActiveInBuffer, *m_pActiveOutBuffer;
	
	public:
		UserModeWorkerBackend()
			: _Cancelling(false)
			, m_pActiveOutBuffer(NULL)
			, m_pActiveInBuffer(NULL)
		{
		}

	public:	
		//Called from worker thread to submit a request to user-mode
		bool ProcessRequest(CBuffer &inputBuffer, CBuffer &outputBuffer);

	public:	//Called from IOCTL handlers
		ActionStatus GetNextPendingRequest(void *pBuffer, IN OUT size_t *pBufferSize);
		ActionStatus SubmitReplyToRequest(const void *pBuffer, size_t replySize);
		size_t GetPendingRequestSize();

		EventWithHandle &GetUserModeEvent() {return _RequestReadyEvent;}
		void Shutdown() {_Cancelling = true; _RequestReadyEvent.Set(); _RequestCompletedEvent.Set();}
	};
}