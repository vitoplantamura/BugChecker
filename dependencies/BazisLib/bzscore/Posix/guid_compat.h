#pragma once
#include "../../bzscore/assert.h"

#pragma pack(push, 1)
typedef struct _GUID {
    unsigned int   Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;
#pragma pack(pop)

C_ASSERT(sizeof(GUID) == 16);

typedef GUID *LPGUID;
typedef GUID *LPCGUID;
