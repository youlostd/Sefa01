/* Copyright (C) 2005-2010, Unigine Corp. All rights reserved.
 *
 * File:    Memory.h
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

#ifndef __MEMORY_H__
#define __MEMORY_H__


#ifdef _WIN32
#define INLINE		__forceinline
#define FASTCALL	__fastcall
#define RESTRICT	__restrict
#else
#define INLINE		__inline__
#define FASTCALL
#define RESTRICT	__restrict
#endif
#include <stddef.h>
#include <cstdint>
#include <cassert>
#include <cstdio>
void StackTraceAdd(void* p);
void StackTraceRemove(void* p);
void StackTraceLog(FILE* out, int64_t minLeaks);

/*
 */
class Memory {
		
		Memory();
		
	public:
		
		// initialize memory managment system
		static int init();
		static int shutdown();
		
		// memory information
		static void info();
		static void dump();
		
		// system memory
		static void *systemAllocate(size_t size);
		static void systemDeallocate(void *ptr);
		
		// managed memory
		static void *allocate(size_t size);
		static void deallocate(void *ptr);
		static void deallocate(void *ptr,size_t size);
		
		// statistics
		static size_t getHeapUsage();
		static size_t getMemoryUsage();
		static int getNumAllocations();
		static int getNumFrameAllocations();
};

/*
 */
#define USE_NEW_MEMORY
#ifdef USE_NEW_MEMORY
INLINE void *operator new(size_t size) { return Memory::allocate(size); }
INLINE void *operator new[](size_t size) { return Memory::allocate(size); }
INLINE void operator delete(void *ptr) { Memory::deallocate(ptr); }
INLINE void operator delete[](void *ptr) { Memory::deallocate(ptr); }
INLINE void operator delete(void *ptr,size_t size) { Memory::deallocate(ptr,size); }
INLINE void operator delete[](void *ptr,size_t size) { Memory::deallocate(ptr,size); }
#endif 

#endif /* __MEMORY_H__ */
