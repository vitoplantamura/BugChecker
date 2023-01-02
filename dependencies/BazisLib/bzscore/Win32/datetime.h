#pragma once
#include "../string.h"

namespace BazisLib
{
	namespace Win32
	{
		class _TimeSpan
		{
		protected:
			ULONGLONG m_Total100NanosecondIntervals;
			_TimeSpan(ULONGLONG Total100NanosecondIntervals) : m_Total100NanosecondIntervals(Total100NanosecondIntervals){}
		};

		class _DateTime
		{
		protected:
			union
			{
				FILETIME m_FileTime;
				ULONGLONG m_Total100NanosecondIntervals;
			} m_Union;
			SYSTEMTIME *m_pCachedLocalTime;

			_DateTime(ULONGLONG Total100NanosecondIntervals) : m_pCachedLocalTime(NULL) { m_Union.m_Total100NanosecondIntervals = Total100NanosecondIntervals;}
			_DateTime(const FILETIME &FileTime) : m_pCachedLocalTime(NULL) { m_Union.m_FileTime = FileTime;}
			~_DateTime() {delete m_pCachedLocalTime;}

		public:
			FILETIME *GetFileTime() {return &m_Union.m_FileTime;}
			const FILETIME *GetFileTime() const {return &m_Union.m_FileTime;}
		};
	}

	class TimeSpan : public Win32::_TimeSpan
	{
	public:
		ULONGLONG GetTotalDays() const
		{
			return m_Total100NanosecondIntervals / (10000000LL * 3600 * 24);
		}

		ULONGLONG GetTotalHours() const
		{
			return m_Total100NanosecondIntervals / (10000000LL * 3600);
		}

		ULONGLONG GetTotalMinutes() const
		{
			return m_Total100NanosecondIntervals / (10000000LL * 60);
		}

		ULONGLONG GetTotalSeconds() const
		{
			return m_Total100NanosecondIntervals / 10000000LL;
		}

		ULONGLONG GetTotalMilliseconds() const
		{
			return m_Total100NanosecondIntervals / 10000LL;
		}

		ULONGLONG GetTotal100NanosecondIntervals() const
		{
			return m_Total100NanosecondIntervals;
		}

		unsigned GetHours() const
		{
			return (unsigned)(GetTotalHours() % 24);
		}

		unsigned GetMinutes() const
		{
			return (unsigned)(GetTotalMinutes() % 60);
		}

		unsigned GetSeconds() const
		{
			return (unsigned)(GetTotalSeconds() % 60);
		}

		TimeSpan(ULONGLONG Total100NanosecondIntervals = 0) : _TimeSpan(Total100NanosecondIntervals)
		{
		}

		TimeSpan(unsigned Hours, unsigned Minutes, unsigned Seconds, unsigned Milliseconds) :
			_TimeSpan( ((((((Hours * 60LL) + Minutes) * 60LL) + Seconds) * 1000LL) * Milliseconds) * 10000LL)
		{
		}

		TimeSpan(unsigned Days, unsigned Hours, unsigned Minutes, unsigned Seconds, unsigned Milliseconds) :
			_TimeSpan( (((((((Days * 24LL) + Hours * 60LL) + Minutes) * 60LL) + Seconds) * 1000LL) * Milliseconds) * 10000LL)
		{
		}

		TimeSpan operator+(const TimeSpan &ts) const
		{
			return TimeSpan(m_Total100NanosecondIntervals + ts.m_Total100NanosecondIntervals);
		}

		TimeSpan operator-(const TimeSpan &ts) const
		{
			return TimeSpan(m_Total100NanosecondIntervals - ts.m_Total100NanosecondIntervals);
		}

		TimeSpan operator*(unsigned Count) const
		{
			return TimeSpan(m_Total100NanosecondIntervals * Count);
		}

		TimeSpan operator/(unsigned Count) const
		{
			return TimeSpan(m_Total100NanosecondIntervals / Count);
		}

		TimeSpan &operator+=(const TimeSpan &ts)
		{
			m_Total100NanosecondIntervals += ts.m_Total100NanosecondIntervals;
			return *this;
		}

		TimeSpan &operator-=(const TimeSpan &ts)
		{
			m_Total100NanosecondIntervals -= ts.m_Total100NanosecondIntervals;
			return *this;
		}

		~TimeSpan()
		{
		}

		String ToString(bool IncludeMsec = false)
		{
			TCHAR tsz[128];
			if (!IncludeMsec)
				_sntprintf_s(tsz, _countof(tsz), _TRUNCATE, _T("%02d:%02d:%02d"), (int)GetTotalHours(), GetMinutes(), GetSeconds());
			else
				_sntprintf_s(tsz, _countof(tsz), _TRUNCATE, _T("%02d:%02d:%02d.%03d"), (int)GetTotalHours(), GetMinutes(), GetSeconds(), (int)(GetTotalMilliseconds() % 1000));
			return tsz;
		}
	};

	class DateTime : public Win32::_DateTime
	{
	private:
		DateTime(ULONGLONG Total100NanosecondIntervals) : _DateTime(Total100NanosecondIntervals) {}

		//Win32-specific methods
	public:

		//This method is declared constant as it does not change the time iteslf. It operates only the internal state.
		const SYSTEMTIME &ToSystemTime() const
		{
			if (m_pCachedLocalTime)
				return *m_pCachedLocalTime;
			
			const_cast<DateTime *>(this)->m_pCachedLocalTime = new SYSTEMTIME;
			if (!m_pCachedLocalTime)
				return *((SYSTEMTIME *)NULL);
			FILETIME UpdatedFileTime;
			FileTimeToLocalFileTime(&m_Union.m_FileTime, &UpdatedFileTime);
			FileTimeToSystemTime(&UpdatedFileTime, m_pCachedLocalTime);
			return *m_pCachedLocalTime;
		}

	public:
		DateTime(const FILETIME &FileTime) : _DateTime(FileTime)
		{
		}

		DateTime() :
			_DateTime(0LL)
		{
		}

		DateTime(DateTime &date, unsigned Hour, unsigned Minute, unsigned Second, unsigned Milliseconds) :
		  _DateTime(date)
		{
			SYSTEMTIME sysTime;
			FileTimeToSystemTime(&m_Union.m_FileTime, &sysTime);
			sysTime.wHour = Hour;
			sysTime.wMinute = Minute;
			sysTime.wSecond = Second;
			sysTime.wMilliseconds = Milliseconds;
			SystemTimeToFileTime(&sysTime, &m_Union.m_FileTime);
		}

		~DateTime()
		{
		}

		//One-based indexes
		unsigned GetYear() const
		{
			return ToSystemTime().wYear;
		}

		unsigned GetMonth() const
		{
			return ToSystemTime().wMonth;
		}

		unsigned GetDay() const
		{
			return ToSystemTime().wDay;
		}

		unsigned GetHour() const
		{
			return ToSystemTime().wHour;
		}

		unsigned GetMinute() const
		{
			return ToSystemTime().wMinute;
		}

		unsigned GetSecond() const
		{
			return ToSystemTime().wSecond;
		}

		unsigned GetDayOfWeek() const //Zero-based index
		{
			return ToSystemTime().wDayOfWeek;
		}	

		TimeSpan operator-(const DateTime& tm) const
		{
			return TimeSpan(m_Union.m_Total100NanosecondIntervals - tm.m_Union.m_Total100NanosecondIntervals);
		}

		DateTime operator-(const TimeSpan& ts) const
		{
			return DateTime(m_Union.m_Total100NanosecondIntervals - ts.GetTotal100NanosecondIntervals());
		}

		DateTime &operator-=(const TimeSpan& ts)
		{
			m_Union.m_Total100NanosecondIntervals -= ts.GetTotal100NanosecondIntervals();
			return *this;
		}

		DateTime operator+(const TimeSpan &ts) const
		{
			return DateTime(m_Union.m_Total100NanosecondIntervals + ts.GetTotal100NanosecondIntervals());
		}

		DateTime &operator+=(const TimeSpan &ts)
		{
			m_Union.m_Total100NanosecondIntervals += ts.GetTotal100NanosecondIntervals();
			return *this;
		}

		bool operator<(const DateTime &tm) const
		{
			return (m_Union.m_Total100NanosecondIntervals < tm.m_Union.m_Total100NanosecondIntervals);
		}

		bool operator==(const DateTime &tm) const
		{
			return (m_Union.m_Total100NanosecondIntervals == tm.m_Union.m_Total100NanosecondIntervals);
		}

		bool operator>(const DateTime &tm) const
		{
			return (m_Union.m_Total100NanosecondIntervals > tm.m_Union.m_Total100NanosecondIntervals);
		}

	public:
		//The following operators may work slow!
		void SetYear(unsigned NewValue);
		void SetMonth(unsigned NewValue);
		void SetDay(unsigned NewValue);
		void SetHour(unsigned NewValue);
		void SetMinute(unsigned NewValue);
		void SetSecond(unsigned NewValue);

	public:
		static DateTime Now()
		{
			FILETIME FileTime;
#ifdef UNDER_CE
			SYSTEMTIME SysTime;
			GetSystemTime(&SysTime);
			SystemTimeToFileTime(&SysTime, &FileTime);
#else
			GetSystemTimeAsFileTime(&FileTime);
#endif
			return DateTime(FileTime);
		}

		static DateTime Today()
		{
			FILETIME FileTime;
#ifdef UNDER_CE
			SYSTEMTIME SysTime;
			GetSystemTime(&SysTime);
			SystemTimeToFileTime(&SysTime, &FileTime);
#else
			GetSystemTimeAsFileTime(&FileTime);
#endif
			DateTime dt = DateTime(FileTime);
			return DateTime(dt, 0, 0, 0, 0);
		}

		//! Returns the number of milliseconds elapsed from this DateTime value
		ULONGLONG MillisecondsElapsed() const
		{
			return (Now() - *this).GetTotalMilliseconds();
		}

		String ToString()
		{
			return String::sFormat(_T("%02d.%02d.%04d %02d:%02d:%02d"), GetDay(), GetMonth(), GetYear(), GetHour(), GetMinute(), GetSecond());
		}
	};
}