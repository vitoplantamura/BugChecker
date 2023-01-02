#pragma once

#include "../cmndef.h"
#include "../string.h"
#include "../assert.h"
#include "../buffer.h"

namespace BazisLib
{
	enum Win32RootKey
	{
		HKEY_LOCAL_MACHINE,
		HKEY_USERS,
	};

	namespace DDK
	{
		//! See the BazisLib::Win32::RegistryKey class for more info
		class RegistryKey
		{
		public:
			//! See the BazisLib::Win32::RegistryKey::accessor class for more info
			class accessor
			{
			private:
				const wchar_t *m_pName;
				RegistryKey *m_pKey;

				friend class RegistryKey;

			private:

				accessor(RegistryKey *pKey, const wchar_t *pName) :
				   m_pKey(pKey),
					   m_pName(pName)
				   {
					   ASSERT(pName && pKey);
				   }

				   accessor(const accessor &acc) :
				   m_pName(acc.m_pName),
					   m_pKey(acc.m_pKey)
				   {
				   }

				   void operator=(const accessor &acc)
				   {
					   m_pName = acc.m_pName;
					   m_pKey = acc.m_pKey;
				   }

			private:
				ActionStatus _assign(const wchar_t *ptszStr, unsigned count = 0)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					ASSERT(ptszStr);
					UNICODE_STRING name;
					RtlInitUnicodeString(&name, m_pName);
					if (!count)
						count = (unsigned)wcslen(ptszStr);
					NTSTATUS st = ZwSetValueKey(m_pKey->m_hKey, &name, 0, REG_SZ, (PVOID)ptszStr, (count + 1) * sizeof(wchar_t));
					return MAKE_STATUS(ActionStatus::FromNTStatus(st));
				}

			public:
#pragma region Strong-typed query methods
				ActionStatus ReadValue(int *pValue)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					ASSERT(m_pName && pValue);
					FixedSizeVarStructWrapper <KEY_VALUE_PARTIAL_INFORMATION, sizeof(int)> data;
					UNICODE_STRING name;
					RtlInitUnicodeString(&name, m_pName);
					ULONG done = 0;
					NTSTATUS st = ZwQueryValueKey(m_pKey->m_hKey, &name, KeyValuePartialInformation, data, data.DataSize, &done);
					if (!NT_SUCCESS(st))
						return MAKE_STATUS(ActionStatus::FromNTStatus(st));
					*pValue = *((int *)data->Data);
					return MAKE_STATUS(Success);
				}

				ActionStatus ReadValue(String *pValue)
				{
					ASSERT(m_pName && pValue);
					TypedBuffer <KEY_VALUE_PARTIAL_INFORMATION> pData;
					ActionStatus st = QueryValueEx(&pData);
					if (!st.Successful())
						return st;
					if (!pData)
						return MAKE_STATUS(UnknownError);
					pValue->assign((wchar_t *)pData->Data, pData->DataLength / sizeof(wchar_t));
					while (pValue->length() > 0 && !(*pValue)[pValue->length() - 1] == 1)
						pValue->SetLength(pValue->length() - 1);
					return MAKE_STATUS(Success);
				}

				ActionStatus ReadValue(unsigned *pValue)
				{
					return ReadValue((int *)pValue);
				}

				ActionStatus ReadValue(bool *bValue)
				{
					int val = 0;
					ActionStatus st = ReadValue(&val);
					if (!st.Successful())
						return st;
					*bValue = (val != 0);
					return MAKE_STATUS(Success);
				}

/*				bool ReadValue(LONGLONG *pValue)
				{
				}

				bool ReadValue(ULONGLONG *pValue)
				{
					return ReadValue((LONGLONG *)pValue);
				}*/

#pragma endregion
#pragma region Simple query operators
				operator int()
				{
					int value = 0;
					ReadValue(&value);
					return value;
				}

				operator String()
				{
					String str;
					ReadValue(&str);
					return str;
				}
#pragma endregion
#pragma region Simple set operators
				ActionStatus operator=(const int &value)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					UNICODE_STRING valName;
					RtlInitUnicodeString(&valName, m_pName);
					NTSTATUS st = ZwSetValueKey(m_pKey->m_hKey, &valName, 0, REG_DWORD, (PVOID)&value, 4);
					return MAKE_STATUS(ActionStatus::FromNTStatus(st));
				}

				ActionStatus operator=(const wchar_t *ptszStr)
				{
					return _assign(ptszStr);
				}

				ActionStatus operator=(const String &str)
				{
					return _assign(str.c_str(), (unsigned)str.size());
				}

#pragma endregion
#pragma region Strong-typed set methods

				template <class _Ty> ActionStatus WriteValue(_Ty *pValue)
				{
					return (*this) = *pValue;
				}

				template<> ActionStatus WriteValue<LONGLONG>(LONGLONG *pValue)
				{
					wchar_t tsz[128];
					int len = _snwprintf(tsz, sizeof(tsz), _T("%I64d"), *pValue);
					return _assign(tsz, len);
				}

				template<> ActionStatus WriteValue<ULONGLONG>(ULONGLONG *pValue)
				{
					wchar_t tsz[128];
					int len = _snwprintf(tsz, sizeof(tsz), _T("%I64u"), *pValue);
					return _assign(tsz, len);
				}
#pragma endregion
#pragma region Additional methods
				//! Queries a registry value independent from its type
				ActionStatus QueryValueEx(TypedBuffer <KEY_VALUE_PARTIAL_INFORMATION> *pData)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					ASSERT(m_pName && pData);
					pData->EnsureSizeAndSetUsed();
					UNICODE_STRING name;
					RtlInitUnicodeString(&name, m_pName);
					ULONG done = 0;
					NTSTATUS st = ZwQueryValueKey(m_pKey->m_hKey, &name, KeyValuePartialInformation, *pData, (ULONG)pData->GetSize(), &done);
					if (!done)
					{
						if (NT_SUCCESS(st))
							return MAKE_STATUS(UnknownError);
						else
							return MAKE_STATUS(ActionStatus::FromNTStatus(st));
					}
					pData->EnsureSizeAndSetUsed(done);
					st = ZwQueryValueKey(m_pKey->m_hKey, &name, KeyValuePartialInformation, *pData, (ULONG)pData->GetSize(), &done);
					return MAKE_STATUS(ActionStatus::FromNTStatus(st));
				}

				ActionStatus SetValueEx(const void *pBuffer, ULONG BufferSize, ULONG dwType)
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					UNICODE_STRING name;
					RtlInitUnicodeString(&name, m_pName);
					NTSTATUS st = ZwSetValueKey(m_pKey->m_hKey, &name, 0, dwType, (PVOID)pBuffer, BufferSize);
					return MAKE_STATUS(ActionStatus::FromNTStatus(st));
				}

				ULONG QueryType()
				{
					ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
					ASSERT(m_pName);
					KEY_VALUE_PARTIAL_INFORMATION data;
					UNICODE_STRING name;
					RtlInitUnicodeString(&name, m_pName);
					ULONG done;
					ZwQueryValueKey(m_pKey->m_hKey, &name, KeyValuePartialInformation, &data, sizeof(data), &done);
					if (done < sizeof(data))
						return 0;
					return data.Type;
				}
#pragma endregion

			};

		private:
			HANDLE m_hKey;

#pragma region Constructors

		private:
			//The RegistryKey object is non-copyable by definition
			RegistryKey(const RegistryKey &key) :
			   m_hKey(NULL)
			   {
				   ASSERT(0);
			   }

			   void operator=(const RegistryKey &key)
			   {
				   ASSERT(0);
			   }

		private:
			void Initialize(HANDLE hRoot, const wchar_t *pwszSubkey, bool ReadOnly, size_t SubKeyLen = -1);

		public:

			//! Creates or opens a registry key for a given NT-format key path
			/*! 
				\param pwszKeyPath Specifies key path to open. For example,
					<b>\Registry\Machine\SYSTEM\MountedDevices</b>
			*/
			RegistryKey(LPCWSTR pwszKeyPath, bool ReadOnly = false);

			RegistryKey(const RegistryKey &rootKey, const wchar_t *ptszSubkey, bool ReadOnly = false, size_t SubKeyLen = -1);
			
			//! Creates or opens a key in a way compatible with Win32 call
			RegistryKey(Win32RootKey hRootKey, const wchar_t *ptszSubkey, bool ReadOnly = false);

			RegistryKey(const accessor &acc, bool ReadOnly = false);


#pragma endregion

		private:
			KEY_VALUE_INFORMATION_CLASS _KeyValueInfoClass(KEY_VALUE_BASIC_INFORMATION *) {return KeyValueBasicInformation; }
			KEY_VALUE_INFORMATION_CLASS _KeyValueInfoClass(KEY_VALUE_FULL_INFORMATION  *) {return KeyValueFullInformation; }
			KEY_VALUE_INFORMATION_CLASS _KeyValueInfoClass(KEY_VALUE_PARTIAL_INFORMATION  *) {return KeyValuePartialInformation; }

		public:

			  ~RegistryKey()
			  {
				  if (m_hKey)
					ZwClose(m_hKey);
			  }

			  accessor operator[](const wchar_t *ptszValue)
			  {
				  return accessor(this, ptszValue);
			  }

			  bool Valid()
			  {
				  return m_hKey != 0;
			  }

			  bool EnumSubkey(unsigned Index, TypedBuffer<KEY_BASIC_INFORMATION> *pName)
			  {
				  ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				  if (!pName)
					  return false;
				  ULONG resultLen = 0;
				  ZwEnumerateKey(m_hKey, Index, KeyBasicInformation, NULL, 0, &resultLen);
				  if (!resultLen)
					  return false;
				  pName->EnsureSizeAndSetUsed(resultLen);
				  return NT_SUCCESS(ZwEnumerateKey(m_hKey, Index, KeyBasicInformation, *pName, (ULONG)pName->GetSize(), &resultLen));
			  }

			  bool DeleteSubKey(LPCWSTR lpSubkey, size_t subKeyLenInWchars = -1)
			  {
				  ASSERT(lpSubkey);
				  if (!Valid())
					  return false;
				  RegistryKey subkey(*this, lpSubkey, false, subKeyLenInWchars);
				  if (!subkey.Valid())
					  return false;
				  return NT_SUCCESS(ZwDeleteKey(subkey.m_hKey));
			  }

			  bool DeleteSelf()
			  {
				  ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				  if (!Valid())
					  return false;
				  return NT_SUCCESS(ZwDeleteKey(m_hKey));
			  }

			  bool DeleteValue(LPCWSTR lpValue, size_t LengthInWchars = -1)
			  {
				  ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				  if (!Valid() || !lpValue)
					  return false;
				  UNICODE_STRING str;
				  if (LengthInWchars == -1)
					  RtlInitUnicodeString(&str, lpValue);
				  else
				  {
					  str.Buffer = (PWCH)lpValue;
					  str.Length = (USHORT)(LengthInWchars * sizeof(wchar_t));
					  str.MaximumLength = str.Length + sizeof(wchar_t);
				  }
				  return NT_SUCCESS(ZwDeleteValueKey(m_hKey, &str));
			  }

			  template <class _KEY_VALUE_XXX_INFORMATION> bool EnumValue(unsigned Index, TypedBuffer<_KEY_VALUE_XXX_INFORMATION> *pBuffer)
			  {
				  ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				  if (!pBuffer)
					  return false;
				  ULONG resultLen = 0;
				  ZwEnumerateValueKey(m_hKey, Index, _KeyValueInfoClass((TypedBuffer<_KEY_VALUE_XXX_INFORMATION>::_MyType *)NULL), NULL, 0, &resultLen);
				  if (!resultLen)
					  return false;
				  pBuffer->EnsureSizeAndSetUsed(resultLen);
				  return NT_SUCCESS(ZwEnumerateValueKey(m_hKey, Index, _KeyValueInfoClass((TypedBuffer<_KEY_VALUE_XXX_INFORMATION>::_MyType *)NULL), *pBuffer, pBuffer->GetSize(), &resultLen));
			  }

			  //! Deletes a key and all its subkeys
			  /*!
					\param LengthInWchars Specifies the length of the subkey string in wchar_t elements, not including the trailing zero.
			  */
			  bool DeleteSubKeyRecursive(LPCWSTR lpSubkey, size_t LengthInWchars = -1);

			  String EnumSubkey(unsigned Index)
			  {
				  ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
				  TypedBuffer<KEY_BASIC_INFORMATION> nameBuf;
				  if (!EnumSubkey(Index, &nameBuf) || !nameBuf)
					  return L"";
				  return String(nameBuf->Name, nameBuf->NameLength / sizeof(wchar_t));
			  }

/*			  //! This method is called by serializer subsystem to read/write a field of a serialized structure
			  template <class _Type> bool _SerializerEntry(const wchar_t *ptszValue, _Type *ref, bool Save)
			  {
				  if (Save)
					  (*this)[ptszValue].WriteValue(ref);
				  else
					  (*this)[ptszValue].ReadValue(ref);
				  return true;
			  }*/

		public:
			NTSTATUS ReadString(LPCWSTR pwszValueName, String &value);


		/*private:
			IMPLEMENT_EMPTY_SERIALIZE_WRAPPER;
			IMPLEMENT_EMPTY_DESERIALIZE_WRAPPER;

		public:
			IMPLEMENT_STD_SERIALIZE_METHOD;
			IMPLEMENT_STD_DESERIALIZE_METHOD;*/
		};
	}

	using DDK::RegistryKey;
}
