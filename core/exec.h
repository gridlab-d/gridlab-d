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

void exec_mls_create(void);
void exec_mls_init(void);
void exec_mls_suspend(void);
void exec_mls_resume(TIMESTAMP next_pause);
void exec_mls_done(void);
void exec_mls_statewait(unsigned states);
void exec_slave_node();

void exec_sync_reset(struct sync_data *d);
void exec_sync_merge(struct sync_data *to, struct sync_data *from);
void exec_sync_set(struct sync_data *d, TIMESTAMP t);
TIMESTAMP exec_sync_get(struct sync_data *d);
unsigned int exec_sync_getevents(struct sync_data *d);
int exec_sync_ishard(struct sync_data *d);
int exec_sync_isnever(struct sync_data *d);
int exec_sync_isinvalid(struct sync_data *d);
STATUS exec_sync_getstatus(struct sync_data *d);

/* Exit codes */
typedef int EXITCODE;
#define XC_SUCCESS 0 /* per system(3) */
#define XC_EXFAILED -1 /* exec/wait failure - per system(3) */
#define XC_ARGERR 1 /* error processing command line arguments */
#define XC_ENVERR 2 /* bad environment startup */
#define XC_TSTERR 3 /* test failed */
#define XC_USRERR 4 /* user reject terms of use */
#define XC_RUNERR 5 /* simulation did not complete as desired */
#define XC_INIERR 6 /* initialization failed */
#define XC_PRCERR 7 /* process control error */
#define XC_SVRKLL 8 /* server killed */
#define XC_IOERR 9 /* I/O error */
#define XC_SHFAILED 127 /* shell failure - per system(3) */
#define XC_SIGNAL 128 /* signal caught - must be or'd with SIG value if known */
#define XC_SIGINT (XC_SIGNAL|SIGINT) /* SIGINT caught */
#define XC_EXCEPTION 255 /* exception caught */

EXITCODE exec_setexitcode(EXITCODE);
EXITCODE exec_getexitcode(void);
const char *exec_getexitcodestr(EXITCODE);

int exec_add_createscript(const char *file);
int exec_add_initscript(const char *file);
int exec_add_syncscript(const char *file);
int exec_add_termscript(const char *file);
int exec_add_scriptexport(const char *file);
EXITCODE exec_run_initscripts(void);
EXITCODE exec_run_syncscripts(void);
EXITCODE exec_run_termscripts(void);

int64 exec_clock(void);

#ifdef __cplusplus
}
#endif

#ifndef max
#define max(n, m) ((n) > (m) ? (n) : (m))
#endif

#endif

/**@}*/
