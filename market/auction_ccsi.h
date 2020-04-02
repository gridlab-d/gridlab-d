/** $Id: auction_ccsi.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file auction_ccsi.h
	@addtogroup auction_ccsi
	@ingroup market

 @{
 **/

#ifndef _auction_ccsi_H
#define _auction_ccsi_H

#include <stdarg.h>
#include "gridlabd.h"
#include "market.h"
#include "bid_ccsi.h"
#include "curve_ccsi.h"

typedef struct s_statistic_ccsi {
	char32 statname;
	STATMODE stat_mode;
	STATTYPE stat_type;
	TIMESTAMP interval;
	double value;
	PROPERTY *prop;
	struct s_statistic_ccsi *next;
} STATISTIC_CCSI;

typedef struct s_market_frame_ccsi{
	int64 market_id;
	TIMESTAMP start_time;
	TIMESTAMP end_time;
	enumeration clearing_type;
	double clearing_price;
	double clearing_quantity;
	double marginal_quantity;
	double total_marginal_quantity;
	double marginal_frac;
	double seller_total_quantity;
	double buyer_total_unrep;
	double buyer_total_quantity;
	double cap_ref_unrep;
	double seller_min_price;
	struct s_market_frame_ccsi *next;
	double *statistics;
} MARKETFRAME_CCSI;

typedef enum {
	AM_NONE_CCSI=0,
	AM_DENY_CCSI=1,
	AM_PROB_CCSI=2
} MARGINMODE_CCSI;

class auction_ccsi : public gld_object {
public:
	bool verbose;
	bool use_future_mean_price;
	typedef enum {ST_ON=0, ST_OFF=1} STATISTICMODE;
	typedef enum {IP_FALSE=0, IP_TRUE=1} IGNOREPRICECAP;
	enumeration ignore_pricecap;
	typedef enum {CO_NORMAL=0, CO_EXTRA=1} CURVEOUTPUT;
	enumeration curve_log_info;
	enumeration margin_mode;
private:
	// functions
	int init_statistics();
	int update_statistics();
	int push_market_frame(TIMESTAMP t1);
	int check_next_market(TIMESTAMP t1);
	TIMESTAMP pop_market_frame(TIMESTAMP t1);
	void record_bid(char *from, double quantity, double real_price, BIDDERSTATE state);
	void record_curve(double, double);
	// variables
	curve_ccsi asks;			/**< demand curve */ 
	curve_ccsi offers;		/**< supply curve */
	int retry;
	BID_CCSI next;			/**< next clearing result */
protected:
public:
	int32 immediate;	// debug variable
	GL_STRING(char32,unit);		/**< unit of quantity (see unitfile.txt) */
	double dPeriod, dLatency;
	TIMESTAMP period;		/**< time period of auction_ccsi closing (s) */
	TIMESTAMP latency;		/**< delay after closing before unit commitment (s) */
	int64 market_id;	/**< id of market to clear */

	TIMESTAMP clearat;	/**< next clearing time */
	TIMESTAMP clearat2;	/**< next clearing time */
	TIMESTAMP checkat;	/**< next price check time */
	object network;		/**< comm network to use */
	double pricecap;	/**< maximum price allowed */
	double prices[168*60]; /**< price history */
	int64 count;		/**< used for sampled-hourly data */
	int16 lasthr, thishr;
	OBJECT *linkref;	/**< reference link object that contains the total load (used to determine unresponsive load when not all load bid) */

	// new stuff
	enumeration special_mode;
	enumeration clearing_type;
	enumeration statistic_mode;
	double fixed_price;
	double fixed_quantity;
	double future_mean_price; // right now the future mean price will be fed with a player
	
	double init_price;
	double init_stdev;
	OBJECT *capacity_reference_object;
	char32 capacity_reference_propname;
	PROPERTY *capacity_reference_property;
	double capacity_reference_bid_price; // the bid price the capacity reference bids
	double max_capacity_reference_bid_quantity; // the maximum bid quantity
	double capacity_reference_bid_quantity; // the bid quantity the capacity reference bids
	double fixed_uncontrollable_load;
	int32 warmup;
	int64 total_samples;
	double clearing_scalar;
	
	// statistics
	double *new_prices;
	double *statdata;
	unsigned int price_index;
	unsigned int64 price_count;
	uint32 history_count;
	// latency market frame queue
	MARKETFRAME_CCSI next_frame;
	MARKETFRAME_CCSI past_frame;
	MARKETFRAME_CCSI current_frame;
	MARKETFRAME_CCSI cleared_frame;
	MARKETFRAME_CCSI *framedata;
	MARKETFRAME_CCSI *back;
	unsigned int latency_front;
	unsigned int latency_back;
	uint32 latency_count;
	uint32 latency_stride;
	// statics
	static STATISTIC_CCSI *stats;
	static TIMESTAMP longest_statistic;
	static uint32 statistic_count;
	static int statistic_check;
public:
	double unresponsive_buy, unresponsive_sell;
	double responsive_buy, responsive_sell;
	double total_buy, total_sell;
	char256 trans_log;
	int64 trans_log_max;
	char256 curve_log;
	int64 curve_log_max;
private:
	FILE *trans_file;
	int64 trans_log_count;
	FILE *curve_file;
	int64 curve_log_count;
public:
	int submit(char *from, double quantity, double real_price, KEY key, BIDDERSTATE state, bool rebid, int64 mkt_id);
private:
	int submit_nolock(char *from, double quantity, double real_price, KEY key, BIDDERSTATE state, bool rebid, int64 mkt_id);
public:
	TIMESTAMP nextclear() const;
private:
	void clear_market(void);
public:
	/* required implementations */
	auction_ccsi(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static auction_ccsi *defaults;
};

EXPORT int64 get_market_for_time_ccsi(OBJECT *obj, TIMESTAMP ts);

#endif

/**@}*/
