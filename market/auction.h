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
typedef struct s_bid {
	OBJECT *from;		/**< object from which bid was received */
	double quantity; 	/**< bid quantity (negative is sell, positive is buy */
	double price; 		/**< bid price */
} BID;
typedef int KEY;

/** Supply/Demand curve */
class curve {
private:
	int len;
	int n_bids;
	BID *bids;
	KEY *keys;
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
};

class auction {
public:
	typedef enum {AT_NONE=0, AT_SINGLE=1, AT_DOUBLE=2} AUCTIONTYPE;
private:
	curve asks;			/**< demand curve */ 
	curve offers;		/**< supply curve */
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

	double avg24;		/**< daily average of price */
	double std24;		/**< daily stdev of price */
	double avg168;		/**< weekly average of price */
	double std168;		/**< weekly stdev of price */
	double prices[168]; /**< price history */
	unsigned char count;/**< number of prices in history */
	
public:
	KEY submit(OBJECT *from, double quantity, double price, KEY key=-1);
	TIMESTAMP nextclear() const;
	void clear_market(void);
public:
	/* required implementations */
	auction(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static auction *defaults;
};

#endif

/**@}*/
