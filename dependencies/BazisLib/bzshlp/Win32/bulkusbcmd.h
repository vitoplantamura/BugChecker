/*
Request format (name:bytecount):
	[RQTYPE:4] [RQCODE:4] [RQDATASIZE:4] [RQDATA]
Result format:
	[RESTYPE:4] [STATUS:4] [RESDATASIZE:4] [RESDATA]

Data size is in bytes.
Minimal header size is 12 bytes with DATASIZE=0
*/

#define BULKUSB_RQTYPE_QUERY_PROTOVER	0x50BAC05A	//Query current protocol version
#define BULKUSB_RQTYPE_QUERY_MAX_SIZE	0x50BAC05B	//Temp buffer size request
#define BULKUSB_RQTYPE_USER				0x50BAC05C	//Standard user request
#define BULKUSB_RQTYPE_USERV			0x50BAC05D	//User request with variable result size (see BULKUSB_RESTYPE_USERV)
#define BULKUSB_RQTYPE_ENTERLDR			0x50BAC05E	//Enter bootloader reques
#define BULKUSB_RQTYPE_ENTER_RAWMODE	0x50BAC05F	//Enter RAW read/write mode

#define BULKUSB_RESTYPE_SYSTEM			0xA0BAC0AB	//Reply to a system request
#define BULKUSB_RESTYPE_USER			0xA0BAC0AC	//Reply to a user request
#define BULKUSB_RESTYPE_USERV			0xA0BAC0AD	//Variable-size reply. The first frame contains only header, while
													//the next ones contain data.

#define BULKUSB_STATUS_OK			0x00000000
#define BULKUSB_STATUS_BADREQUEST	0x80070057L	//E_INVALIDARG
#define BULKUSB_STATUS_FAIL			0x80004005L	//E_FAIL

#define PROTOCOL_VERSION_CURRENT	0x0100

#define BULKUSB_HEADER_SIZE		12
