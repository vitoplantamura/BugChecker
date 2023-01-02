#pragma once
#include "../serializer.h"
#include "../../bzscore/Win32/registry.h"
#include "../../bzscore/status.h"

namespace BazisLib
{
	namespace Win32
	{
		class RegistrySerializer
		{
		public:
			template <class _Structure> static ActionStatus Serialize(Win32::RegistryKey &key, _Structure &value)
			{
				auto worker = Worker(key);
				return value.__SERIALIZER_ITERATE_MEMBERS(worker, true);
			}

			template <class _Structure> static ActionStatus Deserialize(Win32::RegistryKey &key, _Structure &value, bool ignoreMissingValues = true)
			{
				return value.__SERIALIZER_ITERATE_MEMBERS(Worker(key, ignoreMissingValues), false);
			}

		private:
			class Worker
			{
				RegistryKey &Key;
				bool _IgnoreMissingValues;

			public:
				Worker(RegistryKey &key, bool ignoreMissingValues = true)
					: Key(key)
					, _IgnoreMissingValues(ignoreMissingValues)
				{
				}

				template <typename _Ty> ActionStatus inline _NestedSerializerHelper(const TCHAR *pName, _Ty *pObject, bool save, void *)
				{
					if (save)
						return Key[pName].WriteValue(pObject);
					else
					{
						ActionStatus st = Key[pName].ReadValue(pObject);
						if (_IgnoreMissingValues)
							return MAKE_STATUS(Success);
						return st;
					}
				}

				template <typename _Ty> ActionStatus inline _NestedSerializerHelper(const TCHAR *pName, _Ty *pObject, bool save, _SerializableStructBase *)
				{
					RegistryKey subKey(Key[pName], save);
					return pObject->__SERIALIZER_ITERATE_MEMBERS(Worker(subKey), save);
				}

				template <typename _Ty> ActionStatus inline _SerializerEntry(const TCHAR *pName, _Ty *pObject, bool save)
				{
					//Will invoke the version with _SerializableStructBase if _Ty is a serializable object itself.
					//The last argument is not used by the function and is ALWAYS the same as the second one.
					//However, it's used by the compiler to determine which of the _NestedSerializerHelper versions is used.
					return _NestedSerializerHelper(pName, pObject, save, pObject);	
				}
			};
		};
	}
}