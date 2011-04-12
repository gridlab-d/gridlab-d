/** $Id: generator_controller.h 2011-02-03 18:00:00Z d3x593 $
	Copyright (C) 2011 Battelle Memorial Institute
	@file generator_controller.h
	@addtogroup generator_controller
	@ingroup market

 **/

#ifndef _GENCONTROLLER_H
#define _GENCONTROLLER_H

#include "auction.h"

class generator_controller
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

	int gen_state_num;		//Numeric indication of the generator state - mainly to make post-processing easier for some people

	typedef struct s_curveentry {
		CURVEDETAILS *Curve_Info;		//Pointer to particular bid curve of the time
		int	number_bid_curve_sections;	//Number of pieces in this particular curve
		TIMESTAMP bid_curve_parsed;		//Tracking variable - so know if need to update or not
		bool valid_bid_period;			//Variable to indicate if the generator was allowed to bid here, or not
		GENERATOR_STATE expected_state;	//Expected state of generator at this bid period - simple "post-clear" update to let next interval know how to bid
	} CURVEENTRY;						//High-level structure for curves

	GENERATOR_STATE gen_state;			//Current state of the generator (used to influence bids)
	CURVEENTRY *bid_curve_values;		//Storage of bid curve for all latency periods
	CURVEENTRY bid_curve_current;		//Currently parsed bid-curve - used to populate the other intervals
	int number_bid_curve_sections;		//Number of sections in the current bid curve parse
	int number_latency_sections;		//Number of latency periods needed to store
	int latency_write_section;			//Current latency period to write the curve into
	int latency_read_section;			//Current latency period to read the curve out of (this is when the market is active)
	void parse_bid_curve(OBJECT *thisobj, TIMESTAMP t0);	//Function to parse bid curve
	void copy_bid_curve(OBJECT *thisobj);		//Function to copy currently parsed bid curve into latency storage

	int64 last_market_id;
	int64 *curr_market_id;

	generator_controller(MODULE *mod);
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);

	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

	OBJECT *market_object;
	TIMESTAMP market_period;
	TIMESTAMP market_latency;
	double gen_rating;				//Generator output rating
	double current_power_output;	//Current generator output level
	double last_power_output;		//Previous generator output level
	char1024 bidding_curve_txt;
	bool update_curve;				//Flag to force an update of the curve
	bool update_output;				//Flag to set an output update
	double startup_cost;			//Cost associated with starting the generator
	double shutdown_cost;			//Cost associated with shutting down the generator
	double minimum_runtime_dbl;		//Minimum time for shutdown_cost to be accumulated over
	double minimum_downtime_dbl;	//Minimum time a generator must remain off before it can bid again
	double capacity_factor;			//Generator capacity factor
	double amortization_value;		//Exponent term for amortization calculations

private:
	auction *auction_object;
	complex *power_link;
	unsigned char phase_information;
	unsigned char num_phases;
	double price_cap_value;
	double shutdown_cost_remaining;
	double amortization_scaled;
	double shutdown_current_cost;
	TIMESTAMP next_clear;
	TIMESTAMP market_bidding_time;
	TIMESTAMP gen_started;
	TIMESTAMP minimum_downtime;
	int min_num_run_periods;
	int num_runs_completed;
};

#endif // _GENCONTROLLER_H

// EOF
