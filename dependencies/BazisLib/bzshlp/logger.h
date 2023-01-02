#pragma once
#include "cmndef.h"
#include "../bzscore/file.h"

namespace BazisLib
{
	//! Provides data logging functionality for debug purposes
	class SimpleLogger
	{
	private:
		File m_File;

	public:
		SimpleLogger(TCHAR *ptszFileName)
			: m_File(ptszFileName, FileModes::CreateOrOpenRW)
		{
			m_File.Seek(0, FileFlags::FileEnd);
		}

		void LogLineA(const char *pszFormat, ...)
		{
			va_list lst;
			va_start(lst, pszFormat);
			char tsz[1024];
			vsnprintf(tsz, sizeof(tsz), pszFormat, lst);
			m_File.Write(tsz, strlen(tsz));
		}
	};
}