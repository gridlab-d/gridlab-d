/** $Id: lock.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file lock.h
	@defgroup locking Locking memory
	@ingroup module_api

	Memory locking is implemented using compare and exchange methods.  
	Any time more than one object can concurrently write to the same
	region of memory, it is necessary to implement locking to prevent
	one object from overwriting the changes made by another.  
 @{	  
 **/

#include "lock.h"
#include "exception.h"
#include <stdio.h>

//#define LOCKTRACE // enable this to trace locking events back to variables
#define MAXSPIN 1000000000

/** Determine locking method 
 **/
#define METHOD0 /* locking method as of 3.0 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/************************************************************************************** 
   IMPORTANT NOTE: it is vital that the platform specific implementations be tested for
   first because some compilers support __sync_bool_compare_and_swap on platforms that
   have a native (often better) implementation of atomic operations.
 **************************************************************************************/
#if defined(__APPLE__)
	#include <libkern/OSAtomic.h>
	#define atomic_compare_and_swap(dest, comp, xchg) OSAtomicCompareAndSwap32Barrier(comp, xchg, (volatile int32_t *) dest)
	#define atomic_increment(ptr) OSAtomicIncrement32Barrier((volatile int32_t *) ptr)
#elif defined(WIN32)
	#include <intrin.h>
	#pragma intrinsic(_InterlockedCompareExchange)
	#pragma intrinsic(_InterlockedIncrement)
	#define atomic_compare_and_swap(dest, comp, xchg) (_InterlockedCompareExchange((volatile long *) dest, xchg, comp) == comp)
	#define atomic_increment(ptr) _InterlockedIncrement((volatile long *) ptr)
	#ifndef inline
		#define inline __inline
	#endif
#elif defined HAVE___SYNC_BOOL_COMPARE_AND_SWAP
	#define atomic_compare_and_swap __sync_bool_compare_and_swap
	#ifdef HAVE___SYNC_ADD_AND_FETCH
		#define atomic_increment(ptr) __sync_add_and_fetch((volatile long *)ptr, 1)
	#else
		static inline unsigned int atomic_increment(unsigned int *ptr)
		{
			unsigned int value;
			do {
				value = *(volatile long *)ptr;
			} while (!__sync_bool_compare_and_swap((volatile long*)ptr, value, value + 1));
			return value;
		}
	#endif
#else
	#error "Locking is not supported on this system"
#endif

/** Enable lock trace 
 **/
#ifdef LOCKTRACE // this code should only be used in care is mystery lock timeouts
typedef struct s_locklist {
	const char *name;
	unsigned int *lock;
	unsigned int last_value;
	struct s_locklist *next;
} LOCKLIST;
LOCKLIST *locklist = NULL;
/** Register a lock trave
 **/
void register_lock(const char *name, unsigned int *lock)
{
	LOCKLIST *item = (LOCKLIST*)malloc(sizeof(LOCKLIST));
	item->name = name;
	item->lock = lock;
	item->last_value = *lock;
	if ( *lock!=0 )
		printf("%s lock initial value not zero (%d)\n", name, *lock);
	item->next = locklist;
	locklist = item;
}
/** Check a lock trace
 **/
void check_lock(unsigned int *lock, bool write, bool unlock)
{
	LOCKLIST *item;

	// lock locklist
	static unsigned int check_lock=0;
	unsigned int timeout = MAXSPIN;
	unsigned int value;
	do {
		value = check_lock;
		if ( timeout--==0 ) 
			throw_exception("check lock timeout");
	} while ((value&1) || !atomic_compare_and_swap(&check_lock, value, value + 1));

	for ( item=locklist ; item!=NULL ; item=item->next )
	{
		if ( item->lock==lock )
			break;
	}
	if ( item==NULL )
	{
		printf("%s %slock(%p) = %d (unregistered)\n", 
			write?"write":"read ", 
			unlock?"un":"  ",
			lock,
			*lock);
		register_lock("unregistered",lock);
	}
	else 
	{
		bool no_lock = unlock&&((*lock&1)!=1);
//		bool damage = abs(item->last_value-*lock)>1;
//		if ( damage ) // found a registered lock that was damaged
//			printf("%s %slock(%p) = %d (%s) - possible damage (last=%d)\n", write?"write":"read ", unlock?"un":"  ",lock,*lock,item->name, item->last_value);
		if ( no_lock ) // found a registered lock that was not locked
			printf("%s %slock(%p) = %d (%s) - no lock detected (last=%d)\n", write?"write":"read ", unlock?"un":"  ",lock,*lock,item->name, item->last_value);
		item->last_value = *lock;
	}

	// unlock locklist
	atomic_increment(&check_lock);
}
#else
#define check_lock(X,Y,Z)
void register_lock(unsigned int *lock)
{
	// do nothing
}
#endif

#if defined METHOD0 
/**********************************************************************************
 * SINGLE LOCK METHOD
 **********************************************************************************/

/* This locking method uses a standard spinlock.
   The lock/unlock operations work as follows:
   (1) a lock is attempted when the low bit of the lock value is 0
   (2) an atomic compare-and-swap (CAS) operation is performed to take the lock by setting the low bit to 1
   (3) if the CAS operation fails, the lock process starts over at (1)
   (4) to unlock the lock value is incremented (which clears the low bit and increments the lock count).
 */
/** Read lock
 **/
extern "C" void rlock(unsigned int *lock)
{
	unsigned int timeout = MAXSPIN;
	unsigned int value;
	extern unsigned int rlock_count, rlock_spin;
	check_lock(lock,false,false);
	atomic_increment(&rlock_count);
	do {
		value = (*lock);
		atomic_increment(&rlock_spin);
		if ( timeout--==0 ) 
			throw_exception("read lock timeout");
	} while ((value&1) || !atomic_compare_and_swap(lock, value, value + 1));
}
/** Write lock 
 **/
extern "C" void wlock(unsigned int *lock)
{
	unsigned int timeout = MAXSPIN;
	unsigned int value;
	extern unsigned int wlock_count, wlock_spin;
	check_lock(lock,true,false);
	atomic_increment(&wlock_count);
	do {
		value = (*lock);
		atomic_increment(&wlock_spin);
		if ( timeout--==0 ) 
			throw_exception("write lock timeout");
	} while ((value&1) || !atomic_compare_and_swap(lock, value, value + 1));
}
/** Read unlock
 **/
extern "C" void runlock(unsigned int *lock)
{
	unsigned int value = *lock;
	check_lock(lock,false,true);
	atomic_increment(lock);
}
/** Write unlock
 **/
extern "C" void wunlock(unsigned int *lock)
{
	unsigned int value = *lock;
	check_lock(lock,true,true);
	atomic_increment(lock);
}

#elif defined METHOD1 
/**********************************************************************************
 * WEAK R/W LOCK METHOD
 **********************************************************************************/

/* This locking method uses a modified double spinlock
   The read lock operation works as follows:
   (1) a lock is attempted when the low bit is set to zero
   (2) an atomic compare-and-swap (CAS) operation is performed to take the lock by setting the high bit to 1
   (3) if the CAS operation fails, the lock process starts over at (1)
   (4) to unlock the lock value is set to zero (which clears all the bits)
   The write lock operation works as follows:
   (1) a lock is attempted when both the low and high bits are zero
   (2) an atomic CAS operation is performed to take the lock by setting the low bit to 1
   (3) to unlock the lock value is set to zero (which clears all the bits)
 */
static inline void rlock(unsigned int *lock)
{
	unsigned int value;

	do {
		value = (*lock);
	} while ((value&1) || !atomic_compare_and_swap(lock, value, value|0x80000000));
}
static inline void wlock(unsigned int *lock)
{
	unsigned int value;

	do {
		value = (*lock);
	} while ((value&0x80000001) || !atomic_compare_and_swap(lock, value, value + 1));
}
static inline void _unlock(unsigned int *lock)
{
	*lock = 0;
}
#define runlock _unlock
#define wunlock _unlock

#elif defined METHOD2
/**********************************************************************************
 * STRONG R/W LOCK METHOD
 **********************************************************************************/

#define WBIT 0x80000000
#define RBITS 0x7FFFFFFF

static inline void rlock(unsigned int *lock)
{
	unsigned int test;

	// 1. Wait for exclusive write lock to be released, if any
	// 2. Increment reader counter
	do {
		test = *lock;
	} while (test & WBIT || !atomic_compare_and_swap(lock, test, test + 1));
}

static inline void wlock(unsigned int *lock)
{
	unsigned int test;

	// 1. Wait for exclusive write lock to be released, if any
	// 2. Take exclusive write lock
	do {
		test = *lock;
	} while (test & WBIT || !atomic_compare_and_swap(lock, test, test | WBIT));
	// 3. Wait for readers to complete before proceeding
	while ((*lock) & RBITS);
}

static inline void runlock(unsigned int *lock)
{
	unsigned int test;

	// Decrement the reader counter
	do {
		test = *lock;
	} while (!atomic_compare_and_swap(lock, test, test - 1));
}

static inline void wunlock(unsigned int *lock)
{
	unsigned int test;

	// Release write lock
	do {
		test = *lock;
	} while (!atomic_compare_and_swap(lock, test, test & RBITS));
}

#elif defined METHOD3
/**********************************************************************************
 * SEQLOCK METHOD
 **********************************************************************************/

// Start a loop for reading values
#define rlock(lock) {          \
	unsigned int lock_tmp;      \
	do {                        \
		lock_tmp = *lock;

// End the loop, checking that no writes occurred or are in progress
#define runlock(lock) } while (lock_tmp != *(lock) || lock_tmp & 1);   \
}

static inline void wlock(unsigned int *lock)
{
	unsigned int test;

	// 1. Wait for exclusive write lock to be released, if any
	// 2. Take exclusive write lock
	do {
		test = *lock;
	} while (test & 1 || !atomic_compare_and_swap(lock, test, test + 1));
}

static inline void wunlock(unsigned int *lock)
{
	// Release write lock
	do {
		test = *lock;
	} while (!atomic_compare_and_swap(lock, test, test + 1));
}

/** @} **/
#endif
