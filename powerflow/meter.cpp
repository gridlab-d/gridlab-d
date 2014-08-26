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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "meter.h"
#include "timestamp.h"

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
CLASS* meter::oclass = NULL;
CLASS* meter::pclass = NULL;

// the constructor registers the class and properties and sets the defaults
meter::meter(MODULE *mod) : node(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// link to parent class (used by isa)
		pclass = node::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"meter",sizeof(meter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class meter";
		else
			oclass->trl = TRL_PROVEN;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "node",
			PT_double, "measured_real_energy[Wh]", PADDR(measured_real_energy),PT_DESCRIPTION,"metered real energy consumption, cummalitive",
			PT_double, "measured_reactive_energy[VAh]",PADDR(measured_reactive_energy),PT_DESCRIPTION,"metered reactive energy consumption, cummalitive",
			PT_complex, "measured_power[VA]", PADDR(measured_power),PT_DESCRIPTION,"metered real power",
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
		if (gl_publish_function(oclass,"reset",(FUNCTIONADDR)meter_reset)==NULL)
			GL_THROW("unable to publish meter_reset function in %s",__FILE__);

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"delta_linkage_node", (FUNCTIONADDR)delta_linkage)==NULL)
			GL_THROW("Unable to publish meter delta_linkage function");
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_meter)==NULL)
			GL_THROW("Unable to publish meter deltamode function");
		if (gl_publish_function(oclass,	"delta_freq_pwr_object", (FUNCTIONADDR)delta_frequency_node)==NULL)
			GL_THROW("Unable to publish meter deltamode function");
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

	measured_voltage[0] = measured_voltage[1] = measured_voltage[2] = complex(0,0,A);
	measured_voltageD[0] = measured_voltageD[1] = measured_voltageD[2] = complex(0,0,A);
	measured_current[0] = measured_current[1] = measured_current[2] = complex(0,0,J);
	measured_real_energy = measured_reactive_energy = 0.0;
	measured_power = complex(0,0,J);
	measured_demand = 0.0;
	measured_real_power = 0.0;
	measured_reactive_power = 0.0;

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
	meter_power_consumption = complex(0,0);

	//Flag us as a meter
	node_type = METER_NODE;

	meter_NR_servered = false;	//Assume we are just a normal meter at first

	return result;
}

// Initialize a distribution meter, return 1 on success
int meter::init(OBJECT *parent)
{
	char temp_buff[128];

	if(power_market != 0){
		price_prop = gl_get_property(power_market, "current_market.clearing_price");
		if(price_prop == 0){
			GL_THROW("meter::power_market object \'%s\' does not publish \'current_market.clearing_price\'", (power_market->name ? power_market->name : "(anon)"));
		}
	}

	// Count the number of phases...for use with meter_power_consumption
	if (meter_power_consumption != complex(0,0))
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
			prev_voltage_value = (complex *)gl_malloc(3*sizeof(complex));

			//Check it
			if (prev_voltage_value==NULL)
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
			prev_voltage_value[0] = complex(0.0,0.0);
			prev_voltage_value[1] = complex(0.0,0.0);
			prev_voltage_value[2] = complex(0.0,0.0);
		}
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
	if (meter_power_consumption != complex(0,0))
		power[0] = power[1] = power[2] = 0.0;

	//Reliability addition - if momentary flag set - clear it
	if (meter_interrupted_secondary == true)
		meter_interrupted_secondary = false;

	return node::presync(t0);
}

//Functionalized portion for deltamode compatibility
void meter::BOTH_meter_sync_fxn()
{
	int TempNodeRef;

	//Reliability check
	if ((fault_check_object != NULL) && (solver_method == SM_NR))	//proper solver and fault_object isn't null - may need to set a flag
	{
		if (NR_node_reference==-99)	//Childed
		{
			TempNodeRef=*NR_subnode_reference;
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
			if (meter_interrupted_secondary == true)
				meter_interrupted_secondary = false;
		}
		else
		{
			meter_interrupted = false;	//All is well
		}
	}

	if (meter_power_consumption != complex(0,0))
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
	complex temp_current;
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
		// - everything below this can moved to commit function once tape player is collecting from commit function7
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

		if (measured_real_power > measured_demand) 
			measured_demand = measured_real_power;

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

		if( (bill_mode == BM_HOURLY || bill_mode == BM_TIERED_RTP) && power_market != NULL && price_prop != NULL){
			double seconds;
			if (dt != last_t)
				seconds = (double)(dt);
			else
				seconds = 0;
			
			if (seconds > 0)
			{
				hourly_acc += seconds/3600 * price * last_measured_real_power/1000;
				process_bill(t1);
			}

			// Now that we've accumulated the bill for the last time period, update to the new price
			double *pprice = (gl_get_double(power_market, price_prop));
			last_price = price = *pprice;
			last_measured_real_power = measured_real_power;

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

	monthly_energy = measured_real_energy/1000 - previous_energy_total;
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
	//unsigned char pass_mod;
	OBJECT *hdr = OBJECTHDR(this);

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Meter-specific presync items
		if (meter_power_consumption != complex(0,0))
			power[0] = power[1] = power[2] = 0.0;

		//Reliability addition - if momentary flag set - clear it
		if (meter_interrupted_secondary == true)
			meter_interrupted_secondary = false;

		//Call presync-equivalent items
		NR_node_presync_fxn();

		//Meter sync objects
		BOTH_meter_sync_fxn();

		//Call sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
	else	//After the call
	{
		//Perform postsync-like updates on the values
		BOTH_node_postsync_fxn(hdr);

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

		//No control required at this time - powerflow defers to the whims of other modules
		//Code below implements predictor/corrector-type logic, even though it effectively does nothing
		return SM_EVENT;

		////Do deltamode-related logic
		//if (bustype==SWING)	//We're the SWING bus, control our destiny (which is really controlled elsewhere)
		//{
		//	//See what we're on
		//	pass_mod = iteration_count_val - ((iteration_count_val >> 1) << 1);

		//	//Check pass
		//	if (pass_mod==0)	//Predictor pass
		//	{
		//		return SM_DELTA_ITER;	//Reiterate - to get us to corrector pass
		//	}
		//	else	//Corrector pass
		//	{
		//		//As of right now, we're always ready to leave
		//		//Other objects will dictate if we stay (powerflow is indifferent)
		//		return SM_EVENT;
		//	}//End corrector pass
		//}//End SWING bus handling
		//else	//Normal bus
		//{
		//	return SM_EVENT;	//Normal nodes want event mode all the time here - SWING bus will
		//						//control the reiteration process for pred/corr steps
		//}
	}
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
		if (*obj!=NULL)
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

/**@}**/
