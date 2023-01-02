#pragma once

#include "cmndef.h"

namespace BazisLib
{
	//! Provides exponential filtration for some data stream
	/*! The _ExponentialFilter template allows filtering some set of values according to
		the exponential law (value = old + delta * coef, where coef < 1)
		\param _T Represents value type. Can be double, int, LONGLONG, etc.
		\param _Divider Represents the divider part of the coefficient the multiplier
						is specified on construction and can be changed during operation.
						The real coefficient is determined as Coef / _Divider.
						This parameter allows avoiding floating point operations for integer
						data types by specifying filtering coefficient as integer divided
						by constant integer (template parameter).
		\remarks Note that this type of filter is less accurate than sliding average filter, however
				 it consumes less memory and is faster and simplier. The typical use case is the following:
<pre>
ExponentialFilterI32 filter(500);	//500/1000 means 1/2, 1000 is the divider set for ExponentialFilterI32
...
for (...)
{
	...
	int val = GetNextValue();
	val = filter.UpdateValue(val);
	DoSomethingWithNextValue(val);
	...
}
</pre>
	*/
	template <class _T, _T _Divider> class _ExponentialFilter
	{
	private:
		_T m_Value;
		_T m_FilteringCoef;

	public:
		_ExponentialFilter(_T FilteringCoef) :
			m_FilteringCoef(FilteringCoef),
			m_Value(0)
		{
		}

		//! Forcibly resets the filtered value
		void ResetValue(_T Value)
		{
			m_Value = Value;
		}

		//! Updates the filtering coefficient
		/*! This method updates the filtering coefficient.
			\param Coef Specifies the coefficient. The real one equals to
				   Coef / _Divider, where _Divider is the template parameter.
		*/
		void SetCoef(_T Coef)
		{
			m_FilteringCoef = Coef;
		}

		//! Takes the new value and returns the filtered one
		_T UpdateValue(_T Value)
		{
			_T dif = Value - m_Value;
			dif = (dif * m_FilteringCoef) / _Divider;
			m_Value += dif;
			return m_Value;
		}

		//! Returns last filtered value
		_T GetValue()
		{
			return m_Value;
		}
	};

	typedef _ExponentialFilter<int, 1000> ExponentialFilterI32;
	typedef _ExponentialFilter<LONGLONG, 1000> ExponentialFilterI64;
}