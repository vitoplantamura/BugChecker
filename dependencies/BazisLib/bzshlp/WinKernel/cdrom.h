#pragma once

#include "cmndef.h"
#include "volume.h"

struct _CDROM_TOC;
struct _CDROM_TOC_SESSION_DATA;
struct _TRACK_DATA;

namespace BazisLib
{
	namespace DDK
	{
		/*! This class provides a very easy way to maintain a CD volume recognizable by Windows Device Manager.
			Please do not override methods other than listed after "Overridables" comment to avoid incompatibility
			with future versions.

			If you requre any additional functionality, please create another universal library class providing
			that functionality and having the corresponding abstract methods, and contact me via SourceForge.net

			For basic overview of various CD formats and concepts, see \ref cd_basics this page
		*/
		class BasicCDVolume : public BasicStorageVolume
		{
		private:
			unsigned m_SectorSize;
			ULONGLONG m_SectorCount;
			ULONGLONG m_SizeInBytes;
			DEVICE_TYPE m_ReportedDeviceType;

		protected:
			void BuildCDTrackAddress(unsigned SectorNumber, unsigned char *pAddr);
			void InitializeTrackRecord(_TRACK_DATA *pTrack, unsigned TrackNumber, unsigned SectorNumber);
			void InitializeTOCHeader(_CDROM_TOC *pTOC, unsigned TrackCount);

			enum {CD_SECTOR_SIZE = 2048};

		public:
			BasicCDVolume(LPCWSTR pwszDevicePrefix = L"CdRom",
						  bool bDeleteThisAfterRemoveRequest = false,
						  ULONG DeviceType = FILE_DEVICE_CD_ROM,
						  ULONG DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN,
						  bool bExclusive = FALSE,
						  ULONG AdditionalDeviceFlags = DO_POWER_PAGABLE);
			~BasicCDVolume();

		protected:
			virtual unsigned GetSectorSize() {return CD_SECTOR_SIZE;}

		protected:
			virtual NTSTATUS OnEject();
			virtual NTSTATUS OnReadTOC(struct _CDROM_TOC *pTOC);
			virtual NTSTATUS OnGetLastSession(struct _CDROM_TOC_SESSION_DATA *pSession);

		protected:
			/*  This method handles buffered device control codes (defined with METHOD_BUFFERED method). When writing
				an override, consider the following structure:
				switch (ControlCode)
				{
				case IOCTL_xxx:
					return OnXXX();
				case IOCTL_yyy:
					return OnYYY();
				default:
					return __super::OnDeviceControl(...);
				}
			*/
			virtual NTSTATUS OnDeviceControl(ULONG ControlCode, bool IsInternal, void *pInOutBuffer, ULONG InputLength, ULONG OutputLength, PULONG_PTR pBytesDone) override;

		};
	}
}