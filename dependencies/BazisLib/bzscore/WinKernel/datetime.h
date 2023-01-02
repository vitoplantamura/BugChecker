#pragma once
#include "../string.h"

namespace BazisLib
{
	namespace DDK
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
				LARGE_INTEGER m_LargeInteger;
				ULONGLONG m_Total100NanosecondIntervals;
			} m_Union;
			TIME_FIELDS *m_pCachedLocalTime;

			_DateTime(ULONGLONG Total100NanosecondIntervals) : m_pCachedLocalTime(NULL) { m_Union.m_Total100NanosecondIntervals = Total100NanosecondIntervals;}
			_DateTime(const LARGE_INTEGER &LargeInteger) : m_pCachedLocalTime(NULL) { m_Union.m_LargeInteger = LargeInteger;}
			~_DateTime() {delete m_pCachedLocalTime;}

		public:
			LARGE_INTEGER *GetLargeInteger() {return &m_Union.m_LargeInteger;}
			const LARGE_INTEGER *GetLargeInteger() const {return &m_Union.m_LargeInteger;}
		};
	}

	class TimeSpan : public DDK::_TimeSpan
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
	};

	class DateTime : public DDK::_DateTime
	{
	private:
		DateTime(ULONGLONG Total100NanosecondIntervals) : _DateTime(Total100NanosecondIntervals) {}

		//Win32-specific methods
	public:

		//This method is declared constant as it does not change the time iteslf. It operates only the internal state.
		const TIME_FIELDS &ToTimeFields() const
		{
			if (m_pCachedLocalTime)
				return *m_pCachedLocalTime;
			
			const_cast<DateTime *>(this)->m_pCachedLocalTime = new TIME_FIELDS;
			LARGE_INTEGER localTime;
			ExSystemTimeToLocalTime(const_cast<LARGE_INTEGER *>(&m_Union.m_LargeInteger), &localTime);
			RtlTimeToTimeFields(&localTime, m_pCachedLocalTime);
			return *m_pCachedLocalTime;
		}

	public:
		DateTime(const LARGE_INTEGER &LARGE_INTEGER) : _DateTime(LARGE_INTEGER)
		{
		}

		DateTime() :
			_DateTime(0LL)
		{
		}

		DateTime(DateTime &date, unsigned short Hour, unsigned short Minute, unsigned short Second, unsigned short Milliseconds) :
		  _DateTime(date)
		{
			TIME_FIELDS sysTime;
			RtlTimeToTimeFields(&m_Union.m_LargeInteger, &sysTime);
			sysTime.Hour = Hour;
			sysTime.Minute = Minute;
			sysTime.Second = Second;
			sysTime.Milliseconds = Milliseconds;
			RtlTimeFieldsToTime(&sysTime, &m_Union.m_LargeInteger);
		}

		~DateTime()
		{
		}

		//One-based indexes
		unsigned GetYear() const
		{
			return ToTimeFields().Year;
		}

		unsigned GetMonth() const
		{
			return ToTimeFields().Month;
		}

		unsigned GetDay() const
		{
			return ToTimeFields().Day;
		}

		unsigned GetHour() const
		{
			return ToTimeFields().Hour;
		}

		unsigned GetMinute() const
		{
			return ToTimeFields().Minute;
		}

		unsigned GetSecond() const
		{
			return ToTimeFields().Second;
		}

		unsigned GetDayOfWeek() const //Zero-based index
		{
			return ToTimeFields().Weekday;
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
			LARGE_INTEGER now;
			KeQuerySystemTime(&now);
			return DateTime(now);
		}

		static DateTime Today()
		{
			LARGE_INTEGER now;
			KeQuerySystemTime(&now);
			return DateTime(DateTime(now), 0, 0, 0, 0);
		}

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