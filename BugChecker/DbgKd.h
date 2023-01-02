#pragma once

#include "BugChecker.h"

// ----------------------
// FROM REACTOS SOURCES:
// ----------------------

typedef struct _DBGKD_FILE_IO
{
	ULONG ApiNumber;
	ULONG Status;
	/*
	union
	{
		ULONG64 ReserveSpace[7];
		DBGKD_CREATE_FILE CreateFile;
		DBGKD_READ_FILE ReadFile;
		DBGKD_WRITE_FILE WriteFile;
		DBGKD_CLOSE_FILE CloseFile;
	} u;
	*/
} DBGKD_FILE_IO, * PDBGKD_FILE_IO;

//
// File I/O Types
//
#define DbgKdCreateFileApi                 0x00003430
#define DbgKdReadFileApi                   0x00003431
#define DbgKdWriteFileApi                  0x00003432
#define DbgKdCloseFileApi                  0x00003433

//

//
// Manipulate Types
//
#define DbgKdMinimumManipulate              0x00003130
#define DbgKdReadVirtualMemoryApi           0x00003130
#define DbgKdWriteVirtualMemoryApi          0x00003131
#define DbgKdGetContextApi                  0x00003132
#define DbgKdSetContextApi                  0x00003133
#define DbgKdWriteBreakPointApi             0x00003134
#define DbgKdRestoreBreakPointApi           0x00003135
#define DbgKdContinueApi                    0x00003136
#define DbgKdReadControlSpaceApi            0x00003137
#define DbgKdWriteControlSpaceApi           0x00003138
#define DbgKdReadIoSpaceApi                 0x00003139
#define DbgKdWriteIoSpaceApi                0x0000313A
#define DbgKdRebootApi                      0x0000313B
#define DbgKdContinueApi2                   0x0000313C
#define DbgKdReadPhysicalMemoryApi          0x0000313D
#define DbgKdWritePhysicalMemoryApi         0x0000313E
#define DbgKdQuerySpecialCallsApi           0x0000313F
#define DbgKdSetSpecialCallApi              0x00003140
#define DbgKdClearSpecialCallsApi           0x00003141
#define DbgKdSetInternalBreakPointApi       0x00003142
#define DbgKdGetInternalBreakPointApi       0x00003143
#define DbgKdReadIoSpaceExtendedApi         0x00003144
#define DbgKdWriteIoSpaceExtendedApi        0x00003145
#define DbgKdGetVersionApi                  0x00003146
#define DbgKdWriteBreakPointExApi           0x00003147
#define DbgKdRestoreBreakPointExApi         0x00003148
#define DbgKdCauseBugCheckApi               0x00003149
#define DbgKdSwitchProcessor                0x00003150
#define DbgKdPageInApi                      0x00003151
#define DbgKdReadMachineSpecificRegister    0x00003152
#define DbgKdWriteMachineSpecificRegister   0x00003153
#define OldVlm1                             0x00003154
#define OldVlm2                             0x00003155
#define DbgKdSearchMemoryApi                0x00003156
#define DbgKdGetBusDataApi                  0x00003157
#define DbgKdSetBusDataApi                  0x00003158
#define DbgKdCheckLowMemoryApi              0x00003159
#define DbgKdClearAllInternalBreakpointsApi 0x0000315A
#define DbgKdFillMemoryApi                  0x0000315B
#define DbgKdQueryMemoryApi                 0x0000315C
#define DbgKdSwitchPartition                0x0000315D
#define DbgKdWriteCustomBreakpointApi       0x0000315E
#define DbgKdGetContextExApi                0x0000315F
#define DbgKdSetContextExApi                0x00003160
#define DbgKdMaximumManipulate              0x00003161

//
// Packet Size and Control Stream Size
//
#define PACKET_MAX_SIZE                     4000
#define DBGKD_MAXSTREAM                     16

//
// Wait State Change Types
//
#define DbgKdMinimumStateChange             0x00003030
#define DbgKdExceptionStateChange           0x00003030
#define DbgKdLoadSymbolsStateChange         0x00003031
#define DbgKdCommandStringStateChange       0x00003032
#define DbgKdMaximumStateChange             0x00003033

typedef struct _DBGKD_CONTINUE
{
	NTSTATUS ContinueStatus;
} DBGKD_CONTINUE, * PDBGKD_CONTINUE;

#pragma pack (push, 4)

typedef struct _X86_DBGKD_CONTROL_SET
{
	ULONG TraceFlag;
	ULONG Dr7;
	ULONG CurrentSymbolStart;
	ULONG CurrentSymbolEnd;
} X86_DBGKD_CONTROL_SET, * PX86_DBGKD_CONTROL_SET;

typedef struct _AMD64_DBGKD_CONTROL_SET
{
	ULONG TraceFlag;
	ULONG64 Dr7;
	ULONG64 CurrentSymbolStart;
	ULONG64 CurrentSymbolEnd;
} AMD64_DBGKD_CONTROL_SET, * PAMD64_DBGKD_CONTROL_SET;

typedef struct _DBGKD_ANY_CONTROL_SET
{
	union
	{
		X86_DBGKD_CONTROL_SET X86ControlSet;
		//ALPHA_DBGKD_CONTROL_SET AlphaControlSet;
		//IA64_DBGKD_CONTROL_SET IA64ControlSet;
		AMD64_DBGKD_CONTROL_SET Amd64ControlSet;
		//ARM_DBGKD_CONTROL_SET ARMControlSet;
	};
} DBGKD_ANY_CONTROL_SET, * PDBGKD_ANY_CONTROL_SET;

#pragma pack (pop)

#pragma pack (push, 4)

typedef struct _DBGKD_CONTINUE2
{
	NTSTATUS ContinueStatus;
	union
	{
		//DBGKD_CONTROL_SET ControlSet;
		DBGKD_ANY_CONTROL_SET AnyControlSet;
	};
} DBGKD_CONTINUE2, * PDBGKD_CONTINUE2;

#pragma pack (pop)

typedef struct _DBGKM_EXCEPTION64
{
	EXCEPTION_RECORD64 ExceptionRecord;
	ULONG FirstChance;
} DBGKM_EXCEPTION64, * PDBGKM_EXCEPTION64;

typedef struct _DBGKD_LOAD_SYMBOLS64
{
	ULONG PathNameLength;
	ULONG64 BaseOfDll;
	ULONG64 ProcessId;
	ULONG CheckSum;
	ULONG SizeOfImage;
	BOOLEAN UnloadSymbols;
} DBGKD_LOAD_SYMBOLS64, * PDBGKD_LOAD_SYMBOLS64;

typedef struct _DBGKD_COMMAND_STRING
{
	ULONG Flags;
	ULONG Reserved1;
	ULONG64 Reserved2[7];
} DBGKD_COMMAND_STRING, * PDBGKD_COMMAND_STRING;

typedef struct _X86_DBGKD_CONTROL_REPORT
{
	ULONG   Dr6;
	ULONG   Dr7;
	USHORT  InstructionCount;
	USHORT  ReportFlags;
	UCHAR   InstructionStream[DBGKD_MAXSTREAM];
	USHORT  SegCs;
	USHORT  SegDs;
	USHORT  SegEs;
	USHORT  SegFs;
	ULONG   EFlags;
} X86_DBGKD_CONTROL_REPORT, * PX86_DBGKD_CONTROL_REPORT;

typedef struct _AMD64_DBGKD_CONTROL_REPORT
{
	ULONG64 Dr6;
	ULONG64 Dr7;
	ULONG EFlags;
	USHORT InstructionCount;
	USHORT ReportFlags;
	UCHAR InstructionStream[DBGKD_MAXSTREAM];
	USHORT SegCs;
	USHORT SegDs;
	USHORT SegEs;
	USHORT SegFs;
} AMD64_DBGKD_CONTROL_REPORT, * PAMD64_DBGKD_CONTROL_REPORT;

typedef struct _DBGKD_ANY_CONTROL_REPORT
{
	union
	{
		X86_DBGKD_CONTROL_REPORT X86ControlReport;
		//ALPHA_DBGKD_CONTROL_REPORT AlphaControlReport;
		//IA64_DBGKD_CONTROL_REPORT IA64ControlReport;
		AMD64_DBGKD_CONTROL_REPORT Amd64ControlReport;
		//ARM_DBGKD_CONTROL_REPORT ARMControlReport;
	};
} DBGKD_ANY_CONTROL_REPORT, * PDBGKD_ANY_CONTROL_REPORT;

typedef struct _DBGKD_ANY_WAIT_STATE_CHANGE
{
	ULONG NewState;
	USHORT ProcessorLevel;
	USHORT Processor;
	ULONG NumberProcessors;
	ULONG64 Thread;
	ULONG64 ProgramCounter;
	union
	{
		DBGKM_EXCEPTION64 Exception;
		DBGKD_LOAD_SYMBOLS64 LoadSymbols;
		DBGKD_COMMAND_STRING CommandString;
	} u;
	union
	{
		//DBGKD_CONTROL_REPORT ControlReport;
		DBGKD_ANY_CONTROL_REPORT AnyControlReport;
	};
} DBGKD_ANY_WAIT_STATE_CHANGE, * PDBGKD_ANY_WAIT_STATE_CHANGE;

typedef struct _DBGKD_GET_CONTEXT
{
	ULONG Unused;
} DBGKD_GET_CONTEXT, * PDBGKD_GET_CONTEXT;

typedef struct _DBGKD_SET_CONTEXT
{
	ULONG ContextFlags;
} DBGKD_SET_CONTEXT, * PDBGKD_SET_CONTEXT;

typedef struct _DBGKD_WRITE_BREAKPOINT64
{
	ULONG64 BreakPointAddress;
	ULONG BreakPointHandle;
} DBGKD_WRITE_BREAKPOINT64, * PDBGKD_WRITE_BREAKPOINT64;

typedef struct _DBGKD_RESTORE_BREAKPOINT
{
	ULONG BreakPointHandle;
} DBGKD_RESTORE_BREAKPOINT, * PDBGKD_RESTORE_BREAKPOINT;

typedef struct _DBGKD_BREAKPOINTEX
{
	ULONG BreakPointCount;
	NTSTATUS ContinueStatus;
} DBGKD_BREAKPOINTEX, * PDBGKD_BREAKPOINTEX;

typedef struct _DBGKD_GET_VERSION64
{
	USHORT MajorVersion;
	USHORT MinorVersion;
	UCHAR ProtocolVersion;
	UCHAR KdSecondaryVersion;
	USHORT Flags;
	USHORT MachineType;
	UCHAR MaxPacketType;
	UCHAR MaxStateChange;
	UCHAR MaxManipulate;
	UCHAR Simulation;
	USHORT Unused[1];
	ULONG64 KernBase;
	ULONG64 PsLoadedModuleList;
	ULONG64 DebuggerDataList;
} DBGKD_GET_VERSION64, * PDBGKD_GET_VERSION64;

typedef struct _DBGKD_READ_MEMORY64
{
	ULONG64 TargetBaseAddress;
	ULONG TransferCount;
	ULONG ActualBytesRead;
} DBGKD_READ_MEMORY64, * PDBGKD_READ_MEMORY64;

typedef struct _DBGKD_WRITE_MEMORY64
{
	ULONG64 TargetBaseAddress;
	ULONG TransferCount;
	ULONG ActualBytesWritten;
} DBGKD_WRITE_MEMORY64, * PDBGKD_WRITE_MEMORY64;

typedef struct _DBGKD_MANIPULATE_STATE32 // we only support the 64bit version, which is the version used by WinXP 32bit and above.
{
	ULONG ApiNumber;
	USHORT ProcessorLevel;
	USHORT Processor;
	NTSTATUS ReturnStatus;
	union
	{
		//DBGKD_READ_MEMORY32 ReadMemory;
		//DBGKD_WRITE_MEMORY32 WriteMemory;
		//DBGKD_READ_MEMORY64 ReadMemory64;
		//DBGKD_WRITE_MEMORY64 WriteMemory64;
		//DBGKD_GET_CONTEXT GetContext;
		//DBGKD_SET_CONTEXT SetContext;
		//DBGKD_WRITE_BREAKPOINT32 WriteBreakPoint;
		//DBGKD_RESTORE_BREAKPOINT RestoreBreakPoint;
		//DBGKD_CONTINUE Continue;
		//DBGKD_CONTINUE2 Continue2;
		//DBGKD_READ_WRITE_IO32 ReadWriteIo;
		//DBGKD_READ_WRITE_IO_EXTENDED32 ReadWriteIoExtended;
		//DBGKD_QUERY_SPECIAL_CALLS QuerySpecialCalls;
		//DBGKD_SET_SPECIAL_CALL32 SetSpecialCall;
		//DBGKD_SET_INTERNAL_BREAKPOINT32 SetInternalBreakpoint;
		//DBGKD_GET_INTERNAL_BREAKPOINT32 GetInternalBreakpoint;
		//DBGKD_GET_VERSION32 GetVersion32;
		//DBGKD_BREAKPOINTEX BreakPointEx;
		//DBGKD_READ_WRITE_MSR ReadWriteMsr;
		//DBGKD_SEARCH_MEMORY SearchMemory;
		//DBGKD_GET_SET_BUS_DATA GetSetBusData;
		//DBGKD_FILL_MEMORY FillMemory;
		//DBGKD_QUERY_MEMORY QueryMemory;
		//DBGKD_SWITCH_PARTITION SwitchPartition;
	} u;
} DBGKD_MANIPULATE_STATE32, * PDBGKD_MANIPULATE_STATE32;

typedef struct _DBGKD_MANIPULATE_STATE64
{
	ULONG ApiNumber;
	USHORT ProcessorLevel;
	USHORT Processor;
	NTSTATUS ReturnStatus;
	union
	{
		DBGKD_READ_MEMORY64 ReadMemory;
		DBGKD_WRITE_MEMORY64 WriteMemory;
		DBGKD_GET_CONTEXT GetContext;
		DBGKD_SET_CONTEXT SetContext;
		DBGKD_WRITE_BREAKPOINT64 WriteBreakPoint;
		DBGKD_RESTORE_BREAKPOINT RestoreBreakPoint;
		DBGKD_CONTINUE Continue;
		DBGKD_CONTINUE2 Continue2;
		//DBGKD_READ_WRITE_IO64 ReadWriteIo;
		//DBGKD_READ_WRITE_IO_EXTENDED64 ReadWriteIoExtended;
		//DBGKD_QUERY_SPECIAL_CALLS QuerySpecialCalls;
		//DBGKD_SET_SPECIAL_CALL64 SetSpecialCall;
		//DBGKD_SET_INTERNAL_BREAKPOINT64 SetInternalBreakpoint;
		//DBGKD_GET_INTERNAL_BREAKPOINT64 GetInternalBreakpoint;
		DBGKD_GET_VERSION64 GetVersion64;
		DBGKD_BREAKPOINTEX BreakPointEx;
		//DBGKD_READ_WRITE_MSR ReadWriteMsr;
		//DBGKD_SEARCH_MEMORY SearchMemory;
		//DBGKD_GET_SET_BUS_DATA GetSetBusData;
		//DBGKD_FILL_MEMORY FillMemory;
		//DBGKD_QUERY_MEMORY QueryMemory;
		//DBGKD_SWITCH_PARTITION SwitchPartition;
		//DBGKD_WRITE_CUSTOM_BREAKPOINT WriteCustomBreakpoint;
		//DBGKD_CONTEXT_EX ContextEx;
	} u;
} DBGKD_MANIPULATE_STATE64, * PDBGKD_MANIPULATE_STATE64;

//

#define DbgKdPrintStringApi                 0x00003230
#define DbgKdGetStringApi                   0x00003231

typedef struct _DBGKD_PRINT_STRING
{
	ULONG LengthOfString;
} DBGKD_PRINT_STRING, * PDBGKD_PRINT_STRING;

typedef struct _DBGKD_GET_STRING
{
	ULONG LengthOfPromptString;
	ULONG LengthOfStringRead;
} DBGKD_GET_STRING, * PDBGKD_GET_STRING;

typedef struct _DBGKD_DEBUG_IO
{
	ULONG ApiNumber;
	USHORT ProcessorLevel;
	USHORT Processor;
	union
	{
		DBGKD_PRINT_STRING PrintString;
		DBGKD_GET_STRING GetString;
	} u;
} DBGKD_DEBUG_IO, * PDBGKD_DEBUG_IO;

//

//
// Debug Status Codes
//
#define DBG_STATUS_CONTROL_C                1
#define DBG_STATUS_SYSRQ                    2
#define DBG_STATUS_BUGCHECK_FIRST           3
#define DBG_STATUS_BUGCHECK_SECOND          4
#define DBG_STATUS_FATAL                    5
#define DBG_STATUS_DEBUG_CONTROL            6
#define DBG_STATUS_WORKER                   7

//
// DebugService Control Types
//
#define BREAKPOINT_BREAK                    0
#define BREAKPOINT_PRINT                    1
#define BREAKPOINT_PROMPT                   2
#define BREAKPOINT_LOAD_SYMBOLS             3
#define BREAKPOINT_UNLOAD_SYMBOLS           4
#define BREAKPOINT_COMMAND_STRING           5
