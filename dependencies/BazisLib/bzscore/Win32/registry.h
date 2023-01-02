#pragma once
#include "../cmndef.h"
#include "../string.h"
#include "../assert.h"
#include "../status.h"

namespace BazisLib
{
	ActionStatus DoDeleteSubkeyRecursive(HKEY hKey, LPCTSTR lpSubkey);

	namespace Win32
	{
	//! Provides extremely easy and fast registry access
	/*! The RegistryKey class allows accessing registry keys according to the following
		scenario:
<pre>
	RegistryKey key(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion"));
	String str = key[_T("ProgramFilesDir")];
	str[0] = 'C';
	key[_T("ProgramFilesDir")] = str;

	...
	RegistryKey subKey(key, "Run");
	...
</pre>
	\remarks The RegistryKey and the RegistryKey::accessor classes are highly optimized for inline usage.
			 Using these classes according to the scenario presented above guarantees NO EXTRA instructions
			 compared with classical C approach, except for two cases:
			 <ul>
				<li>Querying integer value reads it first to stack variable, and then - to the resulting variable</li>
				<li>Reading strings directly causes one extra allocation operation to be performed</li>
			</ul>
			To ensure maximum efficiency, the following inline methods can be used:
			<ul>
				<li>RegistryKey::accessor::QueryValueEx</li>
				<li>RegistryKey::accessor::SetValueEx</li>
				<li>RegistryKey::accessor::ReadToCharArray</li>
			</ul>
			Example:
<pre>
	...
	TCHAR tsz[256];
	key[_T("SomeValuename")].ReadToCharArray(tsz, sizeof(tsz));
	...
</pre>

	*/
		class RegistryKey
		{
		public:
			//! Represents an internal object used to read or write registry values
			/*! The RegistryKey::accessor class SHOULD NOT be used explicitly. The only correct usage
				is when such temporary object is returned from RegistryKey::operator[]().
				
				In that case optimizer prevents the object from being physically stored in stack.
				Instead, the key and value name pointers are directly passed to corresponding functions.

				To see how it works, you should compile the SAMPLES\\WINDOWS\\REGISTRY sample program
				in Release configuration, and disassemble it.

				All methods/operators are divided into the following groups:
				<ul>
					<li>Strong-typed query methods</li> allow reading value using key[idx].ReadValue(&val) syntax. 
					The main idea for them is to support strong typing, for example, distinguish between
					LONGLONG and ULONGLONG.
					<li>Simple query operators</li> allow reading the value as val = key[idx]
					<li>Simple set operators</li> for writing using simple syntax as key[idx] = val
					<li>Strong-typed set methods</li> allow distinguishing between some similar types, such
					as LONGLONG and ULONGLONG.
					<li>Additional methods, such as SetValueEx()/QueryValueEx()</li> 
				</ul>
			*/
			class accessor
			{
			private:
				const TCHAR *m_pName;
				RegistryKey *m_pKey;

				friend class RegistryKey;

			private:

				accessor(RegistryKey *pKey, const TCHAR *pName) :
					m_pKey(pKey),
					m_pName(pName)
				{
					ASSERT(pKey);
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
				ActionStatus _assign(const TCHAR *ptszStr, unsigned count = 0, int type = REG_SZ)
				{
					ASSERT(ptszStr);
					if (!count)
						count = (unsigned)_tcslen(ptszStr);
					if (RegSetValueEx(m_pKey->m_hKey, m_pName, NULL, type, (LPBYTE)ptszStr, (count + 1) * sizeof(TCHAR)))
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					return MAKE_STATUS(Success);
				}

			public:
#pragma region Strong-typed query methods
				ActionStatus ReadValue(int *pValue)
				{
					DWORD count = 4;
#ifdef _DEBUG
					DWORD dwType;
					if (RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, &dwType, (LPBYTE)pValue, &count))
#else
					if (RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, NULL, (LPBYTE)pValue, &count))
#endif
						return MAKE_STATUS(ActionStatus::FailFromLastError());

#ifdef _DEBUG
					ASSERT(dwType == REG_DWORD);
#endif
					return MAKE_STATUS(Success);
				}

				ActionStatus ReadValue(String *pValue)
				{
					DWORD count = 0;
#ifdef _DEBUG
					DWORD dwType;
					if (RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, &dwType, NULL, &count))
#else
					if (RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, NULL, NULL, &count))
#endif
						return MAKE_STATUS(ActionStatus::FailFromLastError());

#ifdef _DEBUG
				ASSERT((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ));
#endif

					TCHAR *ptszStr = new TCHAR[count / sizeof(TCHAR)];
					if (!ptszStr)
						return MAKE_STATUS(InvalidParameter);
					if (RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, NULL, (LPBYTE)ptszStr, &count))
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					pValue->assign(ptszStr);
					delete ptszStr;
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

				ActionStatus ReadValue(LONGLONG *pValue)
				{
					DWORD count = 0;
#ifdef _DEBUG
					DWORD dwType;
					if (RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, &dwType, NULL, &count))
#else
					if (RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, NULL, NULL, &count))
#endif
						return MAKE_STATUS(ActionStatus::FailFromLastError());

#ifdef _DEBUG
				ASSERT((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ));
#endif

					TCHAR *ptszStr = new TCHAR[count / sizeof(TCHAR)];
					if (!ptszStr)
						return MAKE_STATUS(InvalidParameter);
					if (RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, NULL, (LPBYTE)ptszStr, &count))
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					_stscanf_s(ptszStr, _T("%I64d"), pValue);
					delete ptszStr;
					return MAKE_STATUS(Success);
				}

				ActionStatus ReadValue(ULONGLONG *pValue)
				{
					return ReadValue((LONGLONG *)pValue);
				}

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
					if (RegSetValueEx(m_pKey->m_hKey, m_pName, NULL, REG_DWORD, (LPBYTE)&value, 4))
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					return MAKE_STATUS(Success);
				}

				ActionStatus operator=(const TCHAR *ptszStr)
				{
					return _assign(ptszStr);
				}

				ActionStatus AssignExpandable(const TCHAR *ptszStr)
				{
					return _assign(ptszStr, 0, REG_EXPAND_SZ);
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
					TCHAR tsz[128];
					int len = _sntprintf_s(tsz, _countof(tsz), _TRUNCATE, _T("%I64d"), *pValue);
					return _assign(tsz, len);
				}

				template<> ActionStatus WriteValue<ULONGLONG>(ULONGLONG *pValue)
				{
					TCHAR tsz[128];
					int len = _sntprintf_s(tsz, _countof(tsz), _TRUNCATE, _T("%I64u"), *pValue);
					return _assign(tsz, len);
				}
#pragma endregion
#pragma region Additional methods
				//! Queries a registry value independent from its type
				ActionStatus QueryValueEx(void *pBuffer, DWORD BufferSize, DWORD *dwRealSize = NULL, DWORD *pType = NULL)
				{
					if (RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, pType, (LPBYTE)pBuffer, &BufferSize))
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					if (dwRealSize)
						*dwRealSize = BufferSize;
					return MAKE_STATUS(Success);
				}

				ActionStatus SetValueEx(const void *pBuffer, DWORD BufferSize, DWORD dwType)
				{
					if (RegSetValueEx(m_pKey->m_hKey, m_pName, NULL, dwType, (LPBYTE)pBuffer, BufferSize))
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					return MAKE_STATUS(Success);
				}
				
				ActionStatus ReadToCharArray(TCHAR *ptszArray, DWORD SizeInBytes)
				{
					if (RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, NULL, (LPBYTE)ptszArray, &SizeInBytes))
						return MAKE_STATUS(ActionStatus::FailFromLastError());
					return MAKE_STATUS(Success);
				}

				DWORD QueryType()
				{
					DWORD Type = 0;
					RegQueryValueEx(m_pKey->m_hKey, m_pName, NULL, &Type, NULL, NULL);
					return Type;
				}
#pragma endregion

			};

		private:
			HKEY m_hKey;

#pragma region Constructors

		private:
			//The RegistryKey object is non-copiable by definition
			RegistryKey(const RegistryKey &key) :
				m_hKey(NULL)
			{
				ASSERT(0);
			}

			void operator=(const RegistryKey &key)
			{
				ASSERT(0);
			}

		public:
			RegistryKey(HKEY hRootKey, const TCHAR *ptszSubkey, REGSAM Wow64Options = 0, bool RequireWrite = true, ActionStatus *pStatus = NULL) :
			  m_hKey(0)
			{
				int err = RegCreateKeyEx(hRootKey, ptszSubkey, 0, NULL, 0, (RequireWrite ? KEY_ALL_ACCESS : KEY_READ) | Wow64Options, NULL, &m_hKey, NULL);
				ASSIGN_STATUS(pStatus, ActionStatus::FromWin32Error(err));
//				RegCreateKey(hRootKey, ptszSubkey, &m_hKey);
			}

			RegistryKey(const RegistryKey &rootKey, const TCHAR *ptszSubkey, REGSAM Wow64Options = 0) :
			  m_hKey(0)
			{
				RegCreateKeyEx(rootKey.m_hKey, ptszSubkey, 0, NULL, 0, KEY_ALL_ACCESS | Wow64Options, NULL, &m_hKey, NULL);
//				RegCreateKey(rootKey.m_hKey, ptszSubkey, &m_hKey);
			}

			RegistryKey(const accessor &keyAccessor, bool requireWrite = true, REGSAM Wow64Options = 0) :
			  m_hKey(0)
			{
				RegCreateKeyEx(keyAccessor.m_pKey->m_hKey, keyAccessor.m_pName, 0, NULL, 0, (requireWrite ? KEY_ALL_ACCESS : KEY_READ) | Wow64Options, NULL, &m_hKey, NULL);
//				RegCreateKey(rootKey.m_hKey, ptszSubkey, &m_hKey);
			}

#pragma endregion

			~RegistryKey()
			{
				RegCloseKey(m_hKey);
			}

			accessor operator[](const TCHAR *ptszValue)
			{
				return accessor(this, ptszValue);
			}

			bool Valid()
			{
				return m_hKey != 0;
			}

			bool EnumSubkey(unsigned Index, TCHAR *ptszArray, DWORD SizeInTChars)
			{
				return (RegEnumKeyEx(m_hKey, Index, ptszArray, &SizeInTChars, NULL, NULL, NULL, NULL) == 0);
			}

			bool DeleteSubKey(const String &subkey)
			{
				return (RegDeleteKey(m_hKey, subkey.c_str()) == 0);
			}

			ActionStatus DeleteSubKeyRecursive(const String &subkey)
			{
				return DoDeleteSubkeyRecursive(m_hKey, subkey.c_str());
			}

			String EnumSubkey(unsigned Index)
			{
				TCHAR szKeyName[256] = {0,};
				EnumSubkey(Index, szKeyName, sizeof(szKeyName)/sizeof(szKeyName[0]));
				return szKeyName;
			}
		};

/*		template <class _SerializeableStructure, LPCTSTR lpRegistryPath, HKEY hRootKey = HKEY_LOCAL_MACHINE, bool AutoLoad = true> class _RegistryParametersT : public _SerializeableStructure
		{
		public:
			_RegistryParametersT()
			{
				if (AutoLoad)
					LoadFromRegistry();
			}

			void LoadFromRegistry()
			{
				RegistryKey key(hRootKey, lpRegistryPath);
				key.DeserializeObject(*this);
			}

			void SaveToRegistry()
			{
				RegistryKey key(hRootKey, lpRegistryPath);
				key.SerializeObject(*this);
			}

		};*/
	}

	typedef Win32::RegistryKey RegistryKey;
}