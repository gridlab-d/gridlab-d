/** $Id: supervisory_control.h$
	Copyright (C) 2008 Battelle Memorial Institute
	@file supervisory_control.h
	@addtogroup auction
	@ingroup market

 @{
 **/

#ifndef _supervisory_control_H
#define _supervisory_control_H

#include <stdarg.h>

#include "gridlabd.h"
#include "market.h"
#include "bid.h"
#include "collect.h"

class supervisory_control : public gld_object {
public:
	char32 unit;		/**< unit of quantity (see unitfile.txt) */
	double period;		/**< time period of auction closing (s) */
	int64 market_id;	/**< id of market to clear */
	double droop;
	double nom_freq;
	double frequency_deadband;
	typedef enum {
		OVER_FREQUENCY=0,
		UNDER_FREQUENCY=1,
		OVER_UNDER_FREQUENCY=2,
	} PFCMODE;
	enumeration PFC_mode;
	typedef enum {
		SORT_NONE=0,
		SORT_POWER_INCREASE=1,
		SORT_POWER_DECREASE=2,
		SORT_VOLTAGE=3,
		SORT_VOLTAGE2=4,
	} SORTMODE;
	enumeration sort_mode;
	TIMESTAMP clearat;	/**< next clearing time */
	TIMESTAMP nextclear() const;
	int submit(OBJECT *from, double power, double voltage_deviation, int key=-1, int state=-1);
public:
	/* required implementations */
	supervisory_control(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static supervisory_control *defaults;
private:
	void fetch_double(double **prop, char *name, OBJECT *parent);
	int submit_nolock(OBJECT *from, double power, double PFC_state, int key=-1, int state=-1);
	collect PFC_collect;
	int n_bids_on;
	int n_bids_off;
};

#endif

/**@}*/
	