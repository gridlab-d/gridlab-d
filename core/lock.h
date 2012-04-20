/** $Id: lock.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file lock.h
	@defgroup locking Locking memory
	@ingroup module_api

	Memory locking is implemented using compare and exchange methods.  
	Any time more than one object can concurrently write to the same
	region of memory, it is necessary to implement locking to prevent
	one object from overwriting the changes made by another.  
	For example, more than one link can simultaneously update the 
	admittance and current injection accumulators in nodes.  Thus
	the following code is required to prevent two objects from simultaneously
	reading the same accumulator and posting their modifications without
	considering the other's contribution.  
	
	@code
	complex I = to->V * Y;
	LOCK_OBJECT(from);
	from->Ys += Y;
	from->YVs -= I;
	UNLOCK_OBJECT(from);
	@endcode

	Without locking, only the second one's contribution would be counted
	and the first one's would be lost.
 @{
 **/

#ifndef _LOCK_H
#define _LOCK_H

#define METHOD0 /* use new locking method as of 3.0 */

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
static inline void rlock(unsigned int *lock)
{
	unsigned int value;

	do {
		value = (*lock);
	} while ((value&1) || !atomic_compare_and_swap(lock, value, value + 1));
}
static inline void wlock(unsigned int *lock)
{
	unsigned int value;

	do {
		value = (*lock);
	} while ((value&1) || !atomic_compare_and_swap(lock, value, value + 1));
}
static inline void runlock(unsigned int *lock)
{
	unsigned int value = *lock;
	atomic_increment(lock);
}
static inline void wunlock(unsigned int *lock)
{
	unsigned int value = *lock;
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

#elif define METHOD3
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


#endif

#define READLOCK(X) rlock(X); /**< Locks an item for reading (allows other reads but blocks write) */
#define WRITELOCK(X) wlock(X); /**< Locks an item for writing (blocks all operations) */
#define READUNLOCK(X) runlock(X); /**< Unlocks an read lock */
#define WRITEUNLOCK(X) wunlock(X); /**< Unlocks a write lock */
#define LOCK(X) WRITELOCK(X); /**< @todo this is deprecated and should not be used anymore */
#define UNLOCK(X) WRITEUNLOCK(X); /**< @todo this is deprecated and should not be used anymore */

#define READLOCK_OBJECT(X) READLOCK(&((X)->lock)) /**< Locks an object for reading */
#define WRITELOCK_OBJECT(X) WRITELOCK(&((X)->lock)) /**< Locks an object for writing */
#define READUNLOCK_OBJECT(X) READUNLOCK(&((X)->lock)) /**< Unlocks an object */
#define WRITEUNLOCK_OBJECT(X) WRITEUNLOCK(&((X)->lock)) /**< Unlocks an object */
#define LOCK_OBJECT(X) WRITELOCK_OBJECT(X); /**< @todo this is deprecated and should not be used anymore */
#define UNLOCK_OBJECT(X) WRITEUNLOCK_OBJECT(X); /**< @todo this is deprecated and should not be used anymore */

#define LOCKED(X,C) {WRITELOCK_OBJECT(X);(C);WRITEUNLOCK_OBJECT(X);} /**< @todo this is deprecated and should not be used anymore */

#endif /* _LOCK_H */

/**@}**/

