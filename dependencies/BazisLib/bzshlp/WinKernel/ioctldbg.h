#pragma once

#ifdef _DEBUG
namespace BazisLib
{
	namespace DDK
	{
		const char *MapDeviceControlCode(ULONG Code);
		
		//The format should contain %s for the control code! It will be automatically mapped to string.
		void DumpDeviceControlCode(const char *pszFormat, ULONG Code);

		void DumpBinaryBuffer(const void *pBuffer, int Size);
		void DumpInt32Buffer(const void *pBuffer, int Size);

	}
}

#define DUMP_DEVIOCTL(fmt, code) DumpDeviceControlCode((fmt), (code))

#else

#define DUMP_DEVIOCTL(fmt, code)

#endif
