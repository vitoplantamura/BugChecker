#pragma once

#include "cmndef.h"
#include "storage.h"

/*! \page win_cache_bug Windows Cache bug
	The following strange behaviour has been spotted on Windows XP SP3 for file-based virtual disks:
	If a virtual disk driver with a filesystem mounted on it receives an IRP_MJ_READ request and
	issues a read request for some FS file using <b>the same buffer</b>, this buffer remains
	allocated (held by system cache?) until a big timeout elapses or until the FS is dismounted.
	That leads to strange behaviour for reading large amounts of data from virtual disks: the
	system cache occupies all available RAM and the PC freezes continiously trying to free some
	RAM by moving data in and out the pagefile.
	
	It looks like Windows XP (and Vista?) maintains a global map for buffers used for read requests,
	and the sequence that causes the bug is the following:
	<ul>
		<li>Windows issues a read request A to our virtual device</li>
		<li>The buffer BUF is put in cache map</li>
		<li>Our device issues a read request B for physical file with the same buffer</li>
		<li>The buffer BUF enters some buggy state in cache map (or marked as very-very-frequently-used and, thus,
		non-discardable)</li>
		<li>The normal event for freeing buffer BUF does not work, as it is in some buggy state</li>
		<li>The buffer remains allocated until some other timeout elapses, or the FS is dismounted</li>
	</ul>

	The workaround is very simple: we allocate an intermediate buffer and substitute it to our
	Read() handlers instead of the original one. That way, it can safely be passed to file reading
	functions. If the size of a read request is greater than our buffer size, the request is split
	into several Read() calls.
	
	\attention To enable the workaround, simpy call the BasicStorageVolume::EnableWindowsCacheBugWorkaround()
			method from your child class constructor.
*/
	

namespace BazisLib
{
	namespace DDK
	{
		/*! This class provides a very easy way to maintain a volume recognizable by Windows Device Manager.
			Please do not override methods other than listed after "Overridables" comment to avoid incompatibility
			with future versions.

			If you requre any additional functionality, please create another universal library class providing
			that functionality and having the corresponding abstract methods, and contact me via SourceForge.net
		*/
		class BasicStorageVolume : protected StorageDevice
		{
		private:
			unsigned m_SectorSize;
			//Call UpdateVolumeSize() to force this parameters to be updated later
			ULONGLONG m_SectorCount;
			ULONGLONG m_SizeInBytes;
			DEVICE_TYPE m_ReportedDeviceType;

			void *m_pWinCacheBugWorkaroundBuffer;
			size_t m_WinCacheBugWorkaroundBufferSize;

		public:
			BasicStorageVolume(LPCWSTR pwszDevicePrefix = L"BazisVolume",
						  bool bDeleteThisAfterRemoveRequest = false,
						  ULONG DeviceType = FILE_DEVICE_DISK,
						  ULONG DeviceCharacteristics = FILE_DEVICE_SECURE_OPEN,
						  bool bExclusive = FALSE,
						  ULONG AdditionalDeviceFlags = DO_POWER_PAGABLE);	//DO_DIRECT_IO is set automatically
			~BasicStorageVolume();

			bool Valid() {return __super::Valid();}
			NTSTATUS AddDevice(Driver *pDriver, PDEVICE_OBJECT PhysicalDeviceObject);

		private:
			//! Allocates a shadow buffer for read operations preventing to prevent Windows read cache bug
			/*! This method is called from the OnStartDevice() method and no longer needs to be called from
				a child class. The details on windows cache bug can be found \ref win_cache_bug here.
			*/
			NTSTATUS EnableWindowsCacheBugWorkaround(size_t IntermediateBufferSize = 1024 * 1024);

			//! Updates stored volume size
			bool UpdateVolumeSize();

		protected:
			//Do not override these methods when writing a functional volume (volume serving for some practical purpose)
			//Instead, please create a universal class, and subclass it.

			virtual NTSTATUS DISPATCH_ROUTINE_DECL DispatchRoutine(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp) DISPATCH_ROUTINE_OVERRIDE;

			virtual NTSTATUS OnRead(PMDL pMdl, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone) override;
			virtual NTSTATUS OnWrite(PMDL pMdl, ULONG Length, ULONG Key, ULONGLONG ByteOffset, PULONG_PTR pBytesDone) override;
			virtual NTSTATUS OnCreate(PIO_SECURITY_CONTEXT SecurityContext, ULONG Options, USHORT FileAttributes, USHORT ShareAccess, PFILE_OBJECT pFileObj) override;
			virtual NTSTATUS OnClose() override;

			virtual NTSTATUS OnCheckVerify(ULONG OriginalControlCode, ULONG *pChangeCount) override;
			virtual NTSTATUS OnGetVolumeDiskExtents(PVOLUME_DISK_EXTENTS pExtents, ULONG BufferLength, PULONG_PTR pBytesDone) override;
			virtual NTSTATUS OnGetPartitionInfo(PPARTITION_INFORMATION pInfo, ULONG BufferLength, PULONG_PTR pBytesDone) override;
			virtual NTSTATUS OnGetPartitionInfoEx(PPARTITION_INFORMATION_EX pInfo, ULONG BufferLength, PULONG_PTR pBytesDone) override;
			virtual NTSTATUS OnVolumeOnline() override;
			virtual NTSTATUS OnVolumeOffline() override;
			virtual NTSTATUS OnGetDeviceNumber(PSTORAGE_DEVICE_NUMBER pNumber) override;
			virtual NTSTATUS OnGetDriveGeometry(PDISK_GEOMETRY pGeo) override;
			virtual NTSTATUS OnGetLengthInfo(PGET_LENGTH_INFORMATION pLength) override;
			virtual NTSTATUS OnDiskIsWritable() override;
			virtual NTSTATUS OnMediaRemoval(bool bLock) override;
			virtual NTSTATUS OnGetHotplugInfo(PSTORAGE_HOTPLUG_INFO pHotplugInfo) override;
			virtual NTSTATUS OnDiskVerify(PVERIFY_INFORMATION pVerifyInfo) override;

			virtual NTSTATUS OnSetPartitionInfo(PPARTITION_INFORMATION pInfo, ULONG InputLength) override;
			virtual NTSTATUS OnSetPartitionInfoEx(PPARTITION_INFORMATION_EX pInfo, ULONG InputLength) override;

		protected:
			virtual NTSTATUS OnStartDevice(IN PCM_RESOURCE_LIST AllocatedResources, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated) override;

		protected:
			//Overridables

			virtual unsigned GetSectorSize() {return 512;}
			virtual ULONGLONG GetTotalSize() = 0;
			virtual char GetPartitionType() {return PARTITION_FAT32;}

			//The Size parameter is automatically adjusted not to extend disk size.
			//You can safely skip checking its bounds as well as pBuffer validity when writing an override.
			virtual NTSTATUS Read(ULONGLONG ByteOffset, PVOID pBuffer, ULONG Length, PULONG_PTR pBytesDone)=0;
			virtual NTSTATUS Write(ULONGLONG ByteOffset, PVOID pBuffer, ULONG Length, PULONG_PTR pBytesDone)=0;

		protected:
			ULONGLONG GetSectorCount() {return m_SectorCount;}
		};
	}
}