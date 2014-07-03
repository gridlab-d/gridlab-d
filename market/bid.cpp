#include "bid.h"

EXPORT int64 submit_bid(OBJECT *obj, OBJECT *from, double quantity, double price, KEY bid_id)
{
	char biddername[64];
	if (obj->oclass==auction::oclass)
	{
		gl_verbose("%s submits stateless bid for %.0f at %.0f", gl_name(from,biddername,sizeof(biddername)),quantity,price);
		auction *mkt = OBJECTDATA(obj,auction);
		return mkt->submit(from,quantity,price,bid_id);
	}
	else if(obj->oclass == stubauction::oclass){
		return -1;
	} else	{
		gl_error("%s submitted a bid to an object that is not an auction", gl_name(from,biddername,sizeof(biddername)));
		return -1;
	}
}

EXPORT int64 submit_bid_state(OBJECT *obj, OBJECT *from, double quantity, double price, unsigned int is_on, KEY bid_id)
{
	char biddername[64];
	if (obj->oclass==auction::oclass)
	{
		gl_verbose("%s submits stateful (%s) bid for Q:%.2f at P:%.4f", gl_name(from,biddername,sizeof(biddername)),is_on?"on":"off",quantity,price);
		auction *mkt = OBJECTDATA(obj,auction);
		return mkt->submit(from,quantity,price,bid_id,is_on?BS_ON:BS_OFF);
	} else if(obj->oclass == stubauction::oclass){
		return -1;
	} else {
		gl_error("%s submitted a bid to an object that is not an auction", gl_name(from,biddername,sizeof(biddername)));
		return -1;
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
