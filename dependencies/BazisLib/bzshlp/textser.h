#pragma once
#include "cmndef.h"
#include "../bzscore/autofile.h"
#include "../bzscore/status.h"
#include "../bzscore/objmgr.h"
#include "textfile.h"
#include <map>

namespace BazisLib
{
	//! Provides simple way for saving serialized objects to text files
	/*! The text serializer allows saving serializeable structures to
	text files in a completely human-readable format.
	*/
	class TextFileSerializer
	{
	public:
		template <class _Structure> static ActionStatus Serialize(_MP(BazisLib::AIFile) pFile, _Structure &value)
		{
			SerializeWorker worker(pFile);
			ActionStatus status = worker.BeginSerialization(value);
			if (!status.Successful()) return status;
			status = value.__SERIALIZER_ITERATE_MEMBERS(worker, true);
			if (!status.Successful()) return status;
			status = worker.EndSerialization(value);
			if (!status.Successful()) return status;

			return MAKE_STATUS(Success);
		}

		template <class _Structure> static ActionStatus Deserialize(_MP(BazisLib::TextUnicodeFileReader) pFile, _Structure &value)
		{
			DeserializeWorker worker(pFile);
			ActionStatus status = worker.BeginDeserialization(value);
			if (!status.Successful()) return status;
			status = value.__SERIALIZER_ITERATE_MEMBERS(worker, false);
			if (!status.Successful()) return status;
			status = worker.EndDeserialization(value);
			if (!status.Successful()) return status;

			return MAKE_STATUS(Success);		
		}

	private:
		class SerializeWorker
		{
		private:
			ManagedPointer<AIFile> m_pFile;

		public:
			SerializeWorker(const ManagedPointer<AIFile> &pFile)
				: m_pFile(pFile)
			{
				ASSERT(m_pFile);
			}

#pragma region Internally used serializer-related methods

		public:
			template <class _Ty> ActionStatus _SaveField(const wchar_t *ptszValue, _Ty *ref)
			{
				ActionStatus status;
				m_pFile->Write(ptszValue, wcslen(ptszValue) * sizeof(TCHAR), &status);
				if (!status.Successful())
					return status;
				m_pFile->Write(L": ", 2 * sizeof(wchar_t), &status);
				if (!status.Successful())
					return status;
				wchar_t wsz[128];
				int cnt = _snwprintf(wsz, sizeof(wsz)/sizeof(wsz[0]), Serializer::GetPrintfFormatForType(ref), *ref);
				m_pFile->Write(wsz, cnt * sizeof(wchar_t), &status);
				if (!status.Successful())
					return status;
				m_pFile->Write(L"\r\n", 2 * sizeof(wchar_t), &status);
				if (!status.Successful())
					return status;
				return MAKE_STATUS(Success);
			}

			template<> ActionStatus _SaveField(const wchar_t *ptszValue, String *ref)
			{
				//Multi-line strings cannot be saved using this simple serializer
				ASSERT(ref);
				ASSERT(ref->find_first_of(_T("\r\n")) == String::npos);
				ActionStatus status;
				m_pFile->Write(ptszValue, wcslen(ptszValue) * sizeof(TCHAR), &status);
				if (!status.Successful())
					return status;
				m_pFile->Write(L": ", 2 * sizeof(wchar_t), &status);
				if (!status.Successful())
					return status;
#ifdef UNICODE
				m_pFile->Write(ref->c_str(), ref->size() * sizeof(wchar_t), &status);
#else
#pragma error Unicode serializer does not support non-unicode builds
#endif
				if (!status.Successful())
					return status;
				m_pFile->Write(L"\r\n", 2 * sizeof(wchar_t), &status);
				if (!status.Successful())
					return status;
				return MAKE_STATUS(Success);
			}

			template<> ActionStatus _SaveField(const wchar_t *ptszValue, bool *ref)
			{
				int val = *ref;
				return _SaveField(ptszValue, &val);
			}

			template <class _Type> ActionStatus _SerializerEntry(const wchar_t *ptszValue, _Type *ref, bool Save)
			{
				if (Save)
					return _SaveField(ptszValue, ref);
				else
					return MAKE_STATUS(NotSupported);
			}

		public:
			template <class _Ty> ActionStatus BeginSerialization(_Ty &ref)
			{
				m_pFile->Write(L"[", sizeof(wchar_t));
				m_pFile->Write(Serializer::GetClassName(ref), wcslen(Serializer::GetClassName(ref)) * sizeof(wchar_t));
				m_pFile->Write(L"]\r\n", 3 * sizeof(wchar_t));
				return MAKE_STATUS(Success);
			}

			template <class _Ty> ActionStatus EndSerialization(_Ty &ref)
			{
				m_pFile->Write(L"[/", 2 * sizeof(wchar_t));
				m_pFile->Write(Serializer::GetClassName(ref), wcslen(Serializer::GetClassName(ref)) * sizeof(wchar_t));
				m_pFile->Write(L"]\r\n", 3 * sizeof(wchar_t));
				return MAKE_STATUS(Success);
			}
#pragma endregion
		};

		class DeserializeWorker
		{
		private:
			ManagedPointer<TextUnicodeFileReader> m_pReader;
			std::map<String, String> m_ValueMap;

		public:
			DeserializeWorker(const ConstManagedPointer<TextUnicodeFileReader> &pReader)
				: m_pReader(pReader)
			{
				ASSERT(m_pReader);
			}

#pragma region Internally used serializer-related methods

		public:
			template <class _Ty> ActionStatus _LoadField(const wchar_t *ptszValue, _Ty *ref)
			{
				ASSERT(ref);
				std::map<String, String>::iterator it = m_ValueMap.find(ptszValue);
				if (it == m_ValueMap.end())
					return MAKE_STATUS(FileNotFound);
				swscanf(it->second.c_str(), Serializer::GetPrintfFormatForType(ref), ref);
				return MAKE_STATUS(Success);
			}

			template<> ActionStatus _LoadField(const wchar_t *ptszValue, String *ref)
			{
				ASSERT(ref);
				std::map<String, String>::iterator it = m_ValueMap.find(ptszValue);
				if (it == m_ValueMap.end())
					return MAKE_STATUS(FileNotFound);
				*ref = it->second;
				return MAKE_STATUS(Success);
			}

			template<> ActionStatus _LoadField(const wchar_t *ptszValue, bool *ref)
			{
				int val = 0;
				ActionStatus st =_LoadField(ptszValue, &val);
				if (!st.Successful())
					return MAKE_STATUS(FileNotFound);
				*ref = (val != 0);
				return MAKE_STATUS(Success);
			}

			template <class _Type> ActionStatus _SerializerEntry(const wchar_t *ptszValue, _Type *ref, bool Save)
			{
				if (Save)
					return MAKE_STATUS(NotSupported);
				else
					return _LoadField(ptszValue, ref);
			}

		public:
			template <class _Ty> ActionStatus BeginDeserialization(_Ty &ref)
			{
				m_ValueMap.clear();
				String str = m_pReader->ReadLine();
				if ((str.length() < 1) || (str[0] != '['))
					return MAKE_STATUS(ParsingFailed);
				if (str.substr(1, str.length() - 2) != Serializer::GetClassName(ref))
					return MAKE_STATUS(ParsingFailed);
				while (!m_pReader->IsEOF())
				{
					str = m_pReader->ReadLine();
					if ((str.length() < 1) || (str[0] == ';') || (str[0] == '#'))
						continue;
					if (str[0] == '[')
					{
						if ((str.length() < 4) || (str[1] != '/'))
							return MAKE_STATUS(ParsingFailed);
						break;
					}
					SplitStr spl(str, _T(": "));
					if (!spl.valid)
						continue;
					m_ValueMap[spl.left] = spl.right;
				}
				return MAKE_STATUS(Success);
			}

			template <class _Ty> ActionStatus EndDeserialization(_Ty &ref)
			{
				m_ValueMap.clear();
				return MAKE_STATUS(Success);
			}
#pragma endregion

		};
	};
}