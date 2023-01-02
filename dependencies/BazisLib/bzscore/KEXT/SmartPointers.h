#pragma once

namespace BazisLib
{
	namespace KEXT
	{
		template <class _Obj> class ObjectPointer
		{
			_Obj *m_pObj;

		public:
			ObjectPointer(_Obj *pObj = NULL, bool releaseOriginal = false)
				: m_pObj(pObj)
			{
				if (m_pObj && !releaseOriginal)
					m_pObj->retain();
			}

			~ObjectPointer()
			{
				if (m_pObj)
					m_pObj->release();
			}

			void assign(_Obj *pObj)
			{
				if (m_pObj)
					m_pObj->release();
				m_pObj = pObj;
				if (m_pObj)
					m_pObj->retain();
			}

			ObjectPointer(const ObjectPointer<_Obj> &another)
				: m_pObj(another.m_pObj)
			{
				if (m_pObj)
					m_pObj->retain();
			}

			ObjectPointer &operator=(const ObjectPointer<_Obj> &another)
			{
				assign(another.m_pObj);
				return *this;
			}

			ObjectPointer &operator=(_Obj *another)
			{
				assign(another);
				return *this;
			}

			_Obj *operator->()
			{
				return m_pObj;
			}

			operator bool()
			{
				return !!m_pObj;
			}

			_Obj *GetObjectForSingleUse()
			{
				return m_pObj;
			}
		};
	}
}