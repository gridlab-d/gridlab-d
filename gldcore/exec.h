/** $Id: exec.h 4738 2014-07-03 00:55:39Z dchassin $
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
	unsigned int hard_event; /**< non-zero for hard events that can effect the advance step-to */
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
void exec_sleep(unsigned int usec);
int64 exec_clock(void);

void exec_mls_create(void);
void exec_mls_init(void);
void exec_mls_suspend(void);
void exec_mls_resume(TIMESTAMP next_pause);
void exec_mls_done(void);
void exec_mls_statewait(unsigned states);
void exec_slave_node();
int exec_run_createscripts(void);

void exec_sync_reset(struct sync_data *d);
void exec_sync_merge(struct sync_data *to, struct sync_data *from);
void exec_sync_set(struct sync_data *d, TIMESTAMP t,bool deltaflag);
TIMESTAMP exec_sync_get(struct sync_data *d);
unsigned int exec_sync_getevents(struct sync_data *d);
int exec_sync_ishard(struct sync_data *d);
int exec_sync_isnever(struct sync_data *d);
int exec_sync_isinvalid(struct sync_data *d);
STATUS exec_sync_getstatus(struct sync_data *d);

EXITCODE exec_setexitcode(EXITCODE);
EXITCODE exec_getexitcode(void);
const char *exec_getexitcodestr(EXITCODE);

int exec_add_createscript(const char *file);
int exec_add_initscript(const char *file);
int exec_add_syncscript(const char *file);
int exec_add_commitscript(const char *file);
int exec_add_termscript(const char *file);
int exec_add_scriptexport(const char *file);
EXITCODE exec_run_initscripts(void);
EXITCODE exec_run_syncscripts(void);
EXITCODE exec_run_commitscripts(void);
EXITCODE exec_run_termscripts(void);

int exec_schedule_dump(TIMESTAMP interval,char *filename);
int64 exec_clock(void);

#ifdef __cplusplus
}
#endif

#ifndef max
#define max(n, m) ((n) > (m) ? (n) : (m))
#endif

#endif

/**@}*/
