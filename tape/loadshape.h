/** $Id: loadshape.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file loadshape.h
	@addtogroup loadshape
	@ingroup tape

 @{
 **/
#if 0

#ifndef _LOADSHAPE_H
#define _LOADSHAPE_H

#include "tape.h"
#include "../tape/schedule.h" /* needed for parse_cron() */

EXPORT void new_loadshape(MODULE *mod);

typedef enum {
	LSI_NONE = 0,
	LSI_TIME = 1,
} e_integral;

typedef enum {
	LSP_NOT_USED = -1,
	LSP_DAILY = 0,
	LSP_HOURLY = 1,
	LSP_WEEKLY = 2,
	LSP_MONTHLY = 3,
	LSP_QUARTERLY = 4,
	LSP_YEARLY = 5
} e_period;

typedef enum {
	LSS_NOT_USED = -1,
	LSS_EVERY_MINUTE = 0,
	LSS_EVERY_HOUR = 1,
	LSS_EVERY_DAY = 2
} e_sample;

typedef enum {
	LSO_CSV = 0,
	LSO_SHAPE = 1,
	LSO_PLOT = 2,
	LSO_PNG = 3,
	LSO_JPG = 4
} e_outmode;

class loadshape {
private:
	TIMESTAMP next_period;
	TIMESTAMP next_sample;

	TIMESTAMP sample_len;
	TIMESTAMP period_len;

	schedule_list *sel_list;
public:
	int32 interval;
	int32 limit;
	e_integral integral;
	e_period period;
	int32 period_ex;
	int32 samples;
	int32 sample_rate;
	e_sample sample_mode;
	e_outmode output_mode;
	char256 errmsg;
	char256 output_name;
	char256 prop;
	char256 selection;
	char1024 group;
	TAPESTATUS state;
public:
	static CLASS *oclass;
	static loadshape *defaults;

	loadshape(MODULE *module);
	int create();
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	int commit();

};

#endif // _LOADSHAPE_H

/**@}**/

#endif // zero
