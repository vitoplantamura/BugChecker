#pragma once

#include "strbase.h"

namespace BazisLib
{
	template<class _Elem,
	class _Traits> inline
		BazisLib::_DynamicStringT<_Elem, _Traits> operator+(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (BazisLib::_DynamicStringT<_Elem, _Traits>(_Left) += _Right);
	}

	template<class _Elem,
	class _Traits> inline
		BazisLib::_DynamicStringT<_Elem, _Traits> operator+(
		const _Elem *_Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (BazisLib::_DynamicStringT<_Elem, _Traits>(_Left) += _Right);
	}

	template<class _Elem,
	class _Traits> inline
		BazisLib::_DynamicStringT<_Elem, _Traits> operator+(
		const _Elem _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (BazisLib::_DynamicStringT<_Elem, _Traits>(1, _Left) += _Right);
	}

	template<class _Elem,
	class _Traits> inline
		BazisLib::_DynamicStringT<_Elem, _Traits> operator+(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const _Elem *_Right)
	{
		return (BazisLib::_DynamicStringT<_Elem, _Traits>(_Left) += _Right);
	}

	template<class _Elem,
	class _Traits> inline
		BazisLib::_DynamicStringT<_Elem, _Traits> operator+(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const _Elem _Right)
	{
		return (BazisLib::_DynamicStringT<_Elem, _Traits>(_Left) += _Right);
	}

	template<class _Elem,
	class _Traits> inline
		bool operator==(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		if (_Left.length() != _Right.length())
			return false;
		return (_Left.compare(_Right) == 0);
	}

	template<class _Elem,
	class _Traits> inline
		bool operator==(
		const _Elem * _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (_Right.compare(_Left) == 0);
	}

	template<class _Elem,
	class _Traits> inline
		bool operator==(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const _Elem *_Right)
	{
		return (_Left.compare(_Right) == 0);
	}

	template<class _Elem,
	class _Traits> inline
		bool operator!=(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (!(_Left == _Right));
	}

	template<class _Elem,
	class _Traits> inline
		bool operator!=(
		const _Elem *_Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (!(_Left == _Right));
	}

	template<class _Elem,
	class _Traits> inline
		bool operator!=(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const _Elem *_Right)
	{
		return (!(_Left == _Right));
	}

	template<class _Elem,
	class _Traits> inline
		bool operator<(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (_Left.compare(_Right) < 0);
	}

	template<class _Elem,
	class _Traits> inline
		bool operator<(
		const _Elem * _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (_Right.compare(_Left) > 0);
	}

	template<class _Elem,
	class _Traits> inline
		bool operator<(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const _Elem *_Right)
	{
		return (_Left.compare(_Right) < 0);
	}

	template<class _Elem,
	class _Traits> inline
		bool operator>(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (_Right < _Left);
	}

	template<class _Elem,
	class _Traits> inline
		bool operator>(
		const _Elem * _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (_Right < _Left);
	}

	template<class _Elem,
	class _Traits> inline
		bool operator>(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const _Elem *_Right)
	{
		return (_Right < _Left);
	}

	template<class _Elem,
	class _Traits> inline
		bool operator<=(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (!(_Right < _Left));
	}

	template<class _Elem,
	class _Traits> inline
		bool operator<=(
		const _Elem * _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (!(_Right < _Left));
	}

	template<class _Elem,
	class _Traits> inline
		bool operator<=(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const _Elem *_Right)
	{
		return (!(_Right < _Left));
	}

	template<class _Elem,
	class _Traits> inline
		bool operator>=(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (!(_Left < _Right));
	}

	template<class _Elem,
	class _Traits> inline
		bool operator>=(
		const _Elem * _Left,
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Right)
	{
		return (!(_Left < _Right));
	}

	template<class _Elem,
	class _Traits> inline
		bool operator>=(
		const BazisLib::_TempStringImplT<_Elem, _Traits>& _Left,
		const _Elem *_Right)
	{
		return (!(_Left < _Right));
	}
}
