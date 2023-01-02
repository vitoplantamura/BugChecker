#pragma once
#include "cmndef.h"
#include <stdlib.h>
#include "../bzscore/autofile.h"
#include "../bzscore/objmgr.h"
#include "../bzscore/sync.h"

namespace BazisLib
{
	template <class StorageType> class SingleMallocVector
	{
	private:
		StorageType *pData;
		size_t Alloc, Used;

	public:

		SingleMallocVector()
		{
			Alloc = 16;
			Used = 0;
			pData = (StorageType *)malloc(sizeof(StorageType)*Alloc);
		}

		//WARNING! Array should be allocated via malloc() function
		SingleMallocVector(StorageType *Array, size_t ElementCount)
		{
			Alloc = Used = ElementCount;
			pData = ElementCount;
		}

		StorageType *GetRawPointer()
		{
			return pData;
		}

		void Reset()
		{
			Used = 0;
		}

		~SingleMallocVector()
		{
			Reset();
			free(pData);
		}

		StorageType &operator[](size_t i)
		{
			if (i >= Used)
				return *(StorageType *)NULL;
			return pData[i];
		}

		const StorageType &operator[](size_t i) const
		{
			if (i >= Used)
				return *(StorageType *)NULL;
			return pData[i];
		}

		void EnsureCapacity(size_t NewCapacity)
		{
			if (NewCapacity >= Alloc)
				pData = (StorageType *)realloc(pData,(Alloc = NewCapacity)*sizeof(StorageType));
		}

		bool SetElementCount(size_t NewElementCount)
		{
			if (NewElementCount > Alloc)
			{
				Used = 0;
				return false;
			}
			Used = NewElementCount;
			return true;
		}

		size_t Add(StorageType *pInit)
		{
			if (!pInit)
				return -1;
			if (Used >= Alloc)
				pData = (StorageType *)realloc(pData,(Alloc*=2)*sizeof(StorageType));
			pData[Used] = *pInit;
			return Used++;
		}

		size_t Add(StorageType pInit)
		{
			if (Used >= Alloc)
				pData = (StorageType *)realloc(pData,(Alloc*=2)*sizeof(StorageType));
			pData[Used] = pInit;
			return Used++;
		}

		bool SaveToFile(ManagedPointer<AIFile> file) const
		{
			int TypeSize = sizeof(StorageType);
			if (file->Write(&TypeSize,sizeof(TypeSize)) != sizeof(TypeSize))
				return false;
			if (file->Write(&Used,sizeof(Used)) != sizeof(Used))
				return false;
			if (file->Write(pData,Used * TypeSize) != Used * TypeSize)
				return false;
			return true;
		}

		template <class _FileClass> bool SaveToFile(_FileClass *file) const
		{
			int TypeSize = sizeof(StorageType);
			if (file->Write(&TypeSize,sizeof(TypeSize)) != sizeof(TypeSize))
				return false;
			if (file->Write(&Used,sizeof(Used)) != sizeof(Used))
				return false;
			if (file->Write(pData,Used * TypeSize) != Used * TypeSize)
				return false;
			return true;
		}

		bool LoadFromFile(ManagedPointer<AIFile> file)
		{
			unsigned int TypeSize, NewUsed;
			if (file->Read(&TypeSize,sizeof(TypeSize)) != sizeof(TypeSize))
				return false;
			if (TypeSize != sizeof(StorageType))
			{
				file->Seek(-(LONGLONG)sizeof(TypeSize),FileFlags::FileCurrent);
				return false;
			}
			if (file->Read(&NewUsed,sizeof(NewUsed)) != sizeof(NewUsed))
			{
				file->Seek(-(LONGLONG)(sizeof(TypeSize)+sizeof(NewUsed)),FileFlags::FileCurrent);
				return false;
			}
			Used = NewUsed;
			if (Alloc < NewUsed)
				pData = (StorageType *)realloc(pData,sizeof(StorageType)*(Alloc = Used));
			if (file->Read(pData,Used*sizeof(StorageType)) != Used*sizeof(StorageType))
				return false;
			return true;
		}

		bool LoadFromFileEx(ManagedPointer<AIFile> file, int ExpectedElementSize, StorageType *pDefaultElement)
		{
			DWORD dwOk;
			unsigned int TypeSize, NewUsed;
			if (ExpectedElementSize > sizeof(StorageType))
				return false;
			if (file->Read(&TypeSize,sizeof(TypeSize)) != sizeof(TypeSize))
				return false;
			if (TypeSize != ExpectedElementSize)
			{
				file->Seek(-sizeof(TypeSize),FileFlags::FileCurrent);
				return false;
			}
			if (file->Read(&NewUsed,sizeof(NewUsed)) != sizeof(NewUsed))
			{
				file->Seek(-(sizeof(TypeSize)+sizeof(NewUsed)),FileFlags::FileCurrent);
				return false;
			}
			Used = NewUsed;
			if (Alloc < NewUsed)
				pData = (StorageType *)realloc(pData,sizeof(StorageType)*(Alloc = Used));
			for (int i = 0; i < Used; i++)
			{
				memcpy(&pData[i], pDefaultElement, sizeof(StorageType));
				if (file->Read(&pData[i],ExpectedElementSize) != ExpectedElementSize)
					return false;
			}
			return true;
		}

		size_t Count() const
		{
			return Used;
		}

	/*public:
		//STL support classes & methods
		
		class iterator
		{
		private:
			StorageType *m_pPtr;
			SingleMallocVector *m_pVec;
			friend class SingleMallocVector;
		
		private:
			iterator(SingleMallocVector *pVec, StorageType *pPtr) :
			   m_pPtr(pPtr), m_pVec(pVec)
			   {
			   }

		   operator StorageType *()
		   {
			   return m_pPtr;
		   }

		   iterator &operator++()
		   {
			   if (++m_pPtr > (m_pVec->pData + m_pVec->Used))
				   m_pPtr = NULL;
			   return this;
		   }

		   bool operator !=(const iterator &it)
		   {
			   return (m_pPtr != it.m_pPtr);
		   }
		};

		iterator begin()
		{
			return iterator(this, pData);
		}

		iterator end()
		{
			return iterator(this, pData + Used);
		}*/

	};
}