#pragma once
#include "serializer.h"
#include "../bzscore/status.h"
#include "../bzscore/string.h"

//#include "../bzscore/buffer.h"

namespace BazisLib
{
	//Requires the deserialized structure to match serialized one.
	//Enforces it by checking field types, but not names.
	//Used to simplify user <=> kernel data transfer
	class BinarySerializer
	{
	private:
		enum {kHeader = 'HBLB', kFooter = 'FBLB'};

		enum FieldTypeID {kUndefined = 0,
			  kInt1,
			  kInt2,
			  kInt4,
			  kInt8,
			  kUInt1,
			  kUInt2,
			  kUInt4,
			  kUInt8,
			  kBool,
			  //All variable-length field types should have MSB set
			  kUnicodeString = 0x81,
			  kANSIString,
		};

		class BufferSink
		{
		private:
			size_t _Offset, _MaxLength;
			char *_pBuffer;

		public:
			BufferSink(void *pBuffer, size_t maxLen)
				: _pBuffer((char *)pBuffer)
				, _MaxLength(maxLen)
				, _Offset(0)
			{
			}

			ActionStatus Write(const void *pBuffer, size_t size)
			{
				if ((_Offset + size) > _MaxLength)
					return MAKE_STATUS(NoMemory);
				memcpy(_pBuffer + _Offset, pBuffer, size);
				_Offset += size;
				return MAKE_STATUS(Success);
			}

			size_t GetOffset() {return _Offset;}
		};

		template <class _Sink> class SerializeWorker
		{
		private:
			_Sink &Sink;

		private:
			ActionStatus DoWriteFixedEntry(FieldTypeID typeID, const void *pEntry, size_t sizeInBytes)
			{
				ASSERT(!(typeID & 0x80));
				ActionStatus st = Sink.Write(&typeID, 1);
				if (!st.Successful())
					return st;
				st = Sink.Write(pEntry, sizeInBytes);
				if (!st.Successful())
					return st;
				return MAKE_STATUS(Success);
			}

			ActionStatus DoWriteVarLengthEntry(FieldTypeID typeID, const void *pEntry, size_t sizeInBytes)
			{
				ASSERT(typeID & 0x80);
				ActionStatus st = Sink.Write(&typeID, 1);
				if (!st.Successful())
					return st;
				st = Sink.Write(&sizeInBytes, 4);
				if (!st.Successful())
					return st;
				st = Sink.Write(pEntry, sizeInBytes);
				if (!st.Successful())
					return st;
				return MAKE_STATUS(Success);
			}

		private:
			ActionStatus DoSerializeEntry(signed char &entry) { return DoWriteFixedEntry(kInt1, &entry, 1); }
			ActionStatus DoSerializeEntry(signed short &entry) { return DoWriteFixedEntry(kInt2, &entry, 2); }
			ActionStatus DoSerializeEntry(signed int &entry) { return DoWriteFixedEntry(kInt4, &entry, 4); }
			ActionStatus DoSerializeEntry(signed long &entry) { return DoWriteFixedEntry(kInt4, &entry, 4); C_ASSERT(sizeof(entry) == 4); }
			ActionStatus DoSerializeEntry(LONGLONG &entry) { return DoWriteFixedEntry(kInt8, &entry, 8); }

			ActionStatus DoSerializeEntry(unsigned char &entry) { return DoWriteFixedEntry(kUInt1, &entry, 1); }
			ActionStatus DoSerializeEntry(unsigned short &entry) { return DoWriteFixedEntry(kUInt2, &entry, 2); }
			ActionStatus DoSerializeEntry(unsigned int &entry) { return DoWriteFixedEntry(kUInt4, &entry, 4); }
			ActionStatus DoSerializeEntry(unsigned long &entry) { return DoWriteFixedEntry(kUInt4, &entry, 4); C_ASSERT(sizeof(entry) == 4); }
			ActionStatus DoSerializeEntry(ULONGLONG &entry) { return DoWriteFixedEntry(kUInt8, &entry, 8); }

			ActionStatus DoSerializeEntry(bool &entry) { return DoWriteFixedEntry(kBool, &entry, 1); }

			ActionStatus DoSerializeEntry(DynamicStringW &entry)
			{
				return DoWriteVarLengthEntry(kUnicodeString, entry.c_str(), entry.SizeInBytes(false));
			}

			ActionStatus DoSerializeEntry(DynamicStringA &entry)
			{
				return DoWriteVarLengthEntry(kANSIString, entry.c_str(), entry.SizeInBytes(false));
			}

		public:
			SerializeWorker(_Sink &sink)
				: Sink(sink)
			{
			}

			template <class _Type> ActionStatus _SerializerEntry(const TCHAR *fieldName, _Type *ref, bool Save)
			{
				return DoSerializeEntry(*ref);
			}
		};

		template <class _Sink, class _Structure> static ActionStatus DoSerialize(_Structure &value, _Sink &sink)
		{
			int magic = kHeader;
			ActionStatus st = sink.Write(&magic, 4);
			if (!st.Successful())
				return st;

			SerializeWorker<_Sink> worker(sink);
			value.__SERIALIZER_ITERATE_MEMBERS(worker, true);

			magic = kFooter;
			st = sink.Write(&magic, 4);
			if (!st.Successful())
				return st;
			return MAKE_STATUS(Success);
		}

		//---------------------------- Deserialization ----------------------------//

		class BufferSource
		{
		private:
			size_t _Offset, _MaxLength;
			const char *_pBuffer;

		public:
			BufferSource(const void *pBuffer, size_t maxLen)
				: _pBuffer((const char *)pBuffer)
				, _MaxLength(maxLen)
				, _Offset(0)
			{
			}

			ActionStatus Read(void *pBuffer, size_t size)
			{
				if ((_Offset + size) > _MaxLength)
					return MAKE_STATUS(NoMemory);
				memcpy(pBuffer, _pBuffer + _Offset, size);
				_Offset += size;
				return MAKE_STATUS(Success);
			}

			ActionStatus ReadAndCompare(const void *pBuffer, size_t size)
			{
				if ((_Offset + size) > _MaxLength)
					return MAKE_STATUS(NoMemory);
				bool match = !memcmp(pBuffer, _pBuffer + _Offset, size);
				_Offset += size;
				return match ? MAKE_STATUS(Success) : MAKE_STATUS(ParsingFailed);
			}

			size_t GetOffset() {return _Offset;}
		};


		template <class _Source, class _Structure> static ActionStatus DoDeserialize(_Structure &value, _Source &source)
		{
			int magic = kHeader;
			ActionStatus st = source.ReadAndCompare(&magic, 4);
			if (!st.Successful())
				return st;

			DeserializeWorker<_Source> worker(source);
			value.__SERIALIZER_ITERATE_MEMBERS(worker, false);

			magic = kFooter;
			st = source.ReadAndCompare(&magic, 4);
			if (!st.Successful())
				return st;
			return MAKE_STATUS(Success);
		}

		template <class _Source> class DeserializeWorker
		{
		private:
			_Source &Source;

		private:
			ActionStatus DoReadFixedEntry(FieldTypeID typeID, void *pEntry, size_t sizeInBytes)
			{
				ASSERT(!(typeID & 0x80));
				ActionStatus st = Source.ReadAndCompare(&typeID, 1);
				if (!st.Successful())
					return st;
				st = Source.Read(pEntry, sizeInBytes);
				if (!st.Successful())
					return st;
				return MAKE_STATUS(Success);
			}

			ActionStatus DoVerifyTypeAndReadLength(FieldTypeID typeID, size_t *pSize)
			{
				ActionStatus st = Source.ReadAndCompare(&typeID, 1);
				if (!st.Successful())
					return st;
				return Source.Read(pSize, 4);
			}

		private:
			ActionStatus DoDeserializeEntry(signed char &entry) { return DoReadFixedEntry(kInt1, &entry, 1); }
			ActionStatus DoDeserializeEntry(signed short &entry) { return DoReadFixedEntry(kInt2, &entry, 2); }
			ActionStatus DoDeserializeEntry(signed int &entry) { return DoReadFixedEntry(kInt4, &entry, 4); }
			ActionStatus DoDeserializeEntry(signed long &entry) { return DoReadFixedEntry(kInt4, &entry, 4); C_ASSERT(sizeof(entry) == 4); }
			ActionStatus DoDeserializeEntry(LONGLONG &entry) { return DoReadFixedEntry(kInt8, &entry, 8); }

			ActionStatus DoDeserializeEntry(unsigned char &entry) { return DoReadFixedEntry(kUInt1, &entry, 1); }
			ActionStatus DoDeserializeEntry(unsigned short &entry) { return DoReadFixedEntry(kUInt2, &entry, 2); }
			ActionStatus DoDeserializeEntry(unsigned int &entry) { return DoReadFixedEntry(kUInt4, &entry, 4); }
			ActionStatus DoDeserializeEntry(unsigned long &entry) { return DoReadFixedEntry(kUInt4, &entry, 4); C_ASSERT(sizeof(entry) == 4); }
			ActionStatus DoDeserializeEntry(ULONGLONG &entry) { return DoReadFixedEntry(kUInt8, &entry, 8); }

			ActionStatus DoDeserializeEntry(bool &entry) { return DoReadFixedEntry(kBool, &entry, 1); }

			template <class _CharType> ActionStatus DoDeserializeString(_DynamicStringT<_CharType> &entry, FieldTypeID typeID)
			{
				size_t size;
				ActionStatus st = DoVerifyTypeAndReadLength(typeID, &size);
				if (!st.Successful())
					return st;
				st = Source.Read(entry.PreAllocate((size + sizeof(_CharType) - 1) / sizeof(_CharType), false), size);
				if (!st.Successful())
					return st;
				entry.SetLength((size + sizeof(_CharType) - 1) / sizeof(_CharType));
				return MAKE_STATUS(Success);
			}

			ActionStatus DoDeserializeEntry(DynamicStringW &entry)
			{
				return DoDeserializeString<wchar_t>(entry, kUnicodeString);
			}

			ActionStatus DoDeserializeEntry(DynamicStringA &entry)
			{
				return DoDeserializeString<char>(entry, kANSIString);
			}

		public:
			DeserializeWorker(_Source &source)
				: Source(source)
			{
			}

			template <class _Type> ActionStatus _SerializerEntry(const TCHAR *fieldName, _Type *ref, bool Save)
			{
				return DoDeserializeEntry(*ref);
			}
		};


	public:
		template <class _Structure> static ActionStatus SerializeToBuffer(_Structure &value, void *pBuffer, size_t maxLength, size_t *pTotalLength)
		{
			if (pTotalLength)
				*pTotalLength = 0;
			BufferSink sink(pBuffer, maxLength);
			ActionStatus st =  DoSerialize(value, sink);
			if (!st.Successful())
				return st;
			if (pTotalLength)
				*pTotalLength = sink.GetOffset();
			return MAKE_STATUS(Success);
		}
			
		template <class _Structure> static ActionStatus DeserializeFromBuffer(_Structure &value, const void *pBuffer, size_t length, size_t *pTotalLength)
		{
			if (pTotalLength)
				*pTotalLength = 0;
			BufferSource source(pBuffer, length);
			ActionStatus st =  DoDeserialize(value, source);
			if (!st.Successful())
				return st;
			if (pTotalLength)
				*pTotalLength = source.GetOffset();
			return MAKE_STATUS(Success);
		}
	};
}