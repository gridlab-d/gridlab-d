/** $Id: house_a.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file house.cpp
	@addtogroup house
	@ingroup residential

	The house object implements a single family home.  The house
	only includes the heating/cooling system and the power panel.
	All other end-uses must be explicitly defined and attached to the
	panel using the house::attach() method.

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
	given by load when house::attach() was called.  After a breaker opens, it is
	reclosed within an average of 5 minutes (on an exponential distribution).  Each
	time the breaker is reclosed, the breaker failure probability is increased.
	The probability of failure is always 1/N where N is initially a large number (e.g., 100). 
	N is progressively decremented until it reaches 1 and the probability of failure is 100%.
	
	The house model considers the following four major heat gains/losses that contribute to 
	the building heating/cooling load:

	1.  Conduction through exterior walls, roof and fenestration (based on envelope UA)

	2.  Air infiltration (based on specified air change rate)

	3.  Solar radiation (based on CLTD model and using tmy data)

	4.  Internal gains from lighting, people, equipment and other end use objects.

	The heating/cooling load is calculated as below:

		 Q = U x A x \f$ CLTD_c \f$

		where

		Q = cooling load for roof, wall or fenestration (Btu/h)

		U = overall heat transfer coefficient for roof, wall or fenestration  \f$ (Btu/h.ft^2.degF) \f$

		A = area of wall, roof or fenestration \f$ (ft^2) \f$

		\f$ CLTD_c  \f$ = corrected cooling load temperature difference (degF)


		\f$ CLTD_c = CLTD + LM + (78 - T_{air}) + (T_{out} - 85) \f$

		where

		CLTD = temperature difference based on construction types (taken from ASHRAE Handbook)

		LM = correction for latitude based on tables from ASHRAE Handbook

		\f$ T_{air} \f$ = room air temperature (degF)

		\f$ T_{out} \f$ = average outdoor air temperature on a design day (degF)

	The Equivalent Thermal Parameter (ETP) approach is used to model the residential loads
	and energy consumption.  Solving the ETP model simultaneously for T_{air} and T_{mass},
	the heating/cooling loads can be obtained as a function of time.

	In the current implementation, the HVAC equipment is defined as part of the house and
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
#include "house_a.h"
	
EXPORT CIRCUIT *attach_enduse_house_a(OBJECT *obj, enduse *target, double breaker_amps, int is220)
{
	house *pHouse = 0;
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

	pHouse = OBJECTDATA(obj,house);
	return pHouse->attach(target,breaker_amps,is220);
}

//////////////////////////////////////////////////////////////////////////
// house CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* house::oclass = NULL;
CLASS* house::pclass = NULL;

/** House object constructor:  Registers the class and publishes the variables that can be set by the user. 
Sets default randomized values for published variables.
**/
house::house(MODULE *mod) : residential_enduse(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"house_a",sizeof(house),PC_PRETOPDOWN|PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class house_a";
		else
			oclass->trl = TRL_DEMONSTRATED;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"floor_area[sf]",PADDR(floor_area), PT_DESCRIPTION, "area of the house's ground floor",
			PT_double,"gross_wall_area[sf]",PADDR(gross_wall_area), PT_DESCRIPTION, "total exterior wall area",
			PT_double,"ceiling_height[ft]",PADDR(ceiling_height), PT_DESCRIPTION, "height of the house's ceiling, across all floors",
			PT_double,"aspect_ratio",PADDR(aspect_ratio), PT_DESCRIPTION, "ratio of the length to the width of the house's ground footprint",
			PT_double,"envelope_UA[Btu/degF*h]",PADDR(envelope_UA), PT_DESCRIPTION, "insulative UA value of the house, based on insulation thickness and surface area",
			PT_double,"window_wall_ratio",PADDR(window_wall_ratio), PT_DESCRIPTION, "ratio of exterior window area to exterior wall area",
			PT_double,"glazing_shgc",PADDR(glazing_shgc), PT_DESCRIPTION, "siding glazing solar heat gain coeffient",
			PT_double,"airchange_per_hour",PADDR(airchange_per_hour), PT_DESCRIPTION, "volume of air exchanged between the house and exterior per hour",
			PT_double,"solar_gain[Btu/h]",PADDR(solar_load), PT_DESCRIPTION, "thermal solar input",
			PT_double,"heat_cool_gain[Btu/h]",PADDR(hvac_rated_capacity), PT_DESCRIPTION, "thermal gain rate of the installed HVAC",
			PT_double,"thermostat_deadband[degF]",PADDR(thermostat_deadband), PT_DESCRIPTION, "",
			PT_double,"heating_setpoint[degF]",PADDR(heating_setpoint), PT_DESCRIPTION, "",
			PT_double,"cooling_setpoint[degF]",PADDR(cooling_setpoint), PT_DESCRIPTION, "",
			PT_double, "design_heating_capacity[Btu*h]",PADDR(design_heating_capacity), PT_DESCRIPTION, "",
			PT_double,"design_cooling_capacity[Btu*h]",PADDR(design_cooling_capacity), PT_DESCRIPTION, "",
			PT_double,"heating_COP",PADDR(heating_COP), PT_DESCRIPTION, "",
			PT_double,"cooling_COP",PADDR(cooling_COP), PT_DESCRIPTION, "",
			PT_double,"COP_coeff",PADDR(COP_coeff), PT_DESCRIPTION, "",
			PT_double,"air_temperature[degF]",PADDR(Tair), PT_DESCRIPTION, "",
			PT_double,"outside_temp[degF]",PADDR(Tout), PT_DESCRIPTION, "",
			PT_double,"mass_temperature[degF]",PADDR(Tmaterials), PT_DESCRIPTION, "",
			PT_double,"mass_heat_coeff",PADDR(house_content_heat_transfer_coeff), PT_DESCRIPTION, "",
			PT_double,"outdoor_temperature[degF]",PADDR(outside_temp), PT_DESCRIPTION, "",
			PT_double,"house_thermal_mass[Btu/degF]",PADDR(house_content_thermal_mass), PT_DESCRIPTION, "thermal mass of the house's contained mass",
			PT_enumeration,"heat_mode",PADDR(heat_mode), PT_DESCRIPTION, "",
				PT_KEYWORD,"ELECTRIC",(enumeration)ELECTRIC,
				PT_KEYWORD,"GASHEAT",(enumeration)GASHEAT,
			PT_enumeration,"hc_mode",PADDR(heat_cool_mode), PT_DESCRIPTION, "",
				PT_KEYWORD,"OFF",(enumeration)OFF,
				PT_KEYWORD,"HEAT",(enumeration)HEAT,
				PT_KEYWORD,"COOL",(enumeration)COOL,
			PT_enduse,"houseload",PADDR(tload), PT_DESCRIPTION, "",
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
		gl_publish_function(oclass,	"attach_enduse", (FUNCTIONADDR)&attach_enduse_house_a);		
	}	
}

int house::create() 
{
	int res = residential_enduse::create();

	static char tload_name[32] = "house_total";
	static char load_name[32] = "house_hvac";

	tload.name = tload_name;
	tload.power = complex(0,0,J);
	tload.admittance = complex(0,0,J);
	tload.current = complex(0,0,J);
	load.name = load_name;
	load.power = complex(0,0,J);
	load.admittance = complex(0,0,J);
	load.current = complex(0,0,J);

	tload.end_obj = OBJECTHDR(this);

// initalize published variables.  The model definition can set any of this.
	heat_mode = ELECTRIC;
	floor_area = 0.0;
	ceiling_height = 0.0;
	envelope_UA = 0.0;
	airchange_per_hour = 0.0;
	thermostat_deadband = 0.0;
	heating_setpoint = 0.0;
	cooling_setpoint = 0.0;
	window_wall_ratio = 0.0;
	gross_wall_area = 0.0;
	glazing_shgc = 0.0;
	design_heating_capacity = 0.0;
	design_cooling_capacity = 0.0;
	heating_COP = 3.0;
	cooling_COP = 3.0;
	aspect_ratio = 1.0;
    air_density = 0.0735;			// density of air [lb/cf]
	air_heat_capacity = 0.2402;	// heat capacity of air @ 80F [BTU/lb/F]
	house_content_thermal_mass = 10000.0;		// thermal mass of house [BTU/F]

	// set defaults for panel/meter variables
	panel.circuits=NULL;
	panel.max_amps=200;

	return res;

}

/** Checks for climate object and maps the climate variables to the house object variables.  
Currently Tout, RHout and solar flux data from TMY files are used.  If no climate object is linked,
then Tout will be set to 74 degF, RHout is set to 75% and solar flux will be set to zero for all orientations.
**/
int house::init_climate()
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
			gl_warning("house: no climate data found, using static data");

			//default to mock data
			static double tout=74.0, rhout=0.75, solar[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
			pTout = &tout;
			pRhout = &rhout;
			pSolar = &solar[0];
		}
		else if (climates->hit_count>1)
		{
			gl_warning("house: %d climates found, using first one defined", climates->hit_count);
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
		}
	}
	return 1;
}

/** Map circuit variables to meter.  Initalize house and HVAC model properties,
and internal gain variables.
**/

int house::init(OBJECT *parent)
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
		{&pLine12,				"current_12"}, // maps current load 1-2 onto triplex load
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
				GL_THROW("%s (%s:%d) does not implement triplex_meter variable %s for %s (house:%d)", 
				/*	TROUBLESHOOT
					The House requires that the triplex_meter contains certain published properties in order to properly connect
					the house circuit panel to the meter.  If the triplex_meter does not contain those properties, GridLAB-D may
					suffer fatal pointer errors.  If you encounter this error, please report it to the developers, along with
					the version of GridLAB-D that raised this error.
				*/
					parent->name?parent->name:"unnamed object", parent->oclass->name, parent->id, map[i].varname, obj->name?obj->name:"unnamed", obj->id);
		}
	}
	else
	{
		gl_error("house:%d %s; using static voltages", obj->id, parent==NULL?"has no parent triplex_meter defined":"parent is not a triplex_meter");
		/*	TROUBLESHOOT
			The House model relies on a triplex_meter as a parent to calculate voltages based on
			events within the powerflow module.  Create a triplex_meter object and set it as
			the parent of the house object.
		*/

		// attach meter variables to each circuit in the default_meter
		*(map[0].var) = &default_line_voltage[0];
		*(map[1].var) = &default_line_current[0];
		*(map[2].var) = &default_line_current_12;
	}
		// Set defaults for published variables nor provided by model definition
	while (floor_area <= 500)
		floor_area = gl_random_normal(RNGSTATE,2500,300);		// house size (sf) by 100 ft incs;

	if (ceiling_height <= ROUNDOFF)
		ceiling_height = 8.0;

	if (envelope_UA <= ROUNDOFF)
		envelope_UA = gl_random_uniform(RNGSTATE,0.15,0.2)*floor_area;	// UA of house envelope [BTU/h.F]

	if (aspect_ratio <= ROUNDOFF)
		aspect_ratio = 1.0;

	if (gross_wall_area <= ROUNDOFF)
		gross_wall_area = 4.0 * 2.0 * (aspect_ratio + 1.0) * ceiling_height * sqrt(floor_area/aspect_ratio);

	if (airchange_per_hour <= ROUNDOFF)
		airchange_per_hour = gl_random_uniform(RNGSTATE,4,6);			// air changes per hour [cf/h]

	if (thermostat_deadband <= ROUNDOFF)
		thermostat_deadband = 2;							// thermostat hysteresis [F]

	if (heating_setpoint <= ROUNDOFF)
		heating_setpoint = gl_random_uniform(RNGSTATE,68,72);	// heating setpoint [F]

	if (cooling_setpoint <= ROUNDOFF)
		cooling_setpoint = gl_random_uniform(RNGSTATE,76,80);	// cooling setpoint [F]

	if (window_wall_ratio <= ROUNDOFF)
		window_wall_ratio = 0.15;						// assuming 15% window wall ratio

	if (glazing_shgc <= ROUNDOFF)
		glazing_shgc = 0.65;								// assuming generic double glazing

	if (design_cooling_capacity <= ROUNDOFF)
		design_cooling_capacity = gl_random_uniform(RNGSTATE,18,24); // Btuh/sf

	if (design_heating_capacity <= ROUNDOFF)
		design_heating_capacity = gl_random_uniform(RNGSTATE,18,24); // Btuh/sf
	
	
	// initalize/set hvac model parameters
    if (COP_coeff <= ROUNDOFF)
	    COP_coeff = gl_random_uniform(RNGSTATE,0.9,1.1);	// coefficient of cops [scalar]
    
    if (Tair <= ROUNDOFF)
	    Tair = gl_random_uniform(RNGSTATE,heating_setpoint+thermostat_deadband, cooling_setpoint-thermostat_deadband);	// air temperature [F]
	
    if (over_sizing_factor <= ROUNDOFF)
        over_sizing_factor = gl_random_uniform(RNGSTATE,0.98,1.3);

    heat_cool_mode = house::OFF;							// heating/cooling mode {HEAT, COOL, OFF}

    if (house_content_heat_transfer_coeff <= ROUNDOFF)
	    house_content_heat_transfer_coeff = gl_random_uniform(RNGSTATE,0.5,1.0)*floor_area;	// heat transfer coefficient of house contents [BTU/hr.F]

	//house properties for HVAC
	volume = 8*floor_area;									// volume of air [cf]
	air_mass = air_density*volume;							// mass of air [lb]
	air_thermal_mass = air_heat_capacity*air_mass;			// thermal mass of air [BTU/F]
	Tmaterials = Tair;										// material temperture [F]
	hvac_rated_power = 24*floor_area*over_sizing_factor;	// rated heating/cooling output [BTU/h]

	if (set_Eigen_values() == FALSE)
		return 0;

	if (hdr->latitude < 24 || hdr->latitude > 48)
	{
		/* bind latitudes to [24N, 48N] */
		hdr->latitude = hdr->latitude<24 ? 24 : 48;
		gl_error("Latitude beyond the currently supported range 24 - 48 N, Simulations will continue assuming latitude %.0fN",hdr->latitude);
		/*	TROUBLESHOOT
			GridLAB-D currently only supports latitudes within a temperate band in the northern hemisphere for the building models.
			Latitudes outside 24N to 48N may not correctly calculate solar input.
		*/
	}

	// attach the house HVAC to the panel
	//attach(&load, 50, TRUE);
	hvac_circuit = attach_enduse_house_a(hdr, &load, 50, TRUE);

	return 1;
}

/// Attaches an end-use object to a house panel
/// The attach() method automatically assigns an end-use load
/// to the first appropriate available circuit.
/// @return pointer to the voltage on the assigned circuit
CIRCUIT *house::attach(enduse *pLoad, ///< enduse structure
					   double breaker_amps, ///< breaker capacity (in Amps)
					   int is220///< 0 for 120V, 1 for 240V load
					   ) 
{
	if (pLoad==NULL){
		GL_THROW("end-use load couldn't be connected because it was not provided");
		/*	TROUBLESHOOT
			The house model expects all enduse load models to publish an 'enduse_load' property that points to the top
			of the load aggregator for the appliance.  Please verify that the specified load class publishes an
			'enduse_load' property.
		*/
	}
	
	// construct and id the new circuit
	CIRCUIT *c = new CIRCUIT;
	if (c==NULL)
	{
		GL_THROW("memory allocation failure");

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
	c->pLoad = pLoad;
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

	return c;
}

/**  The PLC control code for house thermostat.  The heat or cool mode is based
on the house air temperature, thermostat setpoints and deadband.
**/
TIMESTAMP house::sync_thermostat(TIMESTAMP t0, TIMESTAMP t1)
{
	const double TcoolOn = cooling_setpoint+thermostat_deadband;
	const double TcoolOff = cooling_setpoint-thermostat_deadband;
	const double TheatOn = heating_setpoint-thermostat_deadband;
	const double TheatOff = heating_setpoint+thermostat_deadband;

	// check for deadband overlap
	if (TcoolOff<TheatOff)
	{
		gl_error("house: thermostat setpoints deadbands overlap (TcoolOff=%.1f < TheatOff=%.1f)", TcoolOff,TheatOff);
		/*	TROUBLESHOOT
			The house models the thermostat with explicit heating and cooling on and off points, dependent on the device
			setpoints and the thermostat's activation deadband.  If the heating and cooling points are too close to each
			other, the device will not emulate a real thermostat.  To remedy this error, either make the thermostat
			deadband smaller, or set the heating and cooling setpoints further apart.
		*/
		return TS_INVALID;
	}

	// change control mode if appropriate
	if (Tair<=TheatOn+0.01) // heating turns on
		heat_cool_mode = HEAT;
	else if (Tair>=TheatOff-0.01 && Tair<=TcoolOff+0.01) // heating/cooling turns off
		heat_cool_mode = OFF;
	else if (Tair>=TcoolOn-0.01) // cooling turns on
		heat_cool_mode = COOL;

	return TS_NEVER;
}

TIMESTAMP house::sync_panel(TIMESTAMP t0, TIMESTAMP t1)
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
		/*	TROUBLESHOOT
			Invalid circuit types are an internal error for the house panel.  Please report this error.  The likely causes
			include an object that is not a house is being processed by the house model, or the panel was not correctly
			initialized.
		*/

		// if breaker is open and reclose time has arrived
		if (c->status==BRK_OPEN && t1>=c->reclose)
		{
			c->status = BRK_CLOSED;
			c->reclose = TS_NEVER;
			sync_time = t1; // must immediately reevaluate devices affected
			gl_debug("house:%d panel breaker %d closed", obj->id, c->id);
		}

		// if breaker is closed
		if (c->status==BRK_CLOSED)
		{
			// compute circuit current
			if ((c->pV)->Mag() == 0)
			{
				gl_debug("house:%d circuit %d (enduse %s) voltage is zero", obj->id, c->id, c->pLoad->name);
				break;
			}
			
			complex current = ~(c->pLoad->total*1000 / *(c->pV)); 

			// check breaker
			if (c->max_amps>0 && current.Mag()>c->max_amps)
			{
				// probability of breaker failure increases over time
				if (c->tripsleft>0 && gl_random_bernoulli(RNGSTATE,1/(c->tripsleft--))==0)
				{
					// breaker opens
					c->status = BRK_OPEN;

					// average five minutes before reclosing, exponentially distributed
					c->reclose = t1 + (TIMESTAMP)(gl_random_exponential(RNGSTATE,1/300.0)*TS_SECOND); 
					gl_debug("house:%d circuit breaker %d tripped - enduse %s overload at %.0f A", obj->id, c->id,
						c->pLoad->name, current.Mag());
				}

				// breaker fails from too frequent operation
				else
				{
					c->status = BRK_FAULT;
					c->reclose = TS_NEVER;
					gl_debug("house:%d circuit breaker %d failed", obj->id, c->id);
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
				tload.energy += c->pLoad->power * gl_tohours(t1-t0);
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


/**  Updates the aggregated power from all end uses, calculates the HVAC kWh use for the next synch time
**/

TIMESTAMP house::presync(TIMESTAMP t0, TIMESTAMP t1) 
{
	if (t0>0 && t1>t0)
		load.energy += load.total.Mag() * gl_tohours(t1-t0);

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


TIMESTAMP house::sync_thermal(TIMESTAMP t1, double nHours){
	DATETIME tv;
	double t = 0.0;
	gl_localtime(t1, &tv);
	
	Tout = *pTout;

	Tsolar = get_Tsolar(tv.hour, tv.month, Tair, *pTout);
	solar_load = 0.0;

	for (int i = 1; i<9; i++)
	{
		solar_load += (gross_wall_area*window_wall_ratio/8.0) * glazing_shgc * pSolar[i];
	}
	double netHeatrate = /*hvac_rated_capacity +*/ tload.heatgain*BTUPHPW + solar_load;
	double Q1 = M_inv11*Tair + M_inv12*Tmaterials;
	double Q2 = M_inv21*Tair + M_inv22*Tmaterials;

	if (nHours > ROUNDOFF)
	{
		double q1 = exp(s1*nHours)*(Q1 + BB11*Tsolar/s1 + BB12*netHeatrate/s1) - BB11*Tsolar/s1 
			- BB12*netHeatrate/s1;
		double q2 = exp(s2*nHours)*(Q2 - BB11*Tsolar/s2 - BB12*netHeatrate/s2) + BB11*Tsolar/s2 
			+ BB12*netHeatrate/s2;

		Tair = q1*(s1-A22)/A21 + q2*(s2-A22)/A21;
		Tmaterials = q1 + q2;
	}
    else
        return TS_NEVER;

	// calculate constants for solving time "t" to reach Tevent
	const double W = (Q1 + (BB11*Tsolar)/s1 + BB12*netHeatrate/s1)*(s1-A22)/A21;
	const double X = (BB11*Tsolar/s1 + BB12*netHeatrate/s1)*(s1-A22)/A21;
	const double Y = (Q2 - (BB11*Tsolar)/s2 - BB12*netHeatrate/s2)*(s2-A22)/A21;
	const double Z = (BB11*Tsolar/s2 + BB12*netHeatrate/s2)*(s2-A22)/A21;
	// end new solution

	// determine next internal event temperature
	int n_solutions = 0;
	double Tevent;
	const double TcoolOn = cooling_setpoint+thermostat_deadband;
	const double TcoolOff = cooling_setpoint-thermostat_deadband;
	const double TheatOn = heating_setpoint-thermostat_deadband;
	const double TheatOff = heating_setpoint+thermostat_deadband;

	/* determine the temperature of the next event */
#define TPREC 0.01
	if (hvac_rated_capacity < 0.0)
		Tevent = TcoolOff;
	else if (hvac_rated_capacity > 0.0)
		Tevent = TheatOff;
	else if (Tair <= TheatOn+TPREC)
		Tevent = TheatOn;
	else if (Tair >= TcoolOn-TPREC)
		Tevent = TcoolOn;
	else
		return TS_NEVER;

#ifdef OLD_SOLVER
    if (nHours > TPREC)
		// int dual_decay_solve(double *ans, double prec, double start, double end, int f, double a, double n, double b, double m, double c)
		n_solutions = dual_decay_solve(&t,TPREC,0.0 ,nHours,W,s1,Y,s2,Z-X-Tevent);

	Tair = Tevent;

	if (n_solutions<0)
		gl_error("house: solver error");
	else if (n_solutions == 0)
		return TS_NEVER;
	else if (t == 0)
		t = 1.0/3600.0;  // one second
	return t1+(TIMESTAMP)(t*3600*TS_SECOND);
#else
	t =  e2solve(W,s1,Y,s2,Z-X-Tevent);
	    Tair = Tevent;

	if (isfinite(t))
    {
		return t1+(TIMESTAMP)(t*3600*TS_SECOND);
    }
	else
		return TS_NEVER;
#endif
}

/** Updates the total internal gain and synchronizes with the hvac equipment load.  
Also synchronizes the voltages and current in the panel with the meter.
**/

TIMESTAMP house::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	double nHours = (gl_tohours(t1)- gl_tohours(t0));
	//load.energy += load.total * nHours;
	/* TIMESTAMP sync_time = */ sync_hvac_load(t1, nHours);

	// sync circuit panel
	TIMESTAMP panel_time = sync_panel(t0,t1);
	TIMESTAMP sync_time = sync_thermal(t1, nHours);

	if (panel_time < sync_time)
		sync_time = panel_time;

	/// @todo check panel main breaker (residential, medium priority) (ticket #140)
	return sync_time;
	
}

/** HVAC load synchronizaion is based on the equipment capacity, COP, solar loads and total internal gain
from end uses.  The modeling approach is based on the Equivalent Thermal Parameter (ETP)
method of calculating the air and mass temperature in the conditioned space.  These are solved using
a dual decay solver to obtain the time for next state change based on the thermostat set points.
This synchronization function updates the HVAC equipment load and power draw.
**/

TIMESTAMP house::sync_hvac_load(TIMESTAMP t1, double nHours)
{
	// compute hvac performance
	outside_temp = *pTout;
	const double heating_cop_adj = (-0.0063*(*pTout)+1.5984);
	const double cooling_cop_adj = -(-0.0108*(*pTout)+2.0389);
	const double heating_capacity_adj = (-0.0063*(*pTout)+1.5984);
	const double cooling_capacity_adj = -(-0.0063*(*pTout)+1.5984);

	double t = 0.0;

	if (heat_cool_mode == HEAT)
	{
		hvac_rated_capacity = design_heating_capacity*floor_area*heating_capacity_adj;
		hvac_rated_power = hvac_rated_capacity/(heating_COP * heating_cop_adj);
	}
	else if (heat_cool_mode == COOL)
	{
		hvac_rated_capacity = design_cooling_capacity*floor_area*cooling_capacity_adj;
		hvac_rated_power = hvac_rated_capacity/(cooling_COP * cooling_cop_adj);
	}
	else
	{
		hvac_rated_capacity = 0.0;
		hvac_rated_power = 0.0;
	}

	gl_enduse_sync(&(residential_enduse::load),t1);
	load.power = hvac_rated_power*KWPBTUPH * ((heat_cool_mode == HEAT) && (heat_mode == GASHEAT) ? 0.01 : 1.0);
	//load.total = load.power;	// calculated by sync_enduse()
	load.heatgain = hvac_rated_capacity;

	//load.heatgain = hvac_rater_power;		/* factored in at netHeatrate */
	//hvac_kWh_use = load.power.Mag()*nHours;  // this updates the energy usage of the elapsed time since last synch
	return TS_NEVER; /* we'll figure that out later */
}

int house::set_Eigen_values()
{
	// The eigen values and constants are calculated once and stored as part of the object
	// based on new solution to house etp network // April 06, 2004

	if (envelope_UA <= ROUNDOFF || house_content_heat_transfer_coeff <= ROUNDOFF || 
		air_thermal_mass <= ROUNDOFF || house_content_thermal_mass <= ROUNDOFF)
	{
		gl_error("House thermal mass or UA invalid.  Eigen values not set.");
		/*	TROUBLESHOOT
			The house envelope_UA, mass_heat_coeff, or floor_area have not been set to legitimate values.
			Please review these properties and set them to values greater than 1e-6.
		*/
		return FALSE;
	}

	double ra = 1/envelope_UA;
	double rm = 1/house_content_heat_transfer_coeff;

	A11 = -1.0/(air_thermal_mass*rm) - 1.0/(ra*air_thermal_mass);
	A12 = 1.0/(rm*air_thermal_mass);
	A21 = 1.0/(rm*house_content_thermal_mass);
	A22 = -1.0/(rm*house_content_thermal_mass);

	B11 = 1.0/(air_thermal_mass*ra);
	B12 = 1.0/air_thermal_mass;
	B21 = 0.0;
	B22 = 0.0;

	/* calculate eigen values */
	s1 = (A11 + A22 + sqrt((A11 + A22)*(A11 + A22) - 4*(A11*A22 - A21*A12)))/2.0;
	s2 = (A11 + A22 - sqrt((A11 + A22)*(A11 + A22) - 4*(A11*A22 - A21*A12)))/2.0;

	/* calculate egien vectors */
	double M11 = (s1 - A22)/A21;
	double M12 = (s2 - A22)/A21;
	double M21 = 1.0;
	double M22 = 1.0;

	double demoninator = (s1 - A22)/A21 - (s2-A22)/A21;
	M_inv11 = 1.0/demoninator;
	M_inv12 = -((s2 - A22)/A21)/demoninator;
	M_inv21 = -1.0/demoninator;
	M_inv22 = ((s1-A22)/A21)/demoninator;

	BB11 = M_inv11*B11 + M_inv12*B21;
	BB21 = M_inv21*B11 + M_inv22*B21;
	BB12 = M_inv11*B12 + M_inv12*B22;
	BB22 = M_inv21*B12 + M_inv22*B22;

	return TRUE;
}
/**  This function calculates the solar air temperature based on the envelope construction,
reflectivity of the color of envelope surface (assumed to be 0.75) and latitude adjustment factor based on time of day.
@return returns the calculated solar air temperature
**/

double house::get_Tsolar(int hour, int month, double Tair, double Tout)
{
	// Wood frame wall CLTD values from ASHRAE 1989 (for sunlighted walls in the north latitude)
	static double CLTD[] = {4.25, 2.75, 1.63, 0.50, -0.50, 3.50, 11.25, 17.88, 22.50, 25.88, 27.88, 29.25, 31.63, 35.13, 38.50, 40.38, 36.88, 28.00, 19.00, 14.00, 11.13, 8.63, 6.25};

	static double LM[4][24] =	{	
		{-1.33, -1.44, -0.89, -1.00, -0.67, -0.44, -0.67, -1.00, -0.89, -1.44, -1.33, -1.22},  // latitude 24
		{-2.89, -1.89, -0.78, -0.44, -0.11, -0.11, -0.11, -0.44, -0.78, -1.89, -2.89, -4.67},  // latitude 32
		{-5.44, -3.22, -1.11, -0.11, 0.22, 0.67, 0.22, -0.11, -1.11, -3.22, -5.44, -6.33},  // latitude 40
		{-7.56, -5.11, -1.78, -0.11, 1.33, 2.00, 1.33, -0.11, -1.78, -5.11, -7.56} // latitude 48
	};

	static double ColorSurface = 0.75;
	static double DR = 15.0;
	double solarTemp = Tair;
	double LMnow = 0.0;
	int LMcol = month-1;

	OBJECT *hdr = OBJECTHDR(this);

	if (hdr->latitude <= 24.0)
		LMnow = LM[0][LMcol];
	else if (hdr->latitude <= 32.)
		LMnow = LM[0][LMcol] + ((LM[1][LMcol]-LM[0][LMcol])*(hdr->latitude-24.0)/12.0);
	else if (hdr->latitude <= 40.)
		LMnow = LM[1][LMcol] + ((LM[2][LMcol]-LM[1][LMcol])*(hdr->latitude-32.0)/12.0);
	else if (hdr->latitude <= 48.)
		LMnow = LM[2][LMcol] + ((LM[3][LMcol]-LM[2][LMcol])*(hdr->latitude-40.0)/12.0);
	else // if (hdr->latitude > 48.0)
		LMnow = LM[3][LMcol];

	solarTemp += (CLTD[hour] + LMnow)*ColorSurface + (78. - Tair) + ((*pTout) - DR/2. - 85.);

	return solarTemp;
}



complex *house::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_house_a(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(house::oclass);
		if (*obj!=NULL)
		{
			house *my = OBJECTDATA(*obj,house);;
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(house_a);
}

EXPORT int init_house_a(OBJECT *obj)
{
	try
	{
		house *my = OBJECTDATA(obj,house);
		my->init_climate();
		return my->init(obj->parent);
	}
	INIT_CATCHALL(house_a);
}

EXPORT TIMESTAMP sync_house_a(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		house *my = OBJECTDATA(obj,house);
		TIMESTAMP t1 = TS_NEVER;
		if (obj->clock <= ROUNDOFF)
			obj->clock = t0;  //set the object clock if it has not been set yet
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
				gl_error("house::sync- invalid pass configuration");
				t1 = TS_INVALID; // serious error in exec.c
		}
		return t1;
	}
	SYNC_CATCHALL(house_a);
}

EXPORT TIMESTAMP plc_house_a(OBJECT *obj, TIMESTAMP t0)
{
	// this will be disabled if a PLC object is attached to the waterheater
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the clock if it has not been set yet

	house *my = OBJECTDATA(obj,house);
	return my->sync_thermostat(obj->clock, t0);
}

/**@}**/
