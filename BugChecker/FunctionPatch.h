#pragma once

#include "BugChecker.h"

//
// This class is from VirtualKD:
//

//! Simplifies inserting JMP commands in the beginning of functions
/*! This class represents a single patched function (with a JMP instruction inserted in the beginning).
    Typically, after creation the FunctionPatch::Patch() method should be called to patch a function. During patching,
    the class instance stores the data that was replaced by inserting a JMP and restores this data back when the
    object is deleted. That way, each patched function should have a corresponding FunctionPatch instance.
*/
class FunctionPatch
{
private:
    char m_PreviousBytes[5];
    void* m_pAddress;

    //! Allows retreiving read/write access to read-only kernel memory.
    /*! This class allows modifying read-only kernel memory. On x86 targets, all kernel memory is always writable
        by kernel-mode code, however, on x64 targets, some memory (such as code segments) is marked as read-only.
        This class allows retreiving a write-enabled pointer to a read-only memory block.
    */
    class MemoryLocker
    {
    private:
        PMDL m_pMdl;
        PVOID m_pPointer;

    public:
        //! Locks read-only pages in memory and creates an additional read-write mapping
        MemoryLocker(void* pData, ULONG size)
        {
            m_pMdl = IoAllocateMdl(pData, size, FALSE, FALSE, NULL);
            ASSERT(m_pMdl);
            MmProbeAndLockPages(m_pMdl, KernelMode, IoReadAccess);
            m_pPointer = MmMapLockedPagesSpecifyCache(m_pMdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
            NTSTATUS status = MmProtectMdlSystemAddress(m_pMdl, PAGE_EXECUTE_READWRITE);
            ASSERT(NT_SUCCESS(status));
        }

        //! Destroys the additional read-write mapping
        ~MemoryLocker()
        {
            MmUnmapLockedPages(m_pPointer, m_pMdl);
            MmUnlockPages(m_pMdl);
            IoFreeMdl(m_pMdl);
        }

        //! Returns a write-enabled pointer to a read-only memory block
        void* GetPointer()
        {
            return m_pPointer;
        }

    };

private:
    //! Replaces the first 5 bytes of a function by a JMP instruction
    /*! This method performs the actual patching and is internally used by Patch() method.
        \param pFunc Specifies the address of a function to be patched
        \param pDest Specifies the target address of the JMP instruction
    */
    static bool _PatchFunction(void* pFunc, void* pDest)
    {
        MemoryLocker locker(pFunc, 5);

        INT_PTR off = ((char*)pDest - (char*)pFunc) - 5;
#ifdef _AMD64_
        if ((off > 0x7FFFFFFFLL) || (off < -0x7FFFFFFFLL))
            return false;
#endif

        unsigned char* p = (unsigned char*)locker.GetPointer();
        p[0] = 0xE9;

        *((unsigned*)(p + 1)) = (unsigned)off;

        return true;
    }

public:
    FunctionPatch()
        : m_pAddress(NULL)
    {
    }

    ~FunctionPatch()
    {
        if (m_pAddress)
        {
            MemoryLocker locker(m_pAddress, 5);
            memcpy(locker.GetPointer(), m_PreviousBytes, sizeof(m_PreviousBytes));
        }
    }


    //! Inserts the JMP instruction in the beginning of a function and stores the bytes being overwritten
    /*!
        \param pFunc Specifies the address of a function to be patched
        \param pNewFunc Specifies the target address of the JMP instruction
    */
    bool Patch(void* pFunc, void* pNewFunc)
    {
        if (m_pAddress)
            return false;
        memcpy(m_PreviousBytes, pFunc, sizeof(m_PreviousBytes));
        if (!_PatchFunction(pFunc, pNewFunc))
            return false;
        m_pAddress = pFunc;
        return true;
    }
};
