#pragma once

#include "BugChecker.h"

//==============================
// This code is from VirtualKD:
//==============================

#pragma pack (push, 2)

//! Values returned by KdReceivePacket
typedef enum _KD_RECV_CODE
{
	KD_RECV_CODE_OK = 0,
	KD_RECV_CODE_TIMEOUT = 1,
	KD_RECV_CODE_FAILED = 2
} KD_RECV_CODE, * PKD_RECV_CODE;

//! Possible packet types used by KdSendPacket()/KdReceivePacket()
enum
{
	KdPacketType2_Ie_StateManipulate = 2,
	KdPacketType3_Ie_DebugIO = 3,
	//! Sent when a data packet was successfully received
	KdPacketAcknowledge = 4,
	//! Sent when a corrupted data packet is received (or it cannot be processed now)
	KdPacketRetryRequest = 5,
	//! Sent by WinDBG to resynchronize target and by target to acknowledge resync
	KdPacketResynchronize = 6,
	KdPacketType7_Ie_StateChange64 = 7,
	//! Not a packet type. When specified to KdReceivePacket(), it checks whether any data is available on COM port.
	KdCheckForAnyPacket = 8,
	KdPacketType11_Ie_FileIO = 11,
};

//! Represents a buffer used by KdSendPacket()/KdReceivePacket().
typedef struct _KD_BUFFER
{
	USHORT  Length;
	USHORT  MaxLength;
#ifdef _AMD64_
	ULONG	__Padding;
#endif
	PUCHAR  pData;
} KD_BUFFER, * PKD_BUFFER;

//! Represents the global state of the KD packet layer
typedef struct _KD_CONTEXT
{
	//! Specifies the packet retry count for KdSendPacket
	ULONG   RetryCount;
	//! Set to TRUE, if the debugger requested breaking execution
	BOOLEAN BreakInRequested;
} KD_CONTEXT, * PKD_CONTEXT;

extern "C"
{
	//! Called by kernel to receive a packet of a specified type
	/*! This function is called by Windows kernel to receive a debug packet of a particular type.
		\param PacketType Specifies the type of packet to receive. If KdCheckForAnyPacket is specified, the function
		checks whether any data is available (was sent by debugger, but not yet received) and returns KD_RECV_CODE_OK
		or KD_RECV_CODE_TIMEOUT respectively without performing any other action.
		\param FirstBuffer Specifies the buffer where the first KD_BUFFER::MaxLength bytes of a packet are stored
		\param SecondBuffer Specifies the buffer where the rest data of the packet is stored.
		\param PayloadBytes Points to an ULONG value receiving the number of bytes written to SecondBuffer
		\param KdContext Points to a KD_CONTEXT variable storing global packet layer context. If debugger requests
						 stopping the execution, the KD_CONTEXT::BreakInRequested is set to TRUE.
	*/
	KD_RECV_CODE
		__stdcall
		KdReceivePacket(
			__in ULONG PacketType,
			__inout_opt PKD_BUFFER FirstBuffer,
			__inout_opt PKD_BUFFER SecondBuffer,
			__out_opt PULONG PayloadBytes,
			__inout_opt PKD_CONTEXT KdContext
		);

	//! Called by kernel to send a debug packet
	/*! This function is called by Windows kernel to send a debug packet.
		\param PacketType Specifies the packet type to send.
		\param FirstBuffer Specifies the first part of the packet body.
		\param SecondBuffer Specifies the second part of the packet body.
		\param KdContext Points to a KD_CONTEXT variable storing global packet layer context. KD_CONTEXT::RetryCount
						 is used by original KDCOM.DLL implementation.
		\remarks Note that the packet itself contains no information about sizes of FirstBuffer and SecondBuffer,
				 its body just contains both buffer contens placed one after another.
	*/
	VOID
		__stdcall
		KdSendPacket(
			__in ULONG PacketType,
			__in PKD_BUFFER FirstBuffer,
			__in_opt PKD_BUFFER SecondBuffer,
			__inout PKD_CONTEXT KdContext
		);

}

//! Represents a KDCOM packet header in the way it is sent via COM port
/*! This structure was extracted from KDCOM.DLL
*/
typedef struct _KD_PACKET_HEADER
{
	ULONG Signature;
	USHORT PacketType;
	USHORT TotalDataLength;
	ULONG PacketID;
	ULONG Checksum;
} KD_PACKET_HEADER, * PKD_PACKET_HEADER;

#ifndef _DDK_
typedef LONG NTSTATUS;
#endif

extern "C"
{
	//! Performs initial KD extension DLL initialization
	NTSTATUS __stdcall KdDebuggerInitialize0(PVOID lpLoaderParameterBlock);
}

//! Contains the KD_BUFFER fields except for buffer pointer. Used for marshalling.
struct SendableKdBuffer
{
	USHORT  Length;
	USHORT  MaxLength;
};

#pragma pack (pop)

//====================
// Function Pointers:
//====================

extern "C" typedef VOID(__stdcall* PFNKdSendPacket)
(__in ULONG PacketType, __in PKD_BUFFER FirstBuffer, __in_opt PKD_BUFFER SecondBuffer, __inout PKD_CONTEXT KdContext);

extern "C" typedef KD_RECV_CODE(__stdcall* PFNKdReceivePacket)
(__in ULONG PacketType, __inout_opt PKD_BUFFER FirstBuffer, __inout_opt PKD_BUFFER SecondBuffer, __out_opt PULONG PayloadBytes, __inout_opt PKD_CONTEXT KdContext);

extern "C" typedef VOID(__stdcall* PFNKdSetBugCheckerCallbacks)
(__in PFNKdSendPacket pfnSend, __in PFNKdReceivePacket pfnReceive);
