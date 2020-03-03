#include "bid.h"

EXPORT int64 submit_bid(OBJECT *obj, OBJECT *from, double quantity, double price, KEY bid_id)
{
	char biddername[64];
	if(obj->oclass == supervisory_control::oclass){
		supervisory_control *mkt = OBJECTDATA(obj,supervisory_control);
		return mkt->submit(from,quantity,price,bid_id);
	}
	else	{
		gl_error("%s submitted a bid to an object that is not an auction", gl_name(from,biddername,sizeof(biddername)));
		return -1;
	}
}
void submit_bid_state(char *from, char *to, const char *function_name, const char *function_class, void *bidding_buffer, size_t bid_len)//(char *obj, char *from, double quantity, double price, unsigned int is_on, KEY bid_id)
{
	char biddername[64];
	int rv;
	OBJECT *obj = NULL;
	BIDINFO *bidding_info = (BIDINFO *)bidding_buffer;
	if( strncmp(function_name, "submit_bid_state", 16)== 0){
		obj = gl_get_object(to);
		if( obj == NULL){
			gl_error("bid::submit_bid_state: No market object exists with given name %s.", to);
			bidding_info->bid_accepted = false;
		}

		if (obj->oclass==auction::oclass)
		{
			if(bidding_info->state == BS_UNKNOWN) {// not a stateful bid
				gl_verbose("%s submits stateless bid for Q:%.2f at P:%.4f", from,bidding_info->quantity,bidding_info->price);
				auction *mkt = OBJECTDATA(obj,auction);
				rv = mkt->submit(from,bidding_info->quantity,bidding_info->price,bidding_info->bid_id,bidding_info->state,bidding_info->rebid, bidding_info->market_id);
			} else {
				gl_verbose("%s submits stateful (%s) bid for Q:%.2f at P:%.4f", from,bidding_info->state,bidding_info->quantity,bidding_info->price);
				auction *mkt = OBJECTDATA(obj,auction);
				rv = mkt->submit(from,bidding_info->quantity,bidding_info->price,bidding_info->bid_id,bidding_info->state,bidding_info->rebid, bidding_info->market_id);
			}
			if(rv == 0) {
				bidding_info->bid_accepted = false;
			}
		} else if(obj->oclass == stubauction::oclass){
			gl_error("bid::submit_bid_state: market object is not an auction object.");
			bidding_info->bid_accepted = false;
	} else if(obj->oclass == supervisory_control::oclass){
		supervisory_control *mkt = OBJECTDATA(obj,supervisory_control);
		OBJECT *fObj = NULL;
		int is_on = 0;
		fObj = gl_get_object(from);
		if(fObj == NULL) {
			gl_error("bid::submit_bid_state: No market object exists with given name %s.", from);
			bidding_info->bid_accepted = false;
		}
		if(bidding_info->state == BS_ON){
			is_on = 1;
		}
		rv = mkt->submit(fObj,bidding_info->quantity,bidding_info->price,bidding_info->market_id,is_on);
		} else {
			gl_error("bid::submit_bid_state: market object is not an auction object.");
			bidding_info->bid_accepted = false;
		}
	} else {
		gl_error("bid::submit_bid_state: This function is not the intended function. %s was the intended function to call. Ignoring bid", function_name);
	}
}

void translate_bid(BIDDEF &biddef, KEY key){
	int64 mask = 0x8FFFFFFFFFFF0000LL;
	biddef.raw = key;
	biddef.bid = (int16)(key & 0x7FFF);
	biddef.market = (key & mask) >> 16;
	biddef.bid_type = (key & 0x8000) ? BID_BUY : BID_SELL;
}

void write_bid(KEY &key, int64 market, int64 bid, BIDTYPE type){
	int64 mask = 0x8FFFFFFFFFFF0000LL;
	key = ((market << 16) & mask) + (type == BID_BUY ? 0x8000 : 0) + (bid & 0x7FFFF);
}

// EOF
