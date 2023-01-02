/*
	BZSREG.H - Fast registry access class.
*/

#pragma once

#include "cmndef.h"
#include <string>
#include <map>
#include "../../bzscore/string.h"

#ifndef _BZSREG_CPP_

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : warning Msg: "

#pragma message(__LOC__"The BZSREG.H interface is deprecated. include "../../bzscore/registry.h" instead.")

#undef __STR2__
#undef __STR1__
#undef __LOC__

#endif //_BZSREG_CPP_

namespace BazisLib
{

	class IRegistryParameters
	{
	public:
		virtual String GetString(LPCTSTR pszKey)=0;
		virtual int GetInteger(LPCTSTR pszKey) =0;
		virtual bool SetInteger(LPCTSTR pszKey, int Value)=0;
		virtual bool SetString(LPCTSTR pszKey, String &Value)=0;

		virtual bool LoadKey(LPCTSTR pszKey)=0;
		virtual bool SaveKey(LPCTSTR pszKey = 0)=0;
		virtual void Reset()=0;
	};

	class CRegistryParameters
	{
	protected:
		std::map <String, String> m_StringParams;
		std::map <String, int> m_IntParams;
		String m_KeyName;

	public:
		CRegistryParameters(LPCTSTR pszKey);
		virtual String GetString(LPCTSTR pszKey);
		virtual int GetInteger(LPCTSTR pszKey);
		virtual bool SetInteger(LPCTSTR pszKey, int Value);
		virtual bool SetString(LPCTSTR pszKey, String &Value);

		virtual bool LoadKey(LPCTSTR pszKey);
		virtual bool SaveKey(LPCTSTR pszKey = 0);
		virtual void Reset();
	};

}