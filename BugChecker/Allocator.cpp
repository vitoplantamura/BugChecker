#include "Allocator.h"

#include "Platform.h"
#include "Root.h"
#include "Glyph.h"

//=====================================================================================
// 
// SerenityOSBitmap
//
// The functions in this class are from the awesome SerenityOS, specifically from the
// "AK/BitmapView.h" and "AK/Bitmap.h" files. They are fast and tested, so there is no
// point in rewriting them!
//
//=====================================================================================

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

class SerenityOSBitmap
{
public:

	/*
	* Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
	*
	* SPDX-License-Identifier: BSD-2-Clause
	*/

	typedef unsigned char u8;

	static u8* m_data;
	static size_t m_size;

	//

	static constexpr size_t max_size = 0xffffffff;

	static size_t find_next_range_of_unset_bits(size_t& from, size_t min_length = 1, size_t max_length = max_size)
	{
		if (min_length > max_length) {
			return (size_t)-1;
		}

		size_t bit_size = 8 * sizeof(size_t);

		size_t* bitmap = (size_t*)m_data;

		// Calculating the start offset.
		size_t start_bucket_index = from / bit_size;
		size_t start_bucket_bit = from % bit_size;

		size_t* start_of_free_chunks = &from;
		size_t free_chunks = 0;

		for (size_t bucket_index = start_bucket_index; bucket_index < m_size / bit_size; ++bucket_index) {
			if (bitmap[bucket_index] == (size_t)-1) {
				// Skip over completely full bucket of size bit_size.
				if (free_chunks >= min_length) {
					return min(free_chunks, max_length);
				}
				free_chunks = 0;
				start_bucket_bit = 0;
				continue;
			}
			if (bitmap[bucket_index] == 0x0) {
				// Skip over completely empty bucket of size bit_size.
				if (free_chunks == 0) {
					*start_of_free_chunks = bucket_index * bit_size;
				}
				free_chunks += bit_size;
				if (free_chunks >= max_length) {
					return max_length;
				}
				start_bucket_bit = 0;
				continue;
			}

			size_t bucket = bitmap[bucket_index];
			u8 viewed_bits = (u8)start_bucket_bit;
			u32 trailing_zeroes = 0;

			bucket >>= viewed_bits;
			start_bucket_bit = 0;

			while (viewed_bits < bit_size) {
				if (bucket == 0) {
					if (free_chunks == 0) {
						*start_of_free_chunks = bucket_index * bit_size + viewed_bits;
					}
					free_chunks += bit_size - viewed_bits;
					viewed_bits = (u8)bit_size;
				}
				else {
					trailing_zeroes = count_trailing_zeroes(bucket);
					bucket >>= trailing_zeroes;

					if (free_chunks == 0) {
						*start_of_free_chunks = bucket_index * bit_size + viewed_bits;
					}
					free_chunks += trailing_zeroes;
					viewed_bits += trailing_zeroes;

					if (free_chunks >= min_length) {
						return min(free_chunks, max_length);
					}

					// Deleting trailing ones.
					u32 trailing_ones = count_trailing_zeroes(~bucket);
					bucket >>= trailing_ones;
					viewed_bits += trailing_ones;
					free_chunks = 0;
				}
			}
		}

		if (free_chunks < min_length) {
			size_t first_trailing_bit = (m_size / bit_size) * bit_size;
			size_t trailing_bits = m_size % bit_size;
			for (size_t i = 0; i < trailing_bits; ++i) {
				if (!get(first_trailing_bit + i)) {
					if (free_chunks == 0)
						*start_of_free_chunks = first_trailing_bit + i;
					if (++free_chunks >= min_length)
						return min(free_chunks, max_length);
				}
				else {
					free_chunks = 0;
				}
			}
			return {};
		}

		return min(free_chunks, max_length);
	}

	static size_t count_in_range(size_t start, size_t len, bool value)
	{
		// VERIFY(start < m_size);
		// VERIFY(start + len <= m_size);
		if (len == 0)
			return 0;

		size_t count;
		const u8* first = &m_data[start / 8];
		const u8* last = &m_data[(start + len) / 8];
		u8 byte = *first;
		byte &= bitmask_first_byte[start % 8];
		if (first == last) {
			byte &= bitmask_last_byte[(start + len) % 8];
			count = popcount(byte);
		}
		else {
			count = popcount(byte);
			// Don't access *last if it's out of bounds
			if (last < &m_data[size_in_bytes()]) {
				byte = *last;
				byte &= bitmask_last_byte[(start + len) % 8];
				count += popcount(byte);
			}
			if (++first < last) {
				const size_t* ptr_large = (const size_t*)(((FlatPtr)first + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1));
				if ((const u8*)ptr_large > last)
					ptr_large = (const size_t*)last;
				while (first < (const u8*)ptr_large) {
					count += popcount(*first);
					first++;
				}
				const size_t* last_large = (const size_t*)((FlatPtr)last & ~(sizeof(size_t) - 1));
				while (ptr_large < last_large) {
					count += popcount(*ptr_large);
					ptr_large++;
				}
				for (first = (const u8*)ptr_large; first < last; first++)
					count += popcount(*first);
			}
		}

		if (!value)
			count = len - count;
		return count;
	}

	template<bool VALUE, bool verify_that_all_bits_flip = false>
	static void set_range(size_t start, size_t len)
	{
		// VERIFY(start < m_size);
		// VERIFY(start + len <= m_size);
		if (len == 0)
			return;

		u8* first = &m_data[start / 8];
		u8* last = &m_data[(start + len) / 8];
		u8 byte_mask = bitmask_first_byte[start % 8];
		if (first == last) {
			byte_mask &= bitmask_last_byte[(start + len) % 8];
			if constexpr (verify_that_all_bits_flip) {
				if constexpr (VALUE) {
					// VERIFY((*first & byte_mask) == 0);
				}
				else {
					// VERIFY((*first & byte_mask) == byte_mask);
				}
			}
			if constexpr (VALUE)
				*first |= byte_mask;
			else
				*first &= ~byte_mask;
		}
		else {
			if constexpr (verify_that_all_bits_flip) {
				if constexpr (VALUE) {
					// VERIFY((*first & byte_mask) == 0);
				}
				else {
					// VERIFY((*first & byte_mask) == byte_mask);
				}
			}
			if constexpr (VALUE)
				*first |= byte_mask;
			else
				*first &= ~byte_mask;
			byte_mask = bitmask_last_byte[(start + len) % 8];
			if constexpr (verify_that_all_bits_flip) {
				if constexpr (VALUE) {
					// VERIFY((*last & byte_mask) == 0);
				}
				else {
					// VERIFY((*last & byte_mask) == byte_mask);
				}
			}
			if constexpr (VALUE)
				*last |= byte_mask;
			else
				*last &= ~byte_mask;
			if (++first < last) {
				if constexpr (VALUE)
					::memset(first, 0xFF, last - first);
				else
					::memset(first, 0x0, last - first);
			}
		}
	}

private:

	typedef unsigned int u32;
	typedef ULONG_PTR FlatPtr;

	static constexpr u8 bitmask_first_byte[8] = { 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80 };
	static constexpr u8 bitmask_last_byte[8] = { 0x00, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F };

	//

	template<typename IntType>
	static int count_trailing_zeroes(IntType value)
	{
		if (!value) return sizeof(IntType) * 8; // In BugChecker, we use BSF instead.

		unsigned long ret = 0;

		if constexpr (sizeof(IntType) == sizeof(unsigned __int32))
			_BitScanForward(&ret, value);
		if constexpr (sizeof(IntType) == sizeof(unsigned __int64))
			_BitScanForward64(&ret, value);

		return ret;
	}

	template<typename IntType>
	static int popcount(IntType val)
	{
		int ones = 0;
		for (size_t i = 0; i < 8 * sizeof(IntType); ++i) {
			if ((val >> i) & 1) {
				++ones;
			}
		}
		return ones;
	}

	//

	static bool get(size_t index)
	{
		// VERIFY(index < m_size);
		return 0 != (m_data[index / 8] & (1u << (index % 8)));
	}

	static size_t size_in_bytes() { return m_size / static_cast<size_t>(8); }
};

SerenityOSBitmap::u8* SerenityOSBitmap::m_data = NULL;
size_t SerenityOSBitmap::m_size = 0;

//========================================================
//
// BugChecker Memory Allocator for IRQLs != PASSIVE_LEVEL
//
//========================================================

BOOLEAN Allocator::ForceUse = FALSE;
int Allocator::Start0to99 = 0;

VOID* Allocator::Arena = NULL;
VOID* Allocator::Bitmap = NULL;

size_t Allocator::Perf_AllocationsNum = 0;
size_t Allocator::Perf_TotalAllocatedSize = 0;

BOOLEAN Allocator::TestBitmap()
{
	auto prevData = SerenityOSBitmap::m_data;
	auto prevSize = SerenityOSBitmap::m_size;

	BYTE vb[256] = {};

	SerenityOSBitmap::m_data = vb;
	SerenityOSBitmap::m_size = sizeof(vb) * 8; // 2048

	SerenityOSBitmap::set_range<TRUE>(2, 1); // 2 bits at the start + 3 bits gap
	SerenityOSBitmap::set_range<TRUE>(6, 800); // 3 bits gap
	SerenityOSBitmap::set_range<TRUE>(809, 2048 - 809 - 1); // 1 bit at the end

	BOOLEAN test = TRUE;

	test = test && (SerenityOSBitmap::count_in_range(0, SerenityOSBitmap::m_size, false) == 9);
	test = test && (SerenityOSBitmap::count_in_range(1, SerenityOSBitmap::m_size - 1 - 1, false) == 7);

	size_t f = 0;
	test = test && (SerenityOSBitmap::find_next_range_of_unset_bits(f, 4, 4) == 0);
	test = test && (f == 2047);

	f = 0;
	test = test && (SerenityOSBitmap::find_next_range_of_unset_bits(f, 3, 3) == 3);
	test = test && (f == 3);

	f += 2;
	test = test && (SerenityOSBitmap::find_next_range_of_unset_bits(f, 2, 2) == 2);
	test = test && (f == 806);

	f = 600;
	test = test && (SerenityOSBitmap::find_next_range_of_unset_bits(f, 1, 1) == 1);
	test = test && (f == 806);

	f++;
	test = test && (SerenityOSBitmap::find_next_range_of_unset_bits(f, 1, 1) == 1);
	test = test && (f == 807);

	f++;
	test = test && (SerenityOSBitmap::find_next_range_of_unset_bits(f, 1, 1) == 1);
	test = test && (f == 808);

	f++;
	test = test && (SerenityOSBitmap::find_next_range_of_unset_bits(f, 1, 1) == 1);
	test = test && (f == 2047);

	SerenityOSBitmap::m_data = prevData;
	SerenityOSBitmap::m_size = prevSize;

	return test;
}

VOID Allocator::Init()
{
	Arena = ::ExAllocatePool(NonPagedPool, ArenaSize);
	Bitmap = ::ExAllocatePool(NonPagedPool, BitmapSize);

	::memset(Arena, 0, ArenaSize);
	::memset(Bitmap, 0, BitmapSize);

	SerenityOSBitmap::m_data = static_cast<BYTE*>(Bitmap);
	SerenityOSBitmap::m_size = BlocksNum;
}

VOID Allocator::Uninit()
{
	if (Arena) ::ExFreePool(Arena);
	if (Bitmap) ::ExFreePool(Bitmap);

	Arena = NULL;
	Bitmap = NULL;

	SerenityOSBitmap::m_data = NULL;
	SerenityOSBitmap::m_size = 0;
}

VOID* Allocator::Alloc(size_t size)
{
	size += HeaderSize;

	size_t sizeInBits = size / BlockSize;
	if (size % BlockSize) sizeInBits++;

	size_t block = (BlocksNum * Start0to99) / 100;

	if (SerenityOSBitmap::find_next_range_of_unset_bits(block, sizeInBits, sizeInBits) != sizeInBits)
	{
		FatalError("INTERNAL_ALLOCATOR_OUT_OF_MEMORY");
		return NULL;
	}

	if (block + sizeInBits > BlocksNum)
	{
		FatalError("INTERNAL_ALLOCATOR_OUT_OF_MEMORY");
		return NULL;
	}

	SerenityOSBitmap::set_range<TRUE>(block, sizeInBits);

	auto header = (size_t*)((ULONG_PTR)Arena + block * BlockSize);

	*header = sizeInBits;

	auto ret = reinterpret_cast<VOID*>((ULONG_PTR)header + HeaderSize);

	Perf_AllocationsNum++;
	Perf_TotalAllocatedSize += sizeInBits * BlockSize;

	return ret;
}

VOID Allocator::Free(VOID* ptr)
{
	if ((ULONG_PTR)ptr < (ULONG_PTR)Arena || (ULONG_PTR)ptr >= (ULONG_PTR)Arena + ArenaSize)
	{
		FatalError("FREE_OF_NON_ALLOCATOR_MEMORY");
		return;
	}

	auto header = (size_t*)((ULONG_PTR)ptr - HeaderSize);

	auto sizeInBits = *header;

	size_t block = ((ULONG_PTR)ptr - (ULONG_PTR)Arena) / BlockSize;

	if (!sizeInBits || block + sizeInBits > BlocksNum)
	{
		FatalError("FREE_OF_WRONG_POINTER");
		return;
	}

	SerenityOSBitmap::set_range<FALSE>(block, sizeInBits);

	Perf_AllocationsNum--;
	Perf_TotalAllocatedSize -= sizeInBits * BlockSize;
}

BOOLEAN Allocator::IsPtrInArena(VOID* ptr)
{
	if (!Arena || (ULONG_PTR)ptr < (ULONG_PTR)Arena || (ULONG_PTR)ptr >= (ULONG_PTR)Arena + ArenaSize)
		return FALSE;
	else
		return TRUE;
}

size_t Allocator::UserSizeInBytes(VOID* ptr)
{
	if ((ULONG_PTR)ptr < (ULONG_PTR)Arena || (ULONG_PTR)ptr >= (ULONG_PTR)Arena + ArenaSize)
	{
		FatalError("USERSIZEINBYTES_OF_NON_ALLOCATOR_MEMORY");
		return 0;
	}

	auto header = (size_t*)((ULONG_PTR)ptr - HeaderSize);

	auto sizeInBits = *header;

	size_t block = ((ULONG_PTR)ptr - (ULONG_PTR)Arena) / BlockSize;

	if (!sizeInBits || block + sizeInBits > BlocksNum)
	{
		FatalError("USERSIZEINBYTES_OF_WRONG_POINTER");
		return 0;
	}

	return sizeInBits * BlockSize - HeaderSize;
}

size_t Allocator::GetSetBitsNumInBitmap()
{
	return SerenityOSBitmap::count_in_range(0, SerenityOSBitmap::m_size, TRUE);
}

VOID Allocator::GetPoolMap(CHAR* out, LONG num)
{
	size_t bitsNum = SerenityOSBitmap::m_size / num;

	for (int i = 0; i < num; i++)
	{
		size_t bitsTrue = SerenityOSBitmap::count_in_range(i * bitsNum, bitsNum, TRUE);

		CHAR c;

		if (!bitsTrue)
			c = 'A';
		else
			c = (CHAR)((((size_t)('Z' - 'B') * bitsTrue) / bitsNum) + 'B');

		out[i] = c;
	}

	out[num] = '\0';
}

VOID Allocator::FatalError(const CHAR* msg) // FIXFIX: find a way to Bug Check here without reentering in the debugger!
{
	while (1)
		if (Root::I && Root::I->VideoAddr)
		{
			Glyph::DrawStr(msg, 0x4F, 0, 0, Root::I->VideoAddr);
			Glyph::UpdateScreen(0, 0, ::strlen(msg) * Root::I->GlyphWidth, Root::I->GlyphHeight);
		}
}

BOOLEAN Allocator::MustUseExXxxPool()
{
	return !Allocator::ForceUse && BcSpinLock::AreInterruptsEnabled() && Platform::GetCurrentIrql() <= DISPATCH_LEVEL;
}

//=========================================
//
// BugChecker implementation of new/delete
//
//=========================================

void* __cdecl operator new(size_t count)
{
	if (Allocator::MustUseExXxxPool())
	{
		VOID* ptr = ::ExAllocatePool(NonPagedPool, count);

		if (!ptr) Allocator::FatalError("EXALLOCATEPOOL_OUT_OF_MEMORY");

		return ptr;
	}
	else
	{
		return Allocator::Alloc(count);
	}
}

void __cdecl operator delete(void* address)
{
	if (!Allocator::IsPtrInArena(address))
	{
		if (Allocator::MustUseExXxxPool())
			::ExFreePool(address);
		else
			Allocator::FatalError("EXFREEPOOL_IN_DEBUGGER_CONTEXT");
	}
	else
	{
		Allocator::Free(address);
	}
}

//

void* __cdecl operator new(size_t, void* address)
{
	return address;
}

void __cdecl operator delete[](void* address)
{
	::operator delete(address);
}

void __cdecl operator delete(void*, void*) {}

void __cdecl operator delete(void* address, size_t)
{
	::operator delete(address);
}

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	return ::operator new(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	return ::operator new(size);
}

void* __cdecl operator new[](size_t size)
{
	return ::operator new(size);
}

//

namespace eastl
{
	EASTL_API allocator* GetDefaultAllocator()
	{
		if (Root::I)
			return &Root::I->eastl_allocator;
		else
			Allocator::FatalError("ROOT_NULL_IN_GETDEFAULTALLOCATOR");
	}
}
