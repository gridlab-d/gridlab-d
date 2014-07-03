/** $Id: schedule.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file schedule.cpp
	@addtogroup schedule
	@ingroup tape


 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <cctype>

#include "../tape/schedule.h"


/*	cron_part takes the individual schedule pattern and returns a
 *	 list of the ranges found within.  The comma, dash, and asterisk
 *	 operators are recognized.  A range of -1,-1 indicates that
 *	 an asterisk was found.  A range of n,n indicates that one value
 *	 was found and should use 'straight' equality.  Elsewise,
 *	 ranges are expected to be treated as [a, b).
 */
range_list *cron_part(char *part, int first=0){
	int off = 0, lo = 0, hi = 0, flip = 0, has_num = 0;
	range_list *list = 0;// = new range_list();
	size_t i = 0, len = strlen(part);
	char num[2] = "";
	
	/* first == 0 indicates first token, legitimate location for star operator */
	if(part[0] == '*' && first == 0){
		return new range_list(-1, -1);
	}

	for(i = 0; i < len && part[i] != 0; ++i){
		if(isdigit(part[i])){
			if(flip == 0){
				++has_num;
				lo = (lo * 10) + (part[i] - '0');
			} else {
				++has_num;
				hi = (hi * 10) + (part[i] - '0');
			}
		} else if(part[i] == '-'){
			if(flip == 0){
				flip = 1;
				has_num = 0;
			} else {
				gl_error("invalid cron part: cannot use multiple dashes!");
				return 0;
			}
		} else if(part[i] == '*'){
			gl_error("invalid cron part: star only valid atomically");
			return 0;
		} else if(part[i] == ','){ /* recurse */
			// make sure we've read a value, or that we're not interrupting a dash operator
			if(has_num == 0){
				gl_error("invalid cron part: comma without prior value, or comma interrupting a dash operator");
				return 0;
			}
			list = cron_part(part+i+1, 1);
			if(list == 0){
				return 0; /* assume already error'ed */
			}
			break; /* avoid processing more of the string with this object */
		}
	}

	if(hi < lo){
		if(flip == 0){
			hi = lo;
		} else {
			gl_error("invalid cron part: cannot use a descending range");
			return 0;
		}
	}
	return new range_list(lo, hi, list);
}

/*	parse_cron will be handed an individual line from either a
 *	 cron file or a semicolon-delimited string of cron parts.
 *	the function should return a schedule_list consisting of a
 *	 tree with all the parts for the particular line fed in.
 *  
 */
schedule_list *parse_cron(char *cron_str){
	double value = 0.0;
	char *mohs, *hods, *doms, *moys, *dows, *vals;
	char cron_cpy[128];
	range_list *mohr = 0, *hodr = 0, *domr = 0, *moyr = 0, *dowr = 0;
	range_list *mohptr = 0, *hodptr = 0, *domptr = 0, *moyptr = 0, *dowptr = 0;
	range_list **moh = 0, **hod = 0, **dom = 0, **moy = 0, **dow = 0;
	schedule_list **sheap = 0, **sheap2 = 0;
	int oobct = 0, listct = 0, i = 0, j = 0, k = 0, l = 0, m = 0, idx = 0;

	strcpy(cron_cpy, cron_str);

	mohs = strtok(cron_cpy, " \t\n\r");
	hods = strtok(NULL, " \t\n\r");
	doms = strtok(NULL, " \t\n\r");
	moys = strtok(NULL, " \t\n\r");
	dows = strtok(NULL, " \t\n\r");
	vals = strtok(NULL, " \t\n\r");
	
	if((mohs && hods && doms && moys && dows) == 0){
		gl_error("Insufficient arguements in cron line \"%s\"", cron_str);
		return 0;
	}

	mohr = cron_part(mohs);
	hodr = cron_part(hods);
	domr = cron_part(doms);
	moyr = cron_part(moys);
	dowr = cron_part(dows);
	
	if((mohr && hodr && domr && moyr && dowr) == 0){
		gl_error("Unable to parse arguements in cron line \"%s\"", cron_str);
		return 0;
	}

	/* -1 valid lower band for * operator */
	if(mohr->check(-1, -1) == 0 && mohr->check(0,60) == 0){
		gl_error("minute_of_hour out of bounds: %s", cron_str);
		++oobct;
	}
	if(hodr->check(-1, -1) == 0 && hodr->check(0,24) == 0){
		gl_error("hour_of_day out of bounds: %s", cron_str);
		++oobct;
	}
	if(domr->check(-1, -1) == 0 && domr->check(0,31) == 0){
		gl_error("day_of_month out of bounds: %s", cron_str);
		++oobct;
	}
	if(moyr->check(-1, -1) == 0 && moyr->check(0,12) == 0){
		gl_error("month_of_year out of bounds: %s", cron_str);
		++oobct;
	}
	if(dowr->check(-1, -1) == 0 && dowr->check(0,7) == 0){
		gl_error("day_of_week out of bounds: %s", cron_str);
		++oobct;
	}

	if(oobct > 0){
		gl_error("out-of-bound cron time ranges can cause incorrect schedule calculations");
		if(mohr != 0) delete mohr;
		if(hodr != 0) delete hodr;
		if(domr != 0) delete domr;
		if(moyr != 0) delete moyr;
		if(dowr != 0) delete dowr;
		return 0;
	}

	if(vals == NULL){
		value = 1.0;
	} else {
		value = atof(vals);
	}

	listct = mohr->get_count() * hodr->get_count() * domr->get_count() * moyr->get_count() * dowr->get_count();
	if(listct < 1){
		gl_error("non-postive product of the range list counts, something strange happened");
		return 0;
	}

	sheap = (schedule_list **)malloc(sizeof(schedule_list *) * listct); // not really heaped so much as a combination of the range lists...
	sheap2 = (schedule_list **)malloc(sizeof(schedule_list *) * listct);

	moh = (range_list **)malloc(sizeof(range_list *) * mohr->get_count());
	hod = (range_list **)malloc(sizeof(range_list *) * hodr->get_count());
	dom = (range_list **)malloc(sizeof(range_list *) * domr->get_count());
	moy = (range_list **)malloc(sizeof(range_list *) * moyr->get_count());
	dow = (range_list **)malloc(sizeof(range_list *) * dowr->get_count());

	dowptr = dowr;
	for(i = 0; i < dowr->get_count() && dowptr != NULL; ++i){
		dow[i] = dowptr;
		dowptr = dowptr->next;
	}
	moyptr = moyr;
	for(j = 0; j < moyr->get_count() && moyptr != NULL; ++j){
		moy[j] = moyptr;
		moyptr = moyptr->next;
	}
	domptr = domr;
	for(k = 0; k < domr->get_count() && domptr != NULL; ++k){
		dom[k] = domptr;
		domptr = domptr->next;
	}
	hodptr = hodr;
	for(l = 0; l < hodr->get_count() && hodptr != NULL; ++l){
		hod[l] = hodptr;
		hodptr = hodptr->next;
	}
	mohptr = mohr;
	for(m = 0; m <mohr->get_count() && mohptr != NULL; ++m){
		moh[m] = mohptr;
		mohptr = mohptr->next;
	}

	for(i = 0; i < dowr->get_count(); ++i){
		for(j = 0; j < moyr->get_count(); ++j){
			for(k = 0; k < domr->get_count(); ++k){
				for(l = 0; l < hodr->get_count(); ++l){
					for(m = 0; m <mohr->get_count(); ++m){
						sheap[idx] = new schedule_list(moh[m], hod[l], dom[k], moy[j], dow[i], value);
						++idx;
					}
				}
			}
		}
	}

	/* should sort from sheap into sheap2 and appropriately create groups */
	for(i = 0; i < listct-1; ++i){
		sheap[i]->group_right = sheap[i+1];
	}
	sheap[i]->group_right = 0;

	return sheap[0];
}

schedule_list::schedule_list(range_list *moh, range_list *hod, range_list *dom, range_list *moy, range_list *dow, double val){
	moh_start = moh->min;
	moh_end = moh->max;
	hod_start = hod->min;
	hod_end = hod->max;
	dom_start = dom->min;
	dom_end = dom->max;
	dow_start = dow->min;
	dow_end = dow->max;
	moy_start = moy->min;
	moy_end = moy->max;
	scale = val;
	next = 0;
	group_left = 0;
	group_right = 0;
	/* could do += but if it works, don't fix it */
	if(dow_start == -1 && dow_end == -1 && dom_start == -1 && dom_end == -1){
		dow_or_dom = schedule_list::wor_skip;
	} else if(dow_start == -1 && dow_end == -1 && dom_start != -1 && dom_end != -1){
		dow_or_dom = schedule_list::wor_month;
	} else if(dow_start != -1 && dow_end != -1 && dom_start == -1 && dom_end == -1){
		dow_or_dom = schedule_list::wor_week;
	} else {
		dow_or_dom = wor_skip;
	}
}

//////////////////////////////////////////////////////////////////////////
// schedule CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* schedule::oclass = NULL;
schedule *schedule::defaults = NULL;

schedule::schedule(MODULE *module) 
{

	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"schedule",sizeof(schedule),PC_PRETOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_double, "value", PADDR(currval), PT_ACCESS, PA_REFERENCE,
			PT_double, "default_value", PADDR(default_value),
			PT_timestamp, "next_ts", PADDR(next_ts), PT_ACCESS, PA_REFERENCE,
			PT_enumeration, "state", PADDR(state), PT_ACCESS, PA_REFERENCE,
				PT_KEYWORD, "INIT", TS_INIT,
				PT_KEYWORD, "OPEN", TS_OPEN,
				PT_KEYWORD, "DONE", TS_DONE,
				PT_KEYWORD, "ERROR", TS_ERROR,
			PT_char256, "error_msg", PADDR(errmsg), PT_ACCESS, PA_REFERENCE,
			PT_char256, "filename", PADDR(filename),
			PT_char1024, "schedule", PADDR(sched),
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		memset(this, 0, sizeof(schedule));
		default_value = 1.0;
		next_ts = TS_INIT;
	}
}

int schedule::create() 
{
	// copy the defaults
	memcpy(this,defaults,sizeof(schedule));
	return 1;
}

int schedule::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	int rv = 1;

	currval = default_value;
	if(filename[0] == 0){
		rv = parse_schedule();
	} else {
		rv = open_sched_file();
	}
	
	if(rv == 0){
		state = TS_ERROR;
		strcpy(errmsg, "Error reading & parsing schedule input source");
	}
	/* group rules together here */

	/* ...or not (yet). */

	if(state == TS_INIT){
		state = TS_OPEN;
	} else if(state == TS_ERROR){
		gl_error("unable to open schedule");
		state = TS_DONE;
	}

	return 1;
}

// check month of year
int test_sched_dt(int lo, int hi, int dt){
	if(lo != -1 && hi != -1){
		if(lo <= dt){
			if(hi == lo){
				if(dt == lo){
					return 1;
				}
			} else if(dt <= hi){
				return 1;
			}
		}
	} else if(lo == -1 && hi == -1){
		return 1;
	}
	return 0;
}
double get_sched_list_value(schedule_list *sched_list, TIMESTAMP t1){
	double res = 0.0;
	return res;
}

TIMESTAMP schedule::presync(TIMESTAMP t0, TIMESTAMP t1){
	/* get localtime & run rules */
	DATETIME dt;
	TIMESTAMP min = TS_NEVER; // earliest start of next state
	TIMESTAMP max = TS_NEVER; // earliest end of current state
	int res = 0;

	schedule_list *lptr = 0, *child = 0;

	//	skip if t0 < next_ts ... we should know when this needs to change

	gl_localtime(t1, &dt);

	currval = default_value;

	for(lptr = sched_list; lptr != NULL; lptr = lptr->next){
		// process lptr rule
		// traverse lptr children
		// track when current state ends OR when next state begins

		/* group_right & group_left should actually be used ... coming soon! */
		for(child = lptr; child != NULL; child = child->group_right){
			res = test_sched_dt(child->moy_start, child->moy_end, dt.month);
			if(res == 0){
				continue;
			}
			if(child->dow_or_dom == schedule_list::wor_skip){
				res = 1; /* short circuit, both are '*' */
			} else if(child->dow_or_dom == schedule_list::wor_month){
				res = test_sched_dt(child->dom_start, child->dom_end, dt.day);
			} else if(child->dow_or_dom == schedule_list::wor_week){
				res = test_sched_dt(child->dow_start, child->dow_end, dt.weekday);
			} else if(child->dow_or_dom == schedule_list::wor_both){
				res = test_sched_dt(child->dom_start, child->dom_end, dt.day) + test_sched_dt(child->dow_start, child->dow_end, dt.weekday);
			} else {
				gl_verbose("invalid day of week/day of month flag");
				continue;
			}
			if(res == 0){
				continue;
			}
			res = test_sched_dt(child->hod_start, child->hod_end, dt.hour);
			res += test_sched_dt(child->moh_start, child->moh_end, dt.minute > 59 ? 59 : dt.minute);
			if(res == 2){
				currval = child->scale;
				break; /* found rule in the group that matches, on to the next group */
			}
		}
	}

	return TS_NEVER;
}

TIMESTAMP schedule::sync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

/**
	Treats the schedule string as the authoratative schedule and
	parse it into a set of rules appropriately.
	@return 1 for success, 0 for failure
 */
int schedule::parse_schedule(){
	char sched_buf[1025];
	char *sched_ptr[128], *temp = 0;
	size_t i = 0, token_ct = 1;
	schedule_list *first = 0, *push = 0, *next = 0;

	strcpy(sched_buf, this->sched);

	/* simulate strtok effects, replacing ';' with '\0' and 
	 *  getting char* to each schedule token. */
	temp = strtok(sched_buf, ";");
	for(i = 0; i < 128 && temp != NULL; ++i){
		sched_ptr[i] = temp;
		temp = strtok(NULL, ";");
	}

	token_ct = i;

	/* parse each schedule token */
	for(i = 0; i < token_ct; ++i){
		push = next;
		next = parse_cron(sched_ptr[i]);
		if(next == 0){
			gl_error("Error parsing schedule string");
			return 0;
		}
		if(first == 0){
			first = next;
		} else {
			push->next = next;
		}
	}

	sched_list = first;
	return 1;
}

/**
	Opens and parses a schedule file, like the shaper.
	@return 1 for success, 0 for failure
 */
int schedule::open_sched_file(){
	gl_error("schedule file input not yet supported");
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

void new_schedule(MODULE *mod){
	new schedule(mod);
}

EXPORT int create_schedule(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(schedule::oclass);
	if (*obj!=NULL)
	{
		schedule *my = OBJECTDATA(*obj,schedule);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_schedule(OBJECT *obj)
{
	schedule *my = OBJECTDATA(obj,schedule);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP commit_schedule(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	//gl_error("We're in the commit function, yay!");
	return TS_NEVER;
}

EXPORT TIMESTAMP sync_schedule(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	schedule *my = OBJECTDATA(obj, schedule);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	
	try {
		switch (pass) 
		{
			case PC_PRETOPDOWN:
				t1 = my->presync(obj->clock, t0);
				break;

			case PC_BOTTOMUP:
				t1 = my->sync(obj->clock, t0);
				obj->clock = t0;
				break;

			default:
				gl_error("schedule::sync- invalid pass configuration");
				t1 = TS_INVALID; // serious error in exec.c
		}
	} 
	catch (char *msg)
	{
		gl_error("schedule::sync exception caught: %s", msg);
		t1 = TS_INVALID;
	}
	catch (...)
	{
		gl_error("schedule::sync exception caught: no info");
		t1 = TS_INVALID;
	}

	obj->clock = t0;
	return t1;
}

/**@}**/
