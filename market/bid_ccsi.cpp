#include "bid_ccsi.h"

EXPORT void submit_bid_state_ccsi(char *from, char *to, char *function_name, char *function_class, void *bidding_buffer, size_t bid_len)//(char *obj, char *from, double quantity, double price, unsigned int is_on, KEY bid_id)
{
	char biddername[64];
	int rv;
	OBJECT *obj = NULL;
	BIDINFO_CCSI *bidding_info = (BIDINFO_CCSI *)bidding_buffer;
	if( strncmp(function_name, "submit_bid_state_ccsi", 16)== 0){
		obj = gl_get_object(to);
		if( obj == NULL){
			gl_error("bid::submit_bid_state_ccsi: No auction object exists with given name %s.", to);
			bidding_info->bid_accepted = false;
		}

		if (obj->oclass==auction_ccsi::oclass)
		{
			if(bidding_info->state == BS_UNKNOWN) {// not a stateful bid
				gl_verbose("%s submits stateless bid for Q:%.2f at P:%.4f", from,bidding_info->quantity,bidding_info->price);
				auction_ccsi *mkt = OBJECTDATA(obj,auction_ccsi);
				rv = mkt->submit(from,bidding_info->quantity,bidding_info->price,bidding_info->bid_id,bidding_info->state,bidding_info->rebid, bidding_info->market_id);
			} else {
				gl_verbose("%s submits stateful (%s) bid for Q:%.2f at P:%.4f", from,bidding_info->state,bidding_info->quantity,bidding_info->price);
				auction_ccsi *mkt = OBJECTDATA(obj,auction_ccsi);
				rv = mkt->submit(from,bidding_info->quantity,bidding_info->price,bidding_info->bid_id,bidding_info->state,bidding_info->rebid, bidding_info->market_id);
			}
			if(rv == 0) {
				bidding_info->bid_accepted = false;
			}
		} else if(obj->oclass == stubauction::oclass){
			gl_error("bid::submit_bid_state_ccsi: market object is not an auction object.");
			bidding_info->bid_accepted = false;
		} else {
			gl_error("bid::submit_bid_state_ccsi: market object is not an auction object.");
			bidding_info->bid_accepted = false;
		}
	} else {
		gl_error("bid::submit_bid_state_ccsi: This function is not the intended function. %s was the intended function to call. Ignoring bid", function_name);
	}
}

void translate_bid(BIDDEF_CCSI &biddef, KEY key){
	int64 mask = 0x8FFFFFFFFFFF0000LL;
	biddef.raw = key;
	biddef.bid = (int16)(key & 0x7FFF);
	biddef.market = (key & mask) >> 16;
	biddef.bid_type = (key & 0x8000) ? BID_CCSI_BUY : BID_CCSI_SELL;
}

void write_bid(KEY &key, int64 market, int64 bid, BIDTYPE_CCSI type){
	int64 mask = 0x8FFFFFFFFFFF0000LL;
	key = ((market << 16) & mask) + (type == BID_CCSI_BUY ? 0x8000 : 0) + (bid & 0x7FFFF);
}

// EOF
