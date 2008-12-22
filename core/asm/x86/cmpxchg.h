/** $Id: cmpxchg.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file cmpxchg.h
	@defgroup locking Locking memory
 @{
 **/

#ifndef _ASM_CMPXCHG_H
#define _ASM_CMPXCHG_H

static __inline unsigned long cmpxchg(volatile unsigned long *dest,
					    unsigned long xchg, unsigned long comp)
{
	unsigned long prev;
	__asm__ __volatile__("lock; cmpxchgl %1,%2"
				  : "=a"(prev)
				  : "r"(xchg), "m"(*dest), "0"(comp)
				  : "memory");
	return prev;
}

#define HAVE_CMPXCHG

#endif /* _ASM_CMPXCHG_H */

/**@}**/

