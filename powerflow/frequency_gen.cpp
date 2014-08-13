/** $Id: frequency_gen.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "frequency_gen.h"
#include "node.h"

//////////////////////////////////////////////////////////////////////////
// frequency_gen CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* frequency_gen::oclass = NULL;
CLASS* frequency_gen::pclass = NULL;

frequency_gen::frequency_gen(MODULE *mod) : powerflow_object(mod)
{
	if(oclass == NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod,"frequency_gen",sizeof(frequency_gen),PC_PRETOPDOWN|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class frequency_gen";
		else
			oclass->trl = TRL_DEMONSTRATED;

		if(gl_publish_variable(oclass,
			PT_enumeration,"Frequency_Mode",PADDR(FreqObjectMode),PT_DESCRIPTION,"Frequency object operations mode",
				PT_KEYWORD,"OFF", (enumeration)OFF,
				PT_KEYWORD,"AUTO", (enumeration)AUTO,
			PT_double,"Frequency[Hz]",PADDR(FrequencyValue),PT_DESCRIPTION,"Instantaneous frequency value",
			PT_double,"FreqChange[Hz/s]",PADDR(FrequencyChange),PT_DESCRIPTION,"Frequency change from last timestep",
			PT_double,"Deadband[Hz]",PADDR(Freq_Histr),PT_DESCRIPTION,"Frequency deadband of the governor",
			PT_double,"Tolerance[%]",PADDR(calc_tol_perc),PT_DESCRIPTION,"% threshold a power difference must be before it is cared about",
			PT_double,"M[pu*s]",PADDR(Mac_Inert),PT_DESCRIPTION,"Inertial constant of the system",
			PT_double,"D[%]",PADDR(D_Load),PT_DESCRIPTION,"Load-damping constant",
			PT_double,"Rated_power[W]",PADDR(system_base),PT_DESCRIPTION,"Rated power of system (base power)",
			PT_double,"Gen_power[W]",PADDR(PMech),PT_DESCRIPTION,"Mechanical power equivalent",
			PT_double,"Load_power[W]",PADDR(LoadPower),PT_DESCRIPTION,"Last sensed load value",
			PT_double,"Gov_delay[s]",PADDR(tgov_delay),PT_DESCRIPTION,"Governor delay time",
			PT_double,"Ramp_rate[W/s]",PADDR(ramp_rate),PT_DESCRIPTION,"Ramp ideal ramp rate",
			PT_double,"Low_Freq_OI[Hz]",PADDR(Freq_Low),PT_DESCRIPTION,"Low frequency setpoint for GFA devices",
			PT_double,"High_Freq_OI[Hz]",PADDR(Freq_High),PT_DESCRIPTION,"High frequency setpoint for GFA devices",
			PT_double,"avg24[Hz]",PADDR(day_average),PT_DESCRIPTION,"Average of last 24 hourly instantaneous measurements",
			PT_double,"std24[Hz]",PADDR(day_std),PT_DESCRIPTION,"Standard deviation of last 24 hourly instantaneous measurements",
			PT_double,"avg168[Hz]",PADDR(week_average),PT_DESCRIPTION,"Average of last 168 hourly instantaneous measurements",
			PT_double,"std168[Hz]",PADDR(week_std),PT_DESCRIPTION,"Standard deviation of last 168 hourly instantaneous measurements",
			PT_int32,"Num_Resp_Eqs",PADDR(Num_Resp_Eqs),PT_DESCRIPTION,"Total number of equations the response can contain",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int frequency_gen::isa(char *classname)
{
	return strcmp(classname,"frequency_gen")==0;
}

int frequency_gen::create(void)
{
	int result = powerflow_object::create();

	FreqObjectMode=AUTO;	//Generator object is on by default

	FrequencyValue = 60.0;
	FrequencyChange = 0.0;
	Freq_Histr = 0.01;	//0.01 Hz default Hysteresis

	Freq_Low = 59.95;
	Freq_High = 60.02;	//Defaults of BPA?

	ramp_rate = 10000000.0;	//10 MW/s default ramp rate

	Mac_Inert = 10.0;	//Arbitrary
	D_Load = 0.75;		//Arbitrary
	system_base = 100000000.0;	//Arbitrary 100 MW
	tgov_delay = 5.0;	//Arbitrary

	Num_Resp_Eqs = 20;	//20 should be good

	calc_tol_perc = 0.01;	//0.01% default threshold

	LoadPower = 0.0;
	PrevLoadPower = 0.0;
	PrevGenPower = 0.0;

	CurrGenCondition = MATCHED;
	NextGenCondition = MATCHED;
	Gen_Ramp_Direction = false;
	Change_Lockout = false;

	prev_time = 0;
	next_time = 0;
	curr_dt = 0;
	iter_passes=0;

	//Initialize statistical stuff
	for (int index=0; index<168; index++)
		stored_freq[index] = 0.0;

	curr_store_loc = 0;
	day_average = day_std = week_average = week_std = 0.0;

	return result;
}

int frequency_gen::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	char index;

	//Store nominal frequency value for calculations
	NominalFreq = FrequencyValue;

	if (!(gl_object_isa(obj->parent,"load","powerflow") | gl_object_isa(obj->parent,"node","powerflow") | gl_object_isa(obj->parent,"meter","powerflow")))
	{
		GL_THROW("Parented object needs to be a node of some sort.");
		/*  TROUBLESHOOT
		The frequency_gen object needs to be parented to a node, load, or meter in
		the powerflow module to properly work.  It is recommended you attach it to
		the SWING or top-node of the system.
		*/
	}

	//Allocate our equation "buffer"
	if (Num_Resp_Eqs==0)
		Num_Resp_Eqs=20;	//Not even erroring this.  If someone enters 0, I choose to ignore them, mainly because they are stupid

	CurrEquations = (ALGCOMPONENTS*)gl_malloc(Num_Resp_Eqs*sizeof(ALGCOMPONENTS));

	//Make sure it worked
	if (CurrEquations==NULL)
	{
		GL_THROW("Failed to allocate memory for equation structure in frequency_gen object.");
		/*  TROUBLESHOOT
		GridLAB-D failed to allocate the memory needed for the equation structure of the frequency_gen object.
		Please try again.  If the error persists, submit your code and a bug report using the trac website.
		*/
	}

	//Initialize the equation structure - first element is our persistent constant
	CurrEquations[0].coeffa = 0.0;
	CurrEquations[0].coeffa = 0.0;
	CurrEquations[0].coeffb = 0.0;
	CurrEquations[0].delay_time = 0.0;
	CurrEquations[0].contype = PERSCONST;
	CurrEquations[0].starttime = TS_NEVER;
	CurrEquations[0].endtime = TS_NEVER;
	CurrEquations[0].enteredtime = TS_NEVER;

	for (index=1; index<Num_Resp_Eqs; index++)
	{
		CurrEquations[index].coeffa = 0.0;
		CurrEquations[index].coeffb = 0.0;
		CurrEquations[index].delay_time = 0.0;
		CurrEquations[index].contype = EMPTY;
		CurrEquations[index].starttime = TS_NEVER;
		CurrEquations[index].endtime = TS_NEVER;
		CurrEquations[index].enteredtime = TS_NEVER;
	}

	//Calculate deadband area (frequency will still change minimally in this area, it just won't trigger a state change
	DB_Low = NominalFreq - Freq_Histr;
	DB_High = NominalFreq + Freq_Histr;

	//Convert low and high frequency to p.u. (for later calcs)
	F_Low_PU = Freq_Low/NominalFreq;
	F_High_PU = Freq_High/NominalFreq;

	//Figure out what the deadband is in power terms
	P_delta = Freq_Histr/NominalFreq*D_Load*system_base;

	//Check to make sure it isn't a zero
	if (D_Load==0.0)
	{
		GL_THROW("The load-damping constant for the frequency_gen object must be non-zero!");
		/*  TROUBLESHOOT
		The D_Load value must be a non-zero value to get proper operation of the frequency_gen object.
		If you want a system with no load damping, a new object needs to be created.
		*/
	}

	//Calculate constants for power/frequency transfer function
	K_val = 1.0/D_Load;
	T_val = Mac_Inert/D_Load;
	invT_val = -1.0/T_val;	//Not a direct inverse, but everywhere I use it is -1.0*, so it goes here
	AK_val = ramp_rate*K_val;

	//Calculate 8tau point (for changes)
	eight_tau_value = (int)(Mac_Inert/D_Load*8.0*TS_SECOND);

	//Do the same for 3tau
	three_tau_value = (int)(Mac_Inert/D_Load*3.0*TS_SECOND);

	//Tolerance value
	calc_tol = calc_tol_perc / 100.0;

	return powerflow_object::init(parent);
}

TIMESTAMP frequency_gen::presync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t1 = powerflow_object::presync(t0);
	int indexval, indexer;
	double tempval;

	if (prev_time == 0)	//Initialization run
	{
		prev_time = t0;
		next_time = TS_NEVER;
		track_time = t0;	//Store when we started, for tracking "hour" changes
	}

	if (FreqObjectMode==AUTO)
	{
		curr_dt = (t0-prev_time);	//Calculate time update

		if (curr_dt>0)	//A change occurred and we are in a good spot to use it
		{
			PrevLoadPower = LoadPower;	//Update power tracking variable
			PrevGenPower = PMech;		//Update generator output tracking variable
			prev_time = t0;				//Update tracking variable

			Change_Lockout = false;		//Clear the lock in case it was set - changes are allowed now

			next_time = pres_updatetime(t0, curr_dt);			//Perform frequency and time updates (initial next time estimate is this)

			CurrGenCondition = NextGenCondition;	//Transition the machine states

			//Statistic tracking/calculation
			if ((t0 - track_time) > 3600)	//Just see if we've gone over an hour, won't force it there since this is all very hand-wavy anyways
			{
				stored_freq[curr_store_loc] = FrequencyValue;	//Store the current frequency value

				//Re-initialize statistic values
				day_average = day_std = 0.0;

				//Now loop through and calculate mean values
				for (indexer=0; indexer<24; indexer++)
				{
					indexval = curr_store_loc-indexer;

					if (indexval<0)	//Loop roll-over handling
						indexval+=168;

					day_average += stored_freq[indexval];	//Accumulate
				}

				//Copy day into week
				week_average = day_average;
				
				//Now proceed with week
				for (indexer=24; indexer<168; indexer++)
				{
					indexval = curr_store_loc - indexer;

					if (indexval<0)	//Loop roll-over handling
						indexval+=168;

					week_average += stored_freq[indexval];	//Accumulate
				}

				//Turn them into averages
				day_average /=24.0;
				week_average /=168.0;

				//Now calculate standard deviation values
				for (indexer=0; indexer<24; indexer++)
				{
					indexval = curr_store_loc - indexer;

					if (indexval<0)	//Loop roll-over handling
						indexval+=168;

					tempval = (stored_freq[indexval] - day_average);	//Difference
					day_std += tempval*tempval;							//Squared
				}

				//Now do the same for the week
				for (indexer=0; indexer<24; indexer++)
				{
					indexval = curr_store_loc - indexer;

					if (indexval<0)	//Loop roll-over handling
						indexval+=168;

					tempval = (stored_freq[indexval] - week_average);	//Difference
					week_std += tempval*tempval;						//Squared
				}

				//Turn them into variances
				day_std /= 23.0;	//N-1 for unbiased
				week_std /= 167.0;

				//Now make standard deviations, as requested
				day_std = sqrt(day_std);
				week_std = sqrt(week_std);

				//Update current location pointer
				curr_store_loc++;

				//Roll-over handling
				if (curr_store_loc >= 168)
					curr_store_loc=0;

				track_time += 3600;	//Update the tracking interval to the next time
			}
		}

		//Update governor response variable, unless it is active
		if (Gov_Delay_Active == false)
		{
			Gov_Delay = t0 + (int64)(tgov_delay);
		}
		else	//It's active, check to see if next_time or Gov_Delay is first
		{
			if ((Gov_Delay < next_time) && (t0 != Gov_Delay) && (NextGenCondition==GOV_DELAY))	//Gov_Delay is less, but not current time (Postsync will change it)
				next_time = Gov_Delay;
			//Else is just next_time as it is
		}

		if (next_time > t1)
			return t1;
		else //must be equal or less
		{
			if (next_time == TS_NEVER)
				return TS_NEVER;
			else
				return -next_time;	//Negative, so it is a "suggestion"
		}
	}//End active mode
	else	//"Dumb" mode
		return TS_NEVER;
}

TIMESTAMP frequency_gen::postsync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	node *ParNode = OBJECTDATA(obj->parent,node);
	TIMESTAMP tret = powerflow_object::postsync(t0);
	TIMESTAMP tupdate, tupdate_b;
	complex temp_power;
	double newFVal, power_diff, tempFreq, tempDFreq, work_val, LoadLow, LoadHigh, time_work;
	int index;
	bool CollapsedDone;

	if (FreqObjectMode==AUTO)
	{
		//Update power calculation
		READLOCK_OBJECT(obj->parent);
		complex pc[] = {ParNode->current_inj[0],ParNode->current_inj[1],ParNode->current_inj[2]};
		READUNLOCK_OBJECT(obj->parent);

		temp_power = ParNode->voltage[0]*~pc[0];
		temp_power += ParNode->voltage[1]*~pc[1];
		temp_power += ParNode->voltage[2]*~pc[2];

		//Store this as the desired power
		LoadPower = temp_power.Re();

		if (PrevLoadPower==0.0)	//Initialization catch
		{
			PrevLoadPower=LoadPower;	//Update load tracking
			PMech=LoadPower;			//Current output equal to the load
			return tret;				//Just get us out of here, or try.  First iteration is always screwy.
		}

		iter_passes++;	//Update iteration passes
	
		//Power difference - load change (p.u.)
		power_diff = (PrevLoadPower-LoadPower)/system_base;

		//Calculate expected steady state final value
		newFVal = FrequencyValue+power_diff/D_Load*NominalFreq;

		//Transition through states to determine course of action
		switch (CurrGenCondition)
		{
			case MATCHED:
				{
					if (iter_passes!=1)	//Multiple pass - delete our scheduled events, just in case they no longer occur
					{
						RemoveScheduled(t0);		//remove items for the current time
						Gov_Delay_Active = false;	//Clear the governor delay flag
					}

					//if ((newFVal < DB_Low) || (newFVal > DB_High))	//Exceeded the band (or it will) - handle this
					LoadLow = PMech - P_delta;
					LoadHigh = PMech + P_delta;
					if ((LoadPower < LoadLow) || (LoadPower > LoadHigh))
					{
						NextGenCondition=GOV_DELAY;	//Transition to governor delay waiting
						Gov_Delay_Active = true;	//Activate the governor delay tracking

						//Schedule in the details of this change
							for (index=0; index<Num_Resp_Eqs; index++)	//Find an open spot
							{
								if (CurrEquations[index].contype==EMPTY)
								{
									CurrEquations[index].coeffa = power_diff*K_val;
									CurrEquations[index].coeffb = invT_val;
									CurrEquations[index].contype = STEPEXP;
									CurrEquations[index].starttime = t0;
									CurrEquations[index].enteredtime = t0;
									CurrEquations[index].endtime = t0 + eight_tau_value;	//Go out to 8 tau.  At that point, we'll just become a constant
									
									break;	//Out da loop, we found an empty entry
								}

								if (index==(Num_Resp_Eqs-1))
								{
									gl_warning("Failed to process frequency deviation at time %lld",t0);
									/*  TROUBLESHOOT
									The equations buffer for the frequency object must be full, so the response to a new power change could
									not be included.  This needs to be increased via Num_Resp_Eqs, or provoke less changes.  It may also be
									an indication that your attached generation is nearly at its limit.
									*/
								}
							}//End empty find

							//Guess when we'll cross critical frequency - theoretically only 1 thing active (this load), so manually calculate
							if ((newFVal <= Freq_Low) || (newFVal > Freq_High))
							{
								if (power_diff<0)	//Change up, so frequency will go down
								{
									tupdate = (int64)(-1.0*T_val*log(1.0 - ((F_Low_PU-1.0)/power_diff/K_val)));
								}
								else	//Must have been up
								{
									tupdate = (int64)(-1.0*T_val*log(1.0 - ((F_High_PU-1)/power_diff/K_val)));
								}

								//Apply offset
								tupdate += t0 + 1;	//In rounds down by default, so if we go +1, we have passed it (chances of hitting right on .0 are minimal)
							}
							else	//Not a factor
								tupdate = TS_NEVER;

							//Determine what we'll hit first - a frequency cross or the governor delay
							if (Gov_Delay < tupdate)
								next_time = Gov_Delay;	//Frequency first
							else
								next_time = tupdate;	//Governor first
					}
					else	//"Normal" operation.  Just apply this as a direct frequency change since it is small
					{
						//If the load has changed a slight bit, apply the change as a constant (update an existing constant, or create a new one)
						if (power_diff != 0.0)	//Any change
						{
							work_val = power_diff/D_Load;	//p.u. frequency change

							//Search for existing permanent constant
							for (index=0; index<Num_Resp_Eqs; index++)
							{
								if (CurrEquations[index].contype == PERSCONST)	//Found one (should be only one anyways)
								{
									CurrEquations[index].coeffa += work_val;
									
									break;	//Out da loop, we found an empty entry
								}
							}//end list traversion
						}//end load change

						NextGenCondition=MATCHED;	//Update state variable
						tupdate = TS_NEVER;			//Onward and upward!
						next_time = TS_NEVER;		//Flag this one as well
					}
					break;
				}
			case GOV_DELAY:
				{
					//See which iteration we are on - if above 1, remove our contributions
					if (iter_passes!=1)	//Multiple pass - delete our scheduled events, just in case they no longer occur
					{
						RemoveScheduled(t0);		//remove items for the current time
					}

					//See if any more load changes have occurred
					if ((power_diff>calc_tol) || (power_diff<-calc_tol))
					{

						//Compute the new response - Schedule in the details of this change
						for (index=0; index<Num_Resp_Eqs; index++)	//Find an open spot
						{
							if (CurrEquations[index].contype==EMPTY)
							{
								CurrEquations[index].coeffa = power_diff*K_val;
								CurrEquations[index].coeffb = invT_val;
								CurrEquations[index].contype = STEPEXP;
								CurrEquations[index].starttime = t0;
								CurrEquations[index].enteredtime = t0;
								CurrEquations[index].endtime = t0 + eight_tau_value;	//Go out to 8 tau.  At that point, we'll just become a constant
								
								break;	//Out da loop, we found an empty entry
							}

							if (index==(Num_Resp_Eqs-1))
							{
								gl_warning("Failed to process frequency deviation at time %lld",t0);
								//Define above
							}
						}//End empty find
					}//End diff != 0

					//Check the status of the delay
					if (t0 >= Gov_Delay)	//Delay is over!
					{
						NextGenCondition = GEN_RAMP;	//Transition us to the ramping stage

						//Apply the ramp to the scheduler, based on current conditions
						for (index=0; index<Num_Resp_Eqs; index++)	//Find an open spot
						{
							if (CurrEquations[index].contype==EMPTY)
							{
								//Find out how far we need to go
								power_diff = PMech - LoadPower;

								if (power_diff>0)	//PG > PL, so ramp down
								{
									work_val = -ramp_rate;
									Gen_Ramp_Direction = false;	//Flag it as so
									time_work = power_diff/ramp_rate; //Calculate the expected time to reach that
								}
								else				//Must be a ramp up
								{
									work_val = ramp_rate;
									Gen_Ramp_Direction = true;
									time_work = -power_diff/ramp_rate; //Calculate the expected time to reach that
								}

								//Integerize
								tupdate_b = (int64)(time_work)+1;

								//per-unit power difference
								work_val /= system_base;

								//Update equation addition
								CurrEquations[index].coeffa = work_val*K_val;
								CurrEquations[index].coeffb = invT_val;
								CurrEquations[index].delay_time = time_work;	//Ramp stop time
								CurrEquations[index].contype = RAMPEXP;
								CurrEquations[index].starttime = t0;
								CurrEquations[index].enteredtime = t0;
								CurrEquations[index].endtime = t0 + eight_tau_value + tupdate_b;	//Go out to 8 tau.  At that point, we'll just become a constant

								//Offset tupdate_b now (absolute vs relative time)
								tupdate_b += t0;
								
								//Determine our update time - when we may hit another frequency of interest
								tempFreq = FrequencyValue;
								tupdate = updatetime((t0+1),1,tempFreq,tempDFreq);	//See when we think we'll hit a FOI

								//Determine which time we want to aim for
								if (tupdate > tupdate_b)	//Ramp time
									next_time = tupdate_b;
								else
									next_time = tupdate;	//Frequency time

								break;	//Out da loop, we found an empty entry
							}

							if (index==(Num_Resp_Eqs-1))
							{
								gl_warning("Failed to process frequency deviation at time %lld",t0);
								//Define above
							}
						}//End empty find
					}//end gov delay over
					else
					{
						//Governor delay or a frequency cross should be all we'd be hitting here
							tempFreq = FrequencyValue;
							tupdate = updatetime((t0+1),1,tempFreq,tempDFreq);	//See when we think we'll hit a FOI

							//Now see which one to follow
							if (tupdate > Gov_Delay)	//Gov first
								next_time = Gov_Delay;
							else
								next_time = tupdate;	//FOI first

						NextGenCondition = GOV_DELAY;	//Make sure we stay here
						Gov_Delay_Active = true;		//This should already be true, but make sure
					}//Gov delay not over
					break;
				}
			case GEN_RAMP:	//Ramping period
				{
					//Apply the ramp update (happens regardless of whether load changed or not)
					if (Gen_Ramp_Direction)	//Ramping up
						PMech += ramp_rate*(double)(curr_dt);
					else	//Must be down
						PMech -= ramp_rate*(double)(curr_dt);

					//Flag that we are no longer in governor delay
					Gov_Delay_Active = false;

					//See if the load has changed in here
					if (((power_diff>calc_tol) || (power_diff<-calc_tol)) && (Change_Lockout==false))	//Load has changed
					{
						//See which iteration we are on - if above 1, remove our contributions
						if (iter_passes!=1)	//Multiple pass - delete our scheduled events, just in case they no longer occur
						{
							RemoveScheduled(t0);		//remove items for the current time
						}

						//Apply this change into the equation list
						for (index=0; index<Num_Resp_Eqs; index++)	//Find an open spot
						{
							if (CurrEquations[index].contype==EMPTY)
							{
								CurrEquations[index].coeffa = power_diff*K_val;
								CurrEquations[index].coeffb = invT_val;
								CurrEquations[index].contype = STEPEXP;
								CurrEquations[index].starttime = t0;
								CurrEquations[index].enteredtime = t0;
								CurrEquations[index].endtime = t0 + eight_tau_value;	//Go out to 8 tau.  At that point, we'll just become a constant
								
								break;	//Out da loop, we found an empty entry
							}

							if (index==(Num_Resp_Eqs-1))
							{
								gl_warning("Failed to process frequency deviation at time %lld",t0);
								//Define above
							}
						}//End empty find

						//Figure out the new difference
						work_val = (PMech - LoadPower)/system_base;

						//Determine which scenario we are in
						if (Gen_Ramp_Direction)	//Ramping up
						{
							if (work_val<0)	//Load is still greater, so we're OK directionally
							{
								//Determine how much our time needs to change
								time_work = (LoadPower - PrevLoadPower)/ramp_rate;
								tupdate_b = (int64)(time_work)+1;	//Find the time differential

								//Find a ramp that satisfies our needs
								for (index=0; index<Num_Resp_Eqs; index++)
								{
									if (CurrEquations[index].contype == RAMPEXP)	//Found one
									{
										//Find out when this one is set to stop
										tupdate = (int64)(CurrEquations[index].delay_time) + CurrEquations[index].starttime;

										//Make sure it hasn't
										if (tupdate > t0)	//We're OK
										{
											//Apply the time update
											CurrEquations[index].delay_time += time_work;

											//Update our end time to reflect this as well
											CurrEquations[index].endtime += tupdate_b;
											
											//Prevent us from making any more changes in this timestep that are load related
											Change_Lockout=true;

											//Get us out of this loop
											break;
										}
									}//end found one
								}//end equation list traversion

								if ((index==Num_Resp_Eqs) && (time_work > 0))	//It went through and did not find one, let's see if an empty spot exists
								{
									//Find a ramp that satisfies our needs
									for (index=0; index<Num_Resp_Eqs; index++)
									{
										if (CurrEquations[index].contype == EMPTY)	//Found one
										{
											//Add in a new ramp, we need to progress further
											//per-unit power difference
											work_val = ramp_rate/system_base;

											//Update equation addition
											CurrEquations[index].coeffa = work_val*K_val;
											CurrEquations[index].coeffb = invT_val;
											CurrEquations[index].delay_time = time_work;	//Ramp stop time
											CurrEquations[index].contype = RAMPEXP;
											CurrEquations[index].starttime = t0;
											CurrEquations[index].enteredtime = t0;
											CurrEquations[index].endtime = t0 + eight_tau_value + tupdate_b;	//Go out to 8 tau.  At that point, we'll just become a constant

											//Break out of the loop
											break;
										}//end found one

										if (index==(Num_Resp_Eqs-1))
										{
											gl_warning("Failed to process frequency deviation at time %lld",t0);
											//Define above
										}
									}//End empty list traversion
								}//End Need empty check
							}
							else	//Need to reverse >:|
							{
								//Traverse the list, find the existing ramp so we can terminate it
								for (index=0; index<Num_Resp_Eqs; index++)
								{
									if (CurrEquations[index].contype == RAMPEXP)	//Found one
									{
										//Find out when this one is set to stop
										tupdate = (int64)(CurrEquations[index].delay_time) + CurrEquations[index].starttime;

										//Make sure it hasn't
										if (tupdate > t0)	//We're OK
										{
											//Apply the time update - should stop us now
											CurrEquations[index].delay_time = (double)(t0 - CurrEquations[index].starttime);

											//Prevent us from making any more changes in this timestep that are load related
											Change_Lockout=true;

											//Get us out of this loop
											break;
										}
									}//end found one
								}//end equation list traversion

								//Now find an empty one and apply a new ramp
								for (index=0; index<Num_Resp_Eqs; index++)
								{
									if (CurrEquations[index].contype == EMPTY)	//Empty!
									{
										//Apply reversed direction
										Gen_Ramp_Direction=false;	//Now going downward

										//Determine time for new ramp
										time_work = (PMech - LoadPower)/ramp_rate;
										tupdate_b = (int64)(time_work)+1;

										//Fill in the values
											//per-unit power difference
											work_val = -ramp_rate/system_base;

											//Update equation addition
											CurrEquations[index].coeffa = work_val*K_val;
											CurrEquations[index].coeffb = invT_val;
											CurrEquations[index].delay_time = time_work;	//Ramp stop time
											CurrEquations[index].contype = RAMPEXP;
											CurrEquations[index].starttime = t0;
											CurrEquations[index].enteredtime = t0;
											CurrEquations[index].endtime = t0 + eight_tau_value + tupdate_b;	//Go out to 8 tau.  At that point, we'll just become a constant

										//Break out of the loop
										break;
									}//end found empty

									if (index==(Num_Resp_Eqs-1))
									{
										gl_warning("Failed to process frequency deviation at time %lld",t0);
										//Define above
									}
								}//end equation list traversion
							}//end reversal
						}//end ramp up
						else	//Must be ramping down
						{
							if (work_val>0)	//Generation is still greater than load, so we're OK directionally
							{
								//Determine how much our time needs to change
								time_work = (PrevLoadPower - LoadPower)/ramp_rate;
								tupdate_b = (int64)(time_work)+1;	//Find the time differential

								//Find a ramp that satisfies our needs
								for (index=0; index<Num_Resp_Eqs; index++)
								{
									if (CurrEquations[index].contype == RAMPEXP)	//Found one
									{
										//Find out when this one is set to stop
										tupdate = (int64)(CurrEquations[index].delay_time) + CurrEquations[index].starttime;

										//Make sure it hasn't
										if (tupdate > t0)	//We're OK
										{
											//Apply the time update
											CurrEquations[index].delay_time += time_work;

											//Update our end time as well
											CurrEquations[index].endtime += tupdate_b;

											//Prevent us from making any more changes in this timestep that are load related
											Change_Lockout=true;

											//Get us out of this loop
											break;
										}
									}//end found one
								}//end equation list traversion

								if ((index==Num_Resp_Eqs) && (time_work > 0))	//It went through and did not find one, let's see if an empty spot exists
								{
									//Find a ramp that satisfies our needs
									for (index=0; index<Num_Resp_Eqs; index++)
									{
										if (CurrEquations[index].contype == EMPTY)	//Found one
										{
											//Add in a new ramp, we need to progress further
											//per-unit power difference
											work_val = -ramp_rate/system_base;

											//Update equation addition
											CurrEquations[index].coeffa = work_val*K_val;
											CurrEquations[index].coeffb = invT_val;
											CurrEquations[index].delay_time = time_work;	//Ramp stop time
											CurrEquations[index].contype = RAMPEXP;
											CurrEquations[index].starttime = t0;
											CurrEquations[index].enteredtime = t0;
											CurrEquations[index].endtime = t0 + eight_tau_value + tupdate_b;	//Go out to 8 tau.  At that point, we'll just become a constant

											//Break out of the loop
											break;
										}//end found one

										if (index==(Num_Resp_Eqs-1))
										{
											gl_warning("Failed to process frequency deviation at time %lld",t0);
											//Define above
										}
									}//End empty list traversion
								}//End Need empty check
							}//end no reversal needed
							else //reversal needed >:|
							{

								//Traverse the list, find the existing ramp so we can terminate it
								for (index=0; index<Num_Resp_Eqs; index++)
								{
									if (CurrEquations[index].contype == RAMPEXP)	//Found one
									{
										//Find out when this one is set to stop
										tupdate = (int64)(CurrEquations[index].delay_time) + CurrEquations[index].starttime;

										//Make sure it hasn't
										if (tupdate > t0)	//We're OK
										{
											//Apply the time update - should stop us now
											CurrEquations[index].delay_time = (double)(t0 - CurrEquations[index].starttime);

											//Prevent us from making any more changes in this timestep that are load related
											Change_Lockout=true;

											//Get us out of this loop
											break;
										}
									}//end found one
								}//end equation list traversion

								//Now find an empty one and apply a new ramp
								for (index=0; index<Num_Resp_Eqs; index++)
								{
									if (CurrEquations[index].contype == EMPTY)	//Empty!
									{
										//Apply reversed direction
										Gen_Ramp_Direction=true;	//Now going upward

										//Determine time for new ramp
										time_work = (LoadPower - PMech)/ramp_rate;
										tupdate_b = (int64)(time_work)+1;

										//Fill in the values
											//per-unit power difference
											work_val = ramp_rate/system_base;

											//Update equation addition
											CurrEquations[index].coeffa = work_val*K_val;
											CurrEquations[index].coeffb = invT_val;
											CurrEquations[index].delay_time = time_work;	//Ramp stop time
											CurrEquations[index].contype = RAMPEXP;
											CurrEquations[index].starttime = t0;
											CurrEquations[index].enteredtime = t0;
											CurrEquations[index].endtime = t0 + eight_tau_value + tupdate_b;	//Go out to 8 tau.  At that point, we'll just become a constant

										//Break out of the loop
										break;
									}//end found empty

									if (index==(Num_Resp_Eqs-1))
									{
										gl_warning("Failed to process frequency deviation at time %lld",t0);
										//Define above
									}
								}//end equation list traversion
							}//end reversal
						}//end ramp down

						//Keep us here (since we've obviously not met the load now
						NextGenCondition = GEN_RAMP;
						Gov_Delay_Active = false;

						//Determine our update time now
						power_diff = PMech - LoadPower;

						//Still need more ramping - figure out how long we might need
						if (power_diff>0)
							tupdate_b = (int64)(power_diff/ramp_rate);
						else
							tupdate_b = (int64)(-power_diff/ramp_rate);

						tupdate_b += t0;	//Apply offset

						if (tupdate_b <= t0)	//Somehow stayed here
							tupdate_b = t0+1;	//Advance us by one, see what changes

						//Determine if a frequency value will be crossed before then
						tempFreq = FrequencyValue;
						tupdate = updatetime((t0+1),1,tempFreq,tempDFreq);	//See when we think we'll hit a FOI

						//Figure out which one we want
						if (tupdate_b > tupdate)	//FOI before done
							next_time = tupdate;
						else
							next_time = tupdate_b;	//Ramping next time of interest
					}//end load change
					else	//No load has changed
					{
						//See if we've reached our desired power level (+/- Freq error of final)
						LoadLow = PrevLoadPower - P_delta;
						LoadHigh = PrevLoadPower + P_delta;

						//See which way we were ramping
						if (Gen_Ramp_Direction)	//Going up?
						{
							//Make sure we haven't just overshot
							if ((PrevGenPower < LoadLow) && (PMech > LoadHigh) && ((LoadHigh-LoadLow) < ramp_rate))
								PMech = PrevLoadPower;
						}
						else	//Must be going down
						{
							//Make sure we haven't just overshot
							if ((PrevGenPower > LoadHigh) && (PMech < LoadLow) && ((LoadHigh-LoadLow) < ramp_rate))
								PMech = PrevLoadPower;
						}

						if ((PMech > LoadLow) && (PMech < LoadHigh) && (Change_Lockout==false))	//Should be in the range
						{
							NextGenCondition = MATCHED;	//We're done, go back to normal!
							Gov_Delay_Active = false;	//Make sure the governor delay is flagged as false
							Change_Lockout = true;		//Prevent us from going in here again, since it will mess everything up

							//Parse the list - anything that is in progress will end in the 8-tau time (should be done by then)
							//Step items.  Others are ignored for now (since they aren't really used)
							tupdate = t0 + eight_tau_value;	//Goal time

							//Figure out what the value of the frequency will be there
							tempFreq = FrequencyValue;
							tupdate_b = updatetime(tupdate,1,tempFreq,tempDFreq);

							//Determine the step size
							//work_val = (tempFreq - FrequencyValue) / NominalFreq;
							work_val = (NominalFreq - FrequencyValue) / NominalFreq;

							//Reflag reduction
							CollapsedDone = false;
							
							for (index=0; index<Num_Resp_Eqs; index++)
							{
								if ((CurrEquations[index].contype == STEPEXP) || (CurrEquations[index].contype == RAMPEXP))	//Something here
								{
									//Doesn't matter what it is, we're now making it a STEPEXP that represents all of these
									if (CollapsedDone == false)	//This one is now our friend
									{
										CurrEquations[index].contype = CSTEPEXP;
										CurrEquations[index].coeffa = work_val;
										CurrEquations[index].coeffb = invT_val;
										CurrEquations[index].delay_time = -work_val;	//This is actually the offset here
										CurrEquations[index].starttime = t0;
										CurrEquations[index].enteredtime = t0;
										CurrEquations[index].endtime = tupdate;	//Set to go away at 8tau

										CollapsedDone = true;	//Indicate we're done
									}
									else	//Already had one done
									{
										//Clear out the entries
										CurrEquations[index].coeffa = 0.0;
										CurrEquations[index].coeffb = 0.0;
										CurrEquations[index].delay_time = 0.0;
										CurrEquations[index].contype = EMPTY;
										CurrEquations[index].starttime = TS_NEVER;
										CurrEquations[index].enteredtime = TS_NEVER;
										CurrEquations[index].endtime = TS_NEVER;
									}
								}
							}

							//Transition the update
							next_time = tupdate;

						}//End PMech reached
						else
						{
							//Find out how far we still have to go
							power_diff = PMech - LoadPower;

							//Still need more ramping - figure out how long we might need
							if (power_diff>0)
								tupdate_b = (int64)(power_diff/ramp_rate);
							else
								tupdate_b = (int64)(-power_diff/ramp_rate);

							if (tupdate_b == 0)	//Integer round down, step next
								tupdate_b = 1;

							tupdate_b += t0;	//Apply offset

							//Determine if a frequency value will be crossed before then
							tempFreq = FrequencyValue;
							tupdate = updatetime((t0+1),1,tempFreq,tempDFreq);	//See when we think we'll hit a FOI

							//Figure out which one we want
							if (tupdate_b > tupdate)	//FOI before done
								next_time = tupdate;
							else
								next_time = tupdate_b;	//Ramping next time of interest

							//Set to stay where we are
							NextGenCondition = GEN_RAMP;
							Gov_Delay_Active = false;	//Make sure delay is still "de-flagged"
						}//End PMech not reached
					}//End no load change
					break;
				}
			default:
				{
					GL_THROW("The frequency object has entered an unknown state.");
					/*  TROUBLESHOOT
					The frequency generation object has entered an unknown state.  Submit
					your code and a bug report using the trac website.
					*/
				}
		}//end switch


		//Determine return time
		if (tret > next_time)
			tret = next_time;
		//Defaulted else, tret = tret

		if (tret==TS_NEVER)
			return TS_NEVER;
		else
			return -tret;	//Crazy negative time
	}//End auto mode
	else	//"Dumb" mode
		return TS_NEVER;
}

//Function to perform frequency updates and determine when (under current circumstances) a FOI will be crossed
TIMESTAMP frequency_gen::pres_updatetime(TIMESTAMP t0, TIMESTAMP dt_val)
{
	TIMESTAMP tnext;
	int index, indexy;
	double WorkVal, OWorkVal, final_value;
	double dt;
	
	//Loop through the contirbutors and remove any "expired" values
	for (index=0; index<Num_Resp_Eqs; index++)
	{
		if (CurrEquations[index].endtime < t0)	//Expired element
		{
			//Get its final value (it will get folded into a constant) - only step-based functions.  Others are special instances that should be handled themselves
			if (CurrEquations[index].contype == STEPEXP)	//Step exponential decay
			{
				dt = (double)(t0 - CurrEquations[index].starttime);		//Just take our value here.  If we're done, well we should be at our final value
				
				WorkVal = 1.0 - exp(CurrEquations[index].coeffb*dt);	//1-exp(b*t)

				final_value = CurrEquations[index].coeffa*WorkVal;		//Apply a*(1-exp(b*t))
			}
			else if (CurrEquations[index].contype == RAMPEXP)	//Stepped ramp
			{
				dt = (double)(t0 - CurrEquations[index].starttime);	//Take value here.  It is assumed that if we're done, should be at our final value

				WorkVal = exp(CurrEquations[index].coeffb*dt) - 1.0; //exp(b*t)-1
				WorkVal *= -1.0/CurrEquations[index].coeffb;		//-1/b(exp(b*t)-1)
				WorkVal += dt;										//t-1/b(exp(b*t)-1)

				//At final value, we're assumed to have passed the delayed step point as well
				OWorkVal = exp(CurrEquations[index].coeffb*(dt - CurrEquations[index].delay_time));			//exp((t-c)*b)
				OWorkVal /= CurrEquations[index].coeffb;													//1/b*exp((t-c)*b)
				OWorkVal -= 1.0/CurrEquations[index].coeffb;												//-1/b+1/b*exp((t-c)*b)
				OWorkVal += CurrEquations[index].delay_time-dt;										//c-t-1/b+1/b*exp((t-c)*b)

				WorkVal += OWorkVal;	//Add in u(t-c) term if time

				final_value = CurrEquations[index].coeffa*WorkVal;	//Apply a*(t-1/b(exp(b*t)-1)+(c-t-1/b+1/b*exp((t-c)*b))*u(t-c))
			}
			else
				final_value = 0.0;	//No change - an item is ending we don't care about

			//Find the persistant constant spot (should be first, but traverse just in case something changed)
			for (indexy=0; indexy<Num_Resp_Eqs; indexy++)
			{
				if (CurrEquations[indexy].contype==PERSCONST)	//Found one
				{
					CurrEquations[indexy].coeffa += final_value;	//Add in this final value
					
					break;	//Found it, done
				}

				if (index==(Num_Resp_Eqs-1))
				{
					GL_THROW("Failed to find persistent constant!");
					/* TROUBLESHOOT
					GridLAB-D failed to find the persistent constant for the frequency object.  This is a bug.
					Please submit your code and a bug report via the trac website.
					*/
				}
			}//End find persistent constant

			CurrEquations[index].coeffa = 0.0;				//Re-initialize it
			CurrEquations[index].coeffb = 0.0;
			CurrEquations[index].delay_time = 0.0;
			CurrEquations[index].contype = EMPTY;
			CurrEquations[index].starttime = TS_NEVER;
			CurrEquations[index].endtime = TS_NEVER;
			CurrEquations[index].enteredtime = TS_NEVER;
		}
	}

	//Calculate updated frequencies
	tnext = updatetime(t0, dt_val, FrequencyValue, FrequencyChange);

	return tnext;
}

//This function calculates updates for the frequency, frequency changes, and next time update
//No changes are interally written, so this can be used to predict future states
TIMESTAMP frequency_gen::updatetime(TIMESTAMP t0, TIMESTAMP dt_val, double &FreqValue, double &DeltaFreq)
{
	TIMESTAMP tnext;
	double dt;
	int index;
	double CalcedFreq;
	double WorkVal, OWorkVal;
	
	//Initialize the frequency to 1.0 (all frequency stuff is p.u.)
	CalcedFreq = 1.0;

	//Loop through the equation contributors and add in their contributions
	for (index=0; index<Num_Resp_Eqs; index++)
	{
		//Determine type of contributor
		switch (CurrEquations[index].contype)
		{
			case EMPTY:	//Do nothing
					break;
			case PERSCONST:	//Add in an always there constant
				{
					CalcedFreq += CurrEquations[index].coeffa;
					break;
				}
			case CONSTANT:
				{
					if ((t0 >= CurrEquations[index].starttime) && (t0 < CurrEquations[index].endtime))	//In valid range
					{
						CalcedFreq += CurrEquations[index].coeffa;	//Constant is just added in
					}
					break;
				}
			case SCALAR:
				{
					if ((t0 >= CurrEquations[index].starttime) && (t0 < CurrEquations[index].endtime))	//In valid range
					{
						dt = (double)(t0 - CurrEquations[index].starttime);	//Figure out how much time has passed

						CalcedFreq += CurrEquations[index].coeffa*dt;		//Apply linear scalar
					}
					break;
				}
			case EXPONENT:
				{
					if ((t0 >= CurrEquations[index].starttime) && (t0 < CurrEquations[index].endtime))	//In valid range
					{
						dt = (double)(t0 - CurrEquations[index].starttime);	//Figure out how much time has passed

						CalcedFreq += CurrEquations[index].coeffa*exp(CurrEquations[index].coeffb*dt);	//Apply a*exp(bt)
					}
					break;
				}
			case STEPEXP:
				{
					if ((t0 >= CurrEquations[index].starttime) && (t0 < CurrEquations[index].endtime))	//In valid range
					{
						dt = (double)(t0 - CurrEquations[index].starttime);	//Figure out how much time has passed
						
						WorkVal = 1.0 - exp(CurrEquations[index].coeffb*dt);	//1-exp(b*t)

						CalcedFreq += CurrEquations[index].coeffa*WorkVal;	//Apply a*(1-exp(b*t))
					}
					break;
				}
			case RAMPEXP:
				{
					if ((t0 >= CurrEquations[index].starttime) && (t0 < CurrEquations[index].endtime))	//In valid range
					{
						dt = (double)(t0 - CurrEquations[index].starttime);	//Figure out how much time has passed

						WorkVal = exp(CurrEquations[index].coeffb*dt) - 1.0; //exp(b*t)-1
						WorkVal *= -1.0/CurrEquations[index].coeffb;		//-1/b(exp(b*t)-1)
						WorkVal += dt;										//t-1/b(exp(b*t)-1)

						if (dt >= (CurrEquations[index].delay_time+1))	//u(t-c) - +1 due to rounding down (being 1 above works better than 1 below
						{
							OWorkVal = exp(CurrEquations[index].coeffb*(dt - CurrEquations[index].delay_time));			//exp((t-c)*b)
							OWorkVal /= CurrEquations[index].coeffb;													//1/b*exp((t-c)*b)
							OWorkVal -= 1.0/CurrEquations[index].coeffb;												//-1/b+1/b*exp((t-c)*b)
							OWorkVal += CurrEquations[index].delay_time-dt;										//c-t-1/b+1/b*exp((t-c)*b)
						}
						else
							OWorkVal = 0.0;

						WorkVal += OWorkVal;	//Add in u(t-c) term if time

						CalcedFreq += CurrEquations[index].coeffa*WorkVal;	//Apply a*(t-1/b(exp(b*t)-1)+(c-t-1/b+1/b*exp((t-c)*b))*u(t-c))
					}
					break;
				}
			case CSTEPEXP:
				{
					if ((t0 >= CurrEquations[index].starttime) && (t0 < CurrEquations[index].endtime))	//In valid range
					{
						dt = (double)(t0 - CurrEquations[index].starttime);	//Figure out how much time has passed
						
						WorkVal = 1.0 - exp(CurrEquations[index].coeffb*dt);	//1-exp(b*t)

						WorkVal *= CurrEquations[index].coeffa;	//a*(1-exp(b*t))

						CalcedFreq += WorkVal + CurrEquations[index].delay_time;		//Apply c+a*(1-exp(b*t))
					}
					break;
				}
			default:
				{
					GL_THROW("Unknown equation component encountered by frequency generation object.");
					/*  TROUBLESHOOT
					While parsing the contributing equations for the frequency generation object, an unknown type
					was encountered.  Please submit your code and a bug report using the trac website.
					*/
				}
		}//end switch
	}//end contribution loop

	//Un-p.u. it
	CalcedFreq *= NominalFreq;

	//Compute rate of change
	DeltaFreq=(CalcedFreq-FreqValue)/dt_val;

	//Update frequency
	FreqValue = CalcedFreq;

	//Perform rough guess at when frequency threshold will be crossed
	if (FreqValue < Freq_Low)	//Below lower threshold
	{
		if (DeltaFreq > 0)	//Ramping up
		{
			tnext = t0 + (int64)((Freq_Low - FreqValue)/DeltaFreq);	//Linear time to reach
		}
		else	//down or steady, so we aren't going towards it
		{
			tnext = TS_NEVER;
		}
	}
	else if ((FreqValue > Freq_Low) && (FreqValue < Freq_High))	//In band
	{
		if (DeltaFreq > 0)	//Ramping up
		{
			tnext = t0 + (int64)((Freq_High - FreqValue)/DeltaFreq);	//Linear time to reach
		}
		else if (DeltaFreq < 0)	//Ramping down
		{
			tnext = t0 + (int64)((Freq_Low - FreqValue)/DeltaFreq);	//Linear time to reach
		}
		else	//Must be 0
		{
			tnext = TS_NEVER;	//No movement = no know when update
		}
	}
	else if (FreqValue > Freq_High)	//Above upper threshold
	{
		if (DeltaFreq < 0)	//Ramping down
		{
			tnext = t0 + (int64)((Freq_High - FreqValue)/DeltaFreq);	//Linear time to reach
		}
		else	//up or steady, so we aren't going towards it
		{
			tnext = TS_NEVER;
		}
	}
	else	//We're somewhere else
		tnext = TS_NEVER;

	if (tnext==t0)	//Must have been a down round, pop it by 1
		tnext=t0+1;

	return tnext;
}

// Debugging function
// Left here for future use - dumps the contents of the
// equation space to testout.txt so you can see what
// is filling it.
void frequency_gen::DumpScheduler(void)
{
	FILE *FP = fopen("dumpout.txt","w");

	for (int index=0; index<Num_Resp_Eqs; index++)
	{
		if (CurrEquations[index].contype == EMPTY)
			fprintf(FP,"%d - Empty - %lld - %lld - %lld - %f - %f ++ %f\n",index,CurrEquations[index].enteredtime,CurrEquations[index].starttime,CurrEquations[index].endtime,CurrEquations[index].coeffa,CurrEquations[index].coeffb,CurrEquations[index].delay_time);
		else if (CurrEquations[index].contype == CONSTANT)
			fprintf(FP,"%d - Constant - %lld - %lld - %lld - %f - %f ++ %f\n",index,CurrEquations[index].enteredtime,CurrEquations[index].starttime,CurrEquations[index].endtime,CurrEquations[index].coeffa,CurrEquations[index].coeffb,CurrEquations[index].delay_time);
		else if (CurrEquations[index].contype == SCALAR)
			fprintf(FP,"%d - Scalar - %lld - %lld - %lld - %f - %f ++ %f\n",index,CurrEquations[index].enteredtime,CurrEquations[index].starttime,CurrEquations[index].endtime,CurrEquations[index].coeffa,CurrEquations[index].coeffb,CurrEquations[index].delay_time);
		else if (CurrEquations[index].contype == EXPONENT)
			fprintf(FP,"%d - Exponent - %lld - %lld - %lld - %f - %f ++ %f\n",index,CurrEquations[index].enteredtime,CurrEquations[index].starttime,CurrEquations[index].endtime,CurrEquations[index].coeffa,CurrEquations[index].coeffb,CurrEquations[index].delay_time);
		else if (CurrEquations[index].contype == STEPEXP)
			fprintf(FP,"%d - Step Exp - %lld - %lld - %lld - %f - %f ++ %f\n",index,CurrEquations[index].enteredtime,CurrEquations[index].starttime,CurrEquations[index].endtime,CurrEquations[index].coeffa,CurrEquations[index].coeffb,CurrEquations[index].delay_time);
		else if (CurrEquations[index].contype == RAMPEXP)
			fprintf(FP,"%d - Ramp Exp - %lld - %lld - %lld - %f - %f ++ %f\n",index,CurrEquations[index].enteredtime,CurrEquations[index].starttime,CurrEquations[index].endtime,CurrEquations[index].coeffa,CurrEquations[index].coeffb,CurrEquations[index].delay_time);
		else if (CurrEquations[index].contype == PERSCONST)
			fprintf(FP,"%d - Persistent Constant - %lld - %lld - %lld - %f - %f ++ %f\n",index,CurrEquations[index].enteredtime,CurrEquations[index].starttime,CurrEquations[index].endtime,CurrEquations[index].coeffa,CurrEquations[index].coeffb,CurrEquations[index].delay_time);
		else if (CurrEquations[index].contype == CSTEPEXP)
			fprintf(FP,"%d - Const Step Exp - %lld - %lld - %lld - %f - %f ++ %f\n",index,CurrEquations[index].enteredtime,CurrEquations[index].starttime,CurrEquations[index].endtime,CurrEquations[index].coeffa,CurrEquations[index].coeffb,CurrEquations[index].delay_time);
		else
			fprintf(FP,"%d - ???\n",index);
	}
	fclose(FP);

}

//Function to remove t0-added items from the contribution scheduler
void frequency_gen::RemoveScheduled(TIMESTAMP t0)
{
	int index;

	//Loop through the scheduler/contributor.  Clear anything added at t0
	for (index=0; index<Num_Resp_Eqs; index++)
	{
		if (CurrEquations[index].enteredtime==t0)
		{
			CurrEquations[index].coeffa = 0.0;
			CurrEquations[index].coeffb = 0.0;
			CurrEquations[index].contype = EMPTY;
			CurrEquations[index].starttime = TS_NEVER;
			CurrEquations[index].endtime = TS_NEVER;
			CurrEquations[index].enteredtime = TS_NEVER;
		}
	}
}

//Commit timestep - after all iterations are done
EXPORT TIMESTAMP commit_frequency_gen(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	frequency_gen *fgen = OBJECTDATA(obj,frequency_gen);
	try {
		fgen->iter_passes=0;	//Reset counter
		return TS_NEVER;
	}
	catch (const char *msg)
	{
		gl_error("%s (frequency_gen:%d): %s", fgen->get_name(), fgen->get_id(), msg);
		return 0; 
	}

}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: frequency_gen
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_frequency_gen(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(frequency_gen::oclass);
		if (*obj!=NULL)
		{
			frequency_gen *my = OBJECTDATA(*obj,frequency_gen);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(frequency_gen);
}

EXPORT int init_frequency_gen(OBJECT *obj, OBJECT *parent)
{
	try {
		frequency_gen *my = OBJECTDATA(obj,frequency_gen);
		return my->init(parent);
	}
	INIT_CATCHALL(frequency_gen);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_frequency_gen(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		frequency_gen *pObj = OBJECTDATA(obj,frequency_gen);
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	}
	SYNC_CATCHALL(frequency_gen);
}

EXPORT int isa_frequency_gen(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,frequency_gen)->isa(classname);
}

/**@}**/
