#pragma once

#include <stdlib.h>
#include "assert.h"
#include "string.h"

#ifndef BZSLIB_PLATFORM_SPECIFIC_SOCKETS_DEFINED
#error Include <bzscore/socket.h> instead of socket_common.h
#endif

namespace BazisLib
{
	namespace Network
	{
		//BufferedSocketBase provides basic functionality for parsing text protocols. This level
		//relies on abstract _Send()/_Recv() implemented in child classes.
		class BufferedSocketBase : public AIAdvancedSocket
		{
		private:
			//Temporary buffer
			char *m_pBuffer;
			unsigned m_AllocatedBufferSize;
			unsigned m_BufferPosition;
			unsigned m_BufferedBytes;
			unsigned m_LastRecvError;

		private:
			unsigned ReadFromBuffer(void *pBuffer, unsigned length);
			bool CopyToBuffer(const void *pData, unsigned length, const void *pData2 = 0, unsigned length2 = 0);

			static unsigned FindMarker(const void *pBuffer, unsigned bufferSize, const char *pMarker, unsigned markerSize)
			{
				_ASSERT(pBuffer && bufferSize && pMarker && markerSize);
				if (bufferSize < markerSize)
					return -1;
				//TODO: optimize
				bufferSize -= (markerSize - 1);
				for (unsigned idx = 0; idx < bufferSize; idx++)
					if (!memcmp(((const char *)pBuffer) + idx, pMarker, markerSize))
						return idx;
				return -1;
			}

		protected:
			BufferedSocketBase() : m_pBuffer(NULL), m_AllocatedBufferSize(0), m_BufferPosition(0), m_BufferedBytes(0), m_LastRecvError(RECV_SUCCESS) {}

			virtual size_t _Send(const void *pBuffer, size_t length = 0)=0;
			virtual size_t _Recv(void *pBuffer, size_t length)=0;
			virtual bool Valid() const=0;

		public:
			virtual bool Connect(unsigned IP, unsigned port, unsigned IPToBind = 0)=0;

		public:
			virtual bool Connect(const char *pszAddr, unsigned port, unsigned IPToBind = 0);
			virtual size_t Send(const void *pBuffer, size_t length = 0) {return _Send(pBuffer, length);}
			virtual size_t Recv(void *pBuffer, size_t length);

			virtual size_t RecvStrict(void *pBuffer, size_t length);
			virtual size_t RecvTo(void *pBuffer, size_t bufferSize, const char *pMarker, unsigned markerSize = 0, unsigned flags = 0);
			virtual size_t RecvTo(std::string &buffer, const char *pszMarker, unsigned flags = 0);
			int GetLastRecvError() const {return m_LastRecvError;}
			virtual ~BufferedSocketBase();
		};
	}
}
