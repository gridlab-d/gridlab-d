/** $Id: cmpxchg.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file cmpxchg.h
	@defgroup locking Locking memory
 @{
 **/

#ifndef _ASM_STUB_CMPXCHG_H
#define _ASM_STUB_CMPXCHG_H

#if defined(__i386__)
	#include "x86/cmpxchg.h"
#elif defined(__x86_64__)
	#include "x86_64/cmpxchg.h"
#elif defined(__ia64__)
	#include "ia64/cmpxchg.h"
#else
	#warning This machine does not have support for cmpxch; locking will most likely not work.
	#undef HAVE_CMPXCHG
#endif

#endif /* _ASM_STUB_CMPXCHG_H */

/**@}**/

