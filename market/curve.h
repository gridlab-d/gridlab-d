#include "gridlabd.h"
#include "market.h"
#include "bid.h"

#ifndef _curve_h_
#define _curve_h_

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
	void sort(bool reverse = false);
	BID *getbid(KEY n);
	inline double get_total() { return total;};
	inline double get_total_on() { return total_on;};
	inline double get_total_off() { return total_off;};
	double get_total_at(double price);
	double get_min();
	friend class auction;
};

#endif
