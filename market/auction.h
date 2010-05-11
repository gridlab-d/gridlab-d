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

/** Bid structure for markets */
typedef enum {BS_UNKNOWN=0, BS_OFF=1, BS_ON=2} BIDDERSTATE;
typedef struct s_bid {
	OBJECT *from;		/**< object from which bid was received */
	double quantity; 	/**< bid quantity (negative is sell, positive is buy */
	double price; 		/**< bid price */
	BIDDERSTATE state;	/**< state of bidder (unknown=0, off=1, on=2) */
} BID;
typedef int KEY;

EXPORT int64 submit_bid(OBJECT *obj, OBJECT *from, double quantity, double price, KEY bid_id);
EXPORT int64 submit_bid_state(OBJECT *obj, OBJECT *from, double quantity, double price, unsigned int is_on, KEY bid_id);

/** Supply/Demand curve */
class curve {
private:
	int len;
	int n_bids;
	BID *bids;
	KEY *keys;
	double total;
	double total_on;
	double total_off;
private:
	static void sort(BID *list, KEY *keys, const int len, const bool reverse);
public:
	curve(void);
	~curve(void);
	inline unsigned int getcount() { return n_bids;};
	void clear(void);
	KEY submit(BID *bid);
	KEY resubmit(BID *bid, KEY key);
	void sort(bool reverse=false);
	BID *getbid(KEY n);
	inline double get_total() { return total;};
	inline double get_total_on() { return total_on;};
	inline double get_total_off() { return total_off;};
	friend class auction;
};

class auction {
public:
	bool verbose;
	typedef enum {AT_NONE=0, AT_SINGLE=1, AT_DOUBLE=2} AUCTIONTYPE;
private:
	double *Qload;		/**< total load (used to determine unresponsive load when not all load bid) */
	curve asks;			/**< demand curve */ 
	curve offers;		/**< supply curve */
	int retry;
protected:
public:
	AUCTIONTYPE type;	/**< auction type */
	char32 unit;		/**< unit of quantity (see unitfile.txt) */
	double period;		/**< time period of auction closing (s) */
	double latency;		/**< delay after closing before unit commitment (s) */
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
	int64 count;		/**< number of prices in history */
	int16 lasthr, thishr;
	OBJECT *linkref;	/**< reference link object that contains power_out (see Qload) */
	
public:
	KEY submit(OBJECT *from, double quantity, double price, KEY key=-1, BIDDERSTATE state=BS_UNKNOWN);
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

#endif

/**@}*/
