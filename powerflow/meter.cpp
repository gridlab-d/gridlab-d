/** $Id: meter.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file meter.cpp
	@addtogroup powerflow_meter Meter
	@ingroup powerflow

	@note The meter object now only implements polyphase meters.  For a singlephase
	meter, see the triplex_meter object.

	Distribution meter can be either single phase or polyphase meters.
	For polyphase meters, the line voltages are nominally 277V line-to-line, and
	480V line-to-ground.  The ground is not presented explicitly (it is assumed).

	Total cumulative energy, instantantenous power and peak demand are metered.

	@{
 **/
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "meter.h"

// useful macros
#define TO_HOURS(t) (((double)t) / (3600 * TS_SECOND))

// meter reset function
EXPORT int64 meter_reset(OBJECT *obj)
{
	meter *pMeter = OBJECTDATA(obj,meter);
	pMeter->measured_demand = 0;
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// meter CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
// class management data
CLASS* meter::oclass = nullptr;
CLASS* meter::pclass = nullptr;

// the constructor registers the class and properties and sets the defaults
meter::meter(MODULE *mod) : node(mod)
{
	// first time init
	if (oclass==nullptr)
	{
		// link to parent class (used by isa)
		pclass = node::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"meter",sizeof(meter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class meter";
		else
			oclass->trl = TRL_PROVEN;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_double, "measured_real_energy[Wh]", PADDR(measured_real_energy),PT_DESCRIPTION,"metered real energy consumption, cummalitive",
            PT_double, "measured_real_energy_delta[Wh]", PADDR(measured_real_energy_delta),PT_DESCRIPTION,"delta in metered real energy consumption from last specified measured_energy_delta_timestep",
			PT_double, "measured_reactive_energy[VAh]",PADDR(measured_reactive_energy),PT_DESCRIPTION,"metered reactive energy consumption, cummalitive",
            PT_double, "measured_reactive_energy_delta[VAh]",PADDR(measured_reactive_energy_delta),PT_DESCRIPTION,"delta in metered reactive energy consumption from last specified measured_energy_delta_timestep",
            PT_double, "measured_energy_delta_timestep[s]",PADDR(measured_energy_delta_timestep),PT_DESCRIPTION,"Period of timestep for real and reactive delta energy calculation",
			PT_complex, "measured_power[VA]", PADDR(measured_power),PT_DESCRIPTION,"metered complex power",
			PT_complex, "measured_power_A[VA]", PADDR(indiv_measured_power[0]),PT_DESCRIPTION,"metered complex power on phase A",
			PT_complex, "measured_power_B[VA]", PADDR(indiv_measured_power[1]),PT_DESCRIPTION,"metered complex power on phase B",
			PT_complex, "measured_power_C[VA]", PADDR(indiv_measured_power[2]),PT_DESCRIPTION,"metered complex power on phase C",
			PT_double, "measured_demand[W]", PADDR(measured_demand),PT_DESCRIPTION,"greatest metered real power during simulation",
			PT_double, "measured_real_power[W]", PADDR(measured_real_power),PT_DESCRIPTION,"metered real power",
			PT_double, "measured_reactive_power[VAr]", PADDR(measured_reactive_power),PT_DESCRIPTION,"metered reactive power",
			PT_complex, "meter_power_consumption[VA]", PADDR(meter_power_consumption),PT_DESCRIPTION,"metered power used for operating the meter; standby and communication losses",
			
			// added to record last voltage/current
			PT_complex, "measured_voltage_A[V]", PADDR(measured_voltage[0]),PT_DESCRIPTION,"measured line-to-neutral voltage on phase A",
			PT_complex, "measured_voltage_B[V]", PADDR(measured_voltage[1]),PT_DESCRIPTION,"measured line-to-neutral voltage on phase B",
			PT_complex, "measured_voltage_C[V]", PADDR(measured_voltage[2]),PT_DESCRIPTION,"measured line-to-neutral voltage on phase C",
			PT_complex, "measured_voltage_AB[V]", PADDR(measured_voltageD[0]),PT_DESCRIPTION,"measured line-to-line voltage on phase AB",
			PT_complex, "measured_voltage_BC[V]", PADDR(measured_voltageD[1]),PT_DESCRIPTION,"measured line-to-line voltage on phase BC",
			PT_complex, "measured_voltage_CA[V]", PADDR(measured_voltageD[2]),PT_DESCRIPTION,"measured line-to-line voltage on phase CA",

			//Voltage items
			PT_double, "measured_real_max_voltage_A_in_interval[V]", PADDR(measured_real_max_voltage_in_interval[0]),PT_DESCRIPTION,"measured real max line-to-neutral voltage on phase A over a specified interval",
			PT_double, "measured_real_max_voltage_B_in_interval[V]", PADDR(measured_real_max_voltage_in_interval[1]),PT_DESCRIPTION,"measured real max line-to-neutral voltage on phase B over a specified interval",
			PT_double, "measured_real_max_voltage_C_in_interval[V]", PADDR(measured_real_max_voltage_in_interval[2]),PT_DESCRIPTION,"measured real max line-to-neutral voltage on phase C over a specified interval",
			PT_double, "measured_reactive_max_voltage_A_in_interval[V]", PADDR(measured_reactive_max_voltage_in_interval[0]),PT_DESCRIPTION,"measured reactive max line-to-neutral voltage on phase A over a specified interval",
			PT_double, "measured_reactive_max_voltage_B_in_interval[V]", PADDR(measured_reactive_max_voltage_in_interval[1]),PT_DESCRIPTION,"measured reactive max line-to-neutral voltage on phase B over a specified interval",
			PT_double, "measured_reactive_max_voltage_C_in_interval[V]", PADDR(measured_reactive_max_voltage_in_interval[2]),PT_DESCRIPTION,"measured reactive max line-to-neutral voltage on phase C over a specified interval",
			PT_double, "measured_real_max_voltage_AB_in_interval[V]", PADDR(measured_real_max_voltageD_in_interval[0]),PT_DESCRIPTION,"measured real max line-to-line voltage on phase A over a specified interval",
			PT_double, "measured_real_max_voltage_BC_in_interval[V]", PADDR(measured_real_max_voltageD_in_interval[1]),PT_DESCRIPTION,"measured real max line-to-line voltage on phase B over a specified interval",
			PT_double, "measured_real_max_voltage_CA_in_interval[V]", PADDR(measured_real_max_voltageD_in_interval[2]),PT_DESCRIPTION,"measured real max line-to-line voltage on phase C over a specified interval",
			PT_double, "measured_reactive_max_voltage_AB_in_interval[V]", PADDR(measured_reactive_max_voltageD_in_interval[0]),PT_DESCRIPTION,"measured reactive max line-to-line voltage on phase A over a specified interval",
			PT_double, "measured_reactive_max_voltage_BC_in_interval[V]", PADDR(measured_reactive_max_voltageD_in_interval[1]),PT_DESCRIPTION,"measured reactive max line-to-line voltage on phase B over a specified interval",
			PT_double, "measured_reactive_max_voltage_CA_in_interval[V]", PADDR(measured_reactive_max_voltageD_in_interval[2]),PT_DESCRIPTION,"measured reactive max line-to-line voltage on phase C over a specified interval",
			PT_double, "measured_real_min_voltage_A_in_interval[V]", PADDR(measured_real_min_voltage_in_interval[0]),PT_DESCRIPTION,"measured real min line-to-neutral voltage on phase A over a specified interval",
			PT_double, "measured_real_min_voltage_B_in_interval[V]", PADDR(measured_real_min_voltage_in_interval[1]),PT_DESCRIPTION,"measured real min line-to-neutral voltage on phase B over a specified interval",
			PT_double, "measured_real_min_voltage_C_in_interval[V]", PADDR(measured_real_min_voltage_in_interval[2]),PT_DESCRIPTION,"measured real min line-to-neutral voltage on phase C over a specified interval",
			PT_double, "measured_reactive_min_voltage_A_in_interval[V]", PADDR(measured_reactive_min_voltage_in_interval[0]),PT_DESCRIPTION,"measured reactive min line-to-neutral voltage on phase A over a specified interval",
			PT_double, "measured_reactive_min_voltage_B_in_interval[V]", PADDR(measured_reactive_min_voltage_in_interval[1]),PT_DESCRIPTION,"measured reactive min line-to-neutral voltage on phase B over a specified interval",
			PT_double, "measured_reactive_min_voltage_C_in_interval[V]", PADDR(measured_reactive_min_voltage_in_interval[2]),PT_DESCRIPTION,"measured reactive min line-to-neutral voltage on phase C over a specified interval",
			PT_double, "measured_real_min_voltage_AB_in_interval[V]", PADDR(measured_real_min_voltageD_in_interval[0]),PT_DESCRIPTION,"measured real min line-to-line voltage on phase A over a specified interval",
			PT_double, "measured_real_min_voltage_BC_in_interval[V]", PADDR(measured_real_min_voltageD_in_interval[1]),PT_DESCRIPTION,"measured real min line-to-line voltage on phase B over a specified interval",
			PT_double, "measured_real_min_voltage_CA_in_interval[V]", PADDR(measured_real_min_voltageD_in_interval[2]),PT_DESCRIPTION,"measured real min line-to-line voltage on phase C over a specified interval",
			PT_double, "measured_reactive_min_voltage_AB_in_interval[V]", PADDR(measured_reactive_min_voltageD_in_interval[0]),PT_DESCRIPTION,"measured reactive min line-to-line voltage on phase A over a specified interval",
			PT_double, "measured_reactive_min_voltage_BC_in_interval[V]", PADDR(measured_reactive_min_voltageD_in_interval[1]),PT_DESCRIPTION,"measured reactive min line-to-line voltage on phase B over a specified interval",
			PT_double, "measured_reactive_min_voltage_CA_in_interval[V]", PADDR(measured_reactive_min_voltageD_in_interval[2]),PT_DESCRIPTION,"measured reactive min line-to-line voltage on phase C over a specified interval",
			PT_double, "measured_avg_voltage_A_mag_in_interval[V]", PADDR(measured_avg_voltage_mag_in_interval[0]),PT_DESCRIPTION,"measured avg line-to-neutral voltage magnitude on phase A over a specified interval",
			PT_double, "measured_avg_voltage_B_mag_in_interval[V]", PADDR(measured_avg_voltage_mag_in_interval[1]),PT_DESCRIPTION,"measured avg line-to-neutral voltage magnitude on phase B over a specified interval",
			PT_double, "measured_avg_voltage_C_mag_in_interval[V]", PADDR(measured_avg_voltage_mag_in_interval[2]),PT_DESCRIPTION,"measured avg line-to-neutral voltage magnitude on phase C over a specified interval",
			PT_double, "measured_avg_voltage_AB_mag_in_interval[V]", PADDR(measured_avg_voltageD_mag_in_interval[0]),PT_DESCRIPTION,"measured avg line-to-line voltage magnitude on phase A over a specified interval",
			PT_double, "measured_avg_voltage_BC_mag_in_interval[V]", PADDR(measured_avg_voltageD_mag_in_interval[1]),PT_DESCRIPTION,"measured avg line-to-line voltage magnitude on phase B over a specified interval",
			PT_double, "measured_avg_voltage_CA_mag_in_interval[V]", PADDR(measured_avg_voltageD_mag_in_interval[2]),PT_DESCRIPTION,"measured avg line-to-line voltage magnitude on phase C over a specified interval",
			
			//power average items
			PT_double, "measured_real_max_power_in_interval[W]", PADDR(measured_real_max_power_in_interval),PT_DESCRIPTION,"measured maximum real power over a specified interval",
			PT_double, "measured_reactive_max_power_in_interval[VAr]", PADDR(measured_reactive_max_power_in_interval),PT_DESCRIPTION,"measured maximum reactive power over a specified interval",
			PT_double, "measured_real_min_power_in_interval[W]", PADDR(measured_real_min_power_in_interval),PT_DESCRIPTION,"measured minimum real power over a specified interval",
			PT_double, "measured_reactive_min_power_in_interval[VAr]", PADDR(measured_reactive_min_power_in_interval),PT_DESCRIPTION,"measured minimum reactive power over a specified interval",
			PT_double, "measured_avg_real_power_in_interval[W]", PADDR(measured_real_avg_power_in_interval),PT_DESCRIPTION,"measured average real power over a specified interval",
			PT_double, "measured_avg_reactive_power_in_interval[VAr]", PADDR(measured_reactive_avg_power_in_interval),PT_DESCRIPTION,"measured average reactive power over a specified interval",

            // per-phase power min/max/average
            // A phase:
			PT_double, "measured_real_max_power_A_in_interval[W]", PADDR(measured_real_max_power_in_interval_3ph[0]),PT_DESCRIPTION,"measured A phase maximum real power over a specified interval",
			PT_double, "measured_reactive_max_power_A_in_interval[VAr]", PADDR(measured_reactive_max_power_in_interval_3ph[0]),PT_DESCRIPTION,"measured A phase maximum reactive power over a specified interval",
			PT_double, "measured_real_min_power_A_in_interval[W]", PADDR(measured_real_min_power_in_interval_3ph[0]),PT_DESCRIPTION,"measured A phase minimum real power over a specified interval",
			PT_double, "measured_reactive_min_power_A_in_interval[VAr]", PADDR(measured_reactive_min_power_in_interval_3ph[0]),PT_DESCRIPTION,"measured A phase minimum reactive power over a specified interval",
			PT_double, "measured_avg_real_power_A_in_interval[W]", PADDR(measured_real_avg_power_in_interval_3ph[0]),PT_DESCRIPTION,"measured A phase average real power over a specified interval",
			PT_double, "measured_avg_reactive_power_A_in_interval[VAr]", PADDR(measured_reactive_avg_power_in_interval_3ph[0]),PT_DESCRIPTION,"measured A phase average reactive power over a specified interval",
            // B phase
			PT_double, "measured_real_max_power_B_in_interval[W]", PADDR(measured_real_max_power_in_interval_3ph[1]),PT_DESCRIPTION,"measured B phase maximum real power over a specified interval",
			PT_double, "measured_reactive_max_power_B_in_interval[VAr]", PADDR(measured_reactive_max_power_in_interval_3ph[1]),PT_DESCRIPTION,"measured B phase maximum reactive power over a specified interval",
			PT_double, "measured_real_min_power_B_in_interval[W]", PADDR(measured_real_min_power_in_interval_3ph[1]),PT_DESCRIPTION,"measured B phase minimum real power over a specified interval",
			PT_double, "measured_reactive_min_power_B_in_interval[VAr]", PADDR(measured_reactive_min_power_in_interval_3ph[1]),PT_DESCRIPTION,"measured B phase minimum reactive power over a specified interval",
			PT_double, "measured_avg_real_power_B_in_interval[W]", PADDR(measured_real_avg_power_in_interval_3ph[1]),PT_DESCRIPTION,"measured B phase average real power over a specified interval",
			PT_double, "measured_avg_reactive_power_B_in_interval[VAr]", PADDR(measured_reactive_avg_power_in_interval_3ph[1]),PT_DESCRIPTION,"measured B phase average reactive power over a specified interval",
            // C phase
			PT_double, "measured_real_max_power_C_in_interval[W]", PADDR(measured_real_max_power_in_interval_3ph[2]),PT_DESCRIPTION,"measured C phase maximum real power over a specified interval",
			PT_double, "measured_reactive_max_power_C_in_interval[VAr]", PADDR(measured_reactive_max_power_in_interval_3ph[2]),PT_DESCRIPTION,"measured C phase maximum reactive power over a specified interval",
			PT_double, "measured_real_min_power_C_in_interval[W]", PADDR(measured_real_min_power_in_interval_3ph[2]),PT_DESCRIPTION,"measured C phase minimum real power over a specified interval",
			PT_double, "measured_reactive_min_power_C_in_interval[VAr]", PADDR(measured_reactive_min_power_in_interval_3ph[2]),PT_DESCRIPTION,"measured C phase minimum reactive power over a specified interval",
			PT_double, "measured_avg_real_power_C_in_interval[W]", PADDR(measured_real_avg_power_in_interval_3ph[2]),PT_DESCRIPTION,"measured C phase average real power over a specified interval",
			PT_double, "measured_avg_reactive_power_C_in_interval[VAr]", PADDR(measured_reactive_avg_power_in_interval_3ph[2]),PT_DESCRIPTION,"measured C phase average reactive power over a specified interval",

			//Interval for the min/max/averages
            PT_double, "measured_stats_interval[s]",PADDR(measured_min_max_avg_timestep),PT_DESCRIPTION,"Period of timestep for min/max/average calculations",

			PT_complex, "measured_current_A[A]", PADDR(measured_current[0]),PT_DESCRIPTION,"measured current on phase A",
			PT_complex, "measured_current_B[A]", PADDR(measured_current[1]),PT_DESCRIPTION,"measured current on phase B",
			PT_complex, "measured_current_C[A]", PADDR(measured_current[2]),PT_DESCRIPTION,"measured current on phase C",
			PT_bool, "customer_interrupted", PADDR(meter_interrupted),PT_DESCRIPTION,"Reliability flag - goes active if the customer is in an 'interrupted' state",
			PT_bool, "customer_interrupted_secondary", PADDR(meter_interrupted_secondary),PT_DESCRIPTION,"Reliability flag - goes active if the customer is in an 'secondary interrupted' state - i.e., momentary",
#ifdef SUPPORT_OUTAGES
			PT_int16, "sustained_count", PADDR(sustained_count),	//reliability sustained event counter
			PT_int16, "momentary_count", PADDR(momentary_count),	//reliability momentary event counter
			PT_int16, "total_count", PADDR(total_count),		//reliability total event counter
			PT_int16, "s_flag", PADDR(s_flag),
			PT_int16, "t_flag", PADDR(t_flag),
			PT_complex, "pre_load", PADDR(pre_load),
#endif
			PT_double, "monthly_bill[$]", PADDR(monthly_bill),PT_DESCRIPTION,"Accumulator for the current month's bill",
			PT_double, "previous_monthly_bill[$]", PADDR(previous_monthly_bill),PT_DESCRIPTION,"Total monthly bill for the previous month",
			PT_double, "previous_monthly_energy[kWh]", PADDR(previous_monthly_energy),PT_DESCRIPTION,"Total monthly energy for the previous month",
			PT_double, "monthly_fee[$]", PADDR(monthly_fee),PT_DESCRIPTION,"Once a month flat fee for customer hook-up",
			PT_double, "monthly_energy[kWh]", PADDR(monthly_energy),PT_DESCRIPTION,"Accumulator for the current month's energy consumption",
			PT_enumeration, "bill_mode", PADDR(bill_mode),PT_DESCRIPTION,"Billing structure desired",
				PT_KEYWORD,"NONE",(enumeration)BM_NONE,
				PT_KEYWORD,"UNIFORM",(enumeration)BM_UNIFORM,
				PT_KEYWORD,"TIERED",(enumeration)BM_TIERED,
				PT_KEYWORD,"HOURLY",(enumeration)BM_HOURLY,
				PT_KEYWORD,"TIERED_RTP",(enumeration)BM_TIERED_RTP,
				PT_KEYWORD,"TIERED_TOU",(enumeration)BM_TIERED_TOU,
			PT_object, "power_market", PADDR(power_market),PT_DESCRIPTION,"Market (auction object) where the price is being received from",
			PT_int32, "bill_day", PADDR(bill_day),PT_DESCRIPTION,"day of month bill is to be processed (currently limited to days 1-28)",
			PT_double, "price[$/kWh]", PADDR(price),PT_DESCRIPTION,"current price of electricity",
			PT_double, "price_base[$/kWh]", PADDR(price_base), PT_DESCRIPTION, "Used only in TIERED_RTP mode to describe the price before the first tier",
			PT_double, "first_tier_price[$/kWh]", PADDR(tier_price[0]),PT_DESCRIPTION,"price of electricity between first tier and second tier energy usage",
			PT_double, "first_tier_energy[kWh]", PADDR(tier_energy[0]),PT_DESCRIPTION,"switching point between base price and first tier price",
			PT_double, "second_tier_price[$/kWh]", PADDR(tier_price[1]),PT_DESCRIPTION,"price of electricity between second tier and third tier energy usage",
			PT_double, "second_tier_energy[kWh]", PADDR(tier_energy[1]),PT_DESCRIPTION,"switching point between first tier price and second tier price",
			PT_double, "third_tier_price[$/kWh]", PADDR(tier_price[2]),PT_DESCRIPTION,"price of electricity when energy usage exceeds third tier energy usage",
			PT_double, "third_tier_energy[kWh]", PADDR(tier_energy[2]),PT_DESCRIPTION,"switching point between second tier price and third tier price",

			//PT_double, "measured_reactive[kVar]", PADDR(measured_reactive), has not implemented yet
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// publish meter reset function
		if (gl_publish_function(oclass,"reset",(FUNCTIONADDR)meter_reset)==nullptr)
			GL_THROW("unable to publish meter_reset function in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_meter)==nullptr)
			GL_THROW("Unable to publish meter deltamode function");
		if (gl_publish_function(oclass,	"pwr_object_swing_swapper", (FUNCTIONADDR)swap_node_swing_status)==nullptr)
			GL_THROW("Unable to publish meter swing-swapping function");
		if (gl_publish_function(oclass,	"pwr_current_injection_update_map", (FUNCTIONADDR)node_map_current_update_function)==nullptr)
			GL_THROW("Unable to publish meter current injection update mapping function");
		if (gl_publish_function(oclass,	"attach_vfd_to_pwr_object", (FUNCTIONADDR)attach_vfd_to_node)==nullptr)
			GL_THROW("Unable to publish meter VFD attachment function");
		if (gl_publish_function(oclass, "pwr_object_reset_disabled_status", (FUNCTIONADDR)node_reset_disabled_status) == nullptr)
			GL_THROW("Unable to publish meter island-status-reset function");
		if (gl_publish_function(oclass, "pwr_object_swing_status_check", (FUNCTIONADDR)node_swing_status) == nullptr)
			GL_THROW("Unable to publish meter swing-status check function");
		if (gl_publish_function(oclass, "pwr_object_shunt_update", (FUNCTIONADDR)node_update_shunt_values) == nullptr)
			GL_THROW("Unable to publish meter shunt update function");
		if (gl_publish_function(oclass, "pwr_object_kmldata", (FUNCTIONADDR)meter_kmldata) == nullptr)
			GL_THROW("Unable to publish meter kmldata function");
		}
}

int meter::isa(char *classname)
{
	return strcmp(classname,"meter")==0 || node::isa(classname);
}

// Create a distribution meter from the defaults template, return 1 on success
int meter::create()
{
	int result = node::create();
	
#ifdef SUPPORT_OUTAGES
	sustained_count=0;	//reliability sustained event counter
	momentary_count=0;	//reliability momentary event counter
	total_count=0;		//reliability total event counter
	s_flag=0;
	t_flag=0;
	pre_load=0;
#endif

	measured_voltage[0] = measured_voltage[1] = measured_voltage[2] = gld::complex(0,0,A);
	measured_voltageD[0] = measured_voltageD[1] = measured_voltageD[2] = gld::complex(0,0,A);
	measured_real_max_voltage_in_interval[0] = measured_real_max_voltage_in_interval[1] = measured_real_max_voltage_in_interval[2] = 0.0;
	measured_reactive_max_voltage_in_interval[0] = measured_reactive_max_voltage_in_interval[1] = measured_reactive_max_voltage_in_interval[2] = 0.0;
	measured_real_max_voltageD_in_interval[0] = measured_real_max_voltageD_in_interval[1] = measured_real_max_voltageD_in_interval[2] = 0.0;
	measured_reactive_max_voltageD_in_interval[0] = measured_reactive_max_voltageD_in_interval[1] = measured_reactive_max_voltageD_in_interval[2] = 0.0;
	measured_real_min_voltage_in_interval[0] = measured_real_min_voltage_in_interval[1] = measured_real_min_voltage_in_interval[2] = 0.0;
	measured_reactive_min_voltage_in_interval[0] = measured_reactive_min_voltage_in_interval[1] = measured_reactive_min_voltage_in_interval[2] = 0.0;
	measured_real_min_voltageD_in_interval[0] = measured_real_min_voltageD_in_interval[1] = measured_real_min_voltageD_in_interval[2] = 0.0;
	measured_reactive_min_voltageD_in_interval[0] = measured_reactive_min_voltageD_in_interval[1] = measured_reactive_min_voltageD_in_interval[2] = 0.0;
	measured_avg_voltage_mag_in_interval[0] = measured_avg_voltage_mag_in_interval[1] = measured_avg_voltage_mag_in_interval[2] = 0.0;
	measured_current[0] = measured_current[1] = measured_current[2] = gld::complex(0,0,J);
	measured_real_energy = measured_reactive_energy = 0.0;
    measured_real_energy_delta = measured_reactive_energy_delta = 0;
    last_measured_voltage[0] = last_measured_voltage[1] = last_measured_voltage[2] = gld::complex(0,0,A);
    last_measured_voltageD[0] = last_measured_voltageD[1] = last_measured_voltageD[2] = gld::complex(0,0,A);
    last_measured_real_energy = last_measured_reactive_energy = 0;
    last_measured_real_power = last_measured_reactive_power = 0.0;
    last_measured_real_power_3ph[0] = last_measured_real_power_3ph[1] = last_measured_real_power_3ph[2] = 0.0;
    last_measured_reactive_power_3ph[0] = last_measured_reactive_power_3ph[1] = last_measured_reactive_power_3ph[2] = 0.0;
	measured_energy_delta_timestep = -1;
	measured_min_max_avg_timestep = -1;
	measured_power = gld::complex(0,0,J);
	measured_demand = 0.0;
	measured_real_power = 0.0;
	measured_reactive_power = 0.0;

	indiv_measured_power[0] = indiv_measured_power[1] = indiv_measured_power[2] = gld::complex(0.0,0.0);

	meter_interrupted = false;	//We default to being in service
	meter_interrupted_secondary = false;	//Default to no momentary interruptions

	hourly_acc = 0.0;
	monthly_bill = 0.0;
	monthly_energy = 0.0;
	previous_monthly_energy = 0.0;
	bill_mode = BM_NONE;
	power_market = 0;
	price_prop = 0;
	bill_day = 15;
	last_bill_month = -1;
	price = 0.0;
	tier_price[0] = tier_price[1] = tier_price[2] = 0;
	tier_energy[0] = tier_energy[1] = tier_energy[2] = 0;
	last_price = 0;
	last_tier_price[0] = 0;
	last_tier_price[1] = 0;
	last_tier_price[2] = 0;
	last_price_base = 0;
	meter_power_consumption = gld::complex(0,0);

	//Flag us as a meter
	node_type = METER_NODE;

	meter_NR_servered = false;	//Assume we are just a normal meter at first

	//power min/max/average items, 3 phase
	measured_real_max_power_in_interval = 0.0;
	measured_reactive_max_power_in_interval = 0.0;
	measured_real_min_power_in_interval = 0.0;
	measured_reactive_min_power_in_interval = 0.0;
	measured_real_avg_power_in_interval = 0.0;
	measured_reactive_avg_power_in_interval = 0.0;
	//power min/max average items, per phase.
	// A
	measured_real_max_power_in_interval_3ph[0] = 0.0;
	measured_reactive_max_power_in_interval_3ph[0] = 0.0;
	measured_real_min_power_in_interval_3ph[0] = 0.0;
	measured_reactive_min_power_in_interval_3ph[0] = 0.0;
	measured_real_avg_power_in_interval_3ph[0] = 0.0;
	measured_reactive_avg_power_in_interval_3ph[0] = 0.0;
	// B
	measured_real_max_power_in_interval_3ph[1] = 0.0;
    measured_reactive_max_power_in_interval_3ph[1] = 0.0;
    measured_real_min_power_in_interval_3ph[1] = 0.0;
    measured_reactive_min_power_in_interval_3ph[1] = 0.0;
    measured_real_avg_power_in_interval_3ph[1] = 0.0;
    measured_reactive_avg_power_in_interval_3ph[1] = 0.0;
    // C
    measured_real_max_power_in_interval_3ph[2] = 0.0;
    measured_reactive_max_power_in_interval_3ph[2] = 0.0;
    measured_real_min_power_in_interval_3ph[2] = 0.0;
    measured_reactive_min_power_in_interval_3ph[2] = 0.0;
    measured_real_avg_power_in_interval_3ph[2] = 0.0;
    measured_reactive_avg_power_in_interval_3ph[2] = 0.0;

	last_measured_max_real_power = 0.0;
	last_measured_min_real_power = 0.0;
	last_measured_max_reactive_power = 0.0;
	last_measured_min_reactive_power = 0.0;
	last_measured_avg_real_power = 0.0;
	last_measured_avg_reactive_power = 0.0;

    // A
	last_measured_max_real_power_3ph[0] = 0.0;
	last_measured_min_real_power_3ph[0] = 0.0;
	last_measured_max_reactive_power_3ph[0] = 0.0;
	last_measured_min_reactive_power_3ph[0] = 0.0;
	last_measured_avg_real_power_3ph[0] = 0.0;
	last_measured_avg_reactive_power_3ph[0] = 0.0;
    // B
	last_measured_max_real_power_3ph[1] = 0.0;
	last_measured_min_real_power_3ph[1] = 0.0;
	last_measured_max_reactive_power_3ph[1] = 0.0;
	last_measured_min_reactive_power_3ph[1] = 0.0;
	last_measured_avg_real_power_3ph[1] = 0.0;
	last_measured_avg_reactive_power_3ph[1] = 0.0;
    // C
	last_measured_max_real_power_3ph[2] = 0.0;
	last_measured_min_real_power_3ph[2] = 0.0;
	last_measured_max_reactive_power_3ph[2] = 0.0;
	last_measured_min_reactive_power_3ph[2] = 0.0;
	last_measured_avg_real_power_3ph[2] = 0.0;
	last_measured_avg_reactive_power_3ph[2] = 0.0;

	return result;
}

// Initialize a distribution meter, return 1 on success
int meter::init(OBJECT *parent)
{
	char temp_buff[128];
	OBJECT *obj = OBJECTHDR(this);

	if(power_market != 0){
		price_prop = gl_get_property(power_market, "current_market.clearing_price");
		if(price_prop == 0){
			GL_THROW(const_cast<char*>(R"(meter::power_market object '%s' does not publish 'current_market.clearing_price')"), (power_market->name ? power_market->name : "(anon)"));
		}
	}

	// Count the number of phases...for use with meter_power_consumption
	if (meter_power_consumption != gld::complex(0,0))
	{
		no_phases = 0;
		if (has_phase(PHASE_A))
			no_phases += 1;
		if (has_phase(PHASE_B))
			no_phases += 1;
		if (has_phase(PHASE_C))
			no_phases += 1;
	}

	check_prices();
	last_t = dt = 0;

	//Update tracking flag
	//Get server mode variable
	gl_global_getvar("multirun_mode",temp_buff,sizeof(temp_buff));

	//See if we're not in standalone
	if (strcmp(temp_buff,"STANDALONE"))	//strcmp returns a 0 if they are the same
	{
		if ((solver_method == SM_NR) && (bustype == SWING))
		{
			meter_NR_servered = true;	//Set this flag for later use

			//Allocate the storage vector
			prev_voltage_value = (gld::complex *)gl_malloc(3*sizeof(gld::complex));

			//Check it
			if (prev_voltage_value==nullptr)
			{
				GL_THROW("Failure to allocate memory for voltage tracking array");
				/*  TROUBLESHOOT
				While attempting to allocate memory for the voltage tracking array used
				by the master/slave functionality, an error occurred.  Please try again.
				If the error persists, please submit your code and a bug report via the trac
				website.
				*/
			}

			//Populate it with zeros for now, just cause - init sets voltages in node
			prev_voltage_value[0] = gld::complex(0.0,0.0);
			prev_voltage_value[1] = gld::complex(0.0,0.0);
			prev_voltage_value[2] = gld::complex(0.0,0.0);
		}
	}

	//Check power and energy properties - if they are initialized, send a warning
	if ((measured_real_power != 0.0) || (measured_reactive_power != 0.0) || (measured_real_energy != 0.0) || (measured_reactive_energy != 0.0))
	{
		gl_warning("meter:%d - %s - measured power or energy is not initialized to zero - unexpected energy values may result",obj->id,(obj->name?obj->name:"Unnamed"));
		/*  TROUBLESHOOT
		An initial value for measured_real_power, measured_reactive_power, measured_real_energy, or measured_reactive_energy was set in the GLM.  This may cause some unexpected
		energy values to be computed for the system on this meter.  If this was not deliberate, be sure to remove those entries from the GLM file.
		*/
	}

	return node::init(parent);
}

int meter::check_prices(){
	if(bill_mode == BM_UNIFORM){
		if(price < 0.0){
			//GL_THROW("triplex_meter price is negative!"); // This shouldn't throw an error - negative prices are okay JCF
		}
	} else if(bill_mode == BM_TIERED || bill_mode == BM_TIERED_RTP){
		if(tier_price[1] == 0){
			tier_price[1] = tier_price[0];
			tier_energy[1] = tier_energy[0];
		}
		if(tier_price[2] == 0){
			tier_price[2] = tier_price[1];
			tier_energy[2] = tier_energy[1];
		}
		if(tier_energy[2] < tier_energy[1] || tier_energy[1] < tier_energy[0]){
			GL_THROW("meter energy tiers quantity trend improperly");
		}
		for(int i = 0; i < 3; ++i){
			if(tier_price[i] < 0.0 || tier_energy[i] < 0.0)
				GL_THROW("meter tiers cannot have negative values");
		}
	} else if (bill_mode == BM_TIERED_TOU) { // beware: TOU pricing schedules haven't pushed values yet
		if(tier_energy[1] == 0){ 
			tier_price[1] = tier_price[0];
			tier_energy[1] = DBL_MAX;
		}
		if(tier_energy[2] == 0){
			tier_price[2] = tier_price[1];
			tier_energy[2] = DBL_MAX;
		}
		if(tier_energy[2] < tier_energy[1] || tier_energy[1] < tier_energy[0]){
			GL_THROW("meter energy tiers quantity trend improperly");
		}
		for(int i = 0; i < 3; ++i){
			if(tier_price[i] < 0.0 || tier_energy[i] < 0.0)
				GL_THROW("meter tiers cannot have negative values");
		}
	}

	if(bill_mode == BM_HOURLY || bill_mode == BM_TIERED_RTP){
		if(power_market == 0 || price_prop == 0){
			GL_THROW("meter cannot use real time energy prices without a power market that publishes the next price");
		}
		//price = *gl_get_double(power_market,price_prop);
	}

	// initialize these to the same value
	last_price = price;
	last_price_base = price_base;
	last_tier_price[0] = tier_price[0];
	last_tier_price[1] = tier_price[1];
	last_tier_price[2] = tier_price[2];

	return 0;
}
TIMESTAMP meter::presync(TIMESTAMP t0)
{
	if (meter_power_consumption != gld::complex(0,0))
		power[0] = power[1] = power[2] = 0.0;

	//Reliability addition - if momentary flag set - clear it
	if (meter_interrupted_secondary)
		meter_interrupted_secondary = false;
    
    // Capturing first timestamp of simulation for use in delta energy measurements.
    if (t0 != 0 && start_timestamp == 0)
        start_timestamp = t0;

	return node::presync(t0);
}

//Functionalized portion for deltamode compatibility
void meter::BOTH_meter_sync_fxn()
{
	int TempNodeRef;
	OBJECT *obj = OBJECTHDR(this);

	//Reliability check
	if ((fault_check_object != nullptr) && (solver_method == SM_NR))	//proper solver and fault_object isn't null - may need to set a flag
	{
		if (NR_node_reference==-99)	//Childed
		{
			TempNodeRef=*NR_subnode_reference;

			//See if our parent was initialized
			if (TempNodeRef == -1)
			{
				//Try to initialize it, for giggles
				node *Temp_Node = OBJECTDATA(SubNodeParent,node);

				//Call the initialization
				Temp_Node->NR_populate();

				//Pull our reference
				TempNodeRef=*NR_subnode_reference;

				//Check it again
				if (TempNodeRef == -1)
				{
					GL_THROW("meter:%d - %s - Uninitialized parent is causing odd issues - fix your model!",obj->id,(obj->name?obj->name:"Unnamed"));
					/*  TROUBLESHOOT
					A childed meter object is having issues mapping to its parent node - this typically happens in very odd cases of islanded/orphaned
					topologies that only contain nodes (no links).  Adjust your GLM to work around this issue.
					*/
				}
			}
		}
		else	//Normal
		{
			//Just assign it to our normal index
			TempNodeRef=NR_node_reference;
		}

		if ((NR_busdata[TempNodeRef].origphases & NR_busdata[TempNodeRef].phases) != NR_busdata[TempNodeRef].origphases)	//We have a phase mismatch - something has been lost
		{
			meter_interrupted = true;	//Someone is out of service, they just may not know it

			//See if we're flagged for a momentary as well - if we are, clear it
			if (meter_interrupted_secondary)
				meter_interrupted_secondary = false;
		}
		else
		{
			meter_interrupted = false;	//All is well
		}
	}

	if (meter_power_consumption != gld::complex(0,0))
	{
		if (has_phase(PHASE_A))
			power[0] += meter_power_consumption / no_phases;
		if (has_phase(PHASE_B))
			power[1] += meter_power_consumption / no_phases;
		if (has_phase(PHASE_C))
			power[2] += meter_power_consumption / no_phases;
	}
}

TIMESTAMP meter::sync(TIMESTAMP t0)
{
	//Call functionalized meter sync
	BOTH_meter_sync_fxn();

	return node::sync(t0);
}

TIMESTAMP meter::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	gld::complex temp_current;
	TIMESTAMP tretval;

	//Perform node update - do it now, otherwise current_inj isn't populated
	tretval = node::postsync(t1);

	measured_voltage[0] = voltageA;
	measured_voltage[1] = voltageB;
	measured_voltage[2] = voltageC;

	measured_voltageD[0] = voltageA - voltageB;
	measured_voltageD[1] = voltageB - voltageC;
	measured_voltageD[2] = voltageC - voltageA;

	if ((solver_method == SM_NR)||solver_method  == SM_FBS)
	{
		if (t1 > last_t)
		{
			dt = t1 - last_t;
			last_t = t1;
		}
		else
			dt = 0;
		
		measured_current[0] = current_inj[0];
		measured_current[1] = current_inj[1];
		measured_current[2] = current_inj[2];

		// compute energy use from previous cycle
		// - everything below this can moved to commit function once tape:recorder is collecting from commit function7
		if (dt > 0 && last_t != dt)
		{	
			measured_real_energy += measured_real_power * TO_HOURS(dt);
			measured_reactive_energy += measured_reactive_power * TO_HOURS(dt);
		}

		// compute demand power
		indiv_measured_power[0] = measured_voltage[0]*(~measured_current[0]);
		indiv_measured_power[1] = measured_voltage[1]*(~measured_current[1]);
		indiv_measured_power[2] = measured_voltage[2]*(~measured_current[2]);

		measured_power = indiv_measured_power[0] + indiv_measured_power[1] + indiv_measured_power[2];

		measured_real_power = (indiv_measured_power[0]).Re()
							+ (indiv_measured_power[1]).Re()
							+ (indiv_measured_power[2]).Re();

		measured_reactive_power = (indiv_measured_power[0]).Im()
								+ (indiv_measured_power[1]).Im()
								+ (indiv_measured_power[2]).Im();

		if (measured_real_power > measured_demand) {
			measured_demand = measured_real_power;
		}
        
        // Delta energy calculation
		if (measured_energy_delta_timestep > 0) {
			if (t0 == start_timestamp) {
				last_delta_timestamp = start_timestamp;

				if (tretval > last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep)) {
					tretval = last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep);
				}
			}

			if ((t1 > last_delta_timestamp) && (t1 < last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep)) && (t1 != t0)) {
				if (tretval > last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep)) {
					tretval = last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep);
				}
			}

			if ((t1 == last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep)) && (t1 != t0) && measured_energy_delta_timestep > 0) {
				measured_real_energy_delta = measured_real_energy - last_measured_real_energy;
				measured_reactive_energy_delta = measured_reactive_energy - last_measured_reactive_energy;
				last_measured_real_energy = measured_real_energy;
				last_measured_reactive_energy = measured_reactive_energy;
				last_delta_timestamp = t1;

				if (tretval > last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep)) {
					tretval = last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep);
				}
			}
		}//End perform delta-energy updates

        // Min/Max/Stat calculation
		if (measured_min_max_avg_timestep > 0) {
			if (t0 == start_timestamp) {
				last_stat_timestamp = start_timestamp;
				voltage_avg_count = 0;
				interval_dt = 0;

				//Voltage values
				measured_real_max_voltage_in_interval[0] = voltageA.Re();
				measured_real_max_voltage_in_interval[1] = voltageB.Re();
				measured_real_max_voltage_in_interval[2] = voltageC.Re();
				measured_real_max_voltageD_in_interval[0] = measured_voltageD[0].Re();
				measured_real_max_voltageD_in_interval[1] = measured_voltageD[1].Re();
				measured_real_max_voltageD_in_interval[2] = measured_voltageD[2].Re();
				measured_real_min_voltage_in_interval[0] = voltageA.Re();
				measured_real_min_voltage_in_interval[1] = voltageB.Re();
				measured_real_min_voltage_in_interval[2] = voltageC.Re();
				measured_real_min_voltageD_in_interval[0] = measured_voltageD[0].Re();
				measured_real_min_voltageD_in_interval[1] = measured_voltageD[1].Re();
				measured_real_min_voltageD_in_interval[2] = measured_voltageD[2].Re();
				measured_reactive_max_voltage_in_interval[0] = voltageA.Im();
				measured_reactive_max_voltage_in_interval[1] = voltageB.Im();
				measured_reactive_max_voltage_in_interval[2] = voltageC.Im();
				measured_reactive_max_voltageD_in_interval[0] = measured_voltageD[0].Im();
				measured_reactive_max_voltageD_in_interval[1] = measured_voltageD[1].Im();
				measured_reactive_max_voltageD_in_interval[2] = measured_voltageD[2].Im();
				measured_reactive_min_voltage_in_interval[0] = voltageA.Im();
				measured_reactive_min_voltage_in_interval[1] = voltageB.Im();
				measured_reactive_min_voltage_in_interval[2] = voltageC.Im();
				measured_reactive_min_voltageD_in_interval[0] = measured_voltageD[0].Im();
				measured_reactive_min_voltageD_in_interval[1] = measured_voltageD[1].Im();
				measured_reactive_min_voltageD_in_interval[2] = measured_voltageD[2].Im();
				measured_avg_voltage_mag_in_interval[0] = 0.0;
				measured_avg_voltage_mag_in_interval[1] = 0.0;
				measured_avg_voltage_mag_in_interval[2] = 0.0;
				measured_avg_voltageD_mag_in_interval[0] = 0.0;
				measured_avg_voltageD_mag_in_interval[1] = 0.0;
				measured_avg_voltageD_mag_in_interval[2] = 0.0;
				
				//Power values
				// 3 phase:
				measured_real_max_power_in_interval = measured_real_power;
				measured_real_min_power_in_interval = measured_real_power;
				measured_real_avg_power_in_interval = 0.0;
				measured_reactive_max_power_in_interval = measured_reactive_power;
				measured_reactive_min_power_in_interval = measured_reactive_power;
				measured_reactive_avg_power_in_interval = 0.0;
				// A:
				measured_real_max_power_in_interval_3ph[0] = (indiv_measured_power[0]).Re();
				measured_real_min_power_in_interval_3ph[0] = (indiv_measured_power[0]).Re();
				measured_real_avg_power_in_interval_3ph[0] = 0.0;
                measured_reactive_max_power_in_interval_3ph[0] = (indiv_measured_power[0]).Im();
                measured_reactive_min_power_in_interval_3ph[0] = (indiv_measured_power[0]).Im();
                measured_reactive_avg_power_in_interval_3ph[0] = 0.0;
                // B:
                measured_real_max_power_in_interval_3ph[1] = (indiv_measured_power[1]).Re();
                measured_real_min_power_in_interval_3ph[1] = (indiv_measured_power[1]).Re();
                measured_real_avg_power_in_interval_3ph[1] = 0.0;
                measured_reactive_max_power_in_interval_3ph[1] = (indiv_measured_power[1]).Im();
                measured_reactive_min_power_in_interval_3ph[1] = (indiv_measured_power[1]).Im();
                measured_reactive_avg_power_in_interval_3ph[1] = 0.0;
				// C:
                measured_real_max_power_in_interval_3ph[2] = (indiv_measured_power[2]).Re();
                measured_real_min_power_in_interval_3ph[2] = (indiv_measured_power[2]).Re();
                measured_real_avg_power_in_interval_3ph[2] = 0.0;
                measured_reactive_max_power_in_interval_3ph[2] = (indiv_measured_power[2]).Im();
                measured_reactive_min_power_in_interval_3ph[2] = (indiv_measured_power[2]).Im();
                measured_reactive_avg_power_in_interval_3ph[2] = 0.0;

				if (tretval > last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep)) {
					tretval = last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep);
				}
			}

			if ((t1 > last_stat_timestamp) && (t1 < last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep)) && (t1 != t0)) {
				if (voltage_avg_count <= 0) {
				    // maximums:
					last_measured_max_voltage_mag[0] = voltageA;
					last_measured_max_voltage_mag[1] = voltageB;
					last_measured_max_voltage_mag[2] = voltageC;
					last_measured_max_voltageD_mag[0] = measured_voltageD[0];
					last_measured_max_voltageD_mag[1] = measured_voltageD[1];
					last_measured_max_voltageD_mag[2] = measured_voltageD[2];
					// minimums:
					last_measured_min_voltage_mag[0] = voltageA;
					last_measured_min_voltage_mag[1] = voltageB;
					last_measured_min_voltage_mag[2] = voltageC;
					last_measured_min_voltageD_mag[0] = measured_voltageD[0];
					last_measured_min_voltageD_mag[1] = measured_voltageD[1];
					last_measured_min_voltageD_mag[2] = measured_voltageD[2];
                    // averages:
					last_measured_avg_voltage_mag[0] = last_measured_voltage[0].Mag();
					last_measured_avg_voltage_mag[1] = last_measured_voltage[1].Mag();
					last_measured_avg_voltage_mag[2] = last_measured_voltage[2].Mag();
					last_measured_avg_voltageD_mag[0] = last_measured_voltageD[0].Mag();
					last_measured_avg_voltageD_mag[1] = last_measured_voltageD[1].Mag();
					last_measured_avg_voltageD_mag[2] = last_measured_voltageD[2].Mag();

					//Power
					// 3 phase:
					last_measured_min_real_power = last_measured_real_power;
					last_measured_max_real_power = last_measured_real_power;
					last_measured_avg_real_power = last_measured_real_power;
					last_measured_min_reactive_power = last_measured_reactive_power;
					last_measured_max_reactive_power = last_measured_reactive_power;
					last_measured_avg_reactive_power = last_measured_reactive_power;
					// A:
                    last_measured_min_real_power_3ph[0] = last_measured_real_power_3ph[0];
                    last_measured_max_real_power_3ph[0] = last_measured_real_power_3ph[0];
                    last_measured_avg_real_power_3ph[0] = last_measured_real_power_3ph[0];
                    last_measured_min_reactive_power_3ph[0] = last_measured_reactive_power_3ph[0];
                    last_measured_max_reactive_power_3ph[0] = last_measured_reactive_power_3ph[0];
                    last_measured_avg_reactive_power_3ph[0] = last_measured_reactive_power_3ph[0];
                    // B:
                    last_measured_min_real_power_3ph[1] = last_measured_real_power_3ph[1];
                    last_measured_max_real_power_3ph[1] = last_measured_real_power_3ph[1];
                    last_measured_avg_real_power_3ph[1] = last_measured_real_power_3ph[1];
                    last_measured_min_reactive_power_3ph[1] = last_measured_reactive_power_3ph[1];
                    last_measured_max_reactive_power_3ph[1] = last_measured_reactive_power_3ph[1];
                    last_measured_avg_reactive_power_3ph[1] = last_measured_reactive_power_3ph[1];
                    // C:
                    last_measured_min_real_power_3ph[2] = last_measured_real_power_3ph[2];
                    last_measured_max_real_power_3ph[2] = last_measured_real_power_3ph[2];
                    last_measured_avg_real_power_3ph[2] = last_measured_real_power_3ph[2];
                    last_measured_min_reactive_power_3ph[2] = last_measured_reactive_power_3ph[2];
                    last_measured_max_reactive_power_3ph[2] = last_measured_reactive_power_3ph[2];
                    last_measured_avg_reactive_power_3ph[2] = last_measured_reactive_power_3ph[2];

				} else {
					if ( last_measured_voltage[0].Mag() > last_measured_max_voltage_mag[0].Mag()) {
						last_measured_max_voltage_mag[0] = last_measured_voltage[0];
					}
					if ( last_measured_voltage[1].Mag() > last_measured_max_voltage_mag[1].Mag()) {
						last_measured_max_voltage_mag[1] = last_measured_voltage[1];
					}
					if ( last_measured_voltage[2].Mag() > last_measured_max_voltage_mag[2].Mag()) {
						last_measured_max_voltage_mag[2] = last_measured_voltage[2];
					}
					if (last_measured_voltageD[0].Mag() > last_measured_max_voltageD_mag[0].Mag()) {
						last_measured_max_voltageD_mag[0] = last_measured_voltageD[0];
					}
					if (last_measured_voltageD[1].Mag() > last_measured_max_voltageD_mag[1].Mag()) {
						last_measured_max_voltageD_mag[1] = last_measured_voltageD[1];
					}
					if (last_measured_voltageD[2].Mag() > last_measured_max_voltageD_mag[2].Mag()) {
						last_measured_max_voltageD_mag[2] = last_measured_voltageD[2];
					}
					if ( last_measured_voltage[0].Mag() < last_measured_min_voltage_mag[0].Mag()) {
						last_measured_min_voltage_mag[0] = last_measured_voltage[0];
					}
					if ( last_measured_voltage[0].Mag() < last_measured_min_voltage_mag[1].Mag()) {
						last_measured_min_voltage_mag[1] = last_measured_voltage[0];
					}
					if ( last_measured_voltage[0].Mag() < last_measured_min_voltage_mag[2].Mag()) {
						last_measured_min_voltage_mag[2] = last_measured_voltage[0];
					}
					if (last_measured_voltageD[0].Mag() < last_measured_min_voltageD_mag[0].Mag()) {
						last_measured_min_voltageD_mag[0] = last_measured_voltageD[0];
					}
					if (last_measured_voltageD[1].Mag() < last_measured_min_voltageD_mag[1].Mag()) {
						last_measured_min_voltageD_mag[1] = last_measured_voltageD[1];
					}
					if (last_measured_voltageD[2].Mag() < last_measured_min_voltageD_mag[2].Mag()) {
						last_measured_min_voltageD_mag[2] = last_measured_voltageD[2];
					}

					//Power min/max check
					// 3 phase:
					if (last_measured_max_real_power < last_measured_real_power)
					{
						last_measured_max_real_power = last_measured_real_power;
					}
					if (last_measured_max_reactive_power < last_measured_reactive_power)
					{
						last_measured_max_reactive_power = last_measured_reactive_power;
					}
					if (last_measured_min_real_power > last_measured_real_power)
					{
						last_measured_min_real_power = last_measured_real_power;
					}
					if (last_measured_min_reactive_power > last_measured_reactive_power)
					{
						last_measured_min_reactive_power = last_measured_reactive_power;
					}
					// A phase:
                    if (last_measured_max_real_power_3ph[0] < last_measured_real_power_3ph[0])
                    {
                        last_measured_max_real_power_3ph[0] = last_measured_real_power_3ph[0];
                    }
                    if (last_measured_max_reactive_power_3ph[0] < last_measured_reactive_power_3ph[0])
                    {
                        last_measured_max_reactive_power_3ph[0] = last_measured_reactive_power_3ph[0];
                    }
                    if (last_measured_min_real_power_3ph[0] > last_measured_real_power_3ph[0])
                    {
                        last_measured_min_real_power_3ph[0] = last_measured_real_power_3ph[0];
                    }
                    if (last_measured_min_reactive_power_3ph[0] > last_measured_reactive_power_3ph[0])
                    {
                        last_measured_min_reactive_power_3ph[0] = last_measured_reactive_power_3ph[0];
                    }
					// B phase:
                    if (last_measured_max_real_power_3ph[1] < last_measured_real_power_3ph[1])
                    {
                        last_measured_max_real_power_3ph[1] = last_measured_real_power_3ph[1];
                    }
                    if (last_measured_max_reactive_power_3ph[1] < last_measured_reactive_power_3ph[1])
                    {
                        last_measured_max_reactive_power_3ph[1] = last_measured_reactive_power_3ph[1];
                    }
                    if (last_measured_min_real_power_3ph[1] > last_measured_real_power_3ph[1])
                    {
                        last_measured_min_real_power_3ph[1] = last_measured_real_power_3ph[1];
                    }
                    if (last_measured_min_reactive_power_3ph[1] > last_measured_reactive_power_3ph[1])
                    {
                        last_measured_min_reactive_power_3ph[1] = last_measured_reactive_power_3ph[1];
                    }
					// C phase:
                    if (last_measured_max_real_power_3ph[2] < last_measured_real_power_3ph[2])
                    {
                        last_measured_max_real_power_3ph[2] = last_measured_real_power_3ph[2];
                    }
                    if (last_measured_max_reactive_power_3ph[2] < last_measured_reactive_power_3ph[2])
                    {
                        last_measured_max_reactive_power_3ph[2] = last_measured_reactive_power_3ph[2];
                    }
                    if (last_measured_min_real_power_3ph[2] > last_measured_real_power_3ph[2])
                    {
                        last_measured_min_real_power_3ph[2] = last_measured_real_power_3ph[2];
                    }
                    if (last_measured_min_reactive_power_3ph[2] > last_measured_reactive_power_3ph[2])
                    {
                        last_measured_min_reactive_power_3ph[2] = last_measured_reactive_power_3ph[2];
                    }

                    // Update voltage averages
					last_measured_avg_voltage_mag[0] = ((interval_dt * last_measured_avg_voltage_mag[0]) + (dt * last_measured_voltage[0].Mag()))/(interval_dt + dt);
					last_measured_avg_voltage_mag[1] = ((interval_dt * last_measured_avg_voltage_mag[1]) + (dt * last_measured_voltage[1].Mag()))/(interval_dt + dt);
					last_measured_avg_voltage_mag[2] = ((interval_dt * last_measured_avg_voltage_mag[2]) + (dt * last_measured_voltage[2].Mag()))/(interval_dt + dt);
					last_measured_avg_voltageD_mag[0] = ((interval_dt * last_measured_avg_voltageD_mag[0]) + (dt * last_measured_voltageD[0].Mag()))/(interval_dt + dt);
					last_measured_avg_voltageD_mag[1] = ((interval_dt * last_measured_avg_voltageD_mag[1]) + (dt * last_measured_voltageD[1].Mag()))/(interval_dt + dt);
					last_measured_avg_voltageD_mag[2] = ((interval_dt * last_measured_avg_voltageD_mag[2]) + (dt * last_measured_voltageD[2].Mag()))/(interval_dt + dt);

					//Update the power averages
					// 3 phase:
					last_measured_avg_real_power = ((interval_dt * last_measured_avg_real_power) + (dt * last_measured_real_power))/(dt + interval_dt);
					last_measured_avg_reactive_power = ((interval_dt * last_measured_avg_reactive_power) + (dt * last_measured_reactive_power))/(dt + interval_dt);
					// A phase:
					last_measured_avg_real_power_3ph[0] = ((interval_dt * last_measured_avg_real_power_3ph[0]) + (dt * last_measured_real_power_3ph[0]))/(dt + interval_dt);
                    last_measured_avg_reactive_power_3ph[0] = ((interval_dt * last_measured_avg_reactive_power_3ph[0]) + (dt * last_measured_reactive_power_3ph[0]))/(dt + interval_dt);
                    // B phase:
                    last_measured_avg_real_power_3ph[1] = ((interval_dt * last_measured_avg_real_power_3ph[1]) + (dt * last_measured_real_power_3ph[1]))/(dt + interval_dt);
                    last_measured_avg_reactive_power_3ph[1] = ((interval_dt * last_measured_avg_reactive_power_3ph[1]) + (dt * last_measured_reactive_power_3ph[1]))/(dt + interval_dt);
                    // C phase:
                    last_measured_avg_real_power_3ph[2] = ((interval_dt * last_measured_avg_real_power_3ph[2]) + (dt * last_measured_real_power_3ph[2]))/(dt + interval_dt);
                    last_measured_avg_reactive_power_3ph[2] = ((interval_dt * last_measured_avg_reactive_power_3ph[2]) + (dt * last_measured_reactive_power_3ph[2]))/(dt + interval_dt);
				}

				if (t1 != last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep)) {
					voltage_avg_count++;
					interval_dt = interval_dt + dt;
				}
				if (tretval > last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep)) {
					tretval = last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep);
				}
			}

			if ((t1 == last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep)) && (t1 != t0) && measured_min_max_avg_timestep > 0) {
				last_stat_timestamp = t1;
				if ( last_measured_voltage[0].Mag() > last_measured_max_voltage_mag[0].Mag()) {
					last_measured_max_voltage_mag[0] = last_measured_voltage[0];
				}
				if ( last_measured_voltage[1].Mag() > last_measured_max_voltage_mag[1].Mag()) {
					last_measured_max_voltage_mag[1] = last_measured_voltage[1];
				}
				if ( last_measured_voltage[2].Mag() > last_measured_max_voltage_mag[2].Mag()) {
					last_measured_max_voltage_mag[2] = last_measured_voltage[2];
				}
				if (last_measured_voltageD[0].Mag() > last_measured_max_voltageD_mag[0].Mag()) {
					last_measured_max_voltageD_mag[0] = last_measured_voltageD[0];
				}
				if (last_measured_voltageD[1].Mag() > last_measured_max_voltageD_mag[1].Mag()) {
					last_measured_max_voltageD_mag[1] = last_measured_voltageD[1];
				}
				if (last_measured_voltageD[2].Mag() > last_measured_max_voltageD_mag[2].Mag()) {
					last_measured_max_voltageD_mag[2] = last_measured_voltageD[2];
				}
				if ( last_measured_voltage[0].Mag() < last_measured_min_voltage_mag[0].Mag()) {
					last_measured_min_voltage_mag[0] = last_measured_voltage[0];
				}
				if ( last_measured_voltage[0].Mag() < last_measured_min_voltage_mag[1].Mag()) {
					last_measured_min_voltage_mag[1] = last_measured_voltage[0];
				}
				if ( last_measured_voltage[0].Mag() < last_measured_min_voltage_mag[2].Mag()) {
					last_measured_min_voltage_mag[2] = last_measured_voltage[0];
				}
				if (last_measured_voltageD[0].Mag() < last_measured_min_voltageD_mag[0].Mag()) {
					last_measured_min_voltageD_mag[0] = last_measured_voltageD[0];
				}
				if (last_measured_voltageD[1].Mag() < last_measured_min_voltageD_mag[1].Mag()) {
					last_measured_min_voltageD_mag[1] = last_measured_voltageD[1];
				}
				if (last_measured_voltageD[2].Mag() < last_measured_min_voltageD_mag[2].Mag()) {
					last_measured_min_voltageD_mag[2] = last_measured_voltageD[2];
				}

				//Power min/max check
				// 3 phase:
				if (last_measured_max_real_power < last_measured_real_power)
				{
					last_measured_max_real_power = last_measured_real_power;
				}
				if (last_measured_max_reactive_power < last_measured_reactive_power)
				{
					last_measured_max_reactive_power = last_measured_reactive_power;
				}
				if (last_measured_min_real_power > last_measured_real_power)
				{
					last_measured_min_real_power = last_measured_real_power;
				}
				if (last_measured_min_reactive_power > last_measured_reactive_power)
				{
					last_measured_min_reactive_power = last_measured_reactive_power;
				}
				// A phase:
                if (last_measured_max_real_power_3ph[0] < last_measured_real_power_3ph[0])
                {
                    last_measured_max_real_power_3ph[0] = last_measured_real_power_3ph[0];
                }
                if (last_measured_max_reactive_power_3ph[0] < last_measured_reactive_power_3ph[0])
                {
                    last_measured_max_reactive_power_3ph[0] = last_measured_reactive_power_3ph[0];
                }
                if (last_measured_min_real_power_3ph[0] > last_measured_real_power_3ph[0])
                {
                    last_measured_min_real_power_3ph[0] = last_measured_real_power_3ph[0];
                }
                if (last_measured_min_reactive_power_3ph[0] > last_measured_reactive_power_3ph[0])
                {
                    last_measured_min_reactive_power_3ph[0] = last_measured_reactive_power_3ph[0];
                }
				// B phase:
                if (last_measured_max_real_power_3ph[1] < last_measured_real_power_3ph[1])
                {
                    last_measured_max_real_power_3ph[1] = last_measured_real_power_3ph[1];
                }
                if (last_measured_max_reactive_power_3ph[1] < last_measured_reactive_power_3ph[1])
                {
                    last_measured_max_reactive_power_3ph[1] = last_measured_reactive_power_3ph[1];
                }
                if (last_measured_min_real_power_3ph[1] > last_measured_real_power_3ph[1])
                {
                    last_measured_min_real_power_3ph[1] = last_measured_real_power_3ph[1];
                }
                if (last_measured_min_reactive_power_3ph[1] > last_measured_reactive_power_3ph[1])
                {
                    last_measured_min_reactive_power_3ph[1] = last_measured_reactive_power_3ph[1];
                }
				// C phase:
                if (last_measured_max_real_power_3ph[2] < last_measured_real_power_3ph[2])
                {
                    last_measured_max_real_power_3ph[2] = last_measured_real_power_3ph[2];
                }
                if (last_measured_max_reactive_power_3ph[2] < last_measured_reactive_power_3ph[2])
                {
                    last_measured_max_reactive_power_3ph[2] = last_measured_reactive_power_3ph[2];
                }
                if (last_measured_min_real_power_3ph[2] > last_measured_real_power_3ph[2])
                {
                    last_measured_min_real_power_3ph[2] = last_measured_real_power_3ph[2];
                }
                if (last_measured_min_reactive_power_3ph[2] > last_measured_reactive_power_3ph[2])
                {
                    last_measured_min_reactive_power_3ph[2] = last_measured_reactive_power_3ph[2];
                }

				last_measured_avg_voltage_mag[0] = ((interval_dt * last_measured_avg_voltage_mag[0]) + (dt * last_measured_voltage[0].Mag()))/(interval_dt + dt);
				last_measured_avg_voltage_mag[1] = ((interval_dt * last_measured_avg_voltage_mag[1]) + (dt * last_measured_voltage[1].Mag()))/(interval_dt + dt);
				last_measured_avg_voltage_mag[2] = ((interval_dt * last_measured_avg_voltage_mag[2]) + (dt * last_measured_voltage[2].Mag()))/(interval_dt + dt);
				last_measured_avg_voltageD_mag[0] = ((interval_dt * last_measured_avg_voltageD_mag[0]) + (dt * last_measured_voltageD[0].Mag()))/(interval_dt + dt);
				last_measured_avg_voltageD_mag[1] = ((interval_dt * last_measured_avg_voltageD_mag[1]) + (dt * last_measured_voltageD[1].Mag()))/(interval_dt + dt);
				last_measured_avg_voltageD_mag[2] = ((interval_dt * last_measured_avg_voltageD_mag[2]) + (dt * last_measured_voltageD[2].Mag()))/(interval_dt + dt);

				//Update the power averages
				// 3 phase:
				last_measured_avg_real_power = ((interval_dt * last_measured_avg_real_power) + (dt * last_measured_real_power))/(dt + interval_dt);
				last_measured_avg_reactive_power = ((interval_dt * last_measured_avg_reactive_power) + (dt * last_measured_reactive_power))/(dt + interval_dt);
				// A phase:
                last_measured_avg_real_power_3ph[0] = ((interval_dt * last_measured_avg_real_power_3ph[0]) + (dt * last_measured_real_power_3ph[0]))/(dt + interval_dt);
                last_measured_avg_reactive_power_3ph[0] = ((interval_dt * last_measured_avg_reactive_power_3ph[0]) + (dt * last_measured_reactive_power_3ph[0]))/(dt + interval_dt);
				// B phase:
                last_measured_avg_real_power_3ph[1] = ((interval_dt * last_measured_avg_real_power_3ph[1]) + (dt * last_measured_real_power_3ph[1]))/(dt + interval_dt);
                last_measured_avg_reactive_power_3ph[1] = ((interval_dt * last_measured_avg_reactive_power_3ph[1]) + (dt * last_measured_reactive_power_3ph[1]))/(dt + interval_dt);
				// C phase:
                last_measured_avg_real_power_3ph[2] = ((interval_dt * last_measured_avg_real_power_3ph[2]) + (dt * last_measured_real_power_3ph[2]))/(dt + interval_dt);
                last_measured_avg_reactive_power_3ph[2] = ((interval_dt * last_measured_avg_reactive_power_3ph[2]) + (dt * last_measured_reactive_power_3ph[2]))/(dt + interval_dt);

				interval_dt = 0;
				voltage_avg_count = 0;
				measured_real_max_voltage_in_interval[0] = last_measured_max_voltage_mag[0].Re();
				measured_real_max_voltage_in_interval[1] = last_measured_max_voltage_mag[1].Re();
				measured_real_max_voltage_in_interval[2] = last_measured_max_voltage_mag[2].Re();
				measured_real_max_voltageD_in_interval[0] = last_measured_max_voltageD_mag[0].Re();
				measured_real_max_voltageD_in_interval[1] = last_measured_max_voltageD_mag[1].Re();
				measured_real_max_voltageD_in_interval[2] = last_measured_max_voltageD_mag[2].Re();
				measured_real_min_voltage_in_interval[0] = last_measured_min_voltage_mag[0].Re();
				measured_real_min_voltage_in_interval[1] = last_measured_min_voltage_mag[1].Re();
				measured_real_min_voltage_in_interval[2] = last_measured_min_voltage_mag[2].Re();
				measured_real_min_voltageD_in_interval[0] = last_measured_min_voltageD_mag[0].Re();
				measured_real_min_voltageD_in_interval[1] = last_measured_min_voltageD_mag[1].Re();
				measured_real_min_voltageD_in_interval[2] = last_measured_min_voltageD_mag[2].Re();
				measured_reactive_max_voltage_in_interval[0] = last_measured_max_voltageD_mag[0].Im();
				measured_reactive_max_voltage_in_interval[1] = last_measured_max_voltageD_mag[1].Im();
				measured_reactive_max_voltage_in_interval[2] = last_measured_max_voltageD_mag[2].Im();
				measured_reactive_max_voltageD_in_interval[0] = last_measured_max_voltageD_mag[0].Im();
				measured_reactive_max_voltageD_in_interval[1] = last_measured_max_voltageD_mag[1].Im();
				measured_reactive_max_voltageD_in_interval[2] = last_measured_max_voltageD_mag[2].Im();
				measured_reactive_min_voltage_in_interval[0] = last_measured_min_voltage_mag[0].Im();
				measured_reactive_min_voltage_in_interval[1] = last_measured_min_voltage_mag[1].Im();
				measured_reactive_min_voltage_in_interval[2] = last_measured_min_voltage_mag[2].Im();
				measured_reactive_min_voltageD_in_interval[0] = last_measured_min_voltageD_mag[0].Im();
				measured_reactive_min_voltageD_in_interval[1] = last_measured_min_voltageD_mag[1].Im();
				measured_reactive_min_voltageD_in_interval[2] = last_measured_min_voltageD_mag[2].Im();
                measured_avg_voltage_mag_in_interval[0] = last_measured_avg_voltage_mag[0];
                measured_avg_voltage_mag_in_interval[1] = last_measured_avg_voltage_mag[1];
                measured_avg_voltage_mag_in_interval[2] = last_measured_avg_voltage_mag[2];
                measured_avg_voltageD_mag_in_interval[0] = last_measured_avg_voltageD_mag[0];
                measured_avg_voltageD_mag_in_interval[1] = last_measured_avg_voltageD_mag[1];
                measured_avg_voltageD_mag_in_interval[2] = last_measured_avg_voltageD_mag[2];

				//Power values
				// 3 phase:
				measured_real_max_power_in_interval = last_measured_max_real_power;
				measured_real_min_power_in_interval = last_measured_min_real_power;
				measured_real_avg_power_in_interval = last_measured_avg_real_power;
				measured_reactive_max_power_in_interval = last_measured_max_reactive_power;
				measured_reactive_min_power_in_interval = last_measured_min_reactive_power;
				measured_reactive_avg_power_in_interval = last_measured_avg_reactive_power;
				// A phase:
				measured_real_max_power_in_interval_3ph[0] = last_measured_max_real_power_3ph[0];
				measured_real_min_power_in_interval_3ph[0] = last_measured_min_real_power_3ph[0];
				measured_real_avg_power_in_interval_3ph[0] = last_measured_avg_real_power_3ph[0];
				measured_reactive_max_power_in_interval_3ph[0] = last_measured_max_reactive_power_3ph[0];
				measured_reactive_min_power_in_interval_3ph[0] = last_measured_min_reactive_power_3ph[0];
				measured_reactive_avg_power_in_interval_3ph[0] = last_measured_avg_reactive_power_3ph[0];
				// B phase:
				measured_real_max_power_in_interval_3ph[1] = last_measured_max_real_power_3ph[1];
				measured_real_min_power_in_interval_3ph[1] = last_measured_min_real_power_3ph[1];
				measured_real_avg_power_in_interval_3ph[1] = last_measured_avg_real_power_3ph[1];
				measured_reactive_max_power_in_interval_3ph[1] = last_measured_max_reactive_power_3ph[1];
				measured_reactive_min_power_in_interval_3ph[1] = last_measured_min_reactive_power_3ph[1];
				measured_reactive_avg_power_in_interval_3ph[1] = last_measured_avg_reactive_power_3ph[1];
				// C phase:
				measured_real_max_power_in_interval_3ph[2] = last_measured_max_real_power_3ph[2];
				measured_real_min_power_in_interval_3ph[2] = last_measured_min_real_power_3ph[2];
				measured_real_avg_power_in_interval_3ph[2] = last_measured_avg_real_power_3ph[2];
				measured_reactive_max_power_in_interval_3ph[2] = last_measured_max_reactive_power_3ph[2];
				measured_reactive_min_power_in_interval_3ph[2] = last_measured_min_reactive_power_3ph[2];
				measured_reactive_avg_power_in_interval_3ph[2] = last_measured_avg_reactive_power_3ph[2];

				if (tretval > last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep)) {
					tretval = last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep);
				}
			}
		}//End Min/Max/Stat calculation

		monthly_energy = measured_real_energy/1000 - previous_energy_total;

		if (bill_mode == BM_UNIFORM || bill_mode == BM_TIERED)
		{
			if (dt > 0)
				process_bill(t1);

			// Decide when the next billing HAS to be processed (one month later)
			if (monthly_bill == previous_monthly_bill)
			{
				DATETIME t_next;
				gl_localtime(t1,&t_next);

				t_next.day = bill_day;

				if (t_next.month != 12)
					t_next.month += 1;
				else
				{
					t_next.month = 1;
					t_next.year += 1;
				}
				t_next.tz[0] = 0;
				next_time =	gl_mktime(&t_next);
			}
		}

		if(bill_mode == BM_HOURLY || bill_mode == BM_TIERED_RTP || bill_mode == BM_TIERED_TOU){
			double seconds;
			if (dt != last_t)
				seconds = (double)(dt);
			else
				seconds = 0;
			
			if (seconds > 0)
			{
				double acc_price = price;
				if (bill_mode == BM_TIERED_TOU)
				{
					if(monthly_energy < tier_energy[0])
						acc_price = last_price;
					else if(monthly_energy < tier_energy[1])
						acc_price = last_tier_price[0];
					else if(monthly_energy < tier_energy[2])
						acc_price = last_tier_price[1];
					else
						acc_price = last_tier_price[2];
				}
				hourly_acc += seconds/3600 * acc_price * last_measured_real_power/1000;
				process_bill(t1);
			}

			// Now that we've accumulated the bill for the last time period, update to the new price (if using the market)
			if (bill_mode != BM_TIERED_TOU && power_market != nullptr && price_prop != nullptr)
			{
				double *pprice = (gl_get_double(power_market, price_prop));
				last_price = price = *pprice;
			}
			last_measured_real_power = measured_real_power;
			last_measured_real_power_3ph[0] = (indiv_measured_power[0]).Re();
			last_measured_real_power_3ph[1] = (indiv_measured_power[1]).Re();
			last_measured_real_power_3ph[2] = (indiv_measured_power[2]).Re();

			// copied logic on when the next bill must be processed
			if (monthly_bill == previous_monthly_bill)
			{
				DATETIME t_next;
				gl_localtime(t1,&t_next);

				t_next.day = bill_day;

				if (t_next.month != 12)
					t_next.month += 1;
				else
				{
					t_next.month = 1;
					t_next.year += 1;
				}
				t_next.tz[0] = 0;
				next_time =	gl_mktime(&t_next);
			}
		}

		//Update trackers -- could probably be one more level out
		last_measured_real_power = measured_real_power;
		last_measured_reactive_power = measured_reactive_power;
        last_measured_real_power_3ph[0] = (indiv_measured_power[0]).Re();
        last_measured_real_power_3ph[1] = (indiv_measured_power[1]).Re();
        last_measured_real_power_3ph[2] = (indiv_measured_power[2]).Re();
        last_measured_reactive_power_3ph[0] = (indiv_measured_power[0]).Im();
        last_measured_reactive_power_3ph[1] = (indiv_measured_power[1]).Im();
        last_measured_reactive_power_3ph[2] = (indiv_measured_power[2]).Im();

        last_measured_voltage[0] = measured_voltage[0];
        last_measured_voltage[1] = measured_voltage[1];
        last_measured_voltage[2] = measured_voltage[2];
        last_measured_voltageD[0] = measured_voltageD[0];
        last_measured_voltageD[1] = measured_voltageD[1];
        last_measured_voltageD[2] = measured_voltageD[2];
	}

	//Multi run (for now) updates to power values
	if (meter_NR_servered)
	{
		// compute demand power
		indiv_measured_power[0] = voltage[0]*(~current_inj[0]);
		indiv_measured_power[1] = voltage[1]*(~current_inj[1]);
		indiv_measured_power[2] = voltage[2]*(~current_inj[2]);
	}

	return tretval;
}

double meter::process_bill(TIMESTAMP t1){
	DATETIME dtime;
	gl_localtime(t1,&dtime);

	monthly_bill = monthly_fee;
	switch(bill_mode){
		case BM_NONE:
			break;
		case BM_UNIFORM:
			monthly_bill += monthly_energy * price;
			break;
		case BM_TIERED:
			if(monthly_energy < tier_energy[0])
				monthly_bill += last_price * monthly_energy;
			else if(monthly_energy < tier_energy[1])
				monthly_bill += last_price*tier_energy[0] + last_tier_price[0]*(monthly_energy - tier_energy[0]);
			else if(monthly_energy < tier_energy[2])
				monthly_bill += last_price*tier_energy[0] + last_tier_price[0]*(tier_energy[1] - tier_energy[0]) + last_tier_price[1]*(monthly_energy - tier_energy[1]);
			else
				monthly_bill += last_price*tier_energy[0] + last_tier_price[0]*(tier_energy[1] - tier_energy[0]) + last_tier_price[1]*(tier_energy[2] - tier_energy[1]) + last_tier_price[2]*(monthly_energy - tier_energy[2]);
			break;
		case BM_HOURLY:
		case BM_TIERED_TOU:
			monthly_bill += hourly_acc;
			break;
		case BM_TIERED_RTP:
			monthly_bill += hourly_acc;
			if(monthly_energy < tier_energy[0])
				monthly_bill += last_price_base * monthly_energy;
			else if(monthly_energy < tier_energy[1])
				monthly_bill += last_price_base*tier_energy[0] + last_tier_price[0]*(monthly_energy - tier_energy[0]);
			else if(monthly_energy < tier_energy[2])
				monthly_bill += last_price_base*tier_energy[0] + last_tier_price[0]*(tier_energy[1] - tier_energy[0]) + last_tier_price[1]*(monthly_energy - tier_energy[1]);
			else
				monthly_bill += last_price_base*tier_energy[0] + last_tier_price[0]*(tier_energy[1] - tier_energy[0]) + last_tier_price[1]*(tier_energy[2] - tier_energy[1]) + last_tier_price[2]*(monthly_energy - tier_energy[2]);
			break;
	}

	if (dtime.day == bill_day && dtime.hour == 0 && dtime.month != last_bill_month)
	{
		previous_monthly_bill = monthly_bill;
		previous_monthly_energy = monthly_energy;
		previous_energy_total = measured_real_energy/1000;
		last_bill_month = dtime.month;
		hourly_acc = 0;
	}

	last_price = price;
	last_price_base = price_base;
	last_tier_price[0] = tier_price[0];
	last_tier_price[1] = tier_price[1];
	last_tier_price[2] = tier_price[2];

	return monthly_bill;
}

//Module-level call
SIMULATIONMODE meter::inter_deltaupdate_meter(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	OBJECT *hdr = OBJECTHDR(this);
	double deltat;
	STATUS return_status_val;

	//Create delta_t variable
	deltat = (double)dt/(double)DT_SECOND;

	//Update time tracking variable - mostly for GFA functionality calls
	if ((iteration_count_val==0) && !interupdate_pos) //Only update timestamp tracker on first iteration
	{
		//Update tracking variable
		prev_time_dbl = gl_globaldeltaclock;

		//Update frequency calculation values (if needed)
		if (fmeas_type != FM_NONE)
		{
			//See which pass
			if (delta_time == 0)
			{
				//Initialize dynamics - first run of new delta call
				init_freq_dynamics(deltat);
			}
			else
			{
				//Copy the tracker value
				memcpy(&prev_freq_state,&curr_freq_state,sizeof(FREQM_STATES));
			}
		}
	}

	//Perform the GFA update, if enabled
	if (GFA_enable && (iteration_count_val == 0) && !interupdate_pos)	//Always just do on the first pass
	{
		//Do the checks
		GFA_Update_time = perform_GFA_checks(deltat);
	}

	if (!interupdate_pos)	//Before powerflow call
	{
		//Meter-specific presync items
		if (meter_power_consumption != gld::complex(0,0))
			power[0] = power[1] = power[2] = 0.0;

		//Reliability addition - if momentary flag set - clear it
		if (meter_interrupted_secondary)
			meter_interrupted_secondary = false;

		//Call presync-equivalent items
		NR_node_presync_fxn(0);

		//Meter sync objects
		BOTH_meter_sync_fxn();

		//Call sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call

	}//End Before NR solver (or inclusive)
	else	//After the call
	{
		//Perform postsync-like updates on the values
		BOTH_node_postsync_fxn(hdr);

		//Frequency measurement stuff
		if (fmeas_type != FM_NONE)
		{
			return_status_val = calc_freq_dynamics(deltat);

			//Check it
			if (return_status_val == FAILED)
			{
				return SM_ERROR;
			}
		}//End frequency measurement desired
		//Default else -- don't calculate it

		//Perform postsync meter functions -- doesn't do energy right now
		measured_voltage[0] = voltageA;
		measured_voltage[1] = voltageB;
		measured_voltage[2] = voltageC;

		measured_voltageD[0] = voltageA - voltageB;
		measured_voltageD[1] = voltageB - voltageC;
		measured_voltageD[2] = voltageC - voltageA;
		
		measured_current[0] = current_inj[0];
		measured_current[1] = current_inj[1];
		measured_current[2] = current_inj[2];

		// compute demand power
		indiv_measured_power[0] = measured_voltage[0]*(~measured_current[0]);
		indiv_measured_power[1] = measured_voltage[1]*(~measured_current[1]);
		indiv_measured_power[2] = measured_voltage[2]*(~measured_current[2]);

		measured_power = indiv_measured_power[0] + indiv_measured_power[1] + indiv_measured_power[2];

		measured_real_power = (indiv_measured_power[0]).Re()
							+ (indiv_measured_power[1]).Re()
							+ (indiv_measured_power[2]).Re();

		measured_reactive_power = (indiv_measured_power[0]).Im()
								+ (indiv_measured_power[1]).Im()
								+ (indiv_measured_power[2]).Im();

		if (measured_real_power > measured_demand) 
			measured_demand = measured_real_power;

		//See if GFA functionality is required, since it may require iterations or "continance"
		if (GFA_enable)
		{
			//See if our return is value
			if ((GFA_Update_time > 0.0) && (GFA_Update_time < 1.7))
			{
				//Force us to stay
				return SM_DELTA;
			}
			else	//Just return whatever we were going to do
			{
				return SM_EVENT;
			}
		}
		else	//Normal mode
		{
			return SM_EVENT;
		}
	}//End "After NR solver" branch
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int isa_meter(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,meter)->isa(classname);
}

EXPORT int create_meter(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(meter::oclass);
		if (*obj!=nullptr)
		{
			meter *my = OBJECTDATA(*obj,meter);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(meter);
}

EXPORT int init_meter(OBJECT *obj)
{
	try {
		meter *my = OBJECTDATA(obj,meter);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(meter);
}

EXPORT TIMESTAMP sync_meter(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		meter *pObj = OBJECTDATA(obj,meter);
		TIMESTAMP t1;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(obj->clock,t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
		throw "invalid pass request";
	}
	SYNC_CATCHALL(meter);
}

EXPORT int notify_meter(OBJECT *obj, int update_mode, PROPERTY *prop, char *value){
	meter *n = OBJECTDATA(obj, meter);
	int rv = 1;

	rv = n->notify(update_mode, prop, value);

	return rv;
}

//Deltamode export
EXPORT SIMULATIONMODE interupdate_meter(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	meter *my = OBJECTDATA(obj,meter);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_meter(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_meter(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

//KML Export
EXPORT int meter_kmldata(OBJECT *obj,int (*stream)(const char*,...))
{
	meter *n = OBJECTDATA(obj, meter);
	int rv = 1;

	rv = n->kmldata(stream);

	return rv;
}

int meter::kmldata(int (*stream)(const char*,...))
{
	int phase[3] = {has_phase(PHASE_A),has_phase(PHASE_B),has_phase(PHASE_C)};
	double basis[3] = {0,240,120};

	// power
	stream("<TR><TH ALIGN=LEFT>Power</TH>");
	for ( int i = 0 ; i<sizeof(phase)/sizeof(phase[0]) ; i++ )
	{
		if ( phase[i] )
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>kVA</TD>", indiv_measured_power[i].Mag()/1000);
		else
			stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\">&mdash;</TD><TD>&nbsp;</TD>");
	}
	stream("</TR>\n");

	// energy
	stream("<TR><TH ALIGN=LEFT>Energy</TH>");
	stream("<TD ALIGN=RIGHT COLSPAN=3 STYLE=\"font-family:courier;\"><NOBR>%.3f kWh</NOBR></TD>", measured_real_energy/1000);
	stream("<TD ALIGN=RIGHT COLSPAN=3 STYLE=\"font-family:courier;\"><NOBR>%.3f kVARh</NOBR></TD>", measured_reactive_energy/1000);
	stream("</TR>\n");

	return 0;
}

/**@}**/
