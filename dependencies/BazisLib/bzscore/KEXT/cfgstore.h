#pragma once
#include "../string.h"
#include "../file.h"
#include "../buffer.h"

#include <libkern/c++/OSSerialize.h>
#include <libkern/c++/OSUnserialize.h>
#include <libkern/c++/OSDictionary.h>
#include <libkern/c++/OSString.h>
#include <libkern/c++/OSCollectionIterator.h>

namespace BazisLib
{
	namespace KEXT
	{
		class ConfigurationPath
		{
		private:
			String _PListFile;
			String _Subpath;
			

		private:
			friend class ConfigurationKey;

		public:
			static ConfigurationPath FromFSPath(const String &path)
			{
				ConfigurationPath result;
				result._PListFile = path;
				return result;
			}

			String ToString() const
			{
				String result = _PListFile;
				if (!_Subpath.empty())
				{
					result.append("|");
					result.append(_Subpath);
				}
				return result;
			}

			static ConfigurationPath Combine(const ConfigurationPath &path, const String &subkey)
			{
				ConfigurationPath result;
				result._PListFile = path._PListFile;
				if (path._Subpath.empty())
					result._Subpath = subkey;
				else
					result._Subpath = Path::Combine(path._Subpath, subkey);
				return result;
			}

			bool Valid() const
			{
				return !_PListFile.empty();
			}

			const String &GetSubpathInsidePList() const
			{
				return _Subpath;
			}
		};

		class ConfigurationKey
		{
			String _Path;
			bool _WriteAccess;

			OSDictionary *m_pRootDictToSave;
			OSDictionary *m_pDict;

		public:
			ConfigurationKey(const ConfigurationPath &path, bool writeAccess = false)
				: _Path(path._PListFile)
				, _WriteAccess(writeAccess)
				, m_pRootDictToSave(NULL)
				, m_pDict(NULL)
			{
				File f(path._PListFile, FileModes::OpenReadOnly);
				if (f.Valid())
				{
					LONGLONG size = f.GetSize();
					if (size && (size < 32 * 1024 * 1024))
					{
						CBuffer buf(size + 1);
						if (f.Read(buf.GetData(), size) == size)
						{
							((char *)buf.GetData())[size] = 0;
							OSObject *pObj = OSUnserializeXML((char *)buf.GetData());
							if (pObj)
								m_pRootDictToSave = OSDynamicCast(OSDictionary, pObj);
						}
					}
				}
				else if (writeAccess)
				{
					CreateNew(path._PListFile);
				}
				
				if (!m_pRootDictToSave)
					return;

				m_pDict = m_pRootDictToSave;
				if (!path._Subpath.empty())
				{
					size_t start = 0, next;
					for (;start != -1;start = next)
					{
						next = path._Subpath.find('/', start);
						size_t len = -1;

						if (next != -1)
							len = next++ - start;

						TempString component = path._Subpath.substr(start, len);
						OSObject *pObj = m_pDict->getObject(String(component).c_str());
						if (!pObj)
						{
							m_pDict = NULL;
							return;
						}
						m_pDict = OSDynamicCast(OSDictionary, pObj);
						if (!m_pDict)
							return;
					}
				}

				m_pDict->retain();
			}

			void CreateNew(const String &plistFile)
			{
				if (m_pRootDictToSave)
					m_pRootDictToSave->release();
				if (m_pDict)
					m_pDict->release();

				_Path = plistFile;
				_WriteAccess = true;
				m_pRootDictToSave = OSDictionary::withCapacity(0);
				m_pDict = m_pRootDictToSave;
				m_pDict->retain();
			}

			bool SubkeyExists(const char *pName)
			{
				if (!m_pDict)
					return false;
				return m_pDict->getObject(pName) != NULL;
			}

			void DeleteSubKey(const char *pKey)
			{
				if (!m_pDict)
					return;
				m_pDict->removeObject(pKey);
			}

			ActionStatus Save()
			{
				if (!m_pRootDictToSave || _Path.empty())
					return MAKE_STATUS(NotSupported);
				ActionStatus st;
				File f(_Path, FileModes::CreateOrTruncateRW, &st);
				if (!f.Valid())
					return st;

				OSSerialize *pSer = OSSerialize::withCapacity(0);
				if (!pSer)
					return MAKE_STATUS(NoMemory);

				if (!m_pRootDictToSave->serialize(pSer))
					return MAKE_STATUS(NotSupported);

				unsigned size = pSer->getLength();
				f.Write(pSer->text(), size - 1, &st);

				pSer->release();
				return st;
			}

			~ConfigurationKey()
			{
				if (m_pRootDictToSave)
					m_pRootDictToSave->release();
				if (m_pDict)
					m_pDict->release();
			}

			bool Valid()
			{
				return m_pDict != NULL;
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
					if (!_Key.m_pDict)
						return String();
					OSObject *pObj = _Key.m_pDict->getObject(_Name);
					if (!pObj)
						return String();

					OSString *pStr = OSDynamicCast(OSString, pObj);
					String result;
					if (pStr)
						result = pStr->getCStringNoCopy();
					return result;
				}

				bool Write(const String &val)
				{
					if (!_Key.m_pDict || !_Key._WriteAccess)
						return false;

					OSString *pStr = OSString::withCString(val.c_str());
					bool result = _Key.m_pDict->setObject(_Name, pStr);
					pStr->release();
					return result;
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
				: _WriteAccess(acc._Key._WriteAccess)
				, m_pDict(NULL)
				, m_pRootDictToSave(NULL)
			{
				if (acc._Key.m_pDict)
				{
					OSObject *pObj = acc._Key.m_pDict->getObject(acc._Name);
					if (pObj)
					{
						m_pDict = OSDynamicCast(OSDictionary, pObj);
						if (m_pDict)
							m_pDict->retain();
					}
					else
					{
						m_pDict = OSDictionary::withCapacity(1);
						acc._Key.m_pDict->setObject(acc._Name, m_pDict);
					}
				}
			}

			accessor operator[](const TCHAR *pKey)
			{
				return accessor(*this, pKey);
			}

			std::vector<String> GetAllSubkeys()
			{
				std::vector<String> result;
				if (!m_pDict)
					return result;
				int cnt = m_pDict->getCount();
				result.reserve(cnt);
				OSCollectionIterator *pIter = OSCollectionIterator::withCollection(m_pDict);
				while(pIter->isValid())
				{
					OSObject *pObj = pIter->getNextObject();
					if (!pObj)
						break;
					OSString *pStr = OSDynamicCast(OSString, pObj);
					if (!pStr)
						continue;

					pObj = m_pDict->getObject(pStr);
					if (pObj && OSDynamicCast(OSDictionary, pObj))
						result.push_back(pStr->getCStringNoCopy());
				}
				pIter->release();
				return result;
			}
		};
	}

	typedef KEXT::ConfigurationKey ConfigurationKey;
	typedef KEXT::ConfigurationPath ConfigurationPath;
}
