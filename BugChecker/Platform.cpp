#include "Platform.h"

#include "Symbols.h"
#include "Root.h"

#include <EASTL/vector.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

#include <ntimage.h>

NtModulesAccess::NtModulesAccess(BOOLEAN acquireSpinLock, const CHAR* dbgPrefixPtr /*= NULL*/)
	: lock(acquireSpinLock), dbgPrefix(dbgPrefixPtr), poolStart(Allocator::Perf_TotalAllocatedSize)
{
	if (lock)
		Root::I->DebuggerLock.Lock(&prevInts);

	Allocator::ForceUse = TRUE;
	Allocator::Start0to99 = ALLOCATOR_START_0TO99_NTMODULES;
}

NtModulesAccess::~NtModulesAccess()
{
	Allocator::ForceUse = FALSE;
	Allocator::Start0to99 = 0;

	if (lock)
		Root::I->DebuggerLock.Unlock(prevInts);

#if 0
	// write a dbg string.
	if (dbgPrefix)
		::DbgPrint("BugChecker::NtModulesAccess -> %s -> allocated %i bytes.", dbgPrefix, (LONG32)((LONG_PTR)Allocator::Perf_TotalAllocatedSize - (LONG_PTR)poolStart));
#endif
}

// >>>

typedef struct _KAPC_STATE {
	LIST_ENTRY ApcListHead[MaximumMode];
	struct _KPROCESS* Process;
	BOOLEAN KernelApcInProgress;
	BOOLEAN KernelApcPending;
	BOOLEAN UserApcPending;
} KAPC_STATE, * PKAPC_STATE, * PRKAPC_STATE;

extern "C"
{
	NTKERNELAPI
		VOID
		KeStackAttachProcess(
			__inout PRKPROCESS PROCESS,
			__out PRKAPC_STATE ApcState
		);

	NTKERNELAPI
		VOID
		KeUnstackDetachProcess(
			__in PRKAPC_STATE ApcState
		);

	NTKERNELAPI
		NTSTATUS
		PsLookupProcessByProcessId(
			__in HANDLE ProcessId,
			__deref_out PEPROCESS* Process
		);
}

// <<<

VOID* Platform::UserProbeAddress = NULL;
VOID* Platform::KernelBaseAddress = NULL;

ImageDebugInfo Platform::KernelDebugInfo;

//======================================================================================
//
// Offsets in Ntoskrnl structures. They are set in the CalculateKernelOffsets function.
// N.B. the values set here are overwritten by the CalculateKernelOffsets function.
//
//======================================================================================

#ifdef _AMD64_ // a=Windows 11 (april 2022), b=Windows XP SP3, c=Windows 2000 SP4
#define abc(a,b,c) a
#else
#define abc(a,b,c) b
#endif

int MACRO_SELFPTR_FIELDOFFSET_IN_PCR = -1; // abc(0x18, 0x1C, 0);

int MACRO_KTEBPTR_FIELDOFFSET_IN_PCR = -1; // abc((0x180 + 0x008), 292, 0x124); // _KPCR::PrcbData or _KPCR::Prcb(_KPRCB)::CurrentThread
int MACRO_KPEBPTR_FIELDOFFSET_IN_KTEB = -1; // abc((0x098 + 0x020), 68, 0x44); // _KTHREAD::ApcState(_KAPC_STATE)::Process
int MACRO_VADROOT_FIELDOFFSET_IN_KPEB = -1; // abc(0x7d8, 284, 0x194); // _EPROCESS::VadRoot
int MACRO_IMAGEBASE_FIELDOFFSET_IN_DRVSEC = -1; // abc(0x030, 24, 0x18); // _LDR_DATA_TABLE_ENTRY::DllBase
int MACRO_IMAGENAME_FIELDOFFSET_IN_DRVSEC = -1; // abc(0x48, 36, 0x24); // _LDR_DATA_TABLE_ENTRY::FullDllName

int MACRO_STARTINGVPN_FIELDOFFSET_IN_VAD = -1; // _6432_(0x18, 0);
int MACRO_ENDINGVPN_FIELDOFFSET_IN_VAD = -1; // _6432_(0x1c, 4);
int MACRO_LEFTLINK_FIELDOFFSET_IN_VAD = -1; // _6432_(0, 12);
int MACRO_RIGHTLINK_FIELDOFFSET_IN_VAD = -1; // _6432_(8, 16);
int MACRO_CONTROLAREA_FIELDOFFSET_IN_VAD = -1; // _6432_(-1, 24);
int MACRO_SIZEOF_VPN_IN_VAD = -1; // 4;
int MACRO_SIZEOF_VAD = 0; // 256; // N.B. this should be large enough to contain all the fields we read.
int MACRO_FILEOBJECT_FIELDOFFSET_IN_CONTROLAREA = -1; // _6432_(-1, 0x24);
int MACRO_FILENAME_FIELDOFFSET_IN_FILEOBJECT = -1; // _6432_(0x58, 0x30);
int MACRO_STARTINGVPNHIGH_FIELDOFFSET_IN_VAD = -1; // _6432_(0x20, -1);
int MACRO_ENDINGVPNHIGH_FIELDOFFSET_IN_VAD = -1; // _6432_(0x21, -1);

int MACRO_SUBSECTION_FIELDOFFSET_IN_VAD = -1; // _6432_(0x48, -1);
int MACRO_CONTROLAREA_FIELDOFFSET_IN_SUBSECTION = -1; // _6432_(0, 0); // present on XP, but used only on Win 11
int MACRO_FILEPOINTER_FIELDOFFSET_IN_CONTROLAREA = -1; // _6432_(0x40, -1);
ULONG64 MACRO_MASKOF_EXFASTREF = 0; // _6432_(0xFFFFFFFFFFFFFFF0, 0xFFFFFFFFFFFFFFF8); // can be 4 or 3 bits wide, depending on the architecture (64 or 32 bits respectively) - starting with Windows Vista.

int MACRO_PSGETNEXTPROCESS_OFFSET = -1;

int MACRO_PEB_FIELDOFFSET_IN_EPROCESS = -1;
int MACRO_LDR_FIELDOFFSET_IN_PEB = -1;
int MACRO_INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA = -1;
int MACRO_INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1;
int MACRO_DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1;
int MACRO_ENTRYPOINT_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1; // unused, but value of this var is valid.
int MACRO_SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1;
int MACRO_FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1;
int MACRO_BASEDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1; // unused, but value of this var is valid.

int MACRO_WOW64_PROCESS_FIELDOFFSET_IN_EPROCESS = -1;
int MACRO_WOW64_PEB_FIELDOFFSET_IN_WOW64PROCESS = -1;
int MACRO_WOW64_LDR_FIELDOFFSET_IN_PEB32 = -1;
int MACRO_WOW64_INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA = -1;
int MACRO_WOW64_INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1;
int MACRO_WOW64_DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1;
int MACRO_WOW64_ENTRYPOINT_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1;
int MACRO_WOW64_SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1;
int MACRO_WOW64_FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1;
int MACRO_WOW64_BASEDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY = -1;

int MACRO_IRQL_FIELDOFFSET_IN_PCR = -1;

#undef abc

//======================================================
// 
// DiscoverBytePointerPosInModules Function Definition.
// 
// This function comes from the first BugChecker, so it was
// compatible only with Windows 2000/XP 32bit. It was then
// ported to 64bit, made compatible with Windows 10/11 and
// finally integrated with the Mem Reader Coroutine mechanism.
// This is the reason why it is a bit messy...
// 
// NOTE AFTER SEVERAL WEEKS: =FIXFIX= now BugChecker has an
// heap allocator that works at any IRQL, and support for
// C++ 20 coroutines: we should update this function
// accordingly...
// 
//======================================================

MemoryReaderCoroutineState* Platform::DiscoverBytePointerPosInModules(MemoryReaderCoroutineState* pMemReadCor)
{
	auto params = (DiscoverBytePointerPosInModules_Params*)pMemReadCor->InputParams;

	switch (pMemReadCor->ResumePoint)
	{
	case 1: goto Resume_Label_01;
	case 2: goto Resume_Label_02;
	case 3: goto Resume_Label_03;
	case 4: goto Resume_Label_04;
	case 5: goto Resume_Label_05;
	case 6: goto Resume_Label_06;
	case 7: goto Resume_Label_07;
	case 8: goto Resume_Label_08;
	case 9: goto Resume_Label_09;
	case 10: goto Resume_Label_10;
	case 11: goto Resume_Label_11;
	case 12: goto Resume_Label_12;
	case 13: goto Resume_Label_13;
	case 14: goto Resume_Label_14;
	case 15: goto Resume_Label_15;
	case 16: goto Resume_Label_16;
	case 17: goto Resume_Label_17;
	case 18: goto Resume_Label_18;
	case 19: goto Resume_Label_19;
	case 20: goto Resume_Label_20;
	case 21: goto Resume_Label_21;
	case 22: goto Resume_Label_22;
	case 23: goto Resume_Label_23;
	}

	static VOID* pvMatchingModuleStart;
	static ULONG ulMatchingModuleLength;
	static BYTE* pbMatchingModuleNtHeaders;
	static CHAR szImageName[256];
	static ULONG ulSizeofNtHeaders;
	static IMAGE_FILE_HEADER ifhFileHeader;
	static IMAGE_DATA_DIRECTORY iddDirDebug;
	static eastl::vector<NtModule>* pvNtModules;
	static NtModule* pNtModule;

	iddDirDebug.VirtualAddress = 0;
	iddDirDebug.Size = 0;

	ulSizeofNtHeaders = sizeof(IMAGE_NT_HEADERS);

	if (params->pszOutputBuffer)
		strcpy(params->pszOutputBuffer, "");

	if (params->pidiDebugInfo)
		::memset(params->pidiDebugInfo, 0, sizeof(ImageDebugInfo));

	pvNtModules = NULL;
	pNtModule = NULL;

	// Check whether the Pointer refers to User Memory or Kernel Memory.

	pvMatchingModuleStart = NULL;
	ulMatchingModuleLength = 0;
	pbMatchingModuleNtHeaders = NULL;

	strcpy(szImageName, "");

	if ((ULONG_PTR)params->pbBytePointer < (ULONG_PTR)params->pvMmUserProbeAddress)
	{
		static BYTE* pbPcr;
		static BYTE* pbKteb;
		static BYTE* pbKpeb;
		static BYTE* pbVadRoot;
		static BYTE* pbMatchingNode;
		static BYTE* pbNode;
		static QWORD qwStart, qwEnd, qwPointer;

		pbPcr = (BYTE*)params->pvPCRAddress;

		//
		// USER SPACE.
		//

		// Get the Root Address of the VAT.

		return pMemReadCor->ReadMemory(19, (ULONG_PTR*)(pbPcr + MACRO_KTEBPTR_FIELDOFFSET_IN_PCR));
	Resume_Label_19:

		if (pMemReadCor->ReadMemory_Succeeded())
		{
			pbKteb = *(BYTE**)pMemReadCor->Buffer;

			if (pbKteb)
			{
				return pMemReadCor->ReadMemory(20, (ULONG_PTR*)(pbKteb + MACRO_KPEBPTR_FIELDOFFSET_IN_KTEB));
			Resume_Label_20:

				if (pMemReadCor->ReadMemory_Succeeded())
				{
					pbKpeb = *(BYTE**)pMemReadCor->Buffer;

					if (pbKpeb)
					{
						if (Root::I->NtModules)
						{
							auto fif = eastl::find_if(Root::I->NtModules->begin(), Root::I->NtModules->end(),
								[&](const eastl::pair<ULONG64, eastl::vector<NtModule>>& e) { return e.first == (ULONG64)(ULONG_PTR)pbKpeb; });

							if (fif != Root::I->NtModules->end())
							{
								pvNtModules = &fif->second;
							}
						}

						return pMemReadCor->ReadMemory(21, (ULONG_PTR*)(pbKpeb + MACRO_VADROOT_FIELDOFFSET_IN_KPEB));
					Resume_Label_21:

						if (pMemReadCor->ReadMemory_Succeeded())
						{
							pbVadRoot = *(BYTE**)pMemReadCor->Buffer;

							if (pbVadRoot)
							{
								pbMatchingNode = NULL;

								qwPointer = (QWORD)params->pbBytePointer;

								// Check this Node.

								pbNode = pbVadRoot;

								while (pbNode)
								{
									return pMemReadCor->ReadMemory(18, pbNode, MACRO_SIZEOF_VAD);
								Resume_Label_18:

									auto temp_origNodePtr = pbNode;

									if (pMemReadCor->ReadMemory_Succeeded())
										pbNode = (BYTE*)pMemReadCor->Buffer;
									else
										break;

									// Calculate the start and end address of the node.

									if (MACRO_SIZEOF_VPN_IN_VAD == 4)
									{
										qwStart = *(DWORD*)(pbNode + MACRO_STARTINGVPN_FIELDOFFSET_IN_VAD);
										qwEnd = *(DWORD*)(pbNode + MACRO_ENDINGVPN_FIELDOFFSET_IN_VAD);
									}
									else if (MACRO_SIZEOF_VPN_IN_VAD == 8)
									{
										qwStart = *(QWORD*)(pbNode + MACRO_STARTINGVPN_FIELDOFFSET_IN_VAD);
										qwEnd = *(QWORD*)(pbNode + MACRO_ENDINGVPN_FIELDOFFSET_IN_VAD);
									}
									else
										break;

									qwStart = qwStart << 12;
									qwEnd = ((qwEnd + 1) << 12) - 1;

									if (MACRO_STARTINGVPNHIGH_FIELDOFFSET_IN_VAD != -1 && MACRO_ENDINGVPNHIGH_FIELDOFFSET_IN_VAD != -1)
									{
										qwStart |= (QWORD) * (BYTE*)(pbNode + MACRO_STARTINGVPNHIGH_FIELDOFFSET_IN_VAD) << 44;
										qwEnd |= (QWORD) * (BYTE*)(pbNode + MACRO_ENDINGVPNHIGH_FIELDOFFSET_IN_VAD) << 44;
									}

									if (qwStart > qwEnd)
										break;

									// Check if we have a match.

									if (qwPointer >= qwStart &&
										qwPointer <= qwEnd)
									{
										pbMatchingNode = temp_origNodePtr;
										break;
									}
									else if (qwPointer < qwStart)
									{
										pbNode = *(BYTE**)(pbNode + MACRO_LEFTLINK_FIELDOFFSET_IN_VAD);
									}
									else
									{
										pbNode = *(BYTE**)(pbNode + MACRO_RIGHTLINK_FIELDOFFSET_IN_VAD);
									}
								}

								if (pbMatchingNode)
								{
									static ULONG_PTR ImageBase;
									static IMAGE_DOS_HEADER* pidhDosHdr;
									static IMAGE_NT_HEADERS* pinhNtHdrs;
									static ULONG_PTR* FileObjectPtr;
									static BOOLEAN IsExFastRef;
									static WORD* pwImageNameLengthPtr;
									static WORD wImageNameLength;
									static ULONG_PTR* ImageNameUnicodePtrPtr;
									static WORD* pwImageNameUnicodePtr;
									static WORD* pwWordPtr;
									static CHAR* pcCharPtr;
									static ULONG ulI;
									static BYTE* pbControlArea;
									static BYTE* pbFileObject;

									ImageBase = (ULONG_PTR)qwStart;

									pvMatchingModuleStart = (VOID*)ImageBase;

									// Check the DOS Header.

									pidhDosHdr = (IMAGE_DOS_HEADER*)ImageBase;

									return pMemReadCor->ReadMemory(9, (BYTE*)pidhDosHdr, sizeof(IMAGE_DOS_HEADER));
								Resume_Label_09:

									if (pMemReadCor->ReadMemory_Succeeded())
									{
										pidhDosHdr = (IMAGE_DOS_HEADER*)pMemReadCor->Buffer;

										if (pidhDosHdr->e_magic == IMAGE_DOS_SIGNATURE &&
											pidhDosHdr->e_lfarlc >= 0x40)
										{
											// Check the NT Headers.

											pinhNtHdrs = (IMAGE_NT_HEADERS*)(ImageBase + pidhDosHdr->e_lfanew);

											return pMemReadCor->ReadMemory(10, (BYTE*)pinhNtHdrs, sizeof(IMAGE_NT_HEADERS64) > sizeof(IMAGE_NT_HEADERS32) ? sizeof(IMAGE_NT_HEADERS64) : sizeof(IMAGE_NT_HEADERS32));
										Resume_Label_10:

											if (pMemReadCor->ReadMemory_Succeeded())
											{
												IMAGE_NT_HEADERS* temp_NtHdrs;
												temp_NtHdrs = (IMAGE_NT_HEADERS*)pMemReadCor->Buffer;

												if (temp_NtHdrs->Signature == IMAGE_NT_SIGNATURE &&
													(temp_NtHdrs->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC || temp_NtHdrs->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC))
												{
													ifhFileHeader = temp_NtHdrs->FileHeader; // N.B. This is required later!

													ULONG temp_SizeOfImage;
													temp_SizeOfImage = 0;

													if (temp_NtHdrs->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
													{
														temp_SizeOfImage = ((IMAGE_NT_HEADERS32*)temp_NtHdrs)->OptionalHeader.SizeOfImage;
														ulSizeofNtHeaders = sizeof(IMAGE_NT_HEADERS32);
														iddDirDebug = ((IMAGE_NT_HEADERS32*)temp_NtHdrs)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
													}
													else if (temp_NtHdrs->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
													{
														temp_SizeOfImage = ((IMAGE_NT_HEADERS64*)temp_NtHdrs)->OptionalHeader.SizeOfImage;
														ulSizeofNtHeaders = sizeof(IMAGE_NT_HEADERS64);
														iddDirDebug = ((IMAGE_NT_HEADERS64*)temp_NtHdrs)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
													}

													if (((ULONG_PTR)params->pbBytePointer) >= ImageBase &&
														((ULONG_PTR)params->pbBytePointer) <= ImageBase + temp_SizeOfImage - 1)
													{
														// Setup the Variables required later.

														ulMatchingModuleLength = temp_SizeOfImage;
														pbMatchingModuleNtHeaders = (BYTE*)pinhNtHdrs;

														// Sometimes Windows 10/11 obfuscates the image name.

														::strcpy(szImageName, "???");

														// Get the Full Name of the Specified Module.

														FileObjectPtr = NULL;
														IsExFastRef = FALSE;

														if (MACRO_CONTROLAREA_FIELDOFFSET_IN_VAD != -1 &&
															MACRO_FILEOBJECT_FIELDOFFSET_IN_CONTROLAREA != -1)
														{
															ULONG_PTR* temp_ControlAreaPtr;

															temp_ControlAreaPtr = (ULONG_PTR*)(pbMatchingNode + MACRO_CONTROLAREA_FIELDOFFSET_IN_VAD);

															return pMemReadCor->ReadMemory(12, temp_ControlAreaPtr);
														Resume_Label_12:

															if (pMemReadCor->ReadMemory_Succeeded())
															{
																pbControlArea = *(BYTE**)pMemReadCor->Buffer;

																if (pbControlArea)
																{
																	FileObjectPtr = (ULONG_PTR*)(pbControlArea + MACRO_FILEOBJECT_FIELDOFFSET_IN_CONTROLAREA);
																}
															}
														}
														else if (MACRO_SUBSECTION_FIELDOFFSET_IN_VAD != -1 &&
															MACRO_CONTROLAREA_FIELDOFFSET_IN_SUBSECTION != -1 &&
															MACRO_FILEPOINTER_FIELDOFFSET_IN_CONTROLAREA != -1)
														{
															ULONG_PTR* temp_SubsectionPtr;
															temp_SubsectionPtr = (ULONG_PTR*)(pbMatchingNode + MACRO_SUBSECTION_FIELDOFFSET_IN_VAD);

															return pMemReadCor->ReadMemory(13, temp_SubsectionPtr);
														Resume_Label_13:

															if (pMemReadCor->ReadMemory_Succeeded())
															{
																BYTE* temp_Subsection;
																temp_Subsection = *(BYTE**)pMemReadCor->Buffer;

																ULONG_PTR* temp_ControlAreaPtr;
																temp_ControlAreaPtr = (ULONG_PTR*)(temp_Subsection + MACRO_CONTROLAREA_FIELDOFFSET_IN_SUBSECTION);

																return pMemReadCor->ReadMemory(14, temp_ControlAreaPtr);
															Resume_Label_14:

																if (pMemReadCor->ReadMemory_Succeeded())
																{
																	auto temp_ControlArea = *(BYTE**)pMemReadCor->Buffer;

																	FileObjectPtr = (ULONG_PTR*)(temp_ControlArea + MACRO_FILEPOINTER_FIELDOFFSET_IN_CONTROLAREA);
																	IsExFastRef = TRUE;
																}
															}
														}

														return pMemReadCor->ReadMemory(11, FileObjectPtr);
													Resume_Label_11:

														if (pMemReadCor->ReadMemory_Succeeded())
														{
															pbFileObject = *(BYTE**)pMemReadCor->Buffer;

															if (IsExFastRef)
																pbFileObject = (BYTE*)((ULONG_PTR)pbFileObject & (ULONG_PTR)MACRO_MASKOF_EXFASTREF);

															pwImageNameLengthPtr = (WORD*)(pbFileObject + MACRO_FILENAME_FIELDOFFSET_IN_FILEOBJECT);

															// Get the Full Name of the Module.

															return pMemReadCor->ReadMemory(15, pwImageNameLengthPtr);
														Resume_Label_15:

															if (pMemReadCor->ReadMemory_Succeeded())
															{
																pwImageNameLengthPtr = (WORD*)pMemReadCor->Buffer;

																wImageNameLength = *pwImageNameLengthPtr / sizeof(WORD);

																if (wImageNameLength != 0 &&
																	wImageNameLength <= sizeof(szImageName) - 1)
																{
																	ImageNameUnicodePtrPtr = (ULONG_PTR*)(pbFileObject +
																		MACRO_FILENAME_FIELDOFFSET_IN_FILEOBJECT + FIELD_OFFSET(UNICODE_STRING, Buffer));

																	return pMemReadCor->ReadMemory(16, ImageNameUnicodePtrPtr);
																Resume_Label_16:

																	if (pMemReadCor->ReadMemory_Succeeded())
																	{
																		ImageNameUnicodePtrPtr = (ULONG_PTR*)pMemReadCor->Buffer;

																		pwImageNameUnicodePtr = (WORD*)*ImageNameUnicodePtrPtr;

																		if (pwImageNameUnicodePtr != NULL)
																		{
																			return pMemReadCor->ReadMemory(17, pwImageNameUnicodePtr, (wImageNameLength + 1) * sizeof(WORD));
																		Resume_Label_17:

																			if (pMemReadCor->ReadMemory_Succeeded())
																			{
																				pwImageNameUnicodePtr = (WORD*)pMemReadCor->Buffer;

																				pwWordPtr = pwImageNameUnicodePtr;
																				pcCharPtr = szImageName;

																				for (ulI = 0; ulI < wImageNameLength; ulI++)
																					*pcCharPtr++ = (CHAR)((*pwWordPtr++) & 0xFF);

																				*pcCharPtr = '\0';
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		static WORD* pwImageNameLengthPtr;
		static WORD wImageNameLength;
		static WORD* pwWordPtr;
		static CHAR* pcCharPtr;
		static ULONG ulI;
		static WORD* pwImageNameUnicodePtr;
		static ULONG_PTR* pImageNameUnicodePtrPtr;
		static ULONG_PTR ImageBase;
		static IMAGE_DOS_HEADER* pidhDosHdr;
		static IMAGE_NT_HEADERS* pinhNtHdrs;
		static LIST_ENTRY* pleListNodePtr;
		static ULONG_PTR* pImageBasePtr;
		static LIST_ENTRY* pleThis;
		static BOOLEAN eraseCacheOn1stAdd;

		//
		// PsLoadedModuleList lookups are slow: this cache was introduced later to speed up the process.
		//

		class CacheEntry
		{
		public:
			LIST_ENTRY* entry;
			ULONG_PTR imageBase;
			ULONG imageSize;
		};

		static eastl::vector<CacheEntry>* cache = NULL;

		if (!cache)
			cache = new eastl::vector<CacheEntry>(); // allocated in BC's high-irql pool: no need to free.

		eraseCacheOn1stAdd = TRUE;

		//
		// KERNEL SPACE.
		//

		pleListNodePtr = (LIST_ENTRY*)params->pvPsLoadedModuleList;

		while (TRUE)
		{
			if (pleListNodePtr == NULL)
				break;

			// first iteration on PsLoadedModuleList: check the cache.

			pleThis = pleListNodePtr;

			if (pleThis == (LIST_ENTRY*)params->pvPsLoadedModuleList)
			{
				auto ptr = (ULONG_PTR)params->pbBytePointer;

				auto res = eastl::find_if(cache->begin(), cache->end(),
					[&ptr](const CacheEntry& e) { return ptr >= e.imageBase && ptr < e.imageBase + e.imageSize; });

				if (res != cache->end())
					pleThis = res->entry;
			}

			// Check the Module Start Address.

			pImageBasePtr = (ULONG_PTR*)(((BYTE*)pleThis) + MACRO_IMAGEBASE_FIELDOFFSET_IN_DRVSEC);

			return pMemReadCor->ReadMemory(1, pImageBasePtr);
		Resume_Label_01:

			if (pMemReadCor->ReadMemory_Succeeded())
			{
				pImageBasePtr = (ULONG_PTR*)pMemReadCor->Buffer;

				ImageBase = *pImageBasePtr;

				// Compare the Image Base and the Specified Pointer.

				if (((ULONG_PTR)params->pbBytePointer) >= ImageBase)
				{
					// Check the DOS Header.

					pidhDosHdr = (IMAGE_DOS_HEADER*)ImageBase;

					return pMemReadCor->ReadMemory(2, (BYTE*)pidhDosHdr, sizeof(IMAGE_DOS_HEADER));
				Resume_Label_02:

					if (pMemReadCor->ReadMemory_Succeeded())
					{
						pidhDosHdr = (IMAGE_DOS_HEADER*)pMemReadCor->Buffer;

						if (pidhDosHdr->e_magic == IMAGE_DOS_SIGNATURE &&
							pidhDosHdr->e_lfarlc >= 0x40)
						{
							// Check the NT Headers.

							pinhNtHdrs = (IMAGE_NT_HEADERS*)(ImageBase + pidhDosHdr->e_lfanew);

							return pMemReadCor->ReadMemory(3, (BYTE*)pinhNtHdrs, sizeof(IMAGE_NT_HEADERS));
						Resume_Label_03:

							if (pMemReadCor->ReadMemory_Succeeded())
							{
								ULONG temp_Signature;
								ULONG temp_SizeOfImage;

								ifhFileHeader = ((IMAGE_NT_HEADERS*)pMemReadCor->Buffer)->FileHeader; // N.B. This is required later!
								temp_Signature = ((IMAGE_NT_HEADERS*)pMemReadCor->Buffer)->Signature;
								temp_SizeOfImage = ((IMAGE_NT_HEADERS*)pMemReadCor->Buffer)->OptionalHeader.SizeOfImage; // we are in the kernel, so OptHdr size is the same as our driver's image.
								iddDirDebug = ((IMAGE_NT_HEADERS*)pMemReadCor->Buffer)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

								if (temp_Signature == IMAGE_NT_SIGNATURE)
								{
									// add the image to the cache.

									if (pleThis == pleListNodePtr) // no cache hit
									{
										if (eraseCacheOn1stAdd)
										{
											eraseCacheOn1stAdd = FALSE;
											cache->clear();
										}

										cache->push_back(CacheEntry{ pleThis, ImageBase, temp_SizeOfImage });
									}

									// Check if the image contains the pointer.

									if (((ULONG_PTR)params->pbBytePointer) <= ImageBase + temp_SizeOfImage - 1)
									{
										// Setup the Variables required later.

										pvMatchingModuleStart = (VOID*)ImageBase;
										ulMatchingModuleLength = temp_SizeOfImage;
										pbMatchingModuleNtHeaders = (BYTE*)pinhNtHdrs;

										// Sometimes Windows 10/11 obfuscates the image name.

										::strcpy(szImageName, "???");

										// Get the Name of the Module.

										pwImageNameLengthPtr = (WORD*)(((BYTE*)pleThis) + MACRO_IMAGENAME_FIELDOFFSET_IN_DRVSEC);

										return pMemReadCor->ReadMemory(4, pwImageNameLengthPtr);
									Resume_Label_04:

										if (pMemReadCor->ReadMemory_Succeeded())
										{
											pwImageNameLengthPtr = (WORD*)pMemReadCor->Buffer;

											wImageNameLength = *pwImageNameLengthPtr / sizeof(WORD);

											if (wImageNameLength != 0 &&
												wImageNameLength <= sizeof(szImageName) - 1)
											{
												pImageNameUnicodePtrPtr = (ULONG_PTR*)(((BYTE*)pleThis) +
													MACRO_IMAGENAME_FIELDOFFSET_IN_DRVSEC + FIELD_OFFSET(UNICODE_STRING, Buffer));

												return pMemReadCor->ReadMemory(5, pImageNameUnicodePtrPtr);
											Resume_Label_05:

												if (pMemReadCor->ReadMemory_Succeeded())
												{
													pImageNameUnicodePtrPtr = (ULONG_PTR*)pMemReadCor->Buffer;

													pwImageNameUnicodePtr = (WORD*)*pImageNameUnicodePtrPtr;

													if (pwImageNameUnicodePtr != NULL)
													{
														return pMemReadCor->ReadMemory(6, pwImageNameUnicodePtr, (wImageNameLength + 1) * sizeof(WORD));
													Resume_Label_06:

														if (pMemReadCor->ReadMemory_Succeeded())
														{
															pwImageNameUnicodePtr = (WORD*)pMemReadCor->Buffer;

															pwWordPtr = pwImageNameUnicodePtr;
															pcCharPtr = szImageName;

															for (ulI = 0; ulI < wImageNameLength; ulI++)
																*pcCharPtr++ = (CHAR)((*pwWordPtr++) & 0xFF);

															*pcCharPtr = '\0';
														}
													}
												}
											}
										}

										// Exit from the Loop.

										break;
									}
								}
							}
						}
					}
				}
			}

			// Get the Pointer to the Next Node.

			return pMemReadCor->ReadMemory(7, (ULONG_PTR*)(((BYTE*)pleListNodePtr) + FIELD_OFFSET(LIST_ENTRY, Flink)));
		Resume_Label_07:

			if (!(pMemReadCor->ReadMemory_Succeeded()))
				break;

			pleListNodePtr = *(LIST_ENTRY**)pMemReadCor->Buffer;

			if (pleListNodePtr == NULL ||
				pleListNodePtr == (LIST_ENTRY*)params->pvPsLoadedModuleList)
				break;
		}
	}

	// Save the debug info separetely.

	if (params->pidiDebugInfo &&
		pvMatchingModuleStart && ulMatchingModuleLength)
	{
		params->pidiDebugInfo->startAddr = (ULONG_PTR)pvMatchingModuleStart;
		params->pidiDebugInfo->length = ulMatchingModuleLength;

		if (iddDirDebug.VirtualAddress && iddDirDebug.Size)
		{
			return pMemReadCor->ReadMemory(22, (VOID*)((ULONG_PTR)pvMatchingModuleStart + iddDirDebug.VirtualAddress), iddDirDebug.Size);
		Resume_Label_22:

			if (pMemReadCor->ReadMemory_Succeeded())
			{
				const static int iddDebDir_size = 32; // ntoskrnl of Windows 11 has 4 of these, and Windows XP only 1...
				static IMAGE_DEBUG_DIRECTORY iddDebDir[iddDebDir_size];
				static ULONG nDebDir;
				static int i;

				IMAGE_DEBUG_DIRECTORY* temp_pDebDir;
				temp_pDebDir = (IMAGE_DEBUG_DIRECTORY*)pMemReadCor->Buffer;

				nDebDir = iddDirDebug.Size / sizeof(IMAGE_DEBUG_DIRECTORY);
				if (nDebDir > iddDebDir_size)
					nDebDir = iddDebDir_size;

				::memcpy(iddDebDir, temp_pDebDir, nDebDir * sizeof(IMAGE_DEBUG_DIRECTORY));

				for (i = 0; i < nDebDir; i++)
				{
					if (iddDebDir[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
					{
						return pMemReadCor->ReadMemory(23,
							(VOID*)((ULONG_PTR)pvMatchingModuleStart + iddDebDir[i].AddressOfRawData),
							iddDebDir[i].SizeOfData < 512 ? iddDebDir[i].SizeOfData : 512);
					Resume_Label_23:

						if (pMemReadCor->ReadMemory_Succeeded())
						{
							auto temp_pbCV = (BYTE*)pMemReadCor->Buffer;

							DWORD sig = *(DWORD*)temp_pbCV;

							if (sig == 'SDSR')
							{
								BYTE* guid = temp_pbCV + sizeof(DWORD);
								DWORD* age = (DWORD*)(guid + 16);

								CHAR* pdbn = (CHAR*)((BYTE*)age + sizeof(DWORD));
								ULONG nfn = iddDebDir[i].SizeOfData - (ULONG)((ULONG_PTR)pdbn - (ULONG_PTR)temp_pbCV);

								if (nfn > sizeof(params->pidiDebugInfo->szPdb) - 1)
									nfn = sizeof(params->pidiDebugInfo->szPdb) - 1;

								::memcpy(params->pidiDebugInfo->guid, guid, 16);

								params->pidiDebugInfo->age = *age;

								::memset(params->pidiDebugInfo->szPdb, 0, sizeof(params->pidiDebugInfo->szPdb));
								::memcpy(params->pidiDebugInfo->szPdb, pdbn, nfn);

								break;
							}
						}
					}
				}
			}
		}
	}

	// compensate with NtModules.

	if (pvNtModules && pvMatchingModuleStart)
	{
		auto fif = eastl::find_if(pvNtModules->begin(), pvNtModules->end(),
			[&](const NtModule& e) { return e.dllBase == (ULONG64)(ULONG_PTR)pvMatchingModuleStart; });

		if (fif != pvNtModules->end())
		{
			pNtModule = fif;
		}
	}

	if (pNtModule && (!::strlen(szImageName) || !::strcmp(szImageName, "???")))
	{
		ulMatchingModuleLength = pNtModule->sizeOfImage;
		if (::strlen(pNtModule->dllName))
			::strcpy(szImageName, pNtModule->dllName);
	}

	if (params->pidiDebugInfo && !::strlen(params->pidiDebugInfo->szPdb) && pNtModule)
	{
		params->pidiDebugInfo->startAddr = (ULONG_PTR)pNtModule->dllBase;
		params->pidiDebugInfo->length = pNtModule->sizeOfImage;
		::memcpy(params->pidiDebugInfo->guid, pNtModule->pdbGuid, 16);
		params->pidiDebugInfo->age = pNtModule->pdbAge;
		::strcpy(params->pidiDebugInfo->szPdb, pNtModule->pdbName);
	}

	// sometimes newer versions of Windows obfuscate the module name: compensate with pdb name, if possible.

	if (params->pidiDebugInfo && !::strcmp(szImageName, "???") && ::strlen(params->pidiDebugInfo->szPdb))
	{
		static CHAR sz0[sizeof(params->pidiDebugInfo->szPdb)];
		::strcpy(sz0, params->pidiDebugInfo->szPdb);

		CHAR* psz = &sz0[::strlen(sz0) - 1];

		for (; psz >= sz0; psz--)
			if (*psz == '.')
				*psz = '\0';
			else if (*psz == '\\' || *psz == '/')
				break;

		psz++;

		if (::strlen(psz))
			::strcpy(szImageName, psz);
	}

	// Check whether there is a Match.

	if (params->pszOutputBuffer &&
		pvMatchingModuleStart && ulMatchingModuleLength &&
		strlen(szImageName))
	{
		static IMAGE_SECTION_HEADER* pishSectionList;
		static ULONG ulI;
		static IMAGE_SECTION_HEADER* pishThis;
		static ULONG_PTR Start, End, Offset;
		static CHAR* pszImageNameToBePrinted;
		static BOOLEAN bPreserveModuleFileExtension;
		static CHAR szSectionName[64];

		bPreserveModuleFileExtension = FALSE; // TRUE

		// Set the Pointer to the Image Name String.

		pszImageNameToBePrinted = &szImageName[strlen(szImageName) - 1];

		for (; pszImageNameToBePrinted >= szImageName; pszImageNameToBePrinted--)
			if (*pszImageNameToBePrinted == '.' && bPreserveModuleFileExtension == FALSE)
				*pszImageNameToBePrinted = '\0';
			else if (*pszImageNameToBePrinted == '\\' ||
				*pszImageNameToBePrinted == '/')
				break;

		pszImageNameToBePrinted++;

		if (strlen(pszImageNameToBePrinted))
		{
			if (!pbMatchingModuleNtHeaders)
			{
				// Compose the Required String without Section Information.

				Offset = ((ULONG_PTR)params->pbBytePointer) - ((ULONG_PTR)pvMatchingModuleStart);

				if (Offset)
				{
					sprintf(params->pszOutputBuffer, "%s+%.8X",
						pszImageNameToBePrinted, (ULONG32)Offset);
				}
				else
				{
					sprintf(params->pszOutputBuffer, "%s",
						pszImageNameToBePrinted);
				}
			}
			else
			{
				// Check the Sections of the Image.

				pishSectionList = (IMAGE_SECTION_HEADER*)(((BYTE*)pbMatchingModuleNtHeaders) + ulSizeofNtHeaders);

				return pMemReadCor->ReadMemory(8, (BYTE*)pishSectionList, ifhFileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
			Resume_Label_08:

				if (pMemReadCor->ReadMemory_Succeeded())
				{
					pishSectionList = (IMAGE_SECTION_HEADER*)pMemReadCor->Buffer;

					// Iterate through the Section List Items.

					for (ulI = 0; ulI < ifhFileHeader.NumberOfSections; ulI++)
					{
						pishThis = &pishSectionList[ulI];

						if (pishThis->SizeOfRawData == 0)
							continue;

						Start = ((ULONG_PTR)pvMatchingModuleStart) + pishThis->VirtualAddress;
						End = Start + pishThis->SizeOfRawData - 1;

						if (((ULONG_PTR)params->pbBytePointer) >= Start &&
							((ULONG_PTR)params->pbBytePointer) <= End)
						{
							Offset = ((ULONG_PTR)params->pbBytePointer) - Start;

							// Compose the Required String.

							memset(szSectionName, 0, sizeof(szSectionName));
							memcpy(szSectionName, pishThis->Name, IMAGE_SIZEOF_SHORT_NAME);

							if (Offset)
							{
								sprintf(params->pszOutputBuffer, "%s!%s+%.8X",
									pszImageNameToBePrinted, szSectionName, (ULONG32)Offset);
							}
							else
							{
								sprintf(params->pszOutputBuffer, "%s!%s",
									pszImageNameToBePrinted, szSectionName);
							}

							// Return to the Caller.

							return pMemReadCor->ReadMemory_End();
						}
					}

					// Compose the Required String without Section Information.

					Offset = ((ULONG_PTR)params->pbBytePointer) - ((ULONG_PTR)pvMatchingModuleStart);

					if (Offset)
					{
						sprintf(params->pszOutputBuffer, "%s+%.8X",
							pszImageNameToBePrinted, (ULONG32)Offset);
					}
					else
					{
						sprintf(params->pszOutputBuffer, "%s",
							pszImageNameToBePrinted);
					}
				}
			}
		}
	}

	// Return to the Caller.

	return pMemReadCor->ReadMemory_End();
}

VOID* Platform::GetPcrAddress()
{
	return _6432_((VOID*)__readgsqword(MACRO_SELFPTR_FIELDOFFSET_IN_PCR), (VOID*)__readfsdword(MACRO_SELFPTR_FIELDOFFSET_IN_PCR));
}

VOID* Platform::GetCurrentThread()
{
	return *(VOID**)((ULONG_PTR)GetPcrAddress() + MACRO_KTEBPTR_FIELDOFFSET_IN_PCR);
}

KIRQL Platform::GetCurrentIrql()
{
	if (MACRO_IRQL_FIELDOFFSET_IN_PCR < 0) // should never happen!!
		return PASSIVE_LEVEL;

	return *(KIRQL*)((ULONG_PTR)GetPcrAddress() + MACRO_IRQL_FIELDOFFSET_IN_PCR);
}

VOID* Platform::GetKernelBaseAddress(ULONG64 PsLoadedModuleList)
{
	return *(VOID**)((ULONG_PTR)((LIST_ENTRY*)(ULONG_PTR)PsLoadedModuleList)->Flink + MACRO_IMAGEBASE_FIELDOFFSET_IN_DRVSEC);
}

eastl::string Platform::CalculateKernelOffsets(VOID* symbols)
{
	// define useful structs.

	int* offsVad[] = { // array of all the fields in a VAD.
		&MACRO_STARTINGVPN_FIELDOFFSET_IN_VAD,
		&MACRO_ENDINGVPN_FIELDOFFSET_IN_VAD,
		&MACRO_LEFTLINK_FIELDOFFSET_IN_VAD,
		&MACRO_RIGHTLINK_FIELDOFFSET_IN_VAD,
		&MACRO_CONTROLAREA_FIELDOFFSET_IN_VAD,
		&MACRO_STARTINGVPNHIGH_FIELDOFFSET_IN_VAD,
		&MACRO_ENDINGVPNHIGH_FIELDOFFSET_IN_VAD,
		&MACRO_SUBSECTION_FIELDOFFSET_IN_VAD };

	int* offsCanBeNull[] = { // array of all the fields that can be -1, due to platform differences.
		&MACRO_CONTROLAREA_FIELDOFFSET_IN_VAD, // xp
		&MACRO_FILEOBJECT_FIELDOFFSET_IN_CONTROLAREA, // xp
		&MACRO_SUBSECTION_FIELDOFFSET_IN_VAD, // win 11
		&MACRO_CONTROLAREA_FIELDOFFSET_IN_SUBSECTION, // win 11
		&MACRO_FILEPOINTER_FIELDOFFSET_IN_CONTROLAREA, // win 11
		&MACRO_STARTINGVPNHIGH_FIELDOFFSET_IN_VAD, // win 11
		&MACRO_ENDINGVPNHIGH_FIELDOFFSET_IN_VAD, // win 11
		_6432_(NULL, &MACRO_WOW64_PROCESS_FIELDOFFSET_IN_EPROCESS),
		&MACRO_WOW64_PEB_FIELDOFFSET_IN_WOW64PROCESS,
		_6432_(NULL, &MACRO_WOW64_LDR_FIELDOFFSET_IN_PEB32),
		_6432_(NULL, &MACRO_WOW64_INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA),
		_6432_(NULL, &MACRO_WOW64_INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY),
		_6432_(NULL, &MACRO_WOW64_DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY),
		_6432_(NULL, &MACRO_WOW64_ENTRYPOINT_FIELDOFFSET_IN_LDRDATATABLEENTRY),
		_6432_(NULL, &MACRO_WOW64_SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY),
		_6432_(NULL, &MACRO_WOW64_FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY),
		_6432_(NULL, &MACRO_WOW64_BASEDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY)
	};

	// define useful lambdas.

	BCSFILE_DATATYPE_MEMBER notFound = {};
	notFound.offset = -1;

	auto getMember = [&symbols, &notFound](const char* expectedType, const char** types, const char** members) -> const BCSFILE_DATATYPE_MEMBER* {

		for (int i = 0; types[i]; i++)
		{
			auto type = Symbols::GetDatatype(symbols, types[i]);
			if (type)
			{
				for (int j = 0; members[j]; j++)
				{
					auto member = Symbols::GetDatatypeMember(symbols, type, members[j], expectedType);
					if (member)
						return member;
				}
			}
		}

		return &notFound;
	};

	auto assign = [&offsCanBeNull](int& dest, const int& src) {

		if (src == -1) {

			for (int* f : offsCanBeNull)
				if (f == &dest)
					return TRUE;

			return FALSE;
		}

		dest = src;

		return TRUE;
	};

	auto assign2 = [](ULONG64& dest, const ULONG64& src) {

		if (!src)
			return FALSE;

		dest = src;

		return TRUE;
	};

	typedef const char* A[];

	// calculate the offsets.

	if (!assign(MACRO_SELFPTR_FIELDOFFSET_IN_PCR, getMember("_KPCR *", A{ "_KPCR", NULL }, A{ "SelfPcr", "Self", NULL })->offset))
		return "MACRO_SELFPTR_FIELDOFFSET_IN_PCR";

	{
		int res = -1;

		auto Prcb = getMember("_KPRCB", A{ "_KPCR", NULL }, A{ "PrcbData", "Prcb", NULL })->offset;

		auto CurrentThread = getMember("_KTHREAD *", A{ "_KPRCB", NULL }, A{ "CurrentThread", NULL })->offset;

		if (Prcb != notFound.offset && CurrentThread != notFound.offset)
			res = Prcb + CurrentThread;

		if (!assign(MACRO_KTEBPTR_FIELDOFFSET_IN_PCR, res))
			return "MACRO_KTEBPTR_FIELDOFFSET_IN_PCR";
	}

	{
		int res = -1;

		auto ApcState = getMember("_KAPC_STATE", A{ "_KTHREAD", NULL }, A{ "ApcState", NULL })->offset;

		auto Process = getMember("_KPROCESS *", A{ "_KAPC_STATE", NULL }, A{ "Process", NULL })->offset;

		if (ApcState != notFound.offset && Process != notFound.offset)
			res = ApcState + Process;

		if (!assign(MACRO_KPEBPTR_FIELDOFFSET_IN_KTEB, res))
			return "MACRO_KPEBPTR_FIELDOFFSET_IN_KTEB";
	}

	if (!assign(MACRO_VADROOT_FIELDOFFSET_IN_KPEB, getMember(NULL, A{ "_EPROCESS", NULL }, A{ "VadRoot", NULL })->offset))
		return "MACRO_VADROOT_FIELDOFFSET_IN_KPEB";

	if (!assign(MACRO_IMAGEBASE_FIELDOFFSET_IN_DRVSEC, getMember("void *", A{ "_LDR_DATA_TABLE_ENTRY", NULL }, A{ "DllBase", NULL })->offset))
		return "MACRO_IMAGEBASE_FIELDOFFSET_IN_DRVSEC";

	if (!assign(MACRO_IMAGENAME_FIELDOFFSET_IN_DRVSEC, getMember("_UNICODE_STRING", A{ "_LDR_DATA_TABLE_ENTRY", NULL }, A{ "FullDllName", NULL })->offset))
		return "MACRO_IMAGENAME_FIELDOFFSET_IN_DRVSEC";

	{
		auto StartingVpn = getMember(NULL, A{ "_MMVAD_SHORT", NULL }, A{ "StartingVpn", NULL });

		if (!assign(MACRO_STARTINGVPN_FIELDOFFSET_IN_VAD, StartingVpn->offset))
			return "MACRO_STARTINGVPN_FIELDOFFSET_IN_VAD";

		if (!assign(MACRO_ENDINGVPN_FIELDOFFSET_IN_VAD, getMember(NULL, A{ "_MMVAD_SHORT", NULL }, A{ "EndingVpn", NULL })->offset))
			return "MACRO_ENDINGVPN_FIELDOFFSET_IN_VAD";

		int sz = -1;

		if (StartingVpn->offset != notFound.offset)
		{
			auto datatype = Symbols::GetNameFromOffset(symbols, StartingVpn->datatype);

			if (!::strcmp(datatype, "ulong"))
				sz = 4;
			else if (!::strcmp(datatype, "__uint64"))
				sz = 8;
		}

		if (!assign(MACRO_SIZEOF_VPN_IN_VAD, sz))
			return "MACRO_SIZEOF_VPN_IN_VAD";
	}

	{
		auto left = getMember("_MMVAD *", A{ "_MMVAD_SHORT", NULL }, A{ "LeftChild", NULL })->offset; // windows xp
		auto right = getMember("_MMVAD *", A{ "_MMVAD_SHORT", NULL }, A{ "RightChild", NULL })->offset; // windows xp

		if (left == notFound.offset || right == notFound.offset)
		{
			auto VadNode = getMember("_RTL_BALANCED_NODE", A{ "_MMVAD_SHORT", NULL }, A{ "VadNode", NULL })->offset; // windows 11

			if (VadNode != notFound.offset)
			{
				auto left2 = getMember("_RTL_BALANCED_NODE *", A{ "_RTL_BALANCED_NODE", NULL }, A{ "Left", NULL })->offset;
				auto right2 = getMember("_RTL_BALANCED_NODE *", A{ "_RTL_BALANCED_NODE", NULL }, A{ "Right", NULL })->offset;

				if (left2 != notFound.offset && right2 != notFound.offset)
				{
					left = VadNode + left2;
					right = VadNode + right2;
				}
			}
		}

		if (!assign(MACRO_LEFTLINK_FIELDOFFSET_IN_VAD, left))
			return "MACRO_LEFTLINK_FIELDOFFSET_IN_VAD";

		if (!assign(MACRO_RIGHTLINK_FIELDOFFSET_IN_VAD, right))
			return "MACRO_RIGHTLINK_FIELDOFFSET_IN_VAD";
	}

	if (!assign(MACRO_CONTROLAREA_FIELDOFFSET_IN_VAD, getMember("_CONTROL_AREA *", A{ "_MMVAD", NULL }, A{ "ControlArea", NULL })->offset)) // xp only
		return "MACRO_CONTROLAREA_FIELDOFFSET_IN_VAD";

	if (!assign(MACRO_FILEOBJECT_FIELDOFFSET_IN_CONTROLAREA, getMember("_FILE_OBJECT *", A{ "_CONTROL_AREA", NULL }, A{ "FilePointer", NULL })->offset)) // xp only
		return "MACRO_FILEOBJECT_FIELDOFFSET_IN_CONTROLAREA";

	if (!assign(MACRO_FILENAME_FIELDOFFSET_IN_FILEOBJECT, getMember("_UNICODE_STRING", A{ "_FILE_OBJECT", NULL }, A{ "FileName", NULL })->offset))
		return "MACRO_FILENAME_FIELDOFFSET_IN_FILEOBJECT";

	if (!assign(MACRO_STARTINGVPNHIGH_FIELDOFFSET_IN_VAD, getMember("uchar", A{ "_MMVAD_SHORT", NULL }, A{ "StartingVpnHigh", NULL })->offset)) // win 11 only
		return "MACRO_STARTINGVPNHIGH_FIELDOFFSET_IN_VAD";

	if (!assign(MACRO_ENDINGVPNHIGH_FIELDOFFSET_IN_VAD, getMember("uchar", A{ "_MMVAD_SHORT", NULL }, A{ "EndingVpnHigh", NULL })->offset)) // win 11 only
		return "MACRO_ENDINGVPNHIGH_FIELDOFFSET_IN_VAD";

	if (!assign(MACRO_SUBSECTION_FIELDOFFSET_IN_VAD, getMember("_SUBSECTION *", A{ "_MMVAD", NULL }, A{ "Subsection", NULL })->offset)) // win 11 only
		return "MACRO_SUBSECTION_FIELDOFFSET_IN_VAD";

	if (!assign(MACRO_CONTROLAREA_FIELDOFFSET_IN_SUBSECTION, getMember("_CONTROL_AREA *", A{ "_SUBSECTION", NULL }, A{ "ControlArea", NULL })->offset)) // win 11 only
		return "MACRO_CONTROLAREA_FIELDOFFSET_IN_SUBSECTION";

	if (!assign(MACRO_FILEPOINTER_FIELDOFFSET_IN_CONTROLAREA, getMember("_EX_FAST_REF", A{ "_CONTROL_AREA", NULL }, A{ "FilePointer", NULL })->offset)) // win 11 only
		return "MACRO_FILEPOINTER_FIELDOFFSET_IN_CONTROLAREA";

	{
		ULONG64 res = 0;

		auto RefCnt3 = getMember("ulong", A{ "_EX_FAST_REF", NULL }, A{ "RefCnt:0x3:0x0", NULL })->offset;
		auto RefCnt4 = getMember("__uint64", A{ "_EX_FAST_REF", NULL }, A{ "RefCnt:0x4:0x0", NULL })->offset;

		if (RefCnt3 != notFound.offset)
			res = 0xFFFFFFFFFFFFFFF8;
		else if (RefCnt4 != notFound.offset)
			res = 0xFFFFFFFFFFFFFFF0;

		if (!assign2(MACRO_MASKOF_EXFASTREF, res))
			return "MACRO_MASKOF_EXFASTREF";
	}

	{
		auto pub0 = Symbols::GetPublicByName(symbols, "_PsGetNextProcess@4"); // xp
		auto pub1 = Symbols::GetPublicByName(symbols, "PsGetNextProcess"); // win11

		int o = -1;

		if (pub0)
			o = pub0->address;
		else if (pub1)
			o = pub1->address;

		if (!assign(MACRO_PSGETNEXTPROCESS_OFFSET, o))
			return "MACRO_PSGETNEXTPROCESS_OFFSET";
	}

	if (!assign(MACRO_PEB_FIELDOFFSET_IN_EPROCESS, getMember("_PEB *", A{ "_EPROCESS", NULL }, A{ "Peb", NULL })->offset))
		return "MACRO_PEB_FIELDOFFSET_IN_EPROCESS";

	if (!assign(MACRO_LDR_FIELDOFFSET_IN_PEB, getMember("_PEB_LDR_DATA *", A{ "_PEB", NULL }, A{ "Ldr", NULL })->offset))
		return "MACRO_LDR_FIELDOFFSET_IN_PEB";

	if (!assign(MACRO_INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA, getMember("_LIST_ENTRY", A{ "_PEB_LDR_DATA", NULL }, A{ "InLoadOrderModuleList", NULL })->offset))
		return "MACRO_INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA";

	if (!assign(MACRO_INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY, getMember("_LIST_ENTRY", A{ "_LDR_DATA_TABLE_ENTRY", NULL }, A{ "InLoadOrderLinks", NULL })->offset))
		return "MACRO_INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY";

	if (!assign(MACRO_DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY, getMember("void *", A{ "_LDR_DATA_TABLE_ENTRY", NULL }, A{ "DllBase", NULL })->offset))
		return "MACRO_DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY";

	if (!assign(MACRO_ENTRYPOINT_FIELDOFFSET_IN_LDRDATATABLEENTRY, getMember("void *", A{ "_LDR_DATA_TABLE_ENTRY", NULL }, A{ "EntryPoint", NULL })->offset))
		return "MACRO_ENTRYPOINT_FIELDOFFSET_IN_LDRDATATABLEENTRY";

	if (!assign(MACRO_SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY, getMember("ulong", A{ "_LDR_DATA_TABLE_ENTRY", NULL }, A{ "SizeOfImage", NULL })->offset))
		return "MACRO_SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY";

	if (!assign(MACRO_FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY, getMember("_UNICODE_STRING", A{ "_LDR_DATA_TABLE_ENTRY", NULL }, A{ "FullDllName", NULL })->offset))
		return "MACRO_FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY";

	if (!assign(MACRO_BASEDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY, getMember("_UNICODE_STRING", A{ "_LDR_DATA_TABLE_ENTRY", NULL }, A{ "BaseDllName", NULL })->offset))
		return "MACRO_BASEDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY";

	{
		auto wow1 = getMember("_WOW64_PROCESS *", A{ "_EPROCESS", NULL }, A{ "Wow64Process", NULL })->offset;
		auto wow2 = getMember("void *", A{ "_EPROCESS", NULL }, A{ "Wow64Process", NULL })->offset;
		auto wow3 = getMember("_EWOW64PROCESS *", A{ "_EPROCESS", NULL }, A{ "WoW64Process", NULL })->offset;

		auto peb1 = getMember("void *", A{ "_WOW64_PROCESS", NULL }, A{ "Wow64", NULL })->offset;
		auto peb2 = getMember("void *", A{ "_EWOW64PROCESS", NULL }, A{ "Peb", NULL })->offset;

		int wow = -1;

		if (wow1 != notFound.offset)
			wow = wow1;
		else if (wow2 != notFound.offset)
			wow = wow2;
		else if (wow3 != notFound.offset)
			wow = wow3;
		else if (_6432_(TRUE, FALSE))
			return "WOW64_PEB_NOT_FOUND_IN_EPROCESS";

		int peb = -1;

		if (peb1 != notFound.offset)
			peb = peb1;
		else if (peb2 != notFound.offset)
			peb = peb2;
		else if (_6432_(TRUE, FALSE) && wow2 == notFound.offset)
			return "WOW64_PEB_NOT_FOUND_IN_WOW64PROCESS";

		if (!assign(MACRO_WOW64_PROCESS_FIELDOFFSET_IN_EPROCESS, wow))
			return "MACRO_WOW64_PROCESS_FIELDOFFSET_IN_EPROCESS";

		if (!assign(MACRO_WOW64_PEB_FIELDOFFSET_IN_WOW64PROCESS, peb))
			return "MACRO_WOW64_PEB_FIELDOFFSET_IN_WOW64PROCESS";

		if (!assign(MACRO_WOW64_LDR_FIELDOFFSET_IN_PEB32, getMember("ulong", A{ "_PEB32", NULL }, A{ "Ldr", NULL })->offset))
			return "MACRO_WOW64_LDR_FIELDOFFSET_IN_PEB32";

		int field1 = -1;
		int field2 = -1;
		int field3 = -1;
		int field4 = -1;
		int field5 = -1;
		int field6 = -1;
		int field7 = -1;

		if (_6432_(TRUE, FALSE))
		{
			// little hack: we need the offsets of LDRDATA structures for WOW64 processes, but we don't have the symbols of NTDLL.
			// Since these offsets are the same since (at least) Windows XP, we can hardcode them here, but after having checked
			// whether their 64bit counterparts have the values we expect.

			if (MACRO_INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA == 0x10 &&
				MACRO_INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY == 0x00 &&
				MACRO_DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY == 0x30 &&
				MACRO_ENTRYPOINT_FIELDOFFSET_IN_LDRDATATABLEENTRY == 0x38 &&
				MACRO_SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY == 0x40 &&
				MACRO_FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY == 0x48 &&
				MACRO_BASEDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY == 0x58)
			{
				field1 = 0x0C;
				field2 = 0x00;
				field3 = 0x18;
				field4 = 0x1C;
				field5 = 0x20;
				field6 = 0x24;
				field7 = 0x2C;
			}
			else
				return "64BIT_LDRDATA_UNEXPECTED_LAYOUT";
		}

		if (!assign(MACRO_WOW64_INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA, field1))
			return "MACRO_WOW64_INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA";

		if (!assign(MACRO_WOW64_INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY, field2))
			return "MACRO_WOW64_INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY";

		if (!assign(MACRO_WOW64_DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY, field3))
			return "MACRO_WOW64_DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY";

		if (!assign(MACRO_WOW64_ENTRYPOINT_FIELDOFFSET_IN_LDRDATATABLEENTRY, field4))
			return "MACRO_WOW64_ENTRYPOINT_FIELDOFFSET_IN_LDRDATATABLEENTRY";

		if (!assign(MACRO_WOW64_SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY, field5))
			return "MACRO_WOW64_SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY";

		if (!assign(MACRO_WOW64_FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY, field6))
			return "MACRO_WOW64_FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY";

		if (!assign(MACRO_WOW64_BASEDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY, field7))
			return "MACRO_WOW64_BASEDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY";
	}

	if (!assign(MACRO_IRQL_FIELDOFFSET_IN_PCR, getMember("uchar", A{ "_KPCR", NULL }, A{ "Irql", NULL })->offset))
		return "MACRO_IRQL_FIELDOFFSET_IN_PCR";

	// calculate the minimum size of memory needed when reading a VAD and perform final checks on the offsets.
	// N.B. this must be the last thing to do in this function!

	int maxs = 0;
	for (int* v : offsVad)
		if (*v > maxs)
			maxs = *v;

	MACRO_SIZEOF_VAD = maxs + 32;

	int check1 = 0;

	if (MACRO_CONTROLAREA_FIELDOFFSET_IN_VAD != -1 && // this "if/else if" is taken from the DiscoverBytePointerPosInModules function.
		MACRO_FILEOBJECT_FIELDOFFSET_IN_CONTROLAREA != -1)
	{
		check1++;
	}
	else if (MACRO_SUBSECTION_FIELDOFFSET_IN_VAD != -1 &&
		MACRO_CONTROLAREA_FIELDOFFSET_IN_SUBSECTION != -1 &&
		MACRO_FILEPOINTER_FIELDOFFSET_IN_CONTROLAREA != -1)
	{
		check1++;
	}

	if (check1 != 1)
		return "CHECK_1_FAILED";

	if (MACRO_STARTINGVPNHIGH_FIELDOFFSET_IN_VAD == -1 && MACRO_ENDINGVPNHIGH_FIELDOFFSET_IN_VAD == -1)
		if (getMember(NULL, A{ "_MMVAD_SHORT", NULL }, A{ "StartingVpnHigh", NULL })->offset != notFound.offset ||
			getMember(NULL, A{ "_MMVAD_SHORT", NULL }, A{ "EndingVpnHigh", NULL })->offset != notFound.offset)
			return "CHECK_2_FAILED";

	return "";
}

BOOLEAN Platform::TryGetKernelDebugInfoWithoutSymbols(IN PDRIVER_OBJECT pDriverObject, OUT ImageDebugInfo* pDbgInfo)
{
	::memset(pDbgInfo, 0, sizeof(*pDbgInfo));

	auto pslml = (ULONG_PTR)((LIST_ENTRY*)pDriverObject->DriverSection)->Flink; // should be equivalent to PsLoadedModuleList, unchanged since Win2000

	auto base = *(ULONG_PTR*)((ULONG_PTR)((LIST_ENTRY*)pslml)->Flink + _6432_(0x30, 0x18)); // offset of _LDR_DATA_TABLE_ENTRY::DllBase, unchanged since Win2000

	IMAGE_DOS_HEADER* pdh = (IMAGE_DOS_HEADER*)base;

	if (pdh->e_magic == IMAGE_DOS_SIGNATURE)
	{
		IMAGE_NT_HEADERS* pnth = (IMAGE_NT_HEADERS*)(base + pdh->e_lfanew);

		auto& deb = pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

		if (deb.VirtualAddress && deb.Size)
		{
			IMAGE_DEBUG_DIRECTORY* dir = (IMAGE_DEBUG_DIRECTORY*)(base + deb.VirtualAddress);

			auto ndeb = deb.Size / sizeof(IMAGE_DEBUG_DIRECTORY);

			for (int i = 0; i < ndeb; i++)
			{
				if (dir[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
				{
					BYTE* cv = (BYTE*)(base + dir[i].AddressOfRawData);
					ULONG dim = dir[i].SizeOfData;

					DWORD sig = *(DWORD*)cv;

					if (sig == 'SDSR')
					{
						pDbgInfo->startAddr = (ULONG_PTR)base;
						pDbgInfo->length = 0;

						BYTE* guid = cv + sizeof(DWORD);
						::memcpy(pDbgInfo->guid, guid, 16);

						DWORD* age = (DWORD*)(guid + 16);
						pDbgInfo->age = *age;

						CHAR* pdbn = (CHAR*)((BYTE*)age + sizeof(DWORD));

						ULONG nfn = dim - (ULONG)((ULONG_PTR)pdbn - (ULONG_PTR)cv);
						::memcpy(pDbgInfo->szPdb, pdbn, nfn < sizeof(pDbgInfo->szPdb) - 1 ? nfn : sizeof(pDbgInfo->szPdb) - 1);

						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

BcCoroutine Platform::GetCurrentImageFileName(eastl::string& retVal) noexcept
{
	retVal = "???";

	// try to determine the field offset in the symbol file.

	auto symF = Root::I->GetSymbolFileByGuidAndAge(Platform::KernelDebugInfo.guid, Platform::KernelDebugInfo.age);
	if (!symF) co_return;

	auto symH = symF->GetHeader();
	if (!symH) co_return;

	auto type = Symbols::GetDatatype(symH, "_EPROCESS");
	if (!type) co_return;

	auto member15 = Symbols::GetDatatypeMember(symH, type, "ImageFileName", "uchar[15]");
	auto member16 = Symbols::GetDatatypeMember(symH, type, "ImageFileName", "uchar[16]");

	LONG offset = -1;
	LONG size = 0;

	if (member15)
	{
		offset = member15->offset;
		size = 15;
	}
	else if (member16)
	{
		offset = member16->offset;
		size = 16;
	}
	else
		co_return;

	// read the string.

	VOID* ptr;

	ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)GetPcrAddress() + MACRO_KTEBPTR_FIELDOFFSET_IN_PCR, sizeof(VOID*) };
	if (!ptr) co_return;

	ptr = co_await BcAwaiter_ReadMemory{ *(ULONG_PTR*)ptr + MACRO_KPEBPTR_FIELDOFFSET_IN_KTEB, sizeof(VOID*) };
	if (!ptr) co_return;

	ptr = co_await BcAwaiter_ReadMemory{ *(ULONG_PTR*)ptr + offset, (size_t)size };
	if (!ptr) co_return;

	CHAR buffer[32] = { 0 };

	::memcpy(buffer, ptr, size);

	retVal = buffer;

	co_return;
}

BcCoroutine Platform::GetIrql(LONG& retVal) noexcept
{
	retVal = -1;

	// try to determine the field offset in the symbol file.

	auto symF = Root::I->GetSymbolFileByGuidAndAge(Platform::KernelDebugInfo.guid, Platform::KernelDebugInfo.age);
	if (!symF) co_return;

	auto symH = symF->GetHeader();
	if (!symH) co_return;

	LONG offsetPrcb = -1;

	{
		auto type = Symbols::GetDatatype(symH, "_KPCR");
		if (!type) co_return;

		auto member1 = Symbols::GetDatatypeMember(symH, type, "PrcbData", "_KPRCB");
		auto member2 = Symbols::GetDatatypeMember(symH, type, "Prcb", "_KPRCB");

		if (member1)
			offsetPrcb = member1->offset;
		else if (member2)
			offsetPrcb = member2->offset;
		else
			co_return;
	}

	LONG offsetIrql = -1;

	{
		auto type = Symbols::GetDatatype(symH, "_KPRCB");
		if (!type) co_return;

		auto member = Symbols::GetDatatypeMember(symH, type, "DebuggerSavedIRQL", "uchar");

		if (member)
			offsetIrql = member->offset;
		else
			co_return;
	}

	// read the irql.

	VOID* ptr;

	ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)GetPcrAddress() + offsetPrcb + offsetIrql, sizeof(BYTE) };
	if (!ptr) co_return;

	retVal = *(BYTE*)ptr;

	co_return;
}

VOID Platform::GetModuleDebugInfo(ULONG64 dllBase, BYTE* pdbGuid, DWORD& pdbAge, CHAR(*pdbName)[256], ULONG32& arch)
{
	auto PROBE = [](ULONG64 addr) -> ULONG64 {

		if (addr < (ULONG64)(ULONG_PTR)UserProbeAddress)
			return addr;
		else
			return 0;
	};

	__try
	{
		IMAGE_DOS_HEADER* pdh = (IMAGE_DOS_HEADER*)PROBE(dllBase);

		if (pdh->e_magic == IMAGE_DOS_SIGNATURE)
		{
			IMAGE_DATA_DIRECTORY* deb = NULL;

			auto magic = ((IMAGE_NT_HEADERS32*)(PROBE(dllBase + pdh->e_lfanew)))->OptionalHeader.Magic;

			if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			{
				IMAGE_NT_HEADERS32* pnth = (IMAGE_NT_HEADERS32*)(PROBE(dllBase + pdh->e_lfanew));
				deb = &pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

				arch = 32;
			}
			else if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			{
				IMAGE_NT_HEADERS64* pnth = (IMAGE_NT_HEADERS64*)(PROBE(dllBase + pdh->e_lfanew));
				deb = &pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

				arch = 64;
			}

			if (deb && deb->VirtualAddress && deb->Size)
			{
				IMAGE_DEBUG_DIRECTORY* dir = (IMAGE_DEBUG_DIRECTORY*)(PROBE(dllBase + deb->VirtualAddress));

				auto ndeb = deb->Size / sizeof(IMAGE_DEBUG_DIRECTORY);

				for (int i = 0; i < ndeb && i < 64; i++)
				{
					if (dir[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
					{
						BYTE* cv = (BYTE*)(PROBE(dllBase + dir[i].AddressOfRawData));
						ULONG dim = dir[i].SizeOfData;

						DWORD sig = *(DWORD*)cv;

						if (sig == 'SDSR')
						{
							BYTE* guid = cv + sizeof(DWORD);
							::memcpy(pdbGuid, guid, 16);

							DWORD* age = (DWORD*)(guid + 16);
							pdbAge = *age;

							CHAR* pdbn = (CHAR*)((BYTE*)age + sizeof(DWORD));

							ULONG nfn = dim - (ULONG)((ULONG_PTR)pdbn - (ULONG_PTR)cv);

							if (nfn < sizeof(*pdbName))
							{
								::memcpy(*pdbName, pdbn, nfn);
								(*pdbName)[nfn] = '\0';
							}
						}
					}
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

VOID Platform::EnumUserModules(ULONG_PTR prc, VOID* context, BOOLEAN(*callback)(VOID* context, ULONG64 dllBase, ULONG32 sizeOfImage, CHAR(*fullDllName)[256]))
{
	auto PROBE = [](ULONG64 addr) -> ULONG64 {

		if (addr < (ULONG64)(ULONG_PTR)UserProbeAddress)
			return addr;
		else
			return 0;
	};

	// these fields are for native (non-WoW64) modules.

	BOOLEAN is64 = _6432_(TRUE, FALSE);
	int LDR_FIELDOFFSET_IN_PEB = MACRO_LDR_FIELDOFFSET_IN_PEB;
	int FLINK_FIELDOFFSET_IN_LISTENTRY = FIELD_OFFSET(LIST_ENTRY, Flink);
	int LENGTH_FIELDOFFSET_IN_UNICODESTRING = FIELD_OFFSET(UNICODE_STRING, Length);
	int BUFFER_FIELDOFFSET_IN_UNICODESTRING = FIELD_OFFSET(UNICODE_STRING, Buffer);
	int INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA = MACRO_INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA;
	int INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY = MACRO_INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY;
	int DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY = MACRO_DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY;
	int SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY = MACRO_SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY;
	int FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY = MACRO_FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY;

	// two iterations: the first for the PEB, the second for the WoW64 PEB (if present).

	for (int i = 0; i < 2; i++)
	{
		ULONG_PTR peb = 0;

		// get a pointer to the PEB in user space, checking whether this is a WoW64 process.

		if (i == 0)
		{
			peb = *(ULONG_PTR*)(prc + MACRO_PEB_FIELDOFFSET_IN_EPROCESS);
		}
		else if (i == 1)
		{
			if (MACRO_WOW64_PROCESS_FIELDOFFSET_IN_EPROCESS >= 0)
			{
				ULONG_PTR wowProc = *(ULONG_PTR*)(prc + MACRO_WOW64_PROCESS_FIELDOFFSET_IN_EPROCESS);

				if (wowProc)
				{
					if (MACRO_WOW64_PEB_FIELDOFFSET_IN_WOW64PROCESS >= 0)
						wowProc = *(ULONG_PTR*)(wowProc + MACRO_WOW64_PEB_FIELDOFFSET_IN_WOW64PROCESS);

					peb = wowProc;

					// set the offsets compatible with a WOW64 process.

					is64 = FALSE;
					LDR_FIELDOFFSET_IN_PEB = MACRO_WOW64_LDR_FIELDOFFSET_IN_PEB32;
					FLINK_FIELDOFFSET_IN_LISTENTRY = FIELD_OFFSET(LIST_ENTRY32, Flink);
					LENGTH_FIELDOFFSET_IN_UNICODESTRING = FIELD_OFFSET(UNICODE_STRING32, Length);
					BUFFER_FIELDOFFSET_IN_UNICODESTRING = FIELD_OFFSET(UNICODE_STRING32, Buffer);
					INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA = MACRO_WOW64_INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA;
					INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY = MACRO_WOW64_INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY;
					DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY = MACRO_WOW64_DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY;
					SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY = MACRO_WOW64_SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY;
					FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY = MACRO_WOW64_FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY;
				}
			}
		}
		else
		{
			break;
		}

		if (!peb) continue;

		__try
		{
			// get a pointer to the first LDRDATA entry.

			ULONG64 ldr = PROBE(is64 ? *(ULONG64*)(peb + LDR_FIELDOFFSET_IN_PEB) : *(ULONG32*)(peb + LDR_FIELDOFFSET_IN_PEB));

			ULONG64 first = PROBE(is64 ? *(ULONG64*)(ldr + INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA + FLINK_FIELDOFFSET_IN_LISTENTRY) :
				*(ULONG32*)(ldr + INLOADORDERMODULELIST_FIELDOFFSET_IN_PEBLDRDATA + FLINK_FIELDOFFSET_IN_LISTENTRY));

			ULONG64 entry = first;

			do
			{
				ULONG64 dllBase = PROBE(is64 ? *(ULONG64*)(entry + DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY) :
					*(ULONG32*)(entry + DLLBASE_FIELDOFFSET_IN_LDRDATATABLEENTRY));

				if (dllBase)
				{
					// get basic info about the module.

					ULONG32 sizeOfImage = *(ULONG32*)(entry + SIZEOFIMAGE_FIELDOFFSET_IN_LDRDATATABLEENTRY);

					USHORT fullDllNameLength = *(USHORT*)(entry + FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY + LENGTH_FIELDOFFSET_IN_UNICODESTRING) / sizeof(WCHAR);

					ULONG64 fullDllNameBuffer = PROBE(is64 ? *(ULONG64*)(entry + FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY + BUFFER_FIELDOFFSET_IN_UNICODESTRING) :
						*(ULONG32*)(entry + FULLDLLNAME_FIELDOFFSET_IN_LDRDATATABLEENTRY + BUFFER_FIELDOFFSET_IN_UNICODESTRING));

					CHAR fullDllName[256] = "";

					if (fullDllNameLength < sizeof(fullDllName))
					{
						for (int i = 0; i < fullDllNameLength; i++)
						{
							WCHAR c = *((WCHAR*)fullDllNameBuffer + i);
							if (c < 128)
								fullDllName[i] = (CHAR)c;
							else
								fullDllName[i] = '?';
						}

						fullDllName[fullDllNameLength] = '\0';
					}

					// call the callback.

					if (!callback(context, dllBase, sizeOfImage, &fullDllName))
						break;
				}

				// get the pointer to the next ldr_data_table_entry.

				entry = PROBE(is64 ? *(ULONG64*)(entry + INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY + FLINK_FIELDOFFSET_IN_LISTENTRY) :
					*(ULONG32*)(entry + INLOADORDERLINKS_FIELDOFFSET_IN_LDRDATATABLEENTRY + FLINK_FIELDOFFSET_IN_LISTENTRY));

			} while (entry && entry != first);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

VOID Platform::CopyName2(CHAR(*dest)[64], const CHAR* sz0)
{
	const CHAR* psz = &sz0[::strlen(sz0) - 1];

	for (; psz >= sz0; psz--)
		if (*psz == '\\' || *psz == '/')
			break;

	psz++;

	size_t size = _MIN_(::strlen(psz), sizeof(*dest) - 1);

	::memcpy(*dest, psz, size);
	(*dest)[size] = '\0';
}

VOID Platform::GetModulesSnapshot()
{
	size_t poolStart = Allocator::Perf_TotalAllocatedSize;

	// get a function pointer to the non-ntoskrnl-exported PsGetNextProcess function.

	typedef PEPROCESS(*pfnPsGetNextProcess)(PEPROCESS Process);

	pfnPsGetNextProcess PsGetNextProcess = (pfnPsGetNextProcess)(Platform::KernelDebugInfo.startAddr + MACRO_PSGETNEXTPROCESS_OFFSET);

	// iterate through all the processes in the system.

	PEPROCESS process;

	for (process = PsGetNextProcess(NULL); process != NULL; process = PsGetNextProcess(process))
	{
		// attach to the process in order to enumerate all the modules.

		KAPC_STATE apcState;
		
		::KeStackAttachProcess((PRKPROCESS)process, &apcState);

		// enumerate all the user space modules.

		EnumUserModules((ULONG_PTR)process, (VOID*)process, [](VOID* context, ULONG64 dllBase, ULONG32 sizeOfImage, CHAR(*fullDllName)[256]) -> BOOLEAN {

			// try to get the debug info of the module.

			BYTE pdbGuid[16] = { 0 };
			DWORD pdbAge = 0;
			CHAR pdbNameArr[256] = "";

			ULONG32 arch = 0;

			GetModuleDebugInfo(dllBase, pdbGuid, pdbAge, &pdbNameArr, arch);

			// add to the map.

			{
				NtModulesAccess _access_(FALSE);

				if (!Root::I->NtModules)
				{
					Root::I->NtModules = new eastl::vector<eastl::pair<ULONG64, eastl::vector<NtModule>>>(); // allocated in BC's high-irql pool: no need to free.
					Root::I->NtModules->reserve(512); // to reduce pool fragmentation.
				}

				// add the module info to the vector.

				auto& mods = GetNtModulesByProcess((ULONG64)(ULONG_PTR)context);

				auto& mod = mods.push_back();

				mod.dllBase = dllBase;
				mod.sizeOfImage = sizeOfImage;
				CopyName2(&mod.dllName, *fullDllName);

				::memcpy(mod.pdbGuid, pdbGuid, 16);
				mod.pdbAge = pdbAge;
				CopyName2(&mod.pdbName, pdbNameArr);

				mod.arch = arch;
			}

			// continue enumerating.

			return TRUE;
		});

		// detach from the process address space.

		::KeUnstackDetachProcess(&apcState);
	}

	// print a debug string.

	::DbgPrint("Platform::GetModulesSnapshot -> allocated %i bytes.", (LONG32)((LONG_PTR)Allocator::Perf_TotalAllocatedSize - (LONG_PTR)poolStart));
}

eastl::vector<NtModule>& Platform::GetNtModulesByProcess(ULONG64 p, int* ppos /*= NULL*/)
{
	auto fif = eastl::find_if(Root::I->NtModules->begin(), Root::I->NtModules->end(),
		[&](const eastl::pair<ULONG64, eastl::vector<NtModule>>& e) { return e.first == p; });

	if (fif == Root::I->NtModules->end())
	{
		auto& e = Root::I->NtModules->push_back();
		e.first = p;
		if (ppos) *ppos = Root::I->NtModules->size() - 1;
		return e.second;
	}
	else
	{
		if (ppos) *ppos = fif - Root::I->NtModules->begin();
		return fif->second;
	}
}

VOID Platform::CreateProcessNotifyRoutine(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)
{
	if (Create || !ProcessId) return;

	PEPROCESS process = NULL;
	if (!NT_SUCCESS(::PsLookupProcessByProcessId(ProcessId, &process)))
		return;

	if (!process) return;

	::ObDereferenceObject(process);

	// remove the process from NtModules.

	{
		NtModulesAccess _access_(TRUE, "Platform::CreateProcessNotifyRoutine"); // N.B. no calls to BC's allocator OUTSIDE OF this scope (including calls to STL/EASTL).

		int pos = -1;
		auto& mods = GetNtModulesByProcess((ULONG64)(ULONG_PTR)process, &pos);
		if (pos < 0) return;

		Root::I->NtModules->erase(Root::I->NtModules->begin() + pos);
	}
}

VOID Platform::LoadImageNotifyRoutine(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo)
{
	if (!ProcessId || !ImageInfo || !ImageInfo->ImageBase ||
		(ULONG_PTR)ImageInfo->ImageBase >= (ULONG_PTR)UserProbeAddress || !ImageInfo->ImageSize)
		return;

	PEPROCESS process = NULL;
	if (!NT_SUCCESS(::PsLookupProcessByProcessId(ProcessId, &process)))
		return;

	if (!process) return;

	::ObDereferenceObject(process);

	// try to get the debug info of the module.

	BYTE pdbGuid[16] = {0};
	DWORD pdbAge = 0;
	CHAR pdbNameArr[256] = "";

	ULONG32 arch = 0;

	GetModuleDebugInfo((ULONG_PTR)ImageInfo->ImageBase, pdbGuid, pdbAge, &pdbNameArr, arch);

	// get a list of all the module bases in the current address space.

	class LoadedMods
	{
	public:
		LoadedMods()
		{
			ptr = (ULONG64*)::ExAllocatePool(NonPagedPool, size);
		}

		~LoadedMods()
		{
			::ExFreePool(ptr);
		}

		const size_t size = 32 * 1024;
		ULONG64* ptr = NULL;
		int index = 0;

		VOID Add(ULONG64 dllBase)
		{
			if (index >= size / sizeof(ULONG64)) return;

			ptr[index++] = dllBase;
		}

		ULONG64* begin()
		{
			return ptr;
		}

		ULONG64* end()
		{
			return ptr + index;
		}
	};

	LoadedMods loadedMods;

	EnumUserModules((ULONG_PTR)process, &loadedMods, [](VOID* context, ULONG64 dllBase, ULONG32 sizeOfImage, CHAR(*fullDllName)[256]) -> BOOLEAN {

		((LoadedMods*)context)->Add(dllBase);
		return TRUE;
	});

	// get the ASCII name of the image.

	class FullDllName
	{
	public:

		FullDllName(PUNICODE_STRING FullImageName)
		{
			if (FullImageName && FullImageName->Buffer && FullImageName->Length > 0 && FullImageName->Length < 1024)
			{
				int length = FullImageName->Length / sizeof(WCHAR);

				fullDllName = (CHAR*)::ExAllocatePool(NonPagedPool, length + 1);

				for (int i = 0; i < length; i++)
				{
					WCHAR c = *((WCHAR*)FullImageName->Buffer + i);
					if (c < 128)
						fullDllName[i] = (CHAR)c;
					else
						fullDllName[i] = '?';
				}

				fullDllName[length] = '\0';
			}
		}

		~FullDllName()
		{
			if (fullDllName) ::ExFreePool(fullDllName);
		}

		const CHAR* c_str()
		{
			if (!fullDllName)
				return "";
			else
				return fullDllName;
		}

	private:

		CHAR* fullDllName = NULL;
	};

	FullDllName fullDllName(FullImageName);

	// we must have at least the name of the module or the pdb name.

	if (!::strlen(fullDllName.c_str()) && !::strlen(pdbNameArr))
		return;

	// remove the unloaded modules + add the new module.

	{
		NtModulesAccess _access_(TRUE, "Platform::LoadImageNotifyRoutine"); // N.B. no calls to BC's allocator OUTSIDE OF this scope (including calls to STL/EASTL).

		// get the modules vector relative to this PEPROCESS.

		auto& mods = GetNtModulesByProcess((ULONG64)(ULONG_PTR)process);

		// remove the unloaded modules.

		eastl::sort(loadedMods.begin(), loadedMods.end());

		for (int i = mods.size() - 1; i >= 0; i--)
			if (!eastl::binary_search(loadedMods.begin(), loadedMods.end(), mods[i].dllBase))
				mods.erase(mods.begin() + i);

		// add the new module.

		auto& mod = mods.push_back();

		mod.dllBase = (ULONG_PTR)ImageInfo->ImageBase;
		mod.sizeOfImage = ImageInfo->ImageSize;
		CopyName2(&mod.dllName, fullDllName.c_str());

		::memcpy(mod.pdbGuid, pdbGuid, 16);
		mod.pdbAge = pdbAge;
		CopyName2(&mod.pdbName, pdbNameArr);

		mod.arch = arch;
	}
}

BcCoroutine Platform::GetCurrentEprocess(ULONG64& dest) noexcept
{
	dest = 0;

	VOID* ptr;

	ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)GetPcrAddress() + MACRO_KTEBPTR_FIELDOFFSET_IN_PCR, sizeof(VOID*) };
	if (!ptr) co_return;

	ptr = co_await BcAwaiter_ReadMemory{ *(ULONG_PTR*)ptr + MACRO_KPEBPTR_FIELDOFFSET_IN_KTEB, sizeof(VOID*) };
	if (!ptr) co_return;

	dest = *(ULONG_PTR*)ptr;

	co_return;
}

BcCoroutine Platform::GetWow64Peb(ULONG64& dest, ULONG64 eproc) noexcept
{
	dest = 0;

	VOID* ptr;

	if (MACRO_WOW64_PROCESS_FIELDOFFSET_IN_EPROCESS >= 0)
		if (ptr = co_await BcAwaiter_ReadMemory{ (ULONG_PTR)eproc + MACRO_WOW64_PROCESS_FIELDOFFSET_IN_EPROCESS, sizeof(VOID*) })
		{
			ULONG_PTR wowProc = *(ULONG_PTR*)ptr;

			if (wowProc)
			{
				if (MACRO_WOW64_PEB_FIELDOFFSET_IN_WOW64PROCESS >= 0)
					if (ptr = co_await BcAwaiter_ReadMemory{ wowProc + MACRO_WOW64_PEB_FIELDOFFSET_IN_WOW64PROCESS, sizeof(VOID*) })
						wowProc = *(ULONG_PTR*)ptr;

				dest = wowProc;
			}
		}

	co_return;
}

BcCoroutine Platform::RefreshNtModulesForProc0() noexcept
{
	eastl::vector<int> present;

	// get a ptr to the modules vector for eprocess==0 (system space).

	eastl::vector<NtModule>* mods;

	{
		NtModulesAccess _access_(FALSE); // required only when writing.

		mods = &GetNtModulesByProcess(0);
	}

	// iterate through all the images loaded in system space.

	VOID* ptr;

	ULONG_PTR listNodePtr = (ULONG_PTR)Root::I->PsLoadedModuleList;

	do
	{
		ULONG_PTR imageBase = 0;
		ULONG sizeOfImage = 0;
		CHAR imageName[256] = "";

		BYTE pdbGuid[16] = { 0 };
		DWORD pdbAge = 0;
		CHAR pdbName[256] = { 0 };

		ULONG32 arch = _6432_(64, 32);

		if (ptr = co_await BcAwaiter_ReadMemory{ listNodePtr + MACRO_IMAGEBASE_FIELDOFFSET_IN_DRVSEC, sizeof(VOID*) })
		{
			imageBase = *(ULONG_PTR*)ptr;

			if (ptr = co_await BcAwaiter_ReadMemory{ imageBase, sizeof(IMAGE_DOS_HEADER) })
			{
				if (((IMAGE_DOS_HEADER*)ptr)->e_magic == IMAGE_DOS_SIGNATURE &&
					((IMAGE_DOS_HEADER*)ptr)->e_lfarlc >= 0x40)
				{
					if (ptr = co_await BcAwaiter_ReadMemory{ imageBase + ((IMAGE_DOS_HEADER*)ptr)->e_lfanew, sizeof(IMAGE_NT_HEADERS) })
					{
						if (((IMAGE_NT_HEADERS*)ptr)->Signature == IMAGE_NT_SIGNATURE)
						{
							sizeOfImage = ((IMAGE_NT_HEADERS*)ptr)->OptionalHeader.SizeOfImage;

							IMAGE_DATA_DIRECTORY dirDebug = ((IMAGE_NT_HEADERS*)ptr)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

							// is this image already in the vector?

							BOOLEAN found = FALSE;

							for (int i = 0; i < mods->size(); i++)
								if ((*mods)[i].dllBase == imageBase && (*mods)[i].sizeOfImage == sizeOfImage)
								{
									found = TRUE;
									present.push_back(i);
									break;
								}

							if (!found)
							{
								// get the image name.

								if (ptr = co_await BcAwaiter_ReadMemory{ listNodePtr + MACRO_IMAGENAME_FIELDOFFSET_IN_DRVSEC, sizeof(UNICODE_STRING) })
								{
									ULONG nameLen = ((UNICODE_STRING*)ptr)->Length / sizeof(WORD);
									ULONG_PTR nameBuff = (ULONG_PTR)((UNICODE_STRING*)ptr)->Buffer;

									if (nameLen != 0 &&
										nameLen <= sizeof(imageName) - 1)
									{
										if (ptr = co_await BcAwaiter_ReadMemory{ nameBuff, (nameLen + 1) * sizeof(WORD) })
										{
											WORD* wordPtr = (WORD*)ptr;
											CHAR* charPtr = imageName;

											for (ULONG i = 0; i < nameLen; i++)
											{
												WORD w = *wordPtr++;
												*charPtr++ = w >= 128 ? '?' : (CHAR)(w & 0xFF);
											}

											*charPtr = '\0';
										}
									}
								}

								// get the debug info.

								if (dirDebug.VirtualAddress && dirDebug.Size)
								{
									if (ptr = co_await BcAwaiter_ReadMemory{ imageBase + dirDebug.VirtualAddress, dirDebug.Size })
									{
										const static int iddDebDir_size = 32; // ntoskrnl of Windows 11 has 4 of these, and Windows XP only 1...
										IMAGE_DEBUG_DIRECTORY iddDebDir[iddDebDir_size];

										ULONG nDebDir = dirDebug.Size / sizeof(IMAGE_DEBUG_DIRECTORY);
										if (nDebDir > iddDebDir_size)
											nDebDir = iddDebDir_size;

										::memcpy(iddDebDir, ptr, nDebDir * sizeof(IMAGE_DEBUG_DIRECTORY));

										for (int i = 0; i < nDebDir; i++)
										{
											if (iddDebDir[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
											{
												if (ptr = co_await BcAwaiter_ReadMemory{ imageBase + iddDebDir[i].AddressOfRawData,
													iddDebDir[i].SizeOfData < 512 ? iddDebDir[i].SizeOfData : 512 })
												{
													auto temp_pbCV = (BYTE*)ptr;

													DWORD sig = *(DWORD*)temp_pbCV;

													if (sig == 'SDSR')
													{
														BYTE* guid = temp_pbCV + sizeof(DWORD);
														DWORD* age = (DWORD*)(guid + 16);

														CHAR* pdbn = (CHAR*)((BYTE*)age + sizeof(DWORD));
														ULONG nfn = iddDebDir[i].SizeOfData - (ULONG)((ULONG_PTR)pdbn - (ULONG_PTR)temp_pbCV);

														if (nfn > sizeof(pdbName) - 1)
															nfn = sizeof(pdbName) - 1;

														::memcpy(pdbGuid, guid, 16);

														pdbAge = *age;

														::memset(pdbName, 0, sizeof(pdbName));
														::memcpy(pdbName, pdbn, nfn);

														break;
													}
												}
											}
										}
									}
								}

								// sometimes newer versions of Windows obfuscate the module name: compensate with pdb name, if possible.

								if (!::strlen(imageName) && ::strlen(pdbName))
								{
									CHAR sz0[sizeof(pdbName)];
									::strcpy(sz0, pdbName);

									CHAR* psz = &sz0[::strlen(sz0) - 1];

									for (; psz >= sz0; psz--)
										if (*psz == '.')
											*psz = '\0';
										else if (*psz == '\\' || *psz == '/')
											break;

									psz++;

									if (::strlen(psz))
										::strcpy(imageName, psz);
								}

								// add the image to the vector.

								present.push_back(mods->size());

								{
									NtModulesAccess _access_(FALSE); // required only when writing.

									auto& mod = mods->push_back();

									mod.dllBase = imageBase;
									mod.sizeOfImage = sizeOfImage;
									CopyName2(&mod.dllName, imageName);

									::memcpy(mod.pdbGuid, pdbGuid, 16);
									mod.pdbAge = pdbAge;
									CopyName2(&mod.pdbName, pdbName);

									mod.arch = arch;
								}
							}
						}
					}
				}
			}
		}

		// get a ptr to the next entry.

		ptr = co_await BcAwaiter_ReadMemory{ listNodePtr + FIELD_OFFSET(LIST_ENTRY, Flink), sizeof(VOID*) };

		if (!ptr)
			break;
		else
			listNodePtr = *(ULONG_PTR*)ptr;

	} while (listNodePtr && listNodePtr != (ULONG_PTR)Root::I->PsLoadedModuleList);

	// remove all the modules that are no more present.

	{
		NtModulesAccess _access_(FALSE); // required only when writing.

		for (int i = mods->size() - 1; i >= 0; i--)
		{
			BOOLEAN found = FALSE;
			for (int j : present)
				if (j == i)
				{
					found = TRUE;
					break;
				}

			if (!found)
				mods->erase(mods->begin() + i);
		}
	}

	// return to the caller.

	co_return;
}
