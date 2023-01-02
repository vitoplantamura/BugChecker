#include "stdafx.h"
#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "WMI.h"

#ifndef UNDER_CE

#pragma comment(lib, "Wbemuuid")

BazisLib::Win32::WMI::WMIServices::WMIServices( LPCWSTR lpResourcePath /*= L"ROOT\\CIMV2"*/, BazisLib::ActionStatus *pStatus /*= NULL*/ )
{
	HRESULT hR;
	const bool InitializeSecurity = true;
	if (InitializeSecurity)
	{
		hR = CoInitializeSecurity(NULL, 
			-1,                          // COM authentication
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities 
			NULL                         // Reserved
			);
		if (!SUCCEEDED(hR) && (hR != RPC_E_TOO_LATE))
		{
			ASSIGN_STATUS(pStatus, BazisLib::ActionStatus::FromHResult(hR));
			return;
		}
	}
	CComPtr<IWbemLocator> pLocator;
	hR = pLocator.CoCreateInstance(CLSID_WbemLocator);
	if (!SUCCEEDED(hR))
	{
		ASSIGN_STATUS(pStatus, BazisLib::ActionStatus::FromHResult(hR));
		return;
	}

	hR = pLocator->ConnectServer(CComBSTR(lpResourcePath), NULL, NULL, NULL, 0, NULL, NULL, &m_pServices);
	if (!SUCCEEDED(hR))
	{
		ASSIGN_STATUS(pStatus, BazisLib::ActionStatus::FromHResult(hR));
		return;
	}

	hR = CoSetProxyBlanket(m_pServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	if (!SUCCEEDED(hR))
	{
		ASSIGN_STATUS(pStatus, BazisLib::ActionStatus::FromHResult(hR));
		return;
	}

	ASSIGN_STATUS(pStatus, BazisLib::Success);
}

BazisLib::Win32::WMI::WMIObject BazisLib::Win32::WMI::WMIServices::GetObject( LPCWSTR lpObjectName, ActionStatus *pStatus /*= NULL*/, int Flags /*= 0*/, IWbemContext *pCtx /*= NULL*/ )
{
	if (!m_pServices)
	{
		ASSIGN_STATUS(pStatus, NotInitialized);
		return WMIObject();
	}
	ASSIGN_STATUS(pStatus, Success);

	CComPtr<IWbemClassObject> pObj;

	HRESULT hR = m_pServices->GetObject(CComBSTR(lpObjectName), Flags, pCtx, &pObj, NULL);
	if (!SUCCEEDED(hR))
	{
		ASSIGN_STATUS(pStatus, ActionStatus::FromHResult(hR));
		return WMIObject();
	}

	return WMIObject(pObj, m_pServices);
}

BazisLib::Win32::WMI::WMIServices::ObjectCollection BazisLib::Win32::WMI::WMIServices::FindInstances( LPCWSTR lpClassName, ActionStatus *pStatus /*= NULL*/, int Flags /*= 0*/, IWbemContext *pCtx /*= NULL*/ )
{
	if (!m_pServices)
	{
		ASSIGN_STATUS(pStatus, NotInitialized);
		return ObjectCollection();
	}
	CComPtr<IEnumWbemClassObject> pEnum;
	HRESULT hR = m_pServices->CreateInstanceEnum(CComBSTR(lpClassName), Flags, pCtx, &pEnum);
	if (!SUCCEEDED(hR))
	{
		ASSIGN_STATUS(pStatus, ActionStatus::FromHResult(hR));
		return ObjectCollection();
	}
	ASSIGN_STATUS(pStatus, Success);
	return ObjectCollection(pEnum, m_pServices);
}


BazisLib::ActionStatus BazisLib::Win32::WMI::WMIObject::ExecMethod( LPCWSTR lpMethodName, CComVariant *pResult, CComVariant *pFirstOutParam, std::map<DynamicStringW, CComVariant> *pAllOutputParams,
																   LPCWSTR pParam1Name, const CComVariant &Param1,
																   LPCWSTR pParam2Name, const CComVariant &Param2,
																   LPCWSTR pParam3Name, const CComVariant &Param3,
																   LPCWSTR pParam4Name, const CComVariant &Param4,
																   LPCWSTR pParam5Name, const CComVariant &Param5,
																   LPCWSTR pParam6Name, const CComVariant &Param6,
																   LPCWSTR pParam7Name, const CComVariant &Param7,
																   LPCWSTR pParam8Name, const CComVariant &Param8,
																   LPCWSTR pParam9Name, const CComVariant &Param9)

{
	if (!m_pServices || !m_pObj)
		return MAKE_STATUS(NotInitialized);
	CComPtr<IWbemClassObject> pClassDefinition;

	ActionStatus st;
	DynamicStringW tmpProp = GetPropertyString(L"__CLASS", &st);
	if (!st.Successful())
		return st;


	HRESULT hR = m_pServices->GetObject(CComBSTR(tmpProp.c_str()), 0, NULL, &pClassDefinition, NULL);
	if (!SUCCEEDED(hR))
		return MAKE_STATUS(ActionStatus::FromHResult(hR));

	CComPtr<IWbemClassObject> pInParamsClass, pInParamsInstance, pOutParamsInstance;

	hR = pClassDefinition->GetMethod(lpMethodName, 0, &pInParamsClass, NULL);
	if (!SUCCEEDED(hR))
		return MAKE_STATUS(ActionStatus::FromHResult(hR));

	if (pInParamsClass)
	{
		hR = pInParamsClass->SpawnInstance(0, &pInParamsInstance);

		if (!SUCCEEDED(hR))
			return MAKE_STATUS(ActionStatus::FromHResult(hR));


#define PARAM_SET_HELPER(num) \
	if (pParam ## num ## Name && &Param ## num)								\
		{																		\
			hR = pInParamsInstance->Put(pParam ## num ## Name, 0, &const_cast<CComVariant &>(Param ## num), Param ## num.vt);	\
			if (hR == WBEM_E_TYPE_MISMATCH)										\
			{																	\
				CIMTYPE type = VT_NULL;											\
				pInParamsInstance->Get(pParam ## num ## Name, 0, NULL, &type, NULL); \
				hR = pInParamsInstance->Put(pParam ## num ## Name, 0, &const_cast<CComVariant &>(Param ## num), type);	\
				if (hR == WBEM_E_TYPE_MISMATCH)									\
				{																\
					CComVariant typeBugWorkaroundTmp = Param ## num;			\
					typeBugWorkaroundTmp.ChangeType(VT_BSTR);					\
					hR = pInParamsInstance->Put(pParam ## num ## Name, 0, &typeBugWorkaroundTmp, type);	\
				}																\
			}																	\
			if (!SUCCEEDED(hR))													\
				return MAKE_STATUS(ActionStatus::FromHResult(hR));				\
		}

		PARAM_SET_HELPER(1);
		PARAM_SET_HELPER(2);
		PARAM_SET_HELPER(3);
		PARAM_SET_HELPER(4);
		PARAM_SET_HELPER(5);
		PARAM_SET_HELPER(6);
		PARAM_SET_HELPER(7);
		PARAM_SET_HELPER(8);
		PARAM_SET_HELPER(9);
	}

	tmpProp = GetPropertyString(L"__RELPATH", &st);
	if (!st.Successful())
		return st;

	hR = m_pServices->ExecMethod(CComBSTR(tmpProp.c_str()), CComBSTR(lpMethodName), 0, NULL, pInParamsInstance, &pOutParamsInstance, NULL);

	if (!SUCCEEDED(hR))
		return MAKE_STATUS(ActionStatus::FromHResult(hR));

	if (pResult && pOutParamsInstance)
		hR = pOutParamsInstance->Get(L"ReturnValue", 0, pResult, NULL, NULL);

	if ((pAllOutputParams || pFirstOutParam) && pOutParamsInstance)
	{
		hR = pOutParamsInstance->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
		if (!SUCCEEDED(hR))
			return MAKE_STATUS(ActionStatus::FromHResult(hR));

		CComBSTR name;
		CComVariant val;
		for (;;)
		{
			name.AssignBSTR(NULL);
			hR = pOutParamsInstance->Next(0, &name, &val, NULL, NULL);
			if (!name)
				break;

			if (name == L"ReturnValue")
				continue;

			if (pAllOutputParams)
				(*pAllOutputParams)[DynamicStringW(name)] = val;
			else if (pFirstOutParam)
			{
				*pFirstOutParam = val;
				break;
			}

		}
	}


	return MAKE_STATUS(Success);
}


BazisLib::DynamicStringW BazisLib::Win32::WMI::WMIObject::GetPropertyString( LPCWSTR lpPropertyName, ActionStatus *pStatus /*= NULL*/ ) const
{
	if (!m_pObj)
	{
		ASSIGN_STATUS(pStatus, BazisLib::NotInitialized);
		return DynamicStringW();
	}
	CComVariant val;
	HRESULT hR = m_pObj->Get(lpPropertyName, 0, &val, NULL, NULL);
	if (!SUCCEEDED(hR))
	{
		ASSIGN_STATUS(pStatus, BazisLib::ActionStatus::FromHResult(hR));
		return DynamicStringW();
	}
	ASSIGN_STATUS(pStatus, Success);
	if ((val.vt == VT_LPWSTR) || (val.vt == VT_BSTR))
		return DynamicStringW(val.bstrVal);
	val.ChangeType(VT_BSTR);
	return DynamicStringW(val.bstrVal);
}

ATL::CComVariant BazisLib::Win32::WMI::WMIObject::GetPropertyVariant( LPCWSTR lpPropertyName, ActionStatus *pStatus /*= NULL*/ ) const
{
	CComVariant val;
	if (!m_pObj)
	{
		ASSIGN_STATUS(pStatus, BazisLib::NotInitialized);
		return val;
	}
	HRESULT hR = m_pObj->Get(lpPropertyName, 0, &val, NULL, NULL);
	if (!SUCCEEDED(hR))
	{
		ASSIGN_STATUS(pStatus, BazisLib::ActionStatus::FromHResult(hR));
		return val;
	}
	ASSIGN_STATUS(pStatus, Success);
	return val;
}


int BazisLib::Win32::WMI::WMIObject::GetPropertyInt( LPCWSTR lpPropertyName, ActionStatus *pStatus /*= NULL*/ ) const
{
	if (!m_pObj)
	{
		ASSIGN_STATUS(pStatus, BazisLib::NotInitialized);
		return 0;
	}
	CComVariant val;
	HRESULT hR = m_pObj->Get(lpPropertyName, 0, &val, NULL, NULL);
	if (!SUCCEEDED(hR))
	{
		ASSIGN_STATUS(pStatus, BazisLib::ActionStatus::FromHResult(hR));
		return 0;
	}
	ASSIGN_STATUS(pStatus, Success);
	if (val.vt == VT_INT)
		return val.intVal;
	val.ChangeType(VT_INT);
	return val.intVal;
}


BazisLib::Win32::WMI::WMIObject::WMIObject( const VARIANT *pInterface, const CComPtr<IWbemServices> &pServices )
{
	if (!pInterface || (pInterface->vt != VT_UNKNOWN))
		return;
	pInterface->punkVal->QueryInterface(&m_pObj);
	m_pServices = pServices;
}

BazisLib::Win32::WMI::WMIObject::WMIObject( IUnknown *pUnknown, const WMIServices &pServices )
{
	if (!pUnknown)
		return;
	pUnknown->QueryInterface(&m_pObj);
	m_pServices = pServices.m_pServices;
}

BazisLib::Win32::WMI::WMIObject::WMIObject( IUnknown *pUnknown, const CComPtr<IWbemServices> &pServices )
{
	if (!pUnknown)
		return;
	pUnknown->QueryInterface(&m_pObj);
	m_pServices = pServices;
}

#endif
#endif