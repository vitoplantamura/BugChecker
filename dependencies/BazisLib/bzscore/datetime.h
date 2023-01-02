#pragma once
#include "cmndef.h"

#ifdef BZS_POSIX
#include "Posix/datetime.h"
#elif defined (BZSLIB_KEXT)
#include "KEXT/datetime.h"
#elif defined (BZSLIB_WINKERNEL)
#include "WinKernel/datetime.h"
#elif defined (_WIN32)
#include "Win32/datetime.h"
#else
#error Date/time API is not defined for current platform.
#endif

namespace BazisLib
{
	using BazisLib::TimeSpan;
	using BazisLib::DateTime;

//Use the following layout when implementing TimeSpan/DateTime classes for your platform
/*
	class TimeSpan
	{
	public:
		ULONGLONG GetTotalDays() const;
		ULONGLONG GetTotalHours() const;
		ULONGLONG GetTotalMinutes() const;
		ULONGLONG GetTotalSeconds() const;
		ULONGLONG GetTotalMilliseconds() const;
		ULONGLONG GetTotal100NanosecondIntervals() const;

		TimeSpan(ULONGLONG Total100NanosecondIntervals = 0);
		TimeSpan(unsigned Hours, unsigned Minutes, unsigned Seconds, unsigned Milliseconds);
		TimeSpan(unsigned Days, unsigned Hours, unsigned Minutes, unsigned Seconds, unsigned Milliseconds);
		~TimeSpan();

		TimeSpan operator+(const TimeSpan &ts) const;
		TimeSpan operator-(const TimeSpan &ts) const;
		TimeSpan operator*(unsigned Count) const;
		TimeSpan operator/(unsigned Count) const;

		TimeSpan &operator+=(const TimeSpan &ts);
		TimeSpan &operator-=(const TimeSpan &ts);
};

	class DateTime
	{
	public:
		DateTime(DateTime &date, unsigned Hour, unsigned Minute, unsigned Second, unsigned Milliseconds);
		~DateTime();

		//One-based indexes
		unsigned GetYear() const;
		unsigned GetMonth() const;
		unsigned GetDay() const;
		unsigned GetHour() const;
		unsigned GetMinute() const;
		unsigned GetSecond() const;

		unsigned GetDayOfWeek() const;	//Zero-based index

		TimeSpan operator-(const DateTime& tm) const;
		DateTime operator-(const TimeSpan& ts) const;
		DateTime &operator-=(const TimeSpan& ts);

		DateTime operator+(const TimeSpan &ts) const;
		DateTime &operator+=(const TimeSpan &ts);

		bool operator<(const DateTime &tm) const;
		bool operator==(const DateTime &tm) const;
		bool operator>(const DateTime &tm) const;

	public:
		//The following operators may work slow!
		void SetYear(unsigned NewValue);
		void SetMonth(unsigned NewValue);
		void SetDay(unsigned NewValue);
		void SetHour(unsigned NewValue);
		void SetMinute(unsigned NewValue);
		void SetSecond(unsigned NewValue);

	public:
		static DateTime Now();
		static DateTime Today();
	};
*/
}