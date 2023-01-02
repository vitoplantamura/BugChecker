#include "stdafx.h"

#if !defined(BZSLIB_WINKERNEL)
#include "ratecalc.h"

using namespace BazisLib;

RateCalculator::RateCalculator(unsigned AveragingTime) :
	m_AveragingTime(AveragingTime),
	m_DownloadedInLastPeriod(0),
	m_DownloadedTotal(0),
	m_CachedValue(0),
	m_RateFilter(EstimatedTimeRateFilteringCoef)
{
}

void RateCalculator::UpdateAverage()
{
	DateTime tm = DateTime::Now();
	ULONGLONG msec = (tm - m_StartTime).GetTotalMilliseconds();
	if (!msec)
		return;
	if (m_AveragingTime == -1)
		m_CachedValue = (unsigned)((m_DownloadedInLastPeriod * 1000) / msec);
	else if (msec >= m_AveragingTime)
	{
		m_CachedValue = (unsigned)((m_DownloadedInLastPeriod * 1000) / msec);
		m_DownloadedInLastPeriod = 0;
		m_StartTime = tm;
	}
}

void RateCalculator::ReportNewAbsoluteDone(ULONGLONG Done)
{
	if (!m_DownloadedInLastPeriod)
		m_StartTime = DateTime::Now();
	MutexLocker lck(m_Lock);
	m_DownloadedInLastPeriod = Done;
	m_DownloadedTotal = Done;
	UpdateAverage();
}

void RateCalculator::ReportNewRelativeDone(ULONGLONG Done)
{
	if (!m_DownloadedInLastPeriod)
		m_StartTime = DateTime::Now();
	MutexLocker lck(m_Lock);
	m_DownloadedInLastPeriod += Done;
	m_DownloadedTotal += Done;
	UpdateAverage();
}

unsigned RateCalculator::GetBytesPerSecond()
{
	return m_CachedValue;
}

TimeSpan RateCalculator::GetExpectedRemainingTime(ULONGLONG TotalDownloadSize)
{
	unsigned rate = m_CachedValue;
	if ((m_AveragingTime == -1) && ((DateTime::Now() - m_StartTime).GetTotalSeconds() < 5))
		return TimeSpan();
	if (!rate)
		return TimeSpan();
	rate = (unsigned)m_RateFilter.UpdateValue(rate);
	LONGLONG left = TotalDownloadSize - m_DownloadedTotal;
	if (left <= 0)
		return TimeSpan();
	return TimeSpan((10000000LL * left) / rate);
}

#endif