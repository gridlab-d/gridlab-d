#include "gridlabd.h"
#include "market.h"
#include "bid_ccsi.h"

#ifndef _curve_ccsi_h_
#define _curve_ccsi_h_

/** Supply/Demand curve */
class curve_ccsi {
private:
	int len;
	int n_bids;
	BID_CCSI *bids;
	KEY *keys;
	KEY *bid_ids;
	double total;
	double total_on;
	double total_off;
private:
	static void sort(BID_CCSI *list, KEY *keys, const int len, const bool reverse);
public:
	curve_ccsi(void);
	~curve_ccsi(void);
	inline unsigned int getcount() { return n_bids;};
	void clear(void);
	KEY submit(BID_CCSI *bid);
	KEY resubmit(BID_CCSI *bid);
	int remove_bid(KEY bid_id);
	void sort(bool reverse = false);
	BID_CCSI *getbid(KEY n);
	inline double get_total() { return total;};
	inline double get_total_on() { return total_on;};
	inline double get_total_off() { return total_off;};
	double get_total_at(double price);
	double get_min();
	friend class auction_ccsi;
};

#endif
