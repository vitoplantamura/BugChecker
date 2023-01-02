#pragma once
#include "../bzscore/string.h"
#include "../bzscore/buffer.h"

namespace BazisLib
{
	namespace Network
	{
		class Base64
		{
		public:
			//Maps [aaa] to [bbbb] according to BASE64 alphabet. [aa] will be mapped to [bbb=], [a] - to [bb==]
			static void Encode(const void *pPtr, unsigned Size, DynamicStringA &buffer);
			static void Encode(const void *pPtr, unsigned Size, DynamicStringW &buffer);

			static bool Decode(const _TempStringImplT<char> &string, BasicBuffer &buffer);
			static bool Decode(const _TempStringImplT<wchar_t> &string, BasicBuffer &buffer);
		public:

			static std::string EncodeANSIString(const std::string &str)
			{
				DynamicStringA result;
				Encode(str.c_str(), str.size(), result);
				return result.c_str();
			}

		};
	}
}