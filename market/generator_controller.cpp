/** $Id: generator_controller.h 2011-02-03 18:00:00Z d3x593 $
	Copyright (C) 2011 Battelle Memorial Institute
	@file generator_controller.cpp
	@addtogroup generator_controller
	@ingroup market

 **/

#include "generator_controller.h"

//////////////////////////////////////////////////////////////////////////
// generator_controller CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* generator_controller::oclass = NULL;

generator_controller::generator_controller(MODULE *module){
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"generator_controller",sizeof(generator_controller),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
			PT_enumeration,"generator_state", PADDR(gen_state), PT_DESCRIPTION, "Current generator state",
				PT_KEYWORD, "OFF", GEN_OFF,
				PT_KEYWORD, "RUNNING", GEN_ACTIVE,
				PT_KEYWORD, "STARTING", GEN_STARTUP,
			PT_int32,"generator_state_number", PADDR(gen_state_num), PT_DESCRIPTION, "Current generator state as numeric value",
			PT_object,"market", PADDR(market_object), PT_DESCRIPTION, "Market the object will watch and bid into",
			PT_char1024,"bid_curve", PADDR(bidding_curve_txt), PT_DESCRIPTION, "Bidding curve text format",
			PT_bool,"update_curve", PADDR(update_curve), PT_DESCRIPTION, "Flag to force updating of bidding curve parse",
			PT_double,"generator_rating[VA]", PADDR(gen_rating), PT_DESCRIPTION, "Size of generator in VA that is bidding",
			PT_double,"generator_output[W]", PADDR(current_power_output), PT_DESCRIPTION, "Current real power output of generator",
			PT_double,"startup_cost[$]", PADDR(startup_cost), PT_DESCRIPTION, "Cost to start the generator from OFF status",
			PT_double,"shutdown_cost[$]", PADDR(shutdown_cost), PT_DESCRIPTION, "Cost to shut down the generator prematurely",
			PT_double,"minimum_runtime[s]", PADDR(minimum_runtime_dbl), PT_DESCRIPTION, "Minimum time the generator should run to avoid shutdown cost",
			PT_double,"minimum_downtime[s]", PADDR(minimum_downtime_dbl), PT_DESCRIPTION, "Minimum down time for the generator before it can bid again",
			PT_double,"capacity_factor", PADDR(capacity_factor), PT_DESCRIPTION, "Calculation of generator's current capacity factor based on the market period",
			PT_double,"amortization_factor[1/s]",PADDR(amortization_value), PT_DESCRIPTION, "Exponential decay factor in 1/s for shutdown cost repayment",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

int generator_controller::isa(char *classname)
{
	return strcmp(classname,"generator_controller")==0;
}

int generator_controller::create(void)
{
	market_object = NULL;
	market_period = 0;
	market_latency = 0;

	gen_rating = 0;

	bid_curve_values = NULL;
	bid_curve_current.bid_curve_parsed = 0;
	bid_curve_current.Curve_Info = NULL;
	bid_curve_current.number_bid_curve_sections = -1;
	bid_curve_current.valid_bid_period = false;
	bid_curve_current.expected_state = GEN_ACTIVE;

	number_bid_curve_sections = 0;
	number_latency_sections = 0;
	latency_write_section = 0;
	latency_read_section = 0;

	update_curve = false;

	last_market_id = 0;
	curr_market_id = NULL;

	power_link = NULL;
	phase_information = 0x00;
	num_phases = 0;

	next_clear = TS_NEVER;

	current_power_output = 0.0;
	last_power_output = 0.0;
	update_output = false;

	gen_state = GEN_ACTIVE;			//Assumes start running
	gen_state_num = 1;				//Replicates start running
	startup_cost = 0;				//assumes no cost to start up
	shutdown_cost = 0;				//assumes no cost to shut down
	shutdown_cost_remaining = 0.0;	//No shutdown cost remaining

	minimum_runtime_dbl = 1;		//1 second interval
	minimum_downtime_dbl = 0;		//No minimum downtime

	gen_started = 0;				//Generator started a LOOOONG time ago
	minimum_downtime = 0;
	market_bidding_time = 0;
	min_num_run_periods = 1;
	num_runs_completed = 1;

	capacity_factor = 0.0;
	amortization_value = 6.0;	//1/2 in 5 minutes - gets translated by market intervals later
	amortization_scaled = 1.0;		//Initial value
	shutdown_current_cost = 0.0;

	return 1;
}

int generator_controller::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	PROPERTY *ptemp;
	set *temp_set;
	int index;

	//Make sure we have a proper parent
	if (parent == NULL)	//No parent >:|
	{
		GL_THROW("generator_controller:%s has no parent!",obj->name);
		/*  TROUBLESHOOT
		The genererator_control object must be parented to a load or triplex_node in powerflow
		at this time.  It had no parent.  Please specify a parent and try again.
		*/
	}
	if (gl_object_isa(parent,"load","powerflow"))	//Parent is a normal three-phase load
	{
		//Grab the link to the constant_power properties
		ptemp = gl_get_property(parent,"constant_power_A");

		//Check
		if ((ptemp == NULL) || (ptemp->ptype!=PT_complex))
		{
			GL_THROW("generator_controller:%s failed to map the power property of the parent device",obj->name);
			/*  TROUBLESHOOT
			The generator_controller object failed to find the proper variables for mapping the power values.
			*/
		}

		//Get the address
		power_link = (complex *)GETADDR(parent,ptemp);

		//Check this as well
		if (power_link == NULL)
		{
			GL_THROW("generator_controller:%s failed to map the power property of the parent device",obj->name);
			//Defined above
		}

		//Grab the phase information
		ptemp = gl_get_property(parent,"phases");

		//Check
		if ((ptemp == NULL) || (ptemp->ptype!=PT_set))
		{
			GL_THROW("generator_controller:%s failed to map the power property of the parent device",obj->name);
			//Defined above - technically different, but it is part of the same idea, so meh
		}

		//Get the address
		temp_set = (set *)GETADDR(parent,ptemp);

		//Check this as well
		if (temp_set == NULL)
		{
			GL_THROW("generator_controller:%s failed to map the power property of the parent device",obj->name);
			//Defined above - again not really, but meh
		}

		//Now mask it in
		if ((*temp_set & 0x0001) == 0x0001)	//PHASE_A
		{
			phase_information |= 0x04;	//Flag as present - uses NR convention
			num_phases++;				//Increment counter
		}

		if ((*temp_set & 0x0002) == 0x0002)	//PHASE_B
		{
			phase_information |= 0x02;	//Flag as present - uses NR convention
			num_phases++;				//Increment counter
		}

		if ((*temp_set & 0x0004) == 0x0004)	//PHASE_C
		{
			phase_information |= 0x01;	//Flag as present - uses NR convention
			num_phases++;				//Increment counter
		}
	}
	else if (gl_object_isa(parent,"triplex_node","powerflow"))	//Parent is a triplex node
	{
		//Grab the link to the constant_power properties
		ptemp = gl_get_property(parent,"power_1");

		//Check
		if ((ptemp == NULL) || (ptemp->ptype!=PT_complex))
		{
			GL_THROW("generator_controller:%s failed to map the power property of the parent device",obj->name);
			/*  TROUBLESHOOT
			The generator_controller object failed to find the proper variables for mapping the power values.
			*/
		}

		//Get the address
		power_link = (complex *)GETADDR(parent,ptemp);

		//Check this as well
		if (power_link == NULL)
		{
			GL_THROW("generator_controller:%s failed to map the power property of the parent device",obj->name);
			//Defined above
		}

		//Set phase - triplex - so just make assumptions
		phase_information = 0x80;	//SP flag in NR
		num_phases = 2;				//Two phases exist for power

	}
	else	//Invalid parent
	{
		GL_THROW("generator_controller:%s has an invalid parent!",obj->name);
		/*  TROUBLESHOOT
		The genererator_control object must be parented to a load or triplex_node in powerflow
		at this time.  Please specify a valid parent and try again.
		*/
	}

	//Make sure the market object has been found
	if (market_object == NULL)
	{
		GL_THROW("Market object not found for generator_controller:%s",obj->name);
		/*  TROUBLESHOOT
		A generator_controller appears to be missing a market tie.  Please specify a valid
		market object and try again.  If the error persists, please submit your code and a bug
		report via the trac website.
		*/
	}

	//Get this object
	auction_object = OBJECTDATA(market_object,auction);

	//Pull the market information
	market_period = (TIMESTAMP)(auction_object->dPeriod);
	market_latency = (TIMESTAMP)(auction_object->dLatency);
	price_cap_value = auction_object->pricecap+0.01;	//Set to 0.01 higher - used as an initialization variable
	curr_market_id = &(auction_object->market_id);

	//Determine number of latency slots we need
	number_latency_sections = (int)((market_latency/market_period)+2);	//Delay # + current bid + current active

	//Make sure the rating is non-negative and non-zero
	if (gen_rating <= 0)
	{
		GL_THROW("Generator rating in generator_controller:%s must be positive!",obj->name);
		/*  TROUBLESHOOT
		The generator size specified for the generator_controller must be a positive, non-zero value.  Please try again.
		*/
	}

	//Do the initial latency mallocing
	bid_curve_values = (CURVEENTRY*)gl_malloc(number_latency_sections * sizeof(CURVEENTRY));

	//Make sure it worked
	if (bid_curve_values == NULL)
	{
		GL_THROW("Failed to allocate bid_curve memory in %s",obj->name);
		/*  TROUBLESHOOT
		While attempting to allocate memory for the generator_controller bid curve, an error
		was encountered.  Please try again.  If the error persists, please submit your code and a 
		bug report via the trac website.
		*/
	}

	//Initialize the sections
	for (index=0; index<number_latency_sections; index++)
	{
		bid_curve_values[index].bid_curve_parsed = -1;
		bid_curve_values[index].Curve_Info = NULL;
		bid_curve_values[index].number_bid_curve_sections = 0;
		bid_curve_values[index].valid_bid_period = false;
		bid_curve_values[index].expected_state = gen_state;	//Initial bidding needs to know how we are running - assume it populates all the way back
	}

	//Set initial pointer values appropriately (technically messes up the indexing, but it gets fixed after 1 latency cycle)
	latency_write_section = (number_latency_sections-1);
	latency_read_section = 0;

	//Allocate the first "old" value, just for the sake of doing so (so it doesn't fail on first run)
	bid_curve_values[latency_write_section].Curve_Info = (CURVEDETAILS*)gl_malloc(sizeof(CURVEDETAILS));

	//Make sure it worked
	if (bid_curve_values[latency_write_section].Curve_Info == NULL)
	{
		GL_THROW("generator_controller:%s failed to allocate the initial bid curve memory",obj->name);
		/*  TROUBLESHOOT
		While attempting to preallocate the memory for the temporary bid curve, an error was encountered.
		Please try again.  If the error persists, please submit your code and a bug report via the trac website.
		*/
	}

	//Set initial values - junk just so access violations don't occur
	bid_curve_values[latency_write_section].Curve_Info[0].lastbid_id = -1;
	bid_curve_values[latency_write_section].Curve_Info[0].power_delta = 0.0;
	bid_curve_values[latency_write_section].Curve_Info[0].power_start = 0.0;
	bid_curve_values[latency_write_section].Curve_Info[0].power_stop = 0.0;
	bid_curve_values[latency_write_section].Curve_Info[0].price = 0.0;

	//Parse the bid curve
	parse_bid_curve(obj,0);

	//Ensure the shutdown interval isn't negative
	if (minimum_runtime_dbl <= 0.0)
	{
		GL_THROW("generator_controller:%s must have a positive, non-zero minimum_runtime!",obj->name);
		/*  TROUBLESHOOT
		To properly bid into the market, generator_controller objects must have a minimum_runtime that is 
		greater than zero.  If no shutdown effects are desired, set the shutdown_cost to 0.  minimum_downtime
		must always be populated correctly.
		*/
	}

	//Make sure the minimum runtime is set as well - it can be zero
	if (minimum_downtime_dbl < 0.0)
	{
		GL_THROW("generator_controller:%s must have a non-negative minimum_downtime!",obj->name);
		/*  TROUBLESHOOT
		To properly bid into the market, generator_controller objects must have a minimum_downtime that is 
		positive or zero.
		*/
	}

	//Set initial state properly
	gen_state_num = (int)gen_state;

	return 1;
}

TIMESTAMP generator_controller::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	int index;
	int64 bidID_val;
	double per_phase_power, per_phase_old_power, shutdown_cost_temp;
	GENERATOR_STATE prev_bid_state;

	//If market changes, update
	if (last_market_id != *curr_market_id)
	{
		//Populate the previous state - check against 0 bid
		if (auction_object->cleared_frame.clearing_price >= bid_curve_values[latency_write_section].Curve_Info[0].price)	//Price met
		{
			//See if it was a startup bid - if so, flag it as running now
			if (bid_curve_values[latency_write_section].expected_state == GEN_STARTUP)
			{
				//Change over to running - otherwise subsequent bids might get a little squirrely
				bid_curve_values[latency_write_section].expected_state = GEN_ACTIVE;

				//Update our amortization value - only read this each time teh generator kicks on
				amortization_scaled = exp(-1.0* amortization_value * (double)(market_period));
			}

			//Increment startup run tracker
			num_runs_completed++;

		}//End last price met
		else if (bid_curve_values[latency_write_section].expected_state == GEN_ACTIVE)	//Was on
		{
			//Flag it as an off state
			bid_curve_values[latency_write_section].expected_state = GEN_OFF;

			//Update the downtime counter - base off last "bidding time" value
			minimum_downtime = market_bidding_time + (TIMESTAMP)(minimum_downtime_dbl);
		}
		else	//Must not have been met, but was already off
		{
			//Flag it as an off state
			bid_curve_values[latency_write_section].expected_state = GEN_OFF;
		}

		//Copy the previous bidding state
		prev_bid_state = bid_curve_values[latency_write_section].expected_state;

		//Move the pointers
		latency_write_section++;
		latency_read_section++;

		//Make sure it hasn't overrun anything
		if (latency_write_section >= number_latency_sections)
			latency_write_section = 0;

		if (latency_read_section >= number_latency_sections)
			latency_read_section = 0;

		//See if the bid curve needs updating
		if (update_curve == true)
			parse_bid_curve(obj,t0);	//It does, do so first

		//Copy in the new bid curve - ALWAYS do this - since startup and such costs may factor in
		copy_bid_curve(obj);

		//Determine when the market we are bidding into would be active
		if (t0 != 0)
			market_bidding_time = t0 + market_latency;
		else
			market_bidding_time = t1 + market_latency;	//Catch for first run

		//Handle bidding curve logic appropriately
		if (prev_bid_state == GEN_OFF)	//Generator was off or just went through a startup attempt - needs a start up value
		{
			//Figure out how many market periods we need to run for - regardless of if we need to or not
			min_num_run_periods = (int)((TIMESTAMP)(minimum_runtime_dbl) / market_period);

			//If it is zero - set costs appropriately
			if (min_num_run_periods < 1)
			{
				shutdown_cost_remaining = 0.0;	//No costs for mininum run time - if we clear, we met it
				shutdown_current_cost = 0.0;
			}
			else	//Normal operation
			{
				//Ensure the shutdown cost remaining is set to the full cost for these bids
				shutdown_cost_remaining = shutdown_cost;

				//Update the tracking variable
				shutdown_current_cost = shutdown_cost;
			}

			//See if we've been off long enough - correct for the minimum shutdown time
			if (market_bidding_time >= minimum_downtime)
			{
				//Flag as a valid bid
				bid_curve_values[latency_write_section].valid_bid_period = true;

				//Apply appropriate cost values (shutdown and startup)
				for (index=0; index<bid_curve_values[latency_write_section].number_bid_curve_sections; index++)
				{
					if (min_num_run_periods == 0)
					{
						//Apply offset to all portions of the bid - FC + SU - special case, no shutdown for no minimum run time
						bid_curve_values[latency_write_section].Curve_Info[index].price += startup_cost;
					}
					else
					{
						//Apply offset to all portions of the bid - FC + SU + SD
						bid_curve_values[latency_write_section].Curve_Info[index].price += startup_cost + shutdown_cost;
					}
				}

				//Flag us as a startup time
				bid_curve_values[latency_write_section].expected_state = GEN_STARTUP;

				//Reset number of intervals since started
				num_runs_completed = 1;
			}
			else	//Still needs to be off
			{
				//Flag as invalid interval
				bid_curve_values[latency_write_section].valid_bid_period = false;

				//Set all bids to the price cap value, +4 (just cause) - won't bid in, but this will help ensure nothing wierd happens
				for (index=0; index<bid_curve_values[latency_write_section].number_bid_curve_sections; index++)
				{
					//Apply the startup costs to the initial bin
					bid_curve_values[latency_write_section].Curve_Info[index].price += price_cap_value + 4;
				}

				//Set the state, just in case
				bid_curve_values[latency_write_section].expected_state = GEN_OFF;

			}//End minimum downtime not reached
		}//end GEN_OFF
		else if (prev_bid_state == GEN_ACTIVE)	//Running, bid appropriately
		{
			//Compute the shutdown cost influence, if still needed
			if (num_runs_completed < min_num_run_periods)
			{
				//Compute the current amortization value 
				shutdown_current_cost *= amortization_scaled;

				//Make sure we won't go negative on shutdown cost - if so, just bid the difference
				if (shutdown_current_cost > shutdown_cost_remaining)
				{
					//Just bid in the remainder - basically like below
					shutdown_cost_temp = shutdown_cost_remaining;	//Last bit goes in on this bid
					shutdown_cost_remaining = 0.0;					//No more penalty remains to offset
				}
				else	//Normal process, so bid like normal
				{
					shutdown_cost_temp = shutdown_current_cost;			//Apply scaling
					shutdown_cost_remaining -= shutdown_current_cost;	//Decrement appropriately
				}
			}
			else if (num_runs_completed == min_num_run_periods)	//Right at the mininum number, just put the rest of our bid in
			{
				shutdown_cost_temp = shutdown_cost_remaining;	//Last bit goes in on this bid
				shutdown_cost_remaining = 0.0;					//No more penalty remains to offset
			}
			else	//not needed, zero it out
			{
				shutdown_cost_temp = 0.0;
			}

			//Adjust bid appropriately
			for (index=0; index<bid_curve_values[latency_write_section].number_bid_curve_sections; index++)
			{
				//Apply offset to all portions of the bid - FC - SD
				bid_curve_values[latency_write_section].Curve_Info[index].price -= shutdown_cost_temp;
			}

			//Flag us as a running section
			bid_curve_values[latency_write_section].expected_state = GEN_ACTIVE;

			//Flag as a valid period
			bid_curve_values[latency_write_section].valid_bid_period = true;
		}//end GEN_ACTIVE
		else
		{
			GL_THROW("generator_controller:%s invalid generator state encountered",obj->name);
			/*  TROUBLESHOOT
			While attempting to bid into the market, the generator_controller found itself in an unknown state.
			Please try again.  If the error persists, please submit your code and a bug report via the trac website.
			*/
		}

		//See if the current value is valid, then bid in
		if (bid_curve_values[latency_write_section].valid_bid_period == true)
		{
			//Bid ourselves in appropriately
			for (index=0; index<bid_curve_values[latency_write_section].number_bid_curve_sections; index++)
			{
				//Bid into the market
				bidID_val=auction_object->submit(obj,bid_curve_values[latency_write_section].Curve_Info[index].power_delta,bid_curve_values[latency_write_section].Curve_Info[index].price,-1,BS_UNKNOWN);

				//Store the bidID
				bid_curve_values[latency_write_section].Curve_Info[index].lastbid_id = bidID_val;
			}

			//Update market period return
			next_clear = t1 + market_period;

			//Update market tracker
			last_market_id = *curr_market_id;
		}
		else	//Invalid market, put some place holder values
		{
			//Bid ourselves in appropriately
			for (index=0; index<bid_curve_values[latency_write_section].number_bid_curve_sections; index++)
			{
				//Store a junk bidID
				bid_curve_values[latency_write_section].Curve_Info[index].lastbid_id = -1;
			}

			//Update market period return
			next_clear = t1 + market_period;

			//Update market tracker
			last_market_id = -1;
		}

		//Flag an update to the output - if the market changed, the "latencied" active market has to, so recompute the output
		update_output = true;
	}

	//Determine current output
	if (update_output == true)	//Calculate a new value
	{
		//See if we're running
		if (gen_state == GEN_OFF)
		{
			//Zero the output variable
			current_power_output = 0.0;

			//See if this bid period was even valid
			if (bid_curve_values[latency_read_section].valid_bid_period == true)
			{

				for (index=0; index<bid_curve_values[latency_read_section].number_bid_curve_sections; index++)
				{
					//See if the price for this section was cleared
					if (bid_curve_values[latency_read_section].Curve_Info[index].price <= auction_object->current_frame.clearing_price)
					{
						current_power_output += bid_curve_values[latency_read_section].Curve_Info[index].power_delta;
					}
				}

				//See if it is still zero - if it isn't, that implies we turned on!
				if (current_power_output != 0.0)
					gen_state = GEN_ACTIVE;
			}
			//Defaulted else - invalid bid period, stay shut down
		}//end off
		else if (gen_state == GEN_ACTIVE)
		{
			//Zero the output variable
			current_power_output = 0.0;

			//See if the current bid period is valid
			if (bid_curve_values[latency_read_section].valid_bid_period == true)
			{
				for (index=0; index<bid_curve_values[latency_read_section].number_bid_curve_sections; index++)
				{
					//See if the price for this section was cleared
					if (bid_curve_values[latency_read_section].Curve_Info[index].price <= auction_object->current_frame.clearing_price)
					{
						current_power_output += bid_curve_values[latency_read_section].Curve_Info[index].power_delta;
					}
				}

				//See if our output is non-zero.  If it isn't, we just turned ourselves off
				if (current_power_output == 0.0)
				{
					gen_state = GEN_OFF;
				}
			}//End valid period
			else	//It isn't valid - turn us off
			{
				gen_state = GEN_OFF;
			}
		}//end active
		else	//invalid
		{
			GL_THROW("generator_controller:%s invalid generator state encountered",obj->name);
			//Defined above
		}//end invalid

		//Double check to see if this is consistent with what we expected
		if (gen_state != bid_curve_values[latency_read_section].expected_state)
		{
			//toss a warning, but just proceed (otherwise bidding gets jacked)
			gl_warning("generator_controller:%s entered an unexpected state - results may be invalid!",obj->name);
			/*  TROUBLESHOOT
			While determining the output state for the generator, an unexpected condition was encountered (it was turned on
			when it expected to be off, or vice-versa).  This means all subsequent bids are suspect or incorrect.  Please ensure
			nothing has changed cleared markets before the latency period expired.  If this message persists, please submit your code
			and a bug report via the trac website.
			*/
		}

		//Deflag the update
		update_output = false;

		//Update capacity factor - only done once per market clearing right now
		if (gen_rating != 0.0)
		{
		capacity_factor = current_power_output / gen_rating;
	}
		else	//If "no output" (easy way to turn a generator off), just set capacity factor to zero
		{
			capacity_factor = 0.0;
		}
	}

	//Apply the power output
	per_phase_power = current_power_output / num_phases;
	per_phase_old_power = last_power_output / num_phases;

	//Apply it appropriately - "added" as negative load - remove old contribution as well (way being applied causes it to just accumulate forever)
	if ((phase_information & 0x80) == 0x80)	//Triplex
	{
		power_link[0] -= per_phase_power + per_phase_old_power;
		power_link[1] -= per_phase_power + per_phase_old_power;
	}
	else	//Some variation of three phase
	{
		if ((phase_information & 0x04) == 0x04)	//Phase A
			power_link[0] -= per_phase_power - per_phase_old_power;

		if ((phase_information & 0x02) == 0x02)	//Phase B
			power_link[1] -= per_phase_power - per_phase_old_power;

		if ((phase_information & 0x01) == 0x01)	//Phase C
			power_link[2] -= per_phase_power - per_phase_old_power;
	}

	//Update value for keeping the accumulators right
	last_power_output = current_power_output;

	//Set initial state properly
	gen_state_num = (int)gen_state;

	//Return appropriately
	if (next_clear != TS_NEVER)
		return -next_clear;
	else
		return next_clear;
}

//Function to parse out the bid curve - handles allocations, if necessary
void generator_controller::parse_bid_curve(OBJECT *thisobj, TIMESTAMP t0)
{
	double plast, temp_double;
	int index, num_spaces, num_entries;
	bool last_was_space;
	char *curr_ptr, *end_ptr;

	//Make sure the curve isn't empty
	if (bidding_curve_txt[0] == '\0')
	{
		GL_THROW("generator_controller:%s has an empty bid curve!",thisobj->name);
		/*  TROUBLESHOOT
		The bid_curve property of the generator_controller object is empty.  Please specify it as pairs of power values and prices.
		*/
	}

	//Parse the curve list - look for spaces - they indicate breaks
	index=0;
	num_spaces = 0;
	last_was_space = false;

	while ((index<1025) && (bidding_curve_txt[index] != '\0'))
	{
		if (bidding_curve_txt[index] == ' ')
		{
			if (last_was_space==true)
			{
				GL_THROW("generator_controller:%s has two spaces in part of its bid curve - please fix so it is single-space delineated!",thisobj->name);
				/*  TROUBLESHOOT
				The bid_curve specified has to be delineated by single spaces.  Two spaces were found between one item.  Please fix them and try again.
				*/
			}
			else	//Not true
			{
				num_spaces++;			//increment counter
				last_was_space = true;	//Flag as a space
			}
		}
		else
			last_was_space = false;	//Flag that it wasn't a space

		index++;	//Increment the counter
	}

	//Make sure we didn't just reach the end

	//Make sure they occurred in pairs
	num_entries = (num_spaces >> 1);
	if ((num_spaces - (num_entries << 1)) != 1)	//Must be odd, since odd number of spaces needed to separate even number of numbers
	{
		GL_THROW("generator_controller:%s has an improperly formatted bid_curve",thisobj->name);
		/*  TROUBLESHOOT
		While parsing the bid_curve property of the generator object, an error was encountered.  Please ensure
		all values are valid numbers and that they occur in proper x-value and y-value pairs.
		*/
	}

	//Start with the assumption that we need 2 more than indicated (for end point) - we'll allocate that and if we don't need it, change the limit size
	number_bid_curve_sections = num_entries + 2;	//1 is just base (1 space becomes 0, which is 1 set) and 1 is the addition mentioned

	//See if a realloc is needed
	if ((bid_curve_current.Curve_Info != NULL) && (bid_curve_current.number_bid_curve_sections != number_bid_curve_sections))
	{
		//Free the current one
		gl_free(bid_curve_current.Curve_Info);

		//Ensure it was nulled
		bid_curve_current.Curve_Info = NULL;

		//Reset the variable
		bid_curve_current.number_bid_curve_sections = -1;
	}

	//See if one needs to be allocated or not?
	if (bid_curve_current.Curve_Info == NULL)
	{
		//Malloc the new one
		bid_curve_current.Curve_Info = (CURVEDETAILS*)(gl_malloc(number_bid_curve_sections*sizeof(CURVEDETAILS)));

		//Make sure it worked
		if (bid_curve_current.Curve_Info == NULL)
		{
			GL_THROW("generator_controller:%s failed to map memory for the bid curve parsing.",thisobj->name);
			/*  TROUBLESHOOT
			While attempting to allocate memory for the current bid curve implementation, a memory allocation
			error occurred.  Please try again.  If the error persists, please submit your code and a bug report
			using the trac website.
			*/
		}
	}

	//Reinitialize the curve (just in case it wasn't realloced, but was just left
	for (index=0; index<number_bid_curve_sections; index++)
	{
		bid_curve_current.Curve_Info[index].lastbid_id = 0;
		bid_curve_current.Curve_Info[index].power_delta = 0.0;
		bid_curve_current.Curve_Info[index].power_start = 0.0;
		bid_curve_current.Curve_Info[index].power_stop = 0.0;
		bid_curve_current.Curve_Info[index].price = 0.0;
	}

	//Initialize "valid bid" variable - even though it doesn't do much here
	bid_curve_current.valid_bid_period = false;

	//Initialize to "off" position as well - this shouldn't be needed, but let's be paranoid
	bid_curve_current.expected_state = GEN_OFF;

	//Set up the parsing values
	plast = 0.0;
	curr_ptr = bidding_curve_txt;

	//Loop
	for (index=0; index<(num_entries+1); index++)
	{
		bid_curve_current.Curve_Info[index].lastbid_id = 0;	//Initialize

		//Store start
		bid_curve_current.Curve_Info[index].power_start = plast;

		//Get p-value
		temp_double = strtod(curr_ptr,&end_ptr);

		//make sure it is increasing - so long as it isn't the first bid
		if ((index==0) && (temp_double == 0.0))	//First bid and zero - basically we're off
		{
			//Zero full items
			bid_curve_current.Curve_Info[index].power_stop = 0.0;
			bid_curve_current.Curve_Info[index].price = 0.0;
			bid_curve_current.Curve_Info[index].power_delta = 0;

			//Perform same actions as below, then escape
			number_bid_curve_sections = (num_entries + 1);
			
			//Store relevant values
			bid_curve_current.bid_curve_parsed = t0;
			bid_curve_current.number_bid_curve_sections = number_bid_curve_sections;

			//Deflag the update (if it actually was one, may have just been init)
			update_curve = false;

			return;	//Get us out of here
		}
		//Defaulted else - just continue on our merry way

		if (plast >= temp_double)	//Same or greater, implies wrong direction!
		{
			GL_THROW("generator_controller:%s does not have a monotonically increasing bid curve!",thisobj->name);
			/*  TROUBLESHOOT
			A bid curve in the generator_controller object must have incrementing price points.  If they are the same, or negative,
			the bidding process will fail.  Please fix this error and try again.
			*/
		}

		//Store p-end
		bid_curve_current.Curve_Info[index].power_stop = temp_double;

		//Copy in next values
		plast = temp_double;
		curr_ptr = end_ptr;

		//Now get the price
		bid_curve_current.Curve_Info[index].price = strtod(curr_ptr,&end_ptr);

		//Populate the delta
		bid_curve_current.Curve_Info[index].power_delta = bid_curve_current.Curve_Info[index].power_stop - bid_curve_current.Curve_Info[index].power_start;

		//Update the pointer
		curr_ptr = end_ptr;
	}//end parsing FOR

	//See if the last one was below the limit - if not, make one more
	if (bid_curve_current.Curve_Info[num_entries].power_stop < gen_rating)
	{
		//Populate the "last" entry
		bid_curve_current.Curve_Info[(num_entries+1)].lastbid_id = 0;	//Init

		bid_curve_current.Curve_Info[(num_entries+1)].power_start = bid_curve_current.Curve_Info[num_entries].power_stop;	//Last point is our start
		bid_curve_current.Curve_Info[(num_entries+1)].power_stop = gen_rating;	//End point is the max capacity
		bid_curve_current.Curve_Info[(num_entries+1)].power_delta = bid_curve_current.Curve_Info[(num_entries+1)].power_stop - bid_curve_current.Curve_Info[(num_entries+1)].power_start;	//Delta

		bid_curve_current.Curve_Info[(num_entries+1)].price = bid_curve_current.Curve_Info[num_entries].price;	//Price is just last price - if it is supposed to be different, they should have said so
	}
	else if (bid_curve_current.Curve_Info[num_entries].power_stop > gen_rating)	//Bigger - oh noes!
	{
		GL_THROW("Bid curve in generator_controller:%s exceeds the maximum generator rating!",thisobj->name);
		/*  TROUBLESHOOT
		The parsed bid curve in the generator_controller object had a stopping power value greater than the capacity.
		This is not allowed.  Please fix this and try again.
		*/
	}
	else	//Equal value - just make sure the counter is 1 less
	{
		number_bid_curve_sections = (num_entries + 1);
	}
	
	//Store relevant values
	bid_curve_current.bid_curve_parsed = t0;
	bid_curve_current.number_bid_curve_sections = number_bid_curve_sections;

	//Deflag the update (if it actually was one, may have just been init)
	update_curve = false;
}

//Copies current bid curve into latency storage, if necessary
void generator_controller::copy_bid_curve(OBJECT *thisobj)
{
	//See if the number of curve sections matches
	if (bid_curve_values[latency_write_section].number_bid_curve_sections != bid_curve_current.number_bid_curve_sections)
	{
		//Free it, if necessary
		if (bid_curve_values[latency_write_section].Curve_Info != NULL)
		{
			gl_free(bid_curve_values[latency_write_section].Curve_Info);
		}

		//Allocate
		//Malloc the new one
		bid_curve_values[latency_write_section].Curve_Info = (CURVEDETAILS*)(gl_malloc(bid_curve_current.number_bid_curve_sections*sizeof(CURVEDETAILS)));

		//Make sure it worked
		if (bid_curve_values[latency_write_section].Curve_Info == NULL)
		{
			GL_THROW("generator_controller:%s failed to map memory for the bid curve parsing.",thisobj->name);
			//Defined above
		}

		//Store the size
		bid_curve_values[latency_write_section].number_bid_curve_sections = bid_curve_current.number_bid_curve_sections;
	}

	//copy the values
	memcpy(bid_curve_values[latency_write_section].Curve_Info,bid_curve_current.Curve_Info,bid_curve_current.number_bid_curve_sections*sizeof(CURVEDETAILS));

	//Update the timestamp
	bid_curve_values[latency_write_section].bid_curve_parsed = bid_curve_current.bid_curve_parsed;

	//Set the allowed bidding flag to false by default
	bid_curve_values[latency_write_section].valid_bid_period = false;

	//By default, assume we will not be running - this will get populated properly elsewhere
	bid_curve_values[latency_write_section].expected_state = GEN_OFF;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: generator_controller
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_generator_controller(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(generator_controller::oclass);
		if (*obj!=NULL)
		{
			generator_controller *my = OBJECTDATA(*obj,generator_controller);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("%s %s (id=%d): %s", (*obj)->name?(*obj)->name:"unnamed", (*obj)->oclass->name, (*obj)->id, msg);
		return 0;
	}
	return 1;
}

EXPORT int init_generator_controller(OBJECT *obj, OBJECT *parent)
{
	try {
			return OBJECTDATA(obj,generator_controller)->init(parent);
	}
	catch (const char *msg)
	{
		gl_error("%s %s (id=%d): %s", obj->name?obj->name:"unnamed", obj->oclass->name, obj->id, msg);
		return 0;
	}
	return 1;
}

/**
* Sync is called when the clock needs to advance
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_generator_controller(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	generator_controller *my = OBJECTDATA(obj,generator_controller);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			obj->clock = t1;
			break;
		case PC_POSTTOPDOWN:
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
	}
	catch (const char *msg)
	{
		gl_error("generator_controller %s (%s:%d): %s", obj->name, obj->oclass->name, obj->id, msg);
		return TS_INVALID;
	}
	catch (...)
	{
		gl_error("generator_controller %s (%s:%d): unknown exception", obj->name, obj->oclass->name, obj->id);
		return TS_INVALID;
	}
	return t2;
}

EXPORT int isa_generator_controller(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,generator_controller)->isa(classname);
}


/**@}**/
