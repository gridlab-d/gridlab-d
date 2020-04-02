#include <stdarg.h>
#include "gridlabd.h"
#include "market.h"
#include "auction_ccsi.h"
#include "stubauction.h"

#ifndef _market_bid_ccsi_h_
#define _market_bid_ccsi_h_

typedef struct s_bid_ccsi {
//struct s_bid {
	char *from;		/**< object from which bid was received */
	KEY bid_id;
	double quantity; 	/**< bid quantity (negative is sell, positive is buy */
	double price; 		/**< bid price */
	BIDDERSTATE state;	/**< state of bidder (unknown=0, off=1, on=2) */
} BID_CCSI;
//};

typedef enum e_bid_type_ccsi {
	BID_CCSI_BUY,
	BID_CCSI_SELL,
	BID_CCSI_UNKNOWN
} BIDTYPE_CCSI;

typedef struct s_bid_def_ccsi {
	int64 raw;
	int64 market;
	int16 bid;
	BIDTYPE_CCSI bid_type;
} BIDDEF_CCSI;

typedef struct s_bid_info_ccsi {
	double quantity;
	double price;
	int64 market_id;
	bool rebid;
	BIDDERSTATE state;
	KEY bid_id;
	bool bid_accepted;
}BIDINFO_CCSI;
typedef struct s_bid_ccsi BID_CCSI;

/** Bid structure for markets */
EXPORT void submit_bid_state_ccsi(char *from, char *to, char *function_name, char *function_class, void *bidding_buffer, size_t bid_len);

void translate_bid(BIDDEF_CCSI &biddef, KEY key);
void write_bid(KEY &key, int64 market, int64 bid, BIDTYPE_CCSI type);
#endif
