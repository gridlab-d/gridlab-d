/** $Id: office.cpp 4738 2014-07-03 00:55:39Z dchassin $
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
#include <ctype.h>
#include "gridlabd.h"
#include "solvers.h"
#include "office.h"
#include "../climate/climate.h"

EXPORT_CREATE(office)
EXPORT_INIT(office)
EXPORT_SYNC(office)
EXPORT_PLC(office)

/* module globals */
double office::warn_low_temp = 50.0;
double office::warn_high_temp = 90.0;
bool office::warn_control = true;

CLASS *office::oclass = NULL;
office *office::defaults = NULL;

/* Class registration is only called once to register the class with the core */
office::office(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gld_class::create(module,"office",sizeof(office),PC_PRETOPDOWN|PC_BOTTOMUP|PC_AUTOLOCK); 
		if (oclass==NULL)
			throw "unable to register class office";
		oclass->trl = TRL_DEMONSTRATED;
		defaults = this;

		if (gl_publish_variable(oclass,
			PT_double, "floor_area[sf]", PADDR(zone.design.floor_area),
			
			PT_double, "floor_height[ft]", PADDR(zone.design.floor_height),
			PT_double, "exterior_ua[Btu/degF/h]", PADDR(zone.design.exterior_ua),
			PT_double, "interior_ua[Btu/degF/h]", PADDR(zone.design.interior_ua),
			PT_double, "interior_mass[Btu/degF]", PADDR(zone.design.interior_mass),
			
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
			
			// to be ripped out/updated with new schedule system
			PT_double, "occupancy", PADDR(zone.current.occupancy),
			PT_double, "occupants", PADDR(zone.design.occupants),
			PT_char256, "schedule", PADDR(zone.design.schedule),

			PT_double, "air_temperature[degF]", PADDR(zone.current.air_temperature),
			PT_double, "mass_temperature[degF]", PADDR(zone.current.mass_temperature),
			PT_double, "temperature_change[degF/h]", PADDR(zone.current.temperature_change),
			PT_double, "outdoor_temperature[degF]", PADDR(zone.current.out_temp),

			PT_double, "Qh[Btu/h]", PADDR(Qh),
			PT_double, "Qs[Btu/h]", PADDR(Qs),
			PT_double, "Qi[Btu/h]", PADDR(Qi),
			PT_double, "Qz[Btu/h]", PADDR(Qz),

			/* HVAC loads */
			PT_enumeration, "hvac_mode", PADDR(zone.hvac.mode),
				PT_KEYWORD, "HEAT", HC_HEAT,
				PT_KEYWORD, "AUX", HC_AUX,
				PT_KEYWORD, "COOL", HC_COOL,
				PT_KEYWORD, "ECON", HC_ECON,
				PT_KEYWORD, "VENT", HC_VENT,
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
			PT_double, "hvac.heating.efficiency[pu]", PADDR(zone.hvac.heating.efficiency),
			PT_double, "hvac.heating.cop[pu]", PADDR(zone.hvac.heating.cop),

			/* Lighting loads */
			PT_double, "lights.capacity[kW]", PADDR(zone.lights.capacity),
			PT_double, "lights.fraction[pu]", PADDR(zone.lights.fraction),

			/* Plug loads */
			PT_double, "plugs.capacity[kW]", PADDR(zone.plugs.capacity),
			PT_double, "plugs.fraction[pu]", PADDR(zone.plugs.fraction),

			/* End-use data */
			PT_complex, "demand[kW]", PADDR(zone.total.demand),
			PT_complex, "total_load[kW]", PADDR(zone.total.power),
			PT_complex, "energy[kWh]", PADDR(zone.total.energy),
			PT_double, "power_factor", PADDR(zone.total.power_factor),
			PT_complex, "power[kW]", PADDR(zone.total.constant_power),
			PT_complex, "current[A]", PADDR(zone.total.constant_current),
			PT_complex, "admittance[1/Ohm]", PADDR(zone.total.constant_admittance),
		
			PT_complex, "hvac.demand[kW]", PADDR(zone.hvac.enduse.demand),
			PT_complex, "hvac.load[kW]", PADDR(zone.hvac.enduse.power),
			PT_complex, "hvac.energy[kWh]", PADDR(zone.hvac.enduse.energy),
			PT_double, "hvac.power_factor", PADDR(zone.hvac.enduse.power_factor),

			PT_complex, "lights.demand[kW]", PADDR(zone.lights.enduse.demand),
			PT_complex, "lights.load[kW]", PADDR(zone.lights.enduse.power),
			PT_complex, "lights.energy[kWh]", PADDR(zone.lights.enduse.energy),
			PT_double, "lights.power_factor", PADDR(zone.lights.enduse.power_factor),
			PT_double, "lights.heatgain_fraction", PADDR(zone.lights.enduse.heatgain_fraction),
			PT_double, "lights.heatgain[kW]", PADDR(zone.lights.enduse.heatgain),
			
			PT_complex, "plugs.demand[kW]", PADDR(zone.plugs.enduse.demand),
			PT_complex, "plugs.load[kW]", PADDR(zone.plugs.enduse.power),
			PT_complex, "plugs.energy[kWh]", PADDR(zone.plugs.enduse.energy),
			PT_double, "plugs.power_factor", PADDR(zone.plugs.enduse.power_factor),
			PT_double, "plugs.heatgain_fraction", PADDR(zone.plugs.enduse.heatgain_fraction),
			PT_double, "plugs.heatgain[kW]", PADDR(zone.plugs.enduse.heatgain),

			PT_double, "cooling_setpoint[degF]", PADDR(zone.control.cooling_setpoint),
			PT_double, "heating_setpoint[degF]", PADDR(zone.control.heating_setpoint),
			PT_double, "thermostat_deadband[degF]", PADDR(zone.control.setpoint_deadband),
			PT_double, "control.ventilation_fraction", PADDR(zone.control.ventilation_fraction),
			PT_double, "control.lighting_fraction", PADDR(zone.control.lighting_fraction),
			PT_double, "ACH", PADDR(zone.hvac.minimum_ach),

			NULL)<1) throw("unable to publish properties in "__FILE__);
		memset(defaults,0,sizeof(office));

		/* set default power factors */
		zone.lights.enduse.power_factor = 1.0;
		zone.plugs.enduse.power_factor = 1.0;
		zone.hvac.enduse.power_factor = 1.0;

		/* set default climate to static values */
		static double Tout = 59, RHout=0.75, Solar[9]={0,0,0,0,0,0,0,0,0};
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
		zone.control.ventilation_fraction = 1;
		zone.control.lighting_fraction = 0.5;
	}
}

/* Object creation is called once for each object that is created by the core */
int office::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	return 1; /* return 1 on success, 0 on failure */
}

/* convert a schedule into an occupancy buffer 
   syntax: DAYS,HOURS
   where DAYS is a range of day numbers and HOURS is a range of hours
   e.g.,
	"1-5 8-17;6,7 10,11,13,14" means Mon-Fri 8a-5p, and Sat & Holidays 10a-noon and 1p-3p
   with
	Day 0 = Sunday, etc., day 7 = holiday
	Hour 0 = midnight to 1am
 */
static void occupancy_schedule(char *text, char occupied[24])
{
	char days[8];
	memset(days,0,sizeof(days));

	char hours[27];
	memset(hours,0,sizeof(hours));

	char *p = text;
	char next=-1;
	char start=-1, stop=-1;
	char *target = days;

	for (p=text; true; p++)
	{
		if (*p==';')
		{	/* recursion on the rest of the schedule */
			occupancy_schedule(p+1,occupied);
		}
		if (isdigit(*p))
		{
			if (next==-1) next = 0;
			next += (next*10+(*p-'0'));
			continue;
		}
		else if (*p=='*')
		{
			start = 0; next = -1;
			continue;
		}
		else if (*p==',' || *p==' ' || *p==';' || *p=='\0')
		{
			char n;
			stop = next;
			for (n=start; n<=(stop>=0?stop:(target==days?8:24)); n++)
				target[n]=1;
			if (*p==',')
				continue;
			else if (*p==' ')
			{
				target = hours;
				start = -1; stop = -1; next = -1;
				continue;
			}
			else if (*p==';' || *p=='\0')
			{
				char d,h;
				for (d=0; d<8; d++)
					for (h=0; h<24; h++)
						if (days[d] && hours[h])
							SET_OCCUPIED(d,h);
				if (*p=='\0')
					break;
				else
				{
					start = -1; stop = -1; next = -1;
					continue;
				}
			}
			else 
				throw "office/occupancy_schedule(): invalid parser state";

		}
		else if (*p=='-')
		{
			start = next;
			stop = -1;
			next = -1;
			continue;
		}
		else
			throw "office/occupancy_schedule(): schedule syntax error";
	}
}

/* Object initialization is called once after all object have been created */
int office::init(OBJECT *parent)
{
	double oversize = 1.2; /* oversizing factor */
	update_control_setpoints();
	/* sets up default office parameters if none were passed:  floor height = 9ft; interior mass = 2000 Btu/degF;
	interior/exterior ua = 2 Btu/degF/h; floor area = 4000 sf*/

	if (zone.design.floor_area == 0)
		zone.design.floor_area = 10000;
	if (zone.design.floor_height == 0)
		zone.design.floor_height = 9;
	if (zone.design.interior_mass == 0)
		zone.design.interior_mass = 40000;
	if (zone.design.interior_ua == 0)
		zone.design.interior_ua = 1;
	if (zone.design.exterior_ua == 0)
		zone.design.exterior_ua = .375;

	if (zone.hvac.cooling.design_temperature == 0)
		zone.hvac.cooling.design_temperature = 93; // Pittsburgh, PA
	if (zone.hvac.heating.design_temperature == 0)  
		zone.hvac.heating.design_temperature = -6;  // Pittsburgh, PA
	

	if (zone.hvac.minimum_ach==0)
		zone.hvac.minimum_ach = 1.5;
	if (zone.control.economizer_cutin==0)
		zone.control.economizer_cutin=60;
	if (zone.control.auxiliary_cutin==0)
		zone.control.auxiliary_cutin=2;

	/* schedule */
	if (strcmp(zone.design.schedule,"")==0)
		strcpy(zone.design.schedule,"1-5 8-17"); /* default is unoccupied, MTWRF 8a-5p is occupied */
	occupancy_schedule(zone.design.schedule,occupied);

	/* automatic sizing of HVAC equipment */
	if (zone.hvac.heating.capacity==0)
		zone.hvac.heating.capacity = oversize*(zone.design.exterior_ua*(zone.control.heating_setpoint-zone.hvac.heating.design_temperature) /* envelope */
			- (zone.hvac.heating.design_temperature - zone.control.heating_setpoint) * (0.2402 * 0.0735 * zone.design.floor_height * zone.design.floor_area) * zone.hvac.minimum_ach); /* vent */
	if (zone.hvac.cooling.capacity==0)
		zone.hvac.cooling.capacity = oversize*(-zone.design.exterior_ua*(zone.hvac.cooling.design_temperature-zone.control.cooling_setpoint) /* envelope */
			- (zone.design.window_area[0]+zone.design.window_area[1]+zone.design.window_area[2]+zone.design.window_area[3]+zone.design.window_area[4]
				+zone.design.window_area[5]+zone.design.window_area[6]+zone.design.window_area[7]+zone.design.window_area[8])*100*3.412*zone.design.glazing_coeff /* solar */
			- (zone.hvac.cooling.design_temperature - zone.control.cooling_setpoint) * (0.2402 * 0.0735 * zone.design.floor_height * zone.design.floor_area) * zone.hvac.minimum_ach /* vent */
			- (zone.lights.capacity + zone.plugs.capacity)*3.412); /* lights and plugs */
	if (zone.hvac.cooling.cop==0)
		zone.hvac.cooling.cop=-3;
	if (zone.hvac.heating.cop==0)
		zone.hvac.heating.cop= 1.25;

	OBJECT *hdr = OBJECTHDR(this);

	// link to climate data
	static FINDLIST *climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
	if (climates==NULL)
		gl_warning("office: no climate data found, using static data");
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

	/* sanity check the initial values (no ticket) */
	struct {
		char *desc;
		bool test;
	} map[] = {
		/* list simple tests to be made on data (no ticket) */
		{"floor height is not valid", zone.design.floor_height<=0},
		{"interior mass is not valid", zone.design.interior_mass<=0},
		{"interior UA is not valid", zone.design.interior_ua<=0},
		{"exterior UA is not valid", zone.design.exterior_ua<=0},
		{"floor area is not valid",zone.design.floor_area<=0},
		{"control setpoint deadpoint is invalid", zone.control.setpoint_deadband<=0},
		{"heating and cooling setpoints conflict",TheatOn>=TcoolOff},
		{"cooling capacity is not negative", zone.hvac.cooling.capacity>=0},
		{"heating capacity is not positive", zone.hvac.heating.capacity<=0},
		{"cooling cop is not negative", zone.hvac.cooling.cop>=0},
		{"heating cop is not positive", zone.hvac.heating.cop<=0},
		{"minimum ach is not positive", zone.hvac.minimum_ach<=0},
		{"auxiliary cutin is not positive", zone.control.auxiliary_cutin<=0},
		{"economizer cutin is above cooling setpoint deadband", zone.control.economizer_cutin>=zone.control.cooling_setpoint-zone.control.setpoint_deadband},
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
TIMESTAMP office::presync(TIMESTAMP t1) 
{
	TIMESTAMP t0 = get_clock();
	DATETIME dt;

	/* reset the multizone heat transfer */
	Qz = 0;
	
	/* update out_temp */ 
	zone.current.out_temp = *(zone.current.pTemperature);

	/* get the occupancy mode from the schedule, if any */
	if (t0>0)
	{
		int day;// = gl_getweekday(t0);
		int hour;// = gl_gethour(t0);
		
		gl_localtime(t0, &dt);
		
		day = dt.weekday;
		hour = dt.hour;

		if (zone.design.schedule[0]!='\0' )
			zone.current.occupancy = IS_OCCUPIED(day,hour);
	}
	return TS_NEVER;
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP office::sync(TIMESTAMP t1) 
{
	TIMESTAMP t0 = get_clock();

	/* load calculations */
	update_lighting(t1);
	update_plugs(t1);
	
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
		const double dt = dt1/3600; /* model operates in units of hours */

		/* calculate model update */
		if (c2!=0)
		{
			/* update temperatures */
			const double e1 = k1*exp(r1*dt);
			const double e2 = k2*exp(r2*dt);
			Ti = e1 + e2 + Teq;
			Tm = ((r1-c1)*e1 + (r2-c1)*e2 + c6)/c2 + Teq;

			if (warn_control)
			{
				/* check for air temperature excursion */
				if (Ti<warn_low_temp || Ti>warn_high_temp)
				{
					OBJECT *obj = OBJECTHDR(this);
					DATETIME dt0, dt1;
					gl_localtime(t0,&dt0);
					gl_localtime(t1,&dt1);
					char ts0[64], ts1[64];
					gl_warning("office:%d (%s) air temperature excursion (%.1f degF) at between %s and %s", 
						obj->id, obj->name?obj->name:"anonymous", Ti,
						gl_strtime(&dt0,ts0,sizeof(ts0))?ts0:"UNKNOWN", gl_strtime(&dt1,ts1,sizeof(ts1))?ts1:"UNKNOWN");
				}

				/* check for mass temperature excursion */
				if (Tm<warn_low_temp || Tm>warn_high_temp)
				{
					OBJECT *obj = OBJECTHDR(this);
					DATETIME dt0, dt1;
					gl_localtime(t0,&dt0);
					gl_localtime(t1,&dt1);
					char ts0[64], ts1[64];
					gl_warning("office:%d (%s) mass temperature excursion (%.1f degF) at between %s and %s", 
						obj->id, obj->name?obj->name:"anonymous", Tm,
						gl_strtime(&dt0,ts0,sizeof(ts0))?ts0:"UNKNOWN", gl_strtime(&dt1,ts1,sizeof(ts1))?ts1:"UNKNOWN");
				}
			}

			/* calculate the power consumption */
			zone.total.energy += zone.total.power * dt;
		}

		const double Ca = 0.2402 * 0.0735 * zone.design.floor_height * zone.design.floor_area;

		/* update enduses and get internal heat gains */
		Qi = zone.lights.enduse.heatgain + zone.plugs.enduse.heatgain;

		/* compute solar gains */
		Qs = 0; 
		int i;
		for (i=0; i<9; i++)
			Qs += zone.design.window_area[i] * zone.current.pSolar[i]/10;
		Qs *= 3.412;
		if (Qs<0)
			throw "solar gain is negative?!?";

		/* compute heating/cooling effect */
		Qh = update_hvac(); 

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

		/* calculate power */
		zone.total.power = zone.lights.enduse.power + zone.plugs.enduse.power + zone.hvac.enduse.power;

		if (warn_control)
		{
			/* check for heating equipment sizing problem */
			if ((mode==HC_HEAT || mode==HC_AUX) && Teq<TheatOff)
			{
				OBJECT *obj = OBJECTHDR(this);
				DATETIME dt0, dt1;
				gl_localtime(t0,&dt0);
				gl_localtime(t1,&dt1);
				char ts0[64], ts1[64];
				gl_warning("office:%d (%s) %s heating undersized between %s and %s", 
					obj->id, obj->name?obj->name:"anonymous", mode==HC_HEAT?"primary":"auxiliary", 
					gl_strtime(&dt0,ts0,sizeof(ts0))?ts0:"UNKNOWN", gl_strtime(&dt1,ts1,sizeof(ts1))?ts1:"UNKNOWN");
			}

			/* check for cooling equipment sizing problem */
			else if (mode==HC_COOL && Teq>TcoolOff)
			{
				OBJECT *obj = OBJECTHDR(this);
				DATETIME dt0, dt1;
				gl_localtime(t0,&dt0);
				gl_localtime(t1,&dt1);
				char ts0[64], ts1[64];
				gl_warning("office:%d (%s) cooling undersized between %s and %s", 
					obj->id, obj->name?obj->name:"anonymous", mode==HC_COOL?"COOL":"ECON", 
					gl_strtime(&dt0,ts0,sizeof(ts0))?ts0:"UNKNOWN", gl_strtime(&dt1,ts1,sizeof(ts1))?ts1:"UNKNOWN");
			}

			/* check for economizer control problem */
			else if (mode==HC_ECON && Teq>TcoolOff)
			{
				OBJECT *obj = OBJECTHDR(this);
				DATETIME dt;
				gl_localtime(t1,&dt);
				char ts[64];
				gl_warning("office:%d (%s) insufficient economizer control at %s", 
					obj->id, obj->name?obj->name:"anonymous", gl_strtime(&dt,ts,sizeof(ts))?ts:"UNKNOWN");
			}
		}
	}

	/* determine the temperature of the next event */
	if (Tevent == Teq)
		return -(t1+(TIMESTAMP)(3600*TS_SECOND)); /* soft return not more than an hour */

	/* solve for the time to the next event */
	double dt2=(double)TS_NEVER;
	dt2 = e2solve(k1,r1,k2,r2,Teq-Tevent)*3600;
	if (isnan(dt2) || !isfinite(dt2) || dt2<0)
	{
		if (dTi==0)
			/* never more than an hour because of the occupancy schedule */
			return -(t1+(TIMESTAMP)(3600*TS_SECOND)); /* soft return */

		/* do not allow more than 1 degree/hour temperature change before solving again */
		dt2 = fabs(3600/dTi);
		if (dt2>3600)
			dt2 = 3600; /* never more than an hour because of the occupancy schedule */
		return -(t1+(TIMESTAMP)(dt2*TS_SECOND)); /* soft return */
	}
	if (dt2<TS_SECOND)
		return t1+1; /* need to do a second pass to get next state */
	else
		return t1+(TIMESTAMP)(dt2*TS_SECOND); /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

void office::update_control_setpoints()
{
	TcoolOn = zone.control.cooling_setpoint + zone.control.setpoint_deadband;
	TcoolOff = zone.control.cooling_setpoint - zone.control.setpoint_deadband;
	TheatOn = zone.control.heating_setpoint - zone.control.setpoint_deadband;
	TheatOff = zone.control.heating_setpoint + zone.control.setpoint_deadband;
	if (TcoolOff - TheatOff <= 0) // deadband needs to be smaller/ setpoints need to be farther apart
		throw (char *)"thermostat deadband causes heating/cooling turn-off points to overlap";
	zone.control.ventilation_fraction = zone.current.occupancy>0 ? zone.hvac.minimum_ach : 0;
}

TIMESTAMP office::update_lighting(TIMESTAMP t1)
{
	TIMESTAMP t0 = get_clock();

	// power calculation
	zone.lights.enduse.power.SetPowerFactor(zone.lights.capacity *
		zone.lights.fraction, zone.lights.enduse.power_factor, J);
	// energy calculation
	if (t0>0 && t1>t0)
		zone.lights.enduse.energy += zone.lights.enduse.power * gl_tohours(t1-t0);
	// heatgain calculation
	zone.lights.enduse.heatgain = zone.lights.enduse.power.Mag() *
		zone.lights.enduse.heatgain_fraction;
	return TS_NEVER;
}

TIMESTAMP office::update_plugs(TIMESTAMP t1)
{
	TIMESTAMP t0 = get_clock();

	//power calculation
	zone.plugs.enduse.power.SetPowerFactor(zone.plugs.capacity * 
		zone.plugs.fraction, zone.plugs.enduse.power_factor, J);
	// energy calculation
	if (t0>0 && t1>t0)
		zone.plugs.enduse.energy += zone.plugs.enduse.power * gl_tohours(t1-t0);
	// heatgain calculation 
	zone.plugs.enduse.heatgain = zone.plugs.enduse.power.Mag() *
		zone.plugs.enduse.heatgain_fraction;
	return TS_NEVER;
}

double office::update_hvac()
{
	const double &Ti = (zone.current.air_temperature);
	const double &dTi = (zone.current.temperature_change);
	const double &Tout = (*(zone.current.pTemperature));
	const double Trange = 40;	/* range over which HP works in heating mode */
	const double Taux = zone.hvac.heating.balance_temperature-Trange;
	const double &Tecon = zone.hvac.cooling.balance_temperature;
	const double &TbalHeat = zone.hvac.heating.balance_temperature;
	const double &TmaxCool = zone.hvac.cooling.design_temperature;
	HCMODE &mode = zone.hvac.mode;

	/* active/passive heat gain/loss */
	double Qvent = 0;
	double Qactive = 0;

	switch (mode) {
	case HC_OFF: /* system is off */
		cop = 0;
		Qactive = Qvent = 0;
		if (dTi<0 && Ti<TcoolOn)
			Tevent = TheatOn;
		else if (dTi>0 && Ti>TheatOn)
			Tevent = TcoolOn;
		else
			Tevent = Teq;
		break;
	case HC_HEAT: /* system is heating normally */
		cop = 1.0 + (zone.hvac.heating.cop-1)*(Tout-Taux)/Trange;
		Qactive = zone.hvac.heating.capacity + zone.hvac.heating.capacity_perF*(zone.hvac.heating.balance_temperature-Tout);
		Qvent = ((*zone.current.pTemperature) - zone.current.air_temperature) * (0.2402 * 0.0735 * zone.design.floor_height * zone.design.floor_area) * zone.control.ventilation_fraction;
		Tevent = TheatOff;
		break;
	case HC_AUX: /* system is heating aggressively */
		cop = 1.0;
		Qactive = zone.hvac.heating.capacity;
		Qvent = ((*zone.current.pTemperature) - zone.current.air_temperature) * (0.2402 * 0.0735 * zone.design.floor_height * zone.design.floor_area) * zone.control.ventilation_fraction;
		Tevent = TheatOff;
		break;
	case HC_COOL: /* system is actively cooling */
		cop = -1.0 - (zone.hvac.cooling.cop+1)*(Tout-TmaxCool)/(TmaxCool-Tecon);
		Qactive = zone.hvac.cooling.capacity - zone.hvac.cooling.capacity_perF*(Tout-zone.hvac.cooling.balance_temperature);
		Qvent = ((*zone.current.pTemperature) - zone.current.air_temperature) * (0.2402 * 0.0735 * zone.design.floor_height * zone.design.floor_area) * zone.control.ventilation_fraction;
		Tevent = TcoolOff;
		break;
	case HC_VENT: /* system is ventilation (occupied but floating) */
		cop = 0;
		Qactive = 0;
		Qvent = ((*zone.current.pTemperature) - zone.current.air_temperature) * (0.2402 * 0.0735 * zone.design.floor_height * zone.design.floor_area) * zone.control.ventilation_fraction;
		if (dTi<0 && Ti<TcoolOn)
			Tevent = TheatOn;
		else if (dTi>0 && Ti>TheatOn)
			Tevent = TcoolOn;
		else
			Tevent = Teq;
		break;
	case HC_ECON: /* system is passively cooling */
		cop = 0.0;
		Qactive = 0;
		Qvent = ((*zone.current.pTemperature) - zone.current.air_temperature) * (0.2402 * 0.0735 * zone.design.floor_height * zone.design.floor_area) * zone.control.ventilation_fraction;
		Tevent = TcoolOff;
		break;
	default:
		throw "hvac mode is invalid";
		break;
	}

	/* calculate active system power */
	if (Qactive!=0)
		zone.hvac.enduse.power.SetPowerFactor(Qactive/cop/1000,zone.hvac.enduse.power_factor);
	else
		zone.hvac.enduse.power = complex(0,0);

	/* add fan power */
	if (Qvent!=0)
		zone.hvac.enduse.power += complex(.1,-0.01)/1000 * zone.design.floor_area; /* ~ 1 W/sf */

	zone.hvac.enduse.energy += zone.hvac.enduse.power; 
	if (zone.hvac.enduse.power.Re()<0)
		throw "hvac unit is generating electricity";
	else if (!isfinite(zone.hvac.enduse.power.Re()) || !isfinite(zone.hvac.enduse.power.Im()))
		throw "hvac power is not finite";
	return Qvent + Qactive;
}

TIMESTAMP office::plc(TIMESTAMP t1)
{
	TIMESTAMP t0 = get_clock();

	const double &Tout = *(zone.current.pTemperature);
	const double &Tair = zone.current.air_temperature;
	const double &Tmass = zone.current.mass_temperature;
	const double &Tecon = zone.control.economizer_cutin;
	const double &Taux = zone.control.heating_setpoint-zone.control.auxiliary_cutin;
	const double &MinAch = zone.hvac.minimum_ach;
	HCMODE &mode = zone.hvac.mode;
	double &vent = zone.control.ventilation_fraction;

	/* compute the new control temperature */
	update_control_setpoints();

	if (Tair>TheatOff && Tair<TcoolOff )  // enter VENT/OFF mode
		{
		if (vent>0)
			mode = HC_VENT;
		else

		{
			mode = HC_OFF;
		}
	}	
	else if (Tair<=TheatOn || (mode == HC_AUX || mode == HC_HEAT))  // enter HEAT/AUX mode
	{
		if (Tair<=Taux)
			mode = HC_AUX;
		else
			mode = HC_HEAT;
	}
	else if (Tair>=TcoolOn || (mode == HC_ECON || mode == HC_COOL)) // enter ECON/COOL mode
	{
		if (Tout<Tecon)
		{
			/* compute ventilation needed to meet cooling demand */
			double Qgain = Qs + Qi - zone.design.exterior_ua*(Tair - Tout) - zone.design.interior_ua*(Tair-Tmass);
			vent = Qgain / ((Tair - Tout) * (0.2402 * 0.0735 * zone.design.floor_height * zone.design.floor_area)); 
			if (vent<MinAch)
			{
				vent = MinAch;
				mode = HC_ECON;
			}
			else if (vent>5.0)
			{
				if (Tout>Tair) /* no free cooling */
					vent = MinAch;
				mode = HC_COOL;
			}
			else
			{
				mode = HC_ECON;
			}
		}
		else
			mode = HC_COOL;
	}

	return TS_NEVER;
}

/**@}*/
