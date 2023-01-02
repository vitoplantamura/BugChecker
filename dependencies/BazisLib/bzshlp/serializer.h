#pragma once
#include "cmndef.h"
#include "../bzscore/string.h"

namespace BazisLib
{
	//! Serializer-related API
	/*! \namespace BazisLib::Serializer This namespace is reserved for serializer-related API.
		For details about serializer, see \ref serializer_basics "this article".
	*/

	/*! \page serializer_basics Serializer concepts
		The serializer subsystem allows declaring serializeable types, that can be easily
		memberwise saved and loaded to various storage entities, such as registry,
		<b>in a totally human-readable format</b>.
		To declare such a structure, you should use the DECLARE_SERIALIZEABLE_STRUCx() macro,
		where <b>x</b> represents the number of fields. Alternatively, you can use the
		DECLARE_SERIALIZEABLE_STRUCx_I() macro to declare structures with default field
		values (will be placed into generated default constructor).
		<b>The main idea of serializer is to provide a human-readable representation of
		certain internal program structures on some storage device (such as registry).
		The serializer should not be used for just saving some data into binary files.
		For that purpose C/C++ allow using extremely fast pointer-based save/load and
		other techniques.</b>
		A serializeable structure will always have a default constructor and a constructor
		with parameters directly reflecting the field list. To serialize such structure use
		the SERIALIZE_OBJECT() macro. To deserialize - DESERIALIZE_OBJECT().
		Here is a simple usage example:
<pre>
DECLARE_SERIALIZEABLE_STRUC4(SomeParams,
							int,	SomeIntVal,
							unsigned,	SomeOtherIntVal,
							String, SomeStringVal,
							int,		SomeOtherVal);
//---- OR ----
DECLARE_SERIALIZEABLE_STRUC4_I(SomeParams,
							int,	SomeIntVal, 123,
							unsigned,	SomeOtherIntVal, 456,
							String, SomeStringVal, _T("InitialStringValue"),
							int,		SomeOtherVal, 789);
...
void SomeFunc()
{
	SomeParams params(1, 2, _T("TestString"), 4);
	RegistryKey key(HKEY_LOCAL_MACHINE, _T(...));
	key.SerializeObject(params);
	params = SomeParams();
	key.DeserializeObject(params);
}
</pre>
		 You can see the serializer usage example in SAMPLES\\WINDOWS\\REGISTRY.
		 \remarks
		 To develop your own class capable of serialization and deserialization of structure,
		 you should add a _SerializerEntry method to it. Additionally you should add the 
		 serialization and deserialization methods. However, you can use the following macros
		 to implement standard methods:
		 <ul>
			<li>IMPLEMENT_STD_SERIALIZE_METHOD</li>
			<li>IMPLEMENT_STD_DESERIALIZE_METHOD</li>
			<li>IMPLEMENT_EMPTY_SERIALIZE_WRAPPER</li>
			<li>IMPLEMENT_EMPTY_DESERIALIZE_WRAPPER</li>
		 </ul>
		 A typical call sequence for a serialization/deserialization operation is the following:
		 <ul>
			<li>SomeSerializer::SerializeObject()</li> is called by user code
			<li>SomeSerializer::BeginSerialization()</li> performs some initialization, if required
			<li>SerializedType::__SERIALIZER_ITERATE_MEMBERS()</li> iterates all over the fields of a structure
			<li>SomeSerializer::_SerializerEntry()</li> is called for each serialized field
			<li>SomeSerializer::EndSerialization()</li> performs some finalization, if required
		 </ul>
		 Normally, most of the serializer calls are inlined, so that should not be a significant overhead.
		 For details, see the  BazisLib::Win32::RegistryEntry::_SerializerEntry.
		 \attention The serialized structures may only contain fields of the following types:
		 <ul>
			<li>int</li>
			<li>unsigned</li>
			<li>bool</li>
			<li>String</li>
			<li>LONGLONG</li>
			<li>ULONGLONG</li>
		 </ul>
		*/
	namespace Serializer
	{
		//! Returns a name for serializeable structure
		template <class _Ty> const TCHAR *GetClassName(_Ty &ref)
		{
			return __SERIALIZER_GET_TYPE_NAME(&ref);
		}

		static inline const wchar_t *GetPrintfFormatForType(int *pObj) {return L"%d";}
		static inline const wchar_t *GetPrintfFormatForType(unsigned *pObj) {return L"%d";}
		static inline const wchar_t *GetPrintfFormatForType(LONGLONG *pObj) {return L"%d";}
		static inline const wchar_t *GetPrintfFormatForType(ULONGLONG *pObj) {return L"%d";}

#ifdef BAZISLIB_X64
		static inline const wchar_t *GetPrintfFormatForType(PVOID *pObj) {return L"%016I64X";}
#else
		static inline const wchar_t *GetPrintfFormatForType(PVOID *pObj) {return L"%08X";}
#endif

	}
}

template <class _Type> static inline _Type __SERIALIZER_DEFAULT()
{
	return _Type();
}

struct _SerializableStructBase {};

#include "../bzscore/status.h"
#include "ser_def.h"
