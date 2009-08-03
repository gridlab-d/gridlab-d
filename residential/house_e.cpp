/** $Id: house_e.cpp,v 1.71 2008/02/13 02:22:35 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file house_e.cpp
	@addtogroup house_e
	@ingroup residential

	The house_e object implements a single family home.  The house_e
	only includes the heating/cooling system and the power panel.
	All other end-uses must be explicitly defined and attached to the
	panel using the house_e::attach() method.

	Residential panels use a split secondary transformer:

	@verbatim
		-----)||(------------------ 1    <-- 120V
		     )||(      120V         ^
		1puV )||(----------- 3(N)  240V  <-- 0V
		     )||(      120V         v
		-----)||(------------------ 2    <-- 120V
	@endverbatim

	120V objects are assigned alternatively to circuits 1-3 and 2-3 in the order
	in which they call attach. 240V objects are assigned to circuit 1-2

	Circuit breakers will open on over-current with respect to the maximum current
	given by load when house_e::attach() was called.  After a breaker opens, it is
	reclosed within an average of 5 minutes (on an exponential distribution).  Each
	time the breaker is reclosed, the breaker failure probability is increased.
	The probability of failure is always 1/N where N is initially a large number (e.g., 100). 
	N is progressively decremented until it reaches 1 and the probability of failure is 100%.

	The Equivalent Thermal Parameter (ETP) approach is used to model the residential loads
	and energy consumption.  Solving the ETP model simultaneously for T_{air} and T_{mass},
	the heating/cooling loads can be obtained as a function of time.

	In the current implementation, the HVAC equipment is defined as part of the house_e and
	attached to the electrical panel with a 50 amp/220-240V circuit.

	@par Implicit enduses

	The use of implicit enduses is controlled globally by the #implicit_enduse global variable.
	All houses in the system will employ the same set of global enduses, meaning that the
	loadshape is controlled by the default schedule.

	@par Credits

	The original concept for ETP was developed by Rob Pratt and Todd Taylor around 1990.  
	The first derivation and implementation of the solution was done by Ross Guttromson 
	and David Chassin for PDSS in 2004.

	@par Billing system

	Contract terms are defined according to which contract type is being used.  For
	subsidized and fixed price contracts, the terms are defined using the following format:
	\code
	period=[YQMWDH];fee=[$/period];energy=[c/kWh];
	\endcode

	Time-use contract terms are defined using
	\code
	period=[YQMWDH];fee=[$/period];offpeak=[c/kWh];onpeak=[c/kWh];hours=[int24mask];
	\endcode

	When TOU includes critical peak pricing, use
	\code
	period=[YQMWDH];fee=[$/period];offpeak=[c/kWh];onpeak=[c/kWh];hours=[int24mask]>;critical=[$/kWh];
	\endcode

	RTP is defined using
	\code
	period=[YQMWDH];fee=[$/period];bid_fee=[$/bid];market=[name];
	\endcode

	Demand pricing uses
	\code
	period=[YQMWDH];fee=[$/period];energy=[c/kWh];demand_limit=[kW];demand_price=[$/kW];
	\endcode
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "solvers.h"
#include "house_e.h"
#include "lock.h"
#include "complex.h"

#ifndef WIN32
char *strlwr(char *s)
{
	char *r=s;
	while (*s!='\0') 
	{
		*s = (*s>='A'&&*s<='Z') ? (*s-'A'+'a') : *s;
		s++;
	}
	return r;
}
#endif

// list of enduses that are implicitly active
set house_e::implicit_enduses_active = 0xffffffff;

//////////////////////////////////////////////////////////////////////////
// implicit loadshapes - these are enabled by using implicit_enduses global
//////////////////////////////////////////////////////////////////////////
struct s_implicit_enduse_list {
	char *implicit_name;
	struct {
		double breaker_amps; 
		int circuit_is220;
		struct {
			double z, i, p;
		} fractions;
		double power_factor;
		double heat_fraction;
	} load;
	char *shape;
	char *schedule_name;
	char *schedule_definition;
} implicit_enduse_data[] =
{
	// lighting (source: ELCAP lit-sp.dat)
	{	"LIGHTS", 
		{30, false, {0,0,1}, 0.97, 0.9},
		"type:analog; schedule: residential-lights-default; power: 1.0 kW",
		"residential-lights-default", 
		"weekday-summer {"
		"	*  0 * 4-9 1-5 0.380; *  1 * 4-9 1-5 0.340; *  2 * 4-9 1-5 0.320; *  3 * 4-9 1-5 0.320"
		"	*  4 * 4-9 1-5 0.320; *  5 * 4-9 1-5 0.350; *  6 * 4-9 1-5 0.410; *  7 * 4-9 1-5 0.450"
		"	*  8 * 4-9 1-5 0.450; *  9 * 4-9 1-5 0.450; * 10 * 4-9 1-5 0.450; * 11 * 4-9 1-5 0.450"
		"	* 12 * 4-9 1-5 0.450; * 13 * 4-9 1-5 0.440; * 14 * 4-9 1-5 0.440; * 15 * 4-9 1-5 0.450"
		"	* 16 * 4-9 1-5 0.470; * 17 * 4-9 1-5 0.510; * 18 * 4-9 1-5 0.540; * 19 * 4-9 1-5 0.560"
		"	* 20 * 4-9 1-5 0.630; * 21 * 4-9 1-5 0.710; * 22 * 4-9 1-5 0.650; * 23 * 4-9 1-5 0.490"
		"}"
		"weekend-summer {"
		"	*  0 * 4-9 6-0 0.410; *  1 * 4-9 6-0 0.360; *  2 * 4-9 6-0 0.330; *  3 * 4-9 6-0 0.320"
		"	*  4 * 4-9 6-0 0.320; *  5 * 4-9 6-0 0.320; *  6 * 4-9 6-0 0.340; *  7 * 4-9 6-0 0.390"
		"	*  8 * 4-9 6-0 0.440; *  9 * 4-9 6-0 0.470; * 10 * 4-9 6-0 0.470; * 11 * 4-9 6-0 0.470"
		"	* 12 * 4-9 6-0 0.470; * 13 * 4-9 6-0 0.460; * 14 * 4-9 6-0 0.460; * 15 * 4-9 6-0 0.460"
		"	* 16 * 4-9 6-0 0.470; * 17 * 4-9 6-0 0.490; * 18 * 4-9 6-0 0.520; * 19 * 4-9 6-0 0.540"
		"	* 20 * 4-9 6-0 0.610; * 21 * 4-9 6-0 0.680; * 22 * 4-9 6-0 0.630; * 23 * 4-9 6-0 0.500"
		"}"
		"weekday-winter {"
		"	*  0 * 10-3 1-5 0.4200; *  1 * 10-3 1-5 0.3800; *  2 * 10-3 1-5 0.3700; *  3 * 10-3 1-5 0.3600"
		"	*  4 * 10-3 1-5 0.3700; *  5 * 10-3 1-5 0.4200; *  6 * 10-3 1-5 0.5800; *  7 * 10-3 1-5 0.6900"
		"	*  8 * 10-3 1-5 0.6100; *  9 * 10-3 1-5 0.5600; * 10 * 10-3 1-5 0.5300; * 11 * 10-3 1-5 0.5100"
		"	* 12 * 10-3 1-5 0.4900; * 13 * 10-3 1-5 0.4700; * 14 * 10-3 1-5 0.4700; * 15 * 10-3 1-5 0.5100"
		"	* 16 * 10-3 1-5 0.6300; * 17 * 10-3 1-5 0.8400; * 18 * 10-3 1-5 0.9700; * 19 * 10-3 1-5 0.9800"
		"	* 20 * 10-3 1-5 0.9600; * 21 * 10-3 1-5 0.8900; * 22 * 10-3 1-5 0.7400; * 23 * 10-3 1-5 0.5500"
		"}"
		"weekend-winter {"
		"	*  0 * 10-3 6-0 0.4900; *  1 * 10-3 6-0 0.4200; *  2 * 10-3 6-0 0.3800; *  3 * 10-3 6-0 0.3800"
		"	*  4 * 10-3 6-0 0.3700; *  5 * 10-3 6-0 0.3800; *  6 * 10-3 6-0 0.4300; *  7 * 10-3 6-0 0.5100"
		"	*  8 * 10-3 6-0 0.6000; *  9 * 10-3 6-0 0.6300; * 10 * 10-3 6-0 0.6300; * 11 * 10-3 6-0 0.6100"
		"	* 12 * 10-3 6-0 0.6000; * 13 * 10-3 6-0 0.5900; * 14 * 10-3 6-0 0.5900; * 15 * 10-3 6-0 0.6100"
		"	* 16 * 10-3 6-0 0.7100; * 17 * 10-3 6-0 0.8800; * 18 * 10-3 6-0 0.9600; * 19 * 10-3 6-0 0.9700"
		"	* 20 * 10-3 6-0 0.9400; * 21 * 10-3 6-0 0.8800; * 22 * 10-3 6-0 0.7600; * 23 * 10-3 6-0 0.5800"
		"}"
		}
	/// @todo add other implicit enduse schedules and shapes as they are defined
};

EXPORT CIRCUIT *attach_enduse(OBJECT *obj, enduse *target, double breaker_amps, int is220)
{
	house_e *pHouse = 0;
	CIRCUIT *c = 0;

	if(obj == NULL){
		GL_THROW("attach_house_a: null object reference");
	}
	if(target == NULL){
		GL_THROW("attach_house_a: null enduse target data");
	}
	if(breaker_amps < 0 || breaker_amps > 1000){ /* at 3kA, we're looking into substation power levels, not enduses */
		GL_THROW("attach_house_a: breaker amps of %i unrealistic", breaker_amps);
	}

	pHouse = OBJECTDATA(obj,house_e);
	return pHouse->attach(obj,breaker_amps,is220,target);
}

//////////////////////////////////////////////////////////////////////////
// house_e CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* house_e::oclass = NULL;

double house_e::warn_low_temp = 55;
double house_e::warn_high_temp = 95;
bool house_e::warn_control = true;

/** House object constructor:  Registers the class and publishes the variables that can be set by the user. 
Sets default randomized values for published variables.
**/
house_e::house_e(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)  
	{
		// register the class definition
		oclass = gl_register_class(mod,"house_e",sizeof(house_e),PC_PRETOPDOWN|PC_BOTTOMUP);

		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_object,"weather",PADDR(weather),PT_DESCRIPTION,"reference to the climate object",
			PT_double,"floor_area[sf]",PADDR(floor_area),PT_DESCRIPTION,"home conditioned floor area",
			PT_double,"gross_wall_area[sf]",PADDR(gross_wall_area),PT_DESCRIPTION,"gross outdoor wall area",
			PT_double,"ceiling_height[ft]",PADDR(ceiling_height),PT_DESCRIPTION,"average ceiling height",
			PT_double,"aspect_ratio",PADDR(aspect_ratio), PT_DESCRIPTION,"aspect ratio of the home's footprint",
			PT_double,"envelope_UA[Btu/degF.h]",PADDR(envelope_UA),PT_DESCRIPTION,"overall UA of the home's envelope",
			PT_double,"window_wall_ratio",PADDR(window_wall_ratio),PT_DESCRIPTION,"ratio of window area to wall area",
			PT_double,"door_wall_ratio",PADDR(door_wall_ratio),PT_DESCRIPTION,"ratio of door area to wall area",
			PT_double,"glazing_shgc",PADDR(glazing_shgc),PT_DESCRIPTION,"shading coefficient of glazing",PT_DEPRECATED,
			PT_double,"window_shading",PADDR(glazing_shgc),PT_DESCRIPTION,"shading coefficient of windows",
			PT_double,"airchange_per_hour",PADDR(airchange_per_hour),PT_DESCRIPTION,"number of air-changes per hour",
			PT_double,"internal_gain[Btu/h]",PADDR(total.heatgain),PT_DESCRIPTION,"internal heat gains",
			PT_double,"solar_gain[Btu/h]",PADDR(solar_load),PT_DESCRIPTION,"solar heat gains",
			PT_double,"heat_cool_gain[Btu/h]",PADDR(system.heatgain),PT_DESCRIPTION,"system heat gains(losses)",

			PT_double,"thermostat_deadband[degF]",PADDR(thermostat_deadband),PT_DESCRIPTION,"deadband of thermostat control",
			PT_double,"heating_setpoint[degF]",PADDR(heating_setpoint),PT_DESCRIPTION,"thermostat heating setpoint",
			PT_double,"cooling_setpoint[degF]",PADDR(cooling_setpoint),PT_DESCRIPTION,"thermostat cooling setpoint",
			PT_double, "design_heating_capacity[Btu.h/sf]",PADDR(design_heating_capacity),PT_DESCRIPTION,"system heating capacity",
			PT_double,"design_cooling_capacity[Btu.h/sf]",PADDR(design_cooling_capacity),PT_DESCRIPTION,"system cooling capacity",
			PT_double, "cooling_design_temperature[degF]", PADDR(cooling_design_temperature),PT_DESCRIPTION,"system cooling design temperature",
			PT_double, "heating_design_temperature[degF]", PADDR(heating_design_temperature),PT_DESCRIPTION,"system heating design temperature",
			PT_double, "design_peak_solar[W/sf]", PADDR(design_peak_solar),PT_DESCRIPTION,"system design solar load",
			PT_double, "design_internal_gains[W/sf]", PADDR(design_peak_solar),PT_DESCRIPTION,"system design internal gains",

			PT_double,"heating_COP[pu]",PADDR(heating_COP),PT_DESCRIPTION,"system heating performance coefficient",
			PT_double,"cooling_COP[Btu/kWh]",PADDR(cooling_COP),PT_DESCRIPTION,"system cooling performance coefficient",
			PT_double,"COP_coeff",PADDR(COP_coeff),PT_DESCRIPTION,"effective system performance coefficient",
			PT_double,"air_temperature[degF]",PADDR(Tair),PT_DESCRIPTION,"indoor air temperature",
			PT_double,"outdoor_temperature[degF]",PADDR(outside_temperature),PT_DESCRIPTION,"outdoor air temperature",
			PT_double,"mass_heat_capacity[Btu/F]",PADDR(house_content_thermal_mass),PT_DESCRIPTION,"interior mass heat capacity",
			PT_double,"mass_heat_coeff[Btu/F.h]",PADDR(house_content_heat_transfer_coeff),PT_DESCRIPTION,"interior mass heat exchange coefficient",
			PT_double,"mass_temperature[degF]",PADDR(Tmaterials),PT_DESCRIPTION,"interior mass temperature",
			PT_set,"system_type",PADDR(system_type),PT_DESCRIPTION,"heating/cooling system type/options",
				PT_KEYWORD, "GAS",	(set)ST_GAS,
				PT_KEYWORD, "AIRCONDITIONING", (set)ST_AC,
				PT_KEYWORD, "FORCEDAIR", (set)ST_AIR,
				PT_KEYWORD, "TWOSTAGE", (set)ST_VAR,
			PT_enumeration,"system_mode",PADDR(system_mode),PT_DESCRIPTION,"heating/cooling system operation state",
				PT_KEYWORD,"UNKNOWN",SM_UNKNOWN,
				PT_KEYWORD,"HEAT",SM_HEAT,
				PT_KEYWORD,"OFF",SM_OFF,
				PT_KEYWORD,"COOL",SM_COOL,
				PT_KEYWORD,"AUX",SM_AUX,
			PT_double, "Rroof[degF.h/Btu]", PADDR(Rroof),PT_DESCRIPTION,"roof R-value",
			PT_double, "Rwall[degF.h/Btu]", PADDR(Rwall),PT_DESCRIPTION,"wall R-value",
			PT_double, "Rfloor[degF.h/Btu]", PADDR(Rfloor),PT_DESCRIPTION,"floor R-value",
			PT_double, "Rwindows[degF.h/Btu]", PADDR(Rwindows),PT_DESCRIPTION,"window R-value",
			PT_double, "Rdoors[degF.h/Btu]", PADDR(Rdoors),PT_DESCRIPTION,"door R-value",
			
			PT_enduse,"system",PADDR(system),PT_DESCRIPTION,"heating/cooling system enduse load",
			PT_enduse,"total",PADDR(total),PT_DESCRIPTION,"total enduse load",
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);			

		gl_publish_function(oclass,	"attach_enduse", (FUNCTIONADDR)attach_enduse);
		gl_global_create("residential::implicit_enduses",PT_set,&implicit_enduses_active,
			PT_KEYWORD, "LIGHTS", (set)IEU_LIGHTS,
			PT_KEYWORD, "PLUGS", (set)IEU_PLUGS,
			PT_KEYWORD, "OCCUPANCY", (set)IEU_OCCUPANCY,
			PT_KEYWORD, "DISHWASHER", (set)IEU_DISHWASHER,
			PT_KEYWORD, "MICROWAVE", (set)IEU_MICROWAVE,
			PT_KEYWORD, "FREEZER", (set)IEU_FREEZER,
			PT_KEYWORD, "REFRIGERATOR", (set)IEU_REFRIGERATOR,
			PT_KEYWORD, "RANGE", (set)IEU_RANGE,
			PT_KEYWORD, "EVCHARGER", (set)IEU_EVCHARGE,
			PT_KEYWORD, "WATERHEATER", (set)IEU_WATERHEATER,
			PT_KEYWORD, "CLOTHESWASHER", (set)IEU_CLOTHESWASHER,
			PT_KEYWORD, "DRYER", (set)IEU_DRYER,
			PT_KEYWORD, "NONE", (set)0,
			PT_DESCRIPTION, "list of implicit enduses that are active in houses",
			NULL);
		gl_global_create("residential::house_low_temperature_warning[F]",PT_double,&warn_low_temp,
			PT_DESCRIPTION, "the low house indoor temperature at which a warning will be generated",
			NULL);
		gl_global_create("residential::house_high_temperature_warning[F]",PT_double,&warn_high_temp,
			PT_DESCRIPTION, "the high house indoor temperature at which a warning will be generated",
			NULL);
		gl_global_create("residential::thermostat_control_warning",PT_double,&warn_control,
			PT_DESCRIPTION, "boolean to indicate whether a warning is generated when indoor temperature is out of control limits",
			NULL);

		// set up implicit enduse list
		implicit_enduse_list = NULL;
	}	
}

int house_e::create() 
{
	char1024 active_enduses;
	gl_global_getvar("residential::implicit_enduses",active_enduses,sizeof(active_enduses));
	char *token = NULL;

	if (strcmp(active_enduses,"NONE")!=0)
	{
		// scan the implicit_enduse list
		while ((token=strtok(token?NULL:active_enduses,"|"))!=NULL)
		{
			strlwr(token);
			
			// find the implicit enduse description
			struct s_implicit_enduse_list *eu = NULL;
			int found=0;
			for (eu=implicit_enduse_data; eu<implicit_enduse_data+sizeof(implicit_enduse_data)/sizeof(implicit_enduse_data[0]); eu++)
			{
				char name[64];
				sprintf(name,"residential-%s-default",token);
				// matched enduse
				if (strcmp(eu->schedule_name,name)==0)
				{
					SCHEDULE *sched = gl_schedule_create(eu->schedule_name,eu->schedule_definition);
					IMPLICITENDUSE *item = (IMPLICITENDUSE*)gl_malloc(sizeof(IMPLICITENDUSE));
					memset(item,0,sizeof(IMPLICITENDUSE));
					gl_enduse_create(&(item->load));
					item->load.shape = gl_loadshape_create(sched);
					if (gl_set_value_by_type(PT_loadshape,item->load.shape,eu->shape)==0)
					{
						gl_error("loadshape '%s' could not be created", name);
						return 0;
					}
					item->load.name = eu->implicit_name;
					item->next = implicit_enduse_list;
					item->amps = eu->load.breaker_amps;
					item->is220 = eu->load.circuit_is220;
					item->load.impedance_fraction = eu->load.fractions.z;
					item->load.current_fraction = eu->load.fractions.i;
					item->load.power_fraction = eu->load.fractions.p;
					item->load.power_factor = eu->load.power_factor;
					item->load.heatgain_fraction = eu->load.heat_fraction;
					implicit_enduse_list = item;
					found=1;
					break;
				}
			}
			if (found==0)
				gl_warning("house_e data for '%s' implicit enduse not found", token);
		}
	}
	total.name = "total";
	system.name = "system";

	return 1;
}

/** Checks for climate object and maps the climate variables to the house_e object variables.  
Currently Tout, RHout and solar flux data from TMY files are used.  If no climate object is linked,
then Tout will be set to 74 degF, RHout is set to 75% and solar flux will be set to zero for all orientations.
**/
int house_e::init_climate()
{
	static double tout=74.0, rhout=0.75, solar[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	OBJECT *hdr = OBJECTHDR(this);

	// link to climate data
	static FINDLIST *climates = NULL;
	int not_found = 0;
	if (climates==NULL && not_found==0) 
	{
		climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
		if (climates==NULL)
		{
			not_found = 1;
			gl_warning("house_e: no climate data found, using static data");

			//default to mock data
			pTout = &tout;
			pRhout = &rhout;
			pSolar = &solar[0];
		}
		else if (climates->hit_count>1)
		{
			gl_warning("house_e: %d climates found, using first one defined", climates->hit_count);
		}
	}
	if (climates!=NULL)
	{
		if (climates->hit_count==0)
		{
			//default to mock data
			pTout = &tout;
			pRhout = &rhout;
			pSolar = &solar[0];
		}
		else //climate data was found
		{
			// force rank of object w.r.t climate
			OBJECT *obj = gl_find_next(climates,NULL);
			if (obj->rank<=hdr->rank)
				gl_set_dependent(obj,hdr);
			pTout = (double*)GETADDR(obj,gl_get_property(obj,"temperature"));
			pRhout = (double*)GETADDR(obj,gl_get_property(obj,"humidity"));
			pSolar = (double*)GETADDR(obj,gl_get_property(obj,"solar_flux"));
			struct {
				char *name;
				double *dst;
			} map[] = {
				{"record.high",&cooling_design_temperature},
				{"record.low",&heating_design_temperature},
				{"record.solar",&design_peak_solar},
			};
			int i;
			for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			{
				double *src = (double*)GETADDR(obj,gl_get_property(obj,map[i].name));
				if (src) *map[i].dst = *src;
			}
		}
	}
	return 1;
}

/** Map circuit variables to meter.  Initalize house_e and HVAC model properties,
and internal gain variables.
**/

int house_e::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	// construct circuit variable map to meter
	struct {
		complex **var;
		char *varname;
	} map[] = {
		// local object name,	meter object name
		{&pCircuit_V,			"voltage_12"}, // assumes 1N and 2N follow immediately in memory
		{&pLine_I,				"current_1"}, // assumes 2 and 3(N) follow immediately in memory
		{&pLine12,				"current_12"},
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
	};

	extern complex default_line_voltage[3], default_line_current[3];
	static complex default_line_current_12;
	int i;

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	OBJECT *obj = OBJECTHDR(this);
	if (parent!=NULL && gl_object_isa(parent,"triplex_meter"))
	{
		// attach meter variables to each circuit
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			if ((*(map[i].var) = get_complex(parent,map[i].varname))==NULL)
				GL_THROW("%s (%s:%d) does not implement triplex_meter variable %s for %s (house_e:%d)", 
					parent->name?parent->name:"unnamed object", parent->oclass->name, parent->id, map[i].varname, obj->name?obj->name:"unnamed", obj->id);
		}
	}
	else
	{
		gl_warning("house_e:%d %s; using static voltages", obj->id, parent==NULL?"has no parent triplex_meter defined":"parent is not a triplex_meter");

		// attach meter variables to each circuit in the default_meter
		*(map[0].var) = &default_line_voltage[0];
		*(map[1].var) = &default_line_current[0];
		*(map[2].var) = &default_line_current_12;
	}

	// set defaults for panel/meter variables
	if (panel.max_amps==0) panel.max_amps = 200; 
	system.power = complex(0,0,J);

	// Set defaults for published variables nor provided by model definition
	if (heating_COP==0.0)		heating_COP = gl_random_triangle(1,2);
	if (cooling_COP==0.0)		cooling_COP = gl_random_triangle(2,4);

	if (aspect_ratio==0.0)		aspect_ratio = gl_random_triangle(1,2);
	if (floor_area==0)			floor_area = gl_random_triangle(1500,2500);
	if (ceiling_height==0)		ceiling_height = gl_random_triangle(7,9);
	if (gross_wall_area==0)		gross_wall_area = 4.0 * 2.0 * (aspect_ratio + 1.0) * ceiling_height * sqrt(floor_area/aspect_ratio);
	if (window_wall_ratio==0)	window_wall_ratio = 0.15;
	if (door_wall_ratio==0)		door_wall_ratio = 0.05;
	if (glazing_shgc==0)		glazing_shgc = 0.65; // assuming generic double glazing

	if (Rroof==0)				Rroof = gl_random_triangle(50,70);
	if (Rwall==0)				Rwall = gl_random_triangle(15,25);
	if (Rfloor==0)				Rfloor = gl_random_triangle(100,150);
	if (Rwindows==0)			Rwindows = gl_random_triangle(2,4);
	if (Rdoors==0)				Rdoors = gl_random_triangle(1,3);

	if (envelope_UA==0)			envelope_UA = floor_area*(1/Rroof+1/Rfloor) + gross_wall_area*((1-window_wall_ratio-door_wall_ratio)/Rwall + window_wall_ratio/Rwindows + door_wall_ratio/Rdoors);

	if (airchange_per_hour==0)	airchange_per_hour = gl_random_triangle(4,6);

	// initalize/set system model parameters
    if (COP_coeff==0)			COP_coeff = gl_random_uniform(0.9,1.1);	// coefficient of cops [scalar]
    if (Tair==0)				Tair = gl_random_uniform(heating_setpoint, cooling_setpoint);	// air temperature [F]
	if (over_sizing_factor==0)  over_sizing_factor = gl_random_uniform(0.98,1.3);
	if (thermostat_deadband==0)	thermostat_deadband = gl_random_triangle(2,3);
	if (heating_setpoint==0)	heating_setpoint = gl_random_triangle(68,72);
	if (cooling_setpoint==0)	cooling_setpoint = gl_random_triangle(75,79);
	if (design_internal_gains==0) design_internal_gains =  3.413 * floor_area * gl_random_triangle(4,6); // ~5 W/sf estimated
	if (design_cooling_capacity==0)	design_cooling_capacity = envelope_UA  * (cooling_design_temperature - cooling_setpoint) + 3.412*(design_peak_solar * gross_wall_area * window_wall_ratio * (1 - glazing_shgc)) + design_internal_gains;
	if (design_heating_capacity==0)	design_heating_capacity = envelope_UA * (heating_setpoint - heating_design_temperature);
    //if (system_mode==HC_UNKNOWN) system_mode = SM_OFF;	// heating/cooling mode {HEAT, COOL, OFF}

    air_density = 0.0735;			// density of air [lb/cf]
	air_heat_capacity = 0.2402;	// heat capacity of air @ 80F [BTU/lb/F]

	if (house_content_thermal_mass==0) house_content_thermal_mass = gl_random_triangle(4,6)*floor_area;		// thermal mass of house_e [BTU/F]
    if (house_content_heat_transfer_coeff==0) house_content_heat_transfer_coeff = gl_random_uniform(0.5,1.0)*floor_area;	// heat transfer coefficient of house_e contents [BTU/hr.F]

	if (system_mode==SM_OFF)
		Tair = gl_random_uniform(heating_setpoint,cooling_setpoint);
	else if (system_mode==SM_HEAT || system_mode==SM_AUX)
		Tair = gl_random_uniform(heating_setpoint-thermostat_deadband/2,heating_setpoint+thermostat_deadband/2);
	else if (system_mode==SM_COOL)
		Tair = gl_random_uniform(cooling_setpoint-thermostat_deadband/2,cooling_setpoint+thermostat_deadband/2);

	//house_e properties for HVAC
	volume = ceiling_height*floor_area;									// volume of air [cf]
	air_mass = air_density*volume;							// mass of air [lb]
	air_thermal_mass = air_heat_capacity*air_mass;			// thermal mass of air [BTU/F]
	Tmaterials = Tair;	
	
	// connect any implicit loads
	attach_implicit_enduses();
	update_system();
	update_model();

	// attach the house_e HVAC to the panel
	attach(OBJECTHDR(this),50, true, &system);

	return 1;
}

/**  Updates the aggregated power from all end uses, calculates the HVAC kWh use for the next synch time
**/
TIMESTAMP house_e::presync(TIMESTAMP t0, TIMESTAMP t1) 
{
	const double dt = ((double)(t1-t0)*TS_SECOND)/3600;

	/* advance the thermal state of the building */
	if (t0>0 && dt>0)
	{
		/* calculate model update, if possible */
		if (c2!=0)
		{
			/* update temperatures */
			const double e1 = k1*exp(r1*dt);
			const double e2 = k2*exp(r2*dt);
			Tair = e1 + e2 + Teq;
			Tmaterials = ((r1-c1)*e1 + (r2-c1)*e2 + c6)/c2 + Teq;
		}
	}
	return TS_NEVER;
}

/** Updates the total internal gain and synchronizes with the system equipment load.  
Also synchronizes the voltages and current in the panel with the meter.
**/
TIMESTAMP house_e::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER, t;
	const double dt1 = (double)(t1-t0)*TS_SECOND;

	double nHours = dt1 / 3600;

	if (t0==0 || t1>t0)
	{
		outside_temperature = *pTout;
		t = sync_enduses(t0,t1);
		if (t<t2) t2 = t;
		update_system(dt1);
		t = gl_enduse_sync(&system,t1);
		if (t<t2) t2 = t;
		update_model(dt1);
		check_controls();
	}

	/* solve for the time to the next event */
	double dt2;
	dt2 = e2solve(k1,r1,k2,r2,Teq-Tevent)*3600;
	if (isnan(dt2) || !isfinite(dt2) || dt2<0)
	{
		if (sgn(dTair)==sgn(Tair-Tevent)) // imminent control event
		{
			t = t1+1;
			if (t<t2) t2 = t;
		}
	}
	else if (dt2<TS_SECOND)
	{	
		t = t1+1; /* need to do a second pass to get next state */
		if (t<t2) t2 = t;
	}	
	else
	{
		t = t1+(TIMESTAMP)(ceil(dt2)*TS_SECOND); /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
		if (t<t2) t2 = t;
	}

	// sync circuit panel
	t = sync_panel(t0,t1);
	if (t < t2)	t2 = t;

	t = gl_enduse_sync(&total,t1);
	if (t < t2) t2 = t;

	return t2;
	
}

/** The PLC control code for house_e thermostat.  The heat or cool mode is based
	on the house_e air temperature, thermostat setpoints and deadband.
**/
TIMESTAMP house_e::sync_thermostat(TIMESTAMP t0, TIMESTAMP t1)
{
	double tdead = thermostat_deadband/2;
	double terr = dTair/3600; // this is the time-error of 1 second uncertainty
	const double TcoolOn = cooling_setpoint+tdead;
	const double TcoolOff = cooling_setpoint-tdead;
	const double TheatOn = heating_setpoint-tdead;
	const double TheatOff = heating_setpoint+tdead;

	// check for deadband overlap
	if (TcoolOff<TheatOff)
	{
		gl_error("house_e: thermostat setpoints deadbands overlap (TcoolOff=%.1f < TheatOff=%.1f)", TcoolOff,TheatOff);
		return TS_INVALID;
	}

	// change control mode if appropriate
	if (Tair<TheatOn-terr/2 && system_mode!=SM_HEAT) 
	{	// heating on
		// TODO: check for AUX
		system_mode = SM_HEAT;
		Tevent = TheatOff;
	}
	else if (Tair>TcoolOn-terr/2 && system_mode!=SM_COOL)
	{	// cooling on
		system_mode = SM_COOL;
		Tevent = TcoolOff;
	}
	else 
	{	// floating
		system_mode = SM_OFF;
		Tevent = ( dTair<0 ? TheatOn : TcoolOn );
	}

	return TS_NEVER;
}

void house_e::attach_implicit_enduses()
{
	IMPLICITENDUSE *item;
	for (item=implicit_enduse_list; item!=NULL; item=item->next)
		attach(NULL,item->amps,item->is220,&(item->load));

	return;
}

/// Attaches an end-use object to a house_e panel
/// The attach() method automatically assigns an end-use load
/// to the first appropriate available circuit.
/// @return pointer to the voltage on the assigned circuit
CIRCUIT *house_e::attach(OBJECT *obj, ///< object to attach
					   double breaker_amps, ///< breaker capacity (in Amps)
					   int is220, ///< 0 for 120V, 1 for 240V load
					   enduse *pLoad) ///< reference to load structure
{
	// construct and id the new circuit
	CIRCUIT *c = new CIRCUIT;
	if (c==NULL)
	{
		gl_error("memory allocation failure");
		return 0;
		/* Note ~ this returns a null pointer, but iff the malloc fails.  If
		 * that happens, we're already in SEGFAULT sort of bad territory. */
	}
	c->next = panel.circuits;
	c->id = panel.circuits ? panel.circuits->id+1 : 1;

	// set the breaker capacity
	c->max_amps = breaker_amps;

	// get address of load values (if any)
	if (pLoad)
		c->pLoad = pLoad;
	else if (obj)
	{
		c->pLoad = (enduse*)gl_get_addr(obj,"enduse_load");
		if (c->pLoad==NULL)
			GL_THROW("end-use load %s couldn't be connected because it does not publish 'enduse_load' property", c->pObj->name);
	}
	else
			GL_THROW("end-use load couldn't be connected neither an object nor a enduse property was given");
	
	// choose circuit
	if (is220 == 1) // 220V circuit is on x12
	{
		c->type = X12;
		c->id++; // use two circuits
	}
	else if (c->id&0x01) // odd circuit is on x13
		c->type = X13;
	else // even circuit is on x23
		c->type = X23;

	// attach to circuit list
	panel.circuits = c;

	// get voltage
	c->pV = &(pCircuit_V[(int)c->type]);

	// close breaker
	c->status = BRK_CLOSED;

	// set breaker lifetime (at average of 3.5 ops/year, 100 seems reasonable)
	// @todo get data on residential breaker lifetimes (residential, low priority)
	c->tripsleft = 100;

	// attach the enduse for future reference
	c->pObj = obj;
	return c;
}

TIMESTAMP house_e::sync_panel(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP sync_time = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);

	// clear accumulators for panel currents
	complex I[3]; I[X12] = I[X23] = I[X13] = complex(0,0);

	// clear accumulator
	total.heatgain = 0;
	total.total = total.power = total.current = total.admittance = complex(0,0);

	// gather load power and compute current for each circuit
	CIRCUIT *c;
	for (c=panel.circuits; c!=NULL; c=c->next)
	{
		// get circuit type
		int n = (int)c->type;
		if (n<0 || n>2)
			GL_THROW("%s:%d circuit %d has an invalid circuit type (%d)", obj->oclass->name, obj->id, c->id, (int)c->type);

		// if breaker is open and reclose time has arrived
		if (c->status==BRK_OPEN && t1>=c->reclose)
		{
			c->status = BRK_CLOSED;
			c->reclose = TS_NEVER;
			sync_time = t1; // must immediately reevaluate devices affected
			gl_debug("house_e:%d panel breaker %d closed", obj->id, c->id);
		}

		// if breaker is closed
		if (c->status==BRK_CLOSED)
		{
			// compute circuit current
			if ((c->pV)->Mag() == 0)
			{
				gl_debug("house_e:%d circuit %d (%s:%d) voltage is zero", obj->id, c->id, c->pObj->oclass->name, c->pObj->id);
				break;
			}
			
			complex current = ~(c->pLoad->total*1000 / *(c->pV)); 

			// check breaker
			if (current.Mag()>c->max_amps)
			{
				// probability of breaker failure increases over time
				if (c->tripsleft>0 && gl_random_bernoulli(1/(c->tripsleft--))==0)
				{
					// breaker opens
					c->status = BRK_OPEN;

					// average five minutes before reclosing, exponentially distributed
					c->reclose = t1 + (TIMESTAMP)(gl_random_exponential(1/300.0)*TS_SECOND); 
					gl_debug("house_e:%d circuit breaker %d tripped - %s (%s:%d) overload at %.0f A", obj->id, c->id,
						c->pObj->name?c->pObj->name:"unnamed object", c->pObj->oclass->name, c->pObj->id, current.Mag());
				}

				// breaker fails from too frequent operation
				else
				{
					c->status = BRK_FAULT;
					c->reclose = TS_NEVER;
					gl_debug("house_e:%d circuit breaker %d failed", obj->id, c->id);
				}

				// must immediately reevaluate everything
				sync_time = t1; 
			}

			// add to panel current
			else
			{
				total.total += c->pLoad->total;
				total.power += c->pLoad->power;
				total.current += c->pLoad->current;
				total.admittance += c->pLoad->admittance;
				total.heatgain += c->pLoad->heatgain;
				I[n] += current;
				c->reclose = TS_NEVER;
			}
		}

		// sync time
		if (sync_time > c->reclose)
			sync_time = c->reclose;
	}

	// compute line currents and post to meter
	if (obj->parent != NULL)
		LOCK_OBJECT(obj->parent);

	pLine_I[0] = I[X13];
	pLine_I[1] = I[X23];
	pLine_I[2] = 0;
	*pLine12 = I[X12];

	if (obj->parent != NULL)
		UNLOCK_OBJECT(obj->parent);

	return sync_time;
}

TIMESTAMP house_e::sync_enduses(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	IMPLICITENDUSE *eu;
	for (eu=implicit_enduse_list; eu!=NULL; eu=eu->next)
	{
		TIMESTAMP t = gl_enduse_sync(&(eu->load),t1);
		if (t<t2) t2 = t;
	}
	return t2;
}

void house_e::update_model(double dt)
{
		/* local aliases */
	const double &Tout = (*(pTout));
	const double &Ua = (envelope_UA);
	const double &Cm = (house_content_thermal_mass);
	const double &Um = (house_content_heat_transfer_coeff);
	const double &Qi = (system.heatgain);
	double &Qs = (solar_load);
	double &Qh = (system.heatgain);
	double &Ti = (Tair);
	double &dTi = (dTair);
	double &Tm = (Tmaterials);
	SYSTEMMODE &mode = (system_mode);
	const double tdead = thermostat_deadband/2;
	const double TheatOff = heating_setpoint + tdead;
	const double TheatOn = heating_setpoint - tdead;
	const double TcoolOff = cooling_setpoint - tdead;
	const double TcoolOn = cooling_setpoint + tdead;
	const double Ca = air_heat_capacity * air_density * ceiling_height * floor_area;

	/* compute solar gains */
	Qs = 0; 
	int i;
	for (i=0; i<9; i++)
		Qs += (gross_wall_area*window_wall_ratio/8.0) * glazing_shgc * pSolar[i];
	Qs *= 3.412;
	if (Qs<0)
		throw "solar gain is negative?!?";

	if (Ca<=0)
		throw "Ca must be positive";
	if (Cm<=0)
		throw "Cm must be positive";

	// split gains to air and mass
	double f_air = 1.0; /* adjust the fraction of gains that goes to air vs mass */
	double Qa = Qh + f_air*(Qi + Qs);
	double Qm = (1-f_air)*(Qi + Qs);

	c1 = -(Ua + Um)/Ca;
	c2 = Um/Ca;
	c3 = (Qa + Tout*Ua)/Ca;
	c6 = Qm/Cm;
	c7 = Qa/Ca;
	double p1 = 1/c2;
	if (Cm<=0)
		throw "Cm must be positive";
	c4 = Um/Cm;
	c5 = -c4;
	if (c2<=0)
		throw "Um must be positive";
	double p2 = -(c5+c1)/c2;
	double p3 = c1*c5/c2 - c4;
	double p4 = -c3*c5/c2 + c6;
	if (p3==0)
		throw "Teq is not finite";
	Teq = p4/p3;

	/* compute solution roots */
	if (p1==0)
		throw "internal error (p1==0 -> Ca==0 which should have caught)";
	const double ra = 2*p1;
	const double rb = -p2/ra;
	const double rr = p2*p2-4*p1*p3;
	if (rr<0)
		throw "thermal solution does not exist";
	const double rc = sqrt(rr)/ra;
	r1 = rb+rc;
	r2 = rb-rc;
	if (r1>0 || r2>0)
		throw "thermal solution has runaway condition";

	/* compute next initial condition */
	dTi = c2*Tm + c1*Ti - (c1+c2)*Tout + c7;
	k1 = (r1*Ti - r2*Teq - dTi)/(r2-r1);
	k2 = (dTi - r1*k1)/r2;
}

/** HVAC load synchronizaion is based on the equipment capacity, COP, solar loads and total internal gain
from end uses.  The modeling approach is based on the Equivalent Thermal Parameter (ETP)
method of calculating the air and mass temperature in the conditioned space.  These are solved using
a dual decay solver to obtain the time for next state change based on the thermostat set points.
This synchronization function updates the HVAC equipment load and power draw.
**/

void house_e::update_system(double dt)
{
	// compute system performance
	const double heating_cop_adj = (-0.0063*(*pTout)+1.5984);
	const double cooling_cop_adj = -(-0.0108*(*pTout)+2.0389);
	const double heating_capacity_adj = (-0.0063*(*pTout)+1.5984);
	const double cooling_capacity_adj = -(-0.0063*(*pTout)+1.5984);

	switch (system_mode) {
	case SM_HEAT:
	case SM_AUX:
		system_rated_capacity = design_heating_capacity*heating_capacity_adj;
		system_rated_power = system_rated_capacity/(heating_COP * heating_cop_adj);
		break;
	case SM_COOL:
		system_rated_capacity = design_cooling_capacity*cooling_capacity_adj;
		system_rated_power = system_rated_capacity/(cooling_COP * cooling_cop_adj);
		break;
	default:
		system_rated_capacity = 0.0;
		system_rated_power = 0.0;
	}

	/* calculate the power consumption */
	system.power = system_rated_power*KWPBTUPH * ((system_mode == SM_HEAT) && (system_type&ST_GAS) ? 0.01 : 1.0);
	system.heatgain = system_rated_capacity;
}

void house_e::check_controls(void)
{
	if (warn_control)
	{
		/* check for air temperature excursion */
		if (Tair<warn_low_temp || Tair>warn_high_temp)
		{
			OBJECT *obj = OBJECTHDR(this);
			DATETIME dt0;
			gl_localtime(obj->clock,&dt0);
			char ts0[64];
			gl_warning("house_e:%d (%s) air temperature excursion (%.1f degF) at %s", 
				obj->id, obj->name?obj->name:"anonymous", Tair, gl_strtime(&dt0,ts0,sizeof(ts0))?ts0:"UNKNOWN");
		}

		/* check for mass temperature excursion */
		if (Tmaterials<warn_low_temp || Tmaterials>warn_high_temp)
		{
			OBJECT *obj = OBJECTHDR(this);
			DATETIME dt0;
			gl_localtime(obj->clock,&dt0);
			char ts0[64];
			gl_warning("house_e:%d (%s) mass temperature excursion (%.1f degF) at %s", 
				obj->id, obj->name?obj->name:"anonymous", Tmaterials, gl_strtime(&dt0,ts0,sizeof(ts0))?ts0:"UNKNOWN");
		}

		/* check for heating equipment sizing problem */
		if ((system_mode==SM_HEAT || system_mode==SM_AUX) && Teq<heating_setpoint)
		{
			OBJECT *obj = OBJECTHDR(this);
			DATETIME dt0;
			gl_localtime(obj->clock,&dt0);
			char ts0[64];
			gl_warning("house_e:%d (%s) heating equipement undersized at %s", 
				obj->id, obj->name?obj->name:"anonymous", gl_strtime(&dt0,ts0,sizeof(ts0))?ts0:"UNKNOWN");
		}

		/* check for cooling equipment sizing problem */
		else if (system_mode==SM_COOL && Teq>cooling_setpoint)
		{
			OBJECT *obj = OBJECTHDR(this);
			DATETIME dt0;
			gl_localtime(obj->clock,&dt0);
			char ts0[64];
			gl_warning("house_e:%d (%s) cooling equipement undersized at %s", 
				obj->id, obj->name?obj->name:"anonymous", gl_strtime(&dt0,ts0,sizeof(ts0))?ts0:"UNKNOWN");
		}
	}
}

complex *house_e::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_house_e(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(house_e::oclass);
	if (*obj!=NULL)
	{
		house_e *my = OBJECTDATA(*obj,house_e);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_house_e(OBJECT *obj)
{
	try {
		house_e *my = OBJECTDATA(obj,house_e);
		my->init_climate();
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		gl_error("house_e:%d (%s) %s", obj->id, obj->name?obj->name:"anonymous", msg);
		return 0;
	}
}

EXPORT TIMESTAMP sync_house_e(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	house_e *my = OBJECTDATA(obj,house_e);
	TIMESTAMP t1 = TS_NEVER;
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the object clock if it has not been set yet

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
				gl_error("house_e::sync- invalid pass configuration");
				t1 = TS_INVALID; // serious error in exec.c
		}
	} 
	catch (char *msg)
	{
		gl_error("house_e::sync exception caught: %s", msg);
		t1 = TS_INVALID;
	}
	catch (...)
	{
		gl_error("house_e::sync exception caught: no info");
		t1 = TS_INVALID;
	}
	return t1;
}

EXPORT TIMESTAMP plc_house_e(OBJECT *obj, TIMESTAMP t0)
{
	// this will be disabled if a PLC object is attached to the waterheater
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the clock if it has not been set yet

	house_e *my = OBJECTDATA(obj,house_e);
	return my->sync_thermostat(obj->clock, t0);
}

/**@}**/
