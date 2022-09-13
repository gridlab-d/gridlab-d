/** $Id: triplex_meter.cpp 1002 2008-09-29 15:58:23Z d3m998 $
	Copyright (C) 2008 Battelle Memorial Institute
	@file triplex_meter.cpp
	@addtogroup powerflow_triplex_meter Meter
	@ingroup powerflow

	Distribution triplex_meter can be either single phase or polyphase triplex_meters.
	Single phase triplex_meters present three lines to objects
	- Line 1-G: 120V,
	- Line 2-G: 120V
	- Line 3-G: 0V
	- Line 1-2: 240V
	- Line 2-3: 120V
	- Line 1-3: 120V

	Total cumulative energy, instantantenous power and peak demand are triplex_metered.

	@{
 **/
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "triplex_meter.h"

// useful macros
#define TO_HOURS(t) (((double)t) / (3600 * TS_SECOND))

// meter reset function
EXPORT int64 triplex_meter_reset(OBJECT *obj)
{
	triplex_meter *pMeter = OBJECTDATA(obj,triplex_meter);
	pMeter->measured_demand = 0;
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// triplex_meter CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
// class management data
CLASS* triplex_meter::oclass = nullptr;
CLASS* triplex_meter::pclass = nullptr;

// the constructor registers the class and properties and sets the defaults
triplex_meter::triplex_meter(MODULE *mod) : triplex_node(mod)
{
	// first time init
	if (oclass==nullptr)
	{
		// link to parent class (used by isa)
		pclass = triplex_node::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"triplex_meter",sizeof(triplex_meter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class triplex_meter";
		else
			oclass->trl = TRL_PROVEN;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "triplex_node",
			PT_double, "measured_real_energy[Wh]", PADDR(measured_real_energy),PT_DESCRIPTION,"metered real energy consumption",
			PT_double, "measured_real_energy_delta[Wh]", PADDR(measured_real_energy_delta),PT_DESCRIPTION,"delta in metered real energy consumption from last specified measured_energy_delta_timestep",
			PT_double, "measured_reactive_energy[VAh]",PADDR(measured_reactive_energy),PT_DESCRIPTION,"metered reactive energy consumption",
			PT_double, "measured_reactive_energy_delta[VAh]",PADDR(measured_reactive_energy_delta),PT_DESCRIPTION,"delta in metered reactive energy consumption from last specified measured_energy_delta_timestep",
            PT_double, "measured_energy_delta_timestep[s]",PADDR(measured_energy_delta_timestep),PT_DESCRIPTION,"Period of timestep for real and reactive delta energy calculation",
			PT_complex, "measured_power[VA]", PADDR(measured_power),PT_DESCRIPTION,"metered power",
			PT_complex, "indiv_measured_power_1[VA]", PADDR(indiv_measured_power[0]),PT_DESCRIPTION,"metered power, phase 1",
			PT_complex, "indiv_measured_power_2[VA]", PADDR(indiv_measured_power[1]),PT_DESCRIPTION,"metered power, phase 2",
			PT_complex, "indiv_measured_power_N[VA]", PADDR(indiv_measured_power[2]),PT_DESCRIPTION,"metered power, phase N",
			PT_double, "measured_demand[W]", PADDR(measured_demand),PT_DESCRIPTION,"metered demand (peak of power)",
			PT_double, "measured_real_power[W]", PADDR(measured_real_power),PT_DESCRIPTION,"metered real power",
			PT_double, "measured_reactive_power[VAr]", PADDR(measured_reactive_power),PT_DESCRIPTION,"metered reactive power",
			PT_complex, "meter_power_consumption[VA]", PADDR(tpmeter_power_consumption),PT_DESCRIPTION,"power consumed by meter operation",

			// added to record last voltage/current
			PT_complex, "measured_voltage_1[V]", PADDR(measured_voltage[0]),PT_DESCRIPTION,"measured voltage, phase 1 to ground",
			PT_complex, "measured_voltage_2[V]", PADDR(measured_voltage[1]),PT_DESCRIPTION,"measured voltage, phase 2 to ground",
			PT_complex, "measured_voltage_N[V]", PADDR(measured_voltage[2]),PT_DESCRIPTION,"measured voltage, phase N to ground",
			
			//Voltage average items
			PT_double, "measured_real_max_voltage_1_in_interval[V]", PADDR(measured_real_max_voltage_in_interval[0]),PT_DESCRIPTION,"measured real max line-to-ground voltage on phase 1 over a specified interval",
			PT_double, "measured_real_max_voltage_2_in_interval[V]", PADDR(measured_real_max_voltage_in_interval[1]),PT_DESCRIPTION,"measured real max line-to-ground voltage on phase 2 over a specified interval",
			PT_double, "measured_real_max_voltage_12_in_interval[V]", PADDR(measured_real_max_voltage_in_interval[2]),PT_DESCRIPTION,"measured real max line-to-ground voltage on phase 12 over a specified interval",
			PT_double, "measured_imag_max_voltage_1_in_interval[V]", PADDR(measured_imag_max_voltage_in_interval[0]),PT_DESCRIPTION,"measured imaginary max line-to-ground voltage on phase 1 over a specified interval",
			PT_double, "measured_imag_max_voltage_2_in_interval[V]", PADDR(measured_imag_max_voltage_in_interval[1]),PT_DESCRIPTION,"measured imaginary max line-to-ground voltage on phase 2 over a specified interval",
			PT_double, "measured_imag_max_voltage_12_in_interval[V]", PADDR(measured_imag_max_voltage_in_interval[2]),PT_DESCRIPTION,"measured imaginary max line-to-ground voltage on phase 12 over a specified interval",
			PT_double, "measured_real_min_voltage_1_in_interval[V]", PADDR(measured_real_min_voltage_in_interval[0]),PT_DESCRIPTION,"measured real min line-to-ground voltage on phase 1 over a specified interval",
			PT_double, "measured_real_min_voltage_2_in_interval[V]", PADDR(measured_real_min_voltage_in_interval[1]),PT_DESCRIPTION,"measured real min line-to-ground voltage on phase 2 over a specified interval",
			PT_double, "measured_real_min_voltage_12_in_interval[V]", PADDR(measured_real_min_voltage_in_interval[2]),PT_DESCRIPTION,"measured real min line-to-ground voltage on phase 12 over a specified interval",
			PT_double, "measured_imag_min_voltage_1_in_interval[V]", PADDR(measured_imag_min_voltage_in_interval[0]),PT_DESCRIPTION,"measured imaginary min line-to-ground voltage on phase 1 over a specified interval",
			PT_double, "measured_imag_min_voltage_2_in_interval[V]", PADDR(measured_imag_min_voltage_in_interval[1]),PT_DESCRIPTION,"measured imaginary min line-to-ground voltage on phase 2 over a specified interval",
			PT_double, "measured_imag_min_voltage_12_in_interval[V]", PADDR(measured_imag_min_voltage_in_interval[2]),PT_DESCRIPTION,"measured imaginary min line-to-ground voltage on phase 12 over a specified interval",
			PT_double, "measured_avg_voltage_1_mag_in_interval[V]", PADDR(measured_avg_voltage_mag_in_interval[0]),PT_DESCRIPTION,"measured average line-to-ground voltage magnitude on phase 1 over a specified interval",
			PT_double, "measured_avg_voltage_2_mag_in_interval[V]", PADDR(measured_avg_voltage_mag_in_interval[1]),PT_DESCRIPTION,"measured average line-to-ground voltage magnitude on phase 2 over a specified interval",
			PT_double, "measured_avg_voltage_12_mag_in_interval[V]", PADDR(measured_avg_voltage_mag_in_interval[2]),PT_DESCRIPTION,"measured average line-to-ground voltage magnitude on phase 12 over a specified interval",

			//power average items
			PT_double, "measured_real_max_power_in_interval[W]", PADDR(measured_real_max_power_in_interval),PT_DESCRIPTION,"measured maximum real power over a specified interval",
			PT_double, "measured_reactive_max_power_in_interval[VAr]", PADDR(measured_reactive_max_power_in_interval),PT_DESCRIPTION,"measured maximum reactive power over a specified interval",
			PT_double, "measured_real_min_power_in_interval[W]", PADDR(measured_real_min_power_in_interval),PT_DESCRIPTION,"measured minimum real power over a specified interval",
			PT_double, "measured_reactive_min_power_in_interval[VAr]", PADDR(measured_reactive_min_power_in_interval),PT_DESCRIPTION,"measured minimum reactive power over a specified interval",
			PT_double, "measured_avg_real_power_in_interval[W]", PADDR(measured_real_avg_power_in_interval),PT_DESCRIPTION,"measured average real power over a specified interval",
			PT_double, "measured_avg_reactive_power_in_interval[VAr]", PADDR(measured_reactive_avg_power_in_interval),PT_DESCRIPTION,"measured average reactive power over a specified interval",

			//Interval for the min/max/averages
            PT_double, "measured_stats_interval[s]",PADDR(measured_min_max_avg_timestep),PT_DESCRIPTION,"Period of timestep for min/max/average calculations",

			PT_complex, "measured_current_1[A]", PADDR(measured_current[0]),PT_DESCRIPTION,"measured current, phase 1",
			PT_complex, "measured_current_2[A]", PADDR(measured_current[1]),PT_DESCRIPTION,"measured current, phase 2",
			PT_complex, "measured_current_N[A]", PADDR(measured_current[2]),PT_DESCRIPTION,"measured current, phase N",
			PT_bool, "customer_interrupted", PADDR(tpmeter_interrupted),PT_DESCRIPTION,"Reliability flag - goes active if the customer is in an interrupted state",
			PT_bool, "customer_interrupted_secondary", PADDR(tpmeter_interrupted_secondary),PT_DESCRIPTION,"Reliability flag - goes active if the customer is in a secondary interrupted state - i.e., momentary",
#ifdef SUPPORT_OUTAGES
			PT_int16, "sustained_count", PADDR(sustained_count),PT_DESCRIPTION,"reliability sustained event counter",
			PT_int16, "momentary_count", PADDR(momentary_count),PT_DESCRIPTION,"reliability momentary event counter",
			PT_int16, "total_count", PADDR(total_count),PT_DESCRIPTION,"reliability total event counter",
			PT_int16, "s_flag", PADDR(s_flag),PT_DESCRIPTION,"reliability flag that gets set if the meter experienced more than n sustained interruptions",
			PT_int16, "t_flag", PADDR(t_flag),PT_DESCRIPTION,"reliability flage that gets set if the meter experienced more than n events total",
			PT_complex, "pre_load", PADDR(pre_load),PT_DESCRIPTION,"the load prior to being interrupted",
#endif

			PT_double, "monthly_bill[$]", PADDR(monthly_bill),PT_DESCRIPTION,"Accumulator for the current month's bill",
			PT_double, "previous_monthly_bill[$]", PADDR(previous_monthly_bill),PT_DESCRIPTION,"Total monthly bill for the previous month",
			PT_double, "previous_monthly_energy[kWh]", PADDR(previous_monthly_energy),PT_DESCRIPTION,"",
			PT_double, "monthly_fee[$]", PADDR(monthly_fee),PT_DESCRIPTION,"Total monthly energy for the previous month",
			PT_double, "monthly_energy[kWh]", PADDR(monthly_energy),PT_DESCRIPTION,"Accumulator for the current month's energy",
			PT_enumeration, "bill_mode", PADDR(bill_mode),PT_DESCRIPTION,"Designates the bill mode to be used",
				PT_KEYWORD,"NONE",(enumeration)BM_NONE,
				PT_KEYWORD,"UNIFORM",(enumeration)BM_UNIFORM,
				PT_KEYWORD,"TIERED",(enumeration)BM_TIERED,
				PT_KEYWORD,"HOURLY",(enumeration)BM_HOURLY,
				PT_KEYWORD,"TIERED_RTP",(enumeration)BM_TIERED_RTP,
				PT_KEYWORD,"TIERED_TOU",(enumeration)BM_TIERED_TOU,
			PT_object, "power_market", PADDR(power_market),PT_DESCRIPTION,"Designates the auction object where prices are read from for bill mode",
			PT_int32, "bill_day", PADDR(bill_day),PT_DESCRIPTION,"Day bill is to be processed (assumed to occur at midnight of that day)",
			PT_double, "price[$/kWh]", PADDR(price),PT_DESCRIPTION,"Standard uniform pricing",
			PT_double, "price_base[$/kWh]", PADDR(price_base), PT_DESCRIPTION, "Used only in TIERED_RTP mode to describe the price before the first tier",
			PT_double, "first_tier_price[$/kWh]", PADDR(tier_price[0]),PT_DESCRIPTION,"first tier price of energy between first and second tier energy",
			PT_double, "first_tier_energy[kWh]", PADDR(tier_energy[0]),PT_DESCRIPTION,"price of energy on tier above price or price base",
			PT_double, "second_tier_price[$/kWh]", PADDR(tier_price[1]),PT_DESCRIPTION,"first tier price of energy between second and third tier energy",
			PT_double, "second_tier_energy[kWh]", PADDR(tier_energy[1]),PT_DESCRIPTION,"price of energy on tier above first tier",
			PT_double, "third_tier_price[$/kWh]", PADDR(tier_price[2]),PT_DESCRIPTION,"first tier price of energy greater than third tier energy",
			PT_double, "third_tier_energy[kWh]", PADDR(tier_energy[2]),PT_DESCRIPTION,"price of energy on tier above second tier",

			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

			//Deltamode functions
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_triplex_meter)==nullptr)
				GL_THROW("Unable to publish triplex_meter deltamode function");
			if (gl_publish_function(oclass,	"pwr_object_swing_swapper", (FUNCTIONADDR)swap_node_swing_status)==nullptr)
				GL_THROW("Unable to publish triplex_meter swing-swapping function");
			if (gl_publish_function(oclass,	"pwr_current_injection_update_map", (FUNCTIONADDR)node_map_current_update_function)==nullptr)
				GL_THROW("Unable to publish triplex_meter current injection update mapping function");
			if (gl_publish_function(oclass,	"attach_vfd_to_pwr_object", (FUNCTIONADDR)attach_vfd_to_node)==nullptr)
				GL_THROW("Unable to publish triplex_meter VFD attachment function");
			if (gl_publish_function(oclass, "pwr_object_reset_disabled_status", (FUNCTIONADDR)node_reset_disabled_status) == nullptr)
				GL_THROW("Unable to publish triplex_meter island-status-reset function");
			if (gl_publish_function(oclass, "pwr_object_swing_status_check", (FUNCTIONADDR)node_swing_status) == nullptr)
				GL_THROW("Unable to publish triplex_meter swing-status check function");
			if (gl_publish_function(oclass, "pwr_object_shunt_update", (FUNCTIONADDR)node_update_shunt_values) == nullptr)
				GL_THROW("Unable to publish triplex_meter shunt update function");
			if (gl_publish_function(oclass, "pwr_object_kmldata", (FUNCTIONADDR)triplex_meter_kmldata) == nullptr)
				GL_THROW("Unable to publish triplex_meter kmldata function");
		}
}

int triplex_meter::isa(char *classname)
{
	return strcmp(classname,"triplex_meter")==0 || triplex_node::isa(classname);
}

// Create a distribution triplex_meter from the defaults template, return 1 on success
int triplex_meter::create()
{
	int result = triplex_node::create();
	measured_real_energy = measured_reactive_energy = 0;
	measured_real_power = measured_reactive_power = 0.0;
	measured_real_energy_delta = measured_reactive_energy_delta = 0;
    last_measured_real_energy = last_measured_reactive_energy = 0;
	last_measured_real_power = last_measured_reactive_power = 0.0;
    measured_energy_delta_timestep = -1;
    start_timestamp = 0;
    last_delta_timestamp = 0;
	measured_power = 0;
	measured_demand = 0;
	last_t = dt = next_time = 0;
	previous_energy_total = 0;

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
	tpmeter_power_consumption = gld::complex(0,0);

	tpmeter_interrupted = false;	//Assumes we start as "uninterrupted"
	tpmeter_interrupted_secondary = false;	//Assumes start with no momentary interruptions

	//zero the various interval measurements, just because
	measured_real_max_voltage_in_interval[0] =  measured_real_max_voltage_in_interval[1] = measured_real_max_voltage_in_interval[2] = 0.0;
	measured_imag_max_voltage_in_interval[0] = 	measured_imag_max_voltage_in_interval[1] = measured_imag_max_voltage_in_interval[2] = 0.0;
	measured_real_min_voltage_in_interval[0] =  measured_real_min_voltage_in_interval[1] = measured_real_min_voltage_in_interval[2] = 0.0;
	measured_imag_min_voltage_in_interval[0] = measured_imag_min_voltage_in_interval[1] = measured_imag_min_voltage_in_interval[2] = 0.0;
	measured_avg_voltage_mag_in_interval[0] =  measured_avg_voltage_mag_in_interval[1] = measured_avg_voltage_mag_in_interval[2] = 0.0;

	//power average items
	measured_real_max_power_in_interval = 0.0;
	measured_reactive_max_power_in_interval = 0.0;
	measured_real_min_power_in_interval = 0.0;
	measured_reactive_min_power_in_interval = 0.0;
	measured_real_avg_power_in_interval = 0.0;
	measured_reactive_avg_power_in_interval = 0.0;

	last_measured_max_real_power = 0.0;
	last_measured_min_real_power = 0.0;
	last_measured_max_reactive_power = 0.0;
	last_measured_min_reactive_power = 0.0;
	last_measured_avg_real_power = 0.0;
	last_measured_avg_reactive_power = 0.0;

	return result;
}

// Initialize a distribution triplex_meter, return 1 on success
int triplex_meter::init(OBJECT *parent)
{
#ifdef SUPPORT_OUTAGES
	sustained_count=0;	//reliability sustained event counter
	momentary_count=0;	//reliability momentary event counter
	total_count=0;		//reliability total event counter
	s_flag=0;
	t_flag=0;
	pre_load=0;
#endif

	OBJECT *obj = OBJECTHDR(this);

	if(power_market != 0){
		price_prop = gl_get_property(power_market, market_price_name);
		if(price_prop == 0){
			GL_THROW(const_cast<char*>(R"(triplex_meter::power_market object '%s' does not publish '%s')"), (power_market->name ? power_market->name : "(anon)"), (const char*)market_price_name);
		}
	}
	check_prices();

	//Check power and energy properties - if they are initialized, send a warning
	if ((measured_real_power != 0.0) || (measured_reactive_power != 0.0) || (measured_real_energy != 0.0) || (measured_reactive_energy != 0.0))
	{
		gl_warning("triplex_meter:%d - %s - measured power or energy is not initialized to zero - unexpected energy values may result",obj->id,(obj->name?obj->name:"Unnamed"));
		/*  TROUBLESHOOT
		An initial value for measured_real_power, measured_reactive_power, measured_real_energy, or measured_reactive_energy was set in the GLM.  This may cause some unexpected
		energy values to be computed for the system on this triplex_meter.  If this was not deliberate, be sure to remove those entries from the GLM file.
		*/
	}

	return triplex_node::init(parent);
}


int triplex_meter::check_prices(){
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
			GL_THROW("triplex_meter energy tiers quantity trend improperly");
		}
		for(int i = 0; i < 3; ++i){
			if(tier_price[i] < 0.0 || tier_energy[i] < 0.0)
				GL_THROW("triplex_meter tiers cannot have negative values");
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
			GL_THROW("triplex_meter energy tiers quantity trend improperly");
		}
		for(int i = 0; i < 3; ++i){
			if(tier_price[i] < 0.0 || tier_energy[i] < 0.0)
				GL_THROW("triplex_meter tiers cannot have negative values");
		}
	}

	if(bill_mode == BM_HOURLY || bill_mode == BM_TIERED_RTP){
		if(power_market == 0 || price_prop == 0){
			GL_THROW("triplex_meter cannot use real time energy prices without a power market that publishes the next price");
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
TIMESTAMP triplex_meter::presync(TIMESTAMP t0)
{
	if (tpmeter_power_consumption != gld::complex(0,0))
		power[0] = power[1] = 0.0;

	//Reliability addition - clear momentary flag if set
	if (tpmeter_interrupted_secondary)
		tpmeter_interrupted_secondary = false;

    // Capturing first timestamp of simulation for use in delta energy measurements.
    if (t0 != 0 && start_timestamp == 0)
        start_timestamp = t0;

	return triplex_node::presync(t0);
}
//Sync needed for reliability
TIMESTAMP triplex_meter::sync(TIMESTAMP t0)
{
	int TempNodeRef;
	OBJECT *obj = OBJECTHDR(this);

	//Check for straggler nodes - fix so segfaults don't occur
	if ((prev_NTime==0) && (solver_method == SM_NR))
	{
		//See if we've been initialized yet - links typically do this, but single buses get missed
		if (NR_node_reference == -1)
		{
			//Call the populate routine
			NR_populate();
		}
	}

	//Reliability check
	if ((fault_check_object != nullptr) && (solver_method == SM_NR))	//proper solver fault_check is present (so might need to set flag
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
					GL_THROW("triplex_meter:%d - %s - Uninitialized parent is causing odd issues - fix your model!",obj->id,(obj->name?obj->name:"Unnamed"));
					/*  TROUBLESHOOT
					A childed triplex_meter object is having issues mapping to its parent node - this typically happens in very odd cases of islanded/orphaned
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
			tpmeter_interrupted = true;	//Someone is out of service, they just may not know it

			//See if we were "momentary" as well - if so, clear us.
			if (tpmeter_interrupted_secondary)
				tpmeter_interrupted_secondary = false;
		}
		else
		{
			tpmeter_interrupted = false;	//All is well
		}
	}

	if (tpmeter_power_consumption != gld::complex(0,0))
	{
		power[0] += tpmeter_power_consumption/2;
		power[1] += tpmeter_power_consumption/2;
	}

	return triplex_node::sync(t0);

}

// Synchronize a distribution triplex_meter
TIMESTAMP triplex_meter::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP rv = TS_NEVER;
	TIMESTAMP hr = TS_NEVER;

	//Call node postsync now, otherwise current_inj isn't right
	rv = triplex_node::postsync(t1);

	//measured_voltage[0] = voltage[0];
	//measured_voltage[1] = voltage[1];
	//measured_voltage[2] = voltage[2];

	//Really no idea why this is done -- maybe to force a polar status?
	measured_voltage[0].SetPolar(voltage[0].Mag(),voltage[0].Arg());
	measured_voltage[1].SetPolar(voltage[1].Mag(),voltage[1].Arg());
	measured_voltage[2].SetPolar(voltage[2].Mag(),voltage[2].Arg());

	if (t1 > last_t)
	{
		dt = t1 - last_t;
		last_t = t1;
	}
	else
		dt = 0;

	//READLOCK_OBJECT(obj);
	measured_current[0] = current_inj[0];
	measured_current[1] = current_inj[1];
	//READUNLOCK_OBJECT(obj);
	measured_current[2] = -(measured_current[1]+measured_current[0]);

//		if (dt > 0 && last_t != dt)
	if (dt > 0)
	{
		measured_real_energy += measured_real_power * TO_HOURS(dt);
		measured_reactive_energy += measured_reactive_power * TO_HOURS(dt);
	}

	indiv_measured_power[0] = measured_voltage[0]*(~measured_current[0]);
	indiv_measured_power[1] = gld::complex(-1,0) * measured_voltage[1]*(~measured_current[1]);
	indiv_measured_power[2] = measured_voltage[2]*(~measured_current[2]);

	measured_power = indiv_measured_power[0] + indiv_measured_power[1] + indiv_measured_power[2];

	measured_real_power = (indiv_measured_power[0]).Re()
						+ (indiv_measured_power[1]).Re()
						+ (indiv_measured_power[2]).Re();

	measured_reactive_power = (indiv_measured_power[0]).Im()
							+ (indiv_measured_power[1]).Im()
							+ (indiv_measured_power[2]).Im();

	if (measured_real_power>measured_demand)
		measured_demand=measured_real_power;
	
	//Perform delta energy updates
	if(measured_energy_delta_timestep > 0) {
		// Delta energy cacluation
		if (t0 == start_timestamp) {
			last_delta_timestamp = start_timestamp;
		}

		if ((t1 == last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep)) && (t1 != t0))  {
			measured_real_energy_delta = measured_real_energy - last_measured_real_energy;
			measured_reactive_energy_delta = measured_reactive_energy - last_measured_reactive_energy;
			last_measured_real_energy = measured_real_energy;
			last_measured_reactive_energy = measured_reactive_energy;
			last_delta_timestamp = t1;
		}

		if (rv > last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep)) {
			rv = last_delta_timestamp + TIMESTAMP(measured_energy_delta_timestep);
		}
	}//End delta energy updates

	//Perform min/max/avg stat updates
	if(measured_min_max_avg_timestep > 0) {
		// Delta energy cacluation
		if (t0 == start_timestamp) {
			last_stat_timestamp = start_timestamp;

			//Voltage values
			measured_real_max_voltage_in_interval[0] = voltage1.Re();
			measured_real_max_voltage_in_interval[1] = voltage2.Re();
			measured_real_max_voltage_in_interval[2] = voltage12.Re();
			measured_imag_max_voltage_in_interval[0] = voltage1.Im();
			measured_imag_max_voltage_in_interval[1] = voltage2.Im();
			measured_imag_max_voltage_in_interval[2] = voltage12.Im();
			measured_real_min_voltage_in_interval[0] = voltage1.Re();
			measured_real_min_voltage_in_interval[1] = voltage2.Re();
			measured_real_min_voltage_in_interval[2] = voltage12.Re();
			measured_imag_min_voltage_in_interval[0] = voltage1.Im();
			measured_imag_min_voltage_in_interval[1] = voltage2.Im();
			measured_imag_min_voltage_in_interval[2] = voltage12.Im();
			measured_avg_voltage_mag_in_interval[0] = 0.0;
			measured_avg_voltage_mag_in_interval[1] = 0.0;
			measured_avg_voltage_mag_in_interval[2] = 0.0;

			//Power values
			measured_real_max_power_in_interval = measured_real_power;
			measured_real_min_power_in_interval = measured_real_power;
			measured_real_avg_power_in_interval = 0.0;

			measured_reactive_max_power_in_interval = measured_reactive_power;
			measured_reactive_min_power_in_interval = measured_reactive_power;
			measured_reactive_avg_power_in_interval = 0.0;

			interval_dt = 0;
			interval_count = 0;
		}

		if ((t1 > last_stat_timestamp) && (t1 < last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep)) && (t1 != t0)) {
			if (interval_count == 0) {
				last_measured_max_voltage[0] = last_measured_voltage[0];
				last_measured_max_voltage[1] = last_measured_voltage[1];
				last_measured_max_voltage[2] = last_measured_voltage[2];
				last_measured_min_voltage[0] = last_measured_voltage[0];
				last_measured_min_voltage[1] = last_measured_voltage[1];
				last_measured_min_voltage[2] = last_measured_voltage[2];
				last_measured_avg_voltage[0] = last_measured_voltage[0].Mag();
				last_measured_avg_voltage[1] = last_measured_voltage[1].Mag();
				last_measured_avg_voltage[2] = last_measured_voltage[2].Mag();

				//Power
				last_measured_min_real_power = last_measured_real_power;
				last_measured_max_real_power = last_measured_real_power;
				last_measured_avg_real_power = last_measured_real_power;
				last_measured_min_reactive_power = last_measured_reactive_power;
				last_measured_max_reactive_power = last_measured_reactive_power;
				last_measured_avg_reactive_power = last_measured_reactive_power;

			} else {
				if (last_measured_max_voltage[0].Mag() < last_measured_voltage[0].Mag()) {
					last_measured_max_voltage[0] = last_measured_voltage[0];
				}
				if (last_measured_max_voltage[1].Mag() < last_measured_voltage[1].Mag()) {
					last_measured_max_voltage[1] = last_measured_voltage[1];
				}
				if (last_measured_max_voltage[2].Mag() < last_measured_voltage[2].Mag()) {
					last_measured_max_voltage[2] = last_measured_voltage[2];
				}
				if (last_measured_min_voltage[0].Mag() > last_measured_voltage[0].Mag()) {
					last_measured_min_voltage[0] = last_measured_voltage[0];
				}
				if (last_measured_min_voltage[1].Mag() > last_measured_voltage[1].Mag()) {
					last_measured_min_voltage[1] = last_measured_voltage[1];
				}
				if (last_measured_min_voltage[2].Mag() > last_measured_voltage[2].Mag()) {
					last_measured_min_voltage[2] = last_measured_voltage[2];
				}

				//Power min/max check
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

				last_measured_avg_voltage[0] = ((interval_dt * last_measured_avg_voltage[0]) + (dt * last_measured_voltage[0].Mag()))/(dt + interval_dt);
				last_measured_avg_voltage[1] = ((interval_dt * last_measured_avg_voltage[1]) + (dt * last_measured_voltage[1].Mag()))/(dt + interval_dt);
				last_measured_avg_voltage[2] = ((interval_dt * last_measured_avg_voltage[2]) + (dt * last_measured_voltage[2].Mag()))/(dt + interval_dt);

				//Update the power averages
				last_measured_avg_real_power = ((interval_dt * last_measured_avg_real_power) + (dt * last_measured_real_power))/(dt + interval_dt);
				last_measured_avg_reactive_power = ((interval_dt * last_measured_avg_reactive_power) + (dt * last_measured_reactive_power))/(dt + interval_dt);
			}
			interval_count++;
			interval_dt = interval_dt + dt;
		}

		if ((t1 == last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep)) && (t1 != t0))  {
			last_stat_timestamp = t1;
			if (last_measured_max_voltage[0].Mag() < last_measured_voltage[0].Mag()) {
				last_measured_max_voltage[0] = last_measured_voltage[0];
			}
			if (last_measured_max_voltage[1].Mag() < last_measured_voltage[1].Mag()) {
				last_measured_max_voltage[1] = last_measured_voltage[1];
			}
			if (last_measured_max_voltage[2].Mag() < last_measured_voltage[2].Mag()) {
				last_measured_max_voltage[2] = last_measured_voltage[2];
			}
			if (last_measured_min_voltage[0].Mag() > last_measured_voltage[0].Mag()) {
				last_measured_min_voltage[0] = last_measured_voltage[0];
			}
			if (last_measured_min_voltage[1].Mag() > last_measured_voltage[1].Mag()) {
				last_measured_min_voltage[1] = last_measured_voltage[1];
			}
			if (last_measured_min_voltage[2].Mag() > last_measured_voltage[2].Mag()) {
				last_measured_min_voltage[2] = last_measured_voltage[2];
			}

			//Power min/max check
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

			last_measured_avg_voltage[0] = ((interval_dt * last_measured_avg_voltage[0]) + (dt * last_measured_voltage[0].Mag()))/(dt + interval_dt);
			last_measured_avg_voltage[1] = ((interval_dt * last_measured_avg_voltage[1]) + (dt * last_measured_voltage[1].Mag()))/(dt + interval_dt);
			last_measured_avg_voltage[2] = ((interval_dt * last_measured_avg_voltage[2]) + (dt * last_measured_voltage[2].Mag()))/(dt + interval_dt);

			//Update the power averages
			last_measured_avg_real_power = ((interval_dt * last_measured_avg_real_power) + (dt * last_measured_real_power))/(dt + interval_dt);
			last_measured_avg_reactive_power = ((interval_dt * last_measured_avg_reactive_power) + (dt * last_measured_reactive_power))/(dt + interval_dt);

			measured_real_max_voltage_in_interval[0] = last_measured_max_voltage[0].Re();
			measured_real_max_voltage_in_interval[1] = last_measured_max_voltage[1].Re();
			measured_real_max_voltage_in_interval[2] = last_measured_max_voltage[2].Re();
			measured_imag_max_voltage_in_interval[0] = last_measured_max_voltage[0].Im();
			measured_imag_max_voltage_in_interval[1] = last_measured_max_voltage[1].Im();
			measured_imag_max_voltage_in_interval[2] = last_measured_max_voltage[2].Im();
			measured_real_min_voltage_in_interval[0] = last_measured_min_voltage[0].Re();
			measured_real_min_voltage_in_interval[1] = last_measured_min_voltage[1].Re();
			measured_real_min_voltage_in_interval[2] = last_measured_min_voltage[2].Re();
			measured_imag_min_voltage_in_interval[0] = last_measured_min_voltage[0].Im();
			measured_imag_min_voltage_in_interval[1] = last_measured_min_voltage[1].Im();
			measured_imag_min_voltage_in_interval[2] = last_measured_min_voltage[2].Im();
			measured_avg_voltage_mag_in_interval[0] = last_measured_avg_voltage[0];
			measured_avg_voltage_mag_in_interval[1] = last_measured_avg_voltage[1];
			measured_avg_voltage_mag_in_interval[2] = last_measured_avg_voltage[2];

			//Power values
			measured_real_max_power_in_interval = last_measured_max_real_power;
			measured_real_min_power_in_interval = last_measured_min_real_power;
			measured_real_avg_power_in_interval = last_measured_avg_real_power;
			
			measured_reactive_max_power_in_interval = last_measured_max_reactive_power;
			measured_reactive_min_power_in_interval = last_measured_min_reactive_power;
			measured_reactive_avg_power_in_interval = last_measured_avg_reactive_power;

			interval_dt = 0;
			interval_count = 0;
		}

		last_measured_voltage[0] = voltage1;
		last_measured_voltage[1] = voltage2;
		last_measured_voltage[2] = voltage12;
		if (rv > last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep)) {
			rv = last_stat_timestamp + TIMESTAMP(measured_min_max_avg_timestep);
		}
	}//End min/max/avg updates

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

	if (bill_mode == BM_HOURLY || bill_mode == BM_TIERED_RTP || bill_mode == BM_TIERED_TOU) 
	{
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

	//Update the power trackers
	last_measured_real_power = measured_real_power;
	last_measured_reactive_power = measured_reactive_power;

	if (next_time != 0 && next_time < rv)
		return -next_time;
	else
		return rv;
}

double triplex_meter::process_bill(TIMESTAMP t1){
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

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE triplex_meter::inter_deltaupdate_triplex_meter(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	OBJECT *hdr = OBJECTHDR(this);
	int TempNodeRef;
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
		//Triplex meter presync items
			if (tpmeter_power_consumption != gld::complex(0,0))
				power[0] = power[1] = 0.0;

			//Reliability addition - clear momentary flag if set
			if (tpmeter_interrupted_secondary)
				tpmeter_interrupted_secondary = false;

		//Call node presync-equivalent items
		NR_node_presync_fxn(0);

		//Triplex-meter specific sync items
			//Reliability check
			if ((fault_check_object != nullptr) && (solver_method == SM_NR))	//proper solver fault_check is present (so might need to set flag
			{
				if (NR_node_reference==-99)	//Childed
				{
					TempNodeRef=*NR_subnode_reference;

					//No child check from above -- if we're broke this late in the game, we have major problems (should have been caught by now)
				}
				else	//Normal
				{
					//Just assign it to our normal index
					TempNodeRef=NR_node_reference;
				}

				if ((NR_busdata[TempNodeRef].origphases & NR_busdata[TempNodeRef].phases) != NR_busdata[TempNodeRef].origphases)	//We have a phase mismatch - something has been lost
				{
					tpmeter_interrupted = true;	//Someone is out of service, they just may not know it

					//See if we were "momentary" as well - if so, clear us.
					if (tpmeter_interrupted_secondary)
						tpmeter_interrupted_secondary = false;
				}
				else
				{
					tpmeter_interrupted = false;	//All is well
				}
			}

			if (tpmeter_power_consumption != gld::complex(0,0))
			{
				power[0] += tpmeter_power_consumption/2;
				power[1] += tpmeter_power_consumption/2;
			}

		//Call node sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}//End Before NR solver (or inclusive)
	else	//After the call
	{
		//Perform node postsync-like updates on the values - call first so current is right
		BOTH_node_postsync_fxn(hdr);

		//Triplex_meter-specific postsync
			measured_voltage[0].SetPolar(voltageA.Mag(),voltageA.Arg());
			measured_voltage[1].SetPolar(voltageB.Mag(),voltageB.Arg());
			measured_voltage[2].SetPolar(voltageC.Mag(),voltageC.Arg());

			measured_current[0] = current_inj[0];
			measured_current[1] = current_inj[1];
			measured_current[2] = -(measured_current[1]+measured_current[0]);

			indiv_measured_power[0] = measured_voltage[0]*(~measured_current[0]);
			indiv_measured_power[1] = gld::complex(-1,0) * measured_voltage[1]*(~measured_current[1]);
			indiv_measured_power[2] = measured_voltage[2]*(~measured_current[2]);

			measured_power = indiv_measured_power[0] + indiv_measured_power[1] + indiv_measured_power[2];

			measured_real_power = (indiv_measured_power[0]).Re()
								+ (indiv_measured_power[1]).Re()
								+ (indiv_measured_power[2]).Re();

			measured_reactive_power = (indiv_measured_power[0]).Im()
									+ (indiv_measured_power[1]).Im()
									+ (indiv_measured_power[2]).Im();

			if (measured_real_power>measured_demand)
				measured_demand=measured_real_power;

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

EXPORT int isa_triplex_meter(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,triplex_meter)->isa(classname);
}

EXPORT int create_triplex_meter(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(triplex_meter::oclass);
		if (*obj!=nullptr)
		{
			triplex_meter *my = OBJECTDATA(*obj,triplex_meter);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(triplex_meter);
}

EXPORT int init_triplex_meter(OBJECT *obj)
{
	try {
		triplex_meter *my = OBJECTDATA(obj,triplex_meter);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(triplex_meter);
}

EXPORT TIMESTAMP sync_triplex_meter(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		triplex_meter *pObj = OBJECTDATA(obj,triplex_meter);
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
	SYNC_CATCHALL(triplex_meter);
}

EXPORT int notify_triplex_meter(OBJECT *obj, int update_mode, PROPERTY *prop, char *value){
	triplex_meter *n = OBJECTDATA(obj, triplex_meter);
	int rv = 1;

	rv = n->notify(update_mode, prop, value);

	return rv;
}

//Deltamode export
EXPORT SIMULATIONMODE interupdate_triplex_meter(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	triplex_meter *my = OBJECTDATA(obj,triplex_meter);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_triplex_meter(delta_time,dt,iteration_count_val,interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_triplex_meter(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

//KML Export
EXPORT int triplex_meter_kmldata(OBJECT *obj,int (*stream)(const char*,...))
{
	triplex_meter *n = OBJECTDATA(obj, triplex_meter);
	int rv = 1;

	rv = n->kmldata(stream);

	return rv;
}

int triplex_meter::kmldata(int (*stream)(const char*,...))
{
	int phase[3] = {has_phase(PHASE_A),has_phase(PHASE_B),has_phase(PHASE_C)};

	// TODO: this voltage and power breakdown should go in triplex_node
	double basis = has_phase(PHASE_A) ? 0 : ( has_phase(PHASE_B) ? 240 : has_phase(PHASE_C) ? 120 : 0 );
	stream("<CAPTION>%s #%d</CAPTION>\n", get_oclass()->get_name(), get_id());
	stream("<TR>"
			"<TH WIDTH=\"25%\" ALIGN=CENTER>Property<HR></TH>\n"
			"<TH WIDTH=\"25%\" COLSPAN=2 ALIGN=CENTER><NOBR>Line 1</NOBR><HR></TH>\n"
			"<TH WIDTH=\"25%\" COLSPAN=2 ALIGN=CENTER><NOBR>Line 2</NOBR><HR></TH>\n"
			"<TH WIDTH=\"25%\" COLSPAN=2 ALIGN=CENTER><NOBR>Neutral</NOBR><HR></TH>\n"
			"</TR>\n");

	// voltages
	stream("<TR><TH ALIGN=LEFT>Voltage</TH>\n");
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>kV</TD>\n", measured_voltage[0].Mag()/1000);
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>kV</TD>\n", measured_voltage[1].Mag()/1000);
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>kV</TD>\n", measured_voltage[2].Mag()/1000);
	stream("</TR>\n");
	stream("<TR><TH ALIGN=LEFT>&nbsp</TH>\n");
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>&deg;</TD>\n", measured_voltage[0].Arg()*180/3.1416 - basis);
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>&deg;</TD>\n", measured_voltage[1].Arg()*180/3.1416 - basis);
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR></TD><TD ALIGN=LEFT>&deg;</TD>\n", measured_voltage[2].Arg()*180/3.1416);
	stream("</TR>\n");

	// power
	stream("<TR><TH ALIGN=LEFT>Power</TH>\n");
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR</TD><TD ALIGN=LEFT>kW</TD>\n", indiv_measured_power[0].Re()/1000);
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR</TD><TD ALIGN=LEFT>kW</TD>\n", indiv_measured_power[1].Re()/1000);
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR</TD><TD ALIGN=LEFT>kW</TD>\n", indiv_measured_power[2].Re()/1000);
	stream("</TR>\n");
	stream("<TR><TH ALIGN=LEFT>Power</TH>\n");
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR</TD><TD ALIGN=LEFT>kVAR</TD>\n", indiv_measured_power[0].Im()/1000);
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR</TD><TD ALIGN=LEFT>kVAR</TD>\n", indiv_measured_power[1].Im()/1000);
	stream("<TD ALIGN=RIGHT STYLE=\"font-family:courier;\"><NOBR>%.3f</NOBR</TD><TD ALIGN=LEFT>kVAR</TD>\n", indiv_measured_power[2].Im()/1000);
	stream("</TR>\n");

	// power
	stream("<TR><TH>&nbsp;</TH><TH ALIGN=CENTER COLSPAN=6>Total<HR/></TH></TR>");
	stream("<TR><TH ALIGN=LEFT>Power</TH>\n");
	stream("<TD ALIGN=CENTER COLSPAN=6 STYLE=\"font-family:courier;\"><NOBR>%.3f&nbsp;kW</NOBR</TD>\n", measured_real_power/1000);
	stream("</TR>\n");

	// energy
	stream("<TR><TH ALIGN=LEFT>Energy</TH>");
	stream("<TD ALIGN=CENTER COLSPAN=6 STYLE=\"font-family:courier;\"><NOBR>%.3f&nbsp;kWh</NOBR</TD>\n", measured_real_energy/1000);
	stream("</TR>\n");

	if ( voltage12.Mag()/2<0.5*nominal_voltage ) return 0; // black
	if ( voltage12.Mag()/2<0.95*nominal_voltage ) return 1; // blue
	if ( voltage12.Mag()/2>1.05*nominal_voltage ) return 3; // red
	return 2; // green
}

/**@}**/
