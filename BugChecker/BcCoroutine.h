#pragma once

#include "BugChecker.h"

#include "Cpp20CoroutineFill.h"
#include "KdCom.h"
#include "DbgKd.h"

#include <EASTL/functional.h>

//
// Quick and simple C++ 20 coroutine support classes for BugChecker.
//

struct BcAwaiterBase;

struct BcCoroutine
{
	struct promise_type
	{
		BcCoroutine get_return_object() noexcept { return BcCoroutine(std::coroutine_handle<BcCoroutine::promise_type>::from_promise(*this)); }

		std::suspend_always initial_suspend() noexcept { return {}; }
		std::suspend_never final_suspend() noexcept { return {}; }
		void unhandled_exception() noexcept { *((volatile int*)0) = 0xDEADC0DE; }
		void return_void() noexcept {}

		volatile BcAwaiterBase** awaiterPtrPtr = NULL;
	};

	BcCoroutine(std::coroutine_handle<BcCoroutine::promise_type> h) : coroHandle(h) {}

	void Start(volatile BcAwaiterBase** pp)
	{
		*pp = NULL;
		coroHandle.promise().awaiterPtrPtr = pp;
		coroHandle();
	}

private:

	std::coroutine_handle<BcCoroutine::promise_type> coroHandle = NULL;
};

using BcAwaiterFunction = const CHAR*;

enum class ProcessAwaiterRetVal
{
	Ok,
	Timeout,
	RepeatLoop
};

struct BcAwaiterBase
{
	static void Resume(volatile BcAwaiterBase* pvol)
	{
		BcAwaiterBase* p = const_cast<BcAwaiterBase*>(pvol);

		*p->coroHandle.promise().awaiterPtrPtr = NULL;
		p->coroHandle();
	}

	BcAwaiterFunction function = NULL;

	static ProcessAwaiterRetVal ProcessAwaiter(DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer, PULONG PayloadBytes);

protected:

	std::coroutine_handle<BcCoroutine::promise_type> coroHandle = NULL;
};

template <typename T>
struct BcAwaiterRetVal : BcAwaiterBase
{
	T retVal;

	constexpr bool await_ready() const noexcept { return false; }
	T await_resume() const noexcept { return retVal; }

	void await_suspend(std::coroutine_handle<BcCoroutine::promise_type> h) noexcept
	{
		coroHandle = h;
		*h.promise().awaiterPtrPtr = this;
	}
};

//
// Various awaiters.
//

typedef BOOLEAN NoReturnValue; // =fixfix= with coroutines and templates, there are better ways to accomplish this!

struct BcAwaiter_StateManipulate : BcAwaiterRetVal<BOOLEAN>
{
	BcAwaiter_StateManipulate(
		eastl::function<void(DBGKD_MANIPULATE_STATE64*, PKD_BUFFER)> prepareRequestParam,
		eastl::function<BOOLEAN(DBGKD_MANIPULATE_STATE64*, PKD_BUFFER)> processResponseParam)
		: prepareRequest(eastl::move(prepareRequestParam)),
		processResponse(eastl::move(processResponseParam))
	{
		function = "BcAwaiter_StateManipulate";
		retVal = FALSE;
	}

	eastl::function<void(DBGKD_MANIPULATE_STATE64*, PKD_BUFFER)> prepareRequest;
	eastl::function<BOOLEAN(DBGKD_MANIPULATE_STATE64*, PKD_BUFFER)> processResponse;
};

class ImageDebugInfo;

struct BcAwaiter_DiscoverPosInModules : BcAwaiterRetVal<NoReturnValue>
{
	BcAwaiter_DiscoverPosInModules(CHAR* outputBufferParam, ImageDebugInfo* debugInfoParam, ULONG_PTR pointerParam);

	void PrepareRequest();

	CHAR* outputBuffer;
	ImageDebugInfo* debugInfo;
	BYTE* bytePointer;
};

struct BcAwaiter_RecvTimeout : BcAwaiterRetVal<NoReturnValue>
{
	BcAwaiter_RecvTimeout()
	{
		function = "BcAwaiter_RecvTimeout";
		retVal = TRUE;
	}
};

struct BcAwaiter_ReadMemory : BcAwaiterRetVal<VOID*>
{
	BcAwaiter_ReadMemory(ULONG_PTR pointerParam, size_t sizeParam)
	{
		function = "BcAwaiter_ReadMemory";
		retVal = NULL;

		pointer = pointerParam;
		size = sizeParam;
	}

	void PrepareRequest();

	ULONG_PTR pointer;
	size_t size;
};

struct BcAwaiter_Join : BcAwaiterRetVal<NoReturnValue>
{
	BcAwaiter_Join(BcCoroutine coroutineParam)
		: coroutine(eastl::move(coroutineParam))
	{
		function = "BcAwaiter_Join";
		retVal = TRUE;
	}

	BcCoroutine coroutine;
};
