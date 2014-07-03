#include "stub_bidder.h"

CLASS* stub_bidder::oclass = NULL;

stub_bidder::stub_bidder(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"stub_bidder",sizeof(stub_bidder),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
				PT_double, "bid_period[s]", PADDR(bid_period),
				PT_int16, "count", PADDR(count),
				PT_object, "market", PADDR(market),
				PT_enumeration, "role", PADDR(role),
					PT_KEYWORD, "BUYER", (enumeration)BUYER,
					PT_KEYWORD, "SELLER", (enumeration)SELLER,
				PT_double, "price", PADDR(price),
				PT_double, "quantity", PADDR(quantity),
				NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
		
		memset(this,0,sizeof(stub_bidder));
	}
}

int stub_bidder::create()
{
	return SUCCESS;
}

int stub_bidder::init(OBJECT *parent)
{
	new_bid_id = -1;
	next_t = 0;
	lastbid_id = -1;
	lastmkt_id = -1;
	if (market==NULL)			
		throw "market is not defined";
	thismkt_id = (int64*)gl_get_addr(market,"market_id");
	if (thismkt_id==NULL)
		throw "market does not define market_id";
	return SUCCESS;
}

int stub_bidder::isa(char *classname)
{
	return strcmp(classname,"stub_bidder")==0;
}

TIMESTAMP stub_bidder::sync(TIMESTAMP t0, TIMESTAMP t1)
{

	if (t1==next_t || next_t==0)
	{
		next_t=t1+(TIMESTAMP)bid_period;
		lastbid_id = (KEY)submit_bid(market,OBJECTHDR(this),role==BUYER?-quantity:quantity,price,*thismkt_id!=lastmkt_id?new_bid_id:lastbid_id);
		count--;
		lastmkt_id = *thismkt_id;
	}
	if (count>0)
		return next_t;
	else
		return TS_NEVER;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_stub_bidder(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(stub_bidder::oclass);
		if (*obj!=NULL)
		{
			stub_bidder *my = OBJECTDATA(*obj,stub_bidder);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_stub_bidder: %s", msg);
		return 0;
	}
	return 1;
}

EXPORT int init_stub_bidder(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL){
			return OBJECTDATA(obj,stub_bidder)->init(parent);
		}
	}
	catch (const char *msg)
	{
		char name[64];
		gl_error("init_stub_bidder(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
		return 0;
	}
	return 1;
}

EXPORT int isa_stub_bidder(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,stub_bidder)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_stub_bidder(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	stub_bidder *my = OBJECTDATA(obj,stub_bidder);
	switch (pass) {
	case PC_BOTTOMUP:
		t2 = my->sync(obj->clock,t1);
		obj->clock = t1;
		break;
	default:
		break;
	}
	return t2;
}