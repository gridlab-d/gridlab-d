/** $Id: metrics.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file metrics.h
	@addtogroup metrics
	@ingroup MODULENAME

 @{
 **/

#ifndef _metrics_H
#define _metrics_H

#include <stdarg.h>
#include "gridlabd.h"

#define CLEARBUF(BUF,N) ((char*)(memset((BUF),0,((N)>>3)+1)))
#define MAKEBUF(N) CLEARBUF((char*)(malloc(((N)>>3)+1)),N)
#define BITSET(BUF,N) (((BUF)[(N)>>3])|=(1<<((N)&7)))
#define BITCLR(BUF,N) (((BUF)[(N)>>3])&=(~(1<<((N)&7))))
#define COUNTBITS(BUF,N) (0 /* TODO */)

typedef struct s_metrics {
	int16 reporting_period;
	double saifi;
	double saidi;
	double caidi;
	double caifi;
	/** @todo other reliability metrics go here */
	struct s_metrics *next;
} METRICS;

typedef struct s_event {
	TIMESTAMP event_time;
	/* IEEE 1366-2003 data collected */
	int32 Ni;	/* number of interrupted customer for each sustained interruption event */
	double ri;	/* restoration time for each interruption event */
	double Li;	/* connection kVA load interrupted for each interruption event */
	double LT;	/* total connected kVA load served */
	struct s_event *next;
} EVENT;

typedef struct s_totals {
	int16 period;
	int16 Nevents;
	int32 CI;	/* customers interrupted = Sum[Ni]*/
	double CMI;	/* customer.minutes interrupted = Sum[ri*Ni] */
	int32 CN;	/* customers who experienced at least one sustained interruption */
} TOTALS;

class metrics {
private:
	EVENT *eventlist;
	FINDLIST *customers;
	FINDLIST *interrupted;
	FILE *fp;
	TOTALS totals;
	METRICS values;
	bool first_year;
public:
	static double major_event_threshold;
	static bool report_event_log;
public:
	char report_file[1024];	/**< the file to which the report is written */
public:
	/* required implementations */
	metrics(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static metrics *defaults;
public:
	inline EVENT *get_event() { return eventlist;};
	inline void add_event(EVENT *p) { p->next=eventlist; eventlist=p;};
	inline void del_event() {if (eventlist) eventlist=eventlist->next;};
	OBJECT *start_event(EVENT *pEvent, FINDLIST *objectlist,const char *targets,char *event_values,char *old_values, int size);
	void end_event(OBJECT* obj, const char *targets, char *old_values);
private:
	double asai(void);
	double ctaidi(void);
	double caidi(void);
	double saidi(void);
	double caifi(void);
	double saifi(void);
};

#endif

/**@}*/
