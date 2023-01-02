#include "stdafx.h"

#if defined(WIN32) && !defined(BZSLIB_WINKERNEL)
#include "BCD.h"

BazisLib::Win32::BCD::BCDStore BazisLib::Win32::BCD::BCDStore::OpenStore( LPCWSTR lpStoreName /*= L""*/, ActionStatus *pStatus /*= NULL*/ )
{
	WMIServices services(L"root\\WMI", pStatus);
	if (!services.Valid())
		return BCDStore();
	WMIObject storeClass = services.GetObject(L"BCDStore", pStatus);
	if (!storeClass.Valid())
		return BCDStore();
	CComVariant res, objStore;
	ActionStatus st = storeClass.ExecMethod(L"OpenStore", &res, &objStore, L"File", L"");
	if (!st.Successful())
	{
		if (pStatus)
			*pStatus = st;
		return BCDStore();
	}
	if ((objStore.vt != VT_UNKNOWN) || !objStore.punkVal)
	{
		ASSIGN_STATUS(pStatus, UnknownError);
		return BCDStore();
	}
	ASSIGN_STATUS(pStatus, Success);
	return BCDStore(objStore.punkVal, services);
}

BazisLib::ActionStatus BazisLib::Win32::BCD::BCDStore::ProvideBcdObjects()
{
	if (!m_BcdObjects.empty())
		return MAKE_STATUS(Success);

	CComVariant res, objects;
	ActionStatus st = ExecMethod(L"EnumerateObjects", &res, &objects, L"Type", CComVariant(0, VT_I4));
	if (!st.Successful())
		return COPY_STATUS(st);

	if (objects.vt != (VT_ARRAY | VT_UNKNOWN))
		return MAKE_STATUS(UnknownError);

	m_BcdObjects.clear();

	size_t nElements = objects.parray->rgsabound[0].cElements;

	m_BcdObjects.reserve(nElements);

	for (ULONG i = 0; i < nElements; i++)
	{
		IUnknown *pUnknown = ((IUnknown **)objects.parray->pvData)[i];
		CComPtr<IWbemClassObject> pBcdObject;

		BCDObject obj(pUnknown, m_pServices);
		if (!obj.Valid())
			return MAKE_STATUS(UnknownError);

		m_BcdObjects.push_back(obj);
	}

	return MAKE_STATUS(Success);
}

BazisLib::Win32::BCD::BCDObject BazisLib::Win32::BCD::BCDStore::CopyObject( const BCDObject &Obj, ActionStatus *pStatus )
{
	if (!Obj.Valid())
	{
		ASSIGN_STATUS(pStatus, InvalidParameter);
		return BCDObject();
	}
	DynamicStringW objID = Obj.GetID();
	if (objID.empty())
	{
		ASSIGN_STATUS(pStatus, InvalidParameter);
		return BCDObject();
	}

	CComVariant res, newobj;
	ActionStatus st = ExecMethod(L"CopyObject", &res, &newobj, L"SourceStoreFile", L"", L"SourceId", objID.c_str(), L"Flags", 1);
	if (!st.Successful())
	{
		if (pStatus) *pStatus = st;
		return BCDObject();
	}
	if ((newobj.vt != VT_UNKNOWN) || !newobj.punkVal)
	{
		ASSIGN_STATUS(pStatus, UnknownError);
		return BCDObject();
	}
	ASSIGN_STATUS(pStatus, Success);
	return BCDObject(newobj.punkVal, m_pServices);
}

BazisLib::Win32::BCD::BCDElement BazisLib::Win32::BCD::BCDObject::GetElement( BCDElementType type )
{
	if (!Valid())
		return BCDElement();

	CComVariant res, bcdElement;
	ActionStatus st = ExecMethod(L"GetElement", &res, &bcdElement, L"Type", (long)type);
	if (!st.Successful())
		return BCDElement();

	if ((bcdElement.vt != VT_UNKNOWN) || !bcdElement.punkVal)
		return BCDElement();

	return BCDElement(bcdElement.punkVal, m_pServices, *this, type);
}

BazisLib::ActionStatus BazisLib::Win32::BCD::BCDObject::SetElementHelper( BCDElementType type, LPCWSTR pFunctionName, LPCWSTR pParamName, const CComVariant &Value )
{
	ASSERT(pFunctionName && pParamName);
	if (!Valid())
		return MAKE_STATUS(NotInitialized);

	CComVariant res;
	ActionStatus st = ExecMethod(pFunctionName, &res, L"Type", (long)type, pParamName, Value);
	if (!st.Successful())
		return st;

	if ((res.vt != VT_BOOL) && (res.boolVal != TRUE))
		return MAKE_STATUS(UnknownError);

	return MAKE_STATUS(Success);
}

BazisLib::ActionStatus BazisLib::Win32::BCD::BCDObjectList::ApplyChanges()
{
	if (!Valid())
		return MAKE_STATUS(NotInitialized);
	
	SAFEARRAY *pArray = SafeArrayCreateVector(VT_BSTR, 0, (ULONG)m_Ids.size());
	if (!pArray)
		return MAKE_STATUS(UnknownError);

	for (size_t i = 0; i < m_Ids.size(); i++)
		((BSTR *)pArray->pvData)[i] = SysAllocString(m_Ids[i].c_str());

	CComVariant objList(pArray);
	SafeArrayDestroy(pArray);
	
	return m_Element.SetElementHelper(L"SetObjectListElement", L"Ids", objList);
}

#endif
