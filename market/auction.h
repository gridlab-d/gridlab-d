/** $Id: auction.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file auction.h
	@addtogroup auction
	@ingroup market

 @{
 **/

#ifndef _auction_H
#define _auction_H

#include <stdarg.h>
#include "gridlabd.h"
#include "market.h"
#include "bid.h"
#include "curve.h"

typedef struct s_statistic {
	char32 statname;
	STATMODE stat_mode;
	STATTYPE stat_type;
	TIMESTAMP interval;
	double value;
	PROPERTY *prop;
	struct s_statistic *next;
} STATISTIC;

typedef struct s_market_frame{
	int64 market_id;
	TIMESTAMP start_time;
	TIMESTAMP end_time;
	CLEARINGTYPE clearing_type;
	double clearing_price;
	double clearing_quantity;
	double marginal_quantity;
	double total_marginal_quantity;
	double marginal_frac;
	double seller_total_quantity;
	double buyer_total_quantity;
	double seller_min_price;
	struct s_market_frame *next;
	double *statistics;
} MARKETFRAME;

class auction {
public:
	bool verbose;
	typedef enum {AT_NONE=0, AT_SINGLE=1, AT_DOUBLE=2} AUCTIONTYPE;
	typedef enum {ST_ON=0, ST_OFF=1} STATISTICMODE;
private:
	// functions
	int init_statistics();
	int update_statistics();
	int push_market_frame(TIMESTAMP t1);
	int check_next_market(TIMESTAMP t1);
	TIMESTAMP pop_market_frame(TIMESTAMP t1);
	// variables
	double *Qload;		/**< total load (used to determine unresponsive load when not all load bid) */
	curve asks;			/**< demand curve */ 
	curve offers;		/**< supply curve */
	int retry;
protected:
public:
	int32 immediate;	// debug variable
	AUCTIONTYPE type;	/**< auction type */
	char32 unit;		/**< unit of quantity (see unitfile.txt) */
	double dPeriod, dLatency;
	TIMESTAMP period;		/**< time period of auction closing (s) */
	TIMESTAMP latency;		/**< delay after closing before unit commitment (s) */
	int64 market_id;	/**< id of market to clear */
	BID last;			/**< last clearing result */
	BID next;			/**< next clearing result */
	TIMESTAMP clearat;	/**< next clearing time */
	TIMESTAMP checkat;	/**< next price check time */
	object network;		/**< comm network to use */
	double pricecap;	/**< maximum price allowed */

	double avg24;		/**< daily average of price */
	double std24;		/**< daily stdev of price */
	double avg72;
	double std72;
	double avg168;		/**< weekly average of price */
	double std168;		/**< weekly stdev of price */
	double prices[168*60]; /**< price history */
	int64 count;		/**< used for sampled-hourly data */
	int16 lasthr, thishr;
	OBJECT *linkref;	/**< reference link object that contains power_out (see Qload) */

	// new stuff
	SPECIALMODE special_mode;
	CLEARINGTYPE clearing_type;
	STATISTICMODE statistic_mode;
	double fixed_price;
	double fixed_quantity;
	double init_price;
	double init_stdev;
	OBJECT *capacity_reference_object;
	char32 capacity_reference_propname;
	PROPERTY *capacity_reference_property;
	double capacity_reference_bid_price; // the bid price the capacity reference bids
	double max_capacity_reference_bid_quantity; // the maximum bid quantity
	double capacity_reference_bid_quantity; // the bid quantity the capacity reference bids
	int32 warmup;
	int64 total_samples;
	double clearing_scalar;
	
	// statistics
	double *new_prices;
	double *statdata;
	unsigned int price_index;
	unsigned int64 price_count;
	size_t history_count;
	// latency market frame queue
	MARKETFRAME next_frame;
	MARKETFRAME past_frame;
	MARKETFRAME current_frame;
	MARKETFRAME cleared_frame;
	MARKETFRAME *framedata;
	MARKETFRAME *back;
	unsigned int latency_front;
	unsigned int latency_back;
	size_t latency_count;
	size_t latency_stride;
	// statics
	static STATISTIC *stats;
	static TIMESTAMP longest_statistic;
	static size_t statistic_count;
	static int statistic_check;
public:
	double unresponsive_buy, unresponsive_sell;
	double responsive_buy, responsive_sell;
	double total_buy, total_sell;
public:
	KEY submit(OBJECT *from, double quantity, double real_price, KEY key=-1, BIDDERSTATE state=BS_UNKNOWN);
	TIMESTAMP nextclear() const;
	void clear_market(void);
public:
	/* required implementations */
	auction(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static auction *defaults;
};

EXPORT int64 get_market_for_time(OBJECT *obj, TIMESTAMP ts);

#endif

/**@}*/
