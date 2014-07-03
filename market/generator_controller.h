/** $Id: generator_controller.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2011 Battelle Memorial Institute
	@file generator_controller.h
	@addtogroup generator_controller
	@ingroup market

 **/

#ifndef _GENCONTROLLER_H
#define _GENCONTROLLER_H

#include "auction.h"

class generator_controller : public gld_object 
{
public:
	static CLASS *oclass;
public:
	typedef struct s_curvedetails {	//Declare in here in case we ever need it externally
		double power_start;		/// Starting point of this linear section
		double power_stop;		/// Stopping point of this linear section
		double price;			/// Price at this flat portion
		double power_delta;		/// Power delta of this linear section (power_stop - power_start)
		KEY	   lastbid_id;		/// Last bid ID used for this section
	} CURVEDETAILS; /// Bid curve details

	typedef enum e_genstate {
		GEN_OFF = 0,		//Generator is offline
		GEN_ACTIVE = 1,		//Generator is active and producing
		GEN_STARTUP = 2,	//Generator would be starting up - used for bid tracking
	} GENERATOR_STATE;

	typedef enum e_amorttype {
		EXPONENTIAL = 0,	//Amortization is handled via exponential function
		LINEAR_COST = 1,	//Amortization is handled via a linear function reducing the shutdown payback
		LINEAR_BID = 2,		//Amortization is handled as a linearly decreasing component to pay back shutdown in minimum period
	} AMORTIZATION_TYPE;

	typedef enum e_genoptype {
		STANDALONE = 0,		//Generator operates as a standalone device
		BUILDING = 1,		//Generator is attached to a building load and "removes" the building if it wins
	} GENERATOR_MODE;

	int gen_state_num;		//Numeric indication of the generator state - mainly to make post-processing easier for some people

	typedef struct s_curveentry {
		CURVEDETAILS *Curve_Info;		//Pointer to particular bid curve of the time
		int	number_bid_curve_sections;	//Number of pieces in this particular curve
		TIMESTAMP bid_curve_parsed;		//Tracking variable - so know if need to update or not
		bool valid_bid_period;			//Variable to indicate if the generator was allowed to bid here, or not
		GENERATOR_STATE expected_state;	//Expected state of generator at this bid period - simple "post-clear" update to let next interval know how to bid
		double generator_capacity;		//Capacity value at the time of the bid - used to track varying capacity generators (e.g., wind)
		double expected_building_load;	//Expected building load for the time of the bid
	} CURVEENTRY;						//High-level structure for curves

	enumeration gen_state;			//Current state of the generator (used to influence bids)
	CURVEENTRY *bid_curve_values;		//Storage of bid curve for all latency periods
	CURVEENTRY bid_curve_current;		//Currently parsed bid-curve - used to populate the other intervals
	enumeration amort_type;		//Type of amortization utilized
	int number_bid_curve_sections;		//Number of sections in the current bid curve parse
	int number_latency_sections;		//Number of latency periods needed to store
	int latency_write_section;			//Current latency period to write the curve into
	int latency_read_section;			//Current latency period to read the curve out of (this is when the market is active)
	void parse_bid_curve(OBJECT *thisobj, TIMESTAMP t0);	//Function to parse bid curve
	void copy_bid_curve(OBJECT *thisobj);		//Function to copy currently parsed bid curve into latency storage

	int64 last_market_id;
	int64 *curr_market_id;
	int64 bid_market_id;

	generator_controller(MODULE *mod);
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);

	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

	OBJECT *market_object;
	TIMESTAMP market_period;
	TIMESTAMP market_latency;
	double gen_rating;				//Generator output rating
	double bid_gen_rating;			//Generator output rating for current bid-curve parsing
	double current_power_output;	//Current generator output level
	double last_power_output;		//Previous generator output level
	char1024 bidding_curve_txt;		//Text field for defining the bidding curve
	char1024 bidding_curve_file;	//Name of file to read bidding curve in from
	bool update_curve;				//Flag to force an update of the curve
	bool update_output;				//Flag to set an output update
	bool is_marginal_gen;			//Flag to indicate if the generator is operating on the margin
	double startup_cost;			//Cost associated with starting the generator
	double shutdown_cost;			//Cost associated with shutting down the generator
	double minimum_runtime_dbl;		//Minimum time for shutdown_cost to be accumulated over
	double minimum_downtime_dbl;	//Minimum time a generator must remain off before it can bid again
	double capacity_factor;			//Generator capacity factor
	double amortization_value;		//Exponent term for amortization calculations
	double bid_delay_dbl;			//Time before market closes to bid into the market

	enumeration gen_mode;		//Operating mode of this generator_controller
	double curr_building_load;		//Value of the building load the generators are attached to (if building mode)
	double bid_building_load;		//Value of the building load the generators are attached to (if building mode) for the period being bid
	double total_hours_year;		//Total hours the generator can run in a year
	double hours_run_this_year;		//Total hours the generator has run this year
	double scaling_factor;			//Value applied to license premium
	char1024 runtime_year_end;		//Date when the runtime year resets
	double license_premium;			//Current value of the license premium
	double hours_in_year;			//Number of hours assumed to be in the year
	double op_and_maint_cost;		//Operation and maintenance costs per runtime year
	double input_unit_base;			//Base multiple of [W] that bidding units are specified in - work around for market mismathc issue right now

private:
	auction *auction_object;
	complex *power_link;
	unsigned char phase_information;
	unsigned char num_phases;
	double price_cap_value;
	double shutdown_cost_remaining;
	double amortization_scaled;
	double shutdown_current_cost;

	double op_maint_cost_pwr;		//O&M cost translater to $/"market-base"/hr
	
	TIMESTAMP *next_clear;
	TIMESTAMP next_bid;
	TIMESTAMP market_bidding_time;
	TIMESTAMP gen_started;
	TIMESTAMP minimum_downtime;
	
	TIMESTAMP end_of_runtime_year;	//Date to reset annual runtime limits
	TIMESTAMP last_update_time;		//Time object was last updated - used for license premium hours track
	double seconds_in_year;			//Number of seconds in "runtime year" - for divisions
	bool runtime_hours_left;		//Flag to determine if the generator is allowed to run, or if it has exceeded its hourly runtime
	double unit_scaling_value;		//Value to scale all bids by for market

	int min_num_run_periods;
	int num_runs_completed;
	bool curve_file_mode;
	TIMESTAMP bid_delay;			//TIMESTAMPed value of bid_delay
	GENERATOR_STATE prev_bid_state;	//Used to track last state in sync
};

#endif // _GENCONTROLLER_H

// EOF
