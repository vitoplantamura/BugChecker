#pragma once
#include "../string.h"
#include <sys/time.h>

namespace BazisLib
{
	namespace Posix
	{
		class _TimeSpan
		{
		protected:
			uint64_t m_TotalMicroseconds;
			_TimeSpan(const timeval &tv)
			{
				m_TotalMicroseconds = (((uint64_t)tv.tv_sec) * 1000000) + tv.tv_usec;
			}

			_TimeSpan(uint64_t microseconds)
				: m_TotalMicroseconds(microseconds)
			{
			}

		public:
			timeval ToTV()
			{
				timeval tv = {0,0};
				tv.tv_sec = (uint32_t)(m_TotalMicroseconds / 1000000);
				tv.tv_usec  = (uint32_t)(m_TotalMicroseconds % 1000000);
				return tv;
			}
		};

		class _DateTime
		{
		protected:
			timeval m_TimeVal;

			mutable struct tm *m_pCachedTM;

			_DateTime(const timeval &tv)
				: m_TimeVal(tv)
				, m_pCachedTM(NULL)
			{
			}

			_DateTime()
			{
				m_TimeVal.tv_sec = m_TimeVal.tv_usec = 0;
				m_pCachedTM = NULL;
			}

#ifndef BAZISLIB_NO_STRUCTURED_TIME
			_DateTime(const tm &t)
			{
				m_pCachedTM = (tm *)malloc(sizeof(tm));
				*m_pCachedTM = t;

				m_TimeVal.tv_sec = mktime(m_pCachedTM);
				m_TimeVal.tv_usec = 0;
			}
#endif

			_DateTime(const _DateTime &dt)
				: m_TimeVal(dt.m_TimeVal)
				, m_pCachedTM(NULL)
			{
			}

			~_DateTime()
			{
				if (m_pCachedTM)
					free(m_pCachedTM);
			}

		public:
			timeval ToTimeval() const
			{
				return m_TimeVal;
			}
		};
	}

	class TimeSpan : public Posix::_TimeSpan
	{
	public:
		uint64_t GetTotalDays() const
		{
			return m_TotalMicroseconds / (1000000LL * 3600 * 24);
		}

		uint64_t GetTotalHours() const
		{
			return m_TotalMicroseconds / (1000000LL * 3600);
		}

		uint64_t GetTotalMinutes() const
		{
			return m_TotalMicroseconds / (1000000LL * 60);
		}

		uint64_t GetTotalSeconds() const
		{
			return m_TotalMicroseconds / 1000000LL;
		}

		uint64_t GetTotalMilliseconds() const
		{
			return m_TotalMicroseconds / 1000LL;
		}

		uint64_t GetTotal100NanosecondIntervals() const
		{
			return m_TotalMicroseconds;
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

		TimeSpan(const timeval &tv) : _TimeSpan(tv)
		{
		}

		TimeSpan(uint64_t microseconds = 0) : _TimeSpan(microseconds)
		{
		}

		TimeSpan(unsigned Hours, unsigned Minutes, unsigned Seconds, unsigned Milliseconds) :
			_TimeSpan(((((((Hours * 60LL) + Minutes) * 60LL) + Seconds) * 1000LL) * Milliseconds) * 1000LL)
		{
		}

		TimeSpan(unsigned Days, unsigned Hours, unsigned Minutes, unsigned Seconds, unsigned Milliseconds) :
			_TimeSpan( (((((((Days * 24LL) + Hours * 60LL) + Minutes) * 60LL) + Seconds) * 1000LL) * Milliseconds) * 1000LL)
		{
		}

		TimeSpan operator+(const TimeSpan &ts) const
		{
			return TimeSpan(m_TotalMicroseconds + ts.m_TotalMicroseconds);
		}

		TimeSpan operator-(const TimeSpan &ts) const
		{
			return TimeSpan(m_TotalMicroseconds - ts.m_TotalMicroseconds);
		}

		TimeSpan operator*(unsigned Count) const
		{
			return TimeSpan(m_TotalMicroseconds * Count);
		}

		TimeSpan operator/(unsigned Count) const
		{
			return TimeSpan(m_TotalMicroseconds / Count);
		}

		TimeSpan &operator+=(const TimeSpan &ts)
		{
			m_TotalMicroseconds += ts.m_TotalMicroseconds;
			return *this;
		}

		TimeSpan &operator-=(const TimeSpan &ts)
		{
			m_TotalMicroseconds -= ts.m_TotalMicroseconds;
			return *this;
		}

		bool operator<(const TimeSpan &right)
		{
			return m_TotalMicroseconds < right.m_TotalMicroseconds;
		}

		bool operator>(const TimeSpan &right)
		{
			return m_TotalMicroseconds > right.m_TotalMicroseconds;
		}

		bool operator==(const TimeSpan &right)
		{
			return m_TotalMicroseconds == right.m_TotalMicroseconds;
		}

		bool operator!=(const TimeSpan &right)
		{
			return m_TotalMicroseconds != right.m_TotalMicroseconds;
		}

		~TimeSpan()
		{
		}

		String ToString(bool IncludeMsec = false)
		{
			if (!IncludeMsec)
				return String::sFormat(_T("%02d:%02d:%02d"), (int)GetTotalHours(), GetMinutes(), GetSeconds());
			else
				return String::sFormat(_T("%02d:%02d:%02d.%03d"), (int)GetTotalHours(), GetMinutes(), GetSeconds(), (int)(GetTotalMilliseconds() % 1000));
		}
	};

	class DateTime : public Posix::_DateTime
	{
	public:
		DateTime(const timeval &tv) : _DateTime(tv) {}
#ifndef BAZISLIB_NO_STRUCTURED_TIME
		DateTime(const tm &t) : _DateTime(t) {}
#endif
		DateTime() {}

	public:

#ifndef BAZISLIB_NO_STRUCTURED_TIME
		//This method is declared constant as it does not change the time itself. It operates only the internal state.
		const tm *ToTM() const
		{
			if (!m_pCachedTM)
			{
				m_pCachedTM = (tm *)malloc(sizeof(tm));
				localtime_r(&m_TimeVal.tv_sec, m_pCachedTM);
			}
			return m_pCachedTM;
		}
#endif

	public:
		/*DateTime(DateTime &date, unsigned Hour, unsigned Minute, unsigned Second, unsigned Milliseconds) :
		  _DateTime(date)
		{
			//TODO: implement
		}*/

		~DateTime()
		{
		}

#ifndef BAZISLIB_NO_STRUCTURED_TIME

		//One-based indexes
		unsigned GetYear() const
		{
			return ToTM()->tm_year + 1900;
		}

		unsigned GetMonth() const
		{
			return ToTM()->tm_mon + 1;
		}

		unsigned GetDay() const
		{
			return ToTM()->tm_mday;
		}

		unsigned GetHour() const
		{
			return ToTM()->tm_hour;
		}

		unsigned GetMinute() const
		{
			return ToTM()->tm_min;
		}

		unsigned GetSecond() const
		{
			return ToTM()->tm_sec;
		}

		unsigned GetDayOfWeek() const //Zero-based index
		{
			return ToTM()->tm_wday;
		}	
#endif

		TimeSpan operator-(const DateTime& tm) const
		{
			return TimeSpan(m_TimeVal) - TimeSpan(tm.m_TimeVal);
		}

		DateTime operator-(const TimeSpan& ts) const
		{
			return DateTime((TimeSpan(m_TimeVal) - ts).ToTV());
		}

		DateTime &operator-=(const TimeSpan& ts)
		{
			m_TimeVal = ((TimeSpan(m_TimeVal) - ts).ToTV());
			if (m_pCachedTM)
			{
				free(m_pCachedTM);
				m_pCachedTM = NULL;
			}
			return *this;
		}

		DateTime operator+(const TimeSpan &ts) const
		{
			return DateTime((TimeSpan(m_TimeVal) + ts).ToTV());
		}

		DateTime &operator+=(const TimeSpan &ts)
		{
			m_TimeVal = ((TimeSpan(m_TimeVal) + ts).ToTV());
			if (m_pCachedTM)
			{
				free(m_pCachedTM);
				m_pCachedTM = NULL;
			}
			return *this;
		}

		bool operator<(const DateTime &tm) const
		{
			return TimeSpan(m_TimeVal) < TimeSpan(tm.m_TimeVal);
		}

		bool operator==(const DateTime &tm) const
		{
			return TimeSpan(m_TimeVal) == TimeSpan(tm.m_TimeVal);
		}

		bool operator>(const DateTime &tm) const
		{
			return TimeSpan(m_TimeVal) > TimeSpan(tm.m_TimeVal);
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
			timeval tv;
			gettimeofday(&tv, NULL);
			return DateTime(tv);
		}

/*		static DateTime Today()
		{
			timeval tv;
			gettimeofday(&tv, NULL);
			return DateTime(DateTime(tv), 0, 0, 0, 0);
		}*/

		//! Returns the number of milliseconds elapsed from this DateTime value
		uint64_t MillisecondsElapsed() const
		{
			return (Now() - *this).GetTotalMilliseconds();
		}

#ifndef BAZISLIB_NO_STRUCTURED_TIME
		String ToString()
		{
			return String::sFormat(_T("%02d.%02d.%04d %02d:%02d:%02d"), GetDay(), GetMonth(), GetYear(), GetHour(), GetMinute(), GetSecond());
		}
#endif
	};
}