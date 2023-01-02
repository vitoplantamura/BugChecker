#pragma once
#include "IOObject.h"
#include <memory.h>
#include "../../bzscore/buffer.h"

namespace BazisLib
{
	namespace MacOS
	{
		struct CallArgs
		{
			enum {MaxScalarCount = 10};

			int _ScalarCount;
			uint64_t _Scalars[MaxScalarCount];
			const void *_pStruct;
			size_t _StructSize;

			CallArgs()
			{
				_ScalarCount = 0;
				_pStruct = NULL;
				_StructSize = 0;
			}

			CallArgs(uint64_t arg1)
			{
				_pStruct = NULL;
				_StructSize = 0;
				_ScalarCount = 1;
				_Scalars[0] = arg1;
			}

			CallArgs(uint64_t arg1, uint64_t arg2)
			{
				_pStruct = NULL;
				_StructSize = 0;
				_ScalarCount = 2;
				_Scalars[0] = arg1;
				_Scalars[1] = arg2;
			}

			CallArgs(uint64_t arg1, uint64_t arg2, uint64_t arg3)
			{
				_pStruct = NULL;
				_StructSize = 0;
				_ScalarCount = 3;
				_Scalars[1] = arg1;
				_Scalars[2] = arg2;
				_Scalars[3] = arg3;
			}

			CallArgs(void *pStruct, size_t size)
			{
				_ScalarCount = 0;
				_pStruct = pStruct;
				_StructSize = size;
			}

			template<typename _Struct> CallArgs(_Struct *pStruct)
			{
				_ScalarCount = 0;
				_pStruct = pStruct;
				_StructSize = sizeof(_Struct);
			}

			CallArgs &AttachStruct(const void *pStruct, size_t size)
			{
				_pStruct = pStruct;
				_StructSize = size;
				return *this;
			}

			CallArgs &AttachStruct(BasicBuffer *pBuf)
			{
				_pStruct = pBuf->GetData();
				_StructSize = pBuf->GetSize();
				return *this;
			}

			template<typename _Struct> CallArgs &AttachStruct(const _Struct *pStruct)
			{
				_pStruct = pStruct;
				_StructSize = sizeof(_Struct);
				return *this;
			}
			
		};

		struct CallResult
		{
			kern_return_t ReturnCode;
			uint64_t OutScalars[CallArgs::MaxScalarCount];
			unsigned ScalarOutputCount;
			size_t OutStructureSize;

			CallResult()
			{
				ScalarOutputCount = 0;
				OutStructureSize = 0;
				memset(OutScalars, 0, sizeof(OutScalars));
			}

			bool Successful()
			{
				return ReturnCode == 0;
			}
		};

		class IOConnectWrapper : public _IOObjectWrapperT<io_connect_t>
		{
		protected:
			typedef _IOObjectWrapperT<io_service_t> super;

		public:
			IOConnectWrapper(io_service_t obj)
				: super (obj)
			{
			}

			void Close()
			{
				IOServiceClose(get());
			}


			IOConnectWrapper()
			{
			}

			kern_return_t CallRaw(uint32_t selector,
				const uint64_t	*input,
				uint32_t	 inputCnt,
				const void      *inputStruct,
				size_t		 inputStructCnt,
				uint64_t	*output,
				uint32_t	*outputCnt,
				void		*outputStruct,
				size_t		*outputStructCnt)
			{
				return IOConnectCallMethod(get(), selector, input, inputCnt, inputStruct, inputStructCnt, output, outputCnt, outputStruct, outputStructCnt);
			}

			CallResult Call(uint32_t selector, const CallArgs &inputArgs = CallArgs(), size_t expectedOutputScalars = 0)
			{
				CallResult result;
				result.ScalarOutputCount = expectedOutputScalars;
				result.ReturnCode = CallRaw(selector, inputArgs._Scalars, inputArgs._ScalarCount, inputArgs._pStruct, inputArgs._StructSize, result.OutScalars, &result.ScalarOutputCount, NULL, NULL);
				return result;
			}

			CallResult Call(uint32_t selector, const CallArgs &inputArgs, void *pOutStruct, size_t outStructSize, size_t expectedOutputScalars = 0)
			{
				CallResult result;
				result.ScalarOutputCount = expectedOutputScalars;
				result.OutStructureSize= outStructSize;
				result.ReturnCode = CallRaw(selector, inputArgs._Scalars, inputArgs._ScalarCount, inputArgs._pStruct, inputArgs._StructSize, result.OutScalars, &result.ScalarOutputCount, pOutStruct, &result.OutStructureSize);
				return result;
			}

		};
	}
}