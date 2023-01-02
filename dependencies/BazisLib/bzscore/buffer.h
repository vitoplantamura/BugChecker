#pragma once

#include "cmndef.h"
#include "assert.h"
#include "objmgr.h"

namespace BazisLib
{
	class BasicBuffer
	{
	private:
		void *m_pBuffer;
		size_t m_AllocatedSize;
		size_t m_UsedSize;

	private:
		BasicBuffer(const BasicBuffer &buf)
		{
			ASSERT(false);
		}

		BasicBuffer &operator=(const BasicBuffer &buf)
		{
			ASSERT(false);
			return *this;
		}

	public:
#pragma region Constructors/destructor
#if _MSC_VER >= 1700
		BasicBuffer(BasicBuffer &&another)
			: m_pBuffer(another.m_pBuffer)
			, m_AllocatedSize(another.m_AllocatedSize)
			, m_UsedSize(another.m_UsedSize)
		{
			another.m_pBuffer = NULL;
			another.m_AllocatedSize = another.m_UsedSize = 0;
		}
#endif

		BasicBuffer(size_t initialSize = 0) :
		  m_pBuffer(0),
		  m_UsedSize(0),
		  m_AllocatedSize(0)
		{
			if (!initialSize)
				return;
			m_pBuffer = malloc(initialSize);
			if (!m_pBuffer)
				return;
			m_AllocatedSize = initialSize;
		}

		BasicBuffer(const char *pszString, size_t length = -1) :
		  m_pBuffer(0),
		  m_UsedSize(0),
		  m_AllocatedSize(0)
		{
			if (!pszString)
				return;
			if (!length)
				return;
			if (length == -1)
				length = strlen(pszString) + 1;
			m_AllocatedSize = length;
			m_UsedSize = m_AllocatedSize;
			m_pBuffer = malloc(m_AllocatedSize);
			if (!m_pBuffer)
				return;
			memcpy(m_pBuffer, pszString, m_AllocatedSize);
		}

		~BasicBuffer()
		{
			if (m_pBuffer)
				free(m_pBuffer);
		}
#pragma endregion
#pragma region Various helper helper methods
		void Reset()
		{
			if (m_pBuffer)
			{
				free(m_pBuffer);
				m_pBuffer = NULL;
			}
			m_AllocatedSize = m_UsedSize = 0;
		}

		bool EnsureSize(size_t size, size_t allocIncrementStep = 0)
		{
			if (m_AllocatedSize >= size)
				return true;

			size_t incrementedSize = m_AllocatedSize + allocIncrementStep;
			if (size < incrementedSize)
				size = incrementedSize;

#ifdef EXTENDED_REALLOC
			m_pBuffer = _realloc(m_pBuffer, size, m_AllocatedSize);
#else
			m_pBuffer = realloc(m_pBuffer, size);
#endif
			if (!m_pBuffer)
			{
				m_AllocatedSize = m_UsedSize = 0;
				return false;
			}
			m_AllocatedSize = size;
			return true;
		}

		bool reserve(size_t size)
		{
			return EnsureSize(size);
		}
#pragma endregion
#pragma region Query methods
		bool Valid() const
		{
			return m_pBuffer && m_AllocatedSize;
		}

		void Fill(char val = 0)
		{
			if (m_pBuffer)
				memset(m_pBuffer, val, m_AllocatedSize);
		}

		size_t GetSize() const
		{
			return m_UsedSize;
		}

		size_t GetAllocated() const
		{
			return m_AllocatedSize;
		}

		const void *GetConstData() const
		{
			return m_pBuffer;
		}

		const void *GetConstData(size_t offset) const
		{
			return ((char *) m_pBuffer) + offset;
		}

		void *GetData(size_t offset)
		{
			return ((char *)m_pBuffer) + offset;
		}

		void *GetData(size_t offset, size_t requiredSize)
		{
			ASSERT((offset + requiredSize) <= m_UsedSize);
			return ((char *)m_pBuffer) + offset;
		}

		void *GetDataAfterEndOfUsedSpace()
		{
			ASSERT(m_UsedSize <= m_AllocatedSize);
			return ((char *)m_pBuffer) + m_UsedSize;
		}

		const void *GetDataAfterEndOfUsedSpace() const
		{
			ASSERT(m_UsedSize <= m_AllocatedSize);
			return ((char *)m_pBuffer) + m_UsedSize;
		}

		void *GetData()
		{
			return m_pBuffer;
		}
#pragma endregion
#pragma region Data set methods
		bool SetData(const void *pData, size_t size)
		{
			if (!EnsureSize(size))
				return false;
			memcpy(m_pBuffer, pData, size);
			m_UsedSize = size;
			return true;
		}

		void SetSize(size_t newSize)
		{
			EnsureSize(newSize);
			m_UsedSize = newSize;
		}

		void IncreaseSize(size_t sizeDelta)
		{
			ASSERT((int)sizeDelta >= 0);
			EnsureSize(m_UsedSize + sizeDelta);
			m_UsedSize += sizeDelta;
		}
#pragma endregion

		void *DetachBuffer()
		{
			void *pR = m_pBuffer;
			m_pBuffer = 0;
			m_AllocatedSize = 0;
			m_UsedSize = 0;
			return pR;
		}

		bool AppendData(const void *pData, size_t Size, size_t allocIncrementStep = 0)
		{
			if (!Size)
				return true;
			if (!pData)
				return false;
			if (!EnsureSize(GetSize() + Size, allocIncrementStep))
				return false;
			memcpy((char *)GetData() + GetSize(), pData, Size);
			SetSize(GetSize() + Size);
			return true;
		}

		bool append(const void *pData, size_t Size, size_t allocIncrementStep = 0)
		{
			return AppendData(pData, Size, allocIncrementStep);
		}
	};

	typedef BasicBuffer CBuffer;

	template <class _Type> class TypedBuffer : public BasicBuffer
	{
	public:
		TypedBuffer()
		{
		}

		TypedBuffer(const TypedBuffer &_Right)
		{
			EnsureSizeAndSetUsed(_Right.GetSize());
			if (_Right.GetSize())
				memcpy(GetData(), _Right.GetConstData(), _Right.GetSize());
		}

		TypedBuffer &operator= (const TypedBuffer &_Right)
		{
			EnsureSizeAndSetUsed(_Right.GetSize());
			if (_Right.GetSize())
				memcpy(GetData(), _Right.GetConstData(), _Right.GetSize());
			return *this;
		}

	public:
		typedef _Type _MyType;

	public:
		operator _Type *()
		{
			return (_Type *)GetData();
		}

		_Type *operator->()
		{
			return (_Type *)GetData();
		}

		_Type &operator[](unsigned Index)
		{
			ASSERT(((Index + 1) * sizeof(_Type)) <= GetSize());
			return ((_Type *)GetData())[Index];
		}

		bool EnsureSize(size_t size = sizeof(_Type))
		{
			return BasicBuffer::EnsureSize(size);
		}

		bool EnsureSizeAndSetUsed(size_t size = sizeof(_Type))
		{
			bool ret = EnsureSize(size);
			SetSize(size);
			return ret;
		}

		void *GetDataAfterStructure()
		{
			return GetData(sizeof(_Type));
		}

	};

	template <class _Type, size_t _Size> class TypedFixedSizeBuffer
	{
	protected:
		char m_Data[_Size];

	public:

		enum {DataSize = _Size};

		_Type *operator->()
		{
			return (_Type *)m_Data;
		}

		operator _Type *()
		{
			return (_Type *)m_Data;
		}
	};

	template <class _Type, size_t _ExtraSize> class FixedSizeVarStructWrapper : public TypedFixedSizeBuffer<_Type, sizeof(_Type) + _ExtraSize>
	{
	private:
		typedef TypedFixedSizeBuffer<_Type, sizeof(_Type) + _ExtraSize> _Base;

	public:
		enum {ExtraDataSize = _ExtraSize};

		void *GetExtraBlock()
		{
			return _Base::m_Data + sizeof(_Type);
		}
	};

	template <class _Type> class TempArrayReference
	{
	private:
		_Type *m_pArray;
		size_t m_Count;

	public:
		TempArrayReference(_Type *pData = NULL, size_t count = 0)
			: m_pArray(pData)
			, m_Count(count)
		{
		}

		_Type *GetData()
		{
			return m_pArray;
		}

		const _Type *GetData() const
		{
			return m_pArray;
		}

		size_t GetCount()
		{
			return m_Count;
		}

		size_t GetSizeInBytes()
		{
			return m_Count * sizeof(_Type);
		}

		bool Empty()
		{
			return !m_pArray || !m_Count;
		}
	};

	template <class _Ty> class SolidVector
	{
	private:
		_Ty *m_pBuffer;
		size_t m_Allocated, m_Used;

	public:
		SolidVector(size_t elementCount = 0)
			: m_Allocated(elementCount)
			, m_Used(0)
		{
			m_pBuffer = (_Ty *)malloc(elementCount * sizeof(_Ty));
		}

		~SolidVector()
		{
			if (m_pBuffer)
				free(m_pBuffer);
		}

		bool EnsureSize(size_t newSize, bool preserveContent = true, bool setUsed = false)
		{
			if (newSize <= m_Allocated)
			{
				if (setUsed)
					m_Used = newSize;
				return true;
			}

			m_Allocated = newSize;

			if (!preserveContent)
			{
				if (m_pBuffer)
					free(m_pBuffer);
				m_pBuffer = (_Ty *)malloc(newSize * sizeof(_Ty));
				m_Used = 0;
			}
			else
			{
				m_pBuffer = (_Ty *)realloc(m_pBuffer, newSize * sizeof(_Ty));
				if (m_Used > m_Allocated)
					m_Used = m_Allocated;
			}

			if (setUsed)
				m_Used = newSize;
			return (m_pBuffer != NULL);
		}

		_Ty *GetData(size_t offset = 0)
		{
			return m_pBuffer + offset;
		}

		unsigned char *GetDataAtByteOffset(size_t offset = 0)
		{
			return (unsigned char *)m_pBuffer + offset;
		}

		_Ty *GetDataAtEndOfUsed()
		{
			return m_pBuffer + m_Used;
		}

		_Ty &operator[](size_t index)
		{
			ASSERT(index < m_Used);
			return m_pBuffer[index];
		}

		size_t GetAllocated()
		{
			return m_Allocated;
		}

		size_t GetUsed()
		{
			return m_Used;
		}

		size_t GetUsedBytes()
		{
			return m_Used * sizeof(_Ty);
		}

		void SetUsed(size_t newUsed)
		{
			ASSERT(newUsed <= m_Allocated);
			m_Used = newUsed;
		}
	};

	template <class _Ty> class ACSimpleArray : AUTO_OBJECT
	{
	private:
		_Ty *m_pBuffer;
		size_t m_ElementCount;

	public:
		ACSimpleArray(size_t elementCount)
			: m_ElementCount(elementCount)
		{
			m_pBuffer = new _Ty[elementCount];
		}

		~ACSimpleArray()
		{
			delete m_pBuffer;
		}

		_Ty *GetData()
		{
			return m_pBuffer;
		}

		_Ty &operator[](size_t index)
		{
			ASSERT(index < m_ElementCount);
			return m_pBuffer[index];
		}
	};
}