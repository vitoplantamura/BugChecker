#pragma once
#include "../../bzscore/file.h"
#include "../../bzscore/autofile.h"
#include "../../bzscore/buffer.h"

namespace BazisLib
{
	namespace Algorithms
	{
		template <class _SrcFileClass, class _DstFileClass> ULONGLONG BulkCopy(_SrcFileClass *pSource, _DstFileClass *pDest, ULONGLONG CopySize, size_t BufferSize, void *pBuffer)
		{
			ASSERT(pSource && pDest && pBuffer && BufferSize);
			if (!CopySize)
				return 0;
			ULONGLONG done = 0;
			while (done < CopySize)
			{
				size_t todo = BufferSize;
				if (todo > (CopySize - done))
					todo = (size_t)(CopySize - done);
				if (pSource->Read(pBuffer, todo) != todo)
					return done;
				if (pDest->Write(pBuffer, todo) != todo)
					return done;
				done += todo;
			}
			ASSERT(done == CopySize);
			return done;
		}

		template <class _SrcFileClass, class _DstFileClass> ULONGLONG BulkCopy(_SrcFileClass *pSource, _DstFileClass *pDest, ULONGLONG CopySize, size_t BufferSize)
		{
			ASSERT(BufferSize);
			char *pBuffer = new char[BufferSize];
			if (!pBuffer)
				return 0;
			ULONGLONG r = BulkCopy(pSource, pDest, CopySize, BufferSize, pBuffer);
			delete pBuffer;
			return r;
		}


		template <class _ContainerType> void DoEnumerateFilesRecursively(const String &fp, _ContainerType *pContainer, LPCTSTR lpFileMask)
		{
			ManagedPointer<AIDirectory> pDir = new ACDirectory(fp);
			
			for (ManagedPointer<AIDirectoryIterator> pIter = pDir->FindFirstFile(lpFileMask ? lpFileMask : _T("")); pIter && pIter->Valid(); pIter->FindNextFile())
			{
				//If the mask was specified, skip directories, as we'll enumerate them later without the mask
				if (pIter->IsAPseudoEntry() || (pIter->IsDirectory() && lpFileMask))
					continue;
				if (pIter->IsDirectory())
				{
					ASSERT(!lpFileMask);
					DoEnumerateFilesRecursively(pIter->GetFullPath(), pContainer, lpFileMask);
				}
				pContainer->push_back(pIter->GetFullPath());
			}
			//If the mask is not specified, a single enumeration is sufficient
			//If the mask was specified, we need to enumerate directories separately
			if (lpFileMask)
			{
				for (ManagedPointer<AIDirectoryIterator> pIter = pDir->FindFirstFile(); pIter && pIter->Valid(); pIter->FindNextFile())
				{
					if (pIter->IsAPseudoEntry() || !pIter->IsDirectory())
						continue;
					DoEnumerateFilesRecursively(pIter->GetFullPath(), pContainer, lpFileMask);
				}
			}
		}

		template <class _ContainerType> _ContainerType static inline EnumerateFilesRecursively(const String &dir, LPCTSTR lpFileMask = NULL)
		{
			_ContainerType container;
			DoEnumerateFilesRecursively<_ContainerType>(dir, &container, lpFileMask);
			return container;
		}

		static inline ActionStatus RemoveDirectoryRecursive(const String &fp, bool ForceDeleteReadonly = false)
		{
			Directory dir(fp);

			for (Directory::enumerator iter = dir.FindFirstFile(); iter.Valid(); iter.Next())
			{
				if (iter.IsAPseudoEntry())
					continue;
				ActionStatus st;
				if (iter.IsDirectory())
					st = RemoveDirectoryRecursive(iter.GetFullPath());
				else
					st = File::Delete(iter.GetFullPath().c_str(), ForceDeleteReadonly);
				if (!st.Successful())
					return st;
			}
			return Directory::Remove(fp);
		}

		static ActionStatus ReadFileToBuffer(const String &filePath, CBuffer &buf)
		{
			ActionStatus st;
			File file(filePath, FileModes::OpenReadOnly.ShareAll(), &st);
			if (!st.Successful())
				return st;
			ULONGLONG fileSize = file.GetSize(&st);
			if (!st.Successful())
				return st;
			if (fileSize > (size_t)-1)
				return MAKE_STATUS(NoMemory);
			if (!buf.EnsureSize((size_t)fileSize))
				return MAKE_STATUS(NoMemory);
			
			size_t done = file.Read(buf.GetData(), (size_t)fileSize, &st);
			if (!st.Successful())
				return st;
			buf.SetSize(done);
			return MAKE_STATUS(Success);
		}

	}
}