// $Id: pqload.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _PQLOAD_H
#define _PQLOAD_H

#include "node.h"

typedef struct s_schedule_list {
	struct s_schedule_list *next;
	int moh_start, moh_end;
	int hod_start, hod_end;
	int dom_start, dom_end;
	int dow_start, dow_end;
	int moy_start, moy_end;
	double scale;
} SCHED_LIST;

SCHED_LIST *new_slist();
int delete_slist(SCHED_LIST *);

class pqload : public load
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
	static pqload *defaults;
	static double zero_F;
public:
	enum {LC_UNKNOWN=0, LC_RESIDENTIAL, LC_COMMERCIAL, LC_INDUSTRIAL, LC_AGRICULTURAL} load_class;
	double imped_p[6];
	double imped_q[6];
	double current_m[6];
	double current_a[6];
	double power_p[6];
	double power_q[6];
	double input[6]; /* temp, humidity, solar, wind, rain, 1.0 */
	double output[6]; /* Zp, Zq, Im, Ia, Pp, Pq */
	complex kZ, kI, kP;

	char1024 schedule;
	OBJECT *weather;
	PROPERTY *temperature, *humidity, *solar, *wind, *rain;
	double temp_nom;

	SCHED_LIST *sched;
	TIMESTAMP sched_until;

	int create(void);

	int build_sched(char *, SCHED_LIST *);
	pqload(MODULE *mod);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	inline pqload(CLASS *cl = oclass) : load(cl){};
	int isa(char *classname);
};

#endif // _LOAD_H

