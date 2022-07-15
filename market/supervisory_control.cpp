/** $Id: supervisory_control.cpp$
	Copyright (C) 2008 Battelle Memorial Institute
	@file supervisory_control.cpp
	@defgroup supervisory_control
	@ingroup market

	The supervisory_control object implements the supervisory control algorithm for frequency responsive loads. 

 **/

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include "gridlabd.h"
#include "supervisory_control.h"

CLASS *supervisory_control::oclass = NULL;
supervisory_control *supervisory_control::defaults = NULL;

static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_POSTTOPDOWN;

/* Class registration is only called once to register the class with the core */
supervisory_control::supervisory_control(MODULE *module) {
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"supervisory_control",sizeof(supervisory_control),passconfig|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class supervisory_control";
		else
			oclass->trl = TRL_QUALIFIED;

		if (gl_publish_variable(oclass,
			PT_char32, "unit", PADDR(unit), PT_DESCRIPTION, "unit of quantity",
			PT_double, "period[s]", PADDR(period), PT_DESCRIPTION, "interval of time between market clearings",
			PT_int32, "market_id", PADDR(market_id), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "unique identifier of market clearing",
			PT_double, "nominal_frequency[Hz]", PADDR(nom_freq), PT_DESCRIPTION, "nominal frequency",
			PT_double, "droop[%]", PADDR(droop), PT_DESCRIPTION, "droop value for the supervisor",
			PT_double, "frequency_deadband[Hz]", PADDR(frequency_deadband), PT_DESCRIPTION, "frequency deadband for assigning trigger frequencies",
			PT_enumeration,"PFC_mode",PADDR(PFC_mode),PT_DESCRIPTION,"operation mode of the primary frequency controller",
				PT_KEYWORD,"OVER_FREQUENCY",(enumeration)OVER_FREQUENCY,
				PT_KEYWORD,"UNDER_FREQUENCY",(enumeration)UNDER_FREQUENCY,
				PT_KEYWORD,"OVER_UNDER_FREQUENCY",(enumeration)OVER_UNDER_FREQUENCY,
			PT_enumeration,"bid_sort_mode",PADDR(sort_mode),PT_DESCRIPTION,"Determines how the bids into the market is sorted to contruct the PF curve",
				PT_KEYWORD,"NONE",(enumeration)SORT_NONE,
				PT_KEYWORD,"POWER_INCREASING",(enumeration)SORT_POWER_INCREASE,
				PT_KEYWORD,"POWER_DECREASING",(enumeration)SORT_POWER_DECREASE,
				PT_KEYWORD,"VOLTAGE_DEVIAION_FROM_NOMINAL",(enumeration)SORT_VOLTAGE,
				PT_KEYWORD,"VOLTAGE_EXTREMES",(enumeration)SORT_VOLTAGE2,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(supervisory_control));
	}
}

void supervisory_control::fetch_double(double **prop, char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_double_by_name(parent, name);
	if(*prop == nullptr){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "supervisory_control:%i", hdr->id);
		if(*name == static_cast<char>(0))
			sprintf(msg, "%s: supervisory_control unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: supervisory_control unable to find %s", namestr, name);
		throw(std::runtime_error(msg));
	}
}

/* Object creation is called once for each object that is created by the core */
int supervisory_control::create(void) {
	memcpy(this,defaults,sizeof(supervisory_control));
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int supervisory_control::init(OBJECT *parent) {
	OBJECT *obj=OBJECTHDR(this);
	market_id = 0;
	n_bids_on = 0;
	n_bids_off = 0;
	if(period == 0) period = 300;
	if(sort_mode == 0) sort_mode = SORT_NONE;
	if(frequency_deadband == 0) frequency_deadband = 0.015;
	clearat = nextclear();
	return 1; /* return 1 on success, 0 on failure */
}


int supervisory_control::isa(char *classname) {
	return strcmp(classname,"supervisory_control")==0;
}


/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP supervisory_control::presync(TIMESTAMP t0, TIMESTAMP t1) {
	DATETIME dt;
	
	if (t1>=clearat)
	{
		gl_verbose("I received #%d bids on and #%d bid off and we are at #%d market id\n",n_bids_on,n_bids_off,market_id);

		// sort the bids according to the sort method picked
		PFC_collect.sort(sort_mode);
		
		// calculate the new frequency thresholds for every unit
		PFC_collect.calculate_freq_thresholds(droop,nom_freq,frequency_deadband,PFC_mode);

		// get ready for next time we want to clear
		PFC_collect.clear();
		n_bids_on = 0;
		n_bids_off = 0;
		market_id++;
		clearat = nextclear();
		gl_localtime(clearat,&dt);
	}
	return (-clearat); /* soft return t2>t1 on success, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP supervisory_control::postsync(TIMESTAMP t0, TIMESTAMP t1) {
	return TS_NEVER;
}

int supervisory_control::submit(OBJECT *from, double power, double voltage_deviation, int key, int state) {
	gld_wlock lock(my());
	return submit_nolock(from,power,voltage_deviation,key,state);
}

int supervisory_control::submit_nolock(OBJECT *from, double power, double voltage_deviation, int key, int state) {
	char biddername[64];
	if (key > market_id){
		gl_error("bids from %s is for future market id", gl_name(from,biddername,sizeof(biddername)));
	}
	else if (key < market_id) {
		gl_error("bids from %s is for previous market id", gl_name(from,biddername,sizeof(biddername)));
	}
	else {
		gl_verbose("I got a bid from %s  ---> Power: (%.1f) PFC state: (%.1f) Key: (%d) State (%d)\n", gl_name(from,biddername,sizeof(biddername)),power,voltage_deviation,key,state);
		
		if (sort_mode == 3) { // Votlage sorint according to nominal
			voltage_deviation = fabs(voltage_deviation);
		}
		else if (sort_mode == 4 && state == 1) {
			voltage_deviation = voltage_deviation-1;
		}
		else if (sort_mode == 4 && state == 0) {
			voltage_deviation = 1-voltage_deviation;
		}


		if (state == 1) { //water heater is on
			n_bids_on = PFC_collect.submit_on(from,power,voltage_deviation,key,state);
		} 
		else if (state == 0) { // water heater is off
			n_bids_off = PFC_collect.submit_off(from,power,voltage_deviation,key,state);
		}
		else {
			gl_error("unknown state of the bid");
		}
	}
	return 0;
}

TIMESTAMP supervisory_control::nextclear(void) const {
	return gl_globalclock + (TIMESTAMP)(period - fmod((double)gl_globalclock, period));
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_supervisory_control(OBJECT **obj, OBJECT *parent) {
	try
	{
		*obj = gl_create_object(supervisory_control::oclass);
		if (*obj!=NULL)
		{
			supervisory_control *my = OBJECTDATA(*obj,supervisory_control);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(supervisory_control);
}

EXPORT int init_supervisory_control(OBJECT *obj, OBJECT *parent) {
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,supervisory_control)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(supervisory_control);
}

EXPORT int isa_supervisory_control(OBJECT *obj, char *classname) {
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,supervisory_control)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_supervisory_control(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass) {
	TIMESTAMP t2 = TS_NEVER;
	supervisory_control *my = OBJECTDATA(obj,supervisory_control);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			gl_error("invalid pass request (%d)", pass);
			t2 = TS_INVALID;
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
		return t2;
	}
	SYNC_CATCHALL(supervisory_control);
}

// End of supervisory_control.cpp
