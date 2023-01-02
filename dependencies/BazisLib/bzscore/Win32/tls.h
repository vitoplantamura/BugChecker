#pragma once

namespace BazisLib
{
	namespace Win32
	{
		/*! Represents a single TLS value.
			\param _BaseType specifies a 32-bit base type
		*/
		template <class _BaseType> class _TLSValue
		{
		protected:
			DWORD m_tlsID;

		public:
			_TLSValue(_BaseType value = _BaseType()) :
			  m_tlsID(TlsAlloc())
			{
				TlsSetValue(m_tlsID, (LPVOID)(INT_PTR)value);
			}

			~_TLSValue()
			{
				TlsFree(m_tlsID);
			}

			operator _BaseType()
			{
				return (_BaseType)(INT_PTR)TlsGetValue(m_tlsID);
			}

			_BaseType operator=(_BaseType value)
			{
				return TlsSetValue(m_tlsID, (LPVOID)(INT_PTR)value);
			}
		};

		template <class _BaseType> class _TLSNumericValue : public _TLSValue<_BaseType>
		{
		public:
			_BaseType operator+=(_BaseType value)
			{
				_BaseType pval = (_BaseType)(INT_PTR)TlsGetValue(_TLSValue<_BaseType>::m_tlsID);
				TlsSetValue(_TLSValue<_BaseType>::m_tlsID, (LPVOID)(INT_PTR)(pval += value));
				return pval;
			}

			_BaseType operator-=(_BaseType value)
			{
				_BaseType pval = (_BaseType)(INT_PTR)TlsGetValue(_TLSValue<_BaseType>::m_tlsID);
				TlsSetValue(_TLSValue<_BaseType>::m_tlsID, (LPVOID)(INT_PTR)(pval -= value));
				return pval;
			}

			_BaseType operator*=(_BaseType value)
			{
				_BaseType pval = (_BaseType)(INT_PTR)TlsGetValue(_TLSValue<_BaseType>::m_tlsID);
				TlsSetValue(_TLSValue<_BaseType>::m_tlsID, (LPVOID)(INT_PTR)(pval *= value));
				return pval;
			}

			_BaseType operator++()
			{
				_BaseType pval = (_BaseType)(INT_PTR)TlsGetValue(_TLSValue<_BaseType>::m_tlsID);
				TlsSetValue(_TLSValue<_BaseType>::m_tlsID, (LPVOID)(INT_PTR)(++pval));
				return pval;
			}

			_BaseType operator++(int)
			{
				_BaseType pval = (_BaseType)(INT_PTR)TlsGetValue(_TLSValue<_BaseType>::m_tlsID);
				TlsSetValue(_TLSValue<_BaseType>::m_tlsID, (LPVOID)(INT_PTR)(pval + 1));
				return pval;
			}

			_BaseType operator--()
			{
				_BaseType pval = (_BaseType)(INT_PTR)TlsGetValue(_TLSValue<_BaseType>::m_tlsID);
				TlsSetValue(_TLSValue<_BaseType>::m_tlsID, (LPVOID)(INT_PTR)(--pval));
				return pval;
			}

			_BaseType operator--(int)
			{
				_BaseType pval = (_BaseType)(INT_PTR)TlsGetValue(_TLSValue<_BaseType>::m_tlsID);
				TlsSetValue(_TLSValue<_BaseType>::m_tlsID, (LPVOID)(INT_PTR)(pval - 1));
				return pval;
			}
		};

		typedef _TLSNumericValue<unsigned> TLSUInt32;
		typedef _TLSNumericValue<int>	   TLSInt32;
	}
}