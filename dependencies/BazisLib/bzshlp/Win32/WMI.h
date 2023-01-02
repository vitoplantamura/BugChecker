#pragma once
#ifndef UNDER_CE

#include "cmndef.h"
#include "../../bzscore/status.h"
#include "../../bzscore/string.h"
#include <map>
#include <atlcomcli.h>
#include <Wbemidl.h>

namespace BazisLib
{
	namespace Win32
	{
		//! Provides classes for manipulating WMI objects
		/*! WMI (Windows Management Instrumentation) is designed to be used from scripting languages (such as VBScript). That way, it uses
			approach similar to IDispatch interface - instead of having compile-time interface information, WMI user should get all type information
			during runtime. Although this approach simplifies development of scripted WMI clients, it makes using WMI from C++ applications quite
			a mess.<br/>
			BazisLib simplifies usage of WMI by providing 2 classes: WMIServices and WMIObject. WMIServices instance represents a root services object,
			that is used to create or find instances of WMI classes. A WMIObject instance corresponds to a single WMI class or an object, allowing to
			call its methods in a relatively convenient way.
			<br/>
			As WMI calls are usually quite slow, BazisLib support for WMI was designed to provide maximum usage simplicity and convenience, making the
			execution speed a second concern. For example, calling a WMI object method with some string and integer parameters would result in creation
			of CComVariant objects for every parameter, however, this will be done fully automatically, and instead of writing tens of lines of code 
			querying method parameter info, instantiating parameter set and setting parameters, a single simple statement is sufficient: <pre>
WMIObject obj = ... //Create or find WMI class instance
CCOMVariant result;
obj.ExecMethod(L"SomeMethod", &result, L"SomeIntParam", 32, L"SomeLongParam", 456L, L"SomeStringParam", L"SomeString");</pre>
			See WMIObject class description for more details.
		*/
		namespace WMI
		{
			using namespace BazisLib;

			//! Represents a single WMI class or object
			/*! This class represents a single WMI class, or an object. To obtain a WMIObject representing a WMI object, use the WMIServices::FindInstances()
				or WMIServices::GetObject() methods. Once the object instance is obtained, you can check its validity by calling WMIObject::Valid() method, and
				perform standard WMI operations, as in this example: <pre>
void ProcessWMIObject(WMIObject &obj)
{
	if (!obj.Valid())
		return MAKE_STATUS(InvalidParameter);
	CCOMVariant result;
	obj[L"SomeIntProperty"] = 123;
	obj[L"SomeIUnknownProperty"] = CCOMVariant(pUnknown);
	DynamicStringW str = obj[L"SomeStringProperty"];
	obj.ExecMethod(L"SomeMethod", &result, L"SomeIntParam", 32, L"SomeLongParam", 456L, L"SomeStringParam", L"SomeString");
}</pre>	
				\remarks Note that you can call <bIWbemClassObject</b> method directly using the "->" operator of a WMIObject class.
			*/
			class WMIObject
			{
			protected:
				CComPtr<IWbemClassObject> m_pObj;
				CComPtr<IWbemServices> m_pServices;

			public:
				//! Allows calling IWbemClassObject methods directly
				IWbemClassObject *operator->()
				{
					return m_pObj;
				}

				//! Creates a WMIObject for a given IWbemClassObject instance
				WMIObject(const CComPtr<IWbemClassObject> &pObj, const CComPtr<IWbemServices> &pServices)
					: m_pObj(pObj)
					, m_pServices(pServices)
				{
				}

				//! Creates an empty (invalid) WMIObject
				WMIObject()
				{

				}

				//! Creates a WMIObject based on a VARIANT of VT_UNKNOWN type supporting IWbemClassObject interface
				WMIObject(const VARIANT *pInterface, const CComPtr<IWbemServices> &pServices);
				//! Creates a WMIObject based on a IUnknown pointer supporting IWbemClassObject interface
				WMIObject(IUnknown *pUnknown, const class WMIServices &pServices);
				//! Creates a WMIObject based on a IUnknown pointer supporting IWbemClassObject interface
				WMIObject(IUnknown *pUnknown, const CComPtr<IWbemServices> &pServices);

				//! Tests WMIObject instance for validity
				bool Valid() const
				{
					return m_pObj != NULL;
				}

				//! Gets the value of a WMI object property as CComVariant instance
				CComVariant GetPropertyVariant(LPCWSTR lpPropertyName, ActionStatus *pStatus = NULL) const;
				//! Gets the value of a WMI object property converted to string
				DynamicStringW GetPropertyString(LPCWSTR lpPropertyName, ActionStatus *pStatus = NULL) const;
				//! Gets the value of a WMI object property converted to an integer
				int GetPropertyInt(LPCWSTR lpPropertyName, ActionStatus *pStatus = NULL) const;

			public:
				//! Used to support expressions like 'obj[L"PropertyName"]'
				/*! This class is used internally by WMIObject::operator[]. Some usage examples can be found
					on the WMIObject reference page.
				*/
				class PropertyAccessor
				{
				public:
					IWbemClassObject *m_pObj;
					LPCWSTR m_lpPropertyName;

				private:
					PropertyAccessor(IWbemClassObject *pObj, LPCWSTR lpPropertyName)
						: m_pObj(pObj)
						, m_lpPropertyName(lpPropertyName)
					{
					}

					PropertyAccessor(const PropertyAccessor &accessor)
					{
						ASSERT(FALSE);
					}

					friend class WMIObject;

				public:
					operator CComVariant()
					{
						CComVariant variant;
						ASSERT(m_pObj);
						m_pObj->Get(m_lpPropertyName, 0, &variant, NULL, NULL);
						return variant;
					}

					operator DynamicStringW()
					{
						CComVariant variant;
						ASSERT(m_pObj);
						m_pObj->Get(m_lpPropertyName, 0, &variant, NULL, NULL);
						if (variant.vt == VT_NULL)
							return DynamicStringW();
						if (variant.vt != VT_BSTR)
							variant.ChangeType(VT_BSTR);
						return variant.bstrVal;
					}

					operator int()
					{
						CComVariant variant;
						ASSERT(m_pObj);
						m_pObj->Get(m_lpPropertyName, 0, &variant, NULL, NULL);
						if (variant.vt == VT_NULL)
							return 0;
						if (variant.vt != VT_INT)
							variant.ChangeType(VT_INT);
						return variant.intVal;
					}

					void operator=(const CComVariant &var)
					{
						ASSERT(m_pObj);
						m_pObj->Put(m_lpPropertyName, 0, &const_cast<CComVariant &>(var), var.vt);
					}

				};

			public:
				//! Allows using 'obj[L"PropertyName"]' syntax. See WMIObject reference.
				PropertyAccessor operator[](const LPCWSTR lpStr)
				{
					return PropertyAccessor(m_pObj, lpStr);
				}

			protected:
				//! Executes a method of a WMI object. Used by public overloads of ExecMethod().
				ActionStatus ExecMethod(LPCWSTR lpMethodName, CComVariant *pResult,
					CComVariant *pFirstOutParam,
					std::map<DynamicStringW, CComVariant> *pAllOutputParams,
					LPCWSTR pParam1Name, const CComVariant &Param1,
					LPCWSTR pParam2Name, const CComVariant &Param2,
					LPCWSTR pParam3Name, const CComVariant &Param3,
					LPCWSTR pParam4Name, const CComVariant &Param4,
					LPCWSTR pParam5Name, const CComVariant &Param5,
					LPCWSTR pParam6Name, const CComVariant &Param6,
					LPCWSTR pParam7Name, const CComVariant &Param7,
					LPCWSTR pParam8Name, const CComVariant &Param8,
					LPCWSTR pParam9Name, const CComVariant &Param9);

			public:
#pragma region ExecMethod() helpers
				//! Executes a WMI object method, that has no output parameters
				ActionStatus ExecMethod(LPCWSTR lpMethodName, CComVariant *pResult = NULL,
					LPCWSTR pParam1Name = NULL, const CComVariant &Param1 = *(CComVariant *)NULL,
					LPCWSTR pParam2Name = NULL, const CComVariant &Param2 = *(CComVariant *)NULL,
					LPCWSTR pParam3Name = NULL, const CComVariant &Param3 = *(CComVariant *)NULL,
					LPCWSTR pParam4Name = NULL, const CComVariant &Param4 = *(CComVariant *)NULL,
					LPCWSTR pParam5Name = NULL, const CComVariant &Param5 = *(CComVariant *)NULL,
					LPCWSTR pParam6Name = NULL, const CComVariant &Param6 = *(CComVariant *)NULL,
					LPCWSTR pParam7Name = NULL, const CComVariant &Param7 = *(CComVariant *)NULL,
					LPCWSTR pParam8Name = NULL, const CComVariant &Param8 = *(CComVariant *)NULL,
					LPCWSTR pParam9Name = NULL, const CComVariant &Param9 = *(CComVariant *)NULL)
				{
					return ExecMethod(lpMethodName, pResult, NULL, NULL,
						pParam1Name, Param1,
						pParam2Name, Param2,
						pParam3Name, Param3,
						pParam4Name, Param4,
						pParam5Name, Param5,
						pParam6Name, Param6,
						pParam7Name, Param7,
						pParam8Name, Param8,
						pParam9Name, Param9);
				}

				//! Executes a WMI object method, that has more than one output parameters
				ActionStatus ExecMethod(LPCWSTR lpMethodName, CComVariant *pResult,
					std::map<DynamicStringW, CComVariant> *pOutputParams,
					LPCWSTR pParam1Name = NULL, const CComVariant &Param1 = *(CComVariant *)NULL,
					LPCWSTR pParam2Name = NULL, const CComVariant &Param2 = *(CComVariant *)NULL,
					LPCWSTR pParam3Name = NULL, const CComVariant &Param3 = *(CComVariant *)NULL,
					LPCWSTR pParam4Name = NULL, const CComVariant &Param4 = *(CComVariant *)NULL,
					LPCWSTR pParam5Name = NULL, const CComVariant &Param5 = *(CComVariant *)NULL,
					LPCWSTR pParam6Name = NULL, const CComVariant &Param6 = *(CComVariant *)NULL,
					LPCWSTR pParam7Name = NULL, const CComVariant &Param7 = *(CComVariant *)NULL,
					LPCWSTR pParam8Name = NULL, const CComVariant &Param8 = *(CComVariant *)NULL,
					LPCWSTR pParam9Name = NULL, const CComVariant &Param9 = *(CComVariant *)NULL)
				{
					return ExecMethod(lpMethodName, pResult, NULL, pOutputParams,
						pParam1Name, Param1,
						pParam2Name, Param2,
						pParam3Name, Param3,
						pParam4Name, Param4,
						pParam5Name, Param5,
						pParam6Name, Param6,
						pParam7Name, Param7,
						pParam8Name, Param8,
						pParam9Name, Param9);
				}

				//! Executes a WMI object method, that has exactly one output parameter
				/*! 
					\remarks This method can be also used to call a WMI object method with more
							 than one output parameter. However, in such case, all output parameters
							 except for the first one, will be ignored.
				*/
				ActionStatus ExecMethod(LPCWSTR lpMethodName, CComVariant *pResult,
					CComVariant *pFirstOutParam,
					LPCWSTR pParam1Name = NULL, const CComVariant &Param1 = *(CComVariant *)NULL,
					LPCWSTR pParam2Name = NULL, const CComVariant &Param2 = *(CComVariant *)NULL,
					LPCWSTR pParam3Name = NULL, const CComVariant &Param3 = *(CComVariant *)NULL,
					LPCWSTR pParam4Name = NULL, const CComVariant &Param4 = *(CComVariant *)NULL,
					LPCWSTR pParam5Name = NULL, const CComVariant &Param5 = *(CComVariant *)NULL,
					LPCWSTR pParam6Name = NULL, const CComVariant &Param6 = *(CComVariant *)NULL,
					LPCWSTR pParam7Name = NULL, const CComVariant &Param7 = *(CComVariant *)NULL,
					LPCWSTR pParam8Name = NULL, const CComVariant &Param8 = *(CComVariant *)NULL,
					LPCWSTR pParam9Name = NULL, const CComVariant &Param9 = *(CComVariant *)NULL)
				{
					return ExecMethod(lpMethodName, pResult, pFirstOutParam, NULL,
						pParam1Name, Param1,
						pParam2Name, Param2,
						pParam3Name, Param3,
						pParam4Name, Param4,
						pParam5Name, Param5,
						pParam6Name, Param6,
						pParam7Name, Param7,
						pParam8Name, Param8,
						pParam9Name, Param9);
				}

				//! Executes a WMI object method, that has exactly one output parameter of a WMIObject type
				/*! 
					\remarks This method can be also used to call a WMI object method with more
							 than one output parameter. However, in such case, all output parameters
							 except for the first one, will be ignored.
				*/
				ActionStatus ExecMethod(LPCWSTR lpMethodName, CComVariant *pResult,
					WMIObject *pFirstOutParam,
					LPCWSTR pParam1Name = NULL, const CComVariant &Param1 = *(CComVariant *)NULL,
					LPCWSTR pParam2Name = NULL, const CComVariant &Param2 = *(CComVariant *)NULL,
					LPCWSTR pParam3Name = NULL, const CComVariant &Param3 = *(CComVariant *)NULL,
					LPCWSTR pParam4Name = NULL, const CComVariant &Param4 = *(CComVariant *)NULL,
					LPCWSTR pParam5Name = NULL, const CComVariant &Param5 = *(CComVariant *)NULL,
					LPCWSTR pParam6Name = NULL, const CComVariant &Param6 = *(CComVariant *)NULL,
					LPCWSTR pParam7Name = NULL, const CComVariant &Param7 = *(CComVariant *)NULL,
					LPCWSTR pParam8Name = NULL, const CComVariant &Param8 = *(CComVariant *)NULL,
					LPCWSTR pParam9Name = NULL, const CComVariant &Param9 = *(CComVariant *)NULL)
				{
					CComVariant var;
					ActionStatus st = ExecMethod(lpMethodName, pResult, &var, NULL,
						pParam1Name, Param1,
						pParam2Name, Param2,
						pParam3Name, Param3,
						pParam4Name, Param4,
						pParam5Name, Param5,
						pParam6Name, Param6,
						pParam7Name, Param7,
						pParam8Name, Param8,
						pParam9Name, Param9);
					if (!st.Successful() || !pFirstOutParam || (var.vt != VT_UNKNOWN) || !var.punkVal)
						return st;
					
					pFirstOutParam->~WMIObject();
					pFirstOutParam->WMIObject::WMIObject(&var, m_pServices);

					return MAKE_STATUS(Success);
				}

#pragma endregion

			};

			//! Represents a root WMI Services object
			class WMIServices
			{
			private:
				CComPtr<IWbemServices> m_pServices;
				friend class WMIObject;

			public:
				WMIServices(LPCWSTR lpResourcePath = L"ROOT\\CIMV2", BazisLib::ActionStatus *pStatus = NULL);

				//! Tests WMIServices for validity
				bool Valid()
				{
					return m_pServices != NULL;
				}

				//! Allows calling IWbemServices methods directly
				IWbemServices *operator->()
				{
					return m_pServices;
				}

			public:
				//! Allows iterating over a set of WMI objects
				class ObjectCollection
				{
				private:
					CComPtr<IEnumWbemClassObject> m_pStartEnum;
					CComPtr<IWbemServices> m_pServices;

				public:
					ObjectCollection(const CComPtr<IEnumWbemClassObject> &pEnum, const CComPtr<IWbemServices> &pServices)
						: m_pStartEnum(pEnum)
						, m_pServices(pServices)
					{
					}

					ObjectCollection()
					{
					}

					bool Valid()
					{
						return m_pStartEnum != NULL;
					}

				public:

					class iterator
					{
					private:
						CComPtr<IEnumWbemClassObject> m_pEnum;
						CComPtr<IWbemClassObject> m_pObject;
						CComPtr<IWbemServices> m_pServices;

					public:
						iterator(const CComPtr<IEnumWbemClassObject> &pStartEnum, const CComPtr<IWbemServices> &pServices)
							: m_pServices(pServices)
						{
							if (!pStartEnum)
								return;
							pStartEnum->Clone(&m_pEnum);
							if (m_pEnum)
							{
								ULONG done = 0;
								if (m_pEnum->Next(0, 1, &m_pObject, &done) != S_OK)
									m_pEnum = NULL;
							}
						}

						iterator(const iterator &iter)
						{
							if (iter.m_pEnum)
								iter.m_pEnum->Clone(&m_pEnum);
							m_pObject = iter.m_pObject;
						}

						iterator()
						{
						}

						void operator++()
						{
							if (m_pEnum)
							{
								m_pObject = NULL;
								ULONG done = 0;
								if (m_pEnum->Next(0, 1, &m_pObject, &done) != S_OK)
									m_pEnum = NULL;
							}
						}

						WMIObject operator*()
						{
							return WMIObject(m_pObject, m_pServices);
						}

						bool operator!=(const iterator &right)
						{
							if (!m_pEnum || !right.m_pEnum)
								return m_pEnum != right.m_pEnum;
							return m_pObject != right.m_pObject;
						}

					};


					iterator begin()	//To support C++ 'for each' operator
					{
						if (!Valid())
							return iterator();
						return iterator(m_pStartEnum, m_pServices);
					}

					iterator end()
					{
						return iterator();
					}

				};

			public:
				//! Returns a set of all instances of a given object.
				/*!
					\remarks Is it recommended to use the 'for each' C++ language extension when calling FindInstances(). Here
					is an example:
<pre>
WMIServices srv(L"root\\WMI");
for each (WMIObject obj in srv.FindInstances(L"Win32_LogicalDisk"))
{
	DynamicStringW str = obj[L"DeviceID"];

	ActionStatus st;
	DynamicStringW diskId = obj.GetPropertyString(L"DeviceID");

	CComVariant res;
	if (diskId == L"X:")
		st = obj.ExecMethod(L"Chkdsk", &res);
}
</pre>
				*/
				ObjectCollection FindInstances(LPCWSTR lpClassName, ActionStatus *pStatus = NULL, int Flags = 0, IWbemContext *pCtx = NULL);
				
				//! Returns a WMIObject representing a given WMI object, or a class
				WMIObject GetObject(LPCWSTR lpObjectName, ActionStatus *pStatus = NULL, int Flags = 0, IWbemContext *pCtx = NULL);

			};

		}
	}

}

#endif