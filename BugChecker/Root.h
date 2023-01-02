#pragma once

#include "BugChecker.h"

#include "SymbolFile.h"
#include "BcCoroutine.h"
#include "FunctionPatch.h"
#include "MemReadCor.h"
#include "DbgKd.h"
#include "Root.h"
#include "Ioctl.h"
#include "DisasmWnd.h"
#include "RegsWnd.h"
#include "InputLine.h"
#include "LogWnd.h"
#include "Cmd.h"
#include "CodeWnd.h"

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>

#define ALLOCATOR_START_0TO99_NTMODULES 60
#define ALLOCATOR_START_0TO99_LOG 40
#define ALLOCATOR_START_0TO99_QUICKJS 20

#define LOG_WINDOW_CONTENTS_MAX_SIZE (1*1024*1024)

typedef enum _DEBUGGER_STATE
{
	DEBST_UNKNOWN,
	DEBST_STATE_CHANGED,
	DEBST_CONTINUE,
	DEBST_MEMREADCOR,
	DEBST_COROUTINE,
	DEBST_PROCESS_AWAITER
} DEBUGGER_STATE;

enum class KdcomHook
{
	None = 0,
	Callback,
	Patch
};

class BreakPoint
{
public:

	ULONG64 address = 0;
	ULONG handle = 0;

	BYTE prevByte = 0;

	ULONG64 eprocess = 0;
	ULONG64 ethread = 0;

	eastl::string when;

	BOOLEAN skip = FALSE;

	BOOLEAN isStepBp = FALSE;

	eastl::string cmd;
};

class BpTrace
{
public:
	ULONG64 addr = 0;
	ULONG64 nextAddr = 0;

	static BpTrace& Get();
};

enum class CursorFocus
{
	Log,
	Code
};

extern char g_debugStr[256];

class MemoryReaderCoroutineState;
struct BcAwaiterBase;
class DisasmWnd;

class Root
{
public: // construction + singleton

	Root();

	static Root* I;

public: // cpp coroutines

	volatile BcAwaiterBase* AwaiterPtr = NULL;
	eastl::vector<BcAwaiterBase*> AwaiterStack;

public: // symbol files

	eastl::vector<SymbolFile> SymbolFiles;

	SymbolFile* GetSymbolFileByGuidAndAge(const BYTE* guid, const ULONG age);

public: // framebuffer

	VOID* VideoAddr = NULL;

	ULONG fbWidth = 0;
	ULONG fbHeight = 0;
	ULONGLONG fbAddress = 0;
	ULONG fbStride = 0;

	ULONG fbMapSize = 0;

	BOOLEAN bcDebug = FALSE;

	DWORD* VmFifo = NULL;
	ULONG64 VmFifoPhys = 0; // 0xE8400000;
	ULONG VmFifoSize = 0; // 0xE85FFFFF - 0xE8400000 + 1;
	ULONG VmIoPort = 0; // 0xC030;

public: // window

	const static int TextBuffersDim = 128 * 128;

	USHORT FrontBuffer[TextBuffersDim] = { 0 };
	USHORT BackBuffer[TextBuffersDim] = { 0 };

	BYTE VideoRestoreBuffer[4 * 1024 * 1024];
	BOOLEAN VideoRestoreBufferState = FALSE;
	ULONG64 VideoRestoreBufferTimer = 0;
	ULONG64 VideoRestoreBufferTimeOut = 0;

	ULONG WndWidth = 80;
	ULONG WndHeight = 25;

	const ULONG GlyphWidth = 9;
	const ULONG GlyphHeight = 16;

	LONG RegsDisasmDivLineY = _6432_(7, 4); // can be < 0 if REGS is hidden.
	LONG DisasmCodeDivLineY = 13; // can be < 0 if DISASM is hidden.
	LONG CodeLogDivLineY = 17; // can be < 0 if CODE is hidden.

	RegsWnd RegsWindow;
	DisasmWnd DisasmWindow;
	LogWnd LogWindow;
	InputLine InputLine;
	CodeWnd CodeWindow;

	eastl::string CurrentImageFileName = "";
	LONG CurrentIrql = -1;

public: // cursor

	VOID CalculateRdtscTimeouts();

	ULONG64 CursorBlinkTime = 0;

	LONG CursorDisplayX = -1;
	LONG CursorDisplayY = -1;

	LONG CursorX = 2;
	LONG CursorY = 22;

	LONG LogCursorX = 2;
	LONG LogCursorY = 22;

	LONG CodeCursorX = 2;
	LONG CodeCursorY = 14;

	CursorFocus CursorFocus = CursorFocus::Log;

public: // Wnd::CheckDivLineYs function

	ULONG PrevWndWidth = 0;
	ULONG PrevWndHeight = 0;

	eastl::vector<LONG> PrevLayout;

public: // nt modules

	eastl::vector<eastl::pair<ULONG64, eastl::vector<NtModule>>>* NtModules = NULL; // we don't use map or multimap because they require rbtree_node_base which is defined in an EASTL implementation file.

	BcSpinLock DebuggerLock; // WARNING!! this also blocks IPIs sent by the kernel to freeze the processors, thus blocking entry into the KD.

	BOOLEAN ProcessNotifyCreated = FALSE;
	BOOLEAN ImageNotifyCreated = FALSE;

public: // kdcom hook

	KdcomHook kdcomHook = KdcomHook::None;

	FunctionPatch* KdSendPacketFp = NULL;
	FunctionPatch* KdReceivePacketFp = NULL;

public: // trace/bp

	BOOLEAN Trace = FALSE;
	eastl::vector<eastl::pair<ULONG64, BpTrace>> BpTraces;

	ULONG64 StepOutThread = 0;

	LONG BpHitIndex = -1;

public: // others

	BYTE ExpandedStack[128 * 1024]; // used by QuickJS.

	eastl::vector<BreakPoint> BreakPoints;

	eastl::vector<eastl::unique_ptr<Cmd>> Cmds;

	DBGKD_ANY_WAIT_STATE_CHANGE StateChange;

	DEBUGGER_STATE DebuggerState = DEBST_UNKNOWN;

	BOOLEAN DebuggerPresentSet = FALSE;

	MemoryReaderCoroutineState MemReadCor;

	ULONG64 PsLoadedModuleList = 0;

	eastl::allocator eastl_allocator;
};
