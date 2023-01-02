#pragma once

#include "../bzscore/socket.h"
#include "../bzscore/buffer.h"
#include "../bzscore/string.h"

namespace BazisLib
{
	namespace Network
	{
		//! Provides "receive until a sequence is encountered" paradigm.
		/*! This class provides a socket capable of 3 different ways of receiving data:
			 - Receive at most N bytes (like the normal recv() function)
			 - Receive data until a given marker (e.g. CRLF) is encountered.
			 - Peek the data into an internal buffer and give a pointer to it, then discard some of the data from the internal buffer.
		*/
		class BufferedSocketBase
		{
		public:
			enum RecvErrorType{kSuccess, kSocketClosed, kInternalError, kBufferFull, kOutOfMemoryAndLostData, kInvalidArgument};

		private:
			BasicBuffer m_Buffer;
			size_t m_BufferOffset;
			RecvErrorType m_LastRecvError;

		protected:
			virtual size_t RawRecv(void *pBuffer, size_t length, bool waitUntilEntireBuffer = false)=0;

		public:
			BufferedSocketBase()
				: m_BufferOffset(0)
			{
			}

			//! Works just like the normal recv() function, but takes the internal buffer into account.
			size_t Recv(void *pBuffer, size_t length, bool waitUntilEntireBuffer = false);
			
			//! Tries to receive more data in the buffer, returns the internal buffer and the actually fetched size.
			/*! 

				\param increment Maximum amount of bytes to fetch in one call. Can be 0.
				\param pTotalSize Points to a variable that receives the total amount of bytes available for reading (fetched before and now).
				\return The method returns the pointer to the buffer containing *pTotalSize bytes.
				\attention Calling any other method of this class after calling Peek() may invalidate the pointer returned by it.
						   The only valid way to use it is to analyze/copy the necessary amount of data from the returned pointer and immediately call Discard() if needed.
			*/
			void *PeekRel(size_t increment, OUT size_t *pTotalSize, bool waitForAllData = false);
			void *PeekAbs(size_t minimumDataLength, OUT size_t *pTotalSize);
			void Discard(void *pData, size_t size);

			//! Receives data from the underlying socket until a given sequence ('marker') is encountered.
			/*! 
				\return The method returns the amount of bytes fetched into the buffer, or 0 if any of the following occurs:
					- The underlying socket closes connection before transferring the 'marker'
					- The marker is not found within the first 'length' bytes
					You can call GetLastRecvToMarkerError() to get more details.
				\remarks The most common use case of this method is to read text from sockets line-by-line (marker = CRLF).
				\attention The returned data always includes the marker.
			*/
			size_t RecvToMarker(void *pBuffer, size_t length, const void *pMarker, size_t markerSize);

			RecvErrorType GetLastRecvError() {return m_LastRecvError;}

			//! Receives a line until a CRLF marker. The CRLF marker itself is not included in the returned line.
			bool RecvLine(DynamicStringA *pLine, size_t bufferSize = 1024 * 1024);

		};

		//! Provides a TCP/IP socket with additional "receive line-by-line"-style interface.
		class TCPSocketEx : public BufferedSocketBase
		{
		private:
			TCPSocket *m_pSocket;
			bool m_bOwn;

		private:
			TCPSocketEx(const TCPSocket &socket)
			{
				//Socket instance cannot be copied
				ASSERT(FALSE);
			}

			void operator=(const TCPSocket &socket)
			{
				//Socket instance cannot be copied
				ASSERT(FALSE);
			}
	

		public:
			SOCKET GetSocketForSingleUse() const {return m_pSocket->GetSocketForSingleUse();}
			SOCKET DetachSocket() { return m_pSocket->DetachSocket();}

		public:
			TCPSocketEx(TCPSocket *pSocket, bool own = false)
				: m_pSocket(pSocket), m_bOwn(own)
			{
			}

			TCPSocketEx(const char *pszAddr, unsigned port, const BazisLib::Network::InternetAddress &addressToBind = BazisLib::Network::InternetAddress())
				: m_bOwn(true)
			{
				m_pSocket = new TCPSocket(pszAddr, port, addressToBind);
			}

			TCPSocketEx(const BazisLib::Network::InternetAddress &addr)
				: m_bOwn(true)
			{
				m_pSocket = new TCPSocket(addr);
			}

			TCPSocketEx(class TCPListener &listener)
				: m_bOwn(true)
			{
				m_pSocket = new TCPSocket(listener);
			}

			~TCPSocketEx()
			{
				if (m_bOwn)
					delete m_pSocket;
			}

			ActionStatus Connect(const BazisLib::Network::InternetAddress &address, const BazisLib::Network::InternetAddress &addressToBind = BazisLib::Network::InternetAddress())
			{
				return m_pSocket->Connect(address, addressToBind);
			}

			ActionStatus SetTimeouts(unsigned receiveTimeoutInMsec = -1, unsigned sendTimeoutInMsec = -1)
			{
				return m_pSocket->SetTimeouts(receiveTimeoutInMsec, sendTimeoutInMsec);
			}

			bool Valid() const 	{ return m_pSocket && m_pSocket->Valid(); }

			void Close() {if (m_pSocket) m_pSocket->Close();}

			size_t Send(const void *pBuffer, size_t length, int rawFlags = 0)
			{
				return m_pSocket->Send(pBuffer, length, rawFlags);
			}

			bool SetNoDelay(bool noDelay) {return m_pSocket->SetNoDelay(noDelay);}
			bool IsNoDelay() {return m_pSocket->IsNoDelay();}

			BazisLib::Network::InternetAddress GetRemoteAddress() {return m_pSocket->GetRemoteAddress();}

#ifdef BZSLIB_SOCKET_LOGGING_SUPPORT
			void StartLogging(const TCHAR *pLogFile)
			{
				m_pSocket->StartLogging(pLogFile);
			}
#endif

		protected:
			virtual size_t RawRecv(void *pBuffer, size_t length, bool waitUntilEntireBuffer = false)
			{
				return m_pSocket->Recv(pBuffer, length, waitUntilEntireBuffer);
			}
		};
	}
}
