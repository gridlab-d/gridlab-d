#include <stdarg.h>

#include "gridlabd.h"
#include "market.h"
#include "auction.h"
#include "stubauction.h"
#include "supervisory_control.h"

#ifndef _market_bid_h_
#define _market_bid_h_

typedef struct s_bid {
//struct s_bid {
	char *from;		/**< object from which bid was received */
	KEY bid_id;
	double quantity; 	/**< bid quantity (negative is sell, positive is buy */
	double price; 		/**< bid price */
	BIDDERSTATE state;	/**< state of bidder (unknown=0, off=1, on=2) */
} BID;
//};

typedef enum e_bid_type {
	BID_BUY,
	BID_SELL,
	BID_UNKNOWN
} BIDTYPE;

typedef struct s_bid_def {
	int64 raw;
	int64 market;
	int16 bid;
	BIDTYPE bid_type;
} BIDDEF;

typedef struct s_bid_info {

	double quantity;
	double price;
	int64 market_id;
	bool rebid;
	BIDDERSTATE state;
	KEY bid_id;
	bool bid_accepted;
}BIDINFO;
typedef struct s_bid BID;

/** Bid structure for markets */
EXPORT void submit_bid_state(char *from, char *to, const char *function_name, const char *function_class, void *bidding_buffer, size_t bid_len);

EXPORT int64 submit_bid(OBJECT *obj, OBJECT *from, double quantity, double price, KEY bid_id);

void translate_bid(BIDDEF &biddef, KEY key);
void write_bid(KEY &key, int64 market, int64 bid, BIDTYPE type);
#endif
