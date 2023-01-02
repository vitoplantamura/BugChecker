#pragma once
#include "../bzscore/buffer.h"
#include "../bzscore/objmgr.h"
#include "../bzscore/autofile.h"

namespace BazisLib
{
	//! Represents a memory file. Object manager-compatible.
	class ACMemoryFile : public BasicFileBase, protected CBuffer
	{
	protected:
		size_t m_Pointer;

	public:
		ACMemoryFile(size_t InitialSize) :
		  CBuffer(InitialSize),
			  m_Pointer(0)
		  {
		  }

		  bool SetData(const void *pData, size_t size)
		  {
			  return __super::SetData(pData, size);
		  }


		  virtual bool Valid() override
		  {
			  return true;
		  }

		  virtual void Close() override
		  {
			  Reset();
			  m_Pointer = 0;
		  }

		  virtual size_t Read(void *pBuffer, size_t size, ActionStatus *pStatus = NULL, bool IncompleteReadSupported = false) override
		  {
			  if (!pBuffer)
			  {
				  ASSIGN_STATUS(pStatus, InvalidPointer);
				  return 0;
			  }
			  size_t fsize = CBuffer::GetSize();
			  ASSERT(fsize >= m_Pointer);
			  size_t todo = fsize - m_Pointer;
			  if (size > todo)
				  size = todo;
			  if (size)
				  memcpy(pBuffer, GetData(m_Pointer), size);
			  m_Pointer += size;
			  if (!todo)
				  ASSIGN_STATUS(pStatus, EndOfFile);
			  else
				  ASSIGN_STATUS(pStatus, Success);
			  return size;
		  }

		  virtual size_t Write(const void *pBuffer, size_t size, ActionStatus *pStatus = NULL) override
		  {
			  if (!pBuffer)
			  {
				  ASSIGN_STATUS(pStatus, InvalidPointer);
				  return 0;
			  }
			  if (!EnsureSize(m_Pointer + size))
			  {
				  ASSIGN_STATUS(pStatus, NoMemory);
				  return 0;
			  }
			  SetSize(m_Pointer + size);
			  memcpy(GetData(m_Pointer), pBuffer, size);
			  m_Pointer += size;
			  ASSIGN_STATUS(pStatus, Success);
			  return size;
		  }

		  virtual LONGLONG GetSize(ActionStatus *pStatus = NULL) override
		  {
			  ASSIGN_STATUS(pStatus, Success);
			  return CBuffer::GetSize();
		  }

		  virtual LONGLONG Seek(LONGLONG Offset, FileFlags::SeekType seekType, ActionStatus *pStatus = NULL) override
		  {
			  size_t size = CBuffer::GetSize();
			  switch (seekType)
			  {
			  case FileFlags::FileBegin:
				  m_Pointer = (size_t)Offset;
				  break;
			  case FileFlags::FileCurrent:
				  m_Pointer = (size_t)(m_Pointer + Offset);
				  break;
			  case FileFlags::FileEnd:
				  m_Pointer = (size_t)(size - Offset);
				  break;
			  }
			  if (m_Pointer > size)
				  m_Pointer = size;
			  ASSIGN_STATUS(pStatus, Success);
			  return m_Pointer;
		  }

		  virtual LONGLONG GetPosition(ActionStatus *pStatus = NULL) override
		  {
			  ASSIGN_STATUS(pStatus, Success);
			  return m_Pointer;
		  }
	};
}