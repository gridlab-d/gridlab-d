/** $Id: house_e.cpp 4738 2014-07-03 00:55:39Z dchassin $
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
#include "complex.h"

#ifndef WIN32
char *_strlwr(char *s)
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
set house_e::implicit_enduses_active = IEU_ALL;
enumeration house_e::implicit_enduse_source = IES_ELCAP1990;
static double aux_cutin_temperature = 10;

EXPORT_METHOD(house_e,smart_breaker)
int jprintf(char *buffer, size_t len, ...)
{
	va_list ptr;
	va_start(ptr,len);
	char *p = buffer;
	if ( len <= 0 )
		return 0;
	*p++ = '{';
	int count = 0;
	while ( p < buffer + len-2 )
	{
		char t[1024];
		char *token = va_arg(ptr,char*);
		if ( token == NULL ) break;
		char *value = va_arg(ptr,char*);
		if ( value == NULL ) 
		{
			gl_error("jprintf(): unexpected NULL value");
			break;
		}
		int rl = snprintf(t,sizeof(t),"\"%s\":\"%s\"",token,value);
		if ( p + rl >= buffer + len-2 )
			return -strlen(buffer);
		p += sprintf(p,"%s%s",count++==0?"":",",t);
	}
	strcat(p,"}");
	va_end(ptr);
	return strlen(buffer);
}
void house_e::update_smartfuse(CIRCUIT *c)
{
	SMARTFUSE *fuse = c->smartfuse;
	double R = fuse->powerRef;
	if ( R <= 0 ) // unable to control to this reference
		return;
	enduse *load = c->pLoad;
	double Z = load->admittance.Mag();
	double I = load->current.Mag();
	double P = load->power.Mag();
	double V = 1.0; // assume unity unless otherwise controlled
	if ( Z <= 0.0 ) 
	{
		if ( I > 0.0 )
		{
			V = (R-P)/I;
		}
	}
	else
	{
		V = (sqrt(I*I-4*(P-R)*Z) - I)/(2*Z);
	}
	double Vm = V;
	if ( Vm < fuse->vMin )
		fuse->vFactor = fuse->vMin;
	else if ( Vm > fuse->vMax )
		fuse->vFactor = fuse->vMax;
	else
		fuse->vFactor = Vm;
}
int house_e::smart_breaker(char *buffer, size_t len)
{
	char reply[1024] = "";

	// parse the incoming message
	char target[64] = "unknown", cmd[1024], value[1024];
	int nargs = sscanf(buffer,"%[^&]&%[^=]=%[^\n]",target,cmd,value);
	if ( nargs < 2 )
	{
		error("house_e::smart_breaker(char *buffer='%s', size_t len=%d): incoming message rejected (sscanf()=%d)",buffer,len,nargs);
		jprintf(reply,sizeof(reply),"error","invalid message format",NULL);
	}

	// find the circuit this message applies to
	CIRCUIT *c;
	for ( c = panel.circuits ; c != NULL ; c = c->next )
	{
		if ( c->pLoad->name != NULL && strcmp(c->pLoad->name,target) == 0 )
			break;
	}
	if ( c == NULL ) // circuit not found
	{
		error("house_e::smart_breaker(char *buffer='%s', size_t len=%d): incoming message rejected (circuit '%s' not found)",buffer,len,target);
		jprintf(reply,sizeof(reply),"status","error","data","circuit not found",NULL);
	}
	else // handle the message
	{
		bool ok = false;
		if ( strcmp(cmd,"GET") == 0 )
		{
			ok = true;
		}
		else if ( strcmp(cmd,"RESET") == 0 )
		{
			c->status = BRK_CLOSED;
			ok = true;
		}
		else if ( strcmp(cmd,"OPEN") == 0 )
		{
			c->status = BRK_OPEN;
			ok = true;
		}
		else if ( strcmp(cmd,"CONTROL") == 0 )
		{
			if ( c->smartfuse == NULL )
			{
				c->smartfuse->powerRef = atof(value);
				ok = true;
			}
			else
			{
				jprintf(reply,sizeof(reply),"status","error","data","smartfuse not installed",NULL);
				ok = false;
			}
		}
		if ( ok )
		{	
			char amps[64];
			sprintf(amps,"%.0f",c->max_amps);
			char volts[64];
			sprintf(volts,"%.0f",c->pV->Mag());
			char data[1024];
			jprintf(data,sizeof(data),"breaker",c->status==BRK_OPEN?"OPEN":(c->status==BRK_CLOSED?"CLOSED":(c->status==BRK_FAULT?"FAULT":"ERROR")),
				"max",amps,
				"voltage",volts,
				"control","None",
				NULL);
			jprintf(reply,sizeof(reply),"status","ok","data",data,NULL);
		}
		else if ( strcmp(reply,"") == 0 ) // no reply to explain failure -- use general failure reply
		{
			jprintf(reply,sizeof(reply),"status","error","data","command rejected",NULL);
		}
	}
	int rv = strlen(reply);
	if ( rv >= len )
	{
		// note this can occur is a closing NULL is missing from the jprintf call
		error("house_e::smart_breaker(char *buffer='%s', size_t len=%d): reply '%s' is too long for the return buffer",buffer,len,reply);
		return 0; // return 0 if no return message, non-zero for message
	}
	strcpy(buffer,reply);
	return rv;
}

//////////////////////////////////////////////////////////////////////////
// implicit loadshapes - these are enabled by using implicit_enduses global
//////////////////////////////////////////////////////////////////////////
typedef struct s_implicit_enduse_list {
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
} IMPLICITENDUSEDATA;
#include "elcap1990.h"
#include "elcap2010.h"
#include "rbsa2014.h"
static IMPLICITENDUSEDATA *implicit_enduse_data = elcap1990;

EXPORT CIRCUIT *attach_enduse_house_e(OBJECT *obj, enduse *target, double breaker_amps, int is220)
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
CLASS* house_e::pclass = NULL;

double house_e::warn_low_temp = 55; // degF
double house_e::warn_high_temp = 95; // degF
bool house_e::warn_control = true;
double house_e::system_dwell_time = 1; // seconds

/** House object constructor:  Registers the class and publishes the variables that can be set by the user. 
Sets default randomized values for published variables.
**/
house_e::house_e(MODULE *mod) : residential_enduse(mod)
{
	// first time init
	if (oclass==NULL)  
	{
		// register the class definition
		oclass = gl_register_class(mod,"house",sizeof(house_e),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class house";
		else
			oclass->trl = TRL_PROVEN;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_object,"weather",PADDR(weather),PT_DESCRIPTION,"reference to the climate object",
			PT_double,"floor_area[sf]",PADDR(floor_area),PT_DESCRIPTION,"home conditioned floor area",
			PT_double,"gross_wall_area[sf]",PADDR(gross_wall_area),PT_DESCRIPTION,"gross outdoor wall area",
			PT_double,"ceiling_height[ft]",PADDR(ceiling_height),PT_DESCRIPTION,"average ceiling height",
			PT_double,"aspect_ratio",PADDR(aspect_ratio), PT_DESCRIPTION,"aspect ratio of the home's footprint",
			PT_double,"envelope_UA[Btu/degF/h]",PADDR(envelope_UA),PT_DESCRIPTION,"overall UA of the home's envelope",
			PT_double,"window_wall_ratio",PADDR(window_wall_ratio),PT_DESCRIPTION,"ratio of window area to wall area",
			PT_double,"number_of_doors",PADDR(number_of_doors),PT_DESCRIPTION,"ratio of door area to wall area",
			PT_double,"exterior_wall_fraction",PADDR(exterior_wall_fraction),PT_DESCRIPTION,"ratio of exterior wall area to total wall area",
			PT_double,"interior_exterior_wall_ratio",PADDR(interior_exterior_wall_ratio),PT_DESCRIPTION,"ratio of interior to exterior walls",
			PT_double,"exterior_ceiling_fraction",PADDR(exterior_ceiling_fraction),PT_DESCRIPTION,"ratio of external ceiling sf to floor area",
			PT_double,"exterior_floor_fraction",PADDR(exterior_floor_fraction),PT_DESCRIPTION,"ratio of floor area used in UA calculation",
			PT_double,"window_shading",PADDR(glazing_shgc),PT_DESCRIPTION,"transmission coefficient through window due to glazing",
			PT_double,"window_exterior_transmission_coefficient",PADDR(window_exterior_transmission_coefficient),PT_DESCRIPTION,"coefficient for the amount of energy that passes through window",
			PT_double,"solar_heatgain_factor",PADDR(solar_heatgain_factor),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"product of the window area, window transmitivity, and the window exterior transmission coefficient",
			PT_double,"airchange_per_hour[unit/h]",PADDR(airchange_per_hour),PT_DESCRIPTION,"number of air-changes per hour",
			PT_double,"airchange_UA[Btu/degF/h]",PADDR(airchange_UA),PT_DESCRIPTION,"additional UA due to air infiltration",
			PT_double,"UA[Btu/degF/h]",PADDR(UA),PT_DESCRIPTION,"the total UA",
			PT_double,"internal_gain[Btu/h]",PADDR(total.heatgain),PT_DESCRIPTION,"internal heat gains",
			PT_double,"solar_gain[Btu/h]",PADDR(solar_load),PT_DESCRIPTION,"solar heat gains",
			PT_double,"incident_solar_radiation[Btu/h/sf]",PADDR(incident_solar_radiation),PT_DESCRIPTION,"average incident solar radiation hitting the house",
			PT_double,"heat_cool_gain[Btu/h]",PADDR(load.heatgain),PT_DESCRIPTION,"system heat gains(losses)",

			PT_set,"include_solar_quadrant",PADDR(include_solar_quadrant),PT_DESCRIPTION,"bit set for determining which solar incidence quadrants should be included in the solar heatgain",
				PT_KEYWORD,"NONE",(set)NO_SOLAR,
				PT_KEYWORD,"H",(set)HORIZONTAL,
				PT_KEYWORD,"N",(set)NORTH,
				PT_KEYWORD,"E",(set)EAST,
				PT_KEYWORD,"S",(set)SOUTH,
				PT_KEYWORD,"W",(set)WEST,
			PT_double,"horizontal_diffuse_solar_radiation[Btu/h/sf]",PADDR(horizontal_diffuse_solar_radiation),PT_DESCRIPTION,"incident solar radiation hitting the top of the house",
			PT_double,"north_incident_solar_radiation[Btu/h/sf]",PADDR(north_incident_solar_radiation),PT_DESCRIPTION,"incident solar radiation hitting the north side of the house",
			PT_double,"northwest_incident_solar_radiation[Btu/h/sf]",PADDR(north_west_incident_solar_radiation),PT_DESCRIPTION,"incident solar radiation hitting the northwest side of the house",
			PT_double,"west_incident_solar_radiation[Btu/h/sf]",PADDR(west_incident_solar_radiation),PT_DESCRIPTION,"incident solar radiation hitting the west side of the house",
			PT_double,"southwest_incident_solar_radiation[Btu/h/sf]",PADDR(south_west_incident_solar_radiation),PT_DESCRIPTION,"incident solar radiation hitting the southwest side of the house",
			PT_double,"south_incident_solar_radiation[Btu/h/sf]",PADDR(south_incident_solar_radiation),PT_DESCRIPTION,"incident solar radiation hitting the south side of the house",
			PT_double,"southeast_incident_solar_radiation[Btu/h/sf]",PADDR(south_east_incident_solar_radiation),PT_DESCRIPTION,"incident solar radiation hitting the southeast side of the house",
			PT_double,"east_incident_solar_radiation[Btu/h/sf]",PADDR(east_incident_solar_radiation),PT_DESCRIPTION,"incident solar radiation hitting the east side of the house",
			PT_double,"northeast_incident_solar_radiation[Btu/h/sf]",PADDR(north_east_incident_solar_radiation),PT_DESCRIPTION,"incident solar radiation hitting the northeast side of the house",
			PT_enumeration,"heating_cop_curve",PADDR(heating_cop_curve),PT_DESCRIPTION,"Defines the function type to use for the adjusted heating COP as a function of outside air temperature.",
				PT_KEYWORD,"DEFAULT",(enumeration)HC_DEFAULT,
				PT_KEYWORD,"FLAT",(enumeration)HC_FLAT,
				PT_KEYWORD,"LINEAR",(enumeration)HC_LINEAR,
				PT_KEYWORD,"CURVED",(enumeration)HC_CURVED,
			PT_enumeration,"heating_cap_curve",PADDR(heating_cap_curve),PT_DESCRIPTION,"Defines the function type to use for the adjusted heating capacity as a function of outside air temperature.",
				PT_KEYWORD,"DEFAULT",(enumeration)HP_DEFAULT,
				PT_KEYWORD,"FLAT",(enumeration)HP_FLAT,
				PT_KEYWORD,"LINEAR",(enumeration)HP_LINEAR,
				PT_KEYWORD,"CURVED",(enumeration)HP_CURVED,
			PT_enumeration,"cooling_cop_curve",PADDR(cooling_cop_curve),PT_DESCRIPTION,"Defines the function type to use for the adjusted cooling COP as a function of outside air temperature.",
				PT_KEYWORD,"DEFAULT",(enumeration)CC_DEFAULT,
				PT_KEYWORD,"FLAT",(enumeration)CC_FLAT,
				PT_KEYWORD,"LINEAR",(enumeration)CC_LINEAR,
				PT_KEYWORD,"CURVED",(enumeration)CC_CURVED,
			PT_enumeration,"cooling_cap_curve",PADDR(cooling_cap_curve),PT_DESCRIPTION,"Defines the function type to use for the adjusted cooling capacity as a function of outside air temperature.",
				PT_KEYWORD,"DEFAULT",(enumeration)CP_DEFAULT,
				PT_KEYWORD,"FLAT",(enumeration)CP_FLAT,
				PT_KEYWORD,"LINEAR",(enumeration)CP_LINEAR,
				PT_KEYWORD,"CURVED",(enumeration)CP_CURVED,
			PT_bool,"use_latent_heat",PADDR(use_latent_heat),PT_DESCRIPTION,"Boolean for using the heat latency of the air to the humidity when cooling.",
			PT_bool,"include_fan_heatgain",PADDR(include_fan_heatgain),PT_DESCRIPTION,"Boolean to choose whether to include the heat generated by the fan in the ETP model.",

			PT_double,"thermostat_deadband[degF]",PADDR(thermostat_deadband),PT_DESCRIPTION,"deadband of thermostat control",
			PT_double,"dlc_offset[degF]",PADDR(dlc_offset),PT_DESCRIPTION,"used as a cap to offset the thermostat deadband for direct load control applications",
			PT_int16,"thermostat_cycle_time",PADDR(thermostat_cycle_time),PT_DESCRIPTION,"minimum time in seconds between thermostat updates",
			PT_int16,"thermostat_off_cycle_time",PADDR(thermostat_off_cycle_time),PT_DESCRIPTION,"the minimum amount of time the thermostat cycle must stay in the off state",
			PT_int16,"thermostat_on_cycle_time",PADDR(thermostat_on_cycle_time),PT_DESCRIPTION,"the minimum amount of time the thermostat cycle must stay in the on state",
			PT_timestamp,"thermostat_last_cycle_time",PADDR(thermostat_last_cycle_time),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"last time the thermostat changed state",
			PT_double,"heating_setpoint[degF]",PADDR(heating_setpoint),PT_DESCRIPTION,"thermostat heating setpoint",
			PT_double,"cooling_setpoint[degF]",PADDR(cooling_setpoint),PT_DESCRIPTION,"thermostat cooling setpoint",
			PT_double,"design_heating_setpoint[degF]",PADDR(design_heating_setpoint),PT_DESCRIPTION,"system design heating setpoint",
			PT_double,"design_cooling_setpoint[degF]",PADDR(design_cooling_setpoint),PT_DESCRIPTION,"system design cooling setpoint",
			PT_double,"over_sizing_factor",PADDR(over_sizing_factor),PT_DESCRIPTION,"over sizes the heating and cooling system from standard specifications (0.2 ='s 120% sizing)",

			PT_bool,"simulate_window_openings",PADDR(window_openings),PT_DESCRIPTION,"activates a representation of an occupant opening a window and de-activating the HVAC system",
			PT_double,"is_window_open",PADDR(window_open),PT_DESCRIPTION,"defines the state of the window opening, 1=open, 2=closed",
			PT_double,"window_low_temperature_cutoff[degF]",PADDR(window_low_temp),PT_DESCRIPTION,"lowest temperature at which the window opening might occur",
			PT_double,"window_high_temperature_cutoff[degF]",PADDR(window_high_temp),PT_DESCRIPTION,"highest temperature at which the window opening might occur",
			PT_double,"window_quadratic_coefficient",PADDR(window_a),PT_DESCRIPTION,"quadratic coefficient for describing function between low and high temperature cutoffs",
			PT_double,"window_linear_coefficient",PADDR(window_b),PT_DESCRIPTION,"linear coefficient for describing function between low and high temperature cutoffs",
			PT_double,"window_constant_coefficient",PADDR(window_c),PT_DESCRIPTION,"constant coefficient for describing function between low and high temperature cutoffs",
			PT_double,"window_temperature_delta",PADDR(window_temp_delta),PT_DESCRIPTION,"change in outdoor temperature required to update the window opening model",

			PT_double,"design_heating_capacity[Btu/h]",PADDR(design_heating_capacity),PT_DESCRIPTION,"system heating capacity",
			PT_double,"design_cooling_capacity[Btu/h]",PADDR(design_cooling_capacity),PT_DESCRIPTION,"system cooling capacity",
			PT_double,"cooling_design_temperature[degF]", PADDR(cooling_design_temperature),PT_DESCRIPTION,"system cooling design temperature",
			PT_double,"heating_design_temperature[degF]", PADDR(heating_design_temperature),PT_DESCRIPTION,"system heating design temperature",
			PT_double,"design_peak_solar[Btu/h/sf]", PADDR(design_peak_solar),PT_DESCRIPTION,"system design solar load",
			PT_double,"design_internal_gains[W/sf]", PADDR(design_internal_gains),PT_DESCRIPTION,"system design internal gains",
			PT_double,"air_heat_fraction[pu]", PADDR(air_heat_fraction), PT_DESCRIPTION, "fraction of heat gain/loss that goes to air (as opposed to mass)",
			PT_double,"mass_solar_gain_fraction[pu]", PADDR(mass_solar_gain_fraction), PT_DESCRIPTION, "fraction of the heat gain/loss from the solar gains that goes to the mass",
			PT_double,"mass_internal_gain_fraction[pu]", PADDR(mass_internal_gain_fraction), PT_DESCRIPTION, "fraction of heat gain/loss from the internal gains that goes to the mass",

			PT_double,"auxiliary_heat_capacity[Btu/h]",PADDR(aux_heat_capacity),PT_DESCRIPTION,"installed auxiliary heating capacity",
			PT_double,"aux_heat_deadband[degF]",PADDR(aux_heat_deadband),PT_DESCRIPTION,"temperature offset from standard heat activation to auxiliary heat activation",
			PT_double,"aux_heat_temperature_lockout[degF]",PADDR(aux_heat_temp_lockout),PT_DESCRIPTION,"temperature at which auxiliary heat will not engage above",
			PT_double,"aux_heat_time_delay[s]",PADDR(aux_heat_time_delay),PT_DESCRIPTION,"time required for heater to run until auxiliary heating engages",

			PT_double,"cooling_supply_air_temp[degF]",PADDR(cooling_supply_air_temp),PT_DESCRIPTION,"temperature of air blown out of the cooling system",
			PT_double,"heating_supply_air_temp[degF]",PADDR(heating_supply_air_temp),PT_DESCRIPTION,"temperature of air blown out of the heating system",
			PT_double,"duct_pressure_drop[inH2O]",PADDR(duct_pressure_drop),PT_DESCRIPTION,"end-to-end pressure drop for the ventilation ducts, in inches of water",
			PT_double,"fan_design_power[W]",PADDR(fan_design_power),PT_DESCRIPTION,"designed maximum power draw of the ventilation fan",
			PT_double,"fan_low_power_fraction[pu]",PADDR(fan_low_power_fraction),PT_DESCRIPTION,"fraction of ventilation fan power draw during low-power mode (two-speed only)",
			PT_double,"fan_power[kW]",PADDR(fan_power),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"current ventilation fan power draw",
			PT_double,"fan_design_airflow[cfm]",PADDR(fan_design_airflow),PT_DESCRIPTION,"designed airflow for the ventilation system",
			PT_double,"fan_impedance_fraction[pu]",PADDR(fan_impedance_fraction),PT_DESCRIPTION,"Impedance component of fan ZIP load",
			PT_double,"fan_power_fraction[pu]",PADDR(fan_power_fraction),PT_DESCRIPTION,"Power component of fan ZIP load",
			PT_double,"fan_current_fraction[pu]",PADDR(fan_current_fraction),PT_DESCRIPTION,"Current component of fan ZIP load",
			PT_double,"fan_power_factor[pu]",PADDR(fan_power_factor),PT_DESCRIPTION,"Power factor of the fan load",

			PT_double,"heating_demand[kW]",PADDR(heating_demand),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the current power draw to run the heating system",
			PT_double,"cooling_demand[kW]",PADDR(cooling_demand),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the current power draw to run the cooling system",
			PT_double,"heating_COP[pu]",PADDR(heating_COP),PT_DESCRIPTION,"system heating performance coefficient",
			PT_double,"cooling_COP[Btu/kWh]",PADDR(cooling_COP),PT_DESCRIPTION,"system cooling performance coefficient",
			//PT_double,"COP_coeff",PADDR(COP_coeff),PT_DESCRIPTION,"effective system performance coefficient",
			PT_double,"air_temperature[degF]",PADDR(Tair),PT_DESCRIPTION,"indoor air temperature",
			PT_double,"outdoor_temperature[degF]",PADDR(outside_temperature),PT_DESCRIPTION,"outdoor air temperature",
			PT_double,"outdoor_rh[%]",PADDR(outdoor_rh),PT_DESCRIPTION,"outdoor relative humidity",
			PT_double,"mass_heat_capacity[Btu/degF]",PADDR(house_content_thermal_mass),PT_DESCRIPTION,"interior mass heat capacity",
			PT_double,"mass_heat_coeff[Btu/degF/h]",PADDR(house_content_heat_transfer_coeff),PT_DESCRIPTION,"interior mass heat exchange coefficient",
			PT_double,"mass_temperature[degF]",PADDR(Tmaterials),PT_DESCRIPTION,"interior mass temperature",
			PT_double,"air_volume[cf]", PADDR(volume), PT_DESCRIPTION,"air volume",
			PT_double,"air_mass[lb]",PADDR(air_mass), PT_DESCRIPTION,"air mass",
			PT_double,"air_heat_capacity[Btu/degF]", PADDR(air_thermal_mass), PT_DESCRIPTION,"air thermal mass",
			PT_double,"latent_load_fraction[pu]", PADDR(latent_load_fraction), PT_DESCRIPTION,"fractional increase in cooling load due to latent heat",
			PT_double,"total_thermal_mass_per_floor_area[Btu/degF/sf]",PADDR(total_thermal_mass_per_floor_area),
			PT_double,"interior_surface_heat_transfer_coeff[Btu/h/degF/sf]",PADDR(interior_surface_heat_transfer_coeff),
			PT_double,"number_of_stories",PADDR(number_of_stories),PT_DESCRIPTION,"number of stories within the structure",

			PT_double,"is_AUX_on",PADDR(is_AUX_on),PT_DESCRIPTION,"logic statement to determine population statistics - is the AUX on? 0 no, 1 yes",
			PT_double,"is_HEAT_on",PADDR(is_HEAT_on),PT_DESCRIPTION,"logic statement to determine population statistics - is the HEAT on? 0 no, 1 yes",
			PT_double,"is_COOL_on",PADDR(is_COOL_on),PT_DESCRIPTION,"logic statement to determine population statistics - is the COOL on? 0 no, 1 yes",
			
			PT_double,"thermal_storage_present",PADDR(thermal_storage_present),PT_DESCRIPTION,"logic statement for determining if energy storage is present",
			PT_double,"thermal_storage_in_use",PADDR(thermal_storage_inuse),PT_DESCRIPTION,"logic statement for determining if energy storage is being utilized",

			PT_enumeration,"thermostat_mode",PADDR(thermostat_mode),PT_DESCRIPTION,"tells the thermostat whether it is even allowed to heat or cool the house.",
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "AUTO", (enumeration)TM_AUTO,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,

			PT_set,"system_type",PADDR(system_type),PT_DESCRIPTION,"heating/cooling system type/options",
				PT_KEYWORD, "NONE", (set)ST_NONE,
				PT_KEYWORD, "GAS",	(set)ST_GAS,
				PT_KEYWORD, "AIRCONDITIONING", (set)ST_AC,
				PT_KEYWORD, "FORCEDAIR", (set)ST_AIR,
				PT_KEYWORD, "TWOSTAGE", (set)ST_VAR,
				PT_KEYWORD, "RESISTIVE", (set)ST_RST,
			PT_set,"auxiliary_strategy",PADDR(auxiliary_strategy),PT_DESCRIPTION,"auxiliary heating activation strategies",
				PT_KEYWORD, "NONE", (set)AX_NONE,
				PT_KEYWORD, "DEADBAND", (set)AX_DEADBAND,
				PT_KEYWORD, "TIMER", (set)AX_TIMER,
				PT_KEYWORD, "LOCKOUT", (set)AX_LOCKOUT,
			PT_enumeration,"system_mode",PADDR(system_mode),PT_DESCRIPTION,"heating/cooling system operation state",
				PT_KEYWORD,"UNKNOWN",(enumeration)SM_UNKNOWN,
				PT_KEYWORD,"HEAT",(enumeration)SM_HEAT,
				PT_KEYWORD,"OFF",(enumeration)SM_OFF,
				PT_KEYWORD,"COOL",(enumeration)SM_COOL,
				PT_KEYWORD,"AUX",(enumeration)SM_AUX,
			PT_enumeration,"last_system_mode",PADDR(last_system_mode),PT_DESCRIPTION,"heating/cooling system operation state",
				PT_KEYWORD,"UNKNOWN",(enumeration)SM_UNKNOWN,
				PT_KEYWORD,"HEAT",(enumeration)SM_HEAT,
				PT_KEYWORD,"OFF",(enumeration)SM_OFF,
				PT_KEYWORD,"COOL",(enumeration)SM_COOL,
				PT_KEYWORD,"AUX",(enumeration)SM_AUX,
			PT_enumeration,"heating_system_type",PADDR(heating_system_type),
				PT_KEYWORD,"NONE",(enumeration)HT_NONE,
				PT_KEYWORD,"GAS",(enumeration)HT_GAS,
				PT_KEYWORD,"HEAT_PUMP",(enumeration)HT_HEAT_PUMP,
				PT_KEYWORD,"RESISTANCE",(enumeration)HT_RESISTANCE,
			PT_enumeration,"cooling_system_type",PADDR(cooling_system_type),
				PT_KEYWORD,"NONE",(enumeration)CT_NONE,
				PT_KEYWORD,"ELECTRIC",(enumeration)CT_ELECTRIC,
				PT_KEYWORD,"HEAT_PUMP",(enumeration)CT_ELECTRIC,
			PT_enumeration,"auxiliary_system_type",PADDR(auxiliary_system_type),
				PT_KEYWORD,"NONE",(enumeration)AT_NONE,
				PT_KEYWORD,"ELECTRIC",(enumeration)AT_ELECTRIC,
			PT_enumeration,"fan_type",PADDR(fan_type),
				PT_KEYWORD,"NONE",(enumeration)FT_NONE,
				PT_KEYWORD,"ONE_SPEED",(enumeration)FT_ONE_SPEED,
				PT_KEYWORD,"TWO_SPEED",(enumeration)FT_TWO_SPEED,
			PT_enumeration,"thermal_integrity_level",PADDR(thermal_integrity_level),PT_DESCRIPTION,"default envelope UA settings",
				PT_KEYWORD,"VERY_LITTLE",(enumeration)TI_VERY_LITTLE,
				PT_KEYWORD,"LITTLE",(enumeration)TI_LITTLE,
				PT_KEYWORD,"BELOW_NORMAL",(enumeration)TI_BELOW_NORMAL,
				PT_KEYWORD,"NORMAL",(enumeration)TI_NORMAL,
				PT_KEYWORD,"ABOVE_NORMAL",(enumeration)TI_ABOVE_NORMAL,
				PT_KEYWORD,"GOOD",(enumeration)TI_GOOD,
				PT_KEYWORD,"VERY_GOOD",(enumeration)TI_VERY_GOOD,
				PT_KEYWORD,"UNKNOWN",(enumeration)TI_UNKNOWN,
			PT_enumeration, "glass_type", PADDR(glass_type), PT_DESCRIPTION, "glass used in the windows",
				PT_KEYWORD,"OTHER",(enumeration)GM_OTHER,
				PT_KEYWORD,"GLASS",(enumeration)GM_GLASS,
				PT_KEYWORD,"LOW_E_GLASS",(enumeration)GM_LOW_E_GLASS,
			PT_enumeration, "window_frame", PADDR(window_frame), PT_DESCRIPTION, "type of window frame",
				PT_KEYWORD, "NONE", (enumeration)WF_NONE,
				PT_KEYWORD, "ALUMINUM", (enumeration)WF_ALUMINUM,
				PT_KEYWORD, "ALUMINIUM", (enumeration)WF_ALUMINUM, // non-American spelling
				PT_KEYWORD, "THERMAL_BREAK", (enumeration)WF_THERMAL_BREAK,
				PT_KEYWORD, "WOOD", (enumeration)WF_WOOD,
				PT_KEYWORD, "INSULATED", (enumeration)WF_INSULATED,
			PT_enumeration, "glazing_treatment", PADDR(glazing_treatment), PT_DESCRIPTION, "the treatment to increase the reflectivity of the exterior windows",
				PT_KEYWORD, "OTHER", (enumeration)GT_OTHER,
				PT_KEYWORD, "CLEAR", (enumeration)GT_CLEAR,
				PT_KEYWORD, "ABS", (enumeration)GT_ABS,
				PT_KEYWORD, "REFL", (enumeration)GT_REFL,
				PT_KEYWORD, "LOW_S", (enumeration)GT_LOW_S,
				PT_KEYWORD, "HIGH_S", (enumeration)GT_HIGH_S,
			PT_enumeration, "glazing_layers", PADDR(glazing_layers), PT_DESCRIPTION, "number of layers of glass in each window",
				PT_KEYWORD, "ONE", (enumeration)GL_ONE,
				PT_KEYWORD, "TWO", (enumeration)GL_TWO,
				PT_KEYWORD, "THREE", (enumeration)GL_THREE,
				PT_KEYWORD, "OTHER",(enumeration) GL_OTHER,
			PT_enumeration, "motor_model", PADDR(motor_model), PT_DESCRIPTION, "indicates the level of detail used in modelling the hvac motor parameters",
				PT_KEYWORD, "NONE", (enumeration)MM_NONE,
				PT_KEYWORD, "BASIC", (enumeration)MM_BASIC,
				PT_KEYWORD, "FULL", (enumeration)MM_FULL,
			PT_enumeration, "motor_efficiency", PADDR(motor_efficiency), PT_DESCRIPTION, "when using motor model, describes the efficiency of the motor",
				PT_KEYWORD, "VERY_POOR", (enumeration)ME_VERY_POOR,
				PT_KEYWORD, "POOR", (enumeration)ME_POOR,
				PT_KEYWORD, "AVERAGE", (enumeration)ME_AVERAGE,
				PT_KEYWORD, "GOOD", (enumeration)ME_GOOD,
				PT_KEYWORD, "VERY_GOOD", (enumeration)ME_VERY_GOOD,
			PT_int64, "last_mode_timer", PADDR(last_mode_timer),
			PT_double, "hvac_motor_efficiency[unit]", PADDR(hvac_motor_efficiency), PT_DESCRIPTION, "when using motor model, percent efficiency of hvac motor",
			PT_double, "hvac_motor_loss_power_factor[unit]", PADDR(hvac_motor_loss_power_factor), PT_DESCRIPTION, "when using motor model, power factor of motor losses",
			PT_double, "Rroof[degF*sf*h/Btu]", PADDR(Rroof),PT_DESCRIPTION,"roof R-value",
			PT_double, "Rwall[degF*sf*h/Btu]", PADDR(Rwall),PT_DESCRIPTION,"wall R-value",
			PT_double, "Rfloor[degF*sf*h/Btu]", PADDR(Rfloor),PT_DESCRIPTION,"floor R-value",
			PT_double, "Rwindows[degF*sf*h/Btu]", PADDR(Rwindows),PT_DESCRIPTION,"window R-value",
			PT_double, "Rdoors[degF*sf*h/Btu]", PADDR(Rdoors),PT_DESCRIPTION,"door R-value",
			PT_double, "hvac_breaker_rating[A]", PADDR(hvac_breaker_rating), PT_DESCRIPTION,"determines the amount of current the HVAC circuit breaker can handle",
			PT_double, "hvac_power_factor[unit]", PADDR(hvac_power_factor), PT_DESCRIPTION,"power factor of hvac",
			
			PT_double,"hvac_load[kW]",PADDR(hvac_load),PT_DESCRIPTION,"heating/cooling system load",
			PT_double,"last_heating_load[kW]",PADDR(last_heating_load),PT_DESCRIPTION,"stores the previous heating/cooling system load",
			PT_double,"last_cooling_load[kW]",PADDR(last_cooling_load),PT_DESCRIPTION,"stores the previous heating/cooling system load",
			PT_complex,"hvac_power[kVA]",PADDR(hvac_power),PT_DESCRIPTION,"describes hvac load complex power consumption",
			PT_double,"total_load[kW]",PADDR(total_load),
			PT_enduse,"panel",PADDR(total),PT_DESCRIPTION,"total panel enduse load",
			PT_double,"design_internal_gain_density[W/sf]",PADDR(design_internal_gain_density),PT_DESCRIPTION,"average density of heat generating devices in the house",
			PT_bool,"compressor_on",PADDR(compressor_on),
			PT_int64,"compressor_count",PADDR(compressor_count),
			PT_timestamp,"hvac_last_on",PADDR(hvac_last_on),
			PT_timestamp,"hvac_last_off",PADDR(hvac_last_off),
			PT_double,"hvac_period_length[s]",PADDR(hvac_period_length),
			PT_double,"hvac_duty_cycle",PADDR(hvac_duty_cycle),

			// these are hidden so we can spy on ETP
			PT_double,"a",PADDR(a),PT_ACCESS,PA_HIDDEN,
			PT_double,"b",PADDR(b),PT_ACCESS,PA_HIDDEN,
			PT_double,"c",PADDR(c),PT_ACCESS,PA_HIDDEN,
			PT_double,"d",PADDR(d),PT_ACCESS,PA_HIDDEN,
			PT_double,"c1",PADDR(c1),PT_ACCESS,PA_HIDDEN,
			PT_double,"c2",PADDR(c2),PT_ACCESS,PA_HIDDEN,
			PT_double,"A3",PADDR(A3),PT_ACCESS,PA_HIDDEN,
			PT_double,"A4",PADDR(A4),PT_ACCESS,PA_HIDDEN,
			PT_double,"k1",PADDR(k1),PT_ACCESS,PA_HIDDEN,
			PT_double,"k2",PADDR(k2),PT_ACCESS,PA_HIDDEN,
			PT_double,"r1",PADDR(r1),PT_ACCESS,PA_HIDDEN,
			PT_double,"r2",PADDR(r2),PT_ACCESS,PA_HIDDEN,
			PT_double,"Teq",PADDR(Teq),PT_ACCESS,PA_HIDDEN,
			PT_double,"Tevent",PADDR(Tevent),PT_ACCESS,PA_HIDDEN,
			PT_double,"Qi",PADDR(Qi),PT_ACCESS,PA_HIDDEN,
			PT_double,"Qa",PADDR(Qa),PT_ACCESS,PA_HIDDEN,
			PT_double,"Qm",PADDR(Qm),PT_ACCESS,PA_HIDDEN,
			PT_double,"Qh",PADDR(load.heatgain),PT_ACCESS,PA_HIDDEN,
			PT_double,"Qlatent",PADDR(Qlatent),PT_ACCESS,PA_HIDDEN,
			PT_double,"dTair",PADDR(dTair),PT_ACCESS,PA_HIDDEN,
			PT_double,"adj_cooling_cap",PADDR(adj_cooling_cap),PT_ACCESS,PA_HIDDEN,
			PT_double,"adj_heating_cap",PADDR(adj_heating_cap),PT_ACCESS,PA_HIDDEN,
			PT_double,"adj_cooling_cop",PADDR(adj_cooling_cop),PT_ACCESS,PA_HIDDEN,
			PT_double,"adj_heating_cop",PADDR(adj_heating_cop),PT_ACCESS,PA_HIDDEN,

			PT_enumeration,"thermostat_control", PADDR(thermostat_control), PT_DESCRIPTION, "determine level of internal thermostatic control",
				PT_KEYWORD, "FULL", (enumeration)TC_FULL, // setpoint/deadband controls HVAC
				PT_KEYWORD, "BAND", (enumeration)TC_BAND, // T<mode>{On,Off} control HVAC (setpoints/deadband are ignored)
				PT_KEYWORD, "NONE", (enumeration)TC_NONE, // system_mode controls HVAC (setpoints/deadband and T<mode>{On,Off} are ignored)

			PT_method, "circuit", get_smart_breaker_offset(), PT_ACCESS,PA_PROTECTED, PT_DESCRIPTION, "smart breaker message handlers", 
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);			

		gl_publish_function(oclass,	"attach_enduse", (FUNCTIONADDR)attach_enduse_house_e);
		gl_global_create("residential::implicit_enduses",PT_set,&implicit_enduses_active,
			PT_KEYWORD, "LIGHTS", (set)IEU_LIGHTS,
			PT_KEYWORD, "PLUGS", (set)IEU_PLUGS,
			PT_KEYWORD, "OCCUPANCY", (set)IEU_OCCUPANCY,
			PT_KEYWORD, "DISHWASHER", (set)IEU_DISHWASHER,
			PT_KEYWORD, "MICROWAVE", (set)IEU_MICROWAVE,
			PT_KEYWORD, "FREEZER", (set)IEU_FREEZER,
			PT_KEYWORD, "REFRIGERATOR", (set)IEU_REFRIGERATOR,
			PT_KEYWORD, "RANGE", (set)IEU_RANGE,
			PT_KEYWORD, "EVCHARGER", (set)IEU_EVCHARGER,
			PT_KEYWORD, "WATERHEATER", (set)IEU_WATERHEATER,
			PT_KEYWORD, "CLOTHESWASHER", (set)IEU_CLOTHESWASHER,
			PT_KEYWORD, "DRYER", (set)IEU_DRYER,
			PT_KEYWORD, "NONE", (set)0,
			PT_DESCRIPTION, "list of implicit enduses that are active in houses",
			NULL);
		gl_global_create("residential::implicit_enduse_source",PT_enumeration,&implicit_enduse_source,
			PT_KEYWORD,"ELCAP1990", (enumeration)IES_ELCAP1990,
			PT_KEYWORD,"ELCAP2010", (enumeration)IES_ELCAP2010,
			PT_KEYWORD,"RBSA2014", (enumeration)IES_RBSA2014,
			NULL);
		gl_global_create("residential::house_low_temperature_warning[degF]",PT_double,&warn_low_temp,
			PT_DESCRIPTION, "the low house indoor temperature at which a warning will be generated",
			NULL);
		gl_global_create("residential::house_high_temperature_warning[degF]",PT_double,&warn_high_temp,
			PT_DESCRIPTION, "the high house indoor temperature at which a warning will be generated",
			NULL);
		gl_global_create("residential::thermostat_control_warning",PT_double,&warn_control,
			PT_DESCRIPTION, "boolean to indicate whether a warning is generated when indoor temperature is out of control limits",
			NULL);
		gl_global_create("residential::system_dwell_time[s]",PT_double,&system_dwell_time,
			PT_DESCRIPTION, "the heating/cooling system dwell time interval for changing system state",
			NULL);
	}	
		gl_global_create("residential::aux_cutin_temperature[degF]",PT_double,&aux_cutin_temperature,
			PT_DESCRIPTION, "the outdoor air temperature below which AUX heating is used",
			NULL);

		if (gl_publish_function(oclass,	"interupdate_res_object", (FUNCTIONADDR)interupdate_house_e)==NULL)
			GL_THROW("Unable to publish house_e deltamode function");
		if (gl_publish_function(oclass,	"postupdate_res_object", (FUNCTIONADDR)postupdate_house_e)==NULL)
			GL_THROW("Unable to publish house_e deltamode function");

}

int house_e::create() 
{
	int result=SUCCESS;
	char active_enduses[1025];
	gl_global_getvar("residential::implicit_enduses",active_enduses,sizeof(active_enduses));
	char *token = NULL;
	error_flag = 0;

	//glazing_shgc = 0.65; // assuming generic double glazing
	// now zero to catch lookup trigger
	glazing_shgc = 0;
	load.power_fraction = 0.8;
	load.impedance_fraction = 0.2;
	load.current_fraction = 0.0;
	design_internal_gain_density = 0.6;
	thermal_integrity_level = TI_UNKNOWN;
	hvac_breaker_rating = 0;
	hvac_power_factor = 0;
	Tmaterials = 0.0;

	cooling_supply_air_temp = 50.0;
	heating_supply_air_temp = 150.0;
	
	heating_system_type = HT_HEAT_PUMP; // assume heat pump under all circumstances until we are told otherwise
	cooling_system_type = CT_UNKNOWN;
	auxiliary_system_type = AT_UNKNOWN;
	fan_type = FT_UNKNOWN;
	fan_power_factor = 0.96;
	fan_current_fraction = 0.7332;
	fan_impedance_fraction = 0.2534;
	fan_power_fraction = 0.0135;

	glazing_layers = GL_TWO;
	glass_type = GM_LOW_E_GLASS;
	glazing_treatment = GT_CLEAR;
	window_frame = WF_THERMAL_BREAK;
	motor_model = MM_NONE;
	motor_efficiency = ME_AVERAGE;
	hvac_motor_efficiency = 1;
	hvac_motor_loss_power_factor = 0.125;
	hvac_motor_real_loss = 0;
	hvac_motor_reactive_loss = 0;
	is_AUX_on = is_HEAT_on = is_COOL_on = 0;

	//Set thermal storage flag
	thermal_storage_present = 0;
	thermal_storage_inuse = 0;

	//Powerflow hooks
	pHouseConn = NULL;
	pMeterStatus = NULL;

	// set up implicit enduse list
	implicit_enduse_list = NULL;
	if (strcmp(active_enduses,"NONE")!=0)
	{
		char *eulist[64];
		char n_eu=0;

		// extract the implicit_enduse list
		while ((token=strtok(token?NULL:active_enduses,"|"))!=NULL)
			eulist[n_eu++] = token;

		while (n_eu-->0)
		{
			char *euname = eulist[n_eu];
			_strlwr(euname);
			
			// find the implicit enduse description
			struct s_implicit_enduse_list *eu = NULL;
			int found=0;
			switch ( implicit_enduse_source ) {
			case IES_ELCAP1990:
				eu = elcap1990;
				break;
			case IES_ELCAP2010:
				gl_warning("ELCAP2010 implicit enduse data is not valid for individual enduses");
				eu = elcap2010;
				break;
			case IES_RBSA2014:
				gl_warning("RBSA2010 implicit enduse data is not valid for individual enduses");
				eu = rbsa2014;
				break;
			default:
				gl_error("implicit enduse source '%d' is not recognized, using default ELCAP1990 instead", implicit_enduse_source);
				eu = elcap1990;
				break;
			}
			for ( ; eu->implicit_name!=NULL ; eu++)
			{
				char name[64];
				sprintf(name,"residential-%s-default",euname);
				// matched enduse and doesn't already exist
				if (strcmp(eu->schedule_name,name)==0)
				{
					SCHEDULE *sched = gl_schedule_find(name);
					if (sched==NULL){
						sched = gl_schedule_create(eu->schedule_name,eu->schedule_definition);
					}
					if(sched == NULL){
						gl_error("error creating schedule for enduse \'%s\'", eu->schedule_name);
						return FAILED;
					}
					IMPLICITENDUSE *item = (IMPLICITENDUSE*)gl_malloc(sizeof(IMPLICITENDUSE));
					memset(item,0,sizeof(IMPLICITENDUSE));
					gl_enduse_create(&(item->load));
					item->load.shape = gl_loadshape_create(sched);
					if (gl_set_value_by_type(PT_loadshape,item->load.shape,eu->shape)==0)
					{
						gl_error("loadshape '%s' could not be created", name);
						result = FAILED;
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
			{
				gl_error("house_e data for '%s' implicit enduse not found", euname);
				result = FAILED;
			}
		}
	}
	total.name = "panel";
	load.name = "system";

	//Reverse ETP Parameter Defaults
	include_solar_quadrant = 0x001e;
	UA = -1;
	airchange_per_hour = -1;
	heating_cop_curve = HC_DEFAULT;
	heating_cap_curve = HP_DEFAULT;
	cooling_cop_curve = CC_DEFAULT;
	cooling_cap_curve = CP_DEFAULT;
	thermostat_off_cycle_time = -1;
	thermostat_on_cycle_time = -1;
	use_latent_heat = true;
	include_fan_heatgain = true;
	fan_design_power = -1;
	heating_setpoint = 0.0;
	cooling_setpoint = 0.0;
	panel.max_amps = 0.0;
	system_type = 0;
	heating_COP = 0;
	cooling_COP = 0;
	number_of_stories = 0;
	aspect_ratio = 0;
	floor_area = 0;
	gross_wall_area = 0;
	window_wall_ratio = 0;
	window_roof_ratio = 0;
	interior_exterior_wall_ratio = 0;
	exterior_wall_fraction = 0;
	exterior_ceiling_fraction = 0;
	exterior_floor_fraction = 0;
	window_exterior_transmission_coefficient = 0;
	Rroof = 0;
	Rwall = 0;
	Rwindows = 0;
	Rdoors = 0;
	volume = 0;
	air_mass = 0;
	air_thermal_mass = 0;
	air_heat_fraction = 0;
	mass_solar_gain_fraction = 0;
	mass_internal_gain_fraction = 0;
	total_thermal_mass_per_floor_area = 0;
	interior_surface_heat_transfer_coeff = 0;
	airchange_UA = 0;
	envelope_UA = 0;
	design_cooling_setpoint = 0;
	design_heating_setpoint = 0;
	design_peak_solar = 0;
	thermostat_deadband = 0;
	thermostat_cycle_time = 0;
	Tair = 0;
	over_sizing_factor = 0;
	cooling_design_temperature = 0;
	design_internal_gains = 0;
	latent_load_fraction = 0;
	design_cooling_capacity = 0;
	design_heating_capacity = 0;
	system_mode = SM_UNKNOWN;
	last_system_mode = SM_UNKNOWN;
	last_mode_timer = 0;
	aux_heat_capacity = 0;
	aux_heat_deadband = 0;
	aux_heat_temp_lockout = 0;
	aux_heat_time_delay = 0;
	duct_pressure_drop = 0;
	fan_design_airflow = 0;
	fan_low_power_fraction = 0;
	fan_power = 0;
	house_content_thermal_mass = 0;
	house_content_heat_transfer_coeff = 0;

	// Window openings
	window_openings = FALSE;
	window_open = 0;			
	window_low_temp = 60;		
	window_high_temp = 80;		
	window_a = 0;			
	window_b = 0;		
	window_c = 1;
	window_temp_delta = 5; 
	last_temperature = 75;
	thermostat_mode = TM_AUTO;

//	printf("module flags = 0x%llx, house flags = 0x%llx\n", module_message_flags, OBJECTHDR(this)->flags);
//	verbose("creating house_e");
//	debug("created house_e");

	//Deltamode variables
	deltamode_inclusive = false;	//By default, don't be included in deltamode simulations
	deltamode_registered = false;
	default_frequency = 60.0;

	return result;
}

/** Checks for climate object and maps the climate variables to the house_e object variables.  
Currently Tout, RHout and solar flux data from TMY files are used.  If no climate object is linked,
then Tout will be set to 74 degF, RHout is set to 75% and solar flux will be set to zero for all orientations.
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
			extern double default_outdoor_temperature, default_humidity, default_solar[9];
			pTout = &default_outdoor_temperature;
			pRhout = &default_humidity;
			pSolar = &default_solar[0];
		}
		else if (climates->hit_count>1 && weather != 0)
		{
			gl_warning("house_e: %d climates found, using first one defined", climates->hit_count);
		}
	}
	if (climates!=NULL)
	{
		if (climates->hit_count==0)
		{
			//default to mock data
			extern double default_outdoor_temperature, default_humidity, default_solar[9];
			pTout = &default_outdoor_temperature;
			pRhout = &default_humidity;
			pSolar = &default_solar[0];
		}
		else //climate data was found
		{
			// force rank of object w.r.t climate
			OBJECT *obj = 0;
			if(weather == NULL){
				obj  = gl_find_next(climates,NULL);
				weather = obj;
			} else {
				obj = weather;
			}
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
			};
			int i;
			for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
			{
				double *src = (double*)GETADDR(obj,gl_get_property(obj,map[i].name));
				if (src) *map[i].dst = *src;
			}
			if((obj->flags & OF_INIT) != OF_INIT){
				char objname[256];
				gl_verbose("house::init(): deferring initialization on %s", gl_name(obj, objname, 255));
				return 0; // defer
			}
		}
	}
	return 1;
}

int house_e::isa(char *classname)
{
	return (strcmp(classname,"house")==0 || residential_enduse::isa(classname));
}

void house_e::set_thermal_integrity(){
	switch (thermal_integrity_level) {
		case TI_VERY_LITTLE:
			if(Rroof <= 0.0) Rroof = 11;
			if(Rwall <= 0.0) Rwall = 4;
			if(Rfloor <= 0.0) Rfloor = 4;
			if(Rdoors <= 0.0) Rdoors = 3;
			if(Rwindows <= 0.0) Rwindows = 1/1.27;
			if(airchange_per_hour < 0.0) airchange_per_hour = 1.5;
			break;
		case TI_LITTLE:
			if(Rroof <= 0.0) Rroof = 19;
			if(Rwall <= 0.0) Rwall = 11;
			if(Rfloor <= 0.0) Rfloor = 4;
			if(Rdoors <= 0.0) Rdoors = 3;
			if(Rwindows <= 0.0) Rwindows = 1/0.81;
			if(airchange_per_hour < 0.0) airchange_per_hour = 1.5;
			break;
		case TI_BELOW_NORMAL:
			if(Rroof <= 0.0) Rroof = 19;
			if(Rwall <= 0.0) Rwall = 11;
			if(Rfloor <= 0.0) Rfloor = 11;
			if(Rdoors <= 0.0) Rdoors = 3;
			if(Rwindows <= 0.0) Rwindows = 1/0.81;
			if(airchange_per_hour < 0.0) airchange_per_hour = 1.0;
			break;
		case TI_NORMAL:
			if(Rroof <= 0.0) Rroof = 30;
			if(Rwall <= 0.0) Rwall = 11;
			if(Rfloor <= 0.0) Rfloor = 19;
			if(Rdoors <= 0.0) Rdoors = 3;
			if(Rwindows <= 0.0) Rwindows = 1/0.6;
			if(airchange_per_hour < 0.0) airchange_per_hour = 1.0;
			break;
		case TI_ABOVE_NORMAL:
			if(Rroof <= 0.0) Rroof = 30;
			if(Rwall <= 0.0) Rwall = 19;
			if(Rfloor <= 0.0) Rfloor = 11;
			if(Rdoors <= 0.0) Rdoors = 3;
			if(Rwindows <= 0.0) Rwindows = 1/0.6;
			if(airchange_per_hour < 0.0) airchange_per_hour = 1.0;
			break;
		case TI_GOOD:
			if(Rroof <= 0.0) Rroof = 30;
			if(Rwall <= 0.0) Rwall = 19;
			if(Rfloor <= 0.0) Rfloor = 22;
			if(Rdoors <= 0.0) Rdoors = 5;
			if(Rwindows <= 0.0) Rwindows = 1/0.47;
			if(airchange_per_hour < 0.0) airchange_per_hour = 0.5;
			break;
		case TI_VERY_GOOD:
			if(Rroof <= 0.0) Rroof = 48;
			if(Rwall <= 0.0) Rwall = 22;
			if(Rfloor <= 0.0) Rfloor = 30;
			if(Rdoors <= 0.0) Rdoors = 11;
			if(Rwindows <= 0.0) Rwindows = 1/0.31;
			if(airchange_per_hour < 0.0) airchange_per_hour = 0.5;
			break;
		case TI_UNKNOWN:
			// do nothing - use all of the built-in defaults or user-specified values as thermal integrity wasn't used
			break;
		default:
			// do nothing - use all of the built-in defaults or user-specified values as thermal integrity wasn't used
			break;
	}
}


void house_e::set_window_shgc(){
	switch(glazing_layers){
		case GL_ONE:
			switch(glazing_treatment){
				case GT_CLEAR:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.86;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.75;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.64;
							break;
					}
					break;
				case GT_ABS:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.73;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.64;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.54;
							break;
					}
					break;
				case GT_REFL:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.31;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.28;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.24;
							break;
					}
					break;
			}
			break;
		case GL_TWO:
			switch(glazing_treatment){
				case GT_CLEAR:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.76;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.67;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.57;
							break;
					}
					break;
				case GT_ABS:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.62;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.55;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.46;
							break;
					}
					break;
				case GT_REFL:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.29;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.27;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.22;
							break;
					}
					break;
				case GT_LOW_S:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.41;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.37;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.31;
							break;
					}
					break;
				case GT_HIGH_S:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.70;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.62;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.52;
							break;
					}
					break;
			}
			break;
		case GL_THREE:
			switch(glazing_treatment){
				case GT_CLEAR:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.68;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.60;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.51;
							break;
					}
					break;
				case GT_ABS:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.34;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.31;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.26;
							break;
					}
					break;
				case GT_REFL:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.34;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.31;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.26;
							break;
					}
					break;
				case GT_LOW_S:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.27;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.25;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.21;
							break;
					}
					break;
				case GT_HIGH_S:
					switch(window_frame){
						case WF_NONE:
							glazing_shgc = 0.62;
							break;
						case WF_ALUMINUM:
						case WF_THERMAL_BREAK:
							glazing_shgc = 0.55;
							break;
						case WF_WOOD:
						case WF_INSULATED:
							glazing_shgc = 0.46;
							break;
					}
					break;
			}
			break;
	}
}

void house_e::set_window_Rvalue(){
	if(glass_type == GM_LOW_E_GLASS){
		switch(glazing_layers){
			case GL_ONE:
				gl_error("no value for one pane of low-e glass");
				break;
			case GL_TWO:
				switch(window_frame){
					case WF_NONE:
						Rwindows = 1.0/0.30;
						break;
					case WF_ALUMINUM:
						Rwindows = 1.0/0.67;
						break;
					case WF_THERMAL_BREAK:
						Rwindows = 1.0/0.47;
						break;
					case WF_WOOD:
						Rwindows = 1.0/0.41;
						break;
					case WF_INSULATED:
						Rwindows = 1.0/0.33;
						break;
				}
				break;
			case GL_THREE:
				switch(window_frame){
					case WF_NONE:
						Rwindows = 1/0.27;
						break;
					case WF_ALUMINUM:
						Rwindows = 1/0.64;
						break;
					case WF_THERMAL_BREAK:
						Rwindows = 1/0.43;
						break;
					case WF_WOOD:
						Rwindows = 1/0.37;
						break;
					case WF_INSULATED:
						Rwindows = 1/0.31;
						break;
				}
				break;
		}
	} else if(glass_type == GM_GLASS){
		switch(glazing_layers){
			case GL_ONE:
				switch(window_frame){
					case WF_NONE:
						Rwindows = 1/1.04;
						break;
					case WF_ALUMINUM:
						Rwindows = 1/1.27;
						break;
					case WF_THERMAL_BREAK:
						Rwindows = 1/1.08;
						break;
					case WF_WOOD:
						Rwindows = 1/0.90;
						break;
					case WF_INSULATED:
						Rwindows = 1/0.81;
						break;
				}
				break;
			case GL_TWO:
				switch(window_frame){
					case WF_NONE:
						Rwindows = 1/0.48;
						break;
					case WF_ALUMINUM:
						Rwindows = 1/0.81;
						break;
					case WF_THERMAL_BREAK:
						Rwindows = 1/0.60;
						break;
					case WF_WOOD:
						Rwindows = 1/0.53;
						break;
					case WF_INSULATED:
						Rwindows = 1/0.44;
						break;
				}
				break;
			case GL_THREE:
				switch(window_frame){
					case WF_NONE:
						Rwindows = 1/0.31;
						break;
					case WF_ALUMINUM:
						Rwindows = 1/0.67;
						break;
					case WF_THERMAL_BREAK:
						Rwindows = 1/0.46;
						break;
					case WF_WOOD:
						Rwindows = 1/0.40;
						break;
					case WF_INSULATED:
						Rwindows = 1/0.34;
						break;
				}
				break;
		}
	} else {//if(glass_type == GM_OTHER){
		Rwindows = 2.0;
	}
}
/** Map circuit variables to meter.  Initalize house_e and HVAC model properties,
and internal gain variables.
**/

int house_e::init(OBJECT *parent)
{
	//Deltamode globals
	extern bool enable_subsecond_models;
	extern int res_object_count;
	extern bool all_house_delta;

	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("house::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	heat_start = false;

	// local object name,	meter object name
	struct {
			complex **var;
			char *varname;
			} map[] = { {&pCircuit_V,			"voltage_12"}, // assumes 1N and 2N follow immediately in memory
						{&pLine_I,				"residential_nominal_current_1"}, // assumes 2 and 3(12) follow immediately in memory - off-nominal angles are handled externally
						{&pShunt,				"shunt_1"},		// assumes 2 and 3 (12) follow immediately in memory
						{&pPower,				"power_1"},		// assumes 2 and 3 (12) follow immediately in memory
					/// @todo use triplex property mapping instead of assuming memory order for meter variables (residential, low priority) (ticket #139)
					};

	extern complex default_line_voltage[3], default_line_current[3], default_line_power[3], default_line_shunt[3];
	extern int default_meter_status;
	int i;

	// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
	OBJECT *obj = OBJECTHDR(this);
	if (parent!=NULL && (gl_object_isa(parent,"triplex_meter","powerflow") || gl_object_isa(obj->parent,"triplex_node","powerflow")))
	{
		// attach meter variables to each circuit
		for (i=0; i<sizeof(map)/sizeof(map[0]); i++)
		{
			if ((*(map[i].var) = get_complex(parent,map[i].varname))==NULL)
				GL_THROW("%s (%s:%d) does not implement triplex_meter variable %s for %s (house_e:%d)", 
					parent->name?parent->name:"unnamed object", parent->oclass->name, parent->id, map[i].varname, obj->name?obj->name:"unnamed", obj->id);
		}
		pFrequency = new gld_property(parent, "measured_frequency");
		if(!pFrequency->is_valid())
			GL_THROW("%s (%s:%d) does not implement triplex_meter variable measured_frequency for %s (house_e:%d)",
				parent->name?parent->name:"unnamed object", parent->oclass->name, parent->id, obj->name?obj->name:"unnamed", obj->id);

		//Map to the triplex variable
		pHouseConn = get_bool(parent,"house_present");

		//Check it, for completeness
		if (pHouseConn==NULL)
		{
			gl_error("Failure to map powerflow variable from house:%s",obj->name);
			/*  TROUBLESHOOT
			While attempting to map the house variables to the required powerflow variables,
			an error occurred.  Please try again.  If the error persists, please submit your
			code and a bug report via the trac website.
			*/
			return 0;
		}

		//Flag that we're attached to a node
		*pHouseConn = true;

		//Map to the meter status property
		pMeterStatus = get_enum(parent,"service_status");

		//Make sure it worked
		if (pMeterStatus==NULL)
		{
			gl_error("Failure to map powerflow variable from house:%s",obj->name);
			//Defined above
			return 0;
		}
	}
	else
	{
		gl_warning("house_e:%d %s; using static voltages", obj->id, parent==NULL?"has no parent triplex_meter defined":"parent is not a triplex_meter");

		// attach meter variables to each circuit in the default_meter
		*(map[0].var) = &default_line_voltage[0];
		*(map[1].var) = &default_line_current[0];
		*(map[2].var) = &default_line_shunt[0];
		*(map[3].var) = &default_line_power[0];
		pFrequency = new gld_property(obj, "default_frequency");

		//Attach the meter status
		pMeterStatus = &default_meter_status;
	}

	//grab the start time of the simulation
	simulation_beginning_time = gl_globalclock;

	// set defaults for panel/meter variables
	if (panel.max_amps==0) panel.max_amps = 200; 
	load.power = complex(0,0,J);

	// old-style HVAC system variable mapping

	if(system_type & ST_GAS)		heating_system_type = HT_GAS;
	else if (system_type & ST_RST)	heating_system_type = HT_RESISTANCE;
	if(system_type & ST_AC)			cooling_system_type = CT_ELECTRIC;
	if(system_type & ST_AIR)		fan_type = FT_ONE_SPEED;
	if(system_type & ST_VAR)		fan_type = FT_TWO_SPEED;

	if(heating_system_type == HT_HEAT_PUMP && fan_type == FT_NONE){
		fan_type = FT_ONE_SPEED;
	}

	if(system_type != 0){ // using legacy format
		if(heating_system_type == HT_UNKNOWN)		heating_system_type = HT_NONE; // should not be possible, but good to put in.
		if(cooling_system_type == CT_UNKNOWN)		cooling_system_type = CT_NONE;
		if(fan_type == FT_UNKNOWN)					fan_type = FT_NONE;
		if(auxiliary_system_type == AT_UNKNOWN){
			auxiliary_system_type = AT_NONE;
			auxiliary_strategy = 0;
		}
	}
	if(system_type == 0){ // not using legacy format
		if(heating_system_type == HT_UNKNOWN)		heating_system_type = HT_HEAT_PUMP;
		if(cooling_system_type == CT_UNKNOWN)		cooling_system_type = CT_ELECTRIC;
		if(fan_type == FT_UNKNOWN)					fan_type = FT_ONE_SPEED;
		if(auxiliary_system_type == AT_UNKNOWN && heating_system_type == HT_HEAT_PUMP){
			auxiliary_system_type = AT_ELECTRIC;
			auxiliary_strategy = AX_DEADBAND;
		}
	} else if(!(system_type & ST_GAS) && !(system_type & ST_RST)){
		// if old style and using a heat pump, assume electric auxiliary with deadband
		auxiliary_system_type = AT_ELECTRIC;
		auxiliary_strategy = AX_DEADBAND;
	}

	if(heating_system_type == HT_HEAT_PUMP) {
		if(cooling_system_type == CT_NONE) 
			gl_warning("A HEAT_PUMP heating_system_type with no air conditioning does not make a lot of sense. You may encounter odd behavior with house %s",obj->name);
		else if(cooling_system_type == CT_UNKNOWN) {
			gl_warning("A HEAT_PUMP heating_system_type with no air conditioning does not make a lot of sense. Setting cooling_system_type to ELECTRIC.");
			cooling_system_type = CT_ELECTRIC;
		}
	}

	// Set defaults for published variables nor provided by model definition
	set_thermal_integrity();

	//	COP only affects heat pumps
	if (heating_COP==0.0)		heating_COP = 2.5;
	if (heating_COP < 0.0)		heating_COP = -heating_COP;
	if (cooling_COP==0.0)		cooling_COP = 3.5;
	if (cooling_COP < 0.0)		cooling_COP = -cooling_COP;

	if (number_of_stories < 1.0)
		number_of_stories = 1.0;
	if (fmod(number_of_stories, 1.0) != 0.0){
		number_of_stories = floor(number_of_stories);

	}
	if (aspect_ratio==0.0)		aspect_ratio = 1.5;
	if (floor_area==0)			floor_area = 2500.0;
	if (ceiling_height==0)		ceiling_height = 8.0;
	if (gross_wall_area==0)		gross_wall_area = 2.0 * number_of_stories * (aspect_ratio + 1.0) * ceiling_height * sqrt(floor_area/aspect_ratio/number_of_stories);
	if (window_wall_ratio==0)	window_wall_ratio = 0.15;
	if (window_roof_ratio==0)	window_roof_ratio = 0.0; // explicitly zero'ed
	if (number_of_doors==0)		number_of_doors = 4.0;
	else						number_of_doors = floor(number_of_doors); /* integer-based */
	if (interior_exterior_wall_ratio == 0) interior_exterior_wall_ratio = 1.5; //Based partions for six rooms per floor
	if (exterior_wall_fraction==0) exterior_wall_fraction = 1.0;
	if (exterior_ceiling_fraction==0) exterior_ceiling_fraction = 1.0;
	if (exterior_floor_fraction==0) exterior_floor_fraction = 1.0;
	if (window_exterior_transmission_coefficient<=0) window_exterior_transmission_coefficient = 0.60; //0.6 represents a window with a screen

	if (glazing_shgc <= 0.0)	set_window_shgc();
	if (Rroof<=0)				Rroof = 30.0;
	if (Rwall<=0)				Rwall = 19.0;
	if (Rfloor<=0)				Rfloor = 22.0;
	if (Rwindows<=0)			set_window_Rvalue();
	if (Rdoors<=0)				Rdoors = 5.0;
	
	air_density = 0.0735;		// density of air [lb/cf]
	air_heat_capacity = 0.2402;	// heat capacity of air @ 80F [BTU/lb/F]

	//house_e properties for HVAC
	if (volume==0) volume = ceiling_height*floor_area;					// volume of air [cf]
	if (air_mass==0) air_mass = air_density*volume;						// mass of air [lb]
	if (air_thermal_mass==0) air_thermal_mass = 3*air_heat_capacity*air_mass;	// thermal mass of air [BTU/F]  //*3 multiplier is to reflect that the air mass includes surface effects from the mass as well.  
	//if (air_heat_fraction==0) air_heat_fraction=0.5;
	if (air_heat_fraction!=0) {
		gl_warning("The air_heat_fraction is no longer used to determine heat gain/loss to the air. Setting mass_solar_gain_fraction and mass_internal_gain_fraction to 1-air_heat_fraction specified for house %s.", obj->name);
		mass_solar_gain_fraction=1-air_heat_fraction;
		mass_internal_gain_fraction=1-air_heat_fraction;
	}
	if (mass_solar_gain_fraction==0) mass_solar_gain_fraction=0.5; // Rob Pratt's implimentation for heat gain from the solar gains
	if (mass_internal_gain_fraction==0) mass_internal_gain_fraction=0.5; // Rob Pratt's implimentation for heat gain from internal gains
	if (air_heat_fraction<0.0 || air_heat_fraction>1.0) throw "air heat fraction is not between 0 and 1";
	if (mass_solar_gain_fraction<0.0 || mass_solar_gain_fraction>1.0) throw "the solar gain fraction to mass is not between 0 and 1";
	if (mass_internal_gain_fraction<0.0 || mass_internal_gain_fraction>1.0) throw "the internal gain fraction to mass is not between 0 and 1";
	if (total_thermal_mass_per_floor_area <= 0.0) total_thermal_mass_per_floor_area = 2.0;
	if (interior_surface_heat_transfer_coeff <= 0.0) interior_surface_heat_transfer_coeff = 1.46;

	if (airchange_per_hour<0)	airchange_per_hour = 0.5;//gl_random_triangle(0.5,1);
	if (airchange_UA <= 0) airchange_UA = airchange_per_hour * volume * air_density * air_heat_capacity;

	double door_area = number_of_doors * 3.0 * 78.0 / 12.0; // 3' wide by 78" tall
	double window_area = gross_wall_area * window_wall_ratio * exterior_wall_fraction;
	double net_exterior_wall_area = exterior_wall_fraction * gross_wall_area - window_area - door_area;
	double exterior_ceiling_area = floor_area * exterior_ceiling_fraction / number_of_stories;
	double exterior_floor_area = floor_area * exterior_floor_fraction / number_of_stories;

	if (envelope_UA==0)	envelope_UA = exterior_ceiling_area/Rroof + exterior_floor_area/Rfloor + net_exterior_wall_area/Rwall + window_area/Rwindows + door_area/Rdoors;
	if(UA < 0){
		UA = envelope_UA + airchange_UA;
	}
	solar_heatgain_factor = window_area * glazing_shgc * window_exterior_transmission_coefficient;

	// initalize/set system model parameters
	if (heating_setpoint==0.0)	heating_setpoint = 70.0;
	if (cooling_setpoint==0.0)	cooling_setpoint = 75.0;
	if (design_cooling_setpoint==0.0) design_cooling_setpoint = 75.0;
	if (design_heating_setpoint==0.0) design_heating_setpoint = 70.0;
	if (design_peak_solar<=0.0)	design_peak_solar = 195.0; //From Rob's defaults

	if (thermostat_deadband<=0.0)	thermostat_deadband = 2.0; // F
	if (thermostat_cycle_time<=0.0) thermostat_cycle_time = 120; // seconds
	if (Tair==0.0){
		/* bind limits between 60 and 140 degF */
		double Thigh = cooling_setpoint+thermostat_deadband/2.0;
		double Tlow  = heating_setpoint-thermostat_deadband/2.0;
		Thigh = clip(Thigh, 60.0, 140.0);
		Tlow = clip(Tlow, 60.0, 140.0);
		Tair = gl_random_uniform(RNGSTATE,Tlow, Thigh);	// air temperature [F]
	}
	if (over_sizing_factor<=0.0)  over_sizing_factor = 0.0;
	if (cooling_design_temperature == 0.0)	cooling_design_temperature = 95.0;
	if (design_internal_gains<=0.0) design_internal_gains =  167.09 * pow(floor_area,0.442); // Numerical estimate of internal gains
	if (latent_load_fraction<=0.0) latent_load_fraction = 0.30;

	double round_value = 0.0;
	if (design_cooling_capacity<=0.0 && cooling_system_type != CT_NONE)	// calculate basic load then round to nearest standard HVAC sizing
	{	
		round_value = 0.0;
		design_cooling_capacity = (1.0 + over_sizing_factor) * (1.0 + latent_load_fraction) * ((UA) * (cooling_design_temperature - design_cooling_setpoint) + design_internal_gains + (design_peak_solar * solar_heatgain_factor));
		round_value = (design_cooling_capacity) / 6000.0;
		design_cooling_capacity = ceil(round_value) * 6000.0;//design_cooling_capacity is rounded up to the next 6000 btu/hr
	}

	if(auxiliary_system_type != AT_NONE && heating_system_type == HT_NONE)
	{	/* auxiliary heating and no normal heating?  crazy talk! */
		static int aux_for_rst = 0;
		if(aux_for_rst == 0){
			gl_warning("house_e heating strategies with auxiliary heat but without normal heating modes are converted"
				"to resistively heated houses, see house %s",obj->name);
			aux_for_rst = 1;
		}
		heating_system_type = HT_RESISTANCE;
	}

	if (design_heating_capacity<=0 && heating_system_type != HT_NONE)	// calculate basic load then round to nearest standard HVAC sizing
	{
		double round_value = 0.0;
		if(heating_system_type == HT_HEAT_PUMP){
			design_heating_capacity = design_cooling_capacity; /* primary is to reverse the heat pump */
		} else {
			design_heating_capacity = (1.0 + over_sizing_factor) * (UA) * (design_heating_setpoint - heating_design_temperature);
			round_value = (design_heating_capacity) / 10000.0;
			design_heating_capacity = ceil(round_value) * 10000.0;//design_heating_capacity is rounded up to the next 10,000 btu/hr
		}
	}

    if (system_mode==SM_UNKNOWN) system_mode = SM_OFF;	// heating/cooling mode {HEAT, COOL, OFF}
	if (last_system_mode == SM_UNKNOWN) last_system_mode = SM_OFF;
	if (last_mode_timer == 0) last_mode_timer = 3600*6; // six hours

	if (aux_heat_capacity<=0.0 && auxiliary_system_type != AT_NONE)
	{
		double round_value = 0.0;
		aux_heat_capacity = (1.0 + over_sizing_factor) * (UA) * (design_heating_setpoint - heating_design_temperature);
		round_value = (aux_heat_capacity) / 10000.0;
		aux_heat_capacity = ceil(round_value) * 10000.0;//aux_heat_capacity is rounded up to the next 10,000 btu/hr
	}

	if (aux_heat_deadband<=0.0)		aux_heat_deadband = thermostat_deadband;
	if (aux_heat_temp_lockout<=0.0)	aux_heat_temp_lockout = 10; // 10 degrees
	if (aux_heat_time_delay<=0.0)	aux_heat_time_delay = 300.0; // five minutes

	if (duct_pressure_drop<=0.0)	duct_pressure_drop = 0.5; // half inch of water pressure
	if (fan_design_airflow<=0.0){
		double design_heating_cfm;
		double design_cooling_cfm;
		double gtr_cfm;
	
		design_heating_cfm = (design_heating_capacity > aux_heat_capacity ? design_heating_capacity : aux_heat_capacity) / (air_density * air_heat_capacity * (heating_supply_air_temp - design_heating_setpoint)) / 60.0;
		design_cooling_cfm = design_cooling_capacity / (1.0 + latent_load_fraction) / (air_density * air_heat_capacity * (design_cooling_setpoint - cooling_supply_air_temp)) / 60.0;
		gtr_cfm = (design_heating_cfm > design_cooling_cfm ? design_heating_cfm : design_cooling_cfm);
		fan_design_airflow = gtr_cfm;
	}

	if (fan_design_power<0.0){
		double roundval;
		//	
		roundval = ceil((0.117 * duct_pressure_drop * fan_design_airflow / 0.42 / 745.7)*8);
		fan_design_power = roundval / 8.0 * 745.7 / 0.88; // fan rounds to the nearest 1/8 HP
	}

	if (fan_low_power_fraction<=0.0 && fan_type == FT_TWO_SPEED)
									fan_low_power_fraction = 0.5; /* low-power mode for two-speed fans */
	if (fan_power <= 0.0)			fan_power = 0.0;

	if (house_content_thermal_mass==0) house_content_thermal_mass = total_thermal_mass_per_floor_area*floor_area - 2 * air_heat_capacity*air_mass;		// thermal mass of house_e [BTU/F]
    if (house_content_heat_transfer_coeff==0) house_content_heat_transfer_coeff = 
		interior_surface_heat_transfer_coeff * (
		//  (net_exterior_wall_area / exterior_wall_fraction)
		(gross_wall_area - window_area - door_area)
		+ gross_wall_area * interior_exterior_wall_ratio + number_of_stories * exterior_ceiling_area / exterior_ceiling_fraction);	// heat transfer coefficient of house_e contents [BTU/hr*F]
		// hs * ((Awt - Ag - Ad) + Awt * IWR + Ac * N / ECR)
	if(Tair == 0){
		if (system_mode==SM_OFF)
			Tair = gl_random_uniform(RNGSTATE,heating_setpoint,cooling_setpoint);
		else if (system_mode==SM_HEAT || system_mode==SM_AUX)
			Tair = gl_random_uniform(RNGSTATE,heating_setpoint-thermostat_deadband/2,heating_setpoint+thermostat_deadband/2);
		else if (system_mode==SM_COOL)
			Tair = gl_random_uniform(RNGSTATE,cooling_setpoint-thermostat_deadband/2,cooling_setpoint+thermostat_deadband/2);
	}

	if (Tmaterials == 0.0)
		Tmaterials = Tair;	

	// Set/calculate HVAC motor parameters
	if (motor_model != MM_NONE)
	{
		if ( hvac_motor_loss_power_factor > 1 || hvac_motor_loss_power_factor < -1 )
			GL_THROW("hvac_motor_power_factor must have a value between -1 and 1");
		if ( hvac_motor_efficiency > 1 || hvac_motor_efficiency < 0 )
			GL_THROW("hvac_motor_efficiency must have a value between 0 and 1");
		switch(motor_efficiency){
			case ME_VERY_POOR:
				hvac_motor_efficiency = 0.8236;
				break;
			case ME_POOR:
				hvac_motor_efficiency = 0.8488;
				break;
			case ME_AVERAGE:
				hvac_motor_efficiency = 0.8740;
				break;
			case ME_GOOD:
				hvac_motor_efficiency = 0.9020;
				break;
			case ME_VERY_GOOD:
				hvac_motor_efficiency = 0.9244;
				break;
			default:
				gl_warning("Unknown motor_efficiency setting.  Setting to AVERAGE.");
				hvac_motor_efficiency = 0.8740;
				motor_efficiency = ME_AVERAGE;
				break;
		}
	}
				

	// calculate thermal constants
#define Ca (air_thermal_mass)
#define Tout (outside_temperature)
#define Cm (house_content_thermal_mass)
#define Ua (UA)
#define Hm (house_content_heat_transfer_coeff)
#define Qs (solar_load)
#define Qh (load.heatgain)
#define Ta (Tair)
#define dTa (dTair)
#define Tm (Tmaterials)

	if (Ca<=0)
		throw "air_thermal_mass must be positive";
	if (Cm<=0)
		throw "house_content_thermal_mass must be positive";
	if(Hm <= 0)
		throw "house_content_heat_transfer_coeff must be positive";
	if(Ua < 0)
		throw "UA must be positive";

	a = Cm*Ca/Hm;
	b = Cm*(Ua+Hm)/Hm+Ca;
	c = Ua;
	c1 = -(Ua + Hm)/Ca;
	c2 = Hm/Ca;
	double rr = sqrt(b*b-4*a*c)/(2*a);
	double r = -b/(2*a);
	r1 = r+rr;
	r2 = r-rr;
	A3 = Ca/Hm * r1 + (Ua+Hm)/Hm;
	A4 = air_thermal_mass/Hm * r2 + (Ua+Hm)/Hm;

	// outside temperature init
	extern double default_outdoor_temperature;
	outside_temperature = default_outdoor_temperature;

	if (hvac_power_factor == 0)
		load.power_factor = 0.97;
	else 
		load.power_factor = hvac_power_factor;

	// connect any implicit loads
	attach_implicit_enduses();
	update_system();
	if(error_flag == 1){
		return 0;
	}
	update_model();
	
	// attach the house_e HVAC to the panel
	if (hvac_breaker_rating == 0)
	{
		load.breaker_amps = 200;
		hvac_breaker_rating = 200;
	}
	else
		load.breaker_amps = hvac_breaker_rating;
	load.config = EUC_IS220;
	attach(OBJECTHDR(this),hvac_breaker_rating, true, &load);

	if(include_fan_heatgain == TRUE){
		fan_heatgain_fraction = 1;
	} else {
		fan_heatgain_fraction = 0;
	}

	//"Cheating" function for compatibility with older models -- enables all houses to be deltamode-ready
	if ((all_house_delta == true) && (enable_subsecond_models==true))
	{
		obj->flags |= OF_DELTAMODE;
	}

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true;	//Set the flag and off we go
	}

	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (enable_subsecond_models!=true)
		{
			gl_warning("house_e:%s indicates it wants to run deltamode, but the module-level flag is not set!",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The house_e object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The house
			will not simulate any dynamics this way.
			*/

			//Deflag us
			deltamode_inclusive = false;
		}
		else
		{
			res_object_count++;	//Increment the counter
		}
	}//End deltamode inclusive
	else	//Not enabled for this model
	{
		if (enable_subsecond_models == true)
		{
			gl_warning("house_e:%d %s - Deltamode is enabled for the module, but not this house!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The house is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this house may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
	}

	return 1;
}

void house_e::attach_implicit_enduses()
{
	IMPLICITENDUSE *item;
	for (item=implicit_enduse_list; item!=NULL; item=item->next){
		attach(NULL,item->amps,item->is220,&(item->load));
		if (item->is220)
			item->load.config |= EUC_IS220;
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
			GL_THROW("end-use load %s couldn't be connected because it does not publish 'enduse_load' property", c->pLoad->name);
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
	// get frequency
	c->pfrequency;
	// close breaker
	c->status = BRK_CLOSED;

	// set breaker lifetime (at average of 3.5 ops/year, 100 seems reasonable)
	// @todo get data on residential breaker lifetimes (residential, low priority)
	c->tripsleft = 100;

	// add smartfuse
	c->smartfuse = new SMARTFUSE;
	c->smartfuse->powerRef = 0.0;
	c->smartfuse->vFactor = 1.0;
	c->smartfuse->vMin = 0.95;
	c->smartfuse->vMax = 1.05;

	return c;
}

void house_e::update_model(double dt)
{
	/* compute solar gains */
	//Qs = 0;
	incident_solar_radiation = 0;
	horizontal_diffuse_solar_radiation = 0;
	north_incident_solar_radiation = 0;
	north_west_incident_solar_radiation = 0;
	west_incident_solar_radiation = 0;
	south_west_incident_solar_radiation = 0;
	south_incident_solar_radiation = 0;
	south_east_incident_solar_radiation = 0;
	east_incident_solar_radiation = 0;
	north_east_incident_solar_radiation = 0;
	double number_of_quadrants = 0;

	// recalculate the constants of the ETP equations based off of the ETP parameters.
	if (Ca<=0)
		throw "air_thermal_mass must be positive";
	if (Cm<=0)
		throw "house_content_thermal_mass must be positive";
	if(Hm <= 0)
		throw "house_content_heat_transfer_coeff must be positive";
	if(Ua < 0)
		throw "UA must be positive";

	a = Cm*Ca/Hm;
	
	if (window_open == 1)
	{
		b = Cm*(10*Ua+Hm)/Hm+Ca;
		c = 10*Ua;
		c1 = -(10*Ua + Hm)/Ca;
	}
	else
	{
		b = Cm*(Ua+Hm)/Hm+Ca;
		c = Ua;
		c1 = -(Ua + Hm)/Ca;
	}

	c2 = Hm/Ca;
	double rr = sqrt(b*b-4*a*c)/(2*a);
	double r = -b/(2*a);
	r1 = r+rr;
	r2 = r-rr;

	if (window_open == 1)
	{
		A3 = Ca/Hm * r1 + (10*Ua+Hm)/Hm;
		A4 = Ca/Hm * r2 + (10*Ua+Hm)/Hm;
	}
	else
	{
		A3 = Ca/Hm * r1 + (Ua+Hm)/Hm;
		A4 = Ca/Hm * r2 + (Ua+Hm)/Hm;
	}

	//for (i=1; i<9; i++) //Compass points of pSolar include direct normal and diffuse radiation into one value
	//	incident_solar_radiation += pSolar[i];
	//	//Qs += pSolar[i];
	horizontal_diffuse_solar_radiation = 3.412*pSolar[0];
	north_incident_solar_radiation = 3.412*pSolar[1];
	north_east_incident_solar_radiation = 3.412*pSolar[2];
	east_incident_solar_radiation = 3.412*pSolar[3];
	south_east_incident_solar_radiation = 3.412*pSolar[4];
	south_incident_solar_radiation = 3.412*pSolar[5];
	south_west_incident_solar_radiation = 3.412*pSolar[6];
	west_incident_solar_radiation = 3.412*pSolar[7];
	north_west_incident_solar_radiation = 3.412*pSolar[8];

	
	if((include_solar_quadrant & 0x0002) == 0x0002){
		incident_solar_radiation += pSolar[1];
		incident_solar_radiation += pSolar[2]/2;
		incident_solar_radiation += pSolar[8]/2;
		if((include_solar_quadrant & 0x0001) == 0x0001){
			incident_solar_radiation += 2*pSolar[0];
		}
	}
	if((include_solar_quadrant & 0x0004) == 0x0004){
		incident_solar_radiation += pSolar[3];
		incident_solar_radiation += pSolar[2]/2;
		incident_solar_radiation += pSolar[4]/2;
		if((include_solar_quadrant & 0x0001) == 0x0001){
			incident_solar_radiation += 2*pSolar[0];
		}
	}
	if((include_solar_quadrant & 0x0008) == 0x0008){
		incident_solar_radiation += pSolar[5];
		incident_solar_radiation += pSolar[4]/2;
		incident_solar_radiation += pSolar[6]/2;
		if((include_solar_quadrant & 0x0001) == 0x0001){
			incident_solar_radiation += 2*pSolar[0];
		}
	}
	if((include_solar_quadrant & 0x0010) == 0x0010){
		incident_solar_radiation += pSolar[7];
		incident_solar_radiation += pSolar[6]/2;
		incident_solar_radiation += pSolar[8]/2;
		if((include_solar_quadrant & 0x0001) == 0x0001){
			incident_solar_radiation += 2*pSolar[0];
		}
	}

	incident_solar_radiation *= 3.412/8;// incident_solar_radiation is now in Btu/(h*sf)
	Qs = incident_solar_radiation*solar_heatgain_factor;//solar_heatgain_factor is the equivalent solar aperature spec in Rob's Sheet
	//Qs *= 3.412 * (gross_wall_area*window_wall_ratio) / 8.0 * (glazing_shgc * window_exterior_transmission_coefficient);
	//sol_inc = Qs/(gross_wall_area*window_wall_ratio*glazing_shgc*window_exterior_transmission_coefficient); // gives Qs in Btu/(hr*sf)


	if (Qs<0)
		throw "solar gain is negative";

	// split gains to air and mass
	Qi = total.heatgain - load.heatgain;
	Qa = Qh + (1-mass_internal_gain_fraction)*Qi + (1-mass_solar_gain_fraction)*Qs;
	Qm = mass_internal_gain_fraction*Qi + mass_solar_gain_fraction*Qs;

	if (window_open == 1)
		d = Qa + Qm + (10*Ua)*Tout;
	else
		d = Qa + Qm + (Ua)*Tout;
	Teq = d/c;

	/* compute next initial condition */
	dTa = c2*Tm + c1*Ta - (c1+c2)*Tout + Qa/Ca;
	k1 = (r2*Tair - r2*Teq - dTa)/(r2-r1);
	k2 = Tair - Teq - k1;
	//printf("update model %f %f %f\n",Tair,Teq,k1);

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
	/// @todo document COP calculation constants
	//const double heating_cop_adj = (-0.0063*(*pTout)+1.5984);
	//const double cooling_cop_adj = -(-0.0108*(*pTout)+2.0389);
	//const double heating_capacity_adj = (-0.0063*(*pTout)+1.5984);
	//const double cooling_capacity_adj = -(-0.0108*(*pTout)+2.0389);

	double heating_cop_adj=0;
	double cooling_cop_adj=0;
	double temp_temperature=0;
	double heating_capacity_adj=0;
	double cooling_capacity_adj=0;
	double temp_c;
	temp_c = 5*((*pTout) - 32)/9;
	
	if(heating_cop_curve == HC_DEFAULT){
		if(*pTout > 80){
			temp_temperature = 80;
			heating_cop_adj = heating_COP / (2.03914613 - 0.03906753*temp_temperature + 0.00045617*temp_temperature*temp_temperature - 0.00000203*temp_temperature*temp_temperature*temp_temperature);
		} else {
			heating_cop_adj = heating_COP / (2.03914613 - 0.03906753*(*pTout) + 0.00045617*(*pTout)*(*pTout) - 0.00000203*(*pTout)*(*pTout)*(*pTout));
		}
	}
	if(heating_cop_curve == HC_FLAT){
		heating_cop_adj = heating_COP;
	}
	if(heating_cop_curve == HC_LINEAR){//rated temperature is 47F/8.33C
		if(*pTout >= 47){
			heating_cop_adj = heating_COP;
		} else {
			heating_cop_adj = heating_COP*(*pTout/47);
		}
	}
	if(heating_cop_curve == HC_CURVED){
		gl_error("CURVED heating_cop_curve is not supported yet.");
		error_flag = 1;
	}

	if(cooling_cop_curve == CC_DEFAULT){
		if(*pTout < 40){
			temp_temperature = 40;
			cooling_cop_adj = cooling_COP / (-0.01363961 + 0.01066989*temp_temperature);
		} else {
			cooling_cop_adj = cooling_COP / (-0.01363961 + 0.01066989*(*pTout));
		}
	}
	if(cooling_cop_curve == CC_FLAT){
		cooling_cop_adj = cooling_COP;
	}
	if(cooling_cop_curve == CC_LINEAR){//rated temperature is at 95F/35C
		if(temp_c <= 35){
			cooling_cop_adj = cooling_COP;
		} else {
			cooling_cop_adj = cooling_COP*35/temp_c;
		}
	}
	if(cooling_cop_curve == CC_CURVED){
		gl_error("CURVE cooling_cop_curve is not supported yet.");
		error_flag = 1;
	}

	//if (*pTout < 40)
	//{
	//	heating_cop_adj = heating_COP / (2.03914613 - 0.03906753*(*pTout) + 0.00045617*(*pTout)*(*pTout) - 0.00000203*(*pTout)*(*pTout)*(*pTout));
	//	double temp_temperature = 40;
	//	cooling_cop_adj = cooling_COP / (-0.01363961 + 0.01066989*temp_temperature);
	//}
	//else if (*pTout > 80)
	//{
	//	cooling_cop_adj = cooling_COP / (-0.01363961 + 0.01066989*(*pTout));
	//	double temp_temperature = 80;
	//	heating_cop_adj = heating_COP / (2.03914613 - 0.03906753*temp_temperature + 0.00045617*temp_temperature*temp_temperature - 0.00000203*temp_temperature*temp_temperature*temp_temperature);
	//}
	//else
	//{
	//	cooling_cop_adj = cooling_COP / (-0.01363961 + 0.01066989*(*pTout));
	//	heating_cop_adj = heating_COP / (2.03914613 - 0.03906753*(*pTout) + 0.00045617*(*pTout)*(*pTout) - 0.00000203*(*pTout)*(*pTout)*(*pTout));
	//}
	adj_cooling_cop = cooling_cop_adj;
	adj_heating_cop = heating_cop_adj;

	//double heating_capacity_adj = design_heating_capacity*(0.34148808 + 0.00894102*(*pTout) + 0.00010787*(*pTout)*(*pTout)); 
	//double cooling_capacity_adj = design_cooling_capacity*(1.48924533 - 0.00514995*(*pTout));
	if(heating_cap_curve == HP_DEFAULT){
		heating_capacity_adj = design_heating_capacity*(0.34148808 + 0.00894102*(*pTout) + 0.00010787*(*pTout)*(*pTout));
	}
	if(heating_cap_curve == HP_FLAT){
		heating_capacity_adj = design_heating_capacity;
	}
	if(heating_cap_curve == HP_LINEAR){
		gl_error("LINEAR heating_cap_curve is not supported at this time");
		error_flag = 1;
	}
	if(heating_cap_curve == HP_CURVED){
		gl_error("CURVED heating _cap_curve is not supported at this time");
		error_flag = 1;
	}

	if(cooling_cap_curve == CP_DEFAULT){
		cooling_capacity_adj = design_cooling_capacity*(1.48924533 - 0.00514995*(*pTout));
	}
	if(cooling_cap_curve == CP_FLAT){
		cooling_capacity_adj = design_cooling_capacity;
	}
	if(cooling_cap_curve == CP_LINEAR){
		gl_error("LINEAR cooling_cap_curve is not supported at this time");
		error_flag = 1;
	}
	if(cooling_cap_curve == CP_CURVED){
		gl_error("CURVED cooling_cap_curve is not supported at this time");
		error_flag = 1;
	}


	adj_cooling_cap = cooling_capacity_adj;
	adj_heating_cap = heating_capacity_adj;
#pragma warning("house_e: add update_system voltage adjustment for heating")
	double voltage_adj = (((pCircuit_V[0]).Mag() * (pCircuit_V[0]).Mag()) / (240.0 * 240.0) * load.impedance_fraction + ((pCircuit_V[0]).Mag() / 240.0) * load.current_fraction + load.power_fraction);
	double voltage_adj_resistive = ((pCircuit_V[0]).Mag() * (pCircuit_V[0]).Mag()) / (240.0 * 240.0);
	
	//Only provide demand in if meter isn't out of service
	if (*pMeterStatus!=0)
	{
		// Set Qlatent to zero. Only gets updated if calculated.
		Qlatent = 0;

		// set system demand
		switch(heating_system_type){
			case HT_NONE:
			case HT_GAS:
				heating_demand = 0.0;
				break;
			case HT_RESISTANCE:
				heating_demand = design_heating_capacity*KWPBTUPH;
				break;
			case HT_HEAT_PUMP:
				if(system_mode == SM_AUX){ // only when we're 'running hot'
					heating_demand = aux_heat_capacity*KWPBTUPH;
				} else {
					heating_demand = heating_capacity_adj / heating_cop_adj * KWPBTUPH;
				}
				break;
		}
		switch(cooling_system_type){
			case CT_NONE: /* shouldn't've gotten here... */
				cooling_demand = 0.0;
				break;
			case CT_ELECTRIC:
				if (thermal_storage_present < 1)	//Thermal storage offline or not here
				{
					cooling_demand = cooling_capacity_adj / cooling_cop_adj * KWPBTUPH;
				}
				else	//Thermal storage online and in use
				{
					cooling_demand = 0.0;
				}
				break;
		}

		switch (system_mode) {
		case SM_HEAT:
			// turn the fan on
			if(fan_type != FT_NONE)
				fan_power = fan_design_power/1000.0; /* convert to kW */
			else
				fan_power = 0.0;

			//Ensure thermal storage is not being used right now (currently supports only cooling modes)
			thermal_storage_inuse = 0;

			//heating_demand = design_heating_capacity*heating_capacity_adj/(heating_COP * heating_cop_adj)*KWPBTUPH;
			//system_rated_capacity = design_heating_capacity*heating_capacity_adj;
			switch(heating_system_type){
				case HT_NONE: /* shouldn't've gotten here... */
					//heating_demand = 0.0;
					system_rated_capacity = 0.0;
					system_rated_power = 0.0;
					fan_power = 0.0; // turn it back off
					break;
				case HT_RESISTANCE:
					//heating_demand = design_heating_capacity*KWPBTUPH;
					system_rated_capacity = design_heating_capacity*voltage_adj_resistive + fan_power*BTUPHPKW*fan_heatgain_fraction;
					system_rated_power = heating_demand;
					break;
				case HT_HEAT_PUMP:
					//heating_demand = heating_capacity_adj / heating_cop_adj * KWPBTUPH;
					system_rated_capacity = heating_capacity_adj*voltage_adj + fan_power*BTUPHPKW*fan_heatgain_fraction;
					system_rated_power = heating_demand;
					break;
				case HT_GAS:
					heating_demand = 0.0;
					system_rated_capacity = design_heating_capacity + fan_power*BTUPHPKW*fan_heatgain_fraction;
					system_rated_power = heating_demand;
					break;
			}
			break;
		case SM_AUX:
			// turn the fan on
			if(fan_type != FT_NONE)
				fan_power = fan_design_power/1000.0; /* convert to kW */
			else
				fan_power = 0.0;

			//Ensure thermal storage is not being used right now
			thermal_storage_inuse = 0;

			switch(auxiliary_system_type){
				case AT_NONE: // really shouldn't've gotten here!
					//heating_demand = 0.0;
					system_rated_capacity = 0.0;
					system_rated_power = 0.0;
					break;
				case AT_ELECTRIC:
					//heating_demand = aux_heat_capacity*KWPBTUPH;
					system_rated_capacity = aux_heat_capacity*voltage_adj_resistive + fan_power*BTUPHPKW*fan_heatgain_fraction;
					system_rated_power = heating_demand;
					break;
			}
			break;
		case SM_COOL:
			// turn the fan on
			if(fan_type != FT_NONE)
				fan_power = fan_design_power/1000.0; /* convert to kW */
			else
				fan_power = 0.0;

			//cooling_demand = design_cooling_capacity*cooling_capacity_adj/(1+latent_load_fraction)/(cooling_COP * cooling_cop_adj)*(1+latent_load_fraction)*KWPBTUPH;
			//system_rated_capacity = -design_cooling_capacity/(1+latent_load_fraction)*cooling_capacity_adj;
			switch(cooling_system_type){
				case CT_NONE: /* shouldn't've gotten here... */
					//cooling_demand = 0.0;
					system_rated_capacity = 0.0;
					system_rated_power = 0.0;
					fan_power = 0.0; // turn it back off
					thermal_storage_inuse = 0;	//No cooling means thermal storage won't be doing anything here
					break;
				case CT_ELECTRIC:
					if (thermal_storage_present<1)	//If not 1, assumes thermal storage not available (not there, or discharged)
					{
						//cooling_demand = cooling_capacity_adj / cooling_cop_adj * KWPBTUPH;
						// DPC: the latent_load_fraction is not as great counted when humidity is low
						if(use_latent_heat == TRUE){
							system_rated_capacity = -cooling_capacity_adj / (1 + 0.1 + latent_load_fraction/(1 + exp(4-10*(*pRhout))))*voltage_adj + fan_power*BTUPHPKW*fan_heatgain_fraction;
							Qlatent = -cooling_capacity_adj*voltage_adj*((1/(1 + 0.1 + latent_load_fraction/(1 + exp(4-10*(*pRhout)))))-1);
						} else {
							system_rated_capacity = -cooling_capacity_adj*voltage_adj + fan_power*BTUPHPKW*fan_heatgain_fraction;
							Qlatent = 0;
						}
						system_rated_power = cooling_demand;
						thermal_storage_inuse = 0;	//Set the flag that no thermal energy storage is being used
					}
					else	//Thermal storage is present and online
					{
						system_rated_capacity = fan_power*BTUPHPKW*fan_heatgain_fraction;	//Only the fan is going right now, so it is the only power and the only heat gain into the system
						system_rated_power = cooling_demand;	//Should be 0.0 from above - basically handled inside the energy storage device
						thermal_storage_inuse = 1;	//Flag that thermal energy storage being utilized
					}
					break;
			}
			break;
		default: // SM_OFF or SM_UNKNOWN
			// two-speed systems use a little power at when off (vent mode)
			if(fan_type == FT_TWO_SPEED){
				fan_power = fan_design_power * fan_low_power_fraction / 1000.0; /* convert to kW */
			} else {
				fan_power = 0.0;
			}
			system_rated_capacity =  fan_power*BTUPHPKW*fan_heatgain_fraction;	// total heat gain of system
			system_rated_power = 0.0;					// total power drawn by system
			thermal_storage_inuse = 0;					//If the system is off, it isn't using thermal storage
			
		}

		/* calculate the power consumption */
		if(include_fan_heatgain == TRUE){
			load.total = system_rated_power + fan_power;
		} else {
			load.total = system_rated_power;
		}
		load.heatgain = system_rated_capacity;

		if(	(cooling_system_type == CT_ELECTRIC		&& system_mode == SM_COOL) ||
			(heating_system_type == HT_HEAT_PUMP	&& system_mode == SM_HEAT)) {
				load.power.SetRect(load.power_fraction * load.total.Re() , load.power_fraction * load.total.Re() * sqrt( 1 / (load.power_factor*load.power_factor) - 1) );
				load.admittance.SetRect(load.impedance_fraction * load.total.Re() , load.impedance_fraction * load.total.Re() * sqrt( 1 / (load.power_factor*load.power_factor) - 1) );
				load.current.SetRect(load.current_fraction * load.total.Re(), load.current_fraction * load.total.Re() * sqrt( 1 / (load.power_factor*load.power_factor) - 1) );
				
				// Motor losses that are related to the efficiency of the induction motor. These contribute to electric power
				// consumed, but are not incorporated into the heat flow equations.
				if (motor_model == MM_BASIC)
				{
					if (system_mode == SM_HEAT)
					{
						hvac_motor_real_loss = hvac_motor_loss_power_factor*(1 - hvac_motor_efficiency) * sqrt( design_heating_capacity*KWPBTUPH*design_heating_capacity*KWPBTUPH / (load.power_factor*load.power_factor*heating_COP*heating_COP) );
						hvac_motor_reactive_loss = sqrt( 1 / (hvac_motor_loss_power_factor*hvac_motor_loss_power_factor) - 1) * hvac_motor_real_loss;
					}					
					else if (system_mode == SM_COOL)
					{
						hvac_motor_real_loss = hvac_motor_loss_power_factor*(1 - hvac_motor_efficiency) * sqrt( design_cooling_capacity*KWPBTUPH*design_cooling_capacity*KWPBTUPH / (load.power_factor*load.power_factor*cooling_COP*cooling_COP) );
						hvac_motor_reactive_loss = sqrt( 1 / (hvac_motor_loss_power_factor*hvac_motor_loss_power_factor) - 1) * hvac_motor_real_loss;
					}

					load.admittance += complex(hvac_motor_real_loss,hvac_motor_reactive_loss);
				}
				else if (motor_model == MM_FULL)
					gl_warning("FULL motor model is not yet supported. No losses are assumed.");

		} else {
			//	gas heat & resistive heat -> fan power P and heating power Z
			//	else just fan & system_rated_power = 0
				load.power.SetRect(fan_power * fan_power_fraction, fan_power * fan_power_fraction * sqrt( 1 / (fan_power_factor * fan_power_factor) - 1));
				load.admittance.SetRect(system_rated_power + fan_power * fan_impedance_fraction, fan_power * fan_impedance_fraction * sqrt( 1 / (fan_power_factor * fan_power_factor) - 1));
				load.current.SetRect(fan_power * fan_current_fraction, fan_power * fan_current_fraction * sqrt( 1 / (fan_power_factor * fan_power_factor) - 1));
		}
	}//End if for meter in service
	else	//Meter not in service, zero HVAC
	{
		load.total = 0.0;
		load.power = 0.0;
		load.admittance = 0.0;
		load.current = 0.0;
		load.heatgain = 0.0;	//No HVAC gains
	}

	// update load
	hvac_load = load.total.Re() * (load.power_fraction + load.voltage_factor*(load.current_fraction + load.impedance_fraction*load.voltage_factor));
	if (system_mode == SM_COOL)
		last_cooling_load = hvac_load;
	else if (system_mode == SM_AUX || system_mode == SM_HEAT)
		last_heating_load = hvac_load;
	hvac_power = load.power + load.admittance*load.voltage_factor*load.voltage_factor + load.current*load.voltage_factor;
	// increment compressor_count?
	if(compressor_on){
		// should it be off?
		if(system_mode == SM_AUX || system_mode == SM_OFF){
			compressor_on = false;
		}
	} else {
		if(	(system_mode == SM_HEAT && heating_system_type == HT_HEAT_PUMP) ||
			(system_mode == SM_COOL && cooling_system_type == CT_ELECTRIC)){
			compressor_on = true;
			++compressor_count;
		}
	}
}

/**  Updates the aggregated power from all end uses, calculates the HVAC kWh use for the next synch time
**/
TIMESTAMP house_e::presync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	const double dt = (double)((t1-t0)*TS_SECOND)/3600;
	CIRCUIT *c;
	extern bool ANSI_voltage_check;

	extern bool enable_subsecond_models;
	extern OBJECT **delta_objects;
	extern FUNCTIONADDR *delta_functions;
	extern FUNCTIONADDR *post_delta_functions;
	extern int res_object_count;
	extern int res_object_current;
	extern void allocate_deltamode_arrays(void);

	//Zero the accumulator
	load_values[0][0] = load_values[0][1] = load_values[0][2] = 0.0;
	load_values[1][0] = load_values[1][1] = load_values[1][2] = 0.0;
	load_values[2][0] = load_values[2][1] = load_values[2][2] = 0.0;

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
			if (window_open == 1)
				Tmaterials = A3*e1 + A4*e2 + Qm/Hm + (Qm+Qa)/(10*Ua) + Tout;
			else
				Tmaterials = A3*e1 + A4*e2 + Qm/Hm + (Qm+Qa)/(Ua) + Tout;
		}
	}

	if (window_openings == TRUE) 
	{
		if (window_high_temp <= window_low_temp)
		{
			gl_error("The high window temperature cutoff must be greater than the low window temperature cutoff.");
			/* TROUBLESHOOT
			The window_high_temperature_cutoff must be a value greater than the window_low_temperature_cutoff.  These
			two values define a temperature "deadband" in which there is a possibility that the window will "open".  
			Please take	a look at your specified values and ensure that the high is greater than the low.
			*/
		}
		if (Tout >= window_low_temp && Tout <= window_high_temp)
		{
			// First time through the model since we hit this temp band
			if ( window_first_time_through == 1 )
			{
				window_first_time_through = 0;

				double random_val = gl_random_uniform(RNGSTATE,0,1);
				double calc_val = window_a * Tout * Tout + window_b * Tout + window_c;

				if (calc_val > 1.0) 
				{
					gl_verbose("Function value for window_opening calculation was greater than 1 (%.2f). Capping value at 1.", calc_val); 
					/* TROUBLESHOOT
					The function to described whether the window should open or not is limited to the range [0,1], as it is compared to a
					uniformly distributed random number in the range [0,1]. If your function (using a=window_quadratic_coefficient,
					b=window_linear_coefficient, and c=window_constant_coefficient), f(a,b,c) = a*Tout^2 + b*Tout + c, calculates a value
					outside the range of [0,1], it will be capped at either side.
					*/
					calc_val = 1.01;
				}
				else if (calc_val < 0.)
				{
					gl_verbose("Function value for window_opening calculation was less than 0 (%.2f). Capping value at 0.", calc_val); 
					/* TROUBLESHOOT
					The function to described whether the window should open or not is limited to the range [0,1], as it is compared to a
					uniformly distributed random number in the range [0,1]. If your function (using a=window_quadratic_coefficient,
					b=window_linear_coefficient, and c=window_constant_coefficient), f(a,b,c) = a*Tout^2 + b*Tout + c, calculates a value
					outside the range of [0,1], it will be capped at either side.
					*/
					calc_val = -0.01;
				}

				if (random_val <= calc_val)
				{
					window_open = 1;
					last_temperature = Tout;
				}
				else
				{
					window_open = 0;
				}
			}
			// We've changed enough that we need to update the model
			else if ( abs(last_temperature - Tout) > window_temp_delta )
			{	
				double random_val = gl_random_uniform(RNGSTATE,0,1);
				double calc_val = window_a * Tout * Tout + window_b * Tout + window_c;

				if (calc_val > 1.0) 
				{
					gl_verbose("Function value for window_opening calculation was greater than 1 or less than 0 (%.2f). Limiting to that range", calc_val); 
					calc_val = 1.01;
				}
				else if (calc_val < 0.)
				{
					gl_verbose("Function value for window_opening calculation was greater than 1 or less than 0 (%.2f). Limiting to that range", calc_val); 
					calc_val = -0.01;
				}

				if (random_val <= calc_val)
				{
					window_open = 1;
					last_temperature = Tout;
				}
				else
				{
					window_open = 0;
					last_temperature = Tout;
				}
			}		
		}
		else
		{
			window_open = 0; // no, it won't be open outside the temperature bounds
			last_temperature = Tout;
			window_first_time_through = 1;
		}
	}

	/* update all voltage factors */
	for (c=panel.circuits; c!=NULL; c=c->next)
	{
		// implement smartfuse controller
		if ( c->smartfuse != NULL )
			update_smartfuse(c);

		// get circuit type
		int n = (int)c->type;
		if (n<0 || n>2)
			GL_THROW("%s:%d circuit %d has an invalid circuit type (%d)", obj->oclass->name, obj->id, c->id, (int)c->type);
		c->pLoad->voltage_factor = c->pV->Mag() / ((c->pLoad->config&EUC_IS220) ? 240 : 120) * c->smartfuse->vFactor;
		if ((c->pLoad->voltage_factor > 1.06 || c->pLoad->voltage_factor < 0.88) && (ANSI_voltage_check==true))
			gl_warning("%s - %s:%d is outside of ANSI standards (voltage = %.0f percent of nominal 120/240)", obj->name, obj->oclass->name,obj->id,c->pLoad->voltage_factor*100);
	}

	//First run allocation - handles overall residential allocation as well
	if (deltamode_inclusive == true)	//Only call if deltamode is even enabled
	{
		//Overall call -- only do this on the first run
		if (deltamode_registered == false)
		{
			if ((res_object_current == -1) && (delta_objects==NULL) && (enable_subsecond_models==true))
			{
				//Call the allocation routine
				allocate_deltamode_arrays();
			}

			//Check limits of the array
			if (res_object_current>=res_object_count)
			{
				GL_THROW("Too many objects tried to populate deltamode objects array in the residential module!");
				/*  TROUBLESHOOT
				While attempting to populate a reference array of deltamode-enabled objects for the residential
				module, an attempt was made to write beyond the allocated array space.  Please try again.  If the
				error persists, please submit a bug report and your code via the trac website.
				*/
			}

			//Add us into the list
			delta_objects[res_object_current] = obj;

			//Map up the function for interupdate
			delta_functions[res_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"interupdate_res_object"));

			//Make sure it worked
			if (delta_functions[res_object_current] == NULL)
			{
				GL_THROW("Failure to map deltamode function for device:%s",obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Map up the function for postupdate
			post_delta_functions[res_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"postupdate_res_object"));

			//Make sure it worked
			if (post_delta_functions[res_object_current] == NULL)
			{
				GL_THROW("Failure to map post-deltamode function for device:%s",obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the postupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Update pointer
			res_object_current++;

			//Deflag us
			deltamode_registered = true;

		}//End first timestep runs
	}//End Deltamode code

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
	
	if(!heat_start){
		// force an update of the outside temperature, even if we don't do anything with it
		outside_temperature = *pTout;
		outdoor_rh = *pRhout*100;
	}
	/* update HVAC power before panel sync */
	if (t0==simulation_beginning_time || t1>t0){
		outside_temperature = *pTout;
		outdoor_rh = *pRhout*100;

		// update the state of the system
		update_system(dt1);
		if(error_flag == 1){
			return TS_INVALID;
		}
	}

	t2 = sync_enduses(t0, t1);
#ifdef _DEBUG
//		gl_debug("house %s (%d) sync_enduses event at '%s'", obj->name, obj->id, gl_strftime(t2));
#endif

	// get the fractions to properly apply themselves
//	gl_enduse_sync(&(residential_enduse::load),t1);

	// sync circuit panel
	t = sync_panel(t0,t1); 
	if (t < t2) {
		t2 = t;
#ifdef _DEBUG
//			gl_debug("house %s (%d) sync_panel event '%s'", obj->name, obj->id, gl_strftime(t2));
#endif
	}

	if ((t0==simulation_beginning_time || t1>t0) || (!heat_start)){

		// update the model of house
		update_model(dt1);
		heat_start = true;

	}

	// determine temperature of next event
	update_Tevent();

	/* solve for the time to the next event */
	double dt2;
	
	/* dt2 is for the next thermal event ... avoid calculating the next time to a given
		temperature until the cycle time has elapse.
	 */
	if(re_override == OV_NORMAL){
		if(thermostat_off_cycle_time == -1 && thermostat_on_cycle_time == -1){
			// this is always false if thermostat_cycle_time == 0
			if(t < thermostat_last_cycle_time + thermostat_cycle_time){
				dt2 = (double)(thermostat_last_cycle_time + thermostat_cycle_time);
			} else {
				dt2 = e2solve(k1,r1,k2,r2,Teq-Tevent)*3600.0;
			}
		} else if(thermostat_off_cycle_time >= 0 && thermostat_on_cycle_time >= 0){
			if(thermostat_last_off_cycle_time > thermostat_last_on_cycle_time){
				if(t < thermostat_last_off_cycle_time + thermostat_off_cycle_time){
					dt2 = (double)(thermostat_last_off_cycle_time + thermostat_off_cycle_time);
				} else {
					dt2 = e2solve(k1,r1,k2,r2,Teq-Tevent)*3600;
				}
			} else if(thermostat_last_off_cycle_time < thermostat_last_on_cycle_time){
				if(t < thermostat_last_on_cycle_time + thermostat_on_cycle_time){
					dt2 = (double)(thermostat_last_on_cycle_time + thermostat_on_cycle_time);
				} else {
					dt2 = e2solve(k1,r1,k2,r2,Teq-Tevent)*3600;
				}
			} else {
				if(t < thermostat_last_cycle_time + thermostat_cycle_time){
					dt2 = (double)(thermostat_last_cycle_time + thermostat_cycle_time);
				} else {
					dt2 = e2solve(k1,r1,k2,r2,Teq-Tevent)*3600;
				}
			}
		} else {
			gl_error("Both the thermostat_off_cycle_time and the thermostat_on_cycle_time must be greater than zero.");
			return TS_INVALID;
		}
	} else {
		dt2 = TS_NEVER;
	}

	// if no solution is found or it has already occurred
	if (isnan(dt2) || !isfinite(dt2) || dt2<0)
	{
#ifdef _DEBUG
//		gl_debug("house %s (%d) time to next event is indeterminate", obj->name, obj->id);
#endif
		// try again in 1 second if there is a solution in the future
		//if (sgn(dTair)==sgn(Tevent-Tair) && Tevent) 
		//	if (t2>t1) t2 = t1+1;
	}

	// if the solution is less than time resolution
	else if (dt2<TS_SECOND)
	{	
		// need to do a second pass to get next state
		t = t1+1; if (t<t2) t2 = t;
#ifdef _DEBUG
//		gl_debug("house %s (%d) time to next event is less than time resolution", obj->name, obj->id);
#endif
	}
	else
	{
		// next event is found
		t = t1+(TIMESTAMP)(ceil(dt2)*TS_SECOND); if (t<t2) t2 = t;
#ifdef _DEBUG
//		gl_debug("house %s (%d) time to next event is %.2f hrs", obj->name, obj->id, dt2/3600);
#endif
	}

#ifdef _DEBUG
	char tbuf[64];
	gl_printtime(t2, tbuf, 64);
//		gl_debug("house %s (%d) next event at '%s'", obj->name, obj->id, tbuf);
#endif

	// enforce dwell time
	if (t2!=TS_NEVER)
	{
		TIMESTAMP t = (TIMESTAMP)(ceil((t2<0 ? -t2 : t2)/system_dwell_time)*system_dwell_time);
		t2 = (t2<0 ? t : -t);
	}
	//gl_output("glsovers returns %f.",dt2);
	//Update the off-return value
	return t2;
}

/** Removes load contributions from parent object **/
TIMESTAMP house_e::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);

	// compute line currents and post to meter
	if (obj->parent != NULL)
		wlock(obj->parent);

	//Post accumulations up to parent meter/node
	//Update power
	pPower[0] -= load_values[0][0];
	pPower[1] -= load_values[0][1];
	pPower[2] -= load_values[0][2];
	
	//Current
	pLine_I[0] -= load_values[1][0];
	pLine_I[1] -= load_values[1][1];
	pLine_I[2] -= load_values[1][2];
	//Neutral not handled in here, since it was always zero anyways

	//Admittance
	pShunt[0] -= load_values[2][0];
	pShunt[1] -= load_values[2][1];
	pShunt[2] -= load_values[2][2];

	if (obj->parent != NULL)
		wunlock(obj->parent);

	return TS_NEVER;
}


void house_e::update_Tevent()
{
	OBJECT *obj = OBJECTHDR(this);

	// Tevent is based on temperature bracket and assumes state is correct
	switch(system_mode) {

	case SM_HEAT: 
		if (dTair > 0) // temperature rising actively
			Tevent = TheatOff;
		else if (auxiliary_strategy == AX_DEADBAND) // temperature is falling
			Tevent = TauxOn;
		break;
	case SM_AUX:  // temperature, we hope, is rising
		Tevent = TheatOff;
		break;
	case SM_COOL: // temperature falling actively
		Tevent = TcoolOff;
		break;

	default: // temperature floating passively
		if (dTair<0) // falling
			Tevent = TheatOn;
		else if (dTair>0) // rising 
			//Tevent = ( (system_type&ST_AC) ? TcoolOn : warn_high_temp) ;
			Tevent = ( cooling_system_type != CT_NONE ? TcoolOn : warn_high_temp );
		else
			Tevent = Tair;
		break;
	}
}

/** The PLC control code for house_e thermostat.  The heat or cool mode is based
    on the house_e air temperature, thermostat setpoints and deadband.  This
    function will update T<mode>{On,Off} as necessary to maintain the setpoints.
**/
TIMESTAMP house_e::sync_thermostat(TIMESTAMP t0, TIMESTAMP t1)
{
	double terr = dTair/3600; // this is the time-error of 1 second uncertainty
	bool turned_on = false, turned_off = false;

	// only update the T<mode>{On,Off} is the thermostat is full
	if (thermostat_control==TC_FULL)
	{
		double tdead = thermostat_deadband/2;
		TcoolOn = cooling_setpoint+tdead;
		TcoolOff = cooling_setpoint-tdead;
		TheatOn = heating_setpoint-tdead;
		TheatOff = heating_setpoint+tdead;
		TauxOn = TheatOn-aux_heat_deadband;
	}

	// check for thermostat cycle lockout
	if(thermostat_off_cycle_time == -1 && thermostat_on_cycle_time == -1){
		if(t1 < thermostat_last_cycle_time + thermostat_cycle_time){
			return (thermostat_last_cycle_time + thermostat_cycle_time); // next time will be calculated in sync_model
		}
	} else if(thermostat_off_cycle_time >=0 && thermostat_on_cycle_time >=0){
		if(thermostat_last_off_cycle_time > thermostat_last_on_cycle_time){
			if(t1 < thermostat_last_off_cycle_time + thermostat_off_cycle_time){
				return (thermostat_last_off_cycle_time + thermostat_off_cycle_time); // next time will be calculated in sync_model
			}
		} else if(thermostat_last_off_cycle_time < thermostat_last_on_cycle_time){
			if(t1 < thermostat_last_on_cycle_time + thermostat_on_cycle_time){
				return (thermostat_last_on_cycle_time + thermostat_on_cycle_time); // next time will be calculated in sync_model
			}
		} else {
			if(t1 < thermostat_last_cycle_time + thermostat_cycle_time){
				return (thermostat_last_cycle_time + thermostat_cycle_time); // next time will be calculated in sync_model
			}
		}
	}

	if (window_openings == TRUE) 
	{
		if (window_open == 1)
		{
			this->re_override = OV_OFF;
			dlc_offset = 20;
		}
		else
		{
			dlc_offset = 0;
		}
	}

	// skip the historisis and turn on or off, if the HVAC is in a state where it _could_ be on or off.
	if (this->re_override == OV_ON){
		if (dlc_offset == 0.0)
		{
			TcoolOn = TcoolOff;
			TheatOn = TheatOff;
			re_override = OV_NORMAL;
		}
		else // this will let the DLC from passive controller adjust the cooling/heating without adjusting setpoints
		{
			switch(last_system_mode){
				case SM_HEAT:
				case SM_AUX:
					TcoolOn = TcoolOn + dlc_offset; 
					TcoolOff = TcoolOff + dlc_offset;
					TheatOn = TheatOff + dlc_offset;
					TheatOff = TheatOff + dlc_offset;
					break;
				case SM_OFF: //Let's make the assumption that cooling wins in this case.
				case SM_COOL:
					TcoolOn = TcoolOff - dlc_offset; 
					TcoolOff = TcoolOff - dlc_offset;
					TheatOn = TheatOn - dlc_offset;
					TheatOff = TheatOff - dlc_offset;
					break;
			}
		}
//		thermostat_last_cycle_time = gl_globalclock - thermostat_cycle_time - 1;
	} else if(this->re_override == OV_OFF){
		if (dlc_offset == 0.0)
		{
			TcoolOff = TcoolOn;
			TheatOff = TheatOn;
			re_override = OV_NORMAL;
		}
		else // this will let the DLC from passive controller adjust the cooling/heating without adjusting setpoints
		{
			TcoolOn = TcoolOn + dlc_offset;
			TcoolOff = TcoolOn + dlc_offset;
			TheatOn = TheatOff - dlc_offset;
			TheatOff = TheatOn - dlc_offset;
		}
//		thermostat_last_cycle_time = gl_globalclock - thermostat_cycle_time - 1;
	} 

	if(t0 < thermostat_last_cycle_time + last_mode_timer){
		last_system_mode = SM_OFF;
	}

	switch(last_system_mode){
		case SM_HEAT:
		case SM_AUX:
			if (TcoolOff<TheatOff)
				TcoolOff = TheatOff;
			if (TcoolOn<TcoolOff+thermostat_deadband)
				TcoolOn = TcoolOff+thermostat_deadband;
			break;
		case SM_OFF: //Let's make the assumption that cooling wins in this case.
		case SM_COOL:
			if (TcoolOff<TheatOff)
				TheatOff = TcoolOff;
			if (TheatOff<TheatOn-thermostat_deadband)
				TheatOff = TheatOn-thermostat_deadband;
			if (TauxOn<TheatOn-aux_heat_deadband)
				TauxOn = TheatOn-aux_heat_deadband;
			break;
	}

	// check for deadband overlap
	if (TcoolOff<TheatOff && cooling_system_type!=CT_NONE)
	{
		char buffer[64];
		gl_error("%s: thermostat setpoints deadbands overlap (TcoolOff=%.1f < TheatOff=%.1f)", gl_name(OBJECTHDR(this),buffer,sizeof(buffer)), TcoolOff, TheatOff);
		return TS_INVALID;
	}

	// unknown mode treated changed to off
	if(system_mode == SM_UNKNOWN)
	{
		char buffer[64];
		gl_warning("%s: system_mode was unknown, changed to off", gl_name(OBJECTHDR(this),buffer,sizeof(buffer)));
		system_mode = SM_OFF;
	}
	
	/* rationale behind thermostat_last_cycle_time:
		at this point, the system's handling PLC code, between presync and sync. t0 is when
		the temperature was updated from, and t1 is "now".  Any changes for "now" must operate
		off of t1. -mhauer
	*/
	// change control mode if necessary

	DATETIME t_next;
	gl_localtime(t1,&t_next);

	if (thermostat_control!=TC_NONE)
	{
		switch(system_mode) {
		case SM_HEAT:
			/* if (aux deadband OR timer tripped) AND below aux lockout, go auxiliary */
			if(thermostat_mode == TM_HEAT || thermostat_mode == TM_AUTO){ //heating is allowed
				if(re_override == OV_NORMAL){
					if ( auxiliary_system_type != AT_NONE	 && 
						((auxiliary_strategy & AX_DEADBAND	 && Tair < TauxOn)
						 || (auxiliary_strategy & AX_TIMER	 && t0 >= thermostat_last_cycle_time + aux_heat_time_delay))
						 || (auxiliary_strategy & AX_LOCKOUT && *pTout <= aux_heat_temp_lockout)
						){
						last_system_mode = system_mode = SM_AUX;
						power_state = PS_ON;
						thermostat_last_cycle_time = t1;
					} else if(Tair > TheatOff - terr/2){
						system_mode = SM_OFF;
						power_state = PS_OFF;
						thermostat_last_cycle_time = t1;
						thermostat_last_off_cycle_time = t1;
						turned_off = true;
					}
				} else if(re_override == OV_OFF){
					system_mode = SM_OFF;
					power_state = PS_OFF;
					thermostat_last_cycle_time = t1;
					thermostat_last_off_cycle_time = t1;
					turned_off = true;
				}
			} else { //heating is not allowed
				system_mode = SM_OFF;
				power_state = PS_OFF;
				thermostat_last_cycle_time = t1;
				thermostat_last_off_cycle_time = t1;
				turned_off = true;
			}
			break;
		case SM_AUX:
			if(thermostat_mode == TM_HEAT || thermostat_mode == TM_AUTO){ //heating is allowed
				if((Tair > TheatOff - terr/2 && re_override == OV_NORMAL) || (re_override == OV_OFF)){
					system_mode = SM_OFF;
					power_state = PS_OFF;
					thermostat_last_cycle_time = t1;
					thermostat_last_off_cycle_time = t1;
					turned_off = true;
				} else if(re_override == OV_OFF){
					system_mode = SM_OFF;
					power_state = PS_OFF;
					thermostat_last_cycle_time = t1;
					thermostat_last_off_cycle_time = t1;
					turned_off = true;
				}
			} else { //heating is not allowed
				system_mode = SM_OFF;
				power_state = PS_OFF;
				thermostat_last_cycle_time = t1;
				thermostat_last_off_cycle_time = t1;
				turned_off = true;
			}
			break;
		case SM_COOL:
			if(thermostat_mode == TM_COOL || thermostat_mode == TM_AUTO){ //cooling is allowed
				if((Tair < TcoolOff - terr/2 && re_override == OV_NORMAL) || (re_override == OV_OFF)){
					system_mode = SM_OFF;
					power_state = PS_OFF;
					thermostat_last_cycle_time = t1;
					thermostat_last_off_cycle_time = t1;
					turned_off = true;
				}
			} else { //cooling is not allowed
				system_mode = SM_OFF;
				power_state = PS_OFF;
				thermostat_last_cycle_time = t1;
				thermostat_last_off_cycle_time = t1;
				turned_off = true;
			}
			break;
		case SM_OFF:
			if((Tair > TcoolOn - terr/2) && (cooling_system_type != CT_NONE) && (thermostat_mode == TM_AUTO || thermostat_mode == TM_COOL))
			{
				last_system_mode = system_mode = SM_COOL;
				power_state = PS_ON;
				thermostat_last_cycle_time = t1;
				thermostat_last_on_cycle_time = t1;
				turned_on = true;
			}
			else if(Tair < TheatOn - terr/2 && (thermostat_mode == TM_AUTO || thermostat_mode == TM_HEAT))
			{
				//if (outside_temperature < aux_cutin_temperature)
				if (Tair < TauxOn && 
					(auxiliary_system_type != AT_NONE) && // turn on aux if we have it
					((auxiliary_strategy & AX_DEADBAND) || // turn aux on if deadband is set
					 (auxiliary_strategy & AX_LOCKOUT && *pTout <= aux_heat_temp_lockout))) // If the air of the house is 2x outside the deadband range, it needs AUX help
				{
					last_system_mode = system_mode = SM_AUX;
					power_state = PS_ON;
					thermostat_last_cycle_time = t1;
					thermostat_last_on_cycle_time = t1;
					turned_on = true;
				}
				else
				{
					last_system_mode = system_mode = SM_HEAT;
					power_state = PS_ON;
					thermostat_last_cycle_time = t1;
					thermostat_last_off_cycle_time = t1;
					turned_on = true;
				}
			}
			break;
		}
	}
	
	if(turned_on){
		if(hvac_last_off != 0){
			hvac_period_off = (double)(t1 - hvac_last_off)/60.0;
		}
		hvac_last_on = t1;
	}
	if(turned_off){
		if(hvac_last_on != 0){
			hvac_period_on = (double)(t1 - hvac_last_on)/60.0;
		}
		hvac_last_off = t1;
	}
	if(hvac_period_on != 0.0 && hvac_period_off != 0){
		hvac_period_length = hvac_period_on + hvac_period_off;
		hvac_duty_cycle = (double)hvac_period_on / (double)hvac_period_length;
	}

	if ( system_mode == SM_HEAT && (heating_system_type == HT_HEAT_PUMP || heating_system_type == HT_RESISTANCE || heating_system_type == HT_GAS) )
	{
		is_AUX_on = is_COOL_on = 0;
		is_HEAT_on = 1;
	}
	else if ( system_mode == SM_COOL && cooling_system_type == CT_ELECTRIC)
	{
		is_AUX_on = is_HEAT_on = 0;
		is_COOL_on = 1;
	}
	else if ( system_mode == SM_AUX && auxiliary_system_type == AT_ELECTRIC)
	{
		is_COOL_on = is_HEAT_on = 0;
		is_AUX_on = 1;
	}
	else
		is_COOL_on = is_HEAT_on = is_AUX_on = 0;

	return TS_NEVER;
}

TIMESTAMP house_e::sync_panel(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	OBJECT *obj = OBJECTHDR(this);

	// clear accumulator
	if((t0 >= simulation_beginning_time && t1 > t0) || (!heat_start)){
		total.heatgain = 0;
	}
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
			t2 = t1; // must immediately reevaluate devices affected
			gl_debug("house_e:%d panel breaker %d closed", obj->id, c->id);
		}

		// if breaker is closed
		if (c->status==BRK_CLOSED)
		{
			// compute circuit current
			if (((c->pV)->Mag() == 0) || (*pMeterStatus==0))	//Meter offline or voltage 0
			{
				gl_debug("house_e:%d circuit %d (enduse %s) voltage is zero", obj->id, c->id, c->pLoad->name);

				if (*pMeterStatus==0)	//If we've been disconnected, still apply latent load heat
				{
					if((t0 >= simulation_beginning_time && t1 > t0) || (!heat_start)){
						total.heatgain += c->pLoad->heatgain;
					}
				}
				continue;
			}
			
			//Current flow is based on the actual load, not nominal load
			complex actual_power = c->pLoad->power + (c->pLoad->current + c->pLoad->admittance * c->pLoad->voltage_factor)* c->pLoad->voltage_factor;
			complex current = ~(actual_power*1000 / *(c->pV)); 

			// check breaker
			if (current.Mag()>c->max_amps)
			{
				// probability of breaker failure increases over time
				if (c->tripsleft>0 && gl_random_bernoulli(RNGSTATE,1/(c->tripsleft--))==0)
				{
					// breaker opens
					c->status = BRK_OPEN;

					// average five minutes before reclosing, exponentially distributed
					c->reclose = t1 + (TIMESTAMP)(gl_random_exponential(RNGSTATE,1/300.0)*TS_SECOND); 
					gl_debug("house_e:%d circuit breaker %d tripped - enduse %s overload at %.0f A", obj->id, c->id,
						c->pLoad->name, current.Mag());
				}

				// breaker fails from too frequent operation
				else
				{
					c->status = BRK_FAULT;
					c->reclose = TS_NEVER;
					gl_warning("house_e:%d, %s circuit breaker %d failed - enduse %s is no longer running", obj->id, obj->name, c->id, c->pLoad->name);
				}

				// must immediately reevaluate everything
				t2 = t1; 
			}

			// add to panel current
			else
			{
				//Convert values appropriately - assume nominal voltages of 240 and 120 (0 degrees)
				//All values are given in kW, so convert to normal

				if (n==0)	//1-2 240 V load
				{
					load_values[0][2] += c->pLoad->power * 1000.0;
					load_values[1][2] += ~(c->pLoad->current * 1000.0 / 240.0);
					load_values[2][2] += ~(c->pLoad->admittance * 1000.0 / (240.0 * 240.0));
				}
				else if (n==1)	//2-N 120 V load
				{
					load_values[0][1] += c->pLoad->power * 1000.0;
					load_values[1][1] += ~(c->pLoad->current * 1000.0 / 120.0);
					load_values[2][1] += ~(c->pLoad->admittance * 1000.0 / (120.0 * 120.0));
				}
				else	//n has to equal 2 here (checked above) - 1-N 120 V load
				{
					load_values[0][0] += c->pLoad->power * 1000.0;
					load_values[1][0] += ~(c->pLoad->current * 1000.0 / 120.0);
					load_values[2][0] += ~(c->pLoad->admittance * 1000.0 / (120.0 * 120.0));
				}

				//load_values[0][1] += c->pLoad->power * 1000.0;
				//load_values[1][1] += ~(c->pLoad->current * 1000.0 / V);
				//load_values[2][1] += ~(c->pLoad->admittance * 1000.0 / (V*V));

				total.total += c->pLoad->total;
				total.power += c->pLoad->power;
				total.current += c->pLoad->current;
				total.admittance += c->pLoad->admittance;
				if((t0 != 0 && t1 > t0) || (!heat_start)){
					total.heatgain += c->pLoad->heatgain;
				}
				c->reclose = TS_NEVER;
			}
		}

		// sync time
		if (t2 > c->reclose)
			t2 = c->reclose;
	}
	/* using an enduse structure for the total is more a matter of having all the values add up for the house,
	 * and it should not sync the struct! ~MH */
	//TIMESTAMP t = gl_enduse_sync(&total,t1); if (t<t2) t2 = t;

	total_load = total.total.Mag();

	// compute line currents and post to meter
	if (obj->parent != NULL)
		wlock(obj->parent);

	//Post accumulations up to parent meter/node
	//Update power
	pPower[0] += load_values[0][0];
	pPower[1] += load_values[0][1];
	pPower[2] += load_values[0][2];
	
	//Current
	pLine_I[0] += load_values[1][0];
	pLine_I[1] += load_values[1][1];
	pLine_I[2] += load_values[1][2];
	//Neutral assumed 0, since it was anyways

	//Admittance
	pShunt[0] += load_values[2][0];
	pShunt[1] += load_values[2][1];
	pShunt[2] += load_values[2][2];

	if (obj->parent != NULL)
		wunlock(obj->parent);

	return t2;
}

TIMESTAMP house_e::sync_enduses(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	IMPLICITENDUSE *eu;
	//OBJECT *obj = OBJECTHDR(this);
	//char one[128], two[128];
	for (eu=implicit_enduse_list; eu!=NULL; eu=eu->next)
	{
		TIMESTAMP t = 0;
		t = gl_enduse_sync(&(eu->load),t1);
		if (t<t2) t2 = t;
	}
	//DATETIME dt1, dt2;
	//gl_localtime(t1,&dt1);
	//gl_strftime(&dt1, two, 127);
	//gl_localtime(t2,&dt2);
	//gl_strftime(&dt2, one, 127);
	//gl_verbose("house_e:%d ~ sync_eu %s at %s", obj->id, one, two);
	//if(0 == strcmp("(invalid time)", gl_strftime(t2))){
		//gl_verbose("TAKE NOTE OF THIS TIMESTEP");
	//}
	return t2;
}

void house_e::check_controls(void)
{
	char buffer[256];
	if (warn_control)
	{
		OBJECT *obj = OBJECTHDR(this);
		/* check for air temperature excursion */
		if (Tair<warn_low_temp || Tair>warn_high_temp)
		{
			gl_warning("house_e:%d (%s) air temperature excursion (%.1f degF) at %s", 
				obj->id, obj->name?obj->name:"anonymous", Tair, gl_strftime(obj->clock, buffer, 255));
		}

		/* check for mass temperature excursion */
		if (Tmaterials<warn_low_temp || Tmaterials>warn_high_temp)
		{
			gl_warning("house_e:%d (%s) mass temperature excursion (%.1f degF) at %s", 
				obj->id, obj->name?obj->name:"anonymous", Tmaterials, gl_strftime(obj->clock, buffer, 255));
		}

		/* check for heating equipment sizing problem */
		if ((system_mode==SM_HEAT || system_mode==SM_AUX) && Teq<heating_setpoint)
		{
			gl_warning("house_e:%d (%s) heating equipement undersized at %s", 
				obj->id, obj->name?obj->name:"anonymous", gl_strftime(obj->clock, buffer, 255));
		}

		/* check for cooling equipment sizing problem */
		else if (system_mode==SM_COOL &&
//			(system_type&ST_AC) &&
			(cooling_system_type != CT_NONE) &&
			Teq>cooling_setpoint)
		{
			gl_warning("house_e:%d (%s) cooling equipement undersized at %s", 
				obj->id, obj->name?obj->name:"anonymous", gl_strftime(obj->clock, buffer, 255));
		}

		/* check for invalid event temperature */
		if ((dTair>0 && Tevent<Tair) || (dTair<0 && Tevent>Tair))
		{
			char mode_buffer[1024];
			gl_warning("house_e:%d (%s) possible control problem (system_mode %s) -- Tevent-Tair mismatch with dTair (Tevent=%.1f, Tair=%.1f, dTair=%.1f) at %s", 
				obj->id, obj->name?obj->name:"anonymous", gl_getvalue(obj,"system_mode", mode_buffer, 1023)==NULL?"ERR":mode_buffer, Tevent, Tair, dTair, gl_strftime(obj->clock, buffer, 255));
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

bool *house_e::get_bool(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_bool)
		return NULL;
	return (bool*)GETADDR(obj,p);
}

int *house_e::get_enum(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_enumeration)
		return NULL;
	return (int*)GETADDR(obj,p);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE house_e::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	OBJECT *obj = OBJECTHDR(this);

	//Right now, houses don't really support deltamode.  This is a half-kludge to get them to play
	//nice with powerflow.  When they did support it, that code would go here.

	//On the very first run -- re-accumulate everything
	if ((iteration_count_val == 0) && (delta_time == 0))
	{
		//Basically replicate the panel posting from sync, right now
		// compute line currents and post to meter
		if (obj->parent != NULL)
			wlock(obj->parent);

		//Post accumulations up to parent meter/node
		//Update power
		pPower[0] += load_values[0][0];
		pPower[1] += load_values[0][1];
		pPower[2] += load_values[0][2];

		//Current
		pLine_I[0] += load_values[1][0];
		pLine_I[1] += load_values[1][1];
		pLine_I[2] += load_values[1][2];
		//Neutral assumed 0, since it was anyways

		//Admittance
		pShunt[0] += load_values[2][0];
		pShunt[1] += load_values[2][1];
		pShunt[2] += load_values[2][2];

		if (obj->parent != NULL)
			wunlock(obj->parent);
	}
	//Default else -- just proceed

	//Always ready to go back to event mode
	return SM_EVENT;
}

//Module-level post update call
STATUS house_e::post_deltaupdate(void)
{
	OBJECT *obj = OBJECTHDR(this);

	//Right now, basically undo what was done in interupdate.  When houses properly support
	//dynamics, we'll revisit how this interaction occurs (get node to zero accumulators or something)

	//Basically replicate the panel posting from sync, right now
	// compute line currents and post to meter
	if (obj->parent != NULL)
		wlock(obj->parent);

	//Post accumulations up to parent meter/node
	//Update power
	pPower[0] -= load_values[0][0];
	pPower[1] -= load_values[0][1];
	pPower[2] -= load_values[0][2];

	//Current
	pLine_I[0] -= load_values[1][0];
	pLine_I[1] -= load_values[1][1];
	pLine_I[2] -= load_values[1][2];
	//Neutral assumed 0, since it was anyways

	//Admittance
	pShunt[0] -= load_values[2][0];
	pShunt[1] -= load_values[2][1];
	pShunt[2] -= load_values[2][2];

	if (obj->parent != NULL)
		wunlock(obj->parent);

	return SUCCESS;	//Always succeeds right now
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT_CREATE_C(house,house_e);

EXPORT int init_house(OBJECT *obj)
{
	try {
		house_e *my = OBJECTDATA(obj,house_e);
		int rv = my->init_climate();
		if(rv == 0){// climate object has not initialized
			return 2;
		} else {
			return my->init(obj->parent);
		}
	}
	INIT_CATCHALL(house);
}

EXPORT int isa_house(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,house_e)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_house(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{

	try {
		house_e *my = OBJECTDATA(obj,house_e);
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
		case PC_POSTTOPDOWN:
			t1 = my->postsync(obj->clock, t0);
			obj->clock = t0;
			break;
		default:
			gl_error("house_e::sync- invalid pass configuration");
			t1 = TS_INVALID; // serious error in exec.c
		}
		return t1;
	} 
	SYNC_CATCHALL(house);
}

EXPORT TIMESTAMP plc_house(OBJECT *obj, TIMESTAMP t0)
{
	// this will be disabled if a PLC object is attached to the waterheater
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the clock if it has not been set yet

	house_e *my = OBJECTDATA(obj,house_e);
	return my->sync_thermostat(obj->clock, t0);
}

//Deltamode exposed functions
EXPORT SIMULATIONMODE interupdate_house_e(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	house_e *my = OBJECTDATA(obj,house_e);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time,dt,iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_house_e(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

EXPORT STATUS postupdate_house_e(OBJECT *obj)
{
	house_e *my = OBJECTDATA(obj,house_e);
	STATUS status = FAILED;
	try
	{
		status = my->post_deltaupdate();
		return status;
	}
	catch (char *msg)
	{
		gl_error("postupdate_house_e(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

/**@}**/

 	  	 
