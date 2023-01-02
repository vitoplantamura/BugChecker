#pragma once
#ifndef UNDER_CE

#include "cmndef.h"
#include "../bzsstream.h"
#include "../../bzscore/string.h"
#include "bzsdev.h"
#include <list>
#include <string>

static const GUID GUID_CLASS_I82930_BULK = {0x873fdf, 0x61a8, 0x11d1, 0xaa, 0x5e, 0x0, 0xc0, 0x4f, 0xb1, 0x72, 0x8b};

namespace BazisLib
{
	namespace Win32
	{
		class IBulkPipe : public IPipe
		{
		public:
			virtual size_t ReadBulk(void *pBuffer, size_t Size)=0;
			virtual size_t WriteBulk(const void *pBuffer, size_t Size)=0;
			virtual ~IBulkPipe(){}
		};

		class CBulkUsbPipe : public IBulkPipe
		{
		private:
			HANDLE m_hInPipe;
			HANDLE m_hOutPipe;

		public:
			CBulkUsbPipe(const String &devicePath, bool SwapPipes = false);

		public:
			static std::list<BazisLib::String> EnumerateDevices(const GUID *pguidInterfaceType = NULL, PDEVICE_SN_FILTER Filter = 0, void *pContext = 0)
			{
				if (!pguidInterfaceType)
					pguidInterfaceType = &GUID_CLASS_I82930_BULK;
				return BazisLib::EnumerateDevicesByInterface(pguidInterfaceType, Filter, pContext);
			}

			static CBulkUsbPipe* OpenDevice(const BazisLib::String &DevicePath, bool SwapPipes);
			static CBulkUsbPipe* CreateDefault(const GUID *pguidInterfaceType = NULL, bool SwapPipes = false);

		public:
			virtual size_t Read(void *pBuffer, size_t Size);
			virtual size_t Write(const void *pBuffer, size_t Size);

			virtual size_t ReadBulk(void *pBuffer, size_t Size) {return Read(pBuffer, Size);}
			virtual size_t WriteBulk(const void *pBuffer, size_t Size) {return Write(pBuffer, Size);}

			virtual LONGLONG GetSize() {return 0;}
			virtual bool SetTimeout(unsigned ReadTimeout, unsigned WriteTimeout);
			virtual bool Valid() {return (m_hInPipe != INVALID_HANDLE_VALUE) && (m_hOutPipe != INVALID_HANDLE_VALUE);}
			virtual void Close();
			virtual ~CBulkUsbPipe();
		};
	}
}

#endif