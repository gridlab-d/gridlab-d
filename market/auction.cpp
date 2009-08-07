/** $Id: auction.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file auction.cpp
	@defgroup auction Template for a new object class
	@ingroup MODULENAME

	The auction object implements the basic auction. 

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "gridlabd.h"
#include "auction.h"

CLASS *auction::oclass = NULL;
auction *auction::defaults = NULL;

static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_POSTTOPDOWN;

EXPORT int64 submit_bid(OBJECT *obj, OBJECT *from, double quantity, double price, KEY bid_id)
{
	char biddername[64];
	if (obj->oclass==auction::oclass)
	{
		auction *mkt = OBJECTDATA(obj,auction);
		return mkt->submit(from,quantity,price,bid_id);
	}
	else
	{
		gl_error("%s submitted a bid to an object that is not an auction", gl_name(from,biddername,sizeof(biddername)));
		return -1;
	}
}

/* Class registration is only called once to register the class with the core */
auction::auction(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"auction",sizeof(auction),passconfig);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
			PT_enumeration, "type", PADDR(type),
				PT_KEYWORD, "NONE", AT_NONE,
				PT_KEYWORD, "DOUBLE", AT_DOUBLE,
			PT_char32, "unit", PADDR(unit),
			PT_double, "period[s]", PADDR(period),
			PT_double, "latency[s]", PADDR(latency),
			PT_int64, "market_id", PADDR(market_id),
			PT_double, "last.Q", PADDR(last.quantity), PT_ACCESS, PA_REFERENCE,
			PT_double, "last.P", PADDR(last.price), PT_ACCESS, PA_REFERENCE,
			PT_double, "next.Q", PADDR(next.quantity), PT_ACCESS, PA_REFERENCE,
			PT_double, "next.P", PADDR(next.price),  PT_ACCESS, PA_REFERENCE,
			PT_object, "network", PADDR(network),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		gl_publish_function(oclass,	"submit_bid", (FUNCTIONADDR)submit_bid);
		defaults = this;
		memset(this,0,sizeof(auction));
	}
}

/* Object creation is called once for each object that is created by the core */
int auction::create(void)
{
	memcpy(this,defaults,sizeof(auction));
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int auction::init(OBJECT *parent)
{
	OBJECT *obj=OBJECTHDR(this);
	if (type==AT_NONE)
	{
		gl_error("%s (auction:%d) market type not specified", obj->name?obj->name:"anonymous", obj->id);
		return 0;
	}
#ifdef NEVER // this is not needed 
	if (network==NULL)
	{
		gl_error("%s (auction:%d) market is not connected to a network", obj->name?obj->name:"anonymous", obj->id);
		return 0;
	}
	else if (!gl_object_isa(network,"comm"))
	{
		gl_error("%s (auction:%d) network is not a comm object (type=%s)", obj->name?obj->name:"anonymous", obj->id, network->oclass->name);
		return 0;
	}
#endif
	return 1; /* return 1 on success, 0 on failure */
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP auction::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	if (clearat==TS_ZERO)
	{
		clearat = nextclear();
		DATETIME dt;
		gl_localtime(clearat,&dt);
		char buffer[256];
		char myname[64];
		gl_verbose("%s first clearing at %s", gl_name(OBJECTHDR(this),myname,sizeof(myname)), gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");
	}
	else
	{
		/* if clock has advanced to a market clearing time */
		if (t1>t0 && fmod((double)(t1/TS_SECOND),period)==0)
		{
			/* save the last clearing and reset the next clearing */
			last = next;
			next.from = NULL; /* in the context of a clearing, from is the marginal resource */
			next.quantity = next.price = 0;
		}
	}
	return -clearat; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP auction::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	if (t1>=clearat)
	{
		DATETIME dt;
		gl_localtime(clearat,&dt);
		char buffer[256];
		char myname[64];
		gl_verbose("%s clearing process started at %s", gl_name(OBJECTHDR(this),myname,sizeof(myname)), gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");

		/* clear market */
		clear_market();

		market_id++;

		char name[64];
		clearat = nextclear();
		gl_localtime(clearat,&dt);
		gl_verbose("%s opens for clearing of market_id %d at %s", gl_name(OBJECTHDR(this),name,sizeof(name)), (int32)market_id, gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");
	}
	return -clearat; /* soft return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

void auction::clear_market(void)
{
	/* sort the bids */
	offers.sort(false);
	asks.sort(true);
	
	if (asks.getcount()>0 && offers.getcount()>0)
	{
		/* clear market */
		unsigned int i=0, j=0;
		BID *buy=asks.getbid(i), *sell=offers.getbid(j);
		BID clear = {NULL,0,0};
		double demand_quantity=0, supply_quantity=0;
		double a=0, b=0;
		bool check=false;

		// dump curves
		char *verbose=gl_global_getvar("verbose",NULL,0);
		if (verbose!=NULL && atoi(verbose)>0)
		{
			char name[64];
			gl_verbose("  supply curve");
			for (i=0; i<offers.getcount(); i++)
				//gl_verbose("  %4d: %s offers %.3f %s at %.2f $/%s",i,gl_name(offers.getbid(i)->from,name,sizeof(name)), offers.getbid(i)->quantity,unit,offers.getbid(i)->price,unit);
			printf("  %4d: %s offers %.3f %s at %.2f $/%s\n",i,gl_name(offers.getbid(i)->from,name,sizeof(name)), offers.getbid(i)->quantity,unit,offers.getbid(i)->price,unit);
				gl_verbose("  demand curve");
			for (i=0; i<asks.getcount(); i++)
				//gl_verbose("  %4d: %s asks %.3f %s at %.2f $/%s",i,gl_name(asks.getbid(i)->from,name,sizeof(name)), asks.getbid(i)->quantity,unit,asks.getbid(i)->price,unit);
			printf("  %4d: %s asks %.3f %s at %.2f $/%s\n",i,gl_name(asks.getbid(i)->from,name,sizeof(name)), asks.getbid(i)->quantity,unit,asks.getbid(i)->price,unit);
		}

		i=j=0;
		while (i<asks.getcount() && j<offers.getcount() && buy->price>=sell->price)
		{
			double buy_quantity = demand_quantity + buy->quantity;
			double sell_quantity = supply_quantity + sell->quantity;
			if (buy_quantity > sell_quantity)
			{
				clear.quantity = supply_quantity = sell_quantity;
				a = b = buy->price;
				sell = offers.getbid(++j);
				check = false;
			}
			else if (buy_quantity < sell_quantity)
			{
				clear.quantity = demand_quantity = buy_quantity;
				a = b = sell->price;
				buy = asks.getbid(++i);
				check = false;
			}
			else /* buy quantity equal sell quantity but price split */
			{
				clear.quantity = demand_quantity = supply_quantity = buy_quantity;
				a = buy->price;
				b = sell->price;
				buy = asks.getbid(++i);
				sell = offers.getbid(++j);
				check = true;
			}
		}
	
		/* check for split price at single quantity */
		while (check)
		{
			if (i>0 && i<asks.getcount() && (a<b ? a : b) <= buy->price)
			{
				b = buy->price;
				buy = asks.getbid(++i);
			}
			else if (j>0 && j<offers.getcount() && (a<b ? a : b) <= sell->price)
			{
				a = sell->price;
				sell = offers.getbid(++j);
			}
			else
				check = false;
		}
		clear.price = a<b ? a : b;
	
		/* check for zero demand but non-zero first unit sell price */
		if (clear.quantity==0 && offers.getcount()>0)
		{
			clear.price = offers.getbid(0)->price;
		}
	
		/* post the price */
		char name[64];
		gl_verbose("  %s clears %.2f %s at $%.2f/%s\n", gl_name(OBJECTHDR(this),name,sizeof(name)), clear.quantity, unit, clear.price, unit);
		next.price = clear.price;
		next.quantity = clear.quantity;

		/* clear the bid lists */
		asks.clear();
		offers.clear();
	}
	else
	{
		char name[64];
		next.price = 0;
		next.quantity = 0;
		gl_verbose("  %s fails to clear due to missing %s\n", gl_name(OBJECTHDR(this),name,sizeof(name)), asks.getcount()==0?(offers.getcount()==0?"buyers and sellers":"buyers"):"sellers");
	}
}

KEY auction::submit(OBJECT *from, double quantity, double price, KEY key)
{
	if (key>=0) // resubmit
	{
		char myname[64];
		char biddername[64];
		//gl_warning("  %s resubmits %s from object %s for %.2f %s at $%.2f/%s", 
			//gl_name(OBJECTHDR(this),myname,sizeof(myname)), quantity<0?"ask":"offer", gl_name(from,biddername,sizeof(biddername)), 
			//fabs(quantity), unit, price, unit);
		printf("  %s resubmits %s from object %s for %.2f %s at $%.2f/%s\n", 
			gl_name(OBJECTHDR(this),myname,sizeof(myname)), quantity<0?"ask":"offer", gl_name(from,biddername,sizeof(biddername)), 
			fabs(quantity), unit, price, unit);
		BID bid = {from,fabs(quantity),price};
		if (quantity<0)
			return asks.resubmit(&bid,key);
		else if (quantity>0)
			return offers.resubmit(&bid,key);
		char name[64];
		gl_warning("zero quantity bid from %s is ignored", gl_name(from,name,sizeof(name)));
		return 0;
	}
	else {
		char myname[64];
		char biddername[64];
		//gl_warning("  %s receives %s from object %s for %.2f %s at $%.2f/%s", 
		//	gl_name(OBJECTHDR(this),myname,sizeof(myname)), quantity<0?"ask":"offer", gl_name(from,biddername,sizeof(biddername)), 
		//	fabs(quantity), unit, price, unit);
		printf("  %s receives %s from object %s for %.2f %s at $%.2f/%s\n", 
			gl_name(OBJECTHDR(this),myname,sizeof(myname)), quantity<0?"ask":"offer", gl_name(from,biddername,sizeof(biddername)), 
			fabs(quantity), unit, price, unit);
		BID bid = {from,fabs(quantity),price};
		if (quantity<0)
			return asks.submit(&bid);
		else if (quantity>0)
			return offers.submit(&bid);
		char name[64];
		gl_warning("zero quantity bid from %s is ignored", gl_name(from,name,sizeof(name)));
		return -1;
	}
}

TIMESTAMP auction::nextclear(void) const
{
	return gl_globalclock + (TIMESTAMP)(period - fmod(gl_globalclock+period,period));
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_auction(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(auction::oclass);
		if (*obj!=NULL)
		{
			auction *my = OBJECTDATA(*obj,auction);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_auction: %s", msg);
	}
	return 1;
}

EXPORT int init_auction(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,auction)->init(parent);
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("init_auction(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return 1;
}

EXPORT TIMESTAMP sync_auction(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	auction *my = OBJECTDATA(obj,auction);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("sync_auction(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return t2;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF BID CURVE
//////////////////////////////////////////////////////////////////////////

curve::curve(void)
{
	len = 0;
	bids = NULL;
	keys = NULL;
	n_bids = 0;
}

curve::~curve(void)
{
	delete [] bids;
	delete [] keys;
}

void curve::clear(void)
{
	n_bids = 0;
}

BID *curve::getbid(KEY n)
{
	return bids+keys[n];
}

KEY curve::submit(BID *bid)
{
	if (len==0) // create the bid list
	{
		len = 8;
		bids = new BID[len];
		keys = new KEY[len];
	}
	else if (n_bids==len) // grow the bid list
	{
		BID *newbids = new BID[len*2];
		KEY *newkeys = new KEY[len*2];
		memcpy(newbids,bids,len*sizeof(BID));
		memcpy(newkeys,keys,len*sizeof(KEY));
		delete[] bids;
		delete[] keys;
		bids = newbids;
		keys = newkeys;
		len*=2;
	}
	keys[n_bids] = n_bids;
	BID *next = bids + n_bids;
	*next = *bid;
	return n_bids++;
}

KEY curve::resubmit(BID *bid, KEY key)
{
	if (key<n_bids)
	{
		bids[keys[key]] = *bid;
		return key;
	}
	else
		return -1;
}

void curve::sort(bool reverse)
{
	sort(bids, keys, n_bids, reverse);
}

void curve::sort(BID *list, KEY *key, const int len, const bool reverse)
{
	//merge sort
	if (len>0)
	{
#define BUFSIZE 1024
		int split = len/2;
		int *a = key, *b = key+split;
		if (split>1) sort(list,a,split,reverse);
		if (len-split>1) sort(list,b,len-split,reverse);
		if (len>BUFSIZE)
			throw "curve::sort(...) n_bids exceeds buffer size";
		KEY res[BUFSIZE];
		KEY *p = res;
		do {
			bool altb = list[*a].price < list[*b].price;
			if ((reverse && !altb) || (!reverse && altb))
				*p++ = *a++;
			else
				*p++ = *b++;
		} while (a<key+split && b<key+len);
		while (a<key+split)
			*p++ = *a++;
		while (b<key+len)
			*p++ = *b++;
		memcpy(key,res,sizeof(KEY)*len);
	}
}
