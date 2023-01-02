#pragma once
#include "../string.h"
#include "../file.h"

namespace BazisLib
{
	namespace Posix
	{
		class ConfigurationPath
		{
		private:
			String _Path;

		private:
			friend class ConfigurationKey;

		public:
			static ConfigurationPath FromFSPath(const String &path)
			{
				ConfigurationPath result;
				result._Path = path;
				return result;
			}

			String ToString()
			{
				return _Path;
			}

			static ConfigurationPath Combine(const ConfigurationPath &path, const String &subkey)
			{
				ConfigurationPath result;
				result._Path = Path::Combine(path._Path, subkey);
				return result;
			}

			bool Valid()
			{
				return !_Path.empty();
			}
		};

		class ConfigurationKey
		{
			String _Path;
			bool _WriteAccess;

		public:
			ConfigurationKey(const ConfigurationPath &path, bool writeAccess = false)
				: _Path(path._Path)
				, _WriteAccess(writeAccess)
			{
				if (writeAccess && !Directory::Exists(_Path))
					Directory::Create(_Path);
			}

			bool Valid()
			{
				return Directory::Exists(_Path);
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

				void operator=(const accessor &acc)
				{
					ASSERT(false);
				}

			private:
				String Read()
				{
					return BazisLib::File::ReadAll<TCHAR>(Path::Combine(_Key._Path, _Name));
				}

				bool Write(const String &val)
				{
					return BazisLib::File::WriteAll(Path::Combine(_Key._Path, _Name), val);
				}

			public:
				operator int()
				{
					String val = Read();
					int result = 0;
#ifdef UNICODE
					swscanf(val.c_str(), L"%d", &result);
#else
					sscanf(val.c_str(), "%d", &result);
#endif
					return result;
				}

				operator String()
				{
					return Read();
				}

				bool operator=(const int &value)
				{
					return Write(stl_itoa(value).c_str());
				}

				bool operator=(const TCHAR *value)
				{
					return Write(value);
				}

				bool operator=(const String &value)
				{
					return Write(value);
				}

			};

		public:
			ConfigurationKey(const accessor &acc)
				: _Path(Path::Combine(acc._Key._Path, acc._Name))
				, _WriteAccess(acc._Key._WriteAccess)
			{
				if (acc._Key._WriteAccess && !Directory::Exists(_Path))
					Directory::Create(_Path);
			}

			accessor operator[](const TCHAR *pKey)
			{
				return accessor(*this, pKey);
			}

			std::vector<String> GetAllSubkeys()
			{
				std::vector<String> result;
				Directory dir = _Path;
				for (Directory::enumerator it(dir.FindFirstFile());it.Valid();it.Next())
				{
					if (!it.IsAPseudoEntry() && it.IsDirectory())
						result.push_back(it.GetRelativePath());
				}
				return result;
			}
		};
	}

	typedef Posix::ConfigurationKey ConfigurationKey;
	typedef Posix::ConfigurationPath ConfigurationPath;
}
