#pragma once

#ifndef UNDER_CE

#include "cmndef.h"
#include <list>
#include <string>
#include <setupapi.h>
#include <cfgmgr32.h>
#include "../../bzscore/string.h"
#include "../../bzscore/buffer.h"
#include <devguid.h>

#pragma comment(lib, "setupapi")

namespace BazisLib
{
	//! Represents a device information set structure (HDEVINFO)
	/*! This class represents an information set containing data about a set of devices.
		Use DeviceInformationSet::iterator class to iterate over the devices.
	*/
	class DeviceInformationSet
	{
	private:
		HDEVINFO m_hDevInfo;
		GUID m_InterfaceGuid;
		bool m_bGuidUsed;

		DeviceInformationSet(HDEVINFO hDevInfo, const GUID *pGuid) :
			m_hDevInfo(hDevInfo)
		{
			if (pGuid)
			{
				m_InterfaceGuid = *pGuid;
				m_bGuidUsed = true;
			}
			else
			{
				memset(&m_InterfaceGuid, 0, sizeof(GUID));
				m_bGuidUsed = false;
			}
		}

	public:
		class iterator
		{
		private:
			friend class DeviceInformationSet;
			
			DeviceInformationSet &m_Set;
			unsigned m_Index;
			SP_DEVICE_INTERFACE_DATA m_Data;
			SP_DEVINFO_DATA m_DevinfoData;
			bool m_bInterfaceDataPresent;
			
			PSP_DEVICE_INTERFACE_DETAIL_DATA m_pDetailData;
			unsigned m_DetailBufferSize;
			
			CBuffer m_InstanceId;

			iterator(const iterator&) = delete;

		public:
			iterator(iterator&& right) :
				m_Set(right.m_Set),
				m_Index(right.m_Index),
				m_Data(right.m_Data),
				m_DevinfoData(right.m_DevinfoData),
				m_bInterfaceDataPresent(right.m_bInterfaceDataPresent),
				m_pDetailData(right.m_pDetailData),
				m_DetailBufferSize(right.m_DetailBufferSize),
				m_InstanceId(std::move(right.m_InstanceId))
			{
			}

		private:
			iterator(DeviceInformationSet &set, unsigned index = 0) :
				m_Set(set),
				m_Index(index),
				m_pDetailData(NULL),
				m_DetailBufferSize(0),
				m_bInterfaceDataPresent(false)
			{
				memset(&m_Data, 0, sizeof(m_Data));
				memset(&m_DevinfoData, 0, sizeof(m_DevinfoData));
				if (!LoadData())
					m_Index = -1;
			}

			bool LoadInstanceID()
			{
				DWORD requiredSizeForID = 0;
				SetupDiGetDeviceInstanceId(m_Set.m_hDevInfo, &m_DevinfoData, NULL, 0, &requiredSizeForID);
				if (!m_InstanceId.EnsureSize(requiredSizeForID * sizeof(TCHAR)))
					return false;
				if (SetupDiGetDeviceInstanceId(m_Set.m_hDevInfo, &m_DevinfoData, (PTSTR)m_InstanceId.GetData(), requiredSizeForID, NULL))
					m_InstanceId.SetSize(requiredSizeForID * sizeof(TCHAR));
				else
					m_InstanceId.SetSize(0);
				return (m_InstanceId.GetSize() != 0);
			}

			//! This method should be called ONLY after m_Data is initialized in LoadData()
			bool LoadDetails()
			{
				DWORD requiredLength = 0;
				for (int i = 0; i < 2; i++)
				{
					if (m_pDetailData)
						m_pDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
					if (!SetupDiGetDeviceInterfaceDetail(m_Set.m_hDevInfo,
														 &m_Data,
														 m_pDetailData,
														 m_DetailBufferSize,
														 &requiredLength,
														 NULL))
					{
						if (requiredLength <= m_DetailBufferSize)
							return false;
						delete m_pDetailData;
						m_DetailBufferSize = requiredLength;
						m_pDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)new char[m_DetailBufferSize];
					}
					else
						return true;
				}
				return false;
			}

			bool LoadInterfaceData()
			{
				if (m_Index == -1)
					return false;
				m_Data.cbSize = sizeof(m_Data);
				if (!SetupDiEnumDeviceInterfaces (m_Set.m_hDevInfo,
											 &m_DevinfoData,
											 m_Set.m_bGuidUsed ? &m_Set.m_InterfaceGuid : NULL,
											 0,
											 &m_Data))
						return false;
				else
					return true;
			}

			bool LoadDevData()
			{
				if (m_Index == -1)
					return false;
				for (;;)
				{
					m_DevinfoData.cbSize = sizeof(m_DevinfoData);
					if (!SetupDiEnumDeviceInfo(m_Set.m_hDevInfo,
											   m_Index,
											   &m_DevinfoData))
					{
						if (ERROR_NO_MORE_ITEMS == GetLastError())
							return false;
						else
							m_Index++;
					}
					else
						return true;
				}
			}

			bool LoadData()
			{
				if (!LoadDevData())
					return false;
				if (LoadInterfaceData())
				{
					m_bInterfaceDataPresent = true;
					if (!LoadDetails())
						return false;
				}
				else
					m_bInterfaceDataPresent = false;
				return true;
			}

		public:
			~iterator()
			{
				delete m_pDetailData;
			}

			bool Valid()
			{
				return (m_Index != -1);
			}

			bool operator!=(const iterator &it)
			{
				return (m_Index != it.m_Index) || (m_Set.m_hDevInfo != it.m_Set.m_hDevInfo);
			}

			iterator &operator++()
			{
				m_Index++;
				m_InstanceId.SetSize(0);
				if (!LoadData())
					m_Index = -1;
				return *this;
			}

			void operator++(int)
			{
				++(*this);
			}

			const TCHAR *GetDevicePath()
			{
				return (Valid() && m_pDetailData) ? m_pDetailData->DevicePath : NULL;
			}

			const TCHAR *GetInstanceId()
			{
				if (!Valid())
					return NULL;
				if (m_InstanceId.GetSize())
					return (TCHAR *)m_InstanceId.GetData();
				if (!LoadInstanceID())
					return NULL;
				return (TCHAR *)m_InstanceId.GetData();
			}

			//! Queries one of the standard device registry properties, such as SPDRP_HARDWAREID
			/*! 
				\param Property Specifies the exact property to query. Property IDs have names like SPDRP_xxx
			*/
			String GetDeviceRegistryProperty(unsigned Property)
			{
				DWORD dataType = 0;
				DWORD requiredSize = 0;
				SetupDiGetDeviceRegistryProperty(m_Set.m_hDevInfo,
												  &m_DevinfoData,
												  Property,
												  &dataType,
												  NULL,
												  NULL,
												  &requiredSize);
				if (!requiredSize)
					return _T("");
				
				if ((dataType != REG_SZ) && (dataType != REG_MULTI_SZ))
					return _T("");

				PBYTE pBuf = new BYTE[requiredSize];

				if (!SetupDiGetDeviceRegistryProperty(m_Set.m_hDevInfo,
													  &m_DevinfoData,
													  Property,
													  &dataType,
													  pBuf,
													  requiredSize,
													  NULL))
				{
					delete pBuf;
					return _T("");
				}

				String prop((TCHAR *)pBuf);
				delete pBuf;
				return prop;
			}

			bool CallClassInstaller(DI_FUNCTION Function)
			{
				return (SetupDiCallClassInstaller(Function, m_Set.m_hDevInfo, &m_DevinfoData) != FALSE);
			}

			//! Changes device state to DICS_ENABLE, DICS_DISABLE, or similar
			bool ChangeState(DWORD dwNewState)
			{
				SP_PROPCHANGE_PARAMS params = {0,};
				params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
				params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
				params.StateChange = dwNewState;
				params.Scope = DICS_FLAG_GLOBAL;
				params.HwProfile = 0;
				if (!SetupDiSetClassInstallParams(m_Set.m_hDevInfo, &m_DevinfoData, &params.ClassInstallHeader, sizeof(params)))
					return false;
				if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, m_Set.m_hDevInfo, &m_DevinfoData))
					return false;
				return true;
			}

			//! Retrieves device state flags such as DN_STARTED, problem flags - CM_PROB_DISABLED or similar
			DWORD QueryStateFlags(DWORD *pCMProblem = NULL)
			{
				SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
				ULONG status = 0, problem = 0;
				DEVINST devInst = 0;

				devInfoListDetail.cbSize = sizeof(devInfoListDetail);
				if(!SetupDiGetDeviceInfoListDetail(m_Set.m_hDevInfo, &devInfoListDetail))
					return 0;

				if (CM_Locate_DevNode(&devInst, (DEVINSTID)GetInstanceId(), 0) != CR_SUCCESS)
					return 0;

				if (CM_Get_DevNode_Status_Ex(&status,&problem,devInst,0,devInfoListDetail.RemoteMachineHandle) != CR_SUCCESS)
					return 0;

				if (pCMProblem)
					*pCMProblem = problem;
				return status;
			}

			bool IsEnabled()
			{
				DWORD problem = 0;
				DWORD stateFlags = QueryStateFlags(&problem);
				if ((stateFlags & DN_HAS_PROBLEM) && (problem == CM_PROB_DISABLED))
					return false;
				return true;
			}

			bool IsStarted()
			{
				return ((QueryStateFlags() & DN_STARTED) != 0);
			}

			private:
				void operator=(const iterator &)
				{

				}

		};

	public:
		bool Valid()
		{
			return (m_hDevInfo != INVALID_HANDLE_VALUE);
		}

		~DeviceInformationSet()
		{
			if (m_hDevInfo != INVALID_HANDLE_VALUE)
				SetupDiDestroyDeviceInfoList(m_hDevInfo);
		}
        
		iterator begin()
		{
			return iterator(*this);
		}

		iterator end()
		{
			return iterator(*this, -1);
		}

	public:
		static DeviceInformationSet GetLocalDevices(const GUID *pInterfaceGuid = NULL, DWORD Flags = DIGCF_PRESENT)
		{
			if (pInterfaceGuid)
				Flags |= DIGCF_DEVICEINTERFACE;
			else
				Flags |= DIGCF_ALLCLASSES;
			return DeviceInformationSet(SetupDiGetClassDevs(pInterfaceGuid, NULL, NULL, Flags),
										pInterfaceGuid);
		}

	};

	class INFClass
	{
	private:
		GUID m_Guid;
		String m_ClassName;

	public:
		INFClass(const GUID &Guid, const TCHAR *ptszName) :
			m_ClassName(ptszName)
		{
			if (&Guid)
				m_Guid = Guid;
			else
				memset(&m_Guid, 0, sizeof(m_Guid));
		}

		static INFClass FromINFFile(const TCHAR *ptszInfPath)
		{
			GUID Guid = {0,};
		    TCHAR ClassName[MAX_CLASS_NAME_LEN];
			if (!SetupDiGetINFClass(ptszInfPath, &Guid, ClassName,sizeof(ClassName)/sizeof(ClassName[0]), NULL))
				return INFClass(Guid, _T(""));
			return INFClass(Guid, ClassName);
		}

		const GUID *GetGuid() const
		{
			return &m_Guid;
		}

		const TCHAR *GetName() const
		{
			return m_ClassName.c_str();
		}

	};

	namespace StandardINFClasses
	{
		//The following device class types are defined as an example of INFClass usage.
		//For full system-provided class list, see http://msdn.microsoft.com/en-us/library/ms791134.aspx
		static const INFClass BatteryDevices = INFClass(GUID_DEVCLASS_BATTERY,		_T("Battery"));
		static const INFClass CDROMDrives    = INFClass(GUID_DEVCLASS_CDROM,		_T("CDROM"));
		static const INFClass DiskDrives     = INFClass(GUID_DEVCLASS_DISKDRIVE,	_T("DiskDrive"));
		static const INFClass HDControllers  = INFClass(GUID_DEVCLASS_HDC,			_T("HDC"));
		static const INFClass HIDDevices     = INFClass(GUID_DEVCLASS_HIDCLASS,		_T("HIDClass"));
		static const INFClass MouseDevices	 = INFClass(GUID_DEVCLASS_MOUSE,		_T("Mouse"));
	}

	typedef bool (*PDEVICE_SN_FILTER)(const String &serial, void *pContext);

	//! Returns a list of device interface paths for given interface GUID
	/*! This routine returns a list of device interface paths that match a given interface GUID. Each interface
		path can be passed to CreateFile() routine to connect to a device.
	*/
	std::list<String> EnumerateDevicesByInterface(const GUID *pguidInterfaceType, PDEVICE_SN_FILTER Filter = NULL, void *pContext = NULL);
	
	//! Returns a list of device instance IDs with the specified hardware ID
	/*!
		\remarks Note that unlike the EnumerateDevicesByInterface() routine, this routine returns a list of device
				 instance IDs (not interface links) that cannot be passed to CreateFile().
	*/
	std::list<String> EnumerateDevicesByHardwareID(const String &ID);

	//! Adds a root-enumerated node to system device list
	/*! This routine should be called to add a new virtual device to the system. It is useful for 
		installing virtual bus drivers and other drivers that do not rely on PDOs created by other
		bus drivers.
		\param Class The device class to be installed
		\param HardwareId Hardware ID for the device. Windows will perform driver matching based on this ID.
	*/
	BazisLib::ActionStatus AddRootEnumeratedNode(const INFClass &Class, LPCTSTR HardwareId);


	template <class _File, class _Structure> ActionStatus QueryVariableSizeStructure(_File &file,
		DWORD ioctlCode, 
		TypedBuffer<_Structure> &pStruct, 
		size_t initialSize,
		DWORD errorOnInsufficientSize = ERROR_INSUFFICIENT_BUFFER,
		const void *pInputBuffer = NULL,
		size_t inputBufferLength = 0)
	{
		if (!file.Valid())
			return MAKE_STATUS(InvalidHandle);
		size_t size = initialSize;
		ActionStatus status;
		for (;;)
		{
			if (!pStruct.EnsureSize(size))
				return MAKE_STATUS(NoMemory);
			size_t done = file.DeviceIoControl(ioctlCode, pInputBuffer, inputBufferLength, pStruct, pStruct.GetAllocated(), &status);
			if (!status.Successful())
			{
				if ((status.GetErrorCode() == ActionStatus::FromWin32Error(errorOnInsufficientSize)) && (size < (1024 * 1024)))
				{
					size *= 2;
					continue;
				}
				return status;
			}
			if (size < sizeof(DRIVE_LAYOUT_INFORMATION_EX))
				return MAKE_STATUS(UnknownError);
			pStruct.SetSize(done);
			return MAKE_STATUS(Success);
		}
	}
}

#endif