/** $Id: schedule.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file schedule.h
	@addtogroup schedule
	@ingroup tape

 @{
 **/

#ifndef _TAPE_SCHEDULE_H
#define _TAPE_SCHEDULE_H

//#include "residential.h"

#include "tape.h"

EXPORT void new_schedule(MODULE *mod);

class range_list{
public:
	int min, max;
	int children;
	range_list *next;
	
	range_list(int lo, int hi, range_list *list=0){min = lo; max = hi; next = list; children = -1;}
	~range_list(){if(next != 0){delete next;}}
	
	/* count the size of the list */
	int get_count(){if(children < 0) return count(); else return children+1;}
	int count(){if(next != 0){children = next->count(); return children+1;} else {children = 0; return 1;}}

	/* verify all list elements are [a, b] */
	int check(int lo, int hi){return (next ? next->check(lo, hi) : 1) && (min >= lo && max <= hi);}
};

range_list *cron_part(char *, int);

/*	Our "schedule list" is more of a list of trees than a true list.  It acts first like
 *	 a normal queue, but can group similar elements together for speed gains.  The tree
 *	 grouping behavior requires that the start and end for sections of the relevant
 *	 timeframes (year, months, weeks, etc) do NOT overlap, and later time intervals are
 *	 arranged to the left of the parent object.  Gaps between time intervals is normal
 *	 behavior.  If a rule (schedule_list with no children) overlaps with any element
 *	 within a tree, it is kicked to the *next group.  This preserves rule ordering within
 *	 a given set of cron definitions, where overlapping rules are considered a feature.
 *	Note that groups are flagged as "weekly", "monthly", or "neither" based on if they
 *	 use the day_of_week or day_of_month field.  Weekly rules should not be grouped with
 *	 monthly rules.  Rules that include both a monthly and weekly rule should be
 *	 grouped seperately, since the value is valid for both the dom and dow specified in
 *	 that rule.
 */

class schedule_list {
public:
	static const int wor_skip = 0;
	static const int wor_week = 1;
	static const int wor_month = 2;
	static const int wor_both = 3;
	schedule_list *next, *group_left, *group_right;
	int moh_start, moh_end;
	int hod_start, hod_end;
	int dom_start, dom_end;
	int dow_start, dow_end;
	int moy_start, moy_end;
	double scale;
	int dow_or_dom; /* for sanity, groups are either day-of-week or day-of-month ONLY */
public:
	schedule_list();
	schedule_list(range_list *, range_list *, range_list *, range_list *, range_list *, double);
	~schedule_list();
};

/*	parse_cron is used to parse a single line from either a cron file
 *	 or a cron schedule string.  Valid operators include commas, dashes,
 *	 and stars.
 */
schedule_list *parse_cron(char *);

schedule_list *new_slist();
int delete_slist(schedule_list *);

class schedule {
private:
	int parse_schedule();
	int open_sched_file();
	// subscriber list
	schedule_list *sched_list;
public:
	double currval;
	double default_value;
	TIMESTAMP next_ts;
	TAPESTATUS state;
	char256 errmsg;
	char256 filename;	// one or the other
	char1024 sched;
public:
	static CLASS *oclass;
	static schedule *defaults;

	schedule(MODULE *module);
	int create();
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _SCHEDULE_H

/**@}**/
