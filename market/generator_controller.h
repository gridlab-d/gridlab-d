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

	typedef struct s_curveentry {
		CURVEDETAILS *Curve_Info;		//Pointer to particular bid curve of the time
		int	number_bid_curve_sections;	//Number of pieces in this particular curve
		TIMESTAMP bid_curve_parsed;		//Tracking variable - so know if need to update or not
	} CURVEENTRY;						//High-level structure for curves

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
	double gen_rating;
	double current_power_output;
	double last_power_output;
	char1024 bidding_curve_txt;
	bool update_curve;
	bool update_output;

private:
	auction *auction_object;
	complex *power_link;
	unsigned char phase_information;
	unsigned char num_phases;
	double price_cap_value;
	TIMESTAMP next_clear;
};

#endif // _GENCONTROLLER_H

// EOF
