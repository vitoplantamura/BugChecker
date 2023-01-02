#pragma once
#include "../string.h"

namespace BazisLib
{
	using BazisLib::RegistryKey;

	namespace Win32
	{
		class ConfigurationPath
		{
		private:
#ifndef BZSLIB_WINKERNEL
			HKEY _RootKey;
#endif
			String _Path;

		private:
			friend class ConfigurationKey;

		public:
#ifdef BZSLIB_WINKERNEL
			static ConfigurationPath FromNTRegistryPath(const String &path)
			{
				ConfigurationPath result;
				result._Path = path;
				return result;
			}
#else
			static ConfigurationPath FromWin32RegistryPath(HKEY hRootKey, const String &path)
			{
				ConfigurationPath result;
				result._RootKey = hRootKey;
				result._Path = path;
				return result;
			}
#endif

			String ToString() const
			{
				return _Path;
			}

			static ConfigurationPath Combine(const ConfigurationPath &path, const String &subkey)
			{
				ConfigurationPath result = path;
				result._Path.append(_T("\\"));
				result._Path.append(subkey);
				return result;
			}

			bool Valid()
			{
				return !_Path.empty();
			}
		};

		class ConfigurationKey
		{
			RegistryKey _Key;

		public:
			ConfigurationKey(const ConfigurationPath &path, bool writeAccess = false)
#ifdef BZSLIB_WINKERNEL
				: _Key(path._Path.c_str(), !writeAccess)
#else
				: _Key(path._RootKey, path._Path.c_str(), 0, writeAccess)
#endif
			{
			}

			bool Valid()
			{
				return _Key.Valid();
			}

		public:
			class accessor
			{
			private:
				ConfigurationKey &_Key;
				const TCHAR *_Name;

				friend class ConfigurationKey;
				accessor(ConfigurationKey &key, const TCHAR *pName)
					: _Key(key)
					, _Name(pName)
				{
				}

				accessor(const accessor &acc) :
					_Name(acc._Name),
					_Key(acc._Key)
				{
				}

				void operator=(const accessor &acc)
				{
					ASSERT(false);
				}

			public:
				operator int()
				{
					return _Key._Key[_Name];
				}

				operator String()
				{
					return _Key._Key[_Name];
				}

				ActionStatus operator=(const int &value)
				{
					return _Key._Key[_Name] = value;
				}

				ActionStatus operator=(const TCHAR *value)
				{
					return _Key._Key[_Name] = value;
				}

				ActionStatus operator=(const String &value)
				{
					return _Key._Key[_Name] = value;
				}

			};

		public:
			ConfigurationKey(const accessor &acc)
				: _Key(acc._Key._Key[acc._Name])
			{

			}

			accessor operator[](const TCHAR *pKey)
			{
				return accessor(*this, pKey);
			}

			std::vector<String> GetAllSubkeys()
			{
				std::vector<String> result;
				for (int i = 0; ;i++)
				{
					String subkey = _Key.EnumSubkey(i);
					if (subkey.empty())
						break;
					result.push_back(subkey);
				}

				return result;
			}
		};
	}

	typedef Win32::ConfigurationKey ConfigurationKey;
	typedef Win32::ConfigurationPath ConfigurationPath;
}
