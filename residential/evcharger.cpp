/** $Id: evcharger.cpp,v 1.13 2008/02/13 02:22:35 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file evcharger.cpp
	@addtogroup evcharger
	@ingroup residential
	
	There are only two types of electric vehicles supported:

	- ELECTRIC	pure electric vehicle with limited range based on battery capacity
	- HYBRID	hybrid electric vehicle with longer range based on fuel capacity

	The evcharger simulation is based on demand state profile of the vehicle.
	When the vehicle is at home, it has a probability of leaving on one of 3
	trips.  

	\image html residential/docs/evcharger/Slide1.PNG "Figure 1 - EV charger state diagram"

	- WORK		trip to work (standard distance defined by trip.d_work)
	- SHORTTRIP	random trip up to 50 miles (battery will not be fully discharged)
	- LONGTRIP	random trip over 50 miles (battery will be discharged up to 25%)
				but only possible for HYBRID vehicles

	\image html residential/docs/evcharger/Slide2.PNG "Figure 2 - EV charger home/work arrival/departure statistics"

	When away, the probability of a return is used to determine when the vehicle
	returns. 

	The charger power can have one of three levels:

	- LOW		120V/15A charger
	- MEDIUM	240V/30A charger
	- HIGH		240V/60A charger

	The trip distances is used to estimate the battery charge upon return according
	to the following rules:

	1. A work trip discharges the battery depending on whether charge_at_work is defined.
	If charging at work is allowed, the battery will discharge for 1 trip, otherwise it will
	discharge for 2 trips.  

	2. A short trip discharges the battery based on the distance traveled.

	3. A long trip discharges the battery to 25%, but is only possible with hybrids.

	In all cases, if the trip distance is greater than 50 miles and the car is a
	hybrid, the discharge will be down to 25%.

	Heat fraction ratio is used to calculate the internal gain from plug loads.

	@par Demand profiles

	The format of the demand profile is as follows:

	\verbatim
	[DAYTYPE]
	DIRTRIP,DIRTRIP,...
	#.###,#.###,...
	#.###,#.###,...
	.
	.
	.
	#.###,#.###,...
	\endverbatim
	where \e DAYTYPE is either \p WEEKDAY or \p WEEKEND, 
	\e DIR is either \p ARR or \p DEP, and \e TRIP 
	is either \p HOME, \p WORK, \p SHRT, or \p LONG.  There must be 24 rows of numbers,
	the numbers must be positive numbers between 0 and 1, and each column must be normalized
	(they must add up to 1.000 over the 24 hour period). 

	You may introduce as many daytype blocks as are supported simultaneously (2 max at this time).

	@todo the 24 rows are not check for compliance with the 24 hour rule, the 0-1 rule, or the normalized rule.
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>

#include <gridlabd.h>

#include "house.h"
#include "evcharger.h"

/////////////////////////////////////////////////////////////////////
/// EV Demand Profiles
/////////////////////////////////////////////////////////////////////
typedef struct s_evdemandprofilelist {
	EVDEMAND *data;
	struct s_evdemandprofilelist *next;
} EVPROFILEITEM;

EVPROFILEITEM *first_demand_profile = NULL;

/// Find an EV demand profile
/// @returns pointer to the matching EVPROFILEITEM structure
EVDEMAND *find_demand_profile(char *name) ///< name of profile
{
	EVPROFILEITEM *item = first_demand_profile;
	while (item!=NULL)
	{
		if (strcmp(name,item->data->name)==0)
			break;
		else
			item = item->next;
	}
	return item?item->data:NULL;
}
/// Add an EV demand profile
/// @returns pointer to the new EVPROFILEITEM structure
EVPROFILEITEM *add_demand_profile(EVDEMAND *data)
{
	EVPROFILEITEM *item = new EVPROFILEITEM;
	if (item==NULL)
		throw "evcharter.cpp: add_demand_profile() memory allocation failed";
	item->data = data;
	item->next = first_demand_profile;
	return first_demand_profile = item;
}
EVDEMAND *load_demand_profile(char *filename)
{
	char *filepath = gl_findfile(filename,NULL,FF_READ);
	if (filepath==NULL)	
	{
		gl_error("searching for demand profile file '%s'",filename);
		throw "unable to find demand profile";
	}
	EVDEMAND *data = new EVDEMAND;
	if (data==NULL)
			throw "unable to allocate memory for new demand profile";
	memset(data,0,sizeof(EVDEMAND));

	int daytype = WEEKDAY;
	int linenum=0;
	int hour=0;

	strcpy(data->name,filepath);
	FILE *fp = fopen(filepath,"r");
	if (fp==NULL)
	{	
		gl_error("reading file %s line %d",filename,linenum);
		throw "unable to open demand profile";
	}
	char line[1024];
	double *hdr[8]; memset(hdr,0,sizeof(hdr));
	while (!feof(fp) && fgets(line,sizeof(line),fp)!=NULL)
	{
		linenum++;

		// look at first character
		if (isspace(line[0])) // blank line
			continue;
		if (line[0]=='#') // comment
			continue;
		if (line[0]=='[') // daytype start
		{
			char blockname[8];
			if (sscanf(line,"[%7s]",blockname)==1)
			{
				if (strcmp(blockname,"WEEKDAY")==0)
					daytype = WEEKDAY;
				else if (strcmp(blockname,"WEEKEND")==0)
					daytype = WEEKEND;
				else
				{
					gl_output("%s(%d): '%s' is an invalid daytype name",filename,linenum,blockname);
					throw "invalid block name";
				}
				data->n_daytypes++;
			}
			else
			{
				gl_output("%s(%d): daytype name syntax is not recognized",filename,linenum,blockname);
				throw "unable to read block name";
			}
		}
		else if (isalpha(line[0])) // header
		{
			char dir[4], trip[5];
			int event;
			int nhdr = 0;
			int offset=0;
			while (sscanf(line+offset,"%3s%4s",dir,trip)==2)
			{
				if (nhdr>sizeof(hdr)/sizeof(hdr[0]))
				{
					gl_output("%s(%d): too many heading items listed (max is %d)",filename,linenum,sizeof(hdr)/sizeof(hdr[0]));
					throw "too many heading items in demand profile";
				}
				if (strcmp(dir,"ARR")==0)
					event = ARRIVE;
				else if (strcmp(dir,"DEP")==0)
					event = DEPART;
				else
				{
					gl_output("%s(%d): '%s' is not an appropriate event direction (e.g., ARR, DEP)",filename,linenum,dir);
					throw "unrecognized event direction in demand profile";
				}
				if (strcmp(trip,"HOME")==0)
					hdr[nhdr++] = data->home[daytype][event];
				else if (strcmp(trip,"WORK")==0)
					hdr[nhdr++] = data->work[daytype][event];
				else if (strcmp(trip,"SHRT")==0)
					hdr[nhdr++] = data->shorttrip[daytype][event];
				else if (strcmp(trip,"LONG")==0)
					hdr[nhdr++] = data->longtrip[daytype][event];
				else
				{
					gl_output("%s(%d): '%s' is not an appropriate trip location (e.g., HOME, WORK)",filename,linenum,dir);
					throw "unrecognized trip location in demand profile";
				}
				offset += 8;
			}
		}
		else if (isdigit(line[0])) // data
		{
			double prob;
			int offset = 0;
			int col = 0;
			while (sscanf(line+offset,"%lf",&prob)==1)
			{
				hdr[col++][hour] = prob;
				while (line[offset]!='\0' && line[offset++]!=',') {}
			}
			hour++;
		}
		else
		{
		}
	}
	if (ferror(fp))
	{
		gl_error("%s(%d): %s",filename,linenum,strerror(errno));
		throw "unable to read demand profile";
	}
	fclose(fp);
	return data;
}
EVDEMAND *get_demand_profile(char *name)
{
	EVDEMAND *profile = find_demand_profile(name);
	if (profile==NULL)
	{
		profile = load_demand_profile(name);
		if (profile!=NULL)
			add_demand_profile(profile);
	}
	return profile;
}

//////////////////////////////////////////////////////////////////////////
// evcharger CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* evcharger::oclass = NULL;
evcharger *evcharger::defaults = NULL;

evcharger::evcharger(MODULE *module) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"evcharger",sizeof(evcharger),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_enumeration,"charger_type",PADDR(charger_type),
				PT_KEYWORD,"LOW",LOW,
				PT_KEYWORD,"MEDIUM",MEDIUM,
				PT_KEYWORD,"HIGH",HIGH,
			PT_enumeration,"vehicle_type",PADDR(vehicle_type),
				PT_KEYWORD,"ELECTRIC",ELECTRIC,
				PT_KEYWORD,"HYBRID",HYBRID,
			PT_enumeration,"state",PADDR(state),
				PT_KEYWORD,"HOME",HOME,
				PT_KEYWORD,"WORK",WORK,
				PT_KEYWORD,"SHORTTRIP",SHORTTRIP,
				PT_KEYWORD,"LONGTRIP",LONGTRIP,
			PT_double,"p_go_home[unit/h]",PADDR(demand.home),
			PT_double,"p_go_work[unit/h]",PADDR(demand.work),
			PT_double,"p_go_shorttrip[unit/h]",PADDR(demand.shorttrip),
			PT_double,"p_go_longtrip[unit/h]",PADDR(demand.longtrip),
			PT_double,"work_dist[mile]",PADDR(distance.work),
			PT_double,"shorttrip_dist[mile]",PADDR(distance.shorttrip),
			PT_double,"longtrip_dist[mile]",PADDR(distance.longtrip),
			PT_double,"capacity[kWh]",PADDR(capacity),
			PT_double,"charge[unit]",PADDR(charge),
			PT_bool,"charge_at_work",PADDR(charge_at_work),
			PT_double,"power_factor[unit]",PADDR(power_factor),
			PT_double,"charge_throttle[unit]", PADDR(charge_throttle),
			PT_char1024,"demand_profile", PADDR(demand_profile),

			// enduse load
			PT_complex,"enduse_load[kW]",PADDR(load.total),
			PT_complex,"constant_power[kW]",PADDR(load.power),
			PT_complex,"constant_current[A]",PADDR(load.current),
			PT_complex,"constant_admittance[1/Ohm]",PADDR(load.admittance),
			PT_double,"internal_gains[kW]",PADDR(load.heatgain),
			PT_double,"energy_meter[kWh]",PADDR(load.energy),
			PT_double,"heat_fraction[unit]",PADDR(heat_fraction),

			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		memset(this,0,sizeof(evcharger));
		load.power = load.admittance = load.current = load.total = complex(0,0,J);
	}
}

evcharger::~evcharger()
{
}

int evcharger::create() 
{
	memcpy(this,defaults,sizeof(evcharger));

	return 1;
	
}

// LOW, MEDIUM, HIGH settings
static double amps[] = {12,30,60};
static bool hiV[] = {false,true,true};

int evcharger::init(OBJECT *parent)
{
	static double sizes[] = {20,30,30,40,40,40,50,50,50,50,60,60,60,70,70,80};
	if (capacity==0) capacity = gl_random_sampled(sizeof(sizes)/sizeof(sizes[0]),sizes); 
	if (power_factor==0) power_factor = 0.95;
	if (charge==0) charge = gl_random_uniform(0.25,1.0);
	if (charge_throttle==0) charge_throttle = 1.0;
	if (mileage==0) mileage = gl_random_uniform(0.8,1.2);
	if (distance.work==0) distance.work = gl_random_lognormal(3,1);
	if (distance.shorttrip==0) distance.shorttrip = gl_random_lognormal(3,1);
	if (distance.longtrip==0) distance.longtrip = gl_random_lognormal(4,2);

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (parent==NULL || !gl_object_isa(parent,"house"))
		throw "evcharger must have a parent house";
	house *pHouse = OBJECTDATA(parent,house);

	// attach object to house panel
	pVoltage = (pHouse->attach(OBJECTHDR(this),amps[charger_type],hiV[charger_type]))->pV;

	// load demand profile
	if (strcmp(demand_profile,"")!=0)
		pDemand = get_demand_profile(demand_profile);

	update_state();

	return 1;
}

double evcharger::update_state(double dt) 
{
	OBJECT *obj = OBJECTHDR(this);
	if (obj->clock>0)
	{
		DATETIME now;
		if (!gl_localtime(obj->clock,&now))
			throw "unable to obtain current time";

		int hour = now.hour;
		int daytype = pDemand->n_daytypes>0 ? (now.weekday>0&&now.weekday<6) : 0;
		demand.home = pDemand->home[daytype][DEPART][hour];
		demand.work = pDemand->home[daytype][ARRIVE][hour];

		// implement any state change (arrival/departure)
		switch (state) {
		case HOME:
			if (gl_random_bernoulli(demand.home))
				state = WORK;
			break;
		case WORK:
			if (gl_random_bernoulli(demand.work))
			{
				state = HOME;
				if (charge_at_work)
					charge -= (distance.work / mileage)/capacity;
				else
					charge -= 2 * (distance.work / mileage)/capacity;
				if (charge<0.25 && vehicle_type==HYBRID)
					charge = 0.25;
				if (charge<=0)
					gl_warning("%s (%s:%d) vehicle has run out of charge while away from home", obj->name?obj->name:"anonymous",obj->oclass->name,obj->id);
			}
			break;
		case SHORTTRIP:
			// TODO - return home after short trip
			// charge -= distance.shorttrip / mileage / capacity;
			break;
		case LONGTRIP:
			// TODO - return home after long trip
			// charge = 0.25;
			break;
		default:
			throw "invalid state";
			break;
		}
	}

	// evaluate effect of current state on home
	switch (state) {
	case HOME:
		if (charge<1.0 && obj->clock>0)
		{
			// compute the charging power
			double charge_kw;
			switch (charger_type) {
			case LOW:
				charge_kw = 15 * pVoltage->Mag() * charge_throttle /1000;
				break;
			case MEDIUM:
				charge_kw = 30 * pVoltage->Mag() * charge_throttle /1000;
				break;
			case HIGH:
				charge_kw = 60 * pVoltage->Mag() * charge_throttle /1000;
				break;
			default:
				throw "invalid charger_type";
				break;
			}

			// compute the amount of energy added for this dt
			double d_charge_kWh = charge_kw*dt/3600;

			// compute the charge added to the battery
			charge += d_charge_kWh/capacity;
			if (charge>1.0)
				throw "charge state error (overcharge)"; // this shouldn't happen--it means that a dt calculate is wrong somewhere
			if (charge<0)
				throw "charge state error (undercharge)"; // this shouldn't happen either

			// compute the time it will take to finish charging to full capacity
			if (charge_kw>0)
			{
				dt = (capacity*(1-charge))/charge_kw*3600;
				if (dt<1)
				{
					charge = 1.0;
					dt = -1; // never;
				}
			}

			else
				dt = -1; //never

			load.power.SetPowerFactor(charge_kw,power_factor,J);
		}
		else
			load.power = complex(0,0,J);
		break;
	case WORK:
		load.power = complex(0,0,J);
		break;
	case SHORTTRIP:
		load.power = complex(0,0,J);
		break;
	case LONGTRIP:
		load.power = complex(0,0,J);
		break;
	default:
		throw "invalid state";
		break;
	}

	if (!isfinite(charge))
		throw "charge state error (not a number)"; // this is really bad
	load.total = load.power;
	load.heatgain = load.total.Mag() * heat_fraction;

	return dt<3600&&dt>=0?dt:3600;
}

TIMESTAMP evcharger::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	// compute the total load and heat gain
	if (t0>0 && t1>t0)
		load.energy += (load.total * gl_tohours(t1-t0)).Mag();
	double dt = update_state(gl_toseconds(t1-t0));

	return dt<0 ? TS_NEVER : (TIMESTAMP)(t1+dt*TS_SECOND); 
}

void evcharger::load_demand_profile(void)
{

}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_evcharger(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(evcharger::oclass);
	if (*obj!=NULL)
	{
		evcharger *my = OBJECTDATA(*obj,evcharger);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_evcharger(OBJECT *obj)
{
	try {
		evcharger *my = OBJECTDATA(obj,evcharger);
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		gl_error("%s (%s:%d): %s", obj->name?obj->name:"anonymous object",obj->oclass->name,obj->id,msg);
		return 0;
	}
}

EXPORT TIMESTAMP sync_evcharger(OBJECT *obj, TIMESTAMP t0)
{
	try {
		evcharger *my = OBJECTDATA(obj, evcharger);
		TIMESTAMP t1 = my->sync(obj->clock, t0);
		obj->clock = t0;
		return t1;
	}
	catch (char *msg)
	{
		gl_error("%s (%s:%d): %s", obj->name?obj->name:"anonymous object",obj->oclass->name,obj->id,msg);
		return TS_INVALID;
	}
}

/**@}**/
