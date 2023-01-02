#pragma once
#include "cmndef.h"
#include "objmgr.h"

#include <list>
#include <vector>

namespace BazisLib
{
	namespace ObjectManager
	{
		template <class _ManagedType, class _InternalCollection> class ManagedCollectionBase : AUTO_OBJECT
		{
		private:
			typedef ManagedCollectionBase<_ManagedType, _InternalCollection> _ThisCollection;

		protected:
			_InternalCollection _List;

		public:
			size_t size() {return _List.size();}

			void push_back(_MP(_ManagedType) pObj)
			{
				pObj->RegisterClient(this);
				_List.push_back(pObj);
			}

			void push_front(_MP(_ManagedType) pObj)
			{
				pObj->RegisterClient(this);
				_List.push_front(pObj);
			}

			bool empty() {return _List.empty();}

			void clear()
			{
				for(_InternalCollection::iterator it = _List.begin(); it != _List.end(); it++)
					if (*it)
						(*it)->RemoveClient(this);
				return _List.clear();
			}

			~ManagedCollectionBase()
			{
				for(_InternalCollection::iterator it = _List.begin(); it != _List.end(); it++)
					if (*it)
						(*it)->RemoveClient(this);
			}

			ManagedPointer<_ManagedType>& operator[](size_t index)
			{
				return _List[index];
			}

		public:
			class iterator
			{
			protected:
				typedef typename typename _InternalCollection::const_iterator _RawIterator;

				typename _RawIterator _It;
				friend class _ThisCollection;

			protected:
				iterator(_RawIterator it)
					: _It(it)
				{
				}

			public:
				iterator (iterator &it)
					: _It(it._It)
				{
				}

				const ManagedPointer<_ManagedType> &operator*()
				{
					return *_It;
				}

				const _ManagedType *operator->() const
				{
					return (*_It).operator->();
				}

				bool operator==(const iterator &right) const
				{
					return _It == right._It;
				}

				bool operator!=(const iterator &right) const
				{
					return _It != right._It;
				}

				void operator++()
				{
					++_It;
				}

				void operator++(int)
				{
					++_It;
				}
			};

			class const_iterator
			{
			protected:
				typedef typename typename _InternalCollection::const_iterator _RawIterator;

				typename _InternalCollection::const_iterator _It;
				friend class _ThisCollection;

			protected:
				const_iterator(_RawIterator it)
					: _It(it)
				{
				}

			public:
				const_iterator (const_iterator &it)
					: _It(it._It)
				{
				}

				const_iterator (iterator &it)
					: _It(it._It)
				{
				}

				const ManagedPointer<_ManagedType> &operator*()
				{
					return *_It;
				}

				const _ManagedType *operator->() const
				{
					return (*_It).operator->();
				}

				bool operator==(const const_iterator &right) const
				{
					return _It == right._It;
				}

				bool operator!=(const const_iterator &right) const
				{
					return _It != right._It;
				}

				void operator++()
				{
					++_It;
				}

				void operator++(int)
				{
					++_It;
				}
			};

		public:
			iterator begin() {return iterator(_List.begin());}
			iterator end() {return iterator(_List.end());}

			const_iterator begin() const {return const_iterator(_List.begin());}
			const_iterator end() const {return const_iterator(_List.end());}

			const_iterator const_begin() const {return const_iterator(_List.begin());}
			const_iterator const_end() const {return const_iterator(_List.end());}
		};

		template <class _ManagedType> class ManagedList : public ManagedCollectionBase<_ManagedType, std::list<ManagedPointer<_ManagedType>>>
		{
		};

		template <class _ManagedType> class ManagedVector : public ManagedCollectionBase<_ManagedType, std::vector<ManagedPointer<_ManagedType>>>
		{
		public:
			ManagedPointer<_ManagedType> &Get(size_t idx)
			{
				return _List[idx];
			}
		};
	}
}