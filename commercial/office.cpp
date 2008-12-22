/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file office.cpp
	@defgroup office Single-zone office building
	@ingroup commercial

	The building simultion uses a single zone ETP model with first order ODEs:

	\f[ T'_i = \frac{1}{c_a} \left[ T_m U_m - T_i (U_a+U_m) + T_o U_a + \Sigma Q_x \right] \f]

	\f[ T'_m = \frac{U_m}{c_m} \left[ T_i - T_m \right] \f]

 where:
 - \f$T_i\f$ is the temperature of the air inside the building
 - \f$T'_i\f$ is \f$\frac{d}{dt}T_i\f$
 - \f$T_m\f$ is the temperature of the mass inside the buildnig (e.g, furniture, inside walls etc)
 - \f$T'_m\f$ is \f$\frac{d}{dt}T_m\f$
 - \f$T_o\f$ is the ambient temperature outside air
 - \f$U_a\f$ is the UA of the building itself
 - \f$U_m\f$ is the UA of the mass of the furniture, inside walls etc
 - \f$c_m\f$ is the heat capacity of the mass of the furniture inside the walls etc
 - \f$c_a\f$ is the heat capacity of the air inside the building
 - \f$Q_i\f$ is the heat rate from internal heat gains of the building (e.g., plugs, lights, people)
 - \f$Q_h\f$ is the heat rate from HVAC unit
 - \f$Q_s\f$ is the heat rate from the sun (solar heating through windows etc)

 General first order ODEs (with \f$C_1 - C_5\f$ defined by inspection above):
                  
    \f[ T'_i = T_i C_1 + T_m C_2 + C_3 \f]
    \f[ T'_m = T_i C_4 + T_m C_5 \f]

 where
	- \f$ C_1 = - (U_a+U_m) / c_a \f$
	- \f$ C_2 = U_m / c_a \f$
	- \f$ C_3 = (\Sigma Q_x + U_a T_o) / c_a \f$
	- \f$ C_4 = U_m / c_m \f$
	- \f$ C_5 = - U_m / c_m \f$

 General form of second order ODE

    \f[ p_4 = p_1 T"_i + p_2 T'_i + p_3 T_i \f]

 where

   - \f$ p_1 = 1 / C_2 \f$
   - \f$ p_2 = -C_5 / C_2 - C_1 / C_2 \f$
   - \f$ p_3 = C_5 C_1 / C_2 - C_4 \f$
   - \f$ p_4 = -C_5 C_3 / C_2\f$

 Solution to second order ODEs for indoor and mass temperatures are

    \f[ T_i(t) = K_1 e^{r_1 t} +  K_1 e^{r_2 t} + \frac{p4}{p3} \f]

	\f[ T_m(t) = \frac{T'_i(t) - C_1 T_i(t) - C_3}{C_2} \f]

 where:

   - \f$ r_1,r_2 = roots(p_1 p_2 p_3) \f$
   - \f$ K_1 = \frac{r_1 T_{i,0} - r_1 \frac{p_4}{p_3} - T'_{i,0}}{r_2-r_1} \f$
   - \f$ K_2 = \frac{T_{i,0} - r_2 K_2}{r_1} \f$
   - \f$t\f$ is the elapsed time
   - \f$T_i(t)\f$ is the temperature of the air inside the building at time \f$t\f$
   - \f$T_{i,0}\f$ is \f$T_i(t=0)\f$, e.g initial temperature of the air inside the building
   - \f$T'_{i,0}\f$ is \f$T'_i(t=0)\f$ e.g the initial temperature gradient of the air inside the building

 @{
 **/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"
#include "solvers.h"
#include "office.h"
#include "../climate/climate.h"

/* the object class definition */
CLASS *office::oclass = NULL;

/* the static default object properties */
office *office::defaults = NULL;

/* specify passes that are needed */
static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_BOTTOMUP;

/* specify pass that advances the clock  */
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
office::office(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"office",sizeof(office),passconfig); 
		if (gl_publish_variable(oclass,
			PT_double, "occupancy", PADDR(zone.current.occupancy),
			PT_double, "floor_height[ft]", PADDR(zone.design.floor_height),
			PT_double, "exterior_ua[Btu/degF/h]", PADDR(zone.design.exterior_ua),
			PT_double, "interior_ua[Btu/degF/h]", PADDR(zone.design.interior_ua),
			PT_double, "interior_mass[Btu/degF]", PADDR(zone.design.interior_mass),
			PT_double, "floor_area[sf]", PADDR(zone.design.floor_area),
			PT_double, "glazing[sf]", PADDR(zone.design.window_area[0]), PT_SIZE, sizeof(zone.design.window_area)/sizeof(zone.design.window_area[0]),
			PT_double, "glazing.north[sf]", PADDR(zone.design.window_area[CP_N]),
			PT_double, "glazing.northeast[sf]", PADDR(zone.design.window_area[CP_NE]),
			PT_double, "glazing.east[sf]", PADDR(zone.design.window_area[CP_E]),
			PT_double, "glazing.southeast[sf]", PADDR(zone.design.window_area[CP_SE]),
			PT_double, "glazing.south[sf]", PADDR(zone.design.window_area[CP_S]),
			PT_double, "glazing.southwest[sf]", PADDR(zone.design.window_area[CP_SW]),
			PT_double, "glazing.west[sf]", PADDR(zone.design.window_area[CP_W]),
			PT_double, "glazing.northwest[sf]", PADDR(zone.design.window_area[CP_NW]),
			PT_double, "glazing.horizontal[sf]", PADDR(zone.design.window_area[CP_H]),
			PT_double, "glazing.coefficient[pu]", PADDR(zone.design.glazing_coeff),

			PT_double, "occupants", PADDR(zone.design.occupants),
			PT_char256, "schedule", PADDR(zone.design.schedule),

			PT_double, "air_temperature[degF]", PADDR(zone.current.air_temperature),
			PT_double, "mass_temperature[degF]", PADDR(zone.current.mass_temperature),
			PT_double, "temperature_change[degF/h]", PADDR(zone.current.temperature_change),
			PT_double, "Qh[Btu/h]", PADDR(Qh),
			PT_double, "Qs[Btu/h]", PADDR(Qs),
			PT_double, "Qi[Btu/h]", PADDR(Qi),
			PT_double, "Qz[Btu/h]", PADDR(Qz),

			/* HVAC loads */
			PT_enumeration, "hvac.mode", PADDR(zone.hvac.mode),
				PT_KEYWORD, "HEAT", HC_HEAT,
				PT_KEYWORD, "AUX", HC_AUX,
				PT_KEYWORD, "COOL", HC_COOL,
				PT_KEYWORD, "ECON", HC_ECON,
				PT_KEYWORD, "OFF", HC_OFF,
			PT_double, "hvac.cooling.balance_temperature[degF]", PADDR(zone.hvac.cooling.balance_temperature),
			PT_double, "hvac.cooling.capacity[Btu/h]", PADDR(zone.hvac.cooling.capacity),
			PT_double, "hvac.cooling.capacity_perF[Btu/degF/h]",PADDR(zone.hvac.cooling.capacity_perF),
			PT_double, "hvac.cooling.design_temperature[degF]", PADDR(zone.hvac.cooling.design_temperature),
			PT_double, "hvac.cooling.efficiency[pu]",PADDR(zone.hvac.cooling.efficiency),
			PT_double, "hvac.cooling.cop[pu]", PADDR(zone.hvac.cooling.cop),
			PT_double, "hvac.heating.balance_temperature[degF]",PADDR(zone.hvac.heating.balance_temperature),
			PT_double, "hvac.heating.capacity[Btu/h]",PADDR(zone.hvac.heating.capacity),
			PT_double, "hvac.heating.capacity_perF[Btu/degF/h]", PADDR(zone.hvac.heating.capacity_perF),
			PT_double, "hvac.heating.design_temperature[degF]", PADDR(zone.hvac.heating.design_temperature),
			PT_double, "hvac.heating.efficiency[pu]", PADDR(zone.hvac.heating.design_temperature),
			PT_double, "hvac.heating.cop[pu]", PADDR(zone.hvac.heating.cop),

			/* Lighting loads */
			PT_double, "lights.capacity[kW]", PADDR(zone.lights.capacity),
			PT_double, "lights.fraction[pu]", PADDR(zone.lights.fraction),

			/* Plug loads */
			PT_double, "plugs.capacity[kW]", PADDR(zone.plugs.capacity),
			PT_double, "plugs.fraction[pu]", PADDR(zone.plugs.fraction),

			/* End-use data */
			PT_complex, "demand[kW]", PADDR(zone.total.demand),
			PT_complex, "power[kW]", PADDR(zone.total.power),
			PT_complex, "energy", PADDR(zone.total.energy),
			PT_double, "power_factor", PADDR(zone.total.power_factor),

			PT_complex, "hvac.demand[kW]", PADDR(zone.hvac.enduse.demand),
			PT_complex, "hvac.power[kW]", PADDR(zone.hvac.enduse.power),
			PT_complex, "hvac.energy", PADDR(zone.hvac.enduse.energy),
			PT_double, "hvac.power_factor", PADDR(zone.hvac.enduse.power_factor),

			PT_complex, "lights.demand[kW]", PADDR(zone.lights.enduse.demand),
			PT_complex, "lights.power[kW]", PADDR(zone.lights.enduse.power),
			PT_complex, "lights.energy", PADDR(zone.lights.enduse.energy),
			PT_double, "lights.power_factor", PADDR(zone.lights.enduse.power_factor),

			PT_complex, "plugs.demand[kW]", PADDR(zone.plugs.enduse.demand),
			PT_complex, "plugs.power[kW]", PADDR(zone.plugs.enduse.power),
			PT_complex, "plugs.energy", PADDR(zone.plugs.enduse.energy),
			PT_double, "plugs.power_factor", PADDR(zone.plugs.enduse.power_factor),

			PT_double, "control.cooling_setpoint", PADDR(zone.control.cooling_setpoint),
			PT_double, "control.heating_setpoint", PADDR(zone.control.heating_setpoint),
			PT_double, "control.setpoint_deadband", PADDR(zone.control.setpoint_deadband),
			PT_double, "control.ventilation_fraction", PADDR(zone.control.ventilation_fraction),
			PT_double, "control.lighting_fraction", PADDR(zone.control.lighting_fraction),
			
			NULL)<1) throw("unable to publish properties in "__FILE__);
		defaults = this;
		memset(defaults,0,sizeof(office));

		/* set default power factors */
		zone.lights.enduse.power_factor = 1.0;
		zone.plugs.enduse.power_factor = 1.0;
		zone.hvac.enduse.power_factor = 1.0;

		/* set default climate to static values */
		static double Tout = 59, RHout=0.75, Solar[9]={1, 0.9,0.9, 0.5,0.5, 0,0, 0,1};
		zone.current.pTemperature = &Tout;
		zone.current.pHumidity = &RHout;
		zone.current.pSolar = Solar;

		/* set default thermal conditions */
		zone.current.air_temperature = Tout;
		zone.current.mass_temperature = Tout;

		/* set default control strategy */
		zone.control.heating_setpoint = 70;
		zone.control.cooling_setpoint = 75;
		zone.control.setpoint_deadband = 1;
		zone.control.ventilation_fraction = 0.2;
		zone.control.lighting_fraction = 0.5;
	}
}

/* Object creation is called once for each object that is created by the core */
int office::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	/** @todo set the static initial value of properties (no ticket) */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int office::init(OBJECT *parent)
{
	update_control_setpoints();

	/** @todo set the dynamic initial value of properties (no ticket) */

	/* automatic sizing of equipment */
	if (zone.hvac.heating.capacity==0)
		zone.hvac.heating.capacity = zone.design.exterior_ua*(zone.control.heating_setpoint-zone.hvac.heating.design_temperature);
	if (zone.hvac.cooling.capacity==0)
		zone.hvac.cooling.capacity = -zone.design.exterior_ua*(zone.hvac.cooling.design_temperature-zone.control.cooling_setpoint) /* losses */
			- (zone.design.window_area[0]+zone.design.window_area[1]+zone.design.window_area[2]+zone.design.window_area[3]+zone.design.window_area[4]
				+zone.design.window_area[5]+zone.design.window_area[6]+zone.design.window_area[7]+zone.design.window_area[8])*100*3.412*zone.design.glazing_coeff /* solar */
			- (zone.lights.capacity + zone.plugs.capacity)*3.412;
	if (zone.hvac.cooling.cop==0)
		zone.hvac.cooling.cop=-3.5;
	if (zone.hvac.heating.cop==0)
		zone.hvac.heating.cop=1.5;

	/** @todo link climate data (no ticket) */
	OBJECT *hdr = OBJECTHDR(this);

	// link to climate data
	static FINDLIST *climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
	if (climates==NULL)
		gl_warning("house: no climate data found, using static data");
	else if (climates->hit_count>1)
		gl_warning("house: %d climates found, using first one defined", climates->hit_count);
	if (climates->hit_count>0)
	{
		OBJECT *obj = gl_find_next(climates,NULL);
		if (obj->rank<=hdr->rank)
			gl_set_dependent(obj,hdr);
		zone.current.pTemperature = (double*)GETADDR(obj,gl_get_property(obj,"temperature"));
		zone.current.pHumidity = (double*)GETADDR(obj,gl_get_property(obj,"humidity"));
		zone.current.pSolar = (double*)GETADDR(obj,gl_get_property(obj,"solar_flux"));
	}

	/** @todo sanity check the initial values (no ticket) */
	struct {
		char *desc;
		bool test;
	} map[] = {
		/** @todo list simple tests to be made on data (no ticket) */
		{"floor height is not valid", zone.design.floor_height<=0},
		{"interior mass is not valid", zone.design.interior_mass<=0},
		{"interior UA is not valid", zone.design.interior_ua<=0},
		{"exterior UA is not valid", zone.design.exterior_ua<=0},
		{"floor area is not valid",zone.design.floor_area<=0},
		{"control setpoint deadpoint is invalid", zone.control.setpoint_deadband<=0},
		{"heating and cooling setpoints conflict",TheatOn>=TcoolOff},
		{"heating balance temperature is not greater than heating design temperature",zone.hvac.heating.design_temperature>=zone.hvac.heating.balance_temperature},
		{"cooling balance temperature is not less than cooling design temperature",zone.hvac.cooling.design_temperature<=zone.hvac.cooling.balance_temperature},
		{"cooling capacity is not negative", zone.hvac.cooling.capacity>=0},
		{"heating capacity is not positive", zone.hvac.heating.capacity<=0},
		{"cooling cop is not negative", zone.hvac.cooling.cop>=0},
		{"heating cop is not positive", zone.hvac.heating.cop<=0},
	};
	int i;
	for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
	{
		if (map[i].test)
			throw map[i].desc;
	}
	return 1; /* return 1 on success, 0 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP office::presync(TIMESTAMP t0, TIMESTAMP t1) 
{
	/* reset the multizone heat transfer */
	Qz = 0;
	return TS_NEVER;
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP office::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	/* local aliases */
	const double &Tout = (*(zone.current.pTemperature));
	const double &Ua = (zone.design.exterior_ua);
	const double &Cm = (zone.design.interior_mass);
	const double &Um = (zone.design.interior_ua);
	double &Ti = (zone.current.air_temperature);
	double &dTi = (zone.current.temperature_change);
	double &Tm = (zone.current.mass_temperature);
	HCMODE &mode = (zone.hvac.mode);

	/* advance the thermal state of the building */
	const double dt1 = t0>0 ? (double)(t1-t0)*TS_SECOND : 0;
	if (dt1>0)
	{
		const double Ca = 0.2402 * 0.0735 * zone.design.floor_height * zone.design.floor_area;
		const double dt = dt1/3600;

		/* update enduses and get internal heat gains */
		Qi = update_lighting(dt) + update_plugs(dt);

		/* compute heating/cooling effect */
		Qh = update_hvac(dt); 

		/** @todo compute solar gains (no ticket) */
		Qs = 0; 
		int i;
		for (i=0; i<9; i++)
			Qs += 3.412 * zone.design.window_area[i] * zone.current.pSolar[i]/10;
		Qs *= dt;
		if (Qs<0)
			throw "solar gain is negative?!?";

		if (Ca<=0)
			throw "Ca must be positive";
		if (Cm<=0)
			throw "Cm must be positive";

		// split gains to air and mass
		double Qa = Qh + 0.5*Qi + 0.5*Qs;
		double Qm = 0.5*Qi + 0.5*Qs;

		double c3 = (Qa + Tout*Ua)/Ca;
		double c6 = Qm/Cm;
		double c7 = Qa/Ca;
		double c1 = -(Ua + Um)/Ca;
		double c2 = Um/Ca;
		double p1 = 1/c2;
		if (Cm<=0)
			throw "Cm must be positive";
		double c4 = Um/Cm;
		double c5 = -c4;
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
	
		/* compute initial condition */
		dTi = c2*Tm + c1*Ti - (c1+c2)*Tout + c7;
		k1 = (r2*Ti - r2*Teq - dTi)/(r2-r1);
		k2 = (dTi - r1*k1)/r2;

		/* calculate the power consumption */
		zone.total.power = zone.lights.enduse.power + zone.plugs.enduse.power + zone.hvac.enduse.power;
		zone.total.energy = zone.total.power * dt;
	}

	/* determine the temperature of the next event */
#define TPREC 0.01
	unsigned int Nevents = 0;
	double dt2=(double)TS_NEVER;
	Tevent = Teq;
	if (Qh<0) /* cooling active */
		Tevent = TcoolOff;
	else if (Qh>0) /* heating active */
		Tevent = TheatOff;
	else if (Teq<=TheatOn+TPREC) /* heating deadband */
		Tevent = TheatOn;
	else if (Teq>=TcoolOn-TPREC) /* cooling deadband */
		Tevent = TcoolOn;
	else /* equilibrium */
		return TS_NEVER;

	/* solve for the time to the next event */
	dt2 = e2solve(k1,r1,k2,r2,Teq-Tevent);
	if (isnan(dt2) || !isfinite(dt2) || dt2<=0)
		return TS_NEVER;
	else
		return t1+(TIMESTAMP)(dt2*3600*TS_SECOND); /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

void office::update_control_setpoints()
{
	TcoolOn = zone.control.cooling_setpoint + zone.control.setpoint_deadband;
	TcoolOff = zone.control.cooling_setpoint - zone.control.setpoint_deadband;
	TheatOn = zone.control.heating_setpoint - zone.control.setpoint_deadband;
	TheatOff = zone.control.heating_setpoint + zone.control.setpoint_deadband;
}

double office::update_lighting(double dt)
{
	/** @todo compute lighting enduse (no ticket) */
	return zone.lights.enduse.power.Mag() * dt;
}

double office::update_plugs(double dt)
{
	/** @todo compute plugs enduse (no ticket) */
	return zone.plugs.enduse.power.Mag() * dt;
}

double office::update_hvac(double dt)
{
	const double &Tout = (*(zone.current.pTemperature));
	const double Trange = 40;	/* range over which HP works in heating mode */
	const double Taux = zone.hvac.heating.balance_temperature-Trange;
	const double &Tecon = zone.hvac.cooling.balance_temperature;
	const double &TbalHeat = zone.hvac.heating.balance_temperature;
	const double &TmaxCool = zone.hvac.cooling.design_temperature;
	HCMODE &mode = zone.hvac.mode;

	switch (mode) {
	case HC_OFF:
		cop = 0;
		Qrated = 0;
		break;
	case HC_HEAT:
		cop = 1.0 + (zone.hvac.heating.cop-1)*(Tout-Taux)/Trange;
		Qrated = zone.hvac.heating.capacity + zone.hvac.heating.capacity_perF*(zone.hvac.heating.balance_temperature-Tout);
		break;
	case HC_AUX:
		cop = 1.0;
		Qrated = zone.hvac.heating.capacity;
		break;
	case HC_COOL:
		cop = -1.0 - (zone.hvac.cooling.cop+1)*(Tout-TmaxCool)/(TmaxCool-Tecon);
		Qrated = zone.hvac.cooling.capacity - zone.hvac.cooling.capacity_perF*(Tout-zone.hvac.cooling.balance_temperature);
		break;
	case HC_ECON:
		cop = 0.0;
		Qrated = zone.hvac.cooling.capacity;
		break;
	default:
		throw "hvac mode is invalid";
		break;
	}
	/** @todo convert to power (no ticket) */
	if (cop!=0)
		zone.hvac.enduse.power.SetPowerFactor(Qrated/cop/1000,zone.hvac.enduse.power_factor);
	else
		zone.hvac.enduse.power = complex(0,0);
	if (zone.hvac.enduse.power.Re()<0)
		throw "hvac unit is generating electricity!?!";
	else if (!isfinite(zone.hvac.enduse.power.Re()) || !isfinite(zone.hvac.enduse.power.Im()))
		throw "hvac power is not finite!?!";
	return Qrated*dt;
}

TIMESTAMP office::plc(TIMESTAMP t0, TIMESTAMP t1)
{
#define Tout (*(zone.current.pTemperature))
#define Ti (zone.current.air_temperature)
#define DUTY_CYCLE 0.5 /** @todo compute duty cycle properly (no ticket) */
#define mode (zone.hvac.mode)
#define Taux (zone.hvac.heating.balance_temperature-40)
#define Tecon (zone.hvac.cooling.balance_temperature)
#define DELAY (TS_SECOND*120)
	TIMESTAMP t2=TS_NEVER;

	/* compute the new control temperature */
	update_control_setpoints();

	if (Ti<TheatOn) 
	{
		if (Tout<=Taux)
			mode = HC_AUX;
		else
			mode = HC_HEAT;
	}
	else if (Ti>TheatOff && Ti<TcoolOff) 
		mode = HC_OFF;
	else if (Ti>TcoolOn) 
	{
		if (Tout<=Tecon)
			mode = HC_ECON;
		else
			mode = HC_COOL;
	}

	return TS_NEVER;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_office(OBJECT **obj, OBJECT *parent) 
{
	try {
		*obj = gl_create_object(office::oclass);
		if (*obj!=NULL)
		{
			office *my = OBJECTDATA(*obj,office);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		return 0;
	} catch (char *msg) {
		gl_error("create_office: %s", msg);
		return 0;
	}
}

EXPORT int init_office(OBJECT *obj, OBJECT *parent) 
{
	try {
		if (obj!=NULL)
			return OBJECTDATA(obj,office)->init(parent);
		return 0;
	} catch (char *msg) {
		gl_error("init_%s(obj=%d;%s): %s", obj->oclass->name, obj->id, obj->name?obj->name:"unnamed", msg);
		return 0;
	}
}

EXPORT TIMESTAMP sync_office(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	try {
		TIMESTAMP t2 = TS_NEVER;
		office *my = OBJECTDATA(obj,office);
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		default:
			throw("invalid pass request");
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;		
		return t2;
	} catch (char *msg) {
		gl_error("sync_office(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return TS_INVALID; /* halt the clock */
	}
}

EXPORT TIMESTAMP plc_office(OBJECT *obj, TIMESTAMP t1)
{
	try {
		return OBJECTDATA(obj,office)->plc(obj->clock,t1);
	} catch (char *msg) {
		gl_error("plc_%s(obj=%d;%s): %s", obj->oclass->name, obj->id, obj->name?obj->name:"unnamed", msg);
		return TS_INVALID;
	}
}

/**@}*/
