/** $Id: generator_controller.cpp 4738 2014-07-03 00:55:39Z dchassin $
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
		oclass = gl_register_class(module,"generator_controller",sizeof(generator_controller),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
			PT_enumeration,"generator_state", PADDR(gen_state), PT_DESCRIPTION, "Current generator state",
				PT_KEYWORD, "OFF", (enumeration)GEN_OFF,
				PT_KEYWORD, "RUNNING", (enumeration)GEN_ACTIVE,
				PT_KEYWORD, "STARTING", (enumeration)GEN_STARTUP,
			PT_enumeration,"amortization_type", PADDR(amort_type), PT_DESCRIPTION, "Amortization compounding method",
				PT_KEYWORD, "EXPONENTIAL", (enumeration)EXPONENTIAL,
				PT_KEYWORD, "LINEAR_COST", (enumeration)LINEAR_COST,
				PT_KEYWORD, "LINEAR_BID", (enumeration)LINEAR_BID,
			PT_int32,"generator_state_number", PADDR(gen_state_num), PT_DESCRIPTION, "Current generator state as numeric value",
			PT_object,"market", PADDR(market_object), PT_DESCRIPTION, "Market the object will watch and bid into",
			PT_char1024,"bid_curve", PADDR(bidding_curve_txt), PT_DESCRIPTION, "Bidding curve text format",
			PT_char1024,"bid_curve_file", PADDR(bidding_curve_file), PT_DESCRIPTION, "Bidding curve file name",
			PT_double,"bid_generator_rating[VA]", PADDR(bid_gen_rating), PT_DESCRIPTION, "Size of generator in VA for the bid curve",
			PT_bool,"update_curve", PADDR(update_curve), PT_DESCRIPTION, "Flag to force updating of bidding curve parse",
			PT_bool,"is_marginal_gen",PADDR(is_marginal_gen),PT_DESCRIPTION, "Flag to indicate if the generator is a marginal generator",
			PT_double,"generator_rating[VA]", PADDR(gen_rating), PT_DESCRIPTION, "Size of generator in VA for the active bid",
			PT_double,"generator_output", PADDR(current_power_output), PT_DESCRIPTION, "Current real power output of generator",	//Units removed for now -For now, "market units" is assumed as MWh - TODO: this needs to be fixed as part of ticket #574 */
			PT_double,"input_unit_base[MW]", PADDR(input_unit_base), PT_DESCRIPTION, "Base multiplier for generator bid units", //This should become irrelevant if #574 is fixed
			PT_double,"startup_cost[$]", PADDR(startup_cost), PT_DESCRIPTION, "Cost to start the generator from OFF status",
			PT_double,"shutdown_cost[$]", PADDR(shutdown_cost), PT_DESCRIPTION, "Cost to shut down the generator prematurely",
			PT_double,"minimum_runtime[s]", PADDR(minimum_runtime_dbl), PT_DESCRIPTION, "Minimum time the generator should run to avoid shutdown cost",
			PT_double,"minimum_downtime[s]", PADDR(minimum_downtime_dbl), PT_DESCRIPTION, "Minimum down time for the generator before it can bid again",
			PT_double,"capacity_factor", PADDR(capacity_factor), PT_DESCRIPTION, "Calculation of generator's current capacity factor based on the market period",
			PT_double,"amortization_factor[1/s]",PADDR(amortization_value), PT_DESCRIPTION, "Exponential decay factor in 1/s for shutdown cost repayment",
			PT_double, "bid_delay", PADDR(bid_delay_dbl), PT_DESCRIPTION, "Time before a market closes to bid",
			PT_enumeration,"generator_attachment", PADDR(gen_mode), PT_DESCRIPTION, "Generator attachment type - determines interactions",
				PT_KEYWORD, "STANDALONE", (enumeration)STANDALONE,
				PT_KEYWORD, "BUILDING", (enumeration)BUILDING,
			PT_double, "building_load_curr", PADDR(curr_building_load), PT_DESCRIPTION, "Present building load value (if BUILDING attachment)",
			PT_double, "building_load_bid", PADDR(bid_building_load), PT_DESCRIPTION, "Expected building load value for currently bidding market period (if BUILDING attachment)",
			PT_double, "year_runtime_limit[h]", PADDR(total_hours_year), PT_DESCRIPTION, "Total number of hours the generator can run in a year",
			PT_double, "current_year_runtime[h]", PADDR(hours_run_this_year), PT_DESCRIPTION, "Total number of hours generator has run this year",
			PT_char1024, "runtime_year_end", PADDR(runtime_year_end), PT_DESCRIPTION, "Date and time the generator runtime year resets",
			PT_double, "scaling_factor[unit]", PADDR(scaling_factor), PT_DESCRIPTION, "scaling factor applied to license premium calculation",
			PT_double, "license_premium", PADDR(license_premium), PT_DESCRIPTION, "current value of the generator license premium calculated",
			PT_double, "hours_in_year[h]", PADDR(hours_in_year), PT_DESCRIPTION, "Number of hours assumed for the total year",
			PT_double, "op_and_maint_cost[$]", PADDR(op_and_maint_cost), PT_DESCRIPTION, "Operation and maintenance cost per runtime year",
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

	gen_rating = 0.0;
	bid_gen_rating = 0.0;

	bid_curve_values = NULL;
	bid_curve_current.bid_curve_parsed = 0;
	bid_curve_current.Curve_Info = NULL;
	bid_curve_current.number_bid_curve_sections = -1;
	bid_curve_current.valid_bid_period = false;
	bid_curve_current.expected_state = GEN_ACTIVE;
	bid_curve_current.generator_capacity = 0.0;

	number_bid_curve_sections = 0;
	number_latency_sections = 0;
	latency_write_section = 0;
	latency_read_section = 0;

	update_curve = false;

	is_marginal_gen = false;

	runtime_hours_left = true;	//Generator is assumed to have life left by default

	last_market_id = 0;
	curr_market_id = NULL;
	bid_market_id = 0;

	power_link = NULL;
	phase_information = 0x00;
	num_phases = 0;

	next_clear = NULL;
	next_bid = TS_NEVER;

	current_power_output = 0.0;
	last_power_output = 0.0;
	update_output = false;
	curve_file_mode = false;		//By default, assumes we're parsing a text field, not a file

	gen_state = GEN_ACTIVE;			//Assumes start running
	prev_bid_state = GEN_ACTIVE;		//Assumes were running
	amort_type = EXPONENTIAL;		//Amortization defaults to exponential decay
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
	bid_delay_dbl = 0.0;		//Start with 1 second delay
	bid_delay = 0;				//Starts off just being one second before next (gets over written anyways)

	gen_mode = STANDALONE;		//Assumes operating as a normal generator
	curr_building_load = 0.0;	//Assumes no building load at first
	bid_building_load = 0.0;	//No bid load either
	total_hours_year = 0.0;		//0.0 = infinite hours
	hours_run_this_year = 0.0;	//None run yet
	scaling_factor = 0.2;		//Default value from an Olypen generator
	runtime_year_end[0] = '\0';	//No date specified
	license_premium = 0.0;		//No premium calculated
	hours_in_year = 8760.0;		//Assumes normal year
	op_and_maint_cost = 0.0;	//No O&M Cost
	op_maint_cost_pwr = 0.0;	//No O&M per unit-hour

	end_of_runtime_year = 0;	//No date set, by default
	seconds_in_year = 31536000.0;	//8760 hours, in seconds
	last_update_time = 0;		//Zero for start

	input_unit_base = 1.0;			//Input base is assumed to be MW, in line with what the typical market it (part of ticket #574)
	unit_scaling_value = 1000000.0;	//Multiplier for bids to get from "input" to load posting value

	return 1;
}

int generator_controller::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	PROPERTY *ptemp;
	set *temp_set;
	int index;
	double glob_min_timestep, temp_val;
	char temp_buff[128];

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
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("generator_controller::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
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
	if((market_object->flags & OF_INIT) != OF_INIT){
		char objname[256];
		gl_verbose("generator_controller::init(): deferring initialization on %s", gl_name(market_object, objname, 255));
		return 2; // defer
	}
	//Get this object
	auction_object = OBJECTDATA(market_object,auction);

	//Pull the market information
	market_period = (TIMESTAMP)(auction_object->dPeriod);
	market_latency = (TIMESTAMP)(auction_object->dLatency);
	price_cap_value = auction_object->pricecap+0.01;	//Set to 0.01 higher - used as an initialization variable
	curr_market_id = &(auction_object->market_id);
	next_clear = &(auction_object->clearat);

	//Determine number of latency slots we need
	number_latency_sections = (int)((market_latency/market_period)+2);	//Delay # + current bid + current active

	//See if gen_rating is zero
	if (gen_rating > 0.0)	//A value is in here
	{
		if (bid_gen_rating <= 0)	//An invalid value for capacity
		{
			bid_gen_rating = gen_rating;	//Put gen rating in its place
		}
		else	//It is valid as well, toss a warning
		{
			gl_warning("Generator_rating and bid_generator_rating both have values in generator_controller:%s",obj->name);
			/*  TROUBLESHOOT
			Both the generator_rating and bid_generator_rating have non-zero values at the start of the simulation.  bid_generator_rating
			will eventually override generator_rating, so please ensure your values are entered appropriately.
			*/
		}
	}
	//Defaulted else - gen rating is invalid, so hopefully bid_gen_rating is right (otherwise next catch gets it)

	//Make sure the rating is non-negative and non-zero
	if (bid_gen_rating <= 0)
	{
		GL_THROW("Generator rating in generator_controller:%s must be positive!",obj->name);
		/*  TROUBLESHOOT
		The generator size specified for the current bid generator_controller must be a positive, non-zero value.  Please try again.
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
		bid_curve_values[index].expected_state = (GENERATOR_STATE)gen_state;		//Initial bidding needs to know how we are running - assume it populates all the way back
		bid_curve_values[index].generator_capacity = 0.0;		//Initially has no capacity
		bid_curve_values[index].expected_building_load = 0.0;	//Assume no building load - either not needed, or is just zeroed
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

	//Determine an initial mode - see if the file field is non-zero
	if (bidding_curve_file[0]!='\0')	//Something is there, default to file
	{
		curve_file_mode = true;	//Set up to parse a file, rather than the curve
	}

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

	//Make sure bid delay wasn't entered as negative - it's implied negative
	if (bid_delay_dbl < 0)
		bid_delay_dbl = -bid_delay_dbl;

	//Convert to timestamp (cast it)
	bid_delay = (TIMESTAMP)bid_delay_dbl;

	//See if a minimum timestep is in place - if so, make sure bid delay is set right
	//Retrieve the global value, only does so as a text string for some reason
	gl_global_getvar("minimum_timestep",temp_buff,sizeof(temp_buff));

	//Initialize our parsing variables
	index = 0;
	glob_min_timestep = 0.0;

	//Loop through the buffer
	while ((index < 128) && (temp_buff[index] != '\0'))
	{
		glob_min_timestep *= 10;					//Adjust previous iteration value
		temp_val = (double)(temp_buff[index]-48);	//Decode the ASCII
		glob_min_timestep += temp_val;				//Accumulate it

		index++;									//Increment the index
	}

	if (glob_min_timestep > 1)					//Now check us
	{
		//See if bid delay is at least one (if it isn't zero)
		if ((bid_delay != 0) && (bid_delay < (TIMESTAMP)(glob_min_timestep)))
		{
			//Round it to one
			bid_delay = (TIMESTAMP)glob_min_timestep;

			//Let user know
			gl_warning("bid_delay was less than the current minimum_timestep setting - rounded up to one whole timestep");
			/*  TROUBLESHOOT
			The time for bid_delay was less than the value set for minimum_timestep in the GLM.  It was rounded to be at
			least one minimum_timestep in value.  If this is not desired, fix your bid delay or minimum_timestep
			*/
		}
	}

	//Make sure the bid delay isn't bigger than a market period
	if (bid_delay > market_period)
	{
		GL_THROW("bid_delay is greater than the market period!");
		/*  TROUBLESHOOT
		The value for bid_delay is larger than the period of the market generator_controller is bidding into.
		Adjust one or the other and try again.
		*/
	}

	//Update the previous state variable - just to be safe
	if (gen_state != GEN_ACTIVE)
	{
		prev_bid_state = GEN_OFF;	//Assumes we were shutdown - if are OFF, were OFF.  If are starting, were OFF
	}

	//Initialize next_bid to current time - that way it fires
	next_bid = gl_globalclock;

	//Initialize last run time to current time
	last_update_time = gl_globalclock;

	//See if a yearly runtime limit is specified
	if (total_hours_year != 0.0)
	{
		//Make sure it is positive
		if (total_hours_year < 0.0)
		{
			GL_THROW("The yearly runtime limit a generator can run must be a positive number");
			/*  TROUBLESHOOT
			The value for yearly_runtime_limit was a negative value.  Please set it to zero (infinite runtime) or
			a positive value.
			*/
		}
		else	//Good value
		{
			//Make sure the initial "total run so far" is valid
			if (hours_run_this_year < 0.0)
			{
				GL_THROW("The current year's runtime must be a positive number, or zero");
				/*  TROUBLESHOOT
				The value for current_year_runtime is a negative number.  It must be zero or positive.
				*/
			}
			else	//Non-negative
			{
				if (hours_run_this_year >= total_hours_year)
				{
					gl_warning("The generator has already exhausted its yearly runtime quota, it will not run this year");
					/*  TROUBLESHOOT
					The value specified for current_year_runtime is larger or equal to yearly_runtime_limit.  This means the generator
					has exhausted all of its available runtime hours for the year and will no longer run.
					*/

					runtime_hours_left = false;	//Flag as done
				}
				else	//Value in range
				{
					//Check the number of hours in a year
					if (hours_in_year <= 0.0)
					{
						GL_THROW("The number of hours in a year must be a positive, non-zero number");
						/*  TROUBLESHOOT
						The hours_in_year value must be a positive, non-negative number (typically 8760).  This value is the calendar
						year, not the runtime year.
						*/
					}
					else	//Valid
					{
						//Convert it to seconds
						seconds_in_year = hours_in_year * 3600.0;	//Convert it to seconds

						//Now check the end of runtime year value
						if (runtime_year_end[0] == '\0')	//Empty
						{
							GL_THROW("The runtime_year_end value must be specified");
							/*  TROUBLESHOOT
							The generator_controller can not properly calculate the license premium unless the value for the end of the
							run time year is specified in runtime_year.  Specify this in normal GridLAB-D date/time syntax.
							e.g., "2007-04-30 00:00:00 PST" for April 30, 2007 at midnight PST
							*/
						}
						else	//Entry there, see if it works
						{
							//Convert the string into a TIMESTAMP value
							end_of_runtime_year = gl_parsetime(runtime_year_end);

							//Make sure it worked - check against "current time"
							if (end_of_runtime_year <= gl_globalclock)
							{
								GL_THROW("Failed to parse runtime_year_end");
								/*  TROUBLESHOOT
								An error occurred while converting the value in runtime_year_end into a date.  Ensure you have written a proper
								date and have not specified a value in the simulation's past.
								*/
							}
							//Default else - valid value
						}//End runtime_year_end populated
					}//end valid hours in year
				}//End check of if hours_run already equal or more than allowed
			}//End of valid hours run so farin year

			//Compute O&M per "generator unit hour" - done by "hours per year allowed to run"
			if (bid_gen_rating != 0.0)	//Make sure it isn't zero - if it is, we can't calculate anything
			{
				op_maint_cost_pwr = op_and_maint_cost / bid_gen_rating / total_hours_year;
			}
			else
			{
				op_maint_cost_pwr = 0.0;	//Assumes no O&M;

				gl_warning("No value for gen_rating set, assuming $0.0 Op and Maint cost");
				//Defined below
			}
		}//End non-negative run so far
	}//End total hours checks
	else	//Total year hours is zero = infinite
	{
		//Calculate O&M based on a full year
		//Compute O&M per "generator unit hour" - done by "hours per year allowed to run"
		if (hours_in_year > 0.0)
		{
			if (bid_gen_rating != 0.0)	//Make sure it isn't zero - if it is, we can't calculate anything
			{
				op_maint_cost_pwr = op_and_maint_cost / bid_gen_rating / hours_in_year;
			}
			else
			{
				op_maint_cost_pwr = 0.0;	//Assumes no O&M;

				gl_warning("No value for bid_gen_rating set, assuming $0.0 Op and Maint cost");
				/*  TROUBLESHOOT
				While attempting to convert the operations and maintenance annual cost into an energy cost,
				the generator rating was not set.  If this is not correct, please specify a value for bid_gen_rating.
				*/
			}
		}
		else if (hours_in_year == 0.0)	//Just use a normal year
		{
			//Reset it
			hours_in_year = 8760.0;

			//Compute O&M per "generator unit hour" - done by "hours per year allowed to run"
			if (bid_gen_rating != 0.0)	//Make sure it isn't zero - if it is, we can't calculate anything
			{
				op_maint_cost_pwr = op_and_maint_cost / bid_gen_rating / hours_in_year;
			}
			else
			{
				op_maint_cost_pwr = 0.0;	//Assumes no O&M;

				gl_warning("No value for bid_gen_rating set, assuming $0.0 Op and Maint cost");
				/*  TROUBLESHOOT
				While attempting to convert the operations and maintenance annual cost into an energy cost,
				the generator rating was not set.  If this is not correct, please specify a value for bid_gen_rating.
				*/
			}
		}
		else	//Failure
		{
			GL_THROW("hours_in_year is a negative number");
			/*  TROUBLESHOOT
			The hours_in_year value must be a positive or zero number (typically 8760) for computing infinite-runtime O&M costs.
			*/
		}
	}//End infinite year time

	//Check the input scaling base - make sure it isn't zero or negative
	/* TODO: may need to be fixed as part of ticket #574 */
	if (input_unit_base <= 0.0)
	{
		GL_THROW("input_unit_base negative or zero!");
		/*  TROUBLESHOOT
		The value of input_unit_base must be a positive, non-zero number.  Please specify it as such.
		*/
	}

	//Determine bidding unit base
	unit_scaling_value = 1000000.0*input_unit_base;

	return 1;
}

TIMESTAMP generator_controller::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);
	int index;
	int64 bidID_val;
	double per_phase_power, per_phase_old_power, shutdown_cost_temp, prev_section_power, prev_section_price, temp_time_var, temp_power_value;

	//Update runtime hours - if necessary
	if (total_hours_year != 0.0)
	{
		//See if we need an update
		if (t1 != last_update_time)
		{
			//See if we need to update the runtime calendar
			if (t0 >= end_of_runtime_year)
			{
				//Zero the accumulator
				hours_run_this_year = 0.0;

				//Set the end_of_runtime_year to the next year - just add hours of year
				end_of_runtime_year += (TIMESTAMP)(hours_in_year * 3600.0);

				//Update the track variable, in case we exceeded limits
				runtime_hours_left = true;
			}

			//See how long has passed
			temp_time_var = (double)(t1 - last_update_time);

			//See if we were running or starting
			if ((gen_state == GEN_ACTIVE) || (gen_state == GEN_STARTUP))
			{
				hours_run_this_year += temp_time_var/3600.0;	//Accumulate this run time
			}

			//See if we're valid
			if (hours_run_this_year >= total_hours_year)
				runtime_hours_left = false;

			//Store the update time
			last_update_time = t1;
		}
	}

	//If market changes - update our output appropriately - update tracking variables for market bidding as well
	if (bid_market_id != *curr_market_id)
	{
		//Populate the previous state - check against 0 bid
		if (auction_object->cleared_frame.clearing_price >= bid_curve_values[latency_write_section].Curve_Info[0].price)	//Price met
		{
			//See if it was a startup bid - if so, flag it as running now
			if (bid_curve_values[latency_write_section].expected_state == GEN_STARTUP)
			{
				//Change over to running - otherwise subsequent bids might get a little squirrely
				bid_curve_values[latency_write_section].expected_state = GEN_ACTIVE;

				//Update our amortization value - only read this each time the generator kicks on
				if (amort_type == EXPONENTIAL)
				{
					amortization_scaled = exp(-1.0* amortization_value * (double)(market_period));
				}
				else if (amort_type == LINEAR_COST)
				{
					//Apply a linear scaling assumption - linearly scale decreasing cost
					amortization_scaled = amortization_value * (double)(market_period);
				}
			}

			//Increment startup run tracker
			num_runs_completed++;

		}//End last price met
		else if (bid_curve_values[latency_write_section].expected_state == GEN_ACTIVE)	//Was on, but didn't meet continued running requirements
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

		//Flag an update to the output - if the market changed, the "latencied" active market has to, so recompute the output
		update_output = true;

		//if we've exceeded runtime limits, flag us as off, just to be safe (should already be there)
		if (runtime_hours_left == false)
			gen_state = GEN_OFF;

		//Update market tracking (clearing) variable
		bid_market_id = *curr_market_id;

		//Store our current load value - if needed
		if (gen_mode == BUILDING)
			bid_curve_values[latency_write_section].expected_building_load = bid_building_load;
		
	}//End last market isn't current market

	//Bidding code
	//If market changes, update
	if ((last_market_id != *curr_market_id) && (t1 >= next_bid))
	{
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

		//Update the license premium appropriately
		if (total_hours_year == 0.0)	//Infinite run
			license_premium = 1.0;
		else if (total_hours_year > hours_run_this_year)	//Track - but still have life left
		{
			temp_time_var = (double)(end_of_runtime_year - t1);	//See how much time is left in the year

			//Make sure we have enough runtime to continue
			if (((total_hours_year - hours_run_this_year)*3600.0) >= (double)(market_period))
			{
				//Update the license premium
				license_premium = scaling_factor * temp_time_var / seconds_in_year * total_hours_year / (total_hours_year - hours_run_this_year);
			}
			else	//Not enough for another period, shut us down
			{
				license_premium = 0.0;
				runtime_hours_left = false;	//Flag us as done - just in case was missed above
			}
		}
		else	//No life left
		{
			license_premium = 0.0;			//Flag as 0 value
			runtime_hours_left = false;		//Indicate we are done - just in case
		}

		//Handle bidding curve logic appropriately
		if (prev_bid_state == GEN_OFF)	//Generator was off or just went through a startup attempt - needs a start up value
		{
			//See if we're allowed to even try
			if (runtime_hours_left == true)
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
					if (amort_type == LINEAR_BID)
					{
						//Determine the startup cost and amortization constant - base on shutdown cost (repay over full interval)
						//Put into the tracking variable
						shutdown_current_cost = shutdown_cost * 2.0 / ((double)(min_num_run_periods));

						//Put into amortization variable for future use
						amortization_scaled = shutdown_current_cost / ((double)(min_num_run_periods+1));
					}
					else	//Must be other method
					{
						//Update the tracking variable
						shutdown_current_cost = shutdown_cost;
					}

					//Ensure the shutdown cost remaining is set to the full cost for these bids - all types use this the same
					shutdown_cost_remaining = shutdown_cost;

				}

				//See if we've been off long enough - correct for the minimum shutdown time
				if (market_bidding_time >= minimum_downtime)
				{
					//Flag as a valid bid
					bid_curve_values[latency_write_section].valid_bid_period = true;

					//Apply appropriate cost values (shutdown and startup)
					for (index=0; index<bid_curve_values[latency_write_section].number_bid_curve_sections; index++)
					{
						//Add in the O&M Cost value
						bid_curve_values[latency_write_section].Curve_Info[index].price += op_maint_cost_pwr;

						//Scale per the market unit base
						/* TODO: this needs to be fixed as part of ticket #574 */
						bid_curve_values[latency_write_section].Curve_Info[index].price /= input_unit_base;

						//Other costs
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


						//Scale this section by the license premium
						bid_curve_values[latency_write_section].Curve_Info[index].price *= license_premium;
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
			}//end allowed to bid
			else	//Runtime exceeded
			{
				//Flag as invalid interval
				bid_curve_values[latency_write_section].valid_bid_period = false;

				//Set all bids to the price cap value, +4 (just cause) - won't bid in, but this will help ensure nothing wierd happens
				for (index=0; index<bid_curve_values[latency_write_section].number_bid_curve_sections; index++)
				{
					//Set bids to insane value, just in case
					bid_curve_values[latency_write_section].Curve_Info[index].price += price_cap_value + 4;
				}

				//Set the state, just in case
				bid_curve_values[latency_write_section].expected_state = GEN_OFF;

			}//End minimum downtime not reached
		}//end GEN_OFF
		else if (prev_bid_state == GEN_ACTIVE)	//Running, bid appropriately
		{
			if (runtime_hours_left == true)	//We can still bid
			{
				//Compute the shutdown cost influence, if still needed
				if (num_runs_completed < min_num_run_periods)
				{
					//Compute the current amortization value
					if (amort_type == EXPONENTIAL)
					{
						//Exponential update
						shutdown_current_cost *= amortization_scaled;
					}
					else if (amort_type == LINEAR_COST)
					{
						//Linear update is just the value
						shutdown_current_cost = amortization_scaled;
					}
					else if (amort_type == LINEAR_BID)
					{
						//Update bid - linear decrease
						shutdown_current_cost -= amortization_scaled;
					}

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
					//Add in the O&M Cost value
					bid_curve_values[latency_write_section].Curve_Info[index].price += op_maint_cost_pwr;

					//Scale per the market unit base
					/* TODO: this needs to be fixed as part of ticket #574 */
					bid_curve_values[latency_write_section].Curve_Info[index].price /= input_unit_base;

					//Apply offset to all portions of the bid - FC - SD
					bid_curve_values[latency_write_section].Curve_Info[index].price -= shutdown_cost_temp;

					//Scale this section by the license premium and the unit conversion
					bid_curve_values[latency_write_section].Curve_Info[index].price *= license_premium;
				}

				//Flag us as a running section
				bid_curve_values[latency_write_section].expected_state = GEN_ACTIVE;

				//Flag as a valid period
				bid_curve_values[latency_write_section].valid_bid_period = true;
			}
			else	//Must not be allowed to run, handle appropriately
			{
				//Set bid, appropriately
				for (index=0; index<bid_curve_values[latency_write_section].number_bid_curve_sections; index++)
				{
					//Set to implausable value - should never bid this, but jsut in case
					bid_curve_values[latency_write_section].Curve_Info[index].price += price_cap_value + 4;
				}

				//Flag us as stopped
				bid_curve_values[latency_write_section].expected_state = GEN_OFF;

				//Flag as an invalid period - keeps us off
				bid_curve_values[latency_write_section].valid_bid_period = false;
			}
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
			if (gen_mode == STANDALONE)
			{
				//Bid ourselves in appropriately
				for (index=0; index<bid_curve_values[latency_write_section].number_bid_curve_sections; index++)
				{
					//Bid into the market
					//For now, "market units" is assumed as MWh
					/* TODO: this needs to be fixed as part of ticket #574 */
					temp_power_value = bid_curve_values[latency_write_section].Curve_Info[index].power_delta*input_unit_base;

					//Bid in proper state
					if (gen_state != GEN_OFF)	//Generator is not off, so must be starting/started
						bidID_val=auction_object->submit(obj,temp_power_value,bid_curve_values[latency_write_section].Curve_Info[index].price,-1,BS_ON);
					else	//Must be off then
						bidID_val=auction_object->submit(obj,temp_power_value,bid_curve_values[latency_write_section].Curve_Info[index].price,-1,BS_OFF);

					//Store the bidID
					bid_curve_values[latency_write_section].Curve_Info[index].lastbid_id = bidID_val;
				}
			}//End Standalone bidding mode
			else	//Must be building mode
			{
				//Assumes there's only 1 bidding section - if more, this doesn't work
				if (bid_curve_values[latency_write_section].number_bid_curve_sections>1)
				{
					GL_THROW("BUILDING mode only supports single section fuel curves right now");
					/*  TROUBLESHOOT
					When operating generator_controller in BUILDING mode, it can only take a single-value
					fuel curve.  If the fuel curve has more than one section, generator_controller can't handle it
					properly and would do odd things.  This may be fixed in future releases.
					*/
				}//More than 1
				else	//Only 1
				{
					//Bid into the market
					//For now, "market units" is assumed as MWh
					/* TODO: this needs to be fixed as part of ticket #574 */
					//temp_power_value = bid_curve_values[latency_write_section].Curve_Info[0].power_delta*input_unit_base;
					temp_power_value = bid_curve_values[latency_write_section].expected_building_load*input_unit_base;
					
					//Bid us in as a negative load - if gen is off, the building is "on"
					if (gen_state == GEN_OFF)
						bidID_val=auction_object->submit(obj,-temp_power_value,bid_curve_values[latency_write_section].Curve_Info[0].price,-1,BS_ON);
					else	//Must be on or starting, so building is "off"
						bidID_val=auction_object->submit(obj,-temp_power_value,bid_curve_values[latency_write_section].Curve_Info[0].price,-1,BS_OFF);

					//Store the bidID
					bid_curve_values[latency_write_section].Curve_Info[0].lastbid_id = bidID_val;
				}//End only 1 (valid) bidding curve section
			}//End building mode bid

			//Update the bid time - do different if a delay is present
			if (bid_delay != 0)
			{
				//Update next bid time
				next_bid = *next_clear + market_period - bid_delay;
			}
			else
			{
				//Update next bid time
				next_bid = *next_clear;
			}

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

			//Update the bid time - do different if a delay is present
			if (bid_delay != 0)
			{
				//Update next bid time
				next_bid = *next_clear + market_period - bid_delay;
			}
			else
			{
				//Update next bid time
				next_bid = *next_clear;
			}

			//Update market tracker
			last_market_id = -1;
		}

		//Write the current capacity value
		bid_curve_values[latency_write_section].generator_capacity = bid_gen_rating;
	}//End bidding curve

	//Determine current output
	if (update_output == true)	//Calculate a new value
	{
		//Default is us not to be the marginal generator
		is_marginal_gen = false;

		//Read out the current building load value, if needed
		if (gen_mode == BUILDING)
			curr_building_load = bid_curve_values[latency_read_section].expected_building_load;

		//See if we're running
		if (gen_state == GEN_OFF)
		{
			//Zero the output variable
			current_power_output = 0.0;

			//See if this bid period was even valid
			if (bid_curve_values[latency_read_section].valid_bid_period == true)
			{
				//Reset previous bid section variables
				prev_section_power = 0.0;
				prev_section_price = 0.0;

				//Parse the bidding curve
				for (index=0; index<bid_curve_values[latency_read_section].number_bid_curve_sections; index++)
				{
					//See if the price for this section was cleared
					if (bid_curve_values[latency_read_section].Curve_Info[index].price <= auction_object->current_frame.clearing_price)
					{
						//For now, "market units" is assumed as MWh
						/* TODO: this needs to be fixed as part of ticket #574 */
						current_power_output += bid_curve_values[latency_read_section].Curve_Info[index].power_delta;

						//Update the last value valid - used for marginal quantities
						prev_section_power = bid_curve_values[latency_read_section].Curve_Info[index].power_delta;
						prev_section_price = bid_curve_values[latency_read_section].Curve_Info[index].price;
					}
				}

				//See if it is still zero - if it isn't, that implies we turned on!
				if (current_power_output != 0.0)
				{
					gen_state = GEN_ACTIVE;

					//See if our last price was the clearing price
					if (prev_section_price == auction_object->current_frame.clearing_price)
					{
						//See if it was a marginal seller condition
						if (auction_object->cleared_frame.clearing_type == CT_SELLER)
						{
							//Figure out our contribution (base on ratio of marginal - in case there is more than one of us) - subtract off the extra
							current_power_output -= prev_section_power*(1-auction_object->cleared_frame.marginal_frac);

							//Flag us as marginal
							is_marginal_gen = true;

						}//End marginal seller condition
					}//End last valid price was clearing price
				}//End power output non-zero
			}
			//Defaulted else - invalid bid period, stay shut down
		}//end off
		else if (gen_state == GEN_ACTIVE)
		{
			//Zero the output variable
			current_power_output = 0.0;

			//See if we're a building or not
			if (gen_mode == STANDALONE)
			{
				//See if the current bid period is valid
				if (bid_curve_values[latency_read_section].valid_bid_period == true)
				{
					//Reset previous bid section variables
					prev_section_power = 0.0;
					prev_section_price = 0.0;

					//Parse the bidding curve
					for (index=0; index<bid_curve_values[latency_read_section].number_bid_curve_sections; index++)
					{
						//See if the price for this section was cleared
						if (bid_curve_values[latency_read_section].Curve_Info[index].price <= auction_object->current_frame.clearing_price)
						{
							current_power_output += bid_curve_values[latency_read_section].Curve_Info[index].power_delta;

							//Update the last value valid - used for marginal quantities
							prev_section_power = bid_curve_values[latency_read_section].Curve_Info[index].power_delta;
							prev_section_price = bid_curve_values[latency_read_section].Curve_Info[index].price;
						}//end 
					}

					//See if our output is non-zero.  If it isn't, we just turned ourselves off
					if (current_power_output == 0.0)
					{
						gen_state = GEN_OFF;
					}
					else	//Valid output
					{
						//See if our last price was the clearing price
						if (prev_section_price == auction_object->current_frame.clearing_price)
						{
							//See if it was a marginal seller condition
							if (auction_object->cleared_frame.clearing_type == CT_SELLER)
							{
								//Figure out our contribution (base on ratio of marginal - in case there is more than one of us) - subtract off the extra
								current_power_output -= prev_section_power*(1-auction_object->cleared_frame.marginal_frac);

								//Flag us as marginal
								is_marginal_gen = true;

							}//End marginal seller condition
						}//End last valid price was clearing price
					}
				}//End valid period
				else	//It isn't valid - turn us off
				{
					gen_state = GEN_OFF;
				}
			}//End standalone generator mode
			else	//Must be building mode
			{
				//See if the current bid period is valid
				if (bid_curve_values[latency_read_section].valid_bid_period == true)
				{
					//Parse the bidding curve
					for (index=0; index<bid_curve_values[latency_read_section].number_bid_curve_sections; index++)
					{
						//See if the price for this section was cleared
						if (bid_curve_values[latency_read_section].Curve_Info[index].price <= auction_object->current_frame.clearing_price)
						{
							current_power_output += bid_curve_values[latency_read_section].Curve_Info[index].power_delta;
						}//end 
					}

					//See if it is bigger than the current building load or not
					if (current_power_output > curr_building_load)	//If bigger, it's "marginal" to the building load
						current_power_output = curr_building_load;

					//See if our output is non-zero.  If it isn't, we just turned ourselves off
					if (current_power_output == 0.0)
					{
						gen_state = GEN_OFF;
					}
					//Default else - we'd be running
				}//End valid period
				else	//It isn't valid - turn us off
				{
					gen_state = GEN_OFF;
				}
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

		//Update the capacity
		gen_rating = bid_curve_values[latency_read_section].generator_capacity;

		//Update capacity factor - only done once per market clearing right now
		if (gen_rating != 0.0)
		{
			capacity_factor = current_power_output / gen_rating;
		}
		else	//If "no output" (easy way to turn a generator off), just set capacity factor to zero
		{
			capacity_factor = 0.0;
		}
	}//End update output

	//See which mode we are operating in
	if (gen_mode == BUILDING)
	{
		if (gen_state != GEN_OFF)	//Generator is starting/running - building goes away - generator assumed to be handling everything
		{
			current_power_output = 0.0;
		}
		else	//Generator off - implies full building load still there - negative to make it load (posted as generation)
		{
			current_power_output = -curr_building_load;
		}
	}
	//Defaulted else, must be standalone

	//Apply the power output - scale for powerflow
	/* TODO: this may need to be fixed as part of ticket #574 - if bidding units get fixed, these may need fixing as well */
	per_phase_power = current_power_output * unit_scaling_value / num_phases;
	per_phase_old_power = last_power_output * unit_scaling_value / num_phases;

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
	if (*next_clear != TS_NEVER)
	{
		if (t0 >= next_bid)
		{
			return -*next_clear;	//Return normal clearing time
		}
		else	//not updating to time above range, shoot for it
		{
			//Make sure it isn't TS_NEVER
			if (next_bid != TS_NEVER)
			{
				return -next_bid;
			}
			else
			{
				return -*next_clear;
			}
		}
	}//End not TS_NEVER
	else	//TS_NEVER return
		return *next_clear;
}

//Function to parse out the bid curve - handles allocations, if necessary
void generator_controller::parse_bid_curve(OBJECT *thisobj, TIMESTAMP t0)
{
	double plast, temp_double;
	int index, num_delims, num_entries, result_val;
	bool last_was_space;
	char *curr_ptr, *end_ptr;
	char temp_index;			//Index for temp_char_value
	char temp_char_value[33];	//Assumes no number in the bid curve will ever be over 32 characters
	FILE *FHandle;

	//See which mode we are in
	if (curve_file_mode == true)	//File import
	{
		//Set the very end character of the temp variable - terminator for various things
		temp_char_value[32]='\0';

		//Make sure the file field isn't empty - just in case it is being "played" in
		if (bidding_curve_file[0] == '\0')
		{
			GL_THROW("generator_controller:%s has an empty bid curve filename!",thisobj->name);
			/*  TROUBLESHOOT
			The bid_curve_file property of the generator_controller object is empty.  If the controller is reading the bid curve from
			a file, it can only operate in that mode for the full duration of the simulation.  Swapping modes mid-simulation is not allowed
			at this time.
			*/
		}

		//Try to open the file
		FHandle = fopen(bidding_curve_file,"rt");

		//See if it worked
		if (FHandle==NULL)
		{
			GL_THROW("generator_controller:%s has an invalid bid_curve_file specified!",thisobj->name);
			/*  TROUBLESHOOT
			The generator_controller was unable to open the CSV containing the bid curve information.  Please ensure
			the file is pointed to properly and is a valid file and try again.  If the error persists, please submit your
			code and bid curve file in a bug report via the trac website.
			*/
		}

		//Initialize counter
		num_delims=0;

		//Grab the first character
		result_val = fgetc(FHandle);

		//Parse the file once for giggles - find commas
		while (result_val > 0)
		{
			//See if the current character is a comma
			if (result_val==',')
			{
				num_delims++;
			}

			result_val = fgetc(FHandle);	//Get next character
		}

		//Make sure it wasn't zero (no delimiters!)
		if (num_delims==0)
		{
			//Close the file handle
			fclose(FHandle);

			//Toss the error
			GL_THROW("Bid curve file is either empty or improperly formatted in generator_controller:%s",thisobj->name);
			/*  TROUBLESHOOT
			While parsing the bid curve file, the generator_controller did not find any commas.  Either the file is empty, or is
			not formatted correctly.  Please ensure the file is correct and try again.  If the error persists, please submit your
			code and a bug report via the trac website.
			*/
		}

		//Set the number of entries - it is the same as the number of delimiters minus 1
		num_entries = num_delims - 1;
	}//end text file parsing
	else	//Text field parsing
	{
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
		num_delims = 0;
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
					num_delims++;			//increment counter
					last_was_space = true;	//Flag as a space
				}
			}
			else
				last_was_space = false;	//Flag that it wasn't a space

			index++;	//Increment the counter
		}

		//Make sure we didn't just reach the end

		//Make sure they occurred in pairs
		num_entries = (num_delims >> 1);
		if ((num_delims - (num_entries << 1)) != 1)	//Must be odd, since odd number of spaces needed to separate even number of numbers
		{
			GL_THROW("generator_controller:%s has an improperly formatted bid_curve",thisobj->name);
			/*  TROUBLESHOOT
			While parsing the bid_curve property of the generator object, an error was encountered.  Please ensure
			all values are valid numbers and that they occur in proper x-value and y-value pairs.
			*/
		}
	}//End text field initial parsing

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

	//Initialize building state to zero as well
	bid_curve_current.expected_building_load = 0.0;

	//Parse the curve dependent on mode
	if (curve_file_mode == true)	//File mode
	{
		//Go back to the beginning of the file
		fseek(FHandle, 0, SEEK_SET);

		//Set up the parsing values
		plast = 0.0;
		temp_index=0;

		//Fill the buffer to the first comma
		result_val = fgetc(FHandle);

		//Parse the file once for giggles - find commas
		while (result_val > 0)
		{
			//See if the current character is a comma
			if (result_val==',')
			{
				//Get us out of the while
				break;
			}
			else	//Not a comma, store it
			{
				temp_char_value[temp_index]=result_val;

				//Increment index
				temp_index++;

				//Check the index
				if (temp_index>31)
				{
					GL_THROW("Bidding curve entry was longer than 32 characters long!");
					/*  TROUBLESHOOT
					While parsing the bid curve entries in a file, a double value over
					32 characters long was found.  Please truncate this number and try again.
					*/
				}
			}

			result_val = fgetc(FHandle);	//Get next character
		}

		//Store a null at the last position - outside to catch invalids
		temp_char_value[temp_index]='\0';

		//Loop
		for (index=0; index<(num_entries+1); index++)
		{
			bid_curve_current.Curve_Info[index].lastbid_id = 0;	//Initialize

			//Store start
			bid_curve_current.Curve_Info[index].power_start = plast;

			//Put the pointer to the buffer
			curr_ptr = temp_char_value;

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
				bid_curve_current.generator_capacity = 0.0;

				//Deflag the update (if it actually was one, may have just been init)
				update_curve = false;

				return;	//Get us out of here
			}
			//Defaulted else - just continue on our merry way

			if (plast >= temp_double)	//Same or greater, implies wrong direction!
			{
				GL_THROW("generator_controller:%s does not have a monotonically increasing bid curve!",thisobj->name);
				//Defined below in text section
			}

			//Store p-end
			bid_curve_current.Curve_Info[index].power_stop = temp_double;

			//Copy in next values
			plast = temp_double;

			//Find an EOL or EOF
			temp_index=0;	//Reset index

			//Grab a value
			result_val = fgetc(FHandle);

			//Parse until an EOL or EOF is found
			while (result_val > 0)
			{
				//See if the current character is a comma
				if ((result_val=='\n') || (result_val==EOF))
				{
					//Get us out of the while
					break;
				}
				else	//Not an expected, store it
				{
					temp_char_value[temp_index]=result_val;

					//Increment index
					temp_index++;

					//Check the index
					if (temp_index>31)
					{
						GL_THROW("Bidding curve entry was longer than 32 characters long!");
						//Defined above
					}
				}

				result_val = fgetc(FHandle);	//Get next character
			}

			//Store a null at the last position - outside to catch invalids
			temp_char_value[temp_index]='\0';

			//Reset the pointer
			curr_ptr = temp_char_value;

			//Now get the price
			bid_curve_current.Curve_Info[index].price = strtod(curr_ptr,&end_ptr);

			//Populate the delta
			bid_curve_current.Curve_Info[index].power_delta = bid_curve_current.Curve_Info[index].power_stop - bid_curve_current.Curve_Info[index].power_start;

			//Look for a comma again
			temp_index=0;

			//Fill the buffer to the first comma
			result_val = fgetc(FHandle);

			//Parse the file once for giggles - find commas
			while (result_val > 0)
			{
				//See if the current character is a comma
				if (result_val==',')
				{
					//Get us out of the while
					break;
				}
				else	//Not a comma, store it
				{
					temp_char_value[temp_index]=result_val;

					//Increment index
					temp_index++;

					//Check the index
					if (temp_index>31)
					{
						GL_THROW("Bidding curve entry was longer than 32 characters long!");
						//Defined above
					}
				}

				result_val = fgetc(FHandle);	//Get next character
			}

			//Store a null at the last position - outside to catch invalids
			temp_char_value[temp_index]='\0';
		}//end parsing FOR

		//Close the file handle
		fclose(FHandle);
	}//End file parsing mode
	else	//Must be field mode
	{
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
				bid_curve_current.generator_capacity = 0.0;

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
	}//End field parsing

	//See if the last one was below the limit - if not, make one more
	if (bid_curve_current.Curve_Info[num_entries].power_stop < bid_gen_rating)
	{
		//Populate the "last" entry
		bid_curve_current.Curve_Info[(num_entries+1)].lastbid_id = 0;	//Init

		bid_curve_current.Curve_Info[(num_entries+1)].power_start = bid_curve_current.Curve_Info[num_entries].power_stop;	//Last point is our start
		bid_curve_current.Curve_Info[(num_entries+1)].power_stop = bid_gen_rating;	//End point is the max capacity
		bid_curve_current.Curve_Info[(num_entries+1)].power_delta = bid_curve_current.Curve_Info[(num_entries+1)].power_stop - bid_curve_current.Curve_Info[(num_entries+1)].power_start;	//Delta

		bid_curve_current.Curve_Info[(num_entries+1)].price = bid_curve_current.Curve_Info[num_entries].price;	//Price is just last price - if it is supposed to be different, they should have said so
	}
	else if (bid_curve_current.Curve_Info[num_entries].power_stop > bid_gen_rating)	//Bigger - oh noes!
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
	bid_curve_current.generator_capacity = bid_gen_rating;

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

	//Copy in the stored capacity value
	bid_curve_values[latency_write_section].generator_capacity = bid_curve_current.generator_capacity;
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
