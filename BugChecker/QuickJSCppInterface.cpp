#include "BugChecker.h"

#include "Root.h"
#include "Glyph.h"
#include "CrtFill.h"
#include "Utils.h"
#include "X86Step.h"

#include <stdarg.h>

#include "QuickJSCInterface.h"
#include "QuickJSCppInterface.h"

extern "C" void __stdcall _bc_resetstdout() // special function called by "QuickJSCInterface.c" and declared in "QuickJSCppInterface.h".
{
	Root::I->LogWindow.AddString("", FALSE, FALSE); // remove last line if empty + DON'T refresh ui.
}

extern "C" int __stdcall _bc_printf(__in_z __format_string const char* _Format, ...); // forward declaration
extern "C" int __stdcall _bc_isfinite(double _X); // defined in "QuickJSCInterface.c".

class SaveFPUState
{
public:
	SaveFPUState() // SSE on Windows x64 should not require saving/restoring SSE state.
	{
#ifndef _AMD64_

		__asm
		{
			mov eax, cr0
			mov cr0Reg, eax

			clts

			fnsave fpuState
		}

#endif
	}

	~SaveFPUState()
	{
#ifndef _AMD64_

		__asm
		{
			frstor fpuState

			mov eax, cr0Reg
			mov cr0, eax
		}

#endif
	}

private:

	static BYTE fpuState[128]; // 108 is the exact size
	static DWORD cr0Reg;
};

BYTE SaveFPUState::fpuState[128];
DWORD SaveFPUState::cr0Reg;

//
// Expanded stack wrappers:
//

extern "C" ULONG_PTR esOld = 0, esNew = 0, esFunction = 0, esParam = 0;

#ifdef _AMD64_
extern "C" VOID ESCall(VOID);
#else
VOID __declspec(naked) ESCall(VOID)
{
	__asm
	{
		mov dword ptr [esOld], esp
		mov esp, dword ptr [esNew]
		call dword ptr [esFunction]
		mov esp, dword ptr [esOld]
		ret
	}
}
#endif

#define ES_CALL(fn, ps) \
	esFunction = (ULONG_PTR)& fn ; \
	esParam = (ULONG_PTR) ps ; \
	esNew = (ULONG_PTR)Root::I->ExpandedStack + sizeof(Root::I->ExpandedStack) - 0x100; \
	esNew -= esNew % 0x10; \
	::ESCall();

// -- QJS_Eval_ES --

class QJS_Eval_ES_Params
{
public:
	const char* src;
	char* asyncArgs;
	int asyncArgsMaxSize;
	unsigned __int64* retValU64;
	BOOL quiet;

	int retVal;
};

static VOID QJS_Eval_ES_Execute()
{
	QJS_Eval_ES_Params* params = (QJS_Eval_ES_Params*)esParam;
	params->retVal = ::QJS_Eval(params->src, params->asyncArgs, params->asyncArgsMaxSize, params->retValU64, params->quiet);
}

static int QJS_Eval_ES(const char* src, char* asyncArgs, int asyncArgsMaxSize, unsigned __int64* retValU64, BOOL quiet)
{
	QJS_Eval_ES_Params* params = new QJS_Eval_ES_Params{ src, asyncArgs, asyncArgsMaxSize, retValU64, quiet, 0 };

	ES_CALL(QJS_Eval_ES_Execute, params);

	auto ret = params->retVal;
	delete params;
	return ret;
}

// -- QJS_SetGlobalStringProp_ES --

class QJS_SetGlobalStringProp_ES_Params
{
public:
	const char* name;
	const char* value;
};

static VOID QJS_SetGlobalStringProp_ES_Execute()
{
	QJS_SetGlobalStringProp_ES_Params* params = (QJS_SetGlobalStringProp_ES_Params*)esParam;
	::QJS_SetGlobalStringProp(params->name, params->value);
}

static void QJS_SetGlobalStringProp_ES(const char* name, const char* value)
{
	QJS_SetGlobalStringProp_ES_Params* params = new QJS_SetGlobalStringProp_ES_Params{ name, value };

	ES_CALL(QJS_SetGlobalStringProp_ES_Execute, params);

	delete params;
}

//
// Implementation of the QuickJSCppInterface class:
//

extern "C" const char* QJSStartupScript = R"bcX(

function QJSPrint( valueToPrint )
{
    var hex_mode = true;
    var eval_mode = "std";
	var has_jscalc = false;

	var stringToReturn = "";

	var std = {

		puts : function ()
		{
			for (var i = 0; i < arguments.length; i++)
				stringToReturn += arguments[i];
		}

	};

	// ====================================================
	// (from QuickJS sources, file: repl.js) START >>>
	// ====================================================

    function number_to_string(a, radix) {
        var s;
        if (!isFinite(a)) {
            /* NaN, Infinite */
            return a.toString();
        } else {
            if (a == 0) {
                if (1 / a < 0)
                    s = "-0";
                else
                    s = "0";
            } else {
                if (radix == 16 && a === Math.floor(a)) {
                    var s;
                    if (a < 0) {
                        a = -a;
                        s = "-";
                    } else {
                        s = "";
                    }
                    s += "0x" + a.toString(16);
                } else {
                    s = a.toString();
                }
            }
            return s;
        }
    }

    function bigfloat_to_string(a, radix) {
        var s;
        if (!BigFloat.isFinite(a)) {
            /* NaN, Infinite */
            if (eval_mode !== "math") {
                return "BigFloat(" + a.toString() + ")";
            } else {
                return a.toString();
            }
        } else {
            if (a == 0) {
                if (1 / a < 0)
                    s = "-0";
                else
                    s = "0";
            } else {
                if (radix == 16) {
                    var s;
                    if (a < 0) {
                        a = -a;
                        s = "-";
                    } else {
                        s = "";
                    }
                    s += "0x" + a.toString(16);
                } else {
                    s = a.toString();
                }
            }
            if (typeof a === "bigfloat" && eval_mode !== "math") {
                s += "l";
            } else if (eval_mode !== "std" && s.indexOf(".") < 0 &&
                ((radix == 16 && s.indexOf("p") < 0) ||
                 (radix == 10 && s.indexOf("e") < 0))) {
                /* add a decimal point so that the floating point type
                   is visible */
                s += ".0";
            }
            return s;
        }
    }

    function bigint_to_string(a, radix) {
        var s;
        if (radix == 16) {
            var s;
            if (a < 0) {
                a = -a;
                s = "-";
            } else {
                s = "";
            }
            s += "0x" + a.toString(16);
        } else {
            s = a.toString();
        }
        if (eval_mode === "std")
            s += "n";
        return s;
    }
    
    function print(a) {
        var stack = [];

        function print_rec(a) {
            var n, i, keys, key, type, s;
            
            type = typeof(a);
            if (type === "object") {
                if (a === null) {
                    std.puts(a);
                } else if (stack.indexOf(a) >= 0) {
                    std.puts("[circular]");
                } else if (has_jscalc && (a instanceof Fraction ||
                                        a instanceof Complex ||
                                        a instanceof Mod ||
                                        a instanceof Polynomial ||
                                        a instanceof PolyMod ||
                                        a instanceof RationalFunction ||
                                        a instanceof Series)) {
                    std.puts(a.toString());
                } else {
                    stack.push(a);
                    if (Array.isArray(a)) {
                        n = a.length;
                        std.puts("[ ");
                        for(i = 0; i < n; i++) {
                            if (i !== 0)
                                std.puts(", ");
                            if (i in a) {
                                print_rec(a[i]);
                            } else {
                                std.puts("<empty>");
                            }
                            if (i > 20) {
                                std.puts("...");
                                break;
                            }
                        }
                        std.puts(" ]");
                    } else if (Object.__getClass(a) === "RegExp") {
                        std.puts(a.toString());
                    } else {
                        keys = Object.keys(a);
                        n = keys.length;
                        std.puts("{ ");
                        for(i = 0; i < n; i++) {
                            if (i !== 0)
                                std.puts(", ");
                            key = keys[i];
                            std.puts(key, ": ");
                            print_rec(a[key]);
                        }
                        std.puts(" }");
                    }
                    stack.pop(a);
                }
            } else if (type === "string") {
                s = a.__quote();
                if (s.length > 79)
                    s = s.substring(0, 75) + "...\"";
                std.puts(s);
            } else if (type === "number") {
                std.puts(number_to_string(a, hex_mode ? 16 : 10));
            } else if (type === "bigint") {
                std.puts(bigint_to_string(a, hex_mode ? 16 : 10));
            } else if (type === "bigfloat") {
                std.puts(bigfloat_to_string(a, hex_mode ? 16 : 10));
            } else if (type === "bigdecimal") {
                std.puts(a.toString() + "m");
            } else if (type === "symbol") {
                std.puts(String(a));
            } else if (type === "function") {
                std.puts("function " + a.name + "()");
            } else {
                std.puts(a);
            }
        }
        print_rec(a);
    }

	// ====================================================
	// <<< END (from QuickJS sources, file: repl.js)
	// ====================================================

	hex_mode = typeof( valueToPrint ) !== "bigfloat";

	print( valueToPrint );

	return stringToReturn;
}

function CheckReturnValue ()
{
	var a = _bc_eval_.ret;

	_bc_eval_.retStr = QJSPrint( a );

	if ( typeof(a) == "undefined" || a == null )
	{
		_bc_eval_.retU64 = "0";
	}
	else if ( typeof(a) == "boolean" )
	{
		_bc_eval_.retU64 = a ? "1" : "0";
	}
	else if ( typeof(a) == "number" )
	{
		if ( a === parseInt(a) && a <= 0x20000000000000 && a >= 0 )
			_bc_eval_.retU64 = a.toString();
	}
	else if ( typeof(a) == "bigint" )
	{
		if ( a <= 0xFFFFFFFFFFFFFFFFn && a >= 0 )
			_bc_eval_.retU64 = a.toString();
	}
}

function ThrowEx(e)
{
	_bc_eval_.ret = "Throw: " + e;
	throw e;
}

function CheckIntArg(a, n)
{
	if ( typeof(a) == "number" )
	{
		if ( a !== parseInt(a) )
			ThrowEx( "Argument '" + n + "' must be an integer." );
		if ( a > 0x20000000000000 )
			ThrowEx( "Argument '" + n + "' is too big, it must be a BigInt (0x1234n instead of 0x1234)." );
	}
	else if ( typeof(a) == "bigint" )
	{
		if ( a > 0xFFFFFFFFFFFFFFFFn )
			ThrowEx( "Argument '" + n + "' is too big." );
	}
	else
		ThrowEx( "Argument '" + n + "' must be an integer." );

	if ( a < 0 )
		ThrowEx( "Argument '" + n + "' must be positive." );
}

async function ReadMem(address, size, dataSize)
{
	if ( dataSize === undefined )
		dataSize = 1;

	CheckIntArg(address, "address");
	CheckIntArg(size, "size");
	CheckIntArg(dataSize, "dataSize");

	if ( ! size )
		ThrowEx( "Argument 'size' is 0." );

	if ( dataSize != 1 && dataSize != 2 && dataSize != 4 && dataSize != 8 )
		ThrowEx( "Argument 'dataSize' must be 1, 2, 4 or 8." );

	if ( dataSize * size > 2 * 1024 )
		ThrowEx( "Argument 'size' is too big." );

	var ret = await new Promise( function(resolve, reject) {
		_bc_eval_.resolve = resolve;
		_bc_eval_.reject = reject;
		_bc_eval_.async_args = "ReadMem" + "$delim$" + address + "$delim$" + size + "$delim$" + dataSize;
	});

	if ( ! Array.isArray( ret ) || ! ret.length )
		ThrowEx( "Unable to read memory." );

	return ret;
}

async function WriteMem(address, dataSize, contents)
{
	CheckIntArg(address, "address");
	CheckIntArg(dataSize, "dataSize");

	if ( dataSize != 1 && dataSize != 2 && dataSize != 4 && dataSize != 8 )
		ThrowEx( "Argument 'dataSize' must be 1, 2, 4 or 8." );

	if ( ! Array.isArray( contents ) )
		ThrowEx( "Argument 'contents' must be an array." );

	if ( ! contents.length )
		ThrowEx( "Argument 'contents' is empty." );

	if ( dataSize * contents.length > 2 * 1024 )
		ThrowEx( "Argument 'contents' is too big." );

	var c = "";
	for ( var i = 0; i < contents.length; i ++ )
	{
		CheckIntArg(contents[i], "item #" + i + " in contents");
		c += contents[i] + ( i == contents.length - 1 ? "" : "$delim$" );
	}

	var ret = await new Promise( function(resolve, reject) {
		_bc_eval_.resolve = resolve;
		_bc_eval_.reject = reject;
		_bc_eval_.async_args = "WriteMem" + "$delim$" + address + "$delim$" + dataSize + "$delim$" + c;
	});

	if ( ret !== true )
		ThrowEx( "Unable to write memory." );

	return true;
}

async function WriteReg(regName, value)
{
	CheckIntArg(value, "value");

	if ( typeof( regName ) != "string" )
		ThrowEx( "Argument 'regName' must be a string." );

	regName = regName.toUpperCase();

	var validRegs = [
		"RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "RSP",
		"RBP", "R8", "R9", "R10", "R11", "R12", "R13",
		"R14", "R15", "RIP", "EAX", "EBX", "ECX", "EDX",
		"ESI", "EDI", "ESP", "EBP", "EIP"];

	var found = false;
	for ( var i = 0; i < validRegs.length ; i ++ )
		if ( validRegs[i] == regName )
		{
			found = true;
			break;
		}

	if ( ! found )
		ThrowEx( "Argument 'regName' must be a valid register name." );

	var ret = await new Promise( function(resolve, reject) {
		_bc_eval_.resolve = resolve;
		_bc_eval_.reject = reject;
		_bc_eval_.async_args = "WriteReg" + "$delim$" + regName + "$delim$" + value;
	});

	if ( ret !== true )
		ThrowEx( "Unable to write register." );

	return true;
}

)bcX";

BcCoroutine QuickJSCppInterface::Eval(const CHAR* psz, BYTE* context, ULONG contextLen, BOOLEAN is32bitCompat, NullableU64* pRetVal /*= NULL*/, BOOLEAN quiet /*= FALSE*/, BOOLEAN wrapInAsyncFn /*= TRUE*/) noexcept
{
	SaveFPUState __fpu_state__;

	ULONG64 qjsRetU64 = 0;
	int qjsRet = 0;

	auto CheckRet = [&]() {

		if (pRetVal && (qjsRet & QJS_EVAL_SUCCESS_RETVALU64))
		{
			pRetVal->value = qjsRetU64;
			pRetVal->isNull = FALSE;
		}
	};

	auto AddReg = [&context](const CHAR* regName, eastl::string& str) {

		ZydisRegister reg = X86Step::StringToZydisReg(regName);

		if (reg != ZYDIS_REGISTER_NONE)
		{
			size_t sz = 0;
			VOID* preg = X86Step::ZydisRegToCtxRegValuePtr((CONTEXT*)context, reg, &sz);

			if (preg && (sz == 4 || sz == 8))
			{
				eastl::string name = regName;

				for (int i = 0; i < 2; i++)
				{
					if (i == 1) Utils::ToLower(name);

					str += name + " = 0x" + Utils::HexToString(sz == 4 ? *(ULONG32*)preg : *(ULONG64*)preg, sz) + "n; ";
				}
			}
		}
	};

	auto AddRegs = [&AddReg](eastl::string& str) {

		const CHAR* regs[] = {
			"RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "RSP",
			"RBP", "R8", "R9", "R10", "R11", "R12", "R13",
			"R14", "R15", "RIP", "EAX", "EBX", "ECX", "EDX",
			"ESI", "EDI", "ESP", "EBP", "EIP" };

		for (const CHAR* reg : regs)
			AddReg(reg, str);
	};

	// update the QJS context.

	eastl::string c = "_bc_eval_ = void 0; ";

	c += eastl::string("is32bitCompat = ") + (is32bitCompat ? "true" : "false") + "; ";

	AddRegs(c);

	eastl::vector<CHAR> asyncArgs(512, 0);

	::QJS_Eval_ES(c.c_str(), asyncArgs.data(), asyncArgs.size(), &qjsRetU64, TRUE);

	// call QuickJS.

	auto s = eastl::string(psz);

	s.trim();
	s.rtrim(";");

	::memset(asyncArgs.data(), 0, asyncArgs.size());

	if (!wrapInAsyncFn)
		::QJS_SetGlobalStringProp_ES("_bc_script_", s.c_str());

	qjsRet = ::QJS_Eval_ES((
		"_bc_eval_ = {" QJS_EVAL_TAG_PROP_NAME ":true}; "
		+ eastl::string(wrapInAsyncFn ? "(async function() " : "") +
		"{ "
		"\"use math\"; "
		"try { "
		"_bc_eval_." QJS_EVAL_RET_PROP_NAME " = "
		+ eastl::string(wrapInAsyncFn ? "(" + s + "); " : "eval('\"use math\"; void 0; ' + _bc_script_); ") +
		"CheckReturnValue (); "
		"} "
		"catch(e) { _bc_eval_." QJS_EVAL_RET_PROP_NAME " = \"Throw: \" + e; } "
		"} "
		+ eastl::string(wrapInAsyncFn ? ")(); " : "")
		).c_str(), asyncArgs.data(), asyncArgs.size(), &qjsRetU64, quiet);

	CheckRet();

	// process all the async operation requests.

	while (::strlen(asyncArgs.data()))
	{
		// determine which async op was requested and its arguments.

		auto v = Utils::Tokenize(eastl::string(asyncArgs.data()), "$delim$");

		::memset(asyncArgs.data(), 0, asyncArgs.size());

		s = "";
		eastl::string addJsCode = "";

		if (v[0] == "ReadMem" && v.size() == 4)
		{
			s = "[";

			auto dataSz = (size_t)::BC_strtoui64(v[3].c_str(), NULL, 10);

			if (dataSz == 1 || dataSz == 2 || dataSz == 4 || dataSz == 8)
			{
				auto sz = dataSz * (size_t)::BC_strtoui64(v[2].c_str(), NULL, 10);

				if (sz)
				{
					BYTE* pb = (BYTE*)co_await BcAwaiter_ReadMemory{
						(ULONG_PTR)::BC_strtoui64(v[1].c_str(), NULL, 10),
						sz };

					if (pb)
						for (int i = 0; i < sz; i += dataSz)
							s += "0x" + Utils::HexToString(dataSz == 1 ? pb[i] : dataSz == 2 ? *(WORD*)&pb[i] : dataSz == 4 ? *(DWORD*)&pb[i] : *(QWORD*)&pb[i], dataSz) +
							(dataSz == 8 ? "n" : "") +
							(i != sz - dataSz ? "," : "");
				}
			}

			s += "]";
		}
		else if (v[0] == "WriteMem" && v.size() > 3)
		{
			s = "false";

			auto address = (ULONG_PTR)::BC_strtoui64(v[1].c_str(), NULL, 10);
			auto dataSz = (size_t)::BC_strtoui64(v[2].c_str(), NULL, 10);

			if (dataSz == 1 || dataSz == 2 || dataSz == 4 || dataSz == 8)
			{
				eastl::vector<BYTE> data;

				for (int i = 3; i < v.size(); i++)
				{
					ULONG64 item = ::BC_strtoui64(v[i].c_str(), NULL, 10);

					const BYTE* start = (BYTE*)&item; // warning: truncates.

					data.insert(data.end(), start, start + dataSz);
				}

				BOOLEAN res = FALSE;

				co_await BcAwaiter_Join{ Cmd::WriteMemory(address, data.data(), data.size(), res) };

				if (res)
					s = "true";
			}
		}
		else if (v[0] == "WriteReg" && v.size() == 3)
		{
			s = "false";

			auto& regName = v[1];
			auto val = (ULONG64)::BC_strtoui64(v[2].c_str(), NULL, 10);

			VOID* preg = NULL;
			size_t sz = 0;

			ZydisRegister reg = X86Step::StringToZydisReg(regName.c_str());
			if (reg != ZYDIS_REGISTER_NONE)
			{
				preg = X86Step::ZydisRegToCtxRegValuePtr((CONTEXT*)context, reg, &sz);
			}

			if (_6432_(TRUE, FALSE) && is32bitCompat && sz == 8) // Unable to change 64-bit register in 32-bit mode.
			{
				preg = NULL;
				sz = 0;
			}

			if (preg && (sz == 4 || sz == 8))
			{
				// set the new value in the context.

				if (sz == 8)
					*(ULONG64*)preg = val;
				else
					*(ULONG32*)preg = (ULONG32)val;

				// set the new context.

				if (co_await BcAwaiter_StateManipulate{
				[&](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

					if (SecondBuffer->pData != NULL && contextLen > 0 && SecondBuffer->MaxLength >= contextLen)
					{
						pState->ApiNumber = DbgKdSetContextApi;

						::memcpy(SecondBuffer->pData, context, contextLen);
						SecondBuffer->Length = contextLen;
					}
				},
				[](DBGKD_MANIPULATE_STATE64* pState, PKD_BUFFER SecondBuffer) {

					return pState->ApiNumber == DbgKdSetContextApi && NT_SUCCESS(pState->ReturnStatus);
				} })
				{
					s = "true";

					AddRegs(addJsCode);
				}
			}
		}
		else
		{
			_bc_printf("Error: unrecognized async operation request.\n");
		}

		// resolve the promise passing our return value.

		if (s.size())
		{
			qjsRet = ::QJS_Eval_ES((
				addJsCode +
				"{ "
				"let resolve = _bc_eval_.resolve; "
				"_bc_eval_.resolve = null; "
				"_bc_eval_.reject = null; "
				"_bc_eval_.async_args = null; "
				"resolve(" + s + "); "
				"} "
				).c_str(), asyncArgs.data(), asyncArgs.size(), &qjsRetU64, quiet);

			CheckRet();
		}
	}

	// remove last line if empty + refresh ui.

	Root::I->LogWindow.AddString("", FALSE, !quiet);

	// return.

	co_return;
}

//
// Definitions of functions imported by QuickJS, that require some form of integration with BC:
//

extern "C" void* __stdcall malloc(__in size_t _Size)
{
	auto prevStart0to99 = Allocator::Start0to99;
	Allocator::Start0to99 = ALLOCATOR_START_0TO99_QUICKJS;

	void* ptr = Allocator::Alloc(_Size);

	Allocator::Start0to99 = prevStart0to99;

	return ptr;
}

extern "C" void __stdcall free(__inout_opt void* _Memory)
{
	Allocator::Free(_Memory);
}

extern "C" void* __stdcall realloc(__in_opt void* _Memory, __in size_t _NewSize)
{
	VOID* newMem = ::malloc(_NewSize);
	if (!newMem)
		return NULL;

	if (!_Memory)
		return newMem;

	auto s1 = Allocator::UserSizeInBytes(newMem);
	auto s2 = Allocator::UserSizeInBytes(_Memory);
	
	::memcpy(newMem, _Memory, _MIN_(s1, s2));

	Allocator::Free(_Memory);

	return newMem;
}

extern "C" int __stdcall _bc_printf_impl(const char* _Format, va_list ap)
{
	const int bufferSize = 512;
	eastl::unique_ptr<CHAR[]> buffer(new CHAR[bufferSize]);
	::memset(buffer.get(), 0, bufferSize);

	int ret = ::_vsnprintf(buffer.get(), bufferSize - 1, _Format, ap);

	Root::I->LogWindow.AddString(buffer.get(), TRUE, FALSE); // ui refresh = false, will be done later...

	return ret;
}

extern "C" int __stdcall _bc_printf(__in_z __format_string const char* _Format, ...)
{
	va_list ap;
	va_start(ap, _Format);

	int ret = ::_bc_printf_impl(_Format, ap);

	va_end(ap);

	return ret;
}

extern "C" int __stdcall _bc_vsnprintf(__out_ecount(_MaxCount) char* s, __in size_t n, __in_z __format_string const char* format, va_list ap)
{
	if (s && n)
		::memset(s, 0, n);

	return ::_vsnprintf(s, !n ? 0 : n - 1, format, ap);
}

extern "C" size_t __stdcall _bc_fwrite(__in_ecount(_Size * _Count) const void* _Str, __in size_t _Size, __in size_t _Count, __inout FILE * _File)
{
	if (_File == &_bc__iob[1] || _File == &_bc__iob[2]) // stdout and stderr, respectively
	{
		eastl::string str((CHAR*)_Str, _Size * _Count);

		::_bc_printf("%s", str.c_str());

		return _Count;
	}
	else
	{
		NOT_IMPLEMENTED("fwrite");
		return 0;
	}
}

extern "C" int __stdcall _bc_fprintf(__inout FILE * _File, __in_z __format_string const char* _Format, ...)
{
	if (_File == &_bc__iob[1] || _File == &_bc__iob[2]) // stdout and stderr, respectively
	{
		va_list ap;
		va_start(ap, _Format);

		int ret = ::_bc_printf_impl(_Format, ap);

		va_end(ap);

		return ret;
	}
	else
	{
		NOT_IMPLEMENTED("fprintf");
		return 0;
	}
}

extern "C" int __stdcall _bc_putchar(__in int _Ch)
{
	CHAR buffer[8];

	buffer[0] = (CHAR)_Ch;
	buffer[1] = 0;

	::_bc_printf("%s", buffer);

	return _Ch;
}

eastl::string _bc_strtod_INV(int decDigits, double valueAsDouble) // *somewhat* equivalent of "%+.*e" with FE_TONEAREST, for example (3, 0.001) returns "+1.000e-03".
{
	double d = valueAsDouble;

	if (decDigits < 0)
		decDigits = 0;

	const int bufferSize = 512;
	eastl::unique_ptr<CHAR[]> buffer(new CHAR[bufferSize]);

	auto toStr = [&](ULONG64 ui64) -> eastl::string {

		::memset(buffer.get(), 0, bufferSize);

		::_snprintf(buffer.get(), bufferSize - 1, "%I64u", (ULONG64)ui64);

		return eastl::string(buffer.get());
	};

	BOOLEAN sign = FALSE;

	if (d < 0)
	{
		sign = TRUE;
		d = -d;
	}

	int e = 0;
	const int maxE = 308;

	while (d < 0.1 && e >= -maxE)
	{
		d *= 10;
		e--;
	}

	if (e < -maxE)
	{
		d = 0;
		e = 0;
	}

	while (d >= 1 && e <= maxE)
	{
		d /= 10;
		e++;
	}

	LONG64 p = 1;
	for (int i = 0; i < decDigits + 1; i++)
		p *= 10;

	BOOLEAN rnd = (LONG64)(d * p * 10) % 10 >= 5;

	d *= p;

	auto dI = (ULONG64)d;

	if (rnd)
	{
		auto prevDI = dI;

		dI++;

		if (toStr(prevDI).size() != toStr(dI).size())
			e++;
	}

	eastl::string decComp = toStr(dI).substr(0, decDigits + 1);

	while (decComp.size() < decDigits + 1)
		decComp += "0";

	LONG64 intComp = decComp[0] - '0';

	decComp = decComp.substr(1);

	e--;

	eastl::string eAbsS = toStr(e >= 0 ? e : -e);

	if (eAbsS.size() == 1)
		eAbsS = "0" + eAbsS;

	return  (!sign ? "+" : "-") + toStr(intComp) + (decDigits ? "." : "") + decComp + "e" + (e >= 0 ? '+' : '-') + eAbsS;
}

extern "C" double __stdcall _bc_strtod(__in_z const char* _Str, __deref_opt_out_z char** _EndPtr)
{
	if (_EndPtr)
	{
		NOT_IMPLEMENTED("strtod");
		return 0;
	}

	if (!_Str)
		return 0;

	eastl::string s = _Str;

	for (CHAR& c : s)
		if (c == 'E')
			c = 'e';

	eastl::string intComp, decComp, expComp;

	auto v = Utils::Tokenize(s, ".");

	if (v.size() == 1)
	{
		auto v2 = Utils::Tokenize(v[0], "e");

		if (v2.size() == 1)
			intComp = v2[0];
		else if (v2.size() == 2)
		{
			intComp = v2[0];
			expComp = v2[1];
		}
	}
	else if (v.size() == 2)
	{
		intComp = v[0];

		auto v2 = Utils::Tokenize(v[1], "e");

		if (v2.size() == 1)
			decComp = v2[0];
		else if (v2.size() == 2)
		{
			decComp = v2[0];
			expComp = v2[1];
		}
	}
	else
		return 0;

	intComp.trim();
	decComp.trim();
	expComp.trim();

	decComp.rtrim("0");

	if (!intComp.size())
		intComp = "0";

	BOOLEAN sign = FALSE;

	auto nAbsI = ::BC_strtoi64(intComp.c_str(), NULL, 10);

	if (nAbsI < 0)
	{
		nAbsI = -nAbsI;
		sign = TRUE;
	}

	double n = nAbsI;

	if (decComp.size())
	{
		auto decCompI = ::BC_strtoi64(decComp.c_str(), NULL, 10);

		if (decCompI)
		{
			n +=
				(decltype(n))decCompI /
				(decltype(n))::BC_strtoi64(("1" + eastl::string(decComp.size(), '0')).c_str(), NULL, 10);
		}
	}

	if (expComp.size())
	{
		int e = ::BC_strtoi64(expComp.c_str(), NULL, 10);

		LONG64 v = 1;
		for (int i = 0; i < (e < 0 ? -e : e); i++)
			v *= 10;

		if (e < 0)
			n /= (decltype(n))v;
		else
			n *= (decltype(n))v;
	}

	if (sign)
		n = -n;

	return (double)n;
}

extern "C" int __stdcall _bc_snprintf(char* s, size_t n, const char* format, ...)
{
	if (s && n)
		::memset(s, 0, n);

	int ret = 0;

	if (!::strcmp(format, "%+.*e"))
	{
		va_list ap;
		va_start(ap, format);

		int decDigits = va_arg(ap, int);
		double d = va_arg(ap, double);

		va_end(ap);

		auto str = ::_bc_strtod_INV(decDigits, d).substr(0, !n ? 0 : n - 1);

		if (s)
			::strcpy(s, str.c_str());

		ret = str.size();
	}
	else
	{
		va_list ap;
		va_start(ap, format);
		ret = ::_vsnprintf(s, !n ? 0 : n - 1, format, ap);
		va_end(ap);
	}

	return ret;
}

extern "C" double __stdcall _bc_fabs(__in double _X)
{
	if (!_bc_isfinite(_X))
		return _X;

	if (_X >= 0)
		return _X;
	else
		return -_X;
}

extern "C" double __stdcall _bc_trunc(__in double _X)
{
	if (!_bc_isfinite(_X))
		return _X;

	if (_bc_fabs(_X) > ((__int64)1<<53))
		return _X;
	else
		return (__int64)_X;
}
