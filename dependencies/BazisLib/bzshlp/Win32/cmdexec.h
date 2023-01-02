#pragma once
#ifndef UNDER_CE
#include "cmndef.h"
#include "bulkusb.h"

namespace BazisLib
{
	namespace Win32
	{
		class CCommandExecuteInterface
		{
		public:
			virtual bool Disconnect()=0;
			virtual bool SetTimeout(int ms)=0;

			virtual unsigned ExecuteCommand(int CmdCode,
				unsigned InputBufferSize,
				void *pInputBuffer,
				unsigned ResultBufferSize,
				void *pResultBuffer)=0;

			virtual bool IsConnected()=0;
			virtual ~CCommandExecuteInterface() {}

			unsigned ExecuteCommandWithStatus(int CmdCode,
				unsigned InputBufferSize = 0,
				void *pInputBuffer = NULL)
			{
				char status = -1;
				if (ExecuteCommand(CmdCode, InputBufferSize, pInputBuffer, 1, &status) != 1)
					return -1;
				return status;
			}
		};

		class CBulkUsbCommandExecuteInterface : public CCommandExecuteInterface
		{
		private:
			CBulkUsbPipe *m_pPipe;
			void *m_pTransferBuffer;
			unsigned m_MaxTransferBufferSize;

		protected:
			bool InitializeDevice();

		public:
			CBulkUsbCommandExecuteInterface(const TCHAR *pszSNMask = NULL);
			virtual ~CBulkUsbCommandExecuteInterface();
			bool Disconnect();
			virtual bool SetTimeout(int ms);

			virtual unsigned ExecuteCommand(int CmdCode,
				unsigned InputBufferSize,
				void *pInputBuffer,
				unsigned ResultBufferSize,
				void *pResultBuffer);

			virtual bool IsConnected();

			CBulkUsbPipe *EnterRawMode();
		};

		class CBulkUsbBootloaderCommandExecuteInterface : public CCommandExecuteInterface
		{
		private:
			CBulkUsbPipe *m_pPipe;

		protected:
			bool InitializeDevice();

		public:
			CBulkUsbBootloaderCommandExecuteInterface(const char *pszSNMask = NULL);
			virtual ~CBulkUsbBootloaderCommandExecuteInterface();
			bool Disconnect();
			virtual bool SetTimeout(int ms);

			virtual unsigned ExecuteCommand(int CmdCode,
				unsigned InputBufferSize,
				void *pInputBuffer,
				unsigned ResultBufferSize,
				void *pResultBuffer);

			virtual bool IsConnected();
		};
	}
}

#endif