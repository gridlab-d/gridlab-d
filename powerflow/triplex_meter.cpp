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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "triplex_meter.h"
#include "timestamp.h"

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
CLASS* triplex_meter::oclass = NULL;
CLASS* triplex_meter::pclass = NULL;

// the constructor registers the class and properties and sets the defaults
triplex_meter::triplex_meter(MODULE *mod) : triplex_node(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// link to parent class (used by isa)
		pclass = triplex_node::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"triplex_meter",sizeof(triplex_meter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class triplex_meter";
		else
			oclass->trl = TRL_PROVEN;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "triplex_node",
			PT_double, "measured_real_energy[Wh]", PADDR(measured_real_energy),PT_DESCRIPTION,"metered real energy consumption",
			PT_double, "measured_reactive_energy[VAh]",PADDR(measured_reactive_energy),PT_DESCRIPTION,"metered reactive energy consumption",
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
			if (gl_publish_function(oclass,	"delta_linkage_node", (FUNCTIONADDR)delta_linkage)==NULL)
				GL_THROW("Unable to publish triplex_meter delta_linkage function");
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_triplex_meter)==NULL)
				GL_THROW("Unable to publish triplex_meter deltamode function");
			if (gl_publish_function(oclass,	"delta_freq_pwr_object", (FUNCTIONADDR)delta_frequency_node)==NULL)
				GL_THROW("Unable to publish triplex_meter deltamode function");
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
	tpmeter_power_consumption = complex(0,0);

	tpmeter_interrupted = false;	//Assumes we start as "uninterrupted"
	tpmeter_interrupted_secondary = false;	//Assumes start with no momentary interruptions


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

	if(power_market != 0){
		price_prop = gl_get_property(power_market, "current_market.clearing_price");
		if(price_prop == 0){
			GL_THROW("triplex_meter::power_market object \'%s\' does not publish \'current_market.clearing_price\'", (power_market->name ? power_market->name : "(anon)"));
		}
	}
	check_prices();

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
	if (tpmeter_power_consumption != complex(0,0))
		power[0] = power[1] = 0.0;

	//Reliability addition - clear momentary flag if set
	if (tpmeter_interrupted_secondary == true)
		tpmeter_interrupted_secondary = false;

	return triplex_node::presync(t0);
}
//Sync needed for reliability
TIMESTAMP triplex_meter::sync(TIMESTAMP t0)
{
	int TempNodeRef;

	//Reliability check
	if ((fault_check_object != NULL) && (solver_method == SM_NR))	//proper solver fault_check is present (so might need to set flag
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
			tpmeter_interrupted = true;	//Someone is out of service, they just may not know it

			//See if we were "momentary" as well - if so, clear us.
			if (tpmeter_interrupted_secondary == true)
				tpmeter_interrupted_secondary = false;
		}
		else
		{
			tpmeter_interrupted = false;	//All is well
		}
	}

	if (tpmeter_power_consumption != complex(0,0))
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

	//measured_voltage[0] = voltageA;
	//measured_voltage[1] = voltageB;
	//measured_voltage[2] = voltageC;
	measured_voltage[0].SetPolar(voltageA.Mag(),voltageA.Arg());
	measured_voltage[1].SetPolar(voltageB.Mag(),voltageB.Arg());
	measured_voltage[2].SetPolar(voltageC.Mag(),voltageC.Arg());

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
	indiv_measured_power[1] = complex(-1,0) * measured_voltage[1]*(~measured_current[1]);
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

	if (next_time != 0 && next_time < rv)
		return -next_time;
	else
		return rv;
}

double triplex_meter::process_bill(TIMESTAMP t1){
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

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
SIMULATIONMODE triplex_meter::inter_deltaupdate_triplex_meter(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val,bool interupdate_pos)
{
	//unsigned char pass_mod;
	OBJECT *hdr = OBJECTHDR(this);
	int TempNodeRef;

	if (interupdate_pos == false)	//Before powerflow call
	{
		//Triplex meter presync items
			if (tpmeter_power_consumption != complex(0,0))
				power[0] = power[1] = 0.0;

			//Reliability addition - clear momentary flag if set
			if (tpmeter_interrupted_secondary == true)
				tpmeter_interrupted_secondary = false;

		//Call triplex-specific call
		BOTH_triplex_node_presync_fxn();

		//Call node presync-equivalent items
		NR_node_presync_fxn();

		//Triplex-meter specific sync items
			//Reliability check
			if ((fault_check_object != NULL) && (solver_method == SM_NR))	//proper solver fault_check is present (so might need to set flag
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
					tpmeter_interrupted = true;	//Someone is out of service, they just may not know it

					//See if we were "momentary" as well - if so, clear us.
					if (tpmeter_interrupted_secondary == true)
						tpmeter_interrupted_secondary = false;
				}
				else
				{
					tpmeter_interrupted = false;	//All is well
				}
			}

			if (tpmeter_power_consumption != complex(0,0))
			{
				power[0] += tpmeter_power_consumption/2;
				power[1] += tpmeter_power_consumption/2;
			}

		//Call sync-equivalent of triplex portion first
		BOTH_triplex_node_sync_fxn();

		//Call node sync-equivalent items (solver occurs at end of sync)
		NR_node_sync_fxn(hdr);

		return SM_DELTA;	//Just return something other than SM_ERROR for this call
	}
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
			indiv_measured_power[1] = complex(-1,0) * measured_voltage[1]*(~measured_current[1]);
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

EXPORT int isa_triplex_meter(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,triplex_meter)->isa(classname);
}

EXPORT int create_triplex_meter(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(triplex_meter::oclass);
		if (*obj!=NULL)
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
/**@}**/
