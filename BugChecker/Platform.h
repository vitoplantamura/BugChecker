#pragma once

#include "BugChecker.h"

#include "MemReadCor.h"
#include "BcCoroutine.h"

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/functional.h>

extern "C"
{
	void _enable();
	void _disable();
#ifdef _AMD64_
	unsigned __int64 __readeflags();
#else
	unsigned int __readeflags();
#endif
}

class BcSpinLock // we don't raise IRQL to HIGH_LEVEL + use NT's spinlock in order to minimize dependencies.
{
public:

	volatile LONG state = 0;

	VOID Lock(BOOLEAN* prevInts) noexcept
	{
		*prevInts = AreInterruptsEnabled();
		::_disable();
		while (::_InterlockedCompareExchange(&state, 1, 0) == 1) ::_mm_pause();
	}

	VOID Unlock(BOOLEAN prevInts) noexcept
	{
		::_InterlockedExchange(&state, 0);
		if (prevInts) ::_enable();
	}

	static BOOLEAN AreInterruptsEnabled()
	{
		return (::__readeflags() & (1 << 9)) != 0;
	}
};

class ImageDebugInfo
{
public:
	ULONG_PTR startAddr;
	ULONG length;
	BYTE guid[16];
	DWORD age;
	CHAR szPdb[256];
};

class NtModulesAccess
{
public:
	NtModulesAccess(BOOLEAN acquireSpinLock, const CHAR* dbgPrefixPtr = NULL);
	~NtModulesAccess();

private:
	BOOLEAN lock;
	const CHAR* dbgPrefix = NULL;
	size_t poolStart = 0;
	BOOLEAN prevInts;
};

class NtModule
{
public:
	ULONG64 dllBase = 0;
	ULONG32 sizeOfImage = 0;
	ULONG32 arch = 0;
	CHAR dllName[64] = { 0 };

	BYTE pdbGuid[16] = { 0 };
	DWORD pdbAge = 0;
	CHAR pdbName[64] = { 0 };
};

class DiscoverBytePointerPosInModules_Params
{
public:
	OUT CHAR* pszOutputBuffer;
	OUT ImageDebugInfo* pidiDebugInfo;
	IN BYTE* pbBytePointer;
	IN VOID* pvPsLoadedModuleList;
	IN VOID* pvMmUserProbeAddress;
	IN VOID* pvPCRAddress;
};

class Platform
{
public:
	static VOID* UserProbeAddress;
	static VOID* KernelBaseAddress;

	static ImageDebugInfo KernelDebugInfo;

	static MemoryReaderCoroutineState* DiscoverBytePointerPosInModules(MemoryReaderCoroutineState* pMemReadCor);

	static VOID* GetPcrAddress();
	static VOID* GetCurrentThread();
	static KIRQL GetCurrentIrql();
	static VOID* GetKernelBaseAddress(ULONG64 PsLoadedModuleList);

	static eastl::string CalculateKernelOffsets(VOID* symbols);
	static BOOLEAN TryGetKernelDebugInfoWithoutSymbols(IN PDRIVER_OBJECT pDriverObject, OUT ImageDebugInfo* pDbgInfo);

	static BcCoroutine GetCurrentImageFileName(eastl::string& retVal) noexcept;
	static BcCoroutine GetIrql(LONG& retVal) noexcept;

	static VOID GetModulesSnapshot();

	static VOID CreateProcessNotifyRoutine(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create);
	static VOID LoadImageNotifyRoutine(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo);

	static BcCoroutine GetCurrentEprocess(ULONG64& dest) noexcept;
	static BcCoroutine GetWow64Peb(ULONG64& dest, ULONG64 eproc) noexcept;

	static BcCoroutine RefreshNtModulesForProc0() noexcept;

private:

	static VOID GetModuleDebugInfo(ULONG64 dllBase, BYTE* pdbGuid, DWORD& pdbAge, CHAR(*pdbName)[256], ULONG32& arch);
	static eastl::vector<NtModule>& GetNtModulesByProcess(ULONG64 p, int* ppos = NULL);
	static VOID EnumUserModules(ULONG_PTR prc, VOID* context, BOOLEAN(*callback)(VOID* context, ULONG64 dllBase, ULONG32 sizeOfImage, CHAR(*fullDllName)[256]));
	static VOID CopyName2(CHAR(*dest)[64], const CHAR* sz0);
};
