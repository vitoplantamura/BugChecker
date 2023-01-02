#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)

#include "bulkusb.h"
#include <setupapi.h>

using namespace std;

#pragma comment(lib, "setupapi")

using namespace BazisLib::Win32;

CBulkUsbPipe::CBulkUsbPipe(const BazisLib::String &devicePath, bool SwapPipes) :
	m_hInPipe(INVALID_HANDLE_VALUE),
	m_hOutPipe(INVALID_HANDLE_VALUE)
{
	const TCHAR *pszInPipeName = SwapPipes ? _T("\\PIPE01") : _T("\\PIPE00");
	const TCHAR *pszOutPipeName = SwapPipes ? _T("\\PIPE00") : _T("\\PIPE01");
	BazisLib::String pipeName = devicePath + pszOutPipeName;
	m_hOutPipe = CreateFile(pipeName.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,//FILE_FLAG_OVERLAPPED,
                NULL);
	pipeName = devicePath + pszInPipeName;
	m_hInPipe = CreateFile(pipeName.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,//FILE_FLAG_OVERLAPPED,
                NULL);
	int i = GetLastError();
}

CBulkUsbPipe* CBulkUsbPipe::OpenDevice(const BazisLib::String &DevicePath, bool SwapPipes)
{
	CBulkUsbPipe *pPipe = new CBulkUsbPipe(DevicePath, SwapPipes);
	if (!pPipe || !pPipe->Valid())
	{
		delete pPipe;
		pPipe = NULL;
	}
	return pPipe;
}

CBulkUsbPipe* CBulkUsbPipe::CreateDefault(const GUID *pguidInterfaceType, bool SwapPipes)
{
	std::list<BazisLib::String> devices = EnumerateDevices(pguidInterfaceType);
	if (devices.size() != 1)
		return NULL;
	return OpenDevice(*(devices.begin()), SwapPipes);
}

size_t CBulkUsbPipe::Read(void *pBuffer, size_t Size)
{
	if (!Valid())
		return 0;
	DWORD dwOk;
	ASSERT_32BIT(Size);
	ReadFile(m_hInPipe, pBuffer, (DWORD)Size, &dwOk, 0);
	return dwOk;
}

size_t CBulkUsbPipe::Write(const void *pBuffer, size_t Size)
{
	if (!Valid())
		return 0;
	DWORD dwOk;
	ASSERT_32BIT(Size);
	WriteFile(m_hOutPipe, pBuffer, (DWORD)Size, &dwOk, 0);
	return dwOk;
}

bool CBulkUsbPipe::SetTimeout(unsigned ReadTimeout, unsigned WriteTimeout)
{
	return false;
}

void CBulkUsbPipe::Close()
{
	CloseHandle(m_hInPipe);
	CloseHandle(m_hOutPipe);
	m_hInPipe = m_hOutPipe = INVALID_HANDLE_VALUE;
}

CBulkUsbPipe::~CBulkUsbPipe()
{
	if (m_hInPipe != INVALID_HANDLE_VALUE)
		CloseHandle(m_hInPipe);
	if (m_hOutPipe != INVALID_HANDLE_VALUE)
		CloseHandle(m_hOutPipe);
}

#endif