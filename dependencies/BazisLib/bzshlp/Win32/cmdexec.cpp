#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "cmdexec.h"
#include "bulkusbcmd.h"

#ifndef UNDER_CE

using namespace BazisLib::Win32;

static bool SNFilter (const BazisLib::String &serial, void *pContext)
{
	const TCHAR *pszMask = (const TCHAR *)pContext;
	bool anyPrefix = false, anySuffix = false;
	if (!pszMask)
		return false;
	BazisLib::String mask(pszMask);
	if (!mask.length())
		return true;
	if (mask[0] == '*')
	{
		if (mask.length() == 1)
			return true;
		anyPrefix = true;
		mask = mask.substr(1);
	}
	if (mask[mask.length() - 1] == '*')
	{
		anySuffix = true;
		mask = mask.substr(0, mask.length() - 1);
	}
	size_t idx = serial.find(mask);
	if (idx == -1)
		return false;
	if (!anyPrefix && (idx != 0))
		return false;
	if (!anySuffix && (idx != (serial.length() - mask.length())))
		return false;
	return true;
}

CBulkUsbCommandExecuteInterface::CBulkUsbCommandExecuteInterface(const TCHAR *pszSNMask)
{
	m_pPipe = NULL;
	m_pTransferBuffer = NULL;
	m_MaxTransferBufferSize = 0;

	std::list<BazisLib::String> devices = CBulkUsbPipe::EnumerateDevices(NULL, pszSNMask ? SNFilter : NULL, (void *)pszSNMask);
	if (devices.size() != 1)
		return;
	m_pPipe = CBulkUsbPipe::OpenDevice(*devices.begin(), false);
	if (!m_pPipe || !m_pPipe->Valid() || !InitializeDevice())
	{
		delete m_pPipe;
		m_pPipe = NULL;
		return;
	}
}

CBulkUsbPipe *CBulkUsbCommandExecuteInterface::EnterRawMode()
{
	int Request[4];
	Request[0] = BULKUSB_RQTYPE_ENTER_RAWMODE;
	Request[1] = 0;
	Request[2] = 0;
	if (m_pPipe->Write(Request, 12) != 12)
		return NULL;
	if (m_pPipe->Read(Request, 12) != 12)
		return NULL;
	if ((Request[0] != BULKUSB_RESTYPE_SYSTEM) || (Request[1] != BULKUSB_STATUS_OK))
		return NULL;
	return m_pPipe;
}

CBulkUsbCommandExecuteInterface::~CBulkUsbCommandExecuteInterface()
{
	Disconnect();
	if (m_pTransferBuffer)
		free(m_pTransferBuffer);
}
	

bool CBulkUsbCommandExecuteInterface::InitializeDevice()
{
	if (!m_pPipe || !m_pPipe->Valid())
		return false;
	int Request[4];
	Request[0] = BULKUSB_RQTYPE_QUERY_PROTOVER;
	Request[1] = 0;
	Request[2] = 0;
	if (m_pPipe->Write(Request, 12) != 12)
		return false;
	if (m_pPipe->Read(Request, 16) != 16)
		return false;
	if ((Request[0] != BULKUSB_RESTYPE_SYSTEM) || (Request[1] != BULKUSB_STATUS_OK) || (Request[2] != 4) || (Request[3] != PROTOCOL_VERSION_CURRENT))
		return false;

	Request[0] = BULKUSB_RQTYPE_QUERY_MAX_SIZE;
	Request[1] = 0;
	Request[2] = 0;
	if (m_pPipe->Write(Request, 12) != 12)
		return false;
	if (m_pPipe->Read(Request, 16) != 16)
		return false;
	if ((Request[0] != BULKUSB_RESTYPE_SYSTEM) || (Request[1] != BULKUSB_STATUS_OK) || (Request[2] != 4))
		return false;
	m_MaxTransferBufferSize = Request[3];

	if (m_pTransferBuffer)
		free(m_pTransferBuffer);
	m_pTransferBuffer = (char *)malloc(m_MaxTransferBufferSize);
	if (!m_pTransferBuffer)
		return false;
	return true;
}

bool CBulkUsbCommandExecuteInterface::Disconnect()
{
	if (!m_pPipe)
		return true;
	delete m_pPipe;
	m_pPipe = NULL;
	return true;
}

bool CBulkUsbCommandExecuteInterface::SetTimeout(int ms)
{
	return m_pPipe->SetTimeout(ms, ms);
}


unsigned CBulkUsbCommandExecuteInterface::ExecuteCommand(int CmdCode,
												 unsigned InputBufferSize,
												 void *pInputBuffer,
												 unsigned ResultBufferSize,
												 void *pResultBuffer)
{
	if ((InputBufferSize >= (m_MaxTransferBufferSize - BULKUSB_HEADER_SIZE)) 
//		|| (ResultBufferSize >= (m_MaxTransferBufferSize - BULKUSB_HEADER_SIZE))
		)
		return 0;
	if (!m_pPipe || !m_pPipe->Valid() || !m_pTransferBuffer)
		return 0;
	if ((InputBufferSize < 0) || (ResultBufferSize < 0))
		return 0;
	
	int *pRequest = (int *)m_pTransferBuffer;
	pRequest[0] = BULKUSB_RQTYPE_USERV;
	pRequest[1] = CmdCode;
	pRequest[2] = InputBufferSize;
	if (InputBufferSize)
		memcpy(&pRequest[3], pInputBuffer, InputBufferSize);
	if (m_pPipe->Write(m_pTransferBuffer, BULKUSB_HEADER_SIZE + InputBufferSize) != (BULKUSB_HEADER_SIZE + InputBufferSize))
		return 0;
	size_t ReadSize = m_pPipe->Read(m_pTransferBuffer, BULKUSB_HEADER_SIZE);
	if (ReadSize != BULKUSB_HEADER_SIZE)
		return 0;
	int Status = pRequest[1];
	if (pRequest[0] != BULKUSB_RESTYPE_USERV)
		return 0;
	unsigned ResultSize = pRequest[2];
	if (ResultSize > ResultBufferSize)
	{
		m_pPipe->Read(m_pTransferBuffer, ResultSize);
		return 0;
	}
	if (ResultSize)
		ReadSize = m_pPipe->Read(pResultBuffer, ResultSize);
	if (ReadSize != ResultSize)
		return 0;
	if (Status != BULKUSB_STATUS_OK)
		return 0;
	return ResultSize;
}					   		

bool CBulkUsbCommandExecuteInterface::IsConnected()
{
	return ((m_pPipe != NULL) && m_pPipe->Valid());
}

//----------------------------------------------------------------------------------------------------------

CBulkUsbBootloaderCommandExecuteInterface::CBulkUsbBootloaderCommandExecuteInterface(const char *pszSNMask)
{
	m_pPipe = NULL;
	std::list<BazisLib::String> devices = CBulkUsbPipe::EnumerateDevices(NULL, pszSNMask ? SNFilter : NULL, (void *)pszSNMask);
	if (devices.size() != 1)
		return;
	m_pPipe = CBulkUsbPipe::OpenDevice(*devices.begin(), false);
	if (!m_pPipe->Valid() || !InitializeDevice())
	{
		delete m_pPipe;
		m_pPipe = NULL;
	}
}

CBulkUsbBootloaderCommandExecuteInterface::~CBulkUsbBootloaderCommandExecuteInterface()
{
	Disconnect();
}
	
bool CBulkUsbBootloaderCommandExecuteInterface::InitializeDevice()
{
	if (!m_pPipe || !m_pPipe->Valid())
		return false;
	int Request[4];
	Request[0] = BULKUSB_RQTYPE_ENTERLDR;
	Request[1] = 0;
	Request[2] = 0;
	if (m_pPipe->Write(Request, 12) != 12)
		return false;
	if (m_pPipe->Read(Request, 12) != 12)
		return false;
	if ((Request[0] != BULKUSB_RESTYPE_SYSTEM) || (Request[1] != BULKUSB_STATUS_OK))
		return false;
	return true;
}

bool CBulkUsbBootloaderCommandExecuteInterface::Disconnect()
{
	if (!m_pPipe)
		return true;
	delete m_pPipe;
	m_pPipe = NULL;
	return true;
}

bool CBulkUsbBootloaderCommandExecuteInterface::SetTimeout(int ms)
{
	return m_pPipe->SetTimeout(ms, ms);
}


unsigned CBulkUsbBootloaderCommandExecuteInterface::ExecuteCommand(int CmdCode,
												 unsigned InputBufferSize,
												 void *pInputBuffer,
												 unsigned ResultBufferSize,
												 void *pResultBuffer)
{
	if (!m_pPipe || !m_pPipe->Valid())
		return 0;
	char *pData = (char*)malloc(InputBufferSize + 1);
	if (!pData)
		return 0;
	pData[0] = CmdCode;
	if (InputBufferSize)
		memcpy(&pData[1], pInputBuffer, InputBufferSize);
	if (m_pPipe->Write(pData, InputBufferSize + 1) != (InputBufferSize + 1))
	{
		free(pData);
		return false;
	}
	free(pData);
	if (ResultBufferSize && pResultBuffer)
		return (unsigned)m_pPipe->Read(pResultBuffer, ResultBufferSize);
	return 0;
}					   		

bool CBulkUsbBootloaderCommandExecuteInterface::IsConnected()
{
	return ((m_pPipe != NULL) && m_pPipe->Valid());
}

#endif
#endif