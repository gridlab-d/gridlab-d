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

//////////////////////////////////////////////////////////////////////////
// implicit loadshapes
//////////////////////////////////////////////////////////////////////////
static LOADSHAPE default_loadshapes[] = {

	{"clotheswasher",
		LST_SEASONAL|LST_WEEKDAY, // pulsed with different seasonal/weekday components
		1.0, 0.0, // scalar (pu or kW) and event (kW)
		0.8, 0.25, // magnitude (kW) and duration (h)
		{	// winter weekday
			0.0036,0.0024,0.0020,0.0019,0.0026,0.0040,0.0062,0.0118,0.0177,0.0211,0.0215,0.0203,0.0176,0.0155,0.0133,0.0130,0.0145,0.0159,0.0166,0.0164,0.0154,0.0149,0.0110,0.0065,
			// winter weekend
			0.0044,0.0030,0.0022,0.0020,0.0021,0.0021,0.0030,0.0067,0.0145,0.0244,0.0310,0.0323,0.0308,0.0285,0.0251,0.0224,0.0215,0.0203,0.0194,0.0188,0.0180,0.0151,0.0122,0.0073,
			// summer weekday
			0.0029,0.0019,0.0014,0.0013,0.0018,0.0026,0.0055,0.0126,0.0181,0.0208,0.0229,0.0216,0.0193,0.0170,0.0145,0.0135,0.0135,0.0142,0.0145,0.0148,0.0146,0.0141,0.0110,0.0062,
			// summer weekend
			0.0031,0.0019,0.0013,0.0012,0.0012,0.0016,0.0027,0.0066,0.0157,0.0220,0.0258,0.0251,0.0231,0.0217,0.0186,0.0157,0.0156,0.0151,0.0147,0.0150,0.0156,0.0148,0.0106,0.0065,
		},
		0.95, // power factor
		0.0, 0.0, // current and impedance fractions
		0.5, // heat fraction
		20, false, // 20 amps, 110V
		NULL // no linkage for defaults
	},

	{"dryer",
		LST_WEEKDAY, // pulsed with different seasonal/weekday components
		1.0, 0.0, // scalar (pu or kW) and event (kW)
		4.5, 0.75, // magnitude (kW) and duration (h)
		{	// weekday
			0.034,0.0145,0.0085,0.0065,0.0075,0.0225,0.0625,0.1075,0.1325,0.1715,0.1945,0.1955,0.1765,0.1635,0.1465,0.1415,0.1465,0.1525,0.1495,0.1495,0.1605,0.1685,0.1385,0.079,
			0.036,0.017,0.0106,0.0066,0.0066,0.0086,0.0186,0.0506,0.1106,0.1836,0.2446,0.2696,0.2686,0.2586,0.2376,0.2236,0.2156,0.2036,0.1906,0.1826,0.1806,0.1786,0.1436,0.086,
		},
		0.99, // power factor
		0.0, 0.9, // current and impedance fractions
		0.2, // heat fraction
		30, true, // 30 amps, 220V
		NULL // no linkage for defaults
	},

	{"dishwasher",
		LST_WEEKDAY, // pulsed with different seasonal/weekday components
		1.0, 0.0, // scalar (pu or kW) and event (kW)
		4.5, 0.75, // magnitude (kW) and duration (h)
		{	// weekday
			0.0068,0.0029,0.0016,0.0013,0.0012,0.0037,0.0075,0.0129,0.0180,0.0177,0.0144,0.0113,0.0116,0.0128,0.0109,0.0105,0.0124,0.0156,0.0278,0.0343,0.0279,0.0234,0.0194,0.0131,
			0.0093,0.0045,0.0021,0.0015,0.0013,0.0015,0.0026,0.0067,0.0142,0.0221,0.0259,0.0238,0.0214,0.0214,0.0188,0.0169,0.0156,0.0166,0.0249,0.0298,0.0267,0.0221,0.0174,0.0145,
		},
		0.99, // power factor
		0.0, 0.5, // current and impedance fractions
		0.2, // heat fraction
		20, false, // 20 amps, 110V
		NULL // no linkage for defaults
	},

	{"refrigerator",
		LST_DIRECT, // absolute load, single shape
		1.0, 0.0,
		0.3, 0.0,
			{	0.163,0.1605,0.1555,0.1515,0.1485,0.1525,0.1615,0.1655,0.1635,0.1645,0.1635,0.1645,0.1715,0.1715,0.1695,0.1725,0.1805,0.1945,0.2025,0.1965,0.1915,0.1885,0.1805,
			},
		0.97,// power factor
		0.0, 0.5, // current and impedance fractions
		0.5, // heat fraction
		20, false, // 20 amps, 110V
		NULL // no linkage for defaults
	},

	{"freezer",
		LST_DIRECT, // absolute load, single shape
		1.0, 0.0,
		0.3, 0.0,
			{	0.163,0.1605,0.1555,0.1515,0.1485,0.1525,0.1615,0.1655,0.1635,0.1645,0.1635,0.1645,0.1715,0.1715,0.1695,0.1725,0.1805,0.1945,0.2025,0.1965,0.1915,0.1885,0.1805,
			},
		0.97,// power factor
		0.0, 0.5, // current and impedance fractions
		0.5, // heat fraction
		20, false, // 20 amps, 110V
		NULL // no linkage for defaults
	},
};

//////////////////////////////////////////////////////////////////////////
// house_e CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* house_e::oclass = NULL;
house_e *house_e::defaults = NULL;

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
			PT_double,"floor_area[sf]",PADDR(floor_area),
			PT_double,"gross_wall_area[sf]",PADDR(gross_wall_area),
			PT_double,"ceiling_height[ft]",PADDR(ceiling_height),
			PT_double,"aspect_ratio",PADDR(aspect_ratio),
			PT_double,"envelope_UA[Btu/degF.h]",PADDR(envelope_UA),
			PT_double,"window_wall_ratio",PADDR(window_wall_ratio),
			PT_double,"glazing_shgc",PADDR(glazing_shgc),
			PT_double,"airchange_per_hour",PADDR(airchange_per_hour),
			PT_double,"internal_gain[Btu/h]",PADDR(tload.heatgain),
			PT_double,"solar_gain[Btu/h]",PADDR(solar_load),
			PT_double,"heat_cool_gain[Btu/h]",PADDR(load.heatgain),
			PT_double,"thermostat_deadband[degF]",PADDR(thermostat_deadband),
			PT_double,"heating_setpoint[degF]",PADDR(heating_setpoint),
			PT_double,"cooling_setpoint[degF]",PADDR(cooling_setpoint),
			PT_double, "design_heating_capacity[Btu.h/sf]",PADDR(design_heating_capacity),
			PT_double,"design_cooling_capacity[Btu.h/sf]",PADDR(design_cooling_capacity),
			PT_double, "cooling_design_temperature[degF]", PADDR(cooling_design_temperature),
			PT_double, "heating_design_temperature[degF]", PADDR(heating_design_temperature),
			PT_double, "design_peak_solar[W/sf]", PADDR(design_peak_solar),
			PT_double, "design_internal_gains[W/sf]", PADDR(design_peak_solar),

			PT_double,"heating_COP",PADDR(heating_COP),
			PT_double,"cooling_COP",PADDR(cooling_COP),
			PT_double,"COP_coeff",PADDR(COP_coeff),
			PT_double,"air_temperature[degF]",PADDR(Tair),
			PT_double,"mass_heat_capacity[Btu/F]",PADDR(house_content_thermal_mass),
			PT_double,"mass_heat_coeff[Btu/F.h]",PADDR(house_content_heat_transfer_coeff),
			PT_double,"mass_temperature[degF]",PADDR(Tmaterials),
			PT_enumeration,"heat_mode",PADDR(heat_type),
				PT_KEYWORD,"UNKNOWN",HT_UNKNOWN,
				PT_KEYWORD,"ELECTRIC",HT_ELECTRIC,
				PT_KEYWORD,"GASHEAT",HT_GASHEAT,
			PT_complex,"total_load[kW]",PADDR(tload.total),
			PT_complex,"enduse_load[kW]",PADDR(load.total),
			PT_complex,"power[kW]",PADDR(load.power),
			PT_complex,"current[A]",PADDR(load.current),
			PT_complex,"admittance[1/Ohm]",PADDR(load.admittance),
			PT_enumeration,"hc_mode",PADDR(heat_cool_mode),
				PT_KEYWORD,"UNKNOWN",HC_UNKNOWN,
				PT_KEYWORD,"HEAT",HC_HEAT,
				PT_KEYWORD,"OFF",HC_OFF,
				PT_KEYWORD,"COOL",HC_COOL,
				PT_KEYWORD,"OFF",HC_OFF,
			PT_double, "Rroof[degF.h/Btu]", PADDR(Rroof),
			PT_double, "Rwall[degF.h/Btu]", PADDR(Rwall),
			PT_double, "Rfloor[degF.h/Btu]", PADDR(Rfloor),
			PT_double, "Rwindows[degF.h/Btu]", PADDR(Rwindows),
			PT_char256, "implicit_enduses", PADDR(implicit_enduses),
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);

		// deafults set during class creation
		defaults = this;
		memset(this,0,sizeof(house_e));
		strcpy(implicit_enduses,"clotheswasher,dishwasher,dryer,freezer,light,microwave,occupants,plugs,refrigerator,waterheater");
	}	
}

int house_e::create() 
{
	// copy the defaults
	memcpy(this,defaults,sizeof(house_e));

	return 1;
}

/** Checks for climate object and maps the climate variables to the house_e object variables.  
Currently Tout, RHout and solar flux data from TMY files are used.  If no climate object is linked,
then Tout will be set to 59 degF, RHout is set to 75% and solar flux will be set to zero for all orientations.
**/
int house_e::init_climate()
{
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
			static double tout=74.0, rhout=0.75, solar[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
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
			static double tout=74.0, rhout=0.75, solar[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
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
		/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
	};

	extern complex default_line_voltage[3], default_line_current[3];
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
	}

	// set defaults for panel/meter variables
	if (panel.max_amps==0) panel.max_amps = 200; 
	load.power = complex(0,0,J);
	load.admittance = complex(0,0,J);
	load.current = complex(0,0,J);

	// Set defaults for published variables nor provided by model definition
	if (heat_type==HT_UNKNOWN)	heat_type = HT_ELECTRIC;
	if (heating_COP==0.0)		heating_COP = gl_random_triangle(1,2);
	if (cooling_COP==0.0)		cooling_COP = gl_random_triangle(2,4);

	if (aspect_ratio==0.0)		aspect_ratio = gl_random_triangle(1,2);
	if (floor_area==0)			floor_area = gl_random_triangle(1500,2500);
	if (ceiling_height==0)		ceiling_height = gl_random_triangle(7,9);
	if (gross_wall_area==0)		gross_wall_area = 4.0 * 2.0 * (aspect_ratio + 1.0) * ceiling_height * sqrt(floor_area/aspect_ratio);
	if (window_wall_ratio==0)	window_wall_ratio = 0.15;
	if (glazing_shgc==0)		glazing_shgc = 0.65; // assuming generic double glazing

	if (Rroof==0)				Rroof = gl_random_triangle(50,70);
	if (Rwall==0)				Rwall = gl_random_triangle(15,25);
	if (Rfloor==0)				Rfloor = gl_random_triangle(100,150);
	if (Rwindows==0)			Rwindows = gl_random_triangle(2,4);

	if (envelope_UA==0)			envelope_UA = floor_area*(1/Rroof+1/Rfloor) + gross_wall_area*((1-window_wall_ratio)/Rwall + window_wall_ratio/Rwindows);

	if (airchange_per_hour==0)	airchange_per_hour = gl_random_triangle(4,6);

	// initalize/set hvac model parameters
    if (COP_coeff==0)			COP_coeff = gl_random_uniform(0.9,1.1);	// coefficient of cops [scalar]
    if (Tair==0)				Tair = gl_random_uniform(heating_setpoint, cooling_setpoint);	// air temperature [F]
	if (over_sizing_factor==0)  over_sizing_factor = gl_random_uniform(0.98,1.3);
	if (thermostat_deadband==0)	thermostat_deadband = gl_random_triangle(2,3);
	if (heating_setpoint==0)	heating_setpoint = gl_random_triangle(68,72);
	if (cooling_setpoint==0)	cooling_setpoint = gl_random_triangle(75,79);
	if (design_internal_gains==0) design_internal_gains =  3.413 * floor_area * gl_random_triangle(4,6); // ~5 W/sf estimated
	if (design_cooling_capacity==0)	design_cooling_capacity = envelope_UA  * (cooling_design_temperature - cooling_setpoint) + 3.412*(design_peak_solar * gross_wall_area * window_wall_ratio * (1 - glazing_shgc)) + design_internal_gains;
	if (design_heating_capacity==0)	design_heating_capacity = envelope_UA * (heating_setpoint - heating_design_temperature);
    if (heat_cool_mode==HC_UNKNOWN) heat_cool_mode = HC_OFF;	// heating/cooling mode {HEAT, COOL, OFF}

    air_density = 0.0735;			// density of air [lb/cf]
	air_heat_capacity = 0.2402;	// heat capacity of air @ 80F [BTU/lb/F]

	if (house_content_thermal_mass==0) house_content_thermal_mass = gl_random_triangle(4,6)*floor_area;		// thermal mass of house_e [BTU/F]
    if (house_content_heat_transfer_coeff==0) house_content_heat_transfer_coeff = gl_random_uniform(0.5,1.0)*floor_area;	// heat transfer coefficient of house_e contents [BTU/hr.F]

	if (heat_cool_mode==HC_OFF)
		Tair = gl_random_uniform(heating_setpoint,cooling_setpoint);
	else if (heat_cool_mode==HC_HEAT || heat_cool_mode==HC_AUX)
		Tair = gl_random_uniform(heating_setpoint-thermostat_deadband/2,heating_setpoint+thermostat_deadband/2);
	else if (heat_cool_mode==HC_COOL)
		Tair = gl_random_uniform(cooling_setpoint-thermostat_deadband/2,cooling_setpoint+thermostat_deadband/2);

	//house_e properties for HVAC
	volume = ceiling_height*floor_area;									// volume of air [cf]
	air_mass = air_density*volume;							// mass of air [lb]
	air_thermal_mass = air_heat_capacity*air_mass;			// thermal mass of air [BTU/F]
	Tmaterials = Tair;										// material temperture [F]

	// connect any implicit loads
	attach_implicit_enduses();
	update_hvac();
	update_model();

	// attach the house_e HVAC to the panel
	attach(OBJECTHDR(this),50, true);

	return 1;
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
	if (Tair<TheatOn-terr/2 && heat_cool_mode!=HC_HEAT) 
	{	// heating on
		// TODO: check for AUX
		heat_cool_mode = HC_HEAT;
		Tevent = TheatOff;
	}
	else if (Tair>TcoolOn-terr/2 && heat_cool_mode!=HC_COOL)
	{	// cooling on
		heat_cool_mode = HC_COOL;
		Tevent = TcoolOff;
	}
	else 
	{	// floating
		heat_cool_mode = HC_OFF;
		Tevent = ( dTair<0 ? TheatOn : TcoolOn );
	}

	return TS_NEVER;
}

/**  Updates the aggregated power from all end uses, calculates the HVAC kWh use for the next synch time
**/
TIMESTAMP house_e::presync(TIMESTAMP t0, TIMESTAMP t1) 
{
	const double dt1 = (double)(t1-t0)*TS_SECOND;

	/* advance the thermal state of the building */
	if (t0>0 && dt1>0)
	{
		const double dt = dt1/3600; /* model operates in units of hours */
		load.energy += load.total.Mag() * dt;

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

	// reset accumulators for the next sync
	/* HVAC accumulators */
	load.heatgain = 0;
	load.total = complex(0,0,J);
	load.admittance = complex(0,0,J);
	load.current = complex(0,0,J);

	/* main panel accumulators */
	tload.heatgain = 0;
	tload.total = complex(0,0,J);
	tload.admittance = complex(0,0,J);
	tload.current = complex(0,0,J);

	return TS_NEVER;
}

/** Updates the total internal gain and synchronizes with the hvac equipment load.  
Also synchronizes the voltages and current in the panel with the meter.
**/

TIMESTAMP house_e::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t2 = TS_NEVER;
	TIMESTAMP te = TS_NEVER;
	const double dt1 = (double)(t1-t0)*TS_SECOND;

	if (t0==0 || t1>t0)
	{
		te = sync_enduses(t0,t1);
		update_hvac(dt1);
		update_model(dt1);
		check_controls();
	}

	/* solve for the time to the next event */
	double dt2=(double)TS_NEVER;
	dt2 = e2solve(k1,r1,k2,r2,Teq-Tevent)*3600;
	if (isnan(dt2) || !isfinite(dt2) || dt2<0)
	{
		if (sgn(dTair)==sgn(Tair-Tevent)) // imminent control event
			t2 = t1+1;
	}
	else if (dt2<TS_SECOND)
		t2 = t1+1; /* need to do a second pass to get next state */
	else
		t2 = t1+(TIMESTAMP)(ceil(dt2)*TS_SECOND); /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */

	// sync circuit panel
	TIMESTAMP panel_time = sync_panel(t0,t1);
	if (panel_time < t2)
		t2 = panel_time;

	return (te!=TS_NEVER&&te<t2)?(t2<TS_NEVER?te:-te):t2;
	
}

void house_e::attach_implicit_enduses(char *enduses)
{
	char list[256], *next=NULL;
	strcpy(list,enduses?enduses:implicit_enduses);
	if (strcmp(list,"none")==0)
		return;
	while ((next=strtok(next?NULL:list,",; \t\n"))!=NULL)
	{
		LOADSHAPE *shape = NULL;
		int i;
		for (i=0; i<sizeof(default_loadshapes)/sizeof(default_loadshapes[0]); i++)
		{
			if (strcmp(default_loadshapes[i].name,next)==0)
			{
				shape = default_loadshapes+i;
				break;
			}
		}
		if (shape==NULL)
			gl_warning("implicit enduse '%s' is not defined in the default loadshapes", next);
		else
		{
			LOADSHAPE *ls = new LOADSHAPE;
			if (ls==NULL)
				throw "house_e::attach_implicity_enduses(char *enduses): memory allocation failure";
			memcpy(ls,shape,sizeof(LOADSHAPE));
			memset(&(ls->load),0,sizeof(ENDUSELOAD));
			ls->next = implicit_loads;
			implicit_loads = ls;
			attach(NULL,ls->breaker_amps,ls->is220,&(ls->load));
		}
	}
	return;
}

/// Attaches an end-use object to a house_e panel
/// The attach() method automatically assigns an end-use load
/// to the first appropriate available circuit.
/// @return pointer to the voltage on the assigned circuit
CIRCUIT *house_e::attach(OBJECT *obj, ///< object to attach
					   double breaker_amps, ///< breaker capacity (in Amps)
					   int is220, ///< 0 for 120V, 1 for 240V load
					   ENDUSELOAD *pLoad) ///< reference to load structure
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
	else 
		c->pLoad = (ENDUSELOAD*)gl_get_addr(obj,"enduse_load");
	if (c->pLoad==NULL){
		GL_THROW("end-use load %s couldn't be connected because it does not publish ENDUSELOAD structure", c->enduse->name);
	}
	
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
	c->enduse = obj;

	return c;
}

TIMESTAMP house_e::sync_panel(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP sync_time = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);

	// clear accumulators for panel currents
	complex I[3]; I[X12] = I[X23] = I[X13] = complex(0,0);

	// clear heatgain accumulator
	double heatgain = 0;

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
				gl_debug("house_e:%d circuit %d (%s:%d) voltage is zero", obj->id, c->id, c->enduse->oclass->name, c->enduse->id);
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
						c->enduse->name?c->enduse->name:"unnamed object", c->enduse->oclass->name, c->enduse->id, current.Mag());
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
				tload.power += c->pLoad->power;	// reminder: |a| + |b| != |a+b|
				tload.current += c->pLoad->current;
				tload.admittance += c->pLoad->admittance; // should this be additive? I don't buy t.a = c->pL->a ... -MH
				tload.total += c->pLoad->total;
				tload.heatgain += c->pLoad->heatgain;
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

	pLine_I[0] = I[X12]+I[X13];
	pLine_I[1] = I[X23]+I[X12];
	pLine_I[2] = -I[X13]-I[X23];

	if (obj->parent != NULL)
		UNLOCK_OBJECT(obj->parent);

	return sync_time;
}

TIMESTAMP house_e::sync_enduses(TIMESTAMP t0, TIMESTAMP t1)
{
	double duration = (double)(3600-t1%3600)/3600.0;
	double dt = (double)(t1-t0)/3600;
	TIMESTAMP t2 = t1 + 3600 - t1%3600;
	OBJECT *obj = OBJECTHDR(this);
	DATETIME t;
	gl_localtime(obj->clock,&t);
	int season = (t.month<5 || t.month>10) ? LS_WINTER : LS_SUMMER;
	int daytype = (t.weekday<1 || t.weekday>5) ? LS_WEEKEND : LS_WEEKDAY;
	int quad = season*2 + daytype;
	int hour = t.hour;
	LOADSHAPE *ls;
	for (ls=implicit_loads; ls!=NULL; ls=ls->next)
	{
		/* shut off load */
		if (ls->stopat>0 && ls->stopat<=t1)
		{
			ls->stopat = 0;
			ls->value = 0;
		}
		else if (ls->stopat>t1 && ls->stopat<TS_NEVER)
		{
			// keep load on
		}

		/* possible new load */
		else if (dt>0)
		{
			// get the value based on which shape is current
			double value=0;

			// get the time until which this load is active
			TIMESTAMP stopat = t1 + (TIMESTAMP)(duration*3600);

			register int type = ls->type;
			if (type&(LST_SEASONAL|LST_WEEKDAY))
				value = ls->shape[quad][hour];
			else if (type&LST_SEASONAL)
				value = ls->shape[season][hour];
			else if (type&LST_WEEKDAY)
				value = ls->shape[daytype][hour];
			else
				value = ls->shape[0][hour];

			// implement queue
			if (ls->event>0)
			{
				ls->queue += value*dt; // fraction of value to posted by now
				double action = ls->queue - ls->event;
				if (action>0 && gl_random_bernoulli(action))
					value = ls->event;
				else
					value = 0;
			}

			// pulse width modulated load
			if (ls->magnitude>0)
			{
				if (gl_random_bernoulli(value/ls->magnitude*dt))
				{
					double trun;
					if (ls->duration==0)
						trun = value *ls->scalar / ls->magnitude;
					else
						trun = ls->duration;
					value = ls->magnitude * trun;
					stopat = t1 + (TIMESTAMP)(trun*3600);
				}
				else
				{
					value = 0;
					stopat = 0;
				}
			}

			// apply scaling factor
			else if (type&LST_NORMAL) 
				value *= ls->scalar;

			// apply floor_area factor
			if (type&LST_RELATIVE) 
				value *= floor_area;

			// else amplitude modulated load
			ls->value = value;
			ls->stopat = stopat;
		}

		ls->load.heatgain = ls->value * 3412 * ls->heat_fraction;
		ls->load.total.SetPowerFactor(ls->value,ls->power_factor);
		ls->load.admittance = ls->load.total * ls->impedance_fraction;
		ls->load.current = ls->load.total *ls->current_fraction;
		ls->load.power = ls->load.total * (1-ls->impedance_fraction-ls->current_fraction);
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
	const double &Qi = (tload.heatgain);
	double &Qs = (solar_load);
	double &Qh = (load.heatgain);
	double &Ti = (Tair);
	double &dTi = (dTair);
	double &Tm = (Tmaterials);
	HCMODE &mode = (heat_cool_mode);
	const double tdead = thermostat_deadband/2;
	const double TheatOff = heating_setpoint + tdead;
	const double TheatOn = heating_setpoint - tdead;
	const double TcoolOff = cooling_setpoint - tdead;
	const double TcoolOn = cooling_setpoint + tdead;
	const double Ca = 0.2402 * 0.0735 * ceiling_height * floor_area;

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
	k1 = (r2*Ti - r2*Teq - dTi)/(r2-r1);
	k2 = (dTi - r1*k1)/r2;
}

/** HVAC load synchronizaion is based on the equipment capacity, COP, solar loads and total internal gain
from end uses.  The modeling approach is based on the Equivalent Thermal Parameter (ETP)
method of calculating the air and mass temperature in the conditioned space.  These are solved using
a dual decay solver to obtain the time for next state change based on the thermostat set points.
This synchronization function updates the HVAC equipment load and power draw.
**/

void house_e::update_hvac(double dt)
{
	// compute hvac performance
	const double heating_cop_adj = (-0.0063*(*pTout)+1.5984);
	const double cooling_cop_adj = -(-0.0108*(*pTout)+2.0389);
	const double heating_capacity_adj = (-0.0063*(*pTout)+1.5984);
	const double cooling_capacity_adj = -(-0.0063*(*pTout)+1.5984);

	switch (heat_cool_mode) {
	case HC_HEAT:
	case HC_AUX:
		hvac_rated_capacity = design_heating_capacity*heating_capacity_adj;
		hvac_rated_power = hvac_rated_capacity/(heating_COP * heating_cop_adj);
		break;
	case HC_COOL:
		hvac_rated_capacity = design_cooling_capacity*cooling_capacity_adj;
		hvac_rated_power = hvac_rated_capacity/(cooling_COP * cooling_cop_adj);
		break;
	default:
		hvac_rated_capacity = 0.0;
		hvac_rated_power = 0.0;
	}

	/* calculate the power consumption */
	load.power = hvac_rated_power*KWPBTUPH * ((heat_cool_mode == HC_HEAT) && (heat_type == HT_GASHEAT) ? 0.01 : 1.0);
	load.admittance = 0;
	load.current = 0;
	load.total = load.power;
	load.heatgain = hvac_rated_capacity;
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
		if ((heat_cool_mode==HC_HEAT || heat_cool_mode==HC_AUX) && Teq<heating_setpoint)
		{
			OBJECT *obj = OBJECTHDR(this);
			DATETIME dt0;
			gl_localtime(obj->clock,&dt0);
			char ts0[64];
			gl_warning("house_e:%d (%s) heating equipement undersized at %s", 
				obj->id, obj->name?obj->name:"anonymous", gl_strtime(&dt0,ts0,sizeof(ts0))?ts0:"UNKNOWN");
		}

		/* check for cooling equipment sizing problem */
		else if (heat_cool_mode==HC_COOL && Teq>cooling_setpoint)
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
