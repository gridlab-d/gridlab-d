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
		oclass = gl_register_class(mod,"triplex_meter",sizeof(triplex_meter),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
		if (oclass==NULL)
			throw "unable to register class triplex_meter";
		else
			oclass->trl = TRL_PROVEN;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "triplex_node",
			PT_double, "measured_real_energy[Wh]", PADDR(measured_real_energy),
			PT_double, "measured_reactive_energy[VAh]",PADDR(measured_reactive_energy),
			PT_complex, "measured_power[VA]", PADDR(measured_power),
			PT_complex, "indiv_measured_power_1[VA]", PADDR(indiv_measured_power[0]),
			PT_complex, "indiv_measured_power_2[VA]", PADDR(indiv_measured_power[1]),
			PT_complex, "indiv_measured_power_N[VA]", PADDR(indiv_measured_power[2]),
			PT_double, "measured_demand[W]", PADDR(measured_demand),
			PT_double, "measured_real_power[W]", PADDR(measured_real_power),
			PT_double, "measured_reactive_power[VAr]", PADDR(measured_reactive_power),
			PT_complex, "meter_power_consumption[VA]", PADDR(tpmeter_power_consumption),

			// added to record last voltage/current
			PT_complex, "measured_voltage_1[V]", PADDR(measured_voltage[0]),
			PT_complex, "measured_voltage_2[V]", PADDR(measured_voltage[1]),
			PT_complex, "measured_voltage_N[V]", PADDR(measured_voltage[2]),
			PT_complex, "measured_current_1[A]", PADDR(measured_current[0]),
			PT_complex, "measured_current_2[A]", PADDR(measured_current[1]),
			PT_complex, "measured_current_N[A]", PADDR(measured_current[2]),
			PT_bool, "customer_interrupted", PADDR(tpmeter_interrupted),
			PT_bool, "customer_interrupted_secondary", PADDR(tpmeter_interrupted_secondary),
#ifdef SUPPORT_OUTAGES
			PT_int16, "sustained_count", PADDR(sustained_count),	//reliability sustained event counter
			PT_int16, "momentary_count", PADDR(momentary_count),	//reliability momentary event counter
			PT_int16, "total_count", PADDR(total_count),		//reliability total event counter
			PT_int16, "s_flag", PADDR(s_flag),
			PT_int16, "t_flag", PADDR(t_flag),
			PT_complex, "pre_load", PADDR(pre_load),
#endif

			PT_double, "monthly_bill[$]", PADDR(monthly_bill),
			PT_double, "previous_monthly_bill[$]", PADDR(previous_monthly_bill),
			PT_double, "previous_monthly_energy[kWh]", PADDR(previous_monthly_energy),
			PT_double, "monthly_fee[$]", PADDR(monthly_fee),
			PT_double, "monthly_energy[kWh]", PADDR(monthly_energy),
			PT_enumeration, "bill_mode", PADDR(bill_mode),
				PT_KEYWORD,"NONE",BM_NONE,
				PT_KEYWORD,"UNIFORM",BM_UNIFORM,
				PT_KEYWORD,"TIERED",BM_TIERED,
				PT_KEYWORD,"HOURLY",BM_HOURLY,
				PT_KEYWORD,"TIERED_RTP",BM_TIERED_RTP,
			PT_object, "power_market", PADDR(power_market),
			PT_int32, "bill_day", PADDR(bill_day),
			PT_double, "price[$/kWh]", PADDR(price),
			PT_double, "price_base[$/kWh]", PADDR(price_base), PT_DESCRIPTION, "Used only in TIERED_RTP mode to describe the price before the first tier",
			PT_double, "first_tier_price[$/kWh]", PADDR(tier_price[0]),
			PT_double, "first_tier_energy[kWh]", PADDR(tier_energy[0]),
			PT_double, "second_tier_price[$/kWh]", PADDR(tier_price[1]),
			PT_double, "second_tier_energy[kWh]", PADDR(tier_energy[1]),
			PT_double, "third_tier_price[$/kWh]", PADDR(tier_price[2]),
			PT_double, "third_tier_energy[kWh]", PADDR(tier_energy[2]),

			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
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

	return triplex_node::presync(t0);
}
//Sync needed for reliability
TIMESTAMP triplex_meter::sync(TIMESTAMP t0)
{
	int TempNodeRef;

	//Reliability check
	if ((NR_mode == false) && (fault_check_object != NULL) && (solver_method == SM_NR))	//solver cycle and fault_check is present (so might need to set flag
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
	TIMESTAMP rv = TS_NEVER;
	TIMESTAMP hr = TS_NEVER;

	//measured_voltage[0] = voltageA;
	//measured_voltage[1] = voltageB;
	//measured_voltage[2] = voltageC;
	measured_voltage[0].SetPolar(voltageA.Mag(),voltageA.Arg());
	measured_voltage[1].SetPolar(voltageB.Mag(),voltageB.Arg());
	measured_voltage[2].SetPolar(voltageC.Mag(),voltageC.Arg());

	if ((solver_method == SM_NR && NR_cycle == true)||solver_method  == SM_FBS)
	{
		//Reliability addition - clear momentary flag if set
		if (tpmeter_interrupted_secondary == true)
			tpmeter_interrupted_secondary = false;

		if (t1 > last_t)
		{
			dt = t1 - last_t;
			last_t = t1;
		}
		else
			dt = 0;

		measured_current[0] = current_inj[0];
		measured_current[1] = current_inj[1];
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
				hourly_acc += seconds/3600 * price * measured_real_power/1000;
				process_bill(t1);
			}

			// Now that we've accumulated the bill for the last time period, update to the new price
			double *pprice = (gl_get_double(power_market, price_prop));
			last_price = price = *pprice;

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
	rv = triplex_node::postsync(t1);

	if (next_time != 0 && next_time < rv)
		return -next_time;
	else
		return rv;
	//return triplex_node::postsync(t1);

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

/**@}**/
