#pragma once
#include <varargs.h>

namespace BazisLib
{
	enum Severity
	{
		sevUnknown = 0,
		sevDebug,
		sevInfo,
		sevWarning,
		sevError,
	};

	class ParsingLogger
	{
	protected:
		virtual void DoLogLine(Severity severity, int additionalType, const String &message)=0;

	public:
		void LogLine(Severity severity, const String &message)
		{
			if (!this)
				return;
			DoLogLine(severity, 0, message);
		}

		void LogLine(Severity severity, const TCHAR *pFormat, ...)
		{
			if (!this)
				return;
			va_list lst, va2;
			va_start(lst, pFormat);
			va_start(va2, pFormat);
			String line;
			line.vFormat(pFormat, lst, va2);
			LogLine(severity, line);
			va_end(lst);
			va_end(va2);
		}

		ActionStatus LogAndReturnError(ActionStatus error, const TCHAR *pFormat, ...)
		{
			if (!this)
				return error;
			va_list lst, va2;
			va_start(lst, pFormat);
			va_start(va2, pFormat);
			String line;
			line.vFormat(pFormat, lst, va2);
			LogLine(sevError, line);
			va_end(lst);
			va_end(va2);
			return error;
		}
	};

	class ParsingLoggerEx: public ParsingLogger
	{
	public:
		virtual void ReportProgress(ULONGLONG progress, ULONGLONG total) {}
		virtual void ReportAdvancedProgress(unsigned stage, unsigned totalStages, TempString &stageName, ULONGLONG stageProgress, ULONGLONG stageTotal) {}
	};
}