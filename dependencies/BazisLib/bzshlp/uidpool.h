#pragma once
#include "../bzscore/sync.h"
#include "cmndef.h"

namespace BazisLib
{
	/*! The UniqueIdPool class allows maintains a list of IDs allowing clients to allocate/release some of them.
		If you want to assign a locally unique ID to any newly created object, consider using this class. It has
		constant running time for both allocation and freeing operations.
	*/
	class UniqueIdPool
	{
	public:
		typedef INT_PTR _type;
		static const _type BadID = -1;

	private:
		_type *m_pValues;
		_type *m_pFirstFree;
		Mutex m_AccessLock;

	public:
		UniqueIdPool(unsigned MaxIDCount)
		{
			//If you got an error here, you're trying to compile this code while sizeof(ptr) is greater than sizeof(_type)
			//You should then change from int to some kind of INT_PTR
			static char __unused[sizeof(_type)-sizeof(_type *)+1];

			m_pValues = new _type[MaxIDCount];
			if (!m_pValues)
				return;
			for (unsigned i = 0; i < (MaxIDCount - 1); i++)
				((_type **)m_pValues)[i] = &m_pValues[i+1];
			m_pValues[MaxIDCount - 1] = 0;

			m_pFirstFree = m_pValues;
		}

		~UniqueIdPool()
		{
			delete m_pValues;
		}

		_type AllocateID()
		{
			if (!m_pValues)
				return BadID;
			MutexLocker lck(m_AccessLock);
			if (!m_pFirstFree)
				return BadID;
			_type id = m_pFirstFree - m_pValues;
			m_pFirstFree = (_type *)*m_pFirstFree;
			return id;
		}

		void ReleaseID(_type ID)
		{
			if ((ID == BadID) || !m_pValues)
				return;
			MutexLocker lck(m_AccessLock);
			_type *pID = m_pValues + ID;
			*pID = (_type)m_pFirstFree;
			m_pFirstFree = pID;
		}
	};
}