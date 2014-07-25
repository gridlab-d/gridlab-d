/** $Id: evcharger.cpp 4738 2014-07-03 00:55:39Z dchassin $
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

#include "house_a.h"
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
		GL_THROW( "evcharter.cpp: add_demand_profile() memory allocation failed" );
	item->data = data;
	item->next = first_demand_profile;
	return first_demand_profile = item;
}
EVDEMAND *load_demand_profile(char *filename)
{
	char filepath[1024];
	if (gl_findfile(filename,NULL,R_OK,filepath,sizeof(filepath))==NULL)	
	{
		//gl_error("searching for demand profile file '%s'",filename);
		GL_THROW( "unable to find demand profile" );
	}
	EVDEMAND *data = new EVDEMAND;
	if (data==NULL)
			GL_THROW( "unable to allocate memory for new demand profile" );
	memset(data,0,sizeof(EVDEMAND));

	int daytype = WEEKDAY;
	int linenum=0;
	int hour=0;

	strcpy(data->name,filepath);
	FILE *fp = fopen(filepath,"r");
	if (fp==NULL)
	{	
		//gl_error("reading file %s line %d",filename,linenum);
		GL_THROW( "unable to read demand profile in '%s' at line %d", filename, linenum );
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
					GL_THROW("%s(%d): '%s' is an invalid daytype name",filename,linenum,blockname);
					//throw "invalid block name";
				}
				data->n_daytypes++;
			}
			else
			{
				GL_THROW("%s(%d): daytype name syntax is not recognized",filename,linenum,blockname);
				//throw "unable to read block name";
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
					GL_THROW( "too many heading items in demand profile" );
				}
				if (strcmp(dir,"ARR")==0)
					event = ARRIVE;
				else if (strcmp(dir,"DEP")==0)
					event = DEPART;
				else
				{
					GL_THROW("%s(%d): '%s' is not an appropriate event direction (e.g., ARR, DEP)",filename,linenum,dir);
					//throw "unrecognized event direction in demand profile";
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
					GL_THROW("%s(%d): '%s' is not an appropriate trip location (e.g., HOME, WORK)",filename,linenum,dir);
					//throw "unrecognized trip location in demand profile";
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
		GL_THROW( "unable to read demand profile" );
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
CLASS* evcharger::pclass = NULL;
evcharger *evcharger::defaults = NULL;

evcharger::evcharger(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"evcharger",sizeof(evcharger),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class evcharger";
		else
			oclass->trl = TRL_DEMONSTRATED;


		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_enumeration,"charger_type",PADDR(charger_type),
				PT_KEYWORD,"LOW",(enumeration)CT_LOW,
				PT_KEYWORD,"MEDIUM",(enumeration)CT_MEDIUM,
				PT_KEYWORD,"HIGH",(enumeration)CT_HIGH,
			PT_enumeration,"vehicle_type",PADDR(vehicle_type),
				PT_KEYWORD,"ELECTRIC",(enumeration)VT_ELECTRIC,
				PT_KEYWORD,"HYBRID",(enumeration)VT_HYBRID,
			PT_enumeration,"state",PADDR(vehicle_state),
				PT_KEYWORD,"UNKNOWN",(enumeration)VS_UNKNOWN,
				PT_KEYWORD,"HOME",(enumeration)VS_HOME,
				PT_KEYWORD,"WORK",(enumeration)VS_WORK,
			// these are not yet supported
			//	PT_KEYWORD,"SHORTTRIP",SHORTTRIP,
			//	PT_KEYWORD,"LONGTRIP",LONGTRIP,
			PT_double,"p_go_home[unit/h]",PADDR(demand.home),
			PT_double,"p_go_work[unit/h]",PADDR(demand.work),
			// these are not yet supported
			//PT_double,"p_go_shorttrip[unit/h]",PADDR(demand.shorttrip),
			//PT_double,"p_go_longtrip[unit/h]",PADDR(demand.longtrip),
			PT_double,"work_dist[mile]",PADDR(distance.work),
			// these are not yet supported
			//PT_double,"shorttrip_dist[mile]",PADDR(distance.shorttrip),
			//PT_double,"longtrip_dist[mile]",PADDR(distance.longtrip),
			PT_double,"capacity[kWh]",PADDR(capacity),
			PT_double,"charge[unit]",PADDR(charge),
			PT_bool,"charge_at_work",PADDR(charge_at_work),
			PT_double,"charge_throttle[unit]", PADDR(charge_throttle),
			PT_double,"charger_efficiency[unit]", PADDR(charging_efficiency),PT_DESCRIPTION, "Efficiency of the charger in terms of energy in to battery stored",
			PT_double,"power_train_efficiency[mile/kWh]", PADDR(mileage), PT_DESCRIPTION, "Miles per kWh of battery charge",
			PT_double,"mileage_classification[mile]", PADDR(mileage_classification), PT_DESCRIPTION, "Miles expected range on battery only",
			PT_char1024,"demand_profile", PADDR(demand_profile),
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

evcharger::~evcharger()
{
}

int evcharger::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;
	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	vehicle_type = VT_HYBRID;

	charging_efficiency = 1.0;	//Assumed 100% Efficient charging at first
	mileage_classification = 0.0;
	mileage = 0.0;

	return res;
}

// LOW, MEDIUM, HIGH settings
static double fuse[] = {15,35,70};
static double amps[] = {12,28,55};
static bool hiV[] = {false,true,true};

int evcharger::init(OBJECT *parent)
{
	int retval;

	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("evcharger::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	static double sizes[] = {20,30,30,40,40,40,50,50,50,50,60,60,60,70,70,80};

	if (mileage==0) mileage = gl_random_uniform(RNGSTATE,0.8,1.2);
	
	//See if mileage classification and mileage are defined
	if ((mileage_classification > 0.0) && (mileage > 0.0))
	{
		capacity=mileage_classification/mileage;	//capacity = miles expected * kWh/mile
	}
	
	if (capacity==0) capacity = gl_random_sampled(RNGSTATE,sizeof(sizes)/sizeof(sizes[0]),sizes); 
	if (power_factor==0) power_factor = 0.95;
	if (charge==0) charge = gl_random_uniform(RNGSTATE,0.25,1.0);
	if (charge_throttle==0) charge_throttle = 1.0;
	if (mileage==0) mileage = gl_random_uniform(RNGSTATE,0.8,1.2);
	if (distance.work==0) distance.work = gl_random_lognormal(RNGSTATE,3,1);
	// these are not yet supported
	//if (distance.shorttrip==0) distance.shorttrip = gl_random_lognormal(3,1);
	//if (distance.longtrip==0) distance.longtrip = gl_random_lognormal(4,2);

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	//Make sure the efficiency is valid
	if ((charging_efficiency > 1.0) || (charging_efficiency < 0.0))
	{
		GL_THROW("Please specify a charging efficiency between 0.0 and 1.0!");
		/*  TROUBLESHOOT
		A charger_efficiency value outside of the possible values was specified.  Please try again with a proper
		value.
		*/
	}

	// load demand profile
	if (strcmp(demand_profile,"")!=0)
		pDemand = get_demand_profile(demand_profile);
	
	retval = residential_enduse::init(parent);

	update_state();

	return retval;
}

int evcharger::isa(char *classname)
{
	return (strcmp(classname,"evcharger")==0 || residential_enduse::isa(classname));
}

double evcharger::update_state(double dt /* seconds */) 
{
	OBJECT *obj = OBJECTHDR(this);
	if (obj->clock>TS_ZERO && dt>0)
	{
		DATETIME now;
		if (!gl_localtime(obj->clock,&now))
			GL_THROW( "unable to obtain current time" );
		/*	TROUBLESHOOT
			The clock likely was not properly initialized, or the object clock has been overwritten with
			garbage.  Please verify that the clock is set by the input model.
		*/

		int hour = now.hour;
		int daytype = 0;
		if(pDemand != NULL){
			pDemand->n_daytypes>0 ? (now.weekday>0&&now.weekday<6) : 0;

			demand.home = pDemand->home[daytype][DEPART][hour];
			demand.work = pDemand->home[daytype][ARRIVE][hour];
		}

		// implement any state change (arrival/departure)
		switch (vehicle_state) {
		case VS_HOME:
			if (gl_random_bernoulli(RNGSTATE,demand.home*dt/3600))
			{
				gl_debug("%s (%s:%d) leaves for work with %.0f%% charge",obj->name?obj->name:"anonymous",obj->oclass->name,obj->id,charge*100);
				vehicle_state = VS_WORK;
			}
			break;
		case VS_WORK:
			if (gl_random_bernoulli(RNGSTATE,demand.work*dt/3600))
			{
				vehicle_state = VS_HOME;
				if (charge_at_work)
					charge -= (distance.work / mileage)/capacity;
				else
					charge -= 2 * (distance.work / mileage)/capacity;
				if (charge<0.25 && vehicle_type==VT_HYBRID)
					charge = 0.25;
				if (charge<=0)
					gl_warning("%s (%s:%d) vehicle has run out of charge while away from home", obj->name?obj->name:"anonymous",obj->oclass->name,obj->id);
				else
					gl_debug("%s (%s:%d) returns from work with %.0f%% charge",obj->name?obj->name:"anonymous",obj->oclass->name,obj->id, charge*100);
			}
			break;
		// these are not yet supported
		//case SHORTTRIP:
		// TODO - return home after short trip
		//	charge -= distance.shorttrip / mileage / capacity;
		//	break;
		//case LONGTRIP:
		// TODO - return home after long trip
		//	charge = 0.25;
		//	break;
		default:
			GL_THROW( "invalid state" );
			/*	TROUBLESHOOT
				The evcharger vehicle has found itself in an ambiguous state while calculating its state
				and level of charge.  Please verify that the object was initialized with "state" as either
				"HOME" or "WORK".  If it is, please fill out a bug report and include the relevant sections
				of the evcharger objects and class blocks from your input model.
			*/
			break;
		}
	}

	// evaluate effect of current state on home
	switch (vehicle_state) {
	case VS_HOME:
		if (charge<1.0 && obj->clock>0)
		{
			// compute the charging power
			double charge_kw;
			switch (charger_type) {
			case CT_LOW:
			case CT_MEDIUM:
			case CT_HIGH:
				charge_kw = amps[(int)charger_type] * pCircuit->pV->Mag() * charge_throttle /1000;
				break;
			default:
				GL_THROW( "invalid charger_type" );
				/*	TROUBLESHOOT
					The vehicle charger is not in a valid state.  It may not have been declared properly in
					the model file.  Please verify that the "charger_type" property is either "LOW", "MEDIUM",
					or "HIGH" in the model file.  If it is defined properly, please fill out a bug report and
					include the relevant sections with the evcharger objects and class blocks from your
					input model.
				*/
				break;
			}

			// compute the amount of energy added for this dt
			double d_charge_kWh = charge_kw*dt/3600*charging_efficiency;

			// compute the charge added to the battery
			charge += d_charge_kWh/capacity;
			if (charge>1.0)
			{
				gl_warning("%s (%s:%d) overcharge by %.0f%% truncated",obj->name?obj->name:"anonymous",obj->oclass->name,obj->id, (charge-1)*100);
				// TODO: the balance of charge should be dumped as heat into the garage
				charge = 1.0;
			}
			else if (charge<0.0)
			{
				gl_warning("%s (%s:%d) 100%% discharged", obj->name?obj->name:"anonymous",obj->oclass->name,obj->id);
				// TODO: the balance of charge should be dumped as heat into the garage
				charge = 0.0;
			}
			else if (charge>0.999)
			{
				gl_debug("%s (%s:%d) charge complete",obj->name?obj->name:"anonymous",obj->oclass->name,obj->id);
				charge = 1.0;
			}


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
				dt = -1; // never

			load.power.SetPowerFactor(charge_kw,power_factor,J);
		}
		else
		{
			load.power = complex(0,0,J);
			dt = -1; // never
		}
		break;
	case VS_WORK:
		load.power = complex(0,0,J);
		dt = -1; // never
		break;
	// these are not yet supported
	//case SHORTTRIP:
	//	load.power = complex(0,0,J);
	//	break;
	//case LONGTRIP:
	//	load.power = complex(0,0,J);
	//	break;
	default:
		GL_THROW( "invalid state" );
		/*	TROUBLESHOOT
			The evcharger vehicle has found itself in an ambiguous state while calculating its
			effects on its parent house.  Please verify that the object was initialized with 
			"state" as either "HOME" or "WORK".  If it is, please fill out a bug report and 
			include the relevant sections of the evcharger objects and class blocks from your
			input model.
		*/
		break;
	}

	if (!isfinite(charge))
		GL_THROW( "charge state error (not a number)" ); // this is really bad
	/*	TROUBLESHOOT
		The charge value is no longer a finite value.  Please submit a bug report with the
		entire offending model file, or create a simplified model using subsections of the
		offending model to isolate or eradicate the error.
	*/
	load.total = load.power;
	load.heatgain = load.total.Mag() * heat_fraction;
	

	return dt<3600&&dt>=0?dt:3600;
}

TIMESTAMP evcharger::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	// compute the total load and heat gain
	if (t0>TS_ZERO && t1>t0)
			load.energy += (load.total * gl_tohours(t1-t0));
	double dt = update_state(gl_toseconds(t1-t0));
	if (dt==0)
		gl_warning("%s (%s:%d) didn't advance the clock",obj->name?obj->name:"anonymous",obj->oclass->name,obj->id);


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
	try
	{
		*obj = gl_create_object(evcharger::oclass);
		if (*obj!=NULL)
		{
			evcharger *my = OBJECTDATA(*obj,evcharger);;
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(evcharger);
}

EXPORT int init_evcharger(OBJECT *obj)
{
	try {
		evcharger *my = OBJECTDATA(obj,evcharger);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(evcharger);
}

EXPORT int isa_evcharger(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,evcharger)->isa(classname);
	} else {
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
	SYNC_CATCHALL(evcharger);
}

/**@}**/
