#include "KdcomInstall.h"

#include "Utils.h"

#define KDBC_DLL_PRODUCT_NAME L"BugChecker"

//
// The code in this file is from VirtualKD.
//

#include <stdio.h>
#include <accctrl.h>
#include <aclapi.h>

#include <BazisLib/bzscore/Win32/security.h>
#include <BazisLib/bzscore/file.h>
#include <BazisLib/bzshlp/Win32/wow64.h>
#include <BazisLib/bzscore/Win32/registry.h>
#include <BazisLib/bzscore/status.h>
#include <BazisLib/bzshlp/Win32/BCD.h>
#include <BazisLib/bzscore/strfast.h>
#include <BazisLib/bzscore/objmgr.h>
#include <BazisLib/bzshlp/Win32/bcd.h>
#include <BazisLib/bzshlp/textfile.h>

using namespace BazisLib;

ActionStatus SetPrivilege(
	HANDLE hToken,          // access token handle
	LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	BOOL bEnablePrivilege   // to enable or disable privilege
)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!LookupPrivilegeValue(
		NULL,            // lookup privilege on local system
		lpszPrivilege,   // privilege to lookup 
		&luid))        // receives LUID of privilege
	{
		return MAKE_STATUS(ActionStatus::FromLastError());
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if (!AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES)NULL,
		(PDWORD)NULL))
	{
		return MAKE_STATUS(ActionStatus::FromLastError());
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		return MAKE_STATUS(ActionStatus::FromLastError());
	}

	return MAKE_STATUS(Success);
}


ActionStatus TakeOwnership(LPTSTR lpszOwnFile)
{
	HANDLE hToken = NULL;
	DWORD dwRes;
	ActionStatus st;

	BazisLib::Win32::Security::Sid sid(WinBuiltinAdministratorsSid);

	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES,
		&hToken))
	{
		st = MAKE_STATUS(ActionStatus::FromLastError());
		goto Cleanup;
	}

	// Enable the SE_TAKE_OWNERSHIP_NAME privilege.
	st = SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, TRUE);
	if (!st.Successful())
	{
		goto Cleanup;
	}

	// Set the owner in the object's security descriptor.
	dwRes = SetNamedSecurityInfo(
		lpszOwnFile,                 // name of the object
		SE_FILE_OBJECT,              // type of object
		OWNER_SECURITY_INFORMATION,  // change only the object's owner
		sid,                   // SID of Administrator group
		NULL,
		NULL,
		NULL);

	if (dwRes != ERROR_SUCCESS)
	{
		st = MAKE_STATUS(ActionStatus::FromWin32Error(dwRes));
		goto Cleanup;
	}

Cleanup:

	if (hToken)
		CloseHandle(hToken);

	return st;

}

static WORD GetOSVersion()
{
	typedef void (WINAPI* RtlGetVersion)(OSVERSIONINFOEXW*);
	OSVERSIONINFOEXW info = { 0 };
	info.dwOSVersionInfoSize = sizeof(info);
	((RtlGetVersion)GetProcAddress(GetModuleHandleW(L"ntdll"), "RtlGetVersion"))(&info);
	return (WORD)((info.dwMajorVersion << 8) | info.dwMinorVersion);
}

bool IsVistaOrLater()
{
	return GetOSVersion() >= 0x0600;
}

bool IsWin8OrLater()
{
	return GetOSVersion() >= 0x0602;
}

bool IsWin10OrLater()
{
	return GetOSVersion() >= 0x0a00;
}

namespace BootEditor
{
	using namespace BazisLib;

	enum KernelDebuggerMode
	{
		kdNone,
		kdUnknown,
		kdStandard,
		kdCustom,
	};

	class AIBootConfigurationEntry : AUTO_INTERFACE
	{
	public:
		virtual BazisLib::String GetDescription() = 0;
		virtual ActionStatus SetDescription(LPCWSTR pDescription) = 0;

		virtual bool IsCurrentOS() = 0;

		virtual KernelDebuggerMode GetDebuggerMode() = 0;
		virtual String GetCustomKDName() = 0;

		virtual ActionStatus EnableCustomKD(LPCWSTR pCustomKDName) = 0;
		virtual ActionStatus ExplicitlyDisableDebugging() = 0;
	};

	class AIBootConfigurationEditor : AUTO_INTERFACE
	{
	public:
		virtual ActionStatus StartEnumeration() = 0;
		virtual ManagedPointer<AIBootConfigurationEntry> GetNextEntry() = 0;

		virtual ManagedPointer<AIBootConfigurationEntry> CopyEntry(const ConstManagedPointer<AIBootConfigurationEntry>& pEntry, bool AddToSelectionList = true, ActionStatus* pStatus = NULL) = 0;
		virtual ActionStatus FinishEditing() = 0;

		virtual ActionStatus SetDefaultEntry(const ConstManagedPointer<AIBootConfigurationEntry>& pEntry) = 0;

		virtual unsigned GetTimeout(ActionStatus* pStatus = NULL) = 0;
		virtual ActionStatus SetTimeout(unsigned TimeoutInSeconds) = 0;

		virtual size_t GetEntryCount() = 0;
	};

	ManagedPointer<AIBootConfigurationEditor> CreateConfigurationEditor();
}

using namespace BootEditor;

namespace VistaBCD
{
	using namespace BazisLib::Win32::BCD;

	class BootEntry : public AIBootConfigurationEntry
	{
	private:
		BCDObject m_Object;

		friend class BootConfigEditor;

	public:
		virtual BazisLib::String GetDescription()
		{
			return m_Object.GetElement(BcdLibraryString_Description).ToString();
		}

		virtual ActionStatus SetDescription(LPCWSTR pDescription)
		{
			return m_Object.SetElement(BcdLibraryString_Description, pDescription);
		}

		virtual bool IsCurrentOS()
		{
			wchar_t wszWinDir[MAX_PATH] = { 0, }, wszRawPath[MAX_PATH] = { 0, };
			GetWindowsDirectoryW(wszWinDir, __countof(wszWinDir));

			wszWinDir[2] = 0;
			if (!QueryDosDevice(wszWinDir, wszRawPath, __countof(wszRawPath)))
				return false;

			BCDDeviceData devData = m_Object.GetElement(BcdOSLoaderDevice_OSDevice).ToDeviceData();
			if (!devData.Valid())
				return false;

			if (devData.GetPartitionPath().icompare(wszRawPath))
				return false;	//Partition mismatch

			wszWinDir[2] = '\\';

			if (m_Object.GetElement(BcdOSLoaderString_SystemRoot).ToString().icompare(&wszWinDir[2]))
				return false;

			return true;
		}

		virtual KernelDebuggerMode GetDebuggerMode()
		{
			if (!m_Object.Valid())
				return kdUnknown;
			if (!m_Object.GetElement(BcdOSLoaderBoolean_KernelDebuggerEnabled).ToBoolean())
				return kdNone;
			if (!m_Object.GetElement(BcdOSLoaderString_DbgTransportPath).ToString().empty())
				return kdCustom;
			return kdStandard;
		}

		virtual String GetCustomKDName()
		{
			if (!m_Object.GetElement(BcdOSLoaderBoolean_KernelDebuggerEnabled).ToBoolean())
				return String();
			return m_Object.GetElement(BcdOSLoaderString_DbgTransportPath).ToString();
		}

		virtual ActionStatus EnableCustomKD(LPCWSTR pCustomKDName)
		{
			if (!pCustomKDName)
				return MAKE_STATUS(InvalidParameter);
			ActionStatus st = m_Object.SetElement(BcdOSLoaderString_DbgTransportPath, pCustomKDName);
			if (!st.Successful())
				return st;
			st = m_Object.SetElement(BcdLibraryBoolean_AllowPrereleaseSignatures, true);
			ActionStatus st2 = m_Object.SetElement(BcdOSLoaderBoolean_AllowPrereleaseSignatures, true);
			if (!st.Successful() && !st2.Successful())
				return st;
			st = m_Object.SetElement(BcdOSLoaderBoolean_KernelDebuggerEnabled, true);
			if (!st.Successful())
				return st;

			if (IsWin8OrLater())
			{
				st = m_Object.SetElement((BCDElementType)0x250000c2, (ULONGLONG)0); //Enable old-style boot selection menu.
				if (!st.Successful())
					return st;
				st = m_Object.SetElement(BcdLibraryBoolean_AutoRecoveryEnabled, false);
				if (!st.Successful())
					return st;
				st = m_Object.SetElement(BcdLibraryInteger_DebuggerType, (ULONGLONG)0); //Set DebuggerType to DebuggerSerial
				if (!st.Successful())
					return st;
			}

			return MAKE_STATUS(Success);
		}

		virtual ActionStatus ExplicitlyDisableDebugging()
		{
			return m_Object.SetElement(BcdOSLoaderBoolean_KernelDebuggerEnabled, false);
		}

		bool Valid()
		{
			return m_Object.Valid() && (m_Object.GetType() == BCDObject::WindowsLoader);
		}

	public:
		BootEntry(const BCDObject& obj)
			: m_Object(obj)
		{
		}
	};

	class BootConfigEditor : public AIBootConfigurationEditor
	{
	private:
		BCDStore m_Store;

		unsigned m_NextBootObject;

	public:
		BootConfigEditor()
			: m_Store(BCDStore::OpenStore())
			, m_NextBootObject(0)
		{
		}

		virtual ActionStatus StartEnumeration()
		{
			m_NextBootObject = 0;
			return MAKE_STATUS(Success);
		}

		virtual ManagedPointer<AIBootConfigurationEntry> GetNextEntry()
		{
			for (;;)
			{
				if (m_NextBootObject >= m_Store.GetObjectCount())
					return NULL;
				const BCDObject& obj = m_Store.GetObjects()[m_NextBootObject++];
				if (obj.GetType() != BCDObject::WindowsLoader)
					continue;
				return new BootEntry(obj);
			}
		}

		virtual size_t GetEntryCount()
		{
			return m_Store.GetObjectCount();
		}

		virtual ManagedPointer<AIBootConfigurationEntry> CopyEntry(const ConstManagedPointer<AIBootConfigurationEntry>& pEntry, bool AddToSelectionList = true, ActionStatus* pStatus = NULL)
		{
			if (!pEntry)
			{
				ASSIGN_STATUS(pStatus, InvalidParameter);
				return NULL;
			}
			ManagedPointer<BootEntry> pBootEntry = pEntry;
			if (!pBootEntry->Valid())
			{
				ASSIGN_STATUS(pStatus, InvalidParameter);
				return NULL;
			}


			BCDObject newObj = m_Store.CopyObject(pBootEntry->m_Object, pStatus);
			if (!newObj.Valid())
			{
				ASSIGN_STATUS(pStatus, UnknownError);
				return NULL;
			}

			if (AddToSelectionList)
			{
				BCDObject globalSettings = m_Store.GetFirstObjectOfType(BCDObject::GlobalSettings);
				if (!globalSettings.Valid())
				{
					ASSIGN_STATUS(pStatus, UnknownError);
					return NULL;
				}
				BCDObjectList selectionList = globalSettings.GetElement(BcdBootMgrObjectList_DisplayOrder).GetObjectList();
				if (!selectionList.Valid())
				{
					ASSIGN_STATUS(pStatus, UnknownError);
					return NULL;
				}

				ActionStatus st = selectionList.InsertObject(newObj);
				if (!st.Successful())
				{
					ASSIGN_STATUS(pStatus, UnknownError);
					return NULL;
				}
			}

			ASSIGN_STATUS(pStatus, Success);
			return new BootEntry(newObj);
		}

		virtual ActionStatus FinishEditing()
		{
			return MAKE_STATUS(Success);
		}

		bool Valid()
		{
			return m_Store.Valid();
		}

		virtual ActionStatus SetDefaultEntry(const ConstManagedPointer<AIBootConfigurationEntry>& pEntry)
		{
			if (!pEntry)
				return MAKE_STATUS(InvalidParameter);
			ManagedPointer<BootEntry> pBootEntry = pEntry;
			if (!pBootEntry || !pBootEntry->Valid())
				return MAKE_STATUS(InvalidParameter);

			BCDObject globalSettings = m_Store.GetFirstObjectOfType(BCDObject::GlobalSettings);
			if (!globalSettings.Valid())
				return MAKE_STATUS(UnknownError);

			return globalSettings.SetElement(BcdBootMgrObject_DefaultObject, pBootEntry->m_Object);
		}

		virtual ActionStatus SetTimeout(unsigned TimeoutInSeconds)
		{
			BCDObject globalSettings = m_Store.GetFirstObjectOfType(BCDObject::GlobalSettings);
			if (!globalSettings.Valid())
				return MAKE_STATUS(UnknownError);

			return globalSettings.SetElement(BcdBootMgrInteger_Timeout, (ULONGLONG)TimeoutInSeconds);
		}

		virtual unsigned GetTimeout(ActionStatus* pStatus = NULL)
		{
			BCDObject globalSettings = m_Store.GetFirstObjectOfType(BCDObject::GlobalSettings);
			if (!globalSettings.Valid())
			{
				ASSIGN_STATUS(pStatus, UnknownError);
				return -1;
			}

			BCDElement el = globalSettings.GetElement(BcdBootMgrInteger_Timeout);
			if (!el.Valid())
			{
				ASSIGN_STATUS(pStatus, UnknownError);
				return -1;
			}

			return (unsigned)el.ToInteger();
		}

	};
}


namespace BootIniEditor
{
	class BootEntry : public AIBootConfigurationEntry
	{
	private:
		DECLARE_REFERENCE(class BootConfigEditor, m_pEditor);
		unsigned m_EntryIndex;

	public:
		virtual BazisLib::String GetDescription();

		virtual ActionStatus SetDescription(LPCWSTR pDescription);
		virtual bool IsCurrentOS();
		virtual KernelDebuggerMode GetDebuggerMode();
		virtual String GetCustomKDName();
		virtual ActionStatus EnableCustomKD(LPCWSTR pCustomKDName);

		bool Valid()
		{
			return (m_EntryIndex != -1);
		}

	public:
		BootEntry(const ConstManagedPointer<BootConfigEditor>& pEditor, int EntryIndex)
			: INIT_REFERENCE(m_pEditor, pEditor)
			, m_EntryIndex(EntryIndex)
		{
		}

		unsigned GetEntryIndex()
		{
			return m_EntryIndex;
		}

		virtual ActionStatus ExplicitlyDisableDebugging()
		{
			return MAKE_STATUS(Success);	//Not needed on XP
		}
	};

	class BootConfigEditor : public AIBootConfigurationEditor
	{
	private:
		class EntryBase
		{
		protected:
			bool m_Modified;
			DynamicStringA m_OriginalString;

			EntryBase()
				: m_Modified(false)
			{
			}

		public:

			bool IsModified()
			{
				return m_Modified;
			}

			const DynamicStringA& GetOriginalString()
			{
				return m_OriginalString;
			}

			virtual DynamicStringA BuildNewString() const = 0;

		};

		class SimpleEntry : public EntryBase
		{
		private:
			DynamicStringA m_Value;

		public:

			static DynamicStringA GetKey(const TempStringA& Line)
			{
				size_t off = Line.find('=');
				if (off == -1)
					return DynamicStringA();
				DynamicStringA res = Line.substr(0, off);
				off = res.find_last_not_of(" \t");
				if (off == -1)
					off = res.length() - 1;
				res.erase(off + 1);
				return res;
			}

			SimpleEntry(const TempStringA& Line = ConstStringA(""))
			{
				size_t off = Line.find('=');
				if (off == -1)
					return;
				off++;
				off = Line.find_first_not_of(" \t", off);
				if (off == -1)
					return;
				m_Value = Line.substr(off);
				m_OriginalString = Line;
			}

		public:
			const DynamicStringA& GetValue()
			{
				return m_Value;
			}

			void SetValue(const TempStringA& NewValue)
			{
				m_Value = NewValue;
				m_Modified = true;
			}

			void SetValue(unsigned NewValue)
			{
				m_Value.Format("%d", NewValue);
				m_Modified = true;
			}

			unsigned ToInteger()
			{
				return atoi(m_Value.c_str());
			}

		public:
			virtual DynamicStringA BuildNewString() const
			{
				DynamicStringA ret = GetKey(m_OriginalString);
				ret += '=';
				ret += m_Value;
				return ret;
			}

		};

		class _CaseInsensitiveLess : public std::binary_function<DynamicStringA, DynamicStringA, bool>
		{
		public:
			bool operator()(const DynamicStringA& _Left, const DynamicStringA& _Right) const
			{
				return _stricmp(_Left.c_str(), _Right.c_str()) < 0;
			}
		};

		class OSEntry : public EntryBase
		{
		private:
			DynamicStringA m_Path;
			DynamicStringA m_Description;
			std::map<DynamicStringA, DynamicStringA, _CaseInsensitiveLess> m_Properties;

			bool m_Valid;

		public:
			OSEntry(const TempStringA& Line)
				: m_Valid(false)
			{
				SplitStrByFirstOfA keyAndValue(Line, ConstStringA("= \t"));
				if (!keyAndValue)
					return;

				m_Path = keyAndValue.left;

				for (size_t context = 0;;)
				{
					TempStringA token = FastStringRoutines::GetNextToken<false>(keyAndValue.right, &context);
					if (token.empty())
						break;
					if (m_Description.empty())
						m_Description = token;
					else
					{
						if (token[0] != '/')
							return;
						SplitStrByFirstOfA keyAndVal(token.substr(1), ConstStringA("="));
						if (keyAndVal)
							m_Properties[keyAndVal.left] = keyAndVal.right;
						else
							m_Properties[token.substr(1)] = DynamicStringA();
					}
				}
				if (m_Description.empty())
					return;

				m_OriginalString = Line;
				m_Valid = true;
			}


			bool Valid()
			{
				return m_Valid;
			}

			const DynamicStringA& GetDescription() const { return m_Description; }
			const DynamicStringA& GetPath() const { return m_Path; }

			const DynamicStringA GetProperty(const DynamicStringA& Key)
			{
				std::map<DynamicStringA, DynamicStringA, _CaseInsensitiveLess>::iterator it = m_Properties.find(Key);
				if (it == m_Properties.end())
					return DynamicStringA();
				return it->second;
			}

			const bool IsPropertySet(const DynamicStringA& Key)
			{
				return m_Properties.find(Key) != m_Properties.end();
			}

			void SetProperty(const DynamicStringA& key, const DynamicStringA& Value = DynamicStringA())
			{
				m_Properties[key] = Value;
				m_Modified = true;
			}

			void SetDescription(const TempStringA& Description)
			{
				m_Description = Description;
				m_Modified = true;
			}

			void FlagAsCopy()
			{
				m_OriginalString.clear();
				m_Modified = true;
			}

			bool IsNewEntry() const
			{
				return m_Modified && m_OriginalString.empty();
			}

		public:
			virtual DynamicStringA BuildNewString() const
			{
				DynamicStringA ret = GetPath();
				ret.AppendFormat("=\"%s\"", GetDescription().c_str());
				for each (const std::pair<DynamicStringA, DynamicStringA> &kv in m_Properties)
				{
					if (!kv.second.empty())
						ret.AppendFormat(" /%s=%s", kv.first.c_str(), kv.second.c_str());
					else
						ret.AppendFormat(" /%s", kv.first.c_str());
				}
				return ret;
			}

		};


		SimpleEntry m_Timeout, m_Default;
		unsigned m_PendingEntry, m_DefaultEntryIndex;

		std::vector<OSEntry> m_OSEntries;

		bool m_bValid;

#define BOOT_INI_FILE_NAME	_T("c:\\boot.ini")
#define BOOT_BAK_FILE_NAME	_T("c:\\boot.bak") // =BC= previous: _T("c:\\boot.vkd")

	public:
		BootConfigEditor()
			: m_PendingEntry(0)
			, m_bValid(false)
			, m_DefaultEntryIndex(-1)
		{
			ManagedPointer<TextANSIFileReader> pBootIni = new TextANSIFileReader(new ACFile(BOOT_INI_FILE_NAME, FileModes::OpenReadOnly));
			if (!pBootIni->Valid())
				return;

			enum CurrentSection
			{
				csUnknown,
				csLoader,
				csOpSystems,
			} curSection = csUnknown;

			while (!pBootIni->IsEOF())
			{
				DynamicStringA line = pBootIni->ReadLine();
				if (line.empty())
					continue;
				if (line[0] == '[')
				{
					if (!line.icompare("[boot loader]"))
						curSection = csLoader;
					else if (!line.icompare("[operating systems]"))
						curSection = csOpSystems;
					else
						curSection = csUnknown;
					continue;
				}
				if (curSection == csLoader)
				{
					DynamicStringA key = SimpleEntry::GetKey(line);
					if (!key.icompare("timeout"))
						m_Timeout = SimpleEntry(line);
					else if (!key.icompare("default"))
						m_Default = SimpleEntry(line);
				}
				else if (curSection == csOpSystems)
				{
					OSEntry entry(line);
					if (entry.Valid())
						m_OSEntries.push_back(entry);
				}
			}
			if (!m_OSEntries.empty() && !m_Default.GetValue().empty() && !m_Timeout.GetValue().empty())
				m_bValid = true;
		}

		virtual ActionStatus StartEnumeration()
		{
			m_PendingEntry = 0;
			return MAKE_STATUS(Success);
		}

		virtual ManagedPointer<AIBootConfigurationEntry> GetNextEntry()
		{
			if (m_PendingEntry >= m_OSEntries.size())
				return NULL;
			return new BootEntry(ManagedPointer<BootConfigEditor>(AUTO_THIS), m_PendingEntry++);
		}

		virtual size_t GetEntryCount()
		{
			return m_OSEntries.size();
		}

		virtual ManagedPointer<AIBootConfigurationEntry> CopyEntry(const ConstManagedPointer<AIBootConfigurationEntry>& pEntry, bool AddToSelectionList = true, ActionStatus* pStatus = NULL)
		{
			ASSERT(pEntry);
			ManagedPointer<BootEntry> pRawEntry = pEntry;
			if (!pRawEntry)
			{
				ASSIGN_STATUS(pStatus, InvalidParameter);
				return NULL;
			}

			m_OSEntries.push_back(m_OSEntries[pRawEntry->GetEntryIndex()]);

			m_OSEntries[m_OSEntries.size() - 1].FlagAsCopy();

			ASSIGN_STATUS(pStatus, Success);
			return new BootEntry(ManagedPointer<BootConfigEditor>(AUTO_THIS), (int)(m_OSEntries.size() - 1));
		}

		virtual ActionStatus FinishEditing()
		{
			DynamicString tempFileName = BOOT_INI_FILE_NAME;
			tempFileName.AppendFormat(L".tmp%04x", GetCurrentProcessId());

			ActionStatus st;
			ManagedPointer<TextANSIFileReader> pBootIni = new TextANSIFileReader(new ACFile(BOOT_INI_FILE_NAME, FileModes::OpenReadOnly, &st));
			if (!pBootIni->Valid())
			{
				if (!st.Successful())
					return st;
				return MAKE_STATUS(UnknownError);
			}

			File newBootIni(tempFileName, FileModes::CreateOrTruncateRW, &st);
			if (!st.Successful())
				return st;
			if (!newBootIni.Valid())
				return MAKE_STATUS(UnknownError);

			size_t entryCount = 2 + m_OSEntries.size();
			//All changes made to BOOT.INI are represented as a list of (old line -> new line) pairs.
			//This basically replaces all matching 'old lines' with corresponding 'new lines', leaving
			//other parts of BOOT.INI untouched.
			EntryBase** ppEntriesToChange = new EntryBase * [entryCount];

			ppEntriesToChange[0] = &m_Timeout;
			ppEntriesToChange[1] = &m_Default;
			for (size_t i = 0; i < m_OSEntries.size(); i++)
			{
				if (m_OSEntries[i].IsModified() && !m_OSEntries[i].GetOriginalString().empty() && (m_DefaultEntryIndex != i))
					ppEntriesToChange[2 + i] = &m_OSEntries[i];
				else
					ppEntriesToChange[2 + i] = NULL;
			}

			OSEntry* pNewDefaultEntry = NULL;
			if (m_DefaultEntryIndex != -1)
				pNewDefaultEntry = &m_OSEntries[m_DefaultEntryIndex];

			bool bOSSectionStarted = false, bNewEntriesAdded = false;

			//Each line of BOOT.INI is processed separately
			while (!pBootIni->IsEOF())
			{
				DynamicStringA line = pBootIni->ReadLine();

				bool lineReplaced = false;
				ActionStatus status;

				//Replace the line, if it matches one of the changed entries
				if (!line.empty() && (line[0] != '['))
				{
					if (pNewDefaultEntry && (pNewDefaultEntry->GetOriginalString() == line))
						continue;	//Default entry is always placed first and is removed from its old position

					for (size_t i = 0; i < entryCount; i++)
					{
						if (!ppEntriesToChange[i] || !ppEntriesToChange[i]->IsModified())
							continue;
						if (!line.compare(ppEntriesToChange[i]->GetOriginalString()))
						{
							newBootIni.WriteLine(ppEntriesToChange[i]->BuildNewString(), &status);
							lineReplaced = true;
							break;
						}
					}
				}

				//Insert all new entries BEFORE the line, if it is an empty line in the [operating systems] section (or a 'C:\xxx' line)
				if (bOSSectionStarted && !bNewEntriesAdded)
				{
					if (line.empty() || ((line.length() >= 2) && line[1] == ':'))
					{
						for each (const OSEntry & entry in m_OSEntries)
							if (entry.IsNewEntry() && (pNewDefaultEntry != &entry))
								newBootIni.WriteLine(entry.BuildNewString());
						bNewEntriesAdded = true;
					}
				}

				//Write the line, if it was not changed
				if (!lineReplaced)
					newBootIni.WriteLine(line, &status);

				//Insert default entry right after the [operating systems] line
				if (pNewDefaultEntry && (line == "[operating systems]"))
				{
					bOSSectionStarted = true;
					newBootIni.WriteLine(pNewDefaultEntry->IsModified() ? pNewDefaultEntry->BuildNewString() : pNewDefaultEntry->GetOriginalString());
				}

				if (!status.Successful())
				{
					delete ppEntriesToChange;
					break;
				}

			}

			if (!bNewEntriesAdded)
			{
				for each (const OSEntry & entry in m_OSEntries)
					if (entry.IsNewEntry() && (pNewDefaultEntry != &entry))
						newBootIni.WriteLine(entry.BuildNewString());
			}

			newBootIni.Close();
			delete ppEntriesToChange;

			pBootIni = NULL;

			if (File::Exists(BOOT_BAK_FILE_NAME))
				File::Delete(BOOT_BAK_FILE_NAME, true);

			if (!MoveFile(BOOT_INI_FILE_NAME, BOOT_BAK_FILE_NAME))
			{
				DWORD err = GetLastError();
				DeleteFile(tempFileName.c_str());
				return MAKE_STATUS(ActionStatus::FromWin32Error(err));
			}

			if (!MoveFile(tempFileName.c_str(), BOOT_INI_FILE_NAME))
			{
				DWORD err = GetLastError();
				MoveFile(BOOT_BAK_FILE_NAME, BOOT_INI_FILE_NAME);
				DeleteFile(tempFileName.c_str());
				return MAKE_STATUS(ActionStatus::FromWin32Error(err));
			}

			DWORD attr = ::GetFileAttributes(BOOT_BAK_FILE_NAME);
			if (attr != INVALID_FILE_ATTRIBUTES)
				::SetFileAttributes(BOOT_INI_FILE_NAME, attr);

			return MAKE_STATUS(Success);
		}

		bool Valid()
		{
			return m_bValid;
		}

		virtual ActionStatus SetDefaultEntry(const ConstManagedPointer<AIBootConfigurationEntry>& pEntry)
		{
			ASSERT(pEntry);
			ManagedPointer<BootEntry> pRawEntry = pEntry;
			if (!pRawEntry)
				return MAKE_STATUS(InvalidParameter);

			DynamicStringA defaultPath = m_OSEntries[pRawEntry->GetEntryIndex()].GetPath();
			if (defaultPath.empty())
				return MAKE_STATUS(InvalidParameter);

			m_DefaultEntryIndex = pRawEntry->GetEntryIndex();

			m_Default.SetValue(defaultPath);
			return MAKE_STATUS(Success);
		}

		virtual ActionStatus SetTimeout(unsigned TimeoutInSeconds)
		{
			m_Timeout.SetValue(TimeoutInSeconds);
			return MAKE_STATUS(Success);
		}

		virtual unsigned GetTimeout(ActionStatus* pStatus = NULL)
		{
			ASSIGN_STATUS(pStatus, Success);
			return m_Timeout.ToInteger();
		}

		friend class BootEntry;

	};


	BazisLib::String BootEntry::GetDescription()
	{
		ASSERT(m_pEditor);
		ASSERT(m_EntryIndex < m_pEditor->m_OSEntries.size());
		return ANSIStringToString(m_pEditor->m_OSEntries[m_EntryIndex].GetDescription());
	}

	BazisLib::ActionStatus BootEntry::SetDescription(LPCWSTR pDescription)
	{
		ASSERT(m_pEditor);
		ASSERT(m_EntryIndex < m_pEditor->m_OSEntries.size());
		m_pEditor->m_OSEntries[m_EntryIndex].SetDescription(StringToANSIString(TempStrPointerWrapper(pDescription)));
		return MAKE_STATUS(Success);
	}

	bool BootEntry::IsCurrentOS()
	{
		ASSERT(m_pEditor);
		ASSERT(m_EntryIndex < m_pEditor->m_OSEntries.size());
		DynamicStringA path = m_pEditor->m_OSEntries[m_EntryIndex].GetPath();
		SplitStrByFirstOfA deviceAndDir(path, ConstStringA("\\"));
		if (!deviceAndDir)
			return false;

		RegistryKey key(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control"));
		DynamicStringA systemBootDevice = StringToANSIString(key[_T("SystemBootDevice")]);

		if (systemBootDevice.empty() && m_pEditor->GetEntryCount() == 1)
		{
			// Old version of Windows that predates SystemBootDevice. Since there is only one entry, its generally safe
			// to say that this is the correct entry. This should be safe to set since it wasn't used.
			key[L"SystemBootDevice"].SetValueEx(ANSIStringToString(deviceAndDir.left).c_str(), (DWORD)(deviceAndDir.left.length() + 1) * sizeof(WCHAR), REG_SZ);
		}
		else
		{
			if (systemBootDevice.icompare(deviceAndDir.left))
				return false;
		}

		char szWinDir[MAX_PATH] = { 0, };
		GetWindowsDirectoryA(szWinDir, __countof(szWinDir));

		if (deviceAndDir.right.icompare(&szWinDir[3]))
			return false;

		return true;
	}

	BootEditor::KernelDebuggerMode BootEntry::GetDebuggerMode()
	{
		ASSERT(m_pEditor);
		ASSERT(m_EntryIndex < m_pEditor->m_OSEntries.size());

		if (!m_pEditor->m_OSEntries[m_EntryIndex].IsPropertySet("DEBUG"))
			return kdNone;

		DynamicStringA port = m_pEditor->m_OSEntries[m_EntryIndex].GetProperty("DEBUGPORT");
		if (port.empty())
			return kdUnknown;

		if (!port.icompare("1394") || !port.icompare(0, 3, "COM"))
			return kdStandard;

		return kdCustom;
	}

	BazisLib::String BootEntry::GetCustomKDName()
	{
		ASSERT(m_pEditor);
		ASSERT(m_EntryIndex < m_pEditor->m_OSEntries.size());

		String kdBase = ANSIStringToString(m_pEditor->m_OSEntries[m_EntryIndex].GetProperty("DEBUGPORT"));
		if (kdBase.empty())
			return kdBase;

		kdBase.insert(0, _T("kd"));
		kdBase.append(_T(".dll"));

		return kdBase;
	}

	BazisLib::ActionStatus BootEntry::EnableCustomKD(LPCWSTR pCustomKDName)
	{
		if (!pCustomKDName || (towupper(pCustomKDName[0]) != 'K') || (towupper(pCustomKDName[1]) != 'D'))
			return MAKE_STATUS(InvalidParameter);

		DynamicStringA customKDName = StringToANSIString(TempStrPointerWrapperW(pCustomKDName + 2));
		if ((customKDName.size() < 4) || (customKDName.substr(customKDName.size() - 4).icompare(".dll")))
			return MAKE_STATUS(InvalidParameter);

		ASSERT(m_pEditor);
		ASSERT(m_EntryIndex < m_pEditor->m_OSEntries.size());

		m_pEditor->m_OSEntries[m_EntryIndex].SetProperty("DEBUG");
		m_pEditor->m_OSEntries[m_EntryIndex].SetProperty("DEBUGPORT", customKDName.substr(0, customKDName.size() - 4));
		return MAKE_STATUS(Success);
	}
}

ManagedPointer<AIBootConfigurationEditor> BootEditor::CreateConfigurationEditor()
{
	// OSVERSIONINFO version = { sizeof(version) }; // =BC= deprecated
	// GetVersionEx(&version); // =BC= deprecated
	if (::IsVistaOrLater()) //We are running Vista or higher // =BC= previous expression: version.dwMajorVersion >= 6
	{
		ManagedPointer<VistaBCD::BootConfigEditor> pEditor = new VistaBCD::BootConfigEditor();
		if (!pEditor->Valid())
			return NULL;
		return pEditor;
	}
	else	//We are running on XP or similar and are modifying BOOT.INI
	{
		ManagedPointer<BootIniEditor::BootConfigEditor> pEditor = new BootIniEditor::BootConfigEditor();
		if (!pEditor->Valid())
			return NULL;
		return pEditor;
	}
}

using namespace BazisLib::Win32::BCD;
using namespace BootEditor;

ActionStatus FindBestOSEntry(ManagedPointer<AIBootConfigurationEntry>* ppEntry, bool* pbAlreadyInstalled, ManagedPointer<AIBootConfigurationEditor> pEditor)
{
	ASSERT(ppEntry);

	*ppEntry = NULL;

	if (!pEditor)
		pEditor = CreateConfigurationEditor();
	if (!pEditor)
		return MAKE_STATUS(UnknownError);

	ActionStatus st = pEditor->StartEnumeration();
	if (!st.Successful())
		return st;

	if (pbAlreadyInstalled)
		*pbAlreadyInstalled = false;

	ManagedPointer<AIBootConfigurationEntry> pEntry, pCurrentOSEntry;
	while (pEntry = pEditor->GetNextEntry())
	{
		if (pEntry->IsCurrentOS())
		{
			if (!pCurrentOSEntry)
				pCurrentOSEntry = pEntry;

			if (pEntry->GetDebuggerMode() == kdCustom)
			{
				WCHAR wcsBuf[MAX_PATH];
				String strFullPath;
				DWORD dwVerSize;
				DWORD dwVerSizeHandle = 0;
				String strData;
				UINT uiValLen = 0;
				WCHAR* wcsProductName;

				if (GetSystemDirectoryW(wcsBuf, _countof(wcsBuf)) == FALSE)
					continue;

				strFullPath = Path::Combine(wcsBuf, pEntry->GetCustomKDName());
				dwVerSize = GetFileVersionInfoSizeW(strFullPath.c_str(), &dwVerSizeHandle);

				if (!dwVerSize)
					continue;

				strData.resize(dwVerSize, L'\0');

				if (GetFileVersionInfoW(strFullPath.c_str(), 0, dwVerSize, (LPVOID)strData.data()) == FALSE)
					continue;

				if (VerQueryValueW(strData.data(), L"\\StringFileInfo\\000904B0\\ProductName", (LPVOID*)&wcsProductName, &uiValLen) == FALSE)
					continue;

				if (wcscmp(wcsProductName, KDBC_DLL_PRODUCT_NAME) != 0) // =BC= previous: L"VirtualKD-Redux"
					continue;

				pCurrentOSEntry = pEntry;
				if (pbAlreadyInstalled)
					*pbAlreadyInstalled = true;
				break;
			}
		}
	}

	*ppEntry = pCurrentOSEntry;

	if (!pCurrentOSEntry)
		return MAKE_STATUS(NotSupported);
	return MAKE_STATUS(Success);
}

ActionStatus CreateVirtualKDBootEntry(bool CreateNewEntry, bool SetNewEntryDefault, LPCWSTR lpEntryName, unsigned Timeout, bool replacingKdcom)
{
	ManagedPointer<AIBootConfigurationEditor> pEditor = CreateConfigurationEditor();
	if (!pEditor)
		return MAKE_STATUS(UnknownError);

	bool alreadyInstalled = false;
	ManagedPointer<AIBootConfigurationEntry> pEntry;

	ActionStatus st = FindBestOSEntry(&pEntry, &alreadyInstalled, pEditor);
	if (!st.Successful())
		return st;

	if (!pEntry)
		return MAKE_STATUS(NotSupported);

	if (CreateNewEntry && !alreadyInstalled)
	{
		pEntry->ExplicitlyDisableDebugging();
		pEntry = pEditor->CopyEntry(pEntry, true, &st);
	}

	if (!pEntry)
		return MAKE_STATUS(NotSupported);

	st = pEntry->EnableCustomKD(replacingKdcom ? L"kdcom.dll" : L"kdbc.dll"); // =BC= previous: L"kdbazis.dll"
	if (!st.Successful())
		return COPY_STATUS(st);

	if (!st.Successful())
		return COPY_STATUS(st);
	if (!pEntry)
		return MAKE_STATUS(UnknownError);

	if (lpEntryName)
	{
		st = pEntry->SetDescription(lpEntryName);
		if (!st.Successful())
			return COPY_STATUS(st);
	}

	if (SetNewEntryDefault)
	{
		st = pEditor->SetDefaultEntry(pEntry);
		if (!st.Successful())
			return COPY_STATUS(st);
	}

	if (Timeout != -1)
	{
		st = pEditor->SetTimeout(Timeout);
		if (!st.Successful())
			return COPY_STATUS(st);
	}

	return pEditor->FinishEditing();
}

static BOOL VerifyFileProductName(String strFullPath)
{
	DWORD dwVerSize;
	DWORD dwVerSizeHandle = 0;
	String strData;
	UINT uiValLen = 0;
	WCHAR* wcsProductName;

	dwVerSize = GetFileVersionInfoSizeW(strFullPath.c_str(), &dwVerSizeHandle);
	if (!dwVerSize)
		return FALSE;

	strData.resize(dwVerSize, L'\0');

	if (GetFileVersionInfoW(strFullPath.c_str(), 0, dwVerSize, (LPVOID)strData.data()) == FALSE)
		return FALSE;

	if (VerQueryValueW(strData.data(), L"\\StringFileInfo\\000904B0\\ProductName", (LPVOID*)&wcsProductName, &uiValLen) == FALSE)
		return FALSE;

	if (wcscmp(wcsProductName, KDBC_DLL_PRODUCT_NAME) != 0) // =BC= previous: L"VirtualKD-Redux"
		return FALSE;

	return TRUE;
}

void KdcomInstall::Install(HWND hWnd)
{
	// get the full path of the source file (kdbc_XXbit.dll) and verify it is present and valid.

	auto dllPath = Utils::GetExeFolderWithBackSlash() + L"kdbc_";

	if (Utils::Is32bitOS())
		dllPath += L"32bit";
	else
		dllPath += L"64bit";

	dllPath += L".dll";

	if (!VerifyFileProductName(dllPath.c_str()))
	{
		::MessageBoxW(hWnd,
			(L"File not found or invalid: \"" + dllPath + L"\".").c_str(),
			NULL,
			MB_ICONERROR);
		return;
	}

	// replace kdcom or copy our kdcom.

	BOOL replaceKdcom = ::IsWin10OrLater();

	String fp = Path::Combine(Path::GetSpecialDirectoryLocation(dirSystem), replaceKdcom ? ConstString(_T("kdcom.dll")) : ConstString(_T("kdbc.dll"))); // =BC= previous: _T("kdbazis.dll")

#ifdef _WIN64
	bool is64Bit = true;
#else
	bool is64Bit = BazisLib::WOW64APIProvider::sIsWow64Process();
#endif

	{
		WOW64FSRedirHolder holder;

		// replace kdcom.

		if (replaceKdcom)
		{
			ActionStatus st;

			String kdcomBackup = Path::Combine(Path::GetSpecialDirectoryLocation(dirSystem), L"kdcom_old.dll");

			for (DWORD i = 2; File::Exists(kdcomBackup); ++i)
			{
				String kdcomBackupCurrentName;
				kdcomBackupCurrentName.AppendFormat(L"kdcom_old_%u.dll", i);
				kdcomBackup = Path::Combine(Path::GetSpecialDirectoryLocation(dirSystem), kdcomBackupCurrentName);
			}

			st = TakeOwnership(const_cast<LPTSTR>(String(fp).c_str()));
			if (!st.Successful())
			{
				::MessageBox(hWnd,
					String::sFormat(_T("Cannot replace owner on kdcom.dll: %s"), st.GetMostInformativeText().c_str()).c_str(),
					NULL,
					MB_ICONERROR);
				return;
			}

			Win32::Security::TranslatedAcl dacl = File::GetDACLForPath(fp, &st);
			if (!st.Successful())
			{
				::MessageBox(hWnd,
					String::sFormat(_T("Cannot query permissions on kdcom.dll: %s"), st.GetMostInformativeText().c_str()).c_str(),
					NULL,
					MB_ICONERROR);
				return;
			}
			dacl.AddAllowingAce(STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL, BazisLib::Win32::Security::Sid::CurrentUserSid());
			st = File::SetDACLForPath(fp, dacl);
			if (!st.Successful())
			{
				::MessageBox(hWnd,
					String::sFormat(_T("Cannot set permissions on kdcom.dll: %s"), st.GetMostInformativeText().c_str()).c_str(),
					NULL,
					MB_ICONERROR);
				return;
			}

			if (!MoveFile(fp.c_str(), kdcomBackup.c_str()))
			{
				::MessageBox(hWnd,
					String::sFormat(_T("Cannot rename old kdcom.dll: %s"), MAKE_STATUS(ActionStatus::FromLastError()).GetMostInformativeText().c_str()).c_str(),
					NULL,
					MB_ICONERROR);
				return;
			}
		}

		// copy our kdcom.

		if (!::CopyFileW(dllPath.c_str(), fp.c_str(), FALSE))
		{
			::MessageBox(hWnd,
				String::sFormat(_T("Cannot copy new kdcom.dll: %s"), MAKE_STATUS(ActionStatus::FromLastError()).GetMostInformativeText().c_str()).c_str(),
				NULL,
				MB_ICONERROR);
			return;
		}
	}

	// create the boot entry.

	{
		bool createNewEntry = true;
		bool setDefault = false;
		String entryName = L"BugChecker (Press F8 to disable DSE)";
		unsigned timeout = 10;

		ActionStatus st = CreateVirtualKDBootEntry(createNewEntry, setDefault, entryName.c_str(), timeout, replaceKdcom);
		if (!st.Successful())
		{
			::MessageBox(hWnd,
				String::sFormat(_T("Cannot create VirtualKD-Redux boot entry: %s"), st.GetMostInformativeText().c_str()).c_str(),
				NULL,
				MB_ICONERROR);
			return;
		}
	}

	// show a success message.

	::MessageBoxA(hWnd, "Operation completed successfully.", "Ok", MB_OK);
}
