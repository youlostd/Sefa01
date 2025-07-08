/* Copyright (C) 2005-2010, Unigine Corp. All rights reserved.
 *
 * File:    Memory.cpp
 * Desc:    Memory managment system
 * Version: 1.19
 * Author:  Alexander Zaprjagaev <frustum@unigine.com>
 *
 * This file is part of the Unigine engine (http://unigine.com/).
 *
 * Your use and or redistribution of this software in source and / or
 * binary form, with or without modification, is subject to: (i) your
 * ongoing acceptance of and compliance with the terms and conditions of
 * the Unigine License Agreement; and (ii) your inclusion of this notice
 * in any version of this software that you use or redistribute.
 * A copy of the Unigine License Agreement is available by contacting
 * Unigine Corp. at http://unigine.com/
 */

#include "Debug.h"
#include "Memory.h"
#include <Windows.h>
#include <ctime>
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cstdint>
#include <cassert>
#include <DbgHelp.h>

#ifdef USE_ASSERT
#include "Log.h"
#ifdef assert
#undef assert
#endif
#ifdef NDEBUG
#define assert(EXP)	(static_cast<void>(0))
#else
#ifdef _LINUX
#define assert(EXP) { if(UNLIKELY(EXP)) { } else { Log::fatal("%s:%d: %s: Assertion: '%s'\n",__FILE__,__LINE__,__ASSERT_FUNCTION,#EXP); } }
#else
#define assert(EXP) { if(UNLIKELY(EXP)) { } else { Log::fatal("%s:%d: Assertion: '%s'\n",__FILE__,__LINE__,#EXP); } }
#endif
#endif
#endif

 /*
 */
#if defined(USE_SSE) || defined(USE_ALTIVEC)
#define ALIGNED4(VALUE) (((size_t)(VALUE) + 3) & ~3)
#define ALIGNED8(VALUE) (((size_t)(VALUE) + 7) & ~7)
#define ALIGNED16(VALUE) (((size_t)(VALUE) + 15) & ~15)
#define ALIGNED128(VALUE) (((size_t)(VALUE) + 127) & ~127)
#define IS_ALIGNED4(VALUE) (((size_t)(VALUE) & 3) == 0)
#define IS_ALIGNED8(VALUE) (((size_t)(VALUE) & 7) == 0)
#define IS_ALIGNED16(VALUE) (((size_t)(VALUE) & 15) == 0)
#define IS_ALIGNED128(VALUE) (((size_t)(VALUE) & 127) == 0)
#define ASSERT_ALIGNED4(VALUE) assert(IS_ALIGNED4(VALUE))
#define ASSERT_ALIGNED8(VALUE) assert(IS_ALIGNED8(VALUE))
#define ASSERT_ALIGNED16(VALUE) assert(IS_ALIGNED16(VALUE))
#define ASSERT_ALIGNED128(VALUE) assert(IS_ALIGNED128(VALUE))
#ifdef _WIN32
#define ATTRIBUTE_ALIGNED4(NAME) __declspec(align(4)) NAME
#define ATTRIBUTE_ALIGNED8(NAME) __declspec(align(8)) NAME
#define ATTRIBUTE_ALIGNED16(NAME) __declspec(align(16)) NAME
#define ATTRIBUTE_ALIGNED128(NAME) __declspec(align(128)) NAME
#else
#define ATTRIBUTE_ALIGNED4(NAME) NAME __attribute__ ((aligned(4)))
#define ATTRIBUTE_ALIGNED8(NAME) NAME __attribute__ ((aligned(8)))
#define ATTRIBUTE_ALIGNED16(NAME) NAME __attribute__ ((aligned(16)))
#define ATTRIBUTE_ALIGNED128(NAME) NAME __attribute__ ((aligned(128)))
#endif
#else
#define ALIGNED4(VALUE) ((size_t)(VALUE))
#define ALIGNED8(VALUE) ((size_t)(VALUE))
#define ALIGNED16(VALUE) ((size_t)(VALUE))
#define ALIGNED128(VALUE) ((size_t)(VALUE))
#define IS_ALIGNED4(VALUE) (1)
#define IS_ALIGNED8(VALUE) (1)
#define IS_ALIGNED16(VALUE) (1)
#define IS_ALIGNED128(VALUE) (1)
#define ASSERT_ALIGNED4(VALUE)
#define ASSERT_ALIGNED8(VALUE)
#define ASSERT_ALIGNED16(VALUE)
#define ASSERT_ALIGNED128(VALUE)
#define ATTRIBUTE_ALIGNED4(NAME) NAME
#define ATTRIBUTE_ALIGNED8(NAME) NAME
#define ATTRIBUTE_ALIGNED16(NAME) NAME
#define ATTRIBUTE_ALIGNED128(NAME) NAME
#endif

struct uhrensohn
{
	void* address;
	void* values[8192];
	int64_t count;
};
struct uhrensohn gStackTrace[4096] = {0};

inline void* StackTraceAddress()
{
	void* address = 0;
	CaptureStackBackTrace(3, 1, &address, 0);
	return address;
}
void StackTraceAdd(void* p)
{
	void* addr = StackTraceAddress();
	for (uint32_t i = 0; i < 4096; ++i)
	{
		if(gStackTrace[i].address == addr)
		{
			for (uint32_t j = 0; j < 8192; ++j)
			{
				if(gStackTrace[i].values[j] == 0)
				{
					gStackTrace[i].values[j] = p;
					break;
				}
			}
			++gStackTrace[i].count;
			return;
		}
	}
	for (uint32_t i = 0; i < 4096; ++i)
	{
		if(gStackTrace[i].address == 0 || gStackTrace[i].count == 0)
		{
			gStackTrace[i].address = addr;
			gStackTrace[i].values[0] = p;
			gStackTrace[i].count = 1;
			break;
		}
	}
}
void StackTraceRemove(void* p)
{
	for (uint32_t i = 0; i < 4096; ++i)
	{
		uint32_t x = 0;
		for (uint32_t j = 0; j < 8192 && x < gStackTrace[i].count; ++j)
		{
			if(gStackTrace[i].values[j] == p)
			{
				gStackTrace[i].values[j] = 0;
				--gStackTrace[i].count;
				return;
			}
			
			if(gStackTrace[i].values[j] != 0)
			{
				++x;
			}
		}
	}
}
void StackTraceLog(FILE* out, int64_t minLeaks)
{
	TCHAR buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	SYMBOL_INFO* info = reinterpret_cast<SYMBOL_INFO*>(buf);
	IMAGEHLP_LINE64 line;
	for (uint32_t i = 0; i < 4096; ++i)
	{
		if(gStackTrace[i].address != 0 && gStackTrace[i].count != 0)
		{
			if ((gStackTrace[i].count & 0x7fffffffffffffff) - (minLeaks & 0x7fffffffffffffff) >= 0)
			{
				memset(info, 0, sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR));
				memset(&line, 0, sizeof(line));
				line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				info->SizeOfStruct = sizeof(SYMBOL_INFO);
				info->MaxNameLen = MAX_SYM_NAME;
				DWORD64 funcDisplacement = 0;
				DWORD lineDisplacement = 0;
				if (SymFromAddr(GetCurrentProcess(), reinterpret_cast<DWORD64>(gStackTrace[i].address), &funcDisplacement, info))
				{
					fprintf(out, "%s", info->Name);
				}
				else
				{
					fprintf(out, "%p", gStackTrace[i].address);
				}
				if (SymGetLineFromAddr64(GetCurrentProcess(), reinterpret_cast<DWORD64>(gStackTrace[i].address), &lineDisplacement, &line))
				{
					fprintf(out, " (%s:%u)", line.FileName, line.LineNumber);
				}
				fprintf(out, ": %lld unfreed allocations du hure\r\n", gStackTrace[i].count);
			}
		}
	}
}


 /*
 */
INLINE int AtomicCAS(volatile int *ptr, int old_value, int new_value) {
#ifdef _WIN32
	return (InterlockedCompareExchange((long volatile*)ptr, new_value, old_value) == old_value);
#elif _LINUX
	return (__sync_val_compare_and_swap(ptr, old_value, new_value) == old_value);
#elif _CELLOS_LV2
	return (cellAtomicCompareAndSwap32((uint32_t*)ptr, (uint32_t)old_value, (uint32_t)new_value) == (uint32_t)old_value);
#else
	if (*ptr != old_value) return 0;
	*ptr = new_value;
	return 1;
#endif
}

/*
*/
INLINE void SpinLock(volatile int *ptr, int old_value, int new_value) {
	while (!AtomicCAS(ptr, old_value, new_value));
}

/*
*/
INLINE void WaitLock(volatile int *ptr, int value) {
	while (!AtomicCAS(ptr, value, value));
}

/*
*/
class AtomicLock {

public:

	INLINE AtomicLock(volatile int *ptr) : ptr(ptr) {
		SpinLock(ptr, 0, 1);
	}
	INLINE ~AtomicLock() {
		SpinLock(ptr, 1, 0);
	}

private:

	volatile int *ptr;
};

/******************************************************************************\
*
* FixedChunk
*
\******************************************************************************/

/*
 */
struct FixedChunk {
	
	// initialized/shutdown
	void init(size_t size,unsigned char num);
	void shutdown(unsigned char num);
	
	// check pointer
	int isAllocator(void *ptr) const;
	
	// allocate memory
	void *allocate(size_t size);
	void deallocate(void *ptr,size_t size);
	
	unsigned char *data;		// blocks data
	unsigned char *data_begin;	// blocks begin
	unsigned char *data_end;	// blocks end
	unsigned char first_block;	// first available block
	unsigned char free_blocks;	// number of available blocks
};

/*
 */
void FixedChunk::init(size_t size,unsigned char num) {
	
	// aligned allocation
	data = static_cast<unsigned char*>(Memory::systemAllocate(size * num + 16));
	if(data == NULL) TraceError("FixedChunk::init(): can't allocate %d bytes\n",size * num + 16);
	data_begin = reinterpret_cast<unsigned char*>ALIGNED16(data);
	data_end = data_begin + size * num;
	
	// initialize chunk
	first_block = 0;
	free_blocks = num;
	unsigned char *ptr = data_begin;
	for(unsigned char i = 1; i < num; i++, ptr += size) {
		*ptr = i;
	}
}

void FixedChunk::shutdown(unsigned char num) {
	
	// deallocate memory
	if(free_blocks == num) Memory::systemDeallocate(data);
	
	// clear chunk
	data = NULL;
	data_begin = NULL;
	data_end = NULL;
	first_block = 0;
	free_blocks = 0;
}

/*
 */
INLINE int FixedChunk::isAllocator(void *ptr) const {
	return (ptr >= data_begin && ptr < data_end);
}

/*
 */
void *FixedChunk::allocate(size_t size) {
	
	// check available blocks
	assert(free_blocks > 0 && "FixedChuck::allocate(): can't allocate memory");
	
	// first free block
	unsigned char *ret = data_begin + first_block * size;
	first_block = *ret;
	free_blocks--;
	return ret;
}

void FixedChunk::deallocate(void *p,size_t size) {
	
	// check pointer
	unsigned char *ptr = static_cast<unsigned char*>(p);
	assert(isAllocator(ptr) && "FixedChuck::deallocate(): is not an allocator");
	assert((ptr - data_begin) % size == 0 && "FixedChuck::deallocate(): bad pointer");
	
	// append free block
	*ptr = first_block;
	first_block = (unsigned char)((ptr - data_begin) / size);
	free_blocks++;
}

/******************************************************************************\
*
* FixedAllocator
*
\******************************************************************************/

/*
 */
struct FixedAllocator {
	
	// initialize/shutdown
	void init(size_t size,unsigned char num);
	void shutdown();
	
	// check pointer
	int isAllocator(void *ptr) const;
	
	// allocate memory
	void *allocate();
	void deallocate(void *ptr);
	
	// statistics
	size_t getHeapUsage() const;
	
	size_t block_size;				// block size
	unsigned char num_blocks;		// number of blocks
	
	int capacity;					// capacity
	int num_chunks;					// number of chunks
	FixedChunk *chunks;				// fixed chunks
	
	FixedChunk *allocated_chunk;	// last allocated chunk
	FixedChunk *deallocated_chunk;	// last deallocated chunk
};

/*
 */
void FixedAllocator::init(size_t size,unsigned char num) {
	
	// parameters
	block_size = size;
	num_blocks = num;
	
	// initialize chunks
	capacity = 0;
	num_chunks = 0;
	chunks = NULL;
	allocated_chunk = NULL;
	deallocated_chunk = NULL;
}

void FixedAllocator::shutdown() {
	
	// shutdown chunks
	for(int i = 0; i < num_chunks; i++) {
		chunks[i].shutdown(num_blocks);
	}
	
	// deallocate memory
	free(chunks);
	
	// clear chunks
	capacity = 0;
	num_chunks = 0;
	chunks = NULL;
	allocated_chunk = NULL;
	deallocated_chunk = NULL;
}

/*
 */
int FixedAllocator::isAllocator(void *ptr) const {
	
	// check last chunks
	if(allocated_chunk && allocated_chunk->isAllocator(ptr)) return 1;
	if(deallocated_chunk && deallocated_chunk->isAllocator(ptr)) return 1;
	
	// check chunks
	for(int i = 0; i < num_chunks; i++) {
		if(chunks[i].isAllocator(ptr)) return 1;
	}
	
	return 0;
}

/*
 */
void *FixedAllocator::allocate() {
	
	// last allocated chunk
	if(allocated_chunk && allocated_chunk->free_blocks > 0) {
		return allocated_chunk->allocate(block_size);
	}
	
	// find free chunks
	for(int i = 0; i < num_chunks; i++) {
		if(chunks[i].free_blocks > 0) {
			allocated_chunk = &chunks[i];
			return allocated_chunk->allocate(block_size);
		}
	}
	
	// create new chunk
	if(num_chunks == capacity) {
		capacity = capacity * 2 + 1;
		chunks = static_cast<FixedChunk*>(realloc(chunks,sizeof(FixedChunk) * capacity));
		if (chunks == NULL) TraceError("FixedAllocator::allocate(): can't allocate %d bytes\n", sizeof(FixedChunk) * capacity);
		allocated_chunk = NULL;
		deallocated_chunk = NULL;
	}
	
	chunks[num_chunks].init(block_size,num_blocks);
	allocated_chunk = &chunks[num_chunks++];
	return allocated_chunk->allocate(block_size);
}

void FixedAllocator::deallocate(void *ptr) {
	
	// find deallocated chunk
	if(deallocated_chunk == NULL || deallocated_chunk->isAllocator(ptr) == 0) {
		FixedChunk *begin = &chunks[0];
		FixedChunk *end = &chunks[num_chunks - 1];
		FixedChunk *left = (deallocated_chunk) ? deallocated_chunk : &chunks[num_chunks / 2];
		FixedChunk *right = left + 1;
		for(int i = 0; i < num_chunks; i++) {
			if(left >= begin) {
				if(left->isAllocator(ptr)) {
					deallocated_chunk = left;
					break;
				}
				left--;
			}
			if(right <= end) {
				if(right->isAllocator(ptr)) {
					deallocated_chunk = right;
					break;
				}
				right++;
			}
		}
	}
	
	// check deallocated chunk
	if(deallocated_chunk == NULL || deallocated_chunk->isAllocator(ptr) == 0) {
		TraceError("FixedAllocator::deallocate(): bad pointer\n");
	}
	
	// deallocate chunk
	deallocated_chunk->deallocate(ptr,block_size);
	
	// delete chunk
	if(deallocated_chunk->free_blocks == num_blocks) {
		deallocated_chunk->shutdown(num_blocks);
		*deallocated_chunk = chunks[--num_chunks];
		allocated_chunk = NULL;
		deallocated_chunk = NULL;
	}
}

/*
 */
size_t FixedAllocator::getHeapUsage() const {
	return block_size * num_blocks * num_chunks;
}

/******************************************************************************\
*
* HeapChunk
*
\******************************************************************************/

/*
 */
struct HeapChunk {
	
	struct List;
	struct Footer;
	
	// initialize/shutdown
	void init(size_t size,HeapChunk *prev,HeapChunk *next,HeapChunk *&allocator);
	
	// check chunk
	int isFree() const;
	
	// linked list
	List *getList();
	
	// chunk footer
	Footer *getFooter();
	
	// allocate memory
	void *allocate(size_t size,HeapChunk *&allocator);
	size_t deallocate(HeapChunk *&allocator);
	
	enum {
		FOOTER_BEGIN = 0x131c3c1f,
		FOOTER_END = 0x01f0f1cd,
	};
	
	struct List {
		HeapChunk *prev;	// previous allocated chunk
		HeapChunk *next;	// next allocated chunk
	};
	
	struct Footer {
		int begin;			// footer begin
		int offset;			// footer offset
		int size;			// footer size
		int end;			// footer end
	};
	
	size_t size;			// chunk size
	size_t free;			// chunk is free
	
	HeapChunk *prev;		// previous chunk
	HeapChunk *next;		// next chunk
};

/*
 */
void HeapChunk::init(size_t s,HeapChunk *p,HeapChunk *n,HeapChunk *&allocator) {
	
	// parameters
	size = s;
	free = 1;
	
	prev = p;
	next = n;
	
	// linked list
	List *list = getList();
	
	// previous chunk
	if(prev) {
		
		prev->next = this;
		
		List *prev_list = prev->getList();
		
		// previous allocated chunk
		if(prev_list->prev) {
			List *prev_prev_list = prev_list->prev->getList();
			list->prev = prev_list->prev;
			prev_prev_list->next = this;
		} else {
			list->prev = NULL;
			allocator = this;
		}
		
		// next allocated chunk
		if(prev_list->next) {
			List *prev_next_list = prev_list->next->getList();
			list->next = prev_list->next;
			prev_next_list->prev = this;
		} else {
			list->next = NULL;
		}
	}
	else {
		list->prev = NULL;
		list->next = NULL;
		allocator = this;
	}
	
	// next chunk
	if(next) {
		next->prev = this;
	}
}

/*
 */
INLINE int HeapChunk::isFree() const {
	return (free && prev == NULL && next == NULL);
}

INLINE HeapChunk::List *HeapChunk::getList() {
	assert(free && "HeapChunk::getList(): chunk is not free");
	unsigned char *ptr = reinterpret_cast<unsigned char*>(this);
	return reinterpret_cast<List*>(ptr + sizeof(HeapChunk));
}

INLINE HeapChunk::Footer *HeapChunk::getFooter() {
	unsigned char *ptr = reinterpret_cast<unsigned char*>(this);
	return reinterpret_cast<Footer*>(ptr + size - sizeof(Footer));
}

/*
 */
void *HeapChunk::allocate(size_t s,HeapChunk *&allocator) {
	
	// check chunk
	assert(free && "HeapChunk::allocate(): chunk is already allocated");
	
	// chunk address
	unsigned char *ptr = reinterpret_cast<unsigned char*>(this);
	
	// split chunk
	if(size > s + s / 2) {
		HeapChunk *chunk = reinterpret_cast<HeapChunk*>(ptr + s);
		chunk->init(size - s,this,next,allocator);
		size = s;
	}
	// update allocated chunk
	else if(allocator == this) {
		List *list = getList();
		if(list->next) list->next->getList()->prev = NULL;
		allocator = list->next;
	}
	// remove chunk
	else {
		List *list = getList();
		if(list->prev) list->prev->getList()->next = list->next;
		if(list->next) list->next->getList()->prev = list->prev;
	}
	
	// chunk is used
	free = 0;
	
	// chunk footer
	Footer *footer = getFooter();
	footer->begin = FOOTER_BEGIN;
	footer->offset = (int)(size - sizeof(Footer));
	footer->size = (int)s;
	footer->end = FOOTER_END;
	
	// allocated memory
	return ptr + sizeof(HeapChunk);
}

size_t HeapChunk::deallocate(HeapChunk *&allocator) {
	
	// check chunk
	if (free) TraceError("HeapChunk::deallocate(): chunk is not allocated\n");
	
	// check footer
	Footer *footer = getFooter();
	if(footer->begin != FOOTER_BEGIN || footer->end != FOOTER_END || footer->offset != (int)(size - sizeof(Footer))) {
		TraceError("HeapChunk::deallocate(): memory corruption detected\n");
	}
	
	// chunk is free
	free = 1;
	
	// merge with previous allocated chunk
	if(prev && prev->free) {
		prev->size += size;
		prev->next = next;
		if(next) {
			next->prev = prev;
			if(next->free) {
				prev->size += next->size;
				prev->next = next->next;
				if(next->next) next->next->prev = prev;
				List *next_list = next->getList();
				if(next_list->prev) next_list->prev->getList()->next = next_list->next;
				if(next_list->next) next_list->next->getList()->prev = next_list->prev;
				if(allocator == next) allocator = next_list->next;
			}
		}
	}
	// merge with next allocated chunk
	else if(next && next->free) {
		List *list = getList();
		List *next_list = next->getList();
		// next chunk is allocated chunk
		if(allocator == next) {
			if(next_list->next) {
				List *next_next_list = next_list->next->getList();
				next_next_list->prev = this;
			}
			list->next = next_list->next;
		}
		// remove next chunk
		else {
			if(allocator) {
				List *allocated_list = allocator->getList();
				allocated_list->prev = this;
			}
			if(next_list->prev) next_list->prev->getList()->next = next_list->next;
			if(next_list->next) next_list->next->getList()->prev = next_list->prev;
			list->next = allocator;
		}
		size += next->size;
		next = next->next;
		if(next) next->prev = this;
		list->prev = NULL;
		allocator = this;
	}
	// create new allocated chunk
	else {
		List *list = getList();
		if(allocator) {
			List *allocated_list = allocator->getList();
			allocated_list->prev = this;
		}
		list->prev = NULL;
		list->next = allocator;
		allocator = this;
	}
	
	// allocated memory
	return footer->size - sizeof(HeapChunk) - sizeof(Footer);
}

/******************************************************************************\
*
* HeapPool
*
\******************************************************************************/

/*
 */
struct HeapPool {
	
	// initialize/shutdown
	void init(size_t size);
	void shutdown();
	
	// check chunk
	int isFree() const;
	
	// check pointer
	int isAllocator(void *ptr) const;
	
	// allocate memory
	void *allocate(size_t size);
	size_t deallocate(void *ptr);
	
	unsigned char *data;		// pool data
	unsigned char *data_begin;	// pool begin
	unsigned char *data_end;	// pool end
	
	HeapChunk *first_chunk;		// first chunk
	HeapChunk *allocated_chunk;	// allocated chunk
};

/*
 */
void HeapPool::init(size_t size) {
	
	// aligned allocation
	data = static_cast<unsigned char*>(Memory::systemAllocate(size + 16));
	if (data == NULL) TraceError("HeapPool::init(): can't allocate %d bytes\n", size + 16);
	data_begin = reinterpret_cast<unsigned char*>ALIGNED16(data);
	data_end = data_begin + size;
	
	// initialize chunks
	allocated_chunk = NULL;
	first_chunk = reinterpret_cast<HeapChunk*>(data_begin);
	first_chunk->init(size,NULL,NULL,allocated_chunk);
}

void HeapPool::shutdown() {
	
	// deallocate memory
	if(first_chunk->isFree()) Memory::systemDeallocate(data);
	
	// clear pool
	data = NULL;
	data_begin = NULL;
	data_end = NULL;
	
	// clear chunks
	first_chunk = NULL;
	allocated_chunk = NULL;
}

/*
 */
INLINE int HeapPool::isFree() const {
	return first_chunk->isFree();
}

INLINE int HeapPool::isAllocator(void *ptr) const {
	return (ptr >= data_begin && ptr < data_end);
}

/*
 */
void *HeapPool::allocate(size_t size) {
	
	// find best fit chunk
	HeapChunk *chunk = NULL;
	HeapChunk *it = allocated_chunk;
	while(it != NULL) {
		if(it->size >= size) {
			if(chunk == NULL || chunk->size > it->size) {
				chunk = it;
			}
		}
		it = it->getList()->next;
	}
	
	// allocate memory
	if(chunk) return chunk->allocate(size,allocated_chunk);
	
	return NULL;
}

size_t HeapPool::deallocate(void *ptr) {
	
	// check pointer
	assert(isAllocator(ptr) && "HeapPool::deallocate(): is not an allocator");
	
	// deallocate memory
	HeapChunk *chunk = reinterpret_cast<HeapChunk*>(static_cast<unsigned char*>(ptr) - sizeof(HeapChunk));
	return chunk->deallocate(allocated_chunk);
}

/******************************************************************************\
*
* HeapAllocator
*
\******************************************************************************/

/*
 */
struct HeapAllocator {
	
	// initialize/shutdown
	void init(size_t size);
	void shutdown();
	
	// check pointer
	int isAllocator(void *ptr) const;
	
	// allocate memory
	void *allocate(size_t size);
	size_t deallocate(void *ptr);
	
	// statistics
	size_t getHeapUsage() const;
	
	size_t pool_size;			// pool size
	
	int capacity;				// capacity
	int num_pools;				// number of pools
	HeapPool *pools;			// heap pools
	
	HeapPool *allocated_pool;	// last allocated pool
	HeapPool *deallocated_pool;	// last deallocated pool
};

/*
 */
void HeapAllocator::init(size_t size) {
	
	// parameters
	pool_size = size;
	
	// initialize pools
	capacity = 0;
	num_pools = 0;
	pools = NULL;
	allocated_pool = NULL;
	deallocated_pool = NULL;
}

void HeapAllocator::shutdown() {
	
	// shutdown pools
	for(int i = 0; i < num_pools; i++) {
		pools[i].shutdown();
	}
	
	// deallocate memory
	free(pools);
	
	// clear pools
	capacity = 0;
	num_pools = 0;
	pools = NULL;
	allocated_pool = NULL;
	deallocated_pool = NULL;
}

/*
 */
int HeapAllocator::isAllocator(void *ptr) const {
	
	// check last pools
	if(allocated_pool && allocated_pool->isAllocator(ptr)) return 1;
	if(deallocated_pool && deallocated_pool->isAllocator(ptr)) return 1;
	
	// check pools
	for(int i = 0; i < num_pools; i++) {
		if(pools[i].isAllocator(ptr)) return 1;
	}
	
	return 0;
}

/*
 */
void *HeapAllocator::allocate(size_t size) {
	
	// aligned allocation
	size += sizeof(HeapChunk) + sizeof(HeapChunk::Footer);
	
	// last allocated pool
	if(allocated_pool) {
		void *ptr = allocated_pool->allocate(size);
		if(ptr) return ptr;
	}
	
	// find free pools
	for(int i = 0; i < num_pools; i++) {
		void *ptr = pools[i].allocate(size);
		if(ptr) {
			allocated_pool = &pools[i];
			return ptr;
		}
	}
	
	// create new pool
	if(num_pools == capacity) {
		capacity = capacity * 2 + 1;
		pools = static_cast<HeapPool*>(realloc(pools,sizeof(HeapPool) * capacity));
		if (pools == NULL) TraceError("HeapAllocator::allocate(): can't allocate %d bytes\n", sizeof(HeapPool) * capacity);
		allocated_pool = NULL;
		deallocated_pool = NULL;
	}
	
	pools[num_pools].init(pool_size);
	allocated_pool = &pools[num_pools++];
	return allocated_pool->allocate(size);
}

size_t HeapAllocator::deallocate(void *ptr) {
	
	// find deallocated pool
	if(deallocated_pool == NULL || deallocated_pool->isAllocator(ptr) == 0) {
		HeapPool *begin = &pools[0];
		HeapPool *end = &pools[num_pools - 1];
		HeapPool *left = (deallocated_pool) ? deallocated_pool : &pools[num_pools / 2];
		HeapPool *right = left + 1;
		for(int i = 0; i < num_pools; i++) {
			if(left >= begin) {
				if(left->isAllocator(ptr)) {
					deallocated_pool = left;
					break;
				}
				left--;
			}
			if(right <= end) {
				if(right->isAllocator(ptr)) {
					deallocated_pool = right;
					break;
				}
				right++;
			}
		}
	}
	
	// check deallocated pool
	if(deallocated_pool == NULL || deallocated_pool->isAllocator(ptr) == 0) {
		TraceError("HeapAllocator::deallocate(): bad pointer\n");
	}
	
	// deallocate pool
	return deallocated_pool->deallocate(ptr);
}

/*
 */
size_t HeapAllocator::getHeapUsage() const {
	return pool_size * num_pools;
}

/******************************************************************************\
*
* SystemAllocator
*
\******************************************************************************/

/*
 */
struct SystemAllocator {
	
	// allocate memory
	static void *allocate(size_t size);
	static size_t deallocate(void *ptr);
	
	enum {
		HEADER_BEGIN = 0x131c3c1f,
		HEADER_END = 0x01f0f1cd,
	};
	
	struct Header {
		int begin;		// header begin
		int offset;		// header offset
		int size;		// header size
		int end;		// header end
	};
};

/*
 */
void *SystemAllocator::allocate(size_t size) {
	
	// aligned allocation
	unsigned char *ptr = static_cast<unsigned char*>(Memory::systemAllocate(size + sizeof(Header) + 16));
	if (ptr == NULL) TraceError("SystemAllocator::allocate(): can't allocate %d bytes\n", size + sizeof(Header) + 16);
	size_t offset = ALIGNED16(ptr) - (size_t)ptr;
	
	// allocation header
	Header *header = reinterpret_cast<Header*>(ptr + offset);
	header->begin = HEADER_BEGIN;
	header->offset = (int)(sizeof(Header) + offset);
	header->size = (int)size;
	header->end = HEADER_END;
	
	return ptr + header->offset;
}

size_t SystemAllocator::deallocate(void *p) {
	
	// allocation address
	unsigned char *ptr = reinterpret_cast<unsigned char*>(p);
	
	// check header
	Header *header = reinterpret_cast<Header*>(ptr - sizeof(Header));
	if(header->begin != HEADER_BEGIN || header->end != HEADER_END) {
		TraceError("SystemAllocator::deallocate(): memory corruption detected\n");
	}
	
	// free allocation
	size_t size = header->size;
	Memory::systemDeallocate(ptr - header->offset);
	
	return size;
}

/******************************************************************************\
*
* Allocator
*
\******************************************************************************/

/*
 */
class Allocator {
		
	public:
		
		Allocator();
		~Allocator();
		
		// memory information
		void info();
		void dump();
		
		// allocate memory
		void *allocate(size_t size);
		
		// deallocate memory
		int deallocate(void *ptr);
		int deallocate(void *ptr,size_t size);
		
		// statistics
		size_t getHeapUsage() const;
		size_t getMemoryUsage() const;
		int getNumAllocations() const;
		int getNumFrameAllocations();
		
	private:
		
		enum {
			
			FIXED_SIZE = 64,
			FIXED_BLOCKS = 255,
			
			SMALL_SIZE = 1024,
			SMALL_HEAP_SIZE = 1024 * 1024 * 16,
			
			MEDIUM_SIZE = 1024 * 16,
			MEDIUM_HEAP_SIZE = 1024 * 1024 * 16,
			
			LARGE_SIZE = 1024 * 1024 * 16,
			LARGE_HEAP_SIZE = 1024 * 1024 * 32,
			
			DUMP_LINES = 8,
			DUMP_STEPS = 64,
		};
		
		// dump allocators
		void dump(const char *name,const FixedAllocator &fixed);
		void dump(const char *name,const HeapAllocator &heap);
		
		// fixed allocator
		int get_fixed_allocator(size_t size);
		void *fixed_allocate(size_t size);
		int fixed_deallocate(void *ptr);
		int fixed_deallocate(void *ptr,size_t size);
		
		// small allocator
		void *small_allocate(size_t size);
		int small_deallocate(void *ptr);
		
		// medium allocator
		void *medium_allocate(size_t size);
		int medium_deallocate(void *ptr);
		
		// large allocator
		void *large_allocate(size_t size);
		int large_deallocate(void *ptr);
		
		volatile int lock;					// lock
		
		int num_fixed_allocators;			// fixed allocators
		FixedAllocator *fixed_allocators;
		FixedAllocator *fixed_deallocator;
		
		HeapAllocator small_allocator;		// small allocator
		HeapAllocator medium_allocator;		// medium allocator
		HeapAllocator large_allocator;		// large allocator
		
		size_t memory_usage;				// statistics
		int num_allocations;
		int num_frame_allocations;
};

/*
 */
Allocator::Allocator() : lock(0) {
	
	// initialize fixed allocators
	num_fixed_allocators = 0;
	for(size_t i = 4; i <= FIXED_SIZE; i <<= 1) {
		num_fixed_allocators++;
	}
	fixed_allocators = static_cast<FixedAllocator*>(malloc(sizeof(FixedAllocator) * num_fixed_allocators));
	if (fixed_allocators == NULL) TraceError("Allocator::Allocator(): can't allocate %d bytes\n", sizeof(FixedAllocator) * num_fixed_allocators);
	for(int i = 0; i < num_fixed_allocators; i++) {
		fixed_allocators[i].init(1 << (i + 2),FIXED_BLOCKS);
	}
	fixed_deallocator = NULL;
	
	// initialize small allocator
	small_allocator.init(SMALL_HEAP_SIZE);
	
	// initialize medium allocator
	medium_allocator.init(MEDIUM_HEAP_SIZE);
	
	// initialize large allocator
	large_allocator.init(LARGE_HEAP_SIZE);
	
	// statistics
	memory_usage = 0;
	num_allocations = 0;
	num_frame_allocations = 0;
}

Allocator::~Allocator() {
	
	// shutdown fixed allocators
	for(int i = 0; i < num_fixed_allocators; i++) {
		fixed_allocators[i].shutdown();
	}
	free(fixed_allocators);
	
	// shutdown small allocator
	small_allocator.shutdown();
	
	// shutdown medium allocator
	medium_allocator.shutdown();
	
	// shutdown large allocator
	large_allocator.shutdown();
}

/*
 */
void Allocator::info() {
	
	// memory usage
	if (memory_usage == 0)TraceError("Memory usage: none\n");
	else if (memory_usage < 1024) TraceError("Memory usage: %db\n", memory_usage);
	else if (memory_usage < 1024 * 1024) TraceError("Memory usage: %.1fKb\n", memory_usage / 1024.0f);
	else TraceError("Memory usage: %.1fMb\n", memory_usage / (1024.0f * 1024.0f));
	
	// number of allocations
	if (num_allocations == 0) TraceError("Allocations:  none\n");
	else TraceError("Allocations:  %d\n", num_allocations);
}

/*
 */
void Allocator::dump(const char *name,const FixedAllocator &fixed) {
	
	// fixed info
	int num_blocks = fixed.num_chunks * FIXED_BLOCKS;
	int num_free_blocks = 0;
	SpinLock(&lock,0,1);
	for(int j = 0; j < fixed.num_chunks; j++) {
		const FixedChunk *chunk = &fixed.chunks[j];
		num_free_blocks += chunk->free_blocks;
	}
	SpinLock(&lock,1,0);
	TraceError("%s %3d: %7d/%-7d %7.1f/%.1fKb\n", name, fixed.block_size, num_blocks, num_blocks - num_free_blocks, num_blocks * fixed.block_size / 1024.0f, (num_blocks - num_free_blocks) * fixed.block_size / 1024.0f);
}

void Allocator::dump(const char *name,const HeapAllocator &heap) {
	
	// dump parameters
	int dump_line_size = (int)(heap.pool_size / DUMP_LINES);
	int dump_step_size = dump_line_size / DUMP_STEPS;
	
	for(int i = 0; i < heap.num_pools; i++) {
		const HeapPool *pool = &heap.pools[i];
		
		// pool info
		int num_chunks = 0;
		int num_free_chunks = 0;
		size_t memory_size = 0;
		size_t memory_usage = 0;
		SpinLock(&lock,0,1);
		HeapChunk *it = pool->first_chunk;
		while(it != NULL) {
			num_chunks++;
			memory_size += it->size;
			if(it->free) num_free_chunks++;
			else memory_usage += it->size;
			it = it->next;
		}
		SpinLock(&lock,1,0);
		TraceError("%s %3d: %7d/%-7d %7.1f/%.1fMb\n", name, memory_size / 1024 / 1024, num_chunks, num_chunks - num_free_chunks, memory_size / (1024.0f * 1024.0f), memory_usage / (1024.0f * 1024.0f));
		
		// pool dump
		for(int j = 0; j < (int)memory_size; j += dump_line_size) {
			size_t line_size = 0;
			int counter[DUMP_STEPS];
			memset(counter,0,sizeof(counter));
			SpinLock(&lock,0,1);
			it = pool->first_chunk;
			while(it != NULL) {
				int pos = (int)(reinterpret_cast<unsigned char*>(it) - pool->data_begin) - j;
				if(pos > dump_line_size) break;
				if(pos + (int)it->size > 0 && it->free == 0) {
					int begin = pos;
					int end = pos + (int)it->size;
					if(begin < 0) begin = 0;
					if(end > dump_line_size) end = dump_line_size;
					line_size += end - begin;
					for(int k = begin; k < end; k += dump_step_size) {
						counter[k / dump_step_size]++;
					}
				}
				it = it->next;
			}
			SpinLock(&lock,1,0);
			char line[1024];
			memset(line,0,sizeof(line));
			const char *codes = " 123456789ABCDEFX";
			for(int k = 0; k < dump_line_size / dump_step_size; k++) {
				int c = counter[k];
				if(c > 16) c = 16;
				line[k] = codes[c];
			}
			TraceError("|%s| %.1fMb\n", line, line_size / (1024.0f * 1024.0f));
		}
	}
}

void Allocator::dump() {
	
	// heap usage
	size_t heap_usage = 0;
	
	// fixed allocators
	for(int i = 0; i < num_fixed_allocators; i++) {
		dump("Fixed ",fixed_allocators[i]);
		heap_usage += fixed_allocators[i].getHeapUsage();
	}
	
	// small allocator
	dump("Small ",small_allocator);
	heap_usage += small_allocator.getHeapUsage();
	
	// medium allocator
	dump("Medium",medium_allocator);
	heap_usage += medium_allocator.getHeapUsage();
	
	// large allocator
	dump("Large ",large_allocator);
	heap_usage += large_allocator.getHeapUsage();
	
	// heap usage
	if (heap_usage == 0) TraceError("Heap usage: none\n");
	else if (heap_usage < 1024) TraceError("Heap usage: %db\n", heap_usage);
	else if (heap_usage < 1024 * 1024) TraceError("Heap usage: %.1fKb\n", heap_usage / 1024.0f);
	else TraceError("Heap usage: %.1fMb\n", heap_usage / (1024.0f * 1024.0f));
}

/*
 */
int Allocator::get_fixed_allocator(size_t size) {
	
	// check fixed allocator size
	if(size > FIXED_SIZE) return -1;
	
	// find the best fixed allocator
	for(int i = 0; i < num_fixed_allocators; i++) {
		if(fixed_allocators[i].block_size >= size) return i;
	}
	
	return -1;
}

void *Allocator::fixed_allocate(size_t size) {
	
	// get fixed allocator by size
	int num = get_fixed_allocator(size);
	if(num != -1) {
		memory_usage += fixed_allocators[num].block_size;
		num_allocations++;
		num_frame_allocations++;
		return fixed_allocators[num].allocate();
	}
	
	return NULL;
}

int Allocator::fixed_deallocate(void *ptr,size_t size) {
	
	// get fixed allocator by size
	int num = get_fixed_allocator(size);
	if(num != -1 && fixed_allocators[num].isAllocator(ptr)) {
		fixed_allocators[num].deallocate(ptr);
		memory_usage -= fixed_allocators[num].block_size;
		num_allocations--;
		return 1;
	}
	
	return 0;
}

int Allocator::fixed_deallocate(void *ptr) {
	
	// try to use last deallocated chunk
	if(fixed_deallocator && fixed_deallocator->isAllocator(ptr)) {
		fixed_deallocator->deallocate(ptr);
		memory_usage -= fixed_deallocator->block_size;
		num_allocations--;
		return 1;
	}
	
	// find deallocated chunk
	FixedAllocator *begin = &fixed_allocators[0];
	FixedAllocator *end = &fixed_allocators[num_fixed_allocators - 1];
	FixedAllocator *left = (fixed_deallocator) ? fixed_deallocator : &fixed_allocators[num_fixed_allocators / 2];
	FixedAllocator *right = left + 1;
	for(int i = 0; i < num_fixed_allocators; i++) {
		if(left >= begin) {
			if(left->isAllocator(ptr)) {
				fixed_deallocator = left;
				return fixed_deallocate(ptr);
			}
			left--;
		}
		if(right <= end) {
			if(right->isAllocator(ptr)) {
				fixed_deallocator = right;
				return fixed_deallocate(ptr);
			}
			right++;
		}
	}
	
	return 0;
}

/*
 */
void *Allocator::small_allocate(size_t size) {
	
	// aligned allocation
	size = ALIGNED16(size);
	
	void *ptr = small_allocator.allocate(size);
	
	if(ptr) {
		memory_usage += size;
		num_allocations++;
		num_frame_allocations++;
	}
	
	return ptr;
}

int Allocator::small_deallocate(void *ptr) {
	
	if(small_allocator.isAllocator(ptr)) {
		memory_usage -= small_allocator.deallocate(ptr);
		num_allocations--;
		return 1;
	}
	
	return 0;
}

/*
 */
void *Allocator::medium_allocate(size_t size) {
	
	// aligned allocation
	size = ALIGNED16(size);
	
	void *ptr = medium_allocator.allocate(size);
	
	if(ptr) {
		memory_usage += size;
		num_allocations++;
		num_frame_allocations++;
	}
	
	return ptr;
}

int Allocator::medium_deallocate(void *ptr) {
	
	if(medium_allocator.isAllocator(ptr)) {
		memory_usage -= medium_allocator.deallocate(ptr);
		num_allocations--;
		return 1;
	}
	
	return 0;
}

/*
 */
void *Allocator::large_allocate(size_t size) {
	
	// aligned allocation
	size = ALIGNED16(size);
	
	void *ptr = large_allocator.allocate(size);
	
	if(ptr) {
		memory_usage += size;
		num_allocations++;
		num_frame_allocations++;
	}
	
	return ptr;
}

int Allocator::large_deallocate(void *ptr) {
	
	if(large_allocator.isAllocator(ptr)) {
		memory_usage -= large_allocator.deallocate(ptr);
		num_allocations--;
		return 1;
	}
	
	return 0;
}

/*
 */
void *Allocator::allocate(size_t size) {
	
	AtomicLock atomic(&lock);
	
	void *ptr = NULL;
	
	if(size < FIXED_SIZE) {
		ptr = fixed_allocate(size);
		if (ptr) {
#ifdef LEAK_DETECT
			StackTraceAdd(ptr);
#endif
			return ptr;
		}
	} else if(size < SMALL_SIZE) {
		ptr = small_allocate(size);
		if (ptr) {
#ifdef LEAK_DETECT
			StackTraceAdd(ptr);
#endif
			return ptr;
		}
	} else if(size < MEDIUM_SIZE) {
		ptr = medium_allocate(size);
		if (ptr) {
#ifdef LEAK_DETECT
			StackTraceAdd(ptr);
#endif
			return ptr;
		}
	} else if(size < LARGE_SIZE) {
		ptr = large_allocate(size);
		if (ptr) {
#ifdef LEAK_DETECT
			StackTraceAdd(ptr);
#endif
			return ptr;
		}
	}
	
	ptr = SystemAllocator::allocate(size);
	if (ptr) {
#ifdef LEAK_DETECT
		StackTraceAdd(ptr);
#endif
		return ptr;
	}
	
	TraceError("Allocator::allocate(): can't allocate %d bytes\n", size);
	
	return NULL;
}

int Allocator::deallocate(void *ptr) {
	
	AtomicLock atomic(&lock);

#ifdef LEAK_DETECT
	StackTraceRemove(ptr);
#endif	

	if(fixed_deallocate(ptr)) return 1;
	if(small_deallocate(ptr)) return 1;
	if(medium_deallocate(ptr)) return 1;
	if(large_deallocate(ptr)) return 1;
	
	return 0;
}

int Allocator::deallocate(void *ptr,size_t size) {
	
	AtomicLock atomic(&lock);
#ifdef LEAK_DETECT

	StackTraceRemove(ptr);
#endif
	if(fixed_deallocate(ptr,size)) return 1;
	if(small_deallocate(ptr)) return 1;
	if(medium_deallocate(ptr)) return 1;
	if(large_deallocate(ptr)) return 1;
	
	return 0;
}

/*
 */
size_t Allocator::getHeapUsage() const {
	size_t heap_usage = 0;
	for(int i = 0; i < num_fixed_allocators; i++) {
		heap_usage += fixed_allocators[i].getHeapUsage();
	}
	heap_usage += small_allocator.getHeapUsage();
	heap_usage += medium_allocator.getHeapUsage();
	heap_usage += large_allocator.getHeapUsage();
	return heap_usage;
}

size_t Allocator::getMemoryUsage() const {
	return memory_usage;
}

int Allocator::getNumAllocations() const {
	return num_allocations;
}

int Allocator::getNumFrameAllocations() {
	int ret = num_frame_allocations;
	num_frame_allocations = 0;
	return ret;
}

/******************************************************************************\
*
* Memory
*
\******************************************************************************/

/*
 */
static int initialized = 0;
static Allocator *allocator = NULL;

/*
 */
//INLINE void *operator new(size_t,void *ptr) { return ptr; }
//INLINE void operator delete(void*,void*) { }

/*
 */
Memory::Memory() {
	
}

/*
 */
int Memory::init() {
	
	assert(initialized == 0 && "Memory::init(): is already initialized");
	initialized = 1;
	memset(gStackTrace, 0, sizeof(gStackTrace));

	if(allocator == NULL) allocator = new(systemAllocate(sizeof(Allocator))) Allocator();
	
	return 1;
}

int Memory::shutdown() {
	
	assert(initialized && "Memory::shutdown(): is not initialized");
	initialized = 0;
	
	return 1;
}

/*
 */
void Memory::info() {
	if(allocator) allocator->info();
}

void Memory::dump() {
	if(allocator) allocator->dump();
}

/*
 */
void *Memory::systemAllocate(size_t size) {
	return malloc(size);
}

void Memory::systemDeallocate(void *ptr) {
	free(ptr);
}

/*
 */
void *Memory::allocate(size_t size) {
	if(size == 0) return NULL;
	if(initialized) return allocator->allocate(size);
	return SystemAllocator::allocate(size);
}

void Memory::deallocate(void *ptr) {
	if(ptr == NULL) return;
	if(allocator == NULL || allocator->deallocate(ptr) == 0) SystemAllocator::deallocate(ptr);
}

void Memory::deallocate(void *ptr,size_t size) {
	if(ptr == NULL) return;
	if(allocator == NULL || allocator->deallocate(ptr,size) == 0) SystemAllocator::deallocate(ptr);
}

/*
 */
size_t Memory::getHeapUsage() {
	return (allocator) ? allocator->getHeapUsage() : 0;
}

size_t Memory::getMemoryUsage() {
	return (allocator) ? allocator->getMemoryUsage() : 0;
}

int Memory::getNumAllocations() {
	return (allocator) ? allocator->getNumAllocations() : 0;
}

int Memory::getNumFrameAllocations() {
	return (allocator) ? allocator->getNumFrameAllocations() : 0;
}
