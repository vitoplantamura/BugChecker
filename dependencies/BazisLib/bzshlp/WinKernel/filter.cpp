#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL
#include "filter.h"

using namespace BazisLib::DDK;

Filter::Filter(LPCWSTR pwszBaseDeviceName,
       bool bDeleteThisAfterRemoveRequest,
	   LPCWSTR pwszFilterDeviceName,
	   DEVICE_TYPE FilterDeviceType,
	   ULONG DeviceCharacteristics,
	   bool bExclusive,
	   ULONG AdditionalDeviceFlags) :
	PNPDevice(FilterDeviceType, bDeleteThisAfterRemoveRequest, DeviceCharacteristics, bExclusive, AdditionalDeviceFlags, pwszFilterDeviceName),
	m_BaseDeviceReference(pwszBaseDeviceName)
{
	if (!m_BaseDeviceReference.Valid())
		ReportInitializationError(STATUS_INVALID_DEVICE_OBJECT_PARAMETER);
}

Filter::~Filter()
{
}

NTSTATUS Filter::Register(Driver *pDriver, const GUID *pInterfaceGuid)
{
	if (!m_BaseDeviceReference.Valid())
		return STATUS_INVALID_DEVICE_OBJECT_PARAMETER;
	return __super::AddDevice(pDriver, m_BaseDeviceReference.GetDeviceObject(), pInterfaceGuid);
}
#endif