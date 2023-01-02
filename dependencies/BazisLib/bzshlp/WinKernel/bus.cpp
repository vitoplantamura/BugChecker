#include "stdafx.h"
#ifdef BZSLIB_WINKERNEL

#include "bus.h"

#include "../../bzscore/sync.h"
#include "../../bzscore/string.h"
#include "devapi.h"
#include <vector>

//#define TRACK_PNP_REQUESTS

#ifdef TRACK_PNP_REQUESTS
PCHAR PnPMinorFunctionString (UCHAR MinorFunction);
#endif

using namespace BazisLib;
using namespace BazisLib::DDK;

BusDevice::BusDevice(String BusPrefix, 
					 bool bDeleteThisAfterRemoveRequest,
					 ULONG DeviceCharacteristics,
					 bool bExclusive,
					 ULONG AdditionalDeviceFlags) :
	IODevice(FILE_DEVICE_BUS_EXTENDER, bDeleteThisAfterRemoveRequest, DeviceCharacteristics, bExclusive, AdditionalDeviceFlags),
	m_BusPrefix(BusPrefix),
	m_NextDeviceNumber(0)
{
#ifdef _DEBUG
	m_pszDeviceDebugName = "BUS FDO";
#endif
	if (!m_ListAccessMutex.Valid())
		ReportInitializationError(STATUS_INSUFFICIENT_RESOURCES);
}

BusDevice::~BusDevice()
{
	std::list<BusDevice::PDOBase *> devicesToCleanup;
	{
		FastMutexLocker lck(m_ListAccessMutex);
		devicesToCleanup.swap(m_PhysicalDevObjects);
	}

	for (std::list<BusDevice::PDOBase *>::iterator it = devicesToCleanup.begin(); it != devicesToCleanup.end(); it++)
	{
		if (*it)
			delete *it;
	}
}

NTSTATUS BusDevice::AddPDO(PDOBase *pPDO, OUT LONG *pUniqueID)
{
	if (GetCurrentPNPState() != Started)
		return STATUS_INVALID_DEVICE_STATE;
	if (!pPDO)
		pPDO = new BusPDO();
	ASSERT(!pPDO->m_pBusDevice);
	pPDO->m_pBusDevice = this;
	if (!pPDO)
		return STATUS_INSUFFICIENT_RESOURCES;

	LONG uniqueID = InterlockedIncrement(&m_NextDeviceNumber);

	pPDO->m_UniqueID = uniqueID;
	pPDO->GenerateIDStrings(uniqueID);
	NTSTATUS st = pPDO->Register(GetDriver());
	if (!NT_SUCCESS(st))
	{
		delete pPDO;
		return st;
	}

	{
		FastMutexLocker lck(m_ListAccessMutex);
		m_PhysicalDevObjects.push_back(pPDO);
	}
	IoInvalidateDeviceRelations(GetUnderlyingPDO(), BusRelations);
	if (pUniqueID)
		*pUniqueID = uniqueID;
	return STATUS_SUCCESS;
}

BusDevice::PDOBase *BusDevice::FindPDOByUniqueID(LONG UniqueID)
{
	if (GetCurrentPNPState() != Started)
		return NULL;
	FastMutexLocker lck(m_ListAccessMutex);
	for (std::list<BusDevice::PDOBase *>::iterator it = m_PhysicalDevObjects.begin(); it != m_PhysicalDevObjects.end(); it++)
	{
		BusDevice::PDOBase *pPDO = *it;
		if (!pPDO)
			continue;
		if (pPDO->m_UniqueID == UniqueID)
			return pPDO;
	}
	return NULL;
}

NTSTATUS BusDevice::QueryParentPDOCapabilities(PDEVICE_CAPABILITIES Capabilities)
{
	OutgoingIRP irp(IRP_MJ_PNP, IRP_MN_QUERY_CAPABILITIES, GetNextDevice());
	if (!irp.Valid())
		return STATUS_UNEXPECTED_IO_ERROR;
	PIO_STACK_LOCATION IrpSp = irp.GetNextStackLocation();
	IrpSp->Parameters.DeviceCapabilities.Capabilities = Capabilities;
	return irp.Call();
}

NTSTATUS BusDevice::OnQueryDeviceRelations(DEVICE_RELATION_TYPE Type, PDEVICE_RELATIONS *ppDeviceRelations)
{
	if (!ppDeviceRelations)
		return STATUS_ACCESS_VIOLATION;

	if (Type != BusRelations)
		return STATUS_NOT_SUPPORTED;

	PDEVICE_RELATIONS pOldRelations = *ppDeviceRelations;
	PDEVICE_RELATIONS pNewRelations = NULL;

	unsigned prevCount = 0;
	if (pOldRelations)
	{
		prevCount = pOldRelations->Count;
		if (prevCount && !m_PhysicalDevObjects.size())
			return STATUS_SUCCESS;
	}
	
	{
		FastMutexLocker lck(m_ListAccessMutex);

		size_t newCount = prevCount + m_PhysicalDevObjects.size();

		pNewRelations = (PDEVICE_RELATIONS)
			ExAllocatePool(PagedPool,
			sizeof(DEVICE_RELATIONS) + ((newCount - 1) * sizeof (PDEVICE_OBJECT)));

		if (!pNewRelations)
			return STATUS_INSUFFICIENT_RESOURCES;
		
		if (prevCount)
			memcpy(pNewRelations->Objects, pOldRelations->Objects, prevCount * sizeof(PDEVICE_OBJECT));

		unsigned actualCount = prevCount;
		
		unsigned i = 0;

		for (std::list<BusDevice::PDOBase *>::iterator it = m_PhysicalDevObjects.begin(); it != m_PhysicalDevObjects.end(); it++, i++)
		{
			BusDevice::PDOBase *pPDO = *it;
			if (pPDO->m_bDevicePresent)
			{
				pNewRelations->Objects[actualCount] = pPDO->GetDeviceObjectForPDO();
				ObReferenceObject(pNewRelations->Objects[actualCount]);
				actualCount++;
			}
			else
				pPDO->m_bReportedMissing = true;
		}

		ASSERT(actualCount <= newCount);

		pNewRelations->Count = actualCount;
	}

	if (pOldRelations)
		ExFreePool(pOldRelations);
	
	*ppDeviceRelations = pNewRelations;
	return STATUS_SUCCESS;
}

NTSTATUS BusDevice::OnSurpriseRemoval()
{
	{
		FastMutexLocker lck(m_ListAccessMutex);
		for (std::list<BusDevice::PDOBase *>::iterator it = m_PhysicalDevObjects.begin(); it != m_PhysicalDevObjects.end(); it++)
			(*it)->m_bReportedMissing = true;
	}
	return __super::OnSurpriseRemoval();
}

NTSTATUS BusDevice::OnRemoveDevice(NTSTATUS LowerDeviceRemovalStatus)
{
	std::vector<BusDevice::PDOBase *> PDOsQueuedForDeletion;
	
	{
		FastMutexLocker lck(m_ListAccessMutex);
		PDOsQueuedForDeletion.reserve(m_PhysicalDevObjects.size());
		for (std::list<BusDevice::PDOBase *>::iterator it = m_PhysicalDevObjects.begin(); it != m_PhysicalDevObjects.end(); it++)
		{
			BusDevice::PDOBase *pPDO = *it;
			pPDO->m_bReportedMissing = true;

			if (pPDO->GetCurrentPNPStateForPDO() != SurpriseRemovePending)
			{
				//Deleting device objects can cause waiting for IRPs to complete. Doing that with a mutex being held could case deadlocks.
				PDOsQueuedForDeletion.push_back(pPDO);
				*it = NULL;
			}
		}
		m_PhysicalDevObjects.clear();
	}

	for each(BusDevice::PDOBase *pPDO in PDOsQueuedForDeletion)
		delete pPDO;
	return __super::OnRemoveDevice(LowerDeviceRemovalStatus);
}

//------------------------------------------------------------------------------------------------------------

BusDevice::PDOBase::PDOBase(const wchar_t *pwszDeviceType, const wchar_t *pwszDeviceName, const wchar_t *pwszLocation) :
	m_pBusDevice(NULL),

	m_bDevicePresent(true),
	m_bReportedMissing(false),
	m_UniqueID(0),

	m_DeviceType(pwszDeviceType),
	m_DeviceName(pwszDeviceName),
	m_DeviceLocation(pwszLocation)
{
#ifdef _DEBUG
	//m_pszDeviceDebugName = "PDO";
#endif
}

void BusDevice::PDOBase::GenerateIDStrings(int UniqueDeviceNumber)
{
	ASSERT(m_pBusDevice);
	wchar_t wsz[100];
	swprintf(wsz, L"%s\\%s", m_pBusDevice->m_BusPrefix.c_str(), m_DeviceType.c_str());
	m_DeviceID = wsz;
	swprintf(wsz, L"%s\\%s", m_pBusDevice->m_BusPrefix.c_str(), m_DeviceType.c_str());
	m_HardwareIDs = wsz;

	swprintf(wsz, L"%02d", UniqueDeviceNumber);
	m_InstanceID = wsz;
	swprintf(wsz, L"%s\\%s", m_pBusDevice->m_BusPrefix.c_str(), m_DeviceType.c_str());
	m_CompatibleIDs = wsz;
}

void BusDevice::PDOBase::sInterfaceReference(PVOID Context)
{
	if (!Context)
		return;
	((IUnknownDeviceInterface *)Context)->AddRef();
}

void BusDevice::PDOBase::sInterfaceDereference(PVOID Context)
{
	if (!Context)
		return;
	((IUnknownDeviceInterface *)Context)->Release();
}

BusDevice::PDOBase::~PDOBase()
{
}

NTSTATUS
  TestIoCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
	((KEvent *)Context)->Set();
	return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS BusDevice::PDOBase::Eject()
{
	IoRequestDeviceEject(GetDeviceObjectForPDO());
	return STATUS_SUCCESS;
}

extern "C" PDEVICE_OBJECT __stdcall
IoGetAttachedDevice(
					IN PDEVICE_OBJECT  DeviceObject
					); 

void BusDevice::PDOBase::PlugOut()
{
	m_bDevicePresent = false;
	ASSERT(m_pBusDevice);
	IoInvalidateDeviceRelations(m_pBusDevice->GetUnderlyingPDO(), BusRelations);
}


NTSTATUS BusDevice::PDOBase::OnQueryCapabilities(PDEVICE_CAPABILITIES Capabilities)
{
	ASSERT(m_pBusDevice);
	NTSTATUS st = m_pBusDevice->QueryParentPDOCapabilities(Capabilities);
	if (!NT_SUCCESS(st))
		return st;
	Capabilities->Removable = TRUE;
	Capabilities->EjectSupported = TRUE;
	Capabilities->UniqueID = FALSE;
	Capabilities->SurpriseRemovalOK = TRUE;

	Capabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;

	if(Capabilities->DeviceState[PowerSystemSleeping1] != PowerDeviceD0)
		Capabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
        
	if(Capabilities->DeviceState[PowerSystemSleeping2] != PowerDeviceD0)
		Capabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;

	if(Capabilities->DeviceState[PowerSystemSleeping3] != PowerDeviceD0)
		Capabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
        
    
	Capabilities->SystemWake = PowerSystemUnspecified;
	Capabilities->DeviceWake = PowerDeviceUnspecified;

	Capabilities->Address = m_UniqueID;
	Capabilities->UINumber = m_UniqueID;

	return STATUS_SUCCESS;
}

NTSTATUS BusDevice::PDOBase::OnQueryDeviceID(BUS_QUERY_ID_TYPE Type, PWCHAR *ppDeviceID)
{
	switch (Type)
	{
	case BusQueryDeviceID:
		*ppDeviceID = m_DeviceID.ExportToPagedPool();
		break;
	case BusQueryHardwareIDs:
		*ppDeviceID = m_HardwareIDs.ExportToPagedPool(1);
		break;
	case BusQueryInstanceID:
		*ppDeviceID = m_InstanceID.ExportToPagedPool();
		break;
	case BusQueryCompatibleIDs:
		*ppDeviceID = m_CompatibleIDs.ExportToPagedPool(1);
		break;
	default:
		return STATUS_NOT_SUPPORTED;
	}
	return STATUS_SUCCESS;
}

NTSTATUS BusDevice::PDOBase::OnQueryDeviceText(DEVICE_TEXT_TYPE Type, LCID Locale, PWCHAR *ppDeviceText)
{
	switch (Type)
	{
	case DeviceTextDescription:
		*ppDeviceText = m_DeviceName.ExportToPagedPool();
		return STATUS_SUCCESS;
	case DeviceTextLocationInformation:
		*ppDeviceText = m_DeviceLocation.ExportToPagedPool();
		return STATUS_SUCCESS;
	default:
		return STATUS_NOT_SUPPORTED;
	}
}

NTSTATUS BusDevice::PDOBase::OnQueryResources(PCM_RESOURCE_LIST *ppResourceList)
{
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS BusDevice::PDOBase::OnQueryResourceRequirements(PIO_RESOURCE_REQUIREMENTS_LIST *ppResourceRequirementsList)
{
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS BusDevice::PDOBase::OnQueryBusInformation(PPNP_BUS_INFORMATION *ppBusInformation)
{
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS BusDevice::PDOBase::OnEject()
{
	m_bDevicePresent = false;
	return STATUS_SUCCESS;
}

NTSTATUS BusDevice::PDOBase::OnQueryInterfaceRaw(CONST GUID *InterfaceType, USHORT Size, USHORT Version, PINTERFACE pInterface, PVOID InterfaceSpecificData)
{
	if (!InterfaceType || (Size != sizeof(INTERFACE)) || !pInterface)
		return STATUS_NOT_SUPPORTED;
	IUnknownDeviceInterface *pUnk = CreateObjectByInterfaceID(InterfaceType, Version, InterfaceSpecificData);
	if (!pUnk)
		return STATUS_NOT_SUPPORTED;
	pInterface->InterfaceReference = sInterfaceReference;
	pInterface->InterfaceDereference = sInterfaceDereference;
	pInterface->Context = pUnk;
	pInterface->Size = sizeof(INTERFACE);
	pInterface->Version = Version;
	return STATUS_SUCCESS;
}

NTSTATUS BusDevice::PDOBase::TryDispatchPNP(IN IncomingIrp *Irp, IO_STACK_LOCATION *IrpSp)
{
#ifdef TRACK_PNP_REQUESTS
	DbgPrint("Physical:   %s\n", PnPMinorFunctionString(IrpSp->MinorFunction));
#endif
	NTSTATUS status = Irp->GetStatus();
	switch (IrpSp->MinorFunction)
	{
	case IRP_MN_QUERY_CAPABILITIES:
		status = OnQueryCapabilities(IrpSp->Parameters.DeviceCapabilities.Capabilities);
		break;
	case IRP_MN_QUERY_ID:
		status = OnQueryDeviceID(IrpSp->Parameters.QueryId.IdType, (PWCHAR *)&Irp->GetIoStatus()->Information);
		break;
	case IRP_MN_QUERY_DEVICE_TEXT:
		status = OnQueryDeviceText(IrpSp->Parameters.QueryDeviceText.DeviceTextType, IrpSp->Parameters.QueryDeviceText.LocaleId, (PWCHAR *)&Irp->GetIoStatus()->Information);
		break;
	case IRP_MN_QUERY_RESOURCES:
		status = OnQueryResources((PCM_RESOURCE_LIST *)&Irp->GetIoStatus()->Information);
		break;
	case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
		status = OnQueryResourceRequirements((PIO_RESOURCE_REQUIREMENTS_LIST *)&Irp->GetIoStatus()->Information);
		break;
	case IRP_MN_QUERY_BUS_INFORMATION:
		status = OnQueryBusInformation((PPNP_BUS_INFORMATION *)&Irp->GetIoStatus()->Information);
		break;
	case IRP_MN_EJECT:
		status = OnEject();
		break;
	case IRP_MN_QUERY_INTERFACE:
		status = OnQueryInterfaceRaw(IrpSp->Parameters.QueryInterface.InterfaceType,
								  IrpSp->Parameters.QueryInterface.Size,
								  IrpSp->Parameters.QueryInterface.Version,
								  IrpSp->Parameters.QueryInterface.Interface,
								  IrpSp->Parameters.QueryInterface.InterfaceSpecificData);
		break;
	case IRP_MN_QUERY_DEVICE_RELATIONS:
		status = OnQueryDeviceRelations(IrpSp->Parameters.QueryDeviceRelations.Type, (PDEVICE_RELATIONS *)&Irp->GetIoStatus()->Information);
		break;
	default:
		status = STATUS_NOT_SUPPORTED;
		break;
	}
	ASSERT(status != STATUS_PENDING);

	//See PNPDevice::DispatchPNP() for explanation about STATUS_NOT_SUPPORTED
	if (status != STATUS_NOT_SUPPORTED)
		Irp->SetIoStatus(status);

	return status;
}

NTSTATUS BusDevice::PDOBase::OnQueryDeviceRelations(DEVICE_RELATION_TYPE Type, PDEVICE_RELATIONS *ppDeviceRelations)
{
	if (Type != TargetDeviceRelation)
		return STATUS_NOT_SUPPORTED;
	*ppDeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));

	if (!*ppDeviceRelations)
		return STATUS_INSUFFICIENT_RESOURCES;

    (*ppDeviceRelations)->Count = 1;
    (*ppDeviceRelations)->Objects[0] = GetDeviceObjectForPDO();
    ObReferenceObject(GetDeviceObjectForPDO());
	return STATUS_SUCCESS;
}

#ifdef TRACK_PNP_REQUESTS
PCHAR PnPMinorFunctionString (UCHAR MinorFunction)
{
    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL";
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            return "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";           
        default:
            return "unknown_pnp_irp";
    }
}

#endif

void BazisLib::DDK::BusDevice::PDOBase::RequestBusToPlugOut()
{
	ASSERT(m_pBusDevice);
	m_pBusDevice->OnPDORequestedPlugout(this);
}

void BazisLib::DDK::BusDevice::OnPDORequestedPlugout( PDOBase *pPDO )
{
	ASSERT(pPDO);
	pPDO->PlugOut();
}
#endif