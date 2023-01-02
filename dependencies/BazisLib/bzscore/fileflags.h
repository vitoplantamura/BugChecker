#pragma once

namespace BazisLib
{
	namespace FileFlags
	{
		enum FileAccess
		{
			QueryAccess		= 0x000001,
			ReadAccess	    = 0x000002,
			WriteAccess	    = 0x000004,
			ReadWriteAccess = 0x000008,
			DeleteAccess	= 0x000010,
			AllAccess		= 0x000020,
			ReadSecurity	= 0x000040,
			ModifySecurity	= 0x000080,
		};

		enum ShareMode
		{
			NoShare		= 0,
			ShareRead	= 0x01,
			ShareWrite	= 0x02,
			ShareDelete = 0x04,
			ShareAll	= 0x08,
		};

		enum OpenMode
		{
			OpenExisting,
			OpenAlways,
			CreateAlways,
			CreateNew,
		};
	}
}