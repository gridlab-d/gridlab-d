/** $Id: deltamode.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2011 Battelle Memorial Institute
 **/

#ifndef _DELTAMODE_H
#define _DELTAMODE_H

STATUS delta_init(void); /* initialize delta mode - 0 on fail */
DT delta_update(void); /* update in delta mode - <=0 on fail, seconds to advance clock if ok */
DT delta_modedesired(DELTAMODEFLAGS *flags); /* ask module how many seconds until deltamode is needed, 0xfffffff(DT_INVALID)->error, oxfffffffe(DT_INFINITY)->no delta mode needed */
static DT delta_preupdate(void); /* send preupdate messages ; dt==0|DT_INVALID failed, dt>0 timestep desired in deltamode  */
static SIMULATIONMODE delta_interupdate(DT timestep, unsigned int iteration_count_val); /* send interupdate messages  - 0=INIT (used?), 1=EVENT, 2=DELTA, 3=DELTA_ITER, 255=ERROR */
static STATUS delta_postupdate(void); /* send postupdate messages - 0 = FAILED, 1=SUCCESS */

typedef struct {
	clock_t t_init; /**< time in initiation */
	clock_t t_preupdate; /**< time in preupdate */
	clock_t t_update; /**< time in update */
	clock_t t_interupdate; /**< time in interupdate */
	clock_t t_postupdate; /**< time in postupdate */
	unsigned int64 t_delta; /**< total elapsed delta mode time (s) */
	unsigned int64 t_count; /**< number of updates */
	unsigned int64 t_max;	/**< maximum delta (ns) */
	unsigned int64 t_min;	/**< minimum delta (ns) */
	char module_list[1024]; /**< list of active modules */
} DELTAPROFILE;
DELTAPROFILE *delta_getprofile(void);

#endif
