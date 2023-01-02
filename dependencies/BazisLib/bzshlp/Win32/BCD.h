#pragma once

#ifndef UNDER_CE

#include "cmndef.h"
#include <vector>
#include "WMI.h"

namespace BazisLib
{
	namespace Win32
	{
		//! Allows editing Boot Configuration Data (BCD) entries used by Windows Vista and later
		/*! The Boot Configuration Data replaces boot.ini on Windows Vista and later. However, the
			interface for modifying BCD entries is based on WMI and is extremely inconvenient to
			be used by C++ programs.<br/>
			BazisLib provides a more convenient way of dealing with BCD. The BCD store itself is
			represented by the BCDStore object. This class can be used to read or modify global
			variables, such as default operating system, or the OS selection dialog timeout.
			Each OS is represented by a BCDObject class. Every object has several properties
			(such as boot device, or entry name in the OS selection dialog), that are represented
			by BCDElement instances.<br/>
			Here is a short example listing all entries in a BCD store:<pre>
BCDStore store = BCDStore::OpenStore();
for each(BCDObject obj in store.GetObjects())
{
	printf("%08X %S %S\n", obj.GetType(), obj.GetID().c_str(), 
		obj.GetElement(BCDObject::BcdLibraryString_Description).ToString().c_str());
}</pre>
			\attention Note that the BazisLib BCD classes cover most, but not all BCD functionality. However,
					   the extensible architecture allows wrapping new BCD calls quite easily using underlying
					   WMIObject methods.
		*/
		namespace BCD
		{
			using namespace BazisLib;
			using namespace BazisLib::Win32::WMI;

#pragma region BCD element types enumeration
			enum BCDElementType
			{
				//Boot manager elements
				BcdBootMgrObjectList_DisplayOrder        = 0x24000001,
				BcdBootMgrObjectList_BootSequence        = 0x24000002,
				BcdBootMgrObject_DefaultObject           = 0x23000003,
				BcdBootMgrInteger_Timeout                = 0x25000004,
				BcdBootMgrBoolean_AttemptResume          = 0x26000005,
				BcdBootMgrObject_ResumeObject            = 0x23000006,
				BcdBootMgrObjectList_ToolsDisplayOrder   = 0x24000010,
				BcdBootMgrDevice_BcdDevice               = 0x21000022,
				BcdBootMgrString_BcdFilePath             = 0x22000023,

				//Device object elements
				BcdDeviceInteger_RamdiskImageOffset   = 0x35000001,
				BcdDeviceInteger_TftpClientPort       = 0x35000002,
				BcdDeviceInteger_SdiDevice            = 0x31000003,
				BcdDeviceInteger_SdiPath              = 0x32000004,
				BcdDeviceInteger_RamdiskImageLength   = 0x35000005,

				//Library object elements
				BcdLibraryDevice_ApplicationDevice                   = 0x11000001,
				BcdLibraryString_ApplicationPath                     = 0x12000002,
				BcdLibraryString_Description                         = 0x12000004,
				BcdLibraryString_PreferredLocale                     = 0x12000005,
				BcdLibraryObjectList_InheritedObjects                = 0x14000006,
				BcdLibraryInteger_TruncatePhysicalMemory             = 0x15000007,
				BcdLibraryObjectList_RecoverySequence                = 0x14000008,
				BcdLibraryBoolean_AutoRecoveryEnabled                = 0x16000009,
				BcdLibraryIntegerList_BadMemoryList                  = 0x1700000a,
				BcdLibraryBoolean_AllowBadMemoryAccess               = 0x1600000b,
				BcdLibraryInteger_FirstMegabytePolicy                = 0x1500000c,
				BcdLibraryBoolean_DebuggerEnabled                    = 0x16000010,
				BcdLibraryInteger_DebuggerType                       = 0x15000011,
				BcdLibraryInteger_SerialDebuggerPortAddress          = 0x15000012,
				BcdLibraryInteger_SerialDebuggerPort                 = 0x15000013,
				BcdLibraryInteger_SerialDebuggerBaudRate             = 0x15000014,
				BcdLibraryInteger_1394DebuggerChannel                = 0x15000015,
				BcdLibraryString_UsbDebuggerTargetName               = 0x12000016,
				BcdLibraryBoolean_DebuggerIgnoreUsermodeExceptions   = 0x16000017,
				BcdLibraryInteger_DebuggerStartPolicy                = 0x15000018,
				BcdLibraryBoolean_EmsEnabled                         = 0x16000020,
				BcdLibraryInteger_EmsPort                            = 0x15000022,
				BcdLibraryInteger_EmsBaudRate                        = 0x15000023,
				BcdLibraryString_LoadOptionsString                   = 0x12000030,
				BcdLibraryBoolean_DisplayAdvancedOptions             = 0x16000040,
				BcdLibraryBoolean_DisplayOptionsEdit                 = 0x16000041,
				BcdLibraryBoolean_GraphicsModeDisabled               = 0x16000046,
				BcdLibraryInteger_ConfigAccessPolicy                 = 0x15000047,
				BcdLibraryBoolean_AllowPrereleaseSignatures          = 0x16000049,

				//MemDiag object elements
				BcdMemDiagInteger_PassCount      = 0x25000001,
				BcdMemDiagInteger_FailureCount   = 0x25000003,

				//OS Loader element objects
				BcdOSLoaderDevice_OSDevice                       = 0x21000001,
				BcdOSLoaderString_SystemRoot                     = 0x22000002,
				BcdOSLoaderObject_AssociatedResumeObject         = 0x23000003,
				BcdOSLoaderBoolean_DetectKernelAndHal            = 0x26000010,
				BcdOSLoaderString_KernelPath                     = 0x22000011,
				BcdOSLoaderString_HalPath                        = 0x22000012,
				BcdOSLoaderString_DbgTransportPath               = 0x22000013,
				BcdOSLoaderInteger_NxPolicy                      = 0x25000020,
				BcdOSLoaderInteger_PAEPolicy                     = 0x25000021,
				BcdOSLoaderBoolean_WinPEMode                     = 0x26000022,
				BcdOSLoaderBoolean_DisableCrashAutoReboot        = 0x26000024,
				BcdOSLoaderBoolean_UseLastGoodSettings           = 0x26000025,
				BcdOSLoaderBoolean_AllowPrereleaseSignatures     = 0x26000027,
				BcdOSLoaderBoolean_NoLowMemory                   = 0x26000030,
				BcdOSLoaderInteger_RemoveMemory                  = 0x25000031,
				BcdOSLoaderInteger_IncreaseUserVa                = 0x25000032,
				BcdOSLoaderBoolean_UseVgaDriver                  = 0x26000040,
				BcdOSLoaderBoolean_DisableBootDisplay            = 0x26000041,
				BcdOSLoaderBoolean_DisableVesaBios               = 0x26000042,
				BcdOSLoaderInteger_ClusterModeAddressing         = 0x25000050,
				BcdOSLoaderBoolean_UsePhysicalDestination        = 0x26000051,
				BcdOSLoaderInteger_RestrictApicCluster           = 0x25000052,
				BcdOSLoaderBoolean_UseBootProcessorOnly          = 0x26000060,
				BcdOSLoaderInteger_NumberOfProcessors            = 0x25000061,
				BcdOSLoaderBoolean_ForceMaximumProcessors        = 0x26000062,
				BcdOSLoaderBoolean_ProcessorConfigurationFlags   = 0x25000063,
				BcdOSLoaderInteger_UseFirmwarePciSettings        = 0x26000070,
				BcdOSLoaderInteger_MsiPolicy                     = 0x26000071,
				BcdOSLoaderInteger_SafeBoot                      = 0x25000080,
				BcdOSLoaderBoolean_SafeBootAlternateShell        = 0x26000081,
				BcdOSLoaderBoolean_BootLogInitialization         = 0x26000090,
				BcdOSLoaderBoolean_VerboseObjectLoadMode         = 0x26000091,
				BcdOSLoaderBoolean_KernelDebuggerEnabled         = 0x260000a0,
				BcdOSLoaderBoolean_DebuggerHalBreakpoint         = 0x260000a1,
				BcdOSLoaderBoolean_EmsEnabled                    = 0x260000b0,
				BcdOSLoaderInteger_DriverLoadFailurePolicy       = 0x250000c1,
				BcdOSLoaderInteger_BootStatusPolicy              = 0x250000E0, 
			};
#pragma endregion

			//! Represents a single OS entry
			/*! This class represents a single OS entry in a BCD store. To get a property (BCD Element) of an entry, use the
				GetElement() method. To set a property (Element), use one of the SetElement() methods.
			*/
			class BCDObject : public WMIObject
			{
			public:
				BCDObject()
				{
				}

				BCDObject( IUnknown *pUnknown, const CComPtr<IWbemServices> &pServices)
					: WMIObject(pUnknown, pServices)
				{
				}

			public:
				//! Returns the GUID of a BCD object
				DynamicStringW GetID() const
				{
					return GetPropertyString(L"Id");
				}

			public:
				enum BCDObjectType
				{
					GlobalSettings		= 0x10100002,	//Windows Boot Manager

					WindowsLoader		= 0x10200003,	//Windows Boot Loader
					HibernateResumer	= 0x10200004,	//Resume from Hibernate
					BootApplication		= 0x10200005,	//Custom boot application, such as Memory Tester

					LegacyOSLoader		= 0x10300006,	//Windows Legacy OS Loader

					ModuleSettings		= 0x20100000,	//EMS Settings, Debugger Settings, RAM defects
					BootLdrSettings		= 0x20200003,	//Boot Loader Settings
					ResumeLdrSettings	= 0x20200004,	//Resume Loader Settings
				};

				//! Returns the BCD object type (OS entry/NTLDR entry/Resume-from-Hibernate entry/etc)
				BCDObjectType GetType() const
				{
					return (BCDObjectType)GetPropertyInt(L"Type");
				}

				//! Returns an property (element) of an object
				class BCDElement GetElement(BCDElementType type);

			private:
				ActionStatus SetElementHelper(BCDElementType type, LPCWSTR pFunctionName, LPCWSTR pParamName, const CComVariant &Value);
				friend class BCDElement;

			public:
				//! Sets a string property (element) of an object
				ActionStatus SetElement(BCDElementType type, LPCWSTR Value)
				{
					return SetElementHelper(type, L"SetStringElement", L"String", CComVariant(Value));
				}

				//! Sets an integer property (element) of an object
				ActionStatus SetElement(BCDElementType type, ULONGLONG Value)
				{
					return SetElementHelper(type, L"SetIntegerElement", L"Integer", Value);
				}

				//! Sets a boolean property (element) of an object
				ActionStatus SetElement(BCDElementType type, bool Value)
				{
					return SetElementHelper(type, L"SetBooleanElement", L"Boolean", Value);
				}

				//! Sets a pointer-to-BCD-object property (element) of an object
				ActionStatus SetElement(BCDElementType type, const BCDObject &Value)
				{
					DynamicStringW objId = Value.GetID();
					if (objId.empty())
						return MAKE_STATUS(InvalidParameter);
					return SetElementHelper(type, L"SetObjectElement", L"Id", objId.c_str());
				}

				//! Tests whether two BCDObject instances represent the same BCD object
				bool operator==(const BCDObject &_Right)
				{
					return GetID() == _Right.GetID();
				}

				//! Tests whether two BCDObject instances represent different BCD objects
				bool operator !=(const BCDObject &_Right)
				{
					return GetID() != _Right.GetID();
				}

			};

			//! Represents a BCD device description, that is used to set boot device for an OS
			class BCDDeviceData : public WMIObject
			{
			public:
				BCDDeviceData ()
				{
				}

				BCDDeviceData (const VARIANT *pInterface, const CComPtr<IWbemServices> &pServices)
					: WMIObject(pInterface, pServices)
				{
				}

			public:
				enum DeviceType
				{
					BootDevice			= 1,
					PartitionDevice		= 2,
					FileDevice			= 3,
					RamdiskDevice		= 4,
					UnknownDevice		= 5,
				};

				DeviceType GetDeviceType()
				{
					return (DeviceType)GetPropertyInt(L"DeviceType");
				}

				DynamicStringW GetAdditionalOptions()
				{
					return GetPropertyString(L"AdditionalOptions");
				}

				DynamicStringW GetPartitionPath()
				{
					return GetPropertyString(L"Path");
				}
			};

			//! Represents a single property (element) of a BCD object
			class BCDElement : public WMIObject
			{
			private:
				BCDElementType m_ElementType;
				BCDObject m_Owner;

			public:
				BCDElement ()
				{
				}

				BCDElement ( IUnknown *pUnknown, const CComPtr<IWbemServices> &pServices, const BCDObject &Owner, BCDElementType ElementType)
					: WMIObject(pUnknown, pServices)
					, m_ElementType(ElementType)
					, m_Owner(Owner)
				{
				}

			public:
				
				//! Returns a variant representation of the property
				CComVariant ToVariant()
				{
					int elType = GetPropertyInt(L"Type");
					switch ((elType & 0x0F000000) >> 24)
					{
					case 1: //Device
						return GetPropertyVariant(L"Device");
					case 2: //String
						return GetPropertyVariant(L"String");
					case 3: //Object
						return GetPropertyVariant(L"Id");
					case 4: //Object List
						return GetPropertyVariant(L"Ids");
					case 5: //64-bit Integer
						return GetPropertyVariant(L"Integer");
					case 6: //Boolean
						return GetPropertyVariant(L"Boolean");
					case 7: //Array of 64-bit integers
						return GetPropertyVariant(L"Integers");
					}
					return CComVariant();
				}

				//! Returns a string representation of the property derived from its variant representation
				DynamicStringW ToString()
				{
					CComVariant var = ToVariant();
					if (var.vt == VT_EMPTY)
						return DynamicStringW();
					if (var.vt != VT_BSTR)
						var.ChangeType(VT_BSTR);
					if (!var.bstrVal)
						return DynamicStringW();
					return var.bstrVal;
				}

				//! Gets a string value for a string element. Returns empty string otherwise
				DynamicStringW GetString()
				{
					return GetPropertyString(L"String");
				}

				//! Converts a BCD property to an integer
				ULONGLONG ToInteger()
				{
					CComVariant var = ToVariant();
					if (var.vt == VT_EMPTY)
						return -1LL;
					if (var.vt != VT_UI8)
						var.ChangeType(VT_UI8);
					return var.ullVal;
				}

				//! Converts a BCD property to a boolean value
				bool ToBoolean()
				{
					CComVariant var = ToVariant();
					if (var.vt == VT_EMPTY)
						return false;
					if (var.vt != VT_BOOL)
						var.ChangeType(VT_BOOL);
					return var.boolVal != FALSE;
				}

				//! Converts a BCD property to a BCD object list
				inline class BCDObjectList GetObjectList();

				//! Converts a BCD property to a BCDDeviceData object
				BCDDeviceData ToDeviceData()
				{
					CComVariant data = GetPropertyVariant(L"Device");
					return BCDDeviceData(&data, m_pServices);
				}


			private:
				//! Used internally to set the property value
				ActionStatus SetElementHelper(LPCWSTR pFunctionName, LPCWSTR pParamName, const CComVariant &Value)
				{
					return m_Owner.SetElementHelper(m_ElementType, pFunctionName, pParamName, Value);
				}
				friend class BCDObjectList;

			};

			//! Represents a list of BCD objects
			/*!
				\remarks Node that the BCDObjectList does not contain a list of BCDObject classes. Instead, it
						 contains a list of object GUIDs. To instantiate such an object, use the BCDStore::GetObject()
						 method.
			*/
			class BCDObjectList
			{
			private:
				std::vector<DynamicStringW> m_Ids;
				BCDElement m_Element;

			public:
				BCDObjectList()
				{
				}

				BCDObjectList (const BCDElement &OwningElement)
				{
					if (!OwningElement.Valid())
						return;
					int elType = OwningElement.GetPropertyInt(L"Type");
					if (((elType & 0x0F000000) >> 24) != 4)	//Object list
						return;
					
					CComVariant rawList = OwningElement.GetPropertyVariant(L"Ids");
					if ((rawList.vt != (VT_ARRAY | VT_BSTR)) || !rawList.parray)
						return;

					size_t nElements = rawList.parray->rgsabound[0].cElements;
					
					m_Ids.reserve(nElements);
					for (ULONG i = 0; i < nElements; i++)
					{
						m_Ids.push_back(((BSTR *)rawList.parray->pvData)[i]);
					}

					//BCDElement is just a wrapper around CComPtr, so copying it is cheap
					m_Element = OwningElement;
				}

				bool Valid()
				{
					return m_Element.Valid();
				}

				//! Converts a list of object GUIDs to a string form
				DynamicStringW ToString(LPCWSTR pSeparator = L"\r\n")
				{
					if (m_Ids.empty())
						return DynamicStringW();

					size_t sepLen = wcslen(pSeparator);
					
					DynamicStringW ret;
					ret.reserve((m_Ids[0].length() + sepLen) * m_Ids.size());

					for (size_t i = 0; i < m_Ids.size(); i++)
					{
						if (i)
							ret += pSeparator;
						ret += m_Ids[0];
					}
					return ret;
				}

			private:
				ActionStatus ApplyChanges();

			public:
				unsigned GetElementCount()
				{
					return (unsigned)m_Ids.size();
				}

				//! Returns a single BCD object GUID from the list
				DynamicStringW operator[](unsigned idx)
				{
					if (idx >= m_Ids.size())
						return DynamicStringW();
					return m_Ids[idx];
				}

				//! Inserts a new BCD object in the list by its ID
				ActionStatus InsertObject(const TempStringW &ObjID, unsigned InsertBefore = -1)
				{
					if (ObjID.empty())
						return MAKE_STATUS(InvalidParameter);
					if (InsertBefore > m_Ids.size())
						InsertBefore = (unsigned)m_Ids.size();
					m_Ids.insert(m_Ids.begin() + InsertBefore, ObjID);
					return ApplyChanges();
				}

				//! Inserts a new BCD object in the list
				ActionStatus InsertObject(const BCDObject &Object, unsigned InsertBefore = -1)
				{
					return InsertObject(Object.GetID(), InsertBefore);
				}
			};

			inline BCDObjectList BazisLib::Win32::BCD::BCDElement::GetObjectList()
			{
				return BCDObjectList(*this);
			}

			//! Represents a BCD store. Use the static BCDStore::OpenStore() method to create an instance.
			class BCDStore : public WMIObject
			{
			private:
				BCDStore()
				{
				}

				BCDStore( IUnknown *pUnknown, const WMIServices &pServices )
					: WMIObject(pUnknown, pServices)
				{
				}

			protected:
				std::vector<BCDObject> m_BcdObjects;

			public:
				//! Opens a BCD store
				static BCDStore OpenStore(LPCWSTR lpStoreName = L"", ActionStatus *pStatus = NULL);

			protected:
				ActionStatus ProvideBcdObjects();

			public:
				//! Returns the number of object in the store
				size_t GetObjectCount()
				{
					if (!ProvideBcdObjects().Successful())
						return 0;
					return m_BcdObjects.size();
				}

				//! Returns a list of BCD objects in a store
				const std::vector<BCDObject> &GetObjects()
				{
					ProvideBcdObjects();
					return m_BcdObjects;
				}

				//! Creates a copy of a BCD object
				BCDObject CopyObject(const BCDObject &Obj, ActionStatus *pStatus = NULL);

				//! Returns first object of the given (e.g. 'global debugging settings')
				BCDObject GetFirstObjectOfType(BCDObject::BCDObjectType type)
				{
					ProvideBcdObjects();
					for each(const BCDObject &obj in m_BcdObjects)
						if (obj.GetType() == type)
							return obj;
					return BCDObject();
				}

				//! Returns an object with given ID
				BCDObject GetObject(const TempStringW &objectID)
				{
					ProvideBcdObjects();
					for each(const BCDObject &obj in m_BcdObjects)
						if (obj.GetID() == objectID)
							return obj;
					return BCDObject();
				}

			};

		}
	}

}

#endif