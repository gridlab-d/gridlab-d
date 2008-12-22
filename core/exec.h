/** $Id: exec.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file exec.h
	@addtogroup exec
	@ingroup core
 @{
 **/


#ifndef _EXEC_H
#define _EXEC_H

#include <setjmp.h>
#include "globals.h"
#include "index.h"

struct sync_data {
	TIMESTAMP step_to; /**< time to advance to */
	int hard_event; /**< non-zero for hard events that can effect the advance step-to */
	STATUS status; /**< the current status */
}; /**< the synchronization state structure */

struct thread_data {
	int count; /**< the thread count */
	struct sync_data *data; /**< pointer to the sync state structure */
};

#ifdef __cplusplus
extern "C" {
#endif
int exec_init(void);
STATUS exec_start(void);
char *simtime(void);
STATUS t_setup_ranks(void);
INDEX **exec_getranks(void);
#ifdef __cplusplus
}
#endif

#ifndef max
#define max(n, m) ((n) > (m) ? (n) : (m))
#endif

#endif

/**@}*/
