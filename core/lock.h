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

#include "object.h"

#ifdef WIN32
	#include <intrin.h>
	#pragma intrinsic(_InterlockedCompareExchange)
	#define cmpxchg(dest, xchg, comp) ((unsigned long) _InterlockedCompareExchange((long *) dest, (long) xchg, (long) comp))
	#define HAVE_CMPXCHG
#elif __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1050
	#include <libkern/OSAtomic.h>
	#define cmpxchg(dest, xchg, comp) (OSAtomicCompareAndSwapLong((long) comp, (long) xchg, (long *) dest) ? xchg : comp)
	#define HAVE_CMPXCHG
#else
	#define cmpxchg(dest, xchg, comp) __sync_val_compare_and_swap(dest, comp, xchg)
	#define HAVE_CMPXCHG
#endif /* defined(WIN32) */

#ifndef HAVE_CMPXCHG
	#define NOLOCKS 1
#endif /* !defined(CMPXCHG) */

#ifdef NOLOCKS
	#define LOCK(flags)
	#define UNLOCK(flags)
	#define LOCK_OBJECT(obj)
	#define UNLOCK_OBJECT(obj)
	#define LOCKED(obj, command)
#else /* !defined(NOLOCKS) */

static __inline int lock(unsigned long *flags)
{
	unsigned int f;
	register int spin=0;
	do {
		spin++;
		f = *flags;
	} while (f & OF_LOCKED || cmpxchg(flags, f | OF_LOCKED, f) & OF_LOCKED);
	return spin;
}

static __inline void unlock(unsigned long *flags)
{
	unsigned int f;

	do {
		f = *flags;
	} while (f & OF_LOCKED && cmpxchg(flags, f ^ OF_LOCKED, f) != f);
}

#define LOCK(flags) ((*(callback->lock_count))++, (*(callback->lock_spin))+=lock(flags)) /**< Locks an item */
#define UNLOCK(flags) unlock(flags) /**< Unlocks an item */
#define LOCK_OBJECT(obj) ((*(callback->lock_count))++, (*(callback->lock_spin))+=lock(&((obj)->flags))) /**< Locks an object */
#define UNLOCK_OBJECT(obj) unlock(&((obj)->flags)) /**< Unlocks an object */
#define LOCKED(obj,command) (LOCK_OBJECT(obj),(command),UNLOCK_OBJECT(obj))

#endif /* defined(NOLOCKS) */

#endif /* _LOCK_H */

/**@}**/

