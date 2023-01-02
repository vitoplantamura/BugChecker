#pragma once

#include <BazisLib/bzscore/datetime.h>
#include <BazisLib/bzscore/sync.h>
#include <BazisLib/bzshlp/filters.h>

namespace BazisLib
{
	//! Performs transfer rate calculation
	/*! The RateCalculator class can be used for realtime transfer rate calculation.
		It requires the number of bytes transferred at a certain point of time. All time
		evaluation and calculation is done automatically.
		\remarks The estimated remaining time prediction algorithm uses ExponentialFilterI32 class
					to provide more smooth results. To alter the smoothing depth, alter the
					EstimatedTimeRateFilteringCoef constant.
	*/
	class RateCalculator
	{
	private:
		unsigned m_AveragingTime;
		ULONGLONG m_DownloadedInLastPeriod, m_DownloadedTotal;
			
		unsigned m_CachedValue;
		DateTime m_StartTime;
			
		Mutex m_Lock;
			
		ExponentialFilterI64 m_RateFilter;

		enum {EstimatedTimeRateFilteringCoef = 100};

	protected:
		void UpdateAverage();

	public:
		//! Creates the sliding average rate calculator
		/*!
			\param AveragingTime Specifies the averaging time in milliseconds
					-1 stands for overall rate calculation.
		*/
		RateCalculator(unsigned AveragingTime = -1);

		void ReportNewAbsoluteDone(ULONGLONG Done);
		void ReportNewRelativeDone(ULONGLONG Done);

		ULONGLONG GetDownloadedTotal() {return m_DownloadedTotal;}

		void SetAveragingTime(unsigned NewTime)
		{
			m_AveragingTime = NewTime;
		}

		void Reset(ULONGLONG InitialValue = 0)
		{
			ReportNewAbsoluteDone(InitialValue);
			m_CachedValue = 0;
			m_StartTime = DateTime::Now();
		}

		TimeSpan GetExpectedRemainingTime(ULONGLONG TotalDownloadSize);

		unsigned GetBytesPerSecond();

		static String FormatByteCount(ULONGLONG value)
		{
			TCHAR *pfx = _T("");
			if (value > (10LL * 1024 * 1024 * 1024))
				value /= (1024 * 1024 * 1024), pfx = _T("G");
			if (value > (10 * 1024 * 1024))
				value /= (1024 * 1024), pfx = _T("M");
			else if (value > (10 * 1024))
				value /= 1024, pfx = _T("K");
			String result;
			result.Format(_T("%d%s"), (unsigned)value, pfx);
			return result;
		}

		static String FormatTimeSpan(const TimeSpan &span)
		{
			if (!span.GetTotal100NanosecondIntervals())
				return _T("---");
			String result;
			result.Format(_T("%d:%02d:%02d"),
						 (unsigned)span.GetTotalHours(),
						 span.GetMinutes(),
						 span.GetSeconds());
			return result;
		}
	};
}