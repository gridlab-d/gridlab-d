/** $Id: auction.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file auction.cpp
	@defgroup auction Template for a new object class
	@ingroup market

	The auction object implements the basic auction. 

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "gridlabd.h"
#include "auction.h"
#include "stubauction.h"

CLASS *auction::oclass = NULL;
auction *auction::defaults = NULL;
STATISTIC *auction::stats = NULL;
TIMESTAMP auction::longest_statistic = 0;
int auction::statistic_check = -1;
uint32 auction::statistic_count = 0;

static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_POSTTOPDOWN;

EXPORT int64 get_market_for_time(OBJECT *obj, TIMESTAMP ts){
	auction *pAuc = 0;
	TIMESTAMP market_time = 0;
	int64 market_id;
	if(obj == 0){
		gl_error("get_market_for_time() was called with a null object pointer");
		/* TROUBLESHOOT
			The call was made without specifying the object that called it.  Double-check the code where this
			method was called and verify that a non-null object pointer is passed in.
			*/
		return -1;
	}
	pAuc = OBJECTDATA(obj, auction);
	// find when the current market started
	market_time = gl_globalclock + pAuc->period + pAuc->latency - ((gl_globalclock + pAuc->period) % pAuc->period);
	if(ts < market_time){
		KEY key;
		int64 remainder = ts - market_time;
		// the market ID wanted is N markets ahead of the current market...
		if(remainder < pAuc->period){
			market_id = pAuc->market_id;
		} else {
			market_id = pAuc->market_id + remainder/pAuc->period;
		}
		write_bid(key, market_id, -1, BID_UNKNOWN);
		return (int64)key;
	} else { // late, the market for that time has already closed
		return -1;
	}
	return -1;
}

/** Auction/register_participant allow an auction to interact propertly with a participating controller.
    This makes it unnecessary to manually set the market rank high in the GLM file and avoids the use
	of gl_set_dependent during init() of participants.
 **/
EXPORT int64 register_participant(OBJECT *mkt, OBJECT *part)
{
	/* need to promote market above rank of participant */
	gl_set_rank(mkt,part->rank);

	/* @todo other actions that may required with market participant are initialized */
	return 1;
}

/* Class registration is only called once to register the class with the core */
auction::auction(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"auction",sizeof(auction),passconfig|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class auction";
		else
			oclass->trl = TRL_QUALIFIED;

		if (gl_publish_variable(oclass,
			PT_char32, "unit", PADDR(unit), PT_DESCRIPTION, "unit of quantity",
			PT_double, "period[s]", PADDR(dPeriod), PT_DESCRIPTION, "interval of time between market clearings",
			PT_double, "latency[s]", PADDR(dLatency), PT_DESCRIPTION, "latency between market clearing and delivery", 
			PT_int64, "market_id", PADDR(market_id), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "unique identifier of market clearing",
/**/		PT_object, "network", PADDR(network), PT_DESCRIPTION, "the comm network used by object to talk to the market (if any)",
			PT_bool, "verbose", PADDR(verbose), PT_DESCRIPTION, "enable verbose auction operations",
			PT_object, "linkref", PADDR(linkref), PT_DEPRECATED, PT_DESCRIPTION, "reference to link object that has demand as power_out (only used when not all loads are bidding)",
			PT_double, "pricecap", PADDR(pricecap), PT_DEPRECATED, PT_DESCRIPTION, "the maximum price (magnitude) allowed",
			PT_double, "price_cap", PADDR(pricecap), PT_DESCRIPTION, "the maximum price (magnitude) allowed",

			PT_enumeration, "special_mode", PADDR(special_mode),
				PT_KEYWORD, "NONE", (enumeration)MD_NONE,
				PT_KEYWORD, "SELLERS_ONLY", (enumeration)MD_SELLERS,
				PT_KEYWORD, "BUYERS_ONLY", (enumeration)MD_BUYERS,
			PT_enumeration, "statistic_mode", PADDR(statistic_mode),
				PT_KEYWORD, "ON", (enumeration)ST_ON,
				PT_KEYWORD, "OFF", (enumeration)ST_OFF,
			PT_double, "fixed_price", PADDR(fixed_price),
			PT_double, "fixed_quantity", PADDR(fixed_quantity),
			PT_object, "capacity_reference_object", PADDR(capacity_reference_object),
			PT_char32, "capacity_reference_property", PADDR(capacity_reference_propname),
			PT_double, "capacity_reference_bid_price", PADDR(capacity_reference_bid_price),
			PT_double, "max_capacity_reference_bid_quantity", PADDR(max_capacity_reference_bid_quantity),
			PT_double, "capacity_reference_bid_quantity", PADDR(capacity_reference_bid_quantity),
			PT_double, "init_price", PADDR(init_price),
			PT_double, "init_stdev", PADDR(init_stdev),
			PT_double, "future_mean_price", PADDR(future_mean_price),
			PT_bool, "use_future_mean_price", PADDR(use_future_mean_price),

			PT_timestamp, "current_market.start_time", PADDR(current_frame.start_time),
			PT_timestamp, "current_market.end_time", PADDR(current_frame.end_time),
			PT_double, "current_market.clearing_price[$]", PADDR(current_frame.clearing_price),
			PT_double, "current_market.clearing_quantity", PADDR(current_frame.clearing_quantity),
			PT_enumeration, "current_market.clearing_type", PADDR(current_frame.clearing_type),
				PT_KEYWORD, "MARGINAL_SELLER", (enumeration)CT_SELLER,
				PT_KEYWORD, "MARGINAL_BUYER", (enumeration)CT_BUYER,
				PT_KEYWORD, "MARGINAL_PRICE", (enumeration)CT_PRICE,
				PT_KEYWORD, "EXACT", (enumeration)CT_EXACT,
				PT_KEYWORD, "FAILURE", (enumeration)CT_FAILURE,
				PT_KEYWORD, "NULL", (enumeration)CT_NULL,
			PT_double, "current_market.marginal_quantity_load", PADDR(current_frame.marginal_quantity),
			PT_double, "current_market.marginal_quantity", PADDR(current_frame.marginal_quantity),
			PT_double, "current_market.marginal_quantity_bid", PADDR(current_frame.total_marginal_quantity),
			PT_double, "current_market.marginal_quantity_frac", PADDR(current_frame.marginal_frac),
			PT_double, "current_market.seller_total_quantity", PADDR(current_frame.seller_total_quantity),
			PT_double, "current_market.buyer_total_quantity", PADDR(current_frame.buyer_total_quantity),
			PT_double, "current_market.seller_min_price", PADDR(current_frame.seller_min_price),
			PT_double, "current_market.buyer_total_unrep", PADDR(current_frame.buyer_total_unrep),
			PT_double, "current_market.cap_ref_unrep", PADDR(current_frame.cap_ref_unrep),

			PT_timestamp, "next_market.start_time", PADDR(next_frame.start_time),
			PT_timestamp, "next_market.end_time", PADDR(next_frame.end_time),
			PT_double, "next_market.clearing_price[$]", PADDR(next_frame.clearing_price),
			PT_double, "next_market.clearing_quantity", PADDR(next_frame.clearing_quantity),
			PT_enumeration, "next_market.clearing_type", PADDR(next_frame.clearing_type),
				PT_KEYWORD, "MARGINAL_SELLER", (enumeration)CT_SELLER,
				PT_KEYWORD, "MARGINAL_BUYER", (enumeration)CT_BUYER,
				PT_KEYWORD, "MARGINAL_PRICE", (enumeration)CT_PRICE,
				PT_KEYWORD, "EXACT", (enumeration)CT_EXACT,
				PT_KEYWORD, "FAILURE", (enumeration)CT_FAILURE,
				PT_KEYWORD, "NULL", (enumeration)CT_NULL,
			PT_double, "next_market.marginal_quantity_load", PADDR(next_frame.marginal_quantity),
			PT_double, "next_market.marginal_quantity_bid", PADDR(next_frame.total_marginal_quantity),
			PT_double, "next_market.marginal_quantity_frac", PADDR(next_frame.marginal_frac),
			PT_double, "next_market.seller_total_quantity", PADDR(next_frame.seller_total_quantity),
			PT_double, "next_market.buyer_total_quantity", PADDR(next_frame.buyer_total_quantity),
			PT_double, "next_market.seller_min_price", PADDR(next_frame.seller_min_price),
			PT_double, "next_market.cap_ref_unrep", PADDR(next_frame.cap_ref_unrep),

			PT_timestamp, "past_market.start_time", PADDR(past_frame.start_time),
			PT_timestamp, "past_market.end_time", PADDR(past_frame.end_time),
			PT_double, "past_market.clearing_price[$]", PADDR(past_frame.clearing_price),
			PT_double, "past_market.clearing_quantity", PADDR(past_frame.clearing_quantity),
			PT_enumeration, "past_market.clearing_type", PADDR(past_frame.clearing_type),
				PT_KEYWORD, "MARGINAL_SELLER", (enumeration)CT_SELLER,
				PT_KEYWORD, "MARGINAL_BUYER", (enumeration)CT_BUYER,
				PT_KEYWORD, "MARGINAL_PRICE", (enumeration)CT_PRICE,
				PT_KEYWORD, "EXACT", (enumeration)CT_EXACT,
				PT_KEYWORD, "FAILURE", (enumeration)CT_FAILURE,
				PT_KEYWORD, "NULL", (enumeration)CT_NULL,
			PT_double, "past_market.marginal_quantity_load", PADDR(past_frame.marginal_quantity),
			PT_double, "past_market.marginal_quantity_bid", PADDR(past_frame.total_marginal_quantity),
			PT_double, "past_market.marginal_quantity_frac", PADDR(past_frame.marginal_frac),
			PT_double, "past_market.seller_total_quantity", PADDR(past_frame.seller_total_quantity),
			PT_double, "past_market.buyer_total_quantity", PADDR(past_frame.buyer_total_quantity),
			PT_double, "past_market.seller_min_price", PADDR(past_frame.seller_min_price),
			PT_double, "past_market.cap_ref_unrep", PADDR(past_frame.cap_ref_unrep),

			PT_enumeration, "margin_mode", PADDR(margin_mode),
				PT_KEYWORD, "NORMAL", 0,
				PT_KEYWORD, "DENY", (enumeration)AM_DENY,
				PT_KEYWORD, "PROB", (enumeration)AM_PROB,
			PT_int32, "warmup", PADDR(warmup),
			PT_enumeration, "ignore_pricecap", PADDR(ignore_pricecap),
				PT_KEYWORD, "FALSE", (enumeration)IP_FALSE,
				PT_KEYWORD, "TRUE", (enumeration)IP_TRUE,
			PT_char256, "transaction_log_file", PADDR(trans_log),
			PT_int32, "transaction_log_limit", PADDR(trans_log_max),
			PT_char256, "curve_log_file", PADDR(curve_log),
			PT_int32, "curve_log_limit", PADDR(curve_log_max),
			PT_enumeration, "curve_log_info", PADDR(curve_log_info),
				PT_KEYWORD, "NORMAL", (enumeration)CO_NORMAL,
				PT_KEYWORD, "EXTRA", (enumeration)CO_EXTRA,

			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
			}
		gl_publish_function(oclass,	"submit_bid", (FUNCTIONADDR)submit_bid);
		gl_publish_function(oclass,	"submit_bid_state", (FUNCTIONADDR)submit_bid_state);
		gl_publish_function(oclass, "get_market_for_time", (FUNCTIONADDR)get_market_for_time);
		gl_publish_function(oclass, "register_participant", (FUNCTIONADDR)register_participant);
		defaults = this;
//		immediate = 1;
		memset(this,0,sizeof(auction));
	}
}


int auction::isa(char *classname)
{
	return strcmp(classname,"auction")==0;
}

/* Object creation is called once for each object that is created by the core */
int auction::create(void)
{
	STATISTIC *stat;
	double val = -1.0;
	memcpy(this,defaults,sizeof(auction));
	lasthr = thishr = -1;
	verbose = 0;
	use_future_mean_price = 0;
	pricecap = 0;
	warmup = 1;
	market_id = 1;
	clearing_scalar = 0.5;
	/* process dynamic statistics */
	if(statistic_check == -1){
		int rv;
		this->statistic_check = 0;
		rv = init_statistics();
		if(rv < 0){
			return 0;
		} else if(rv == 0){
			;
		} // else some number of statistics came back
	}
	for(stat = stats; stat != NULL; stat = stat->next){
		gl_set_value(OBJECTHDR(this), stat->prop, val);
	}
	statistic_mode = ST_ON;
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int auction::init(OBJECT *parent)
{
	OBJECT *obj=OBJECTHDR(this);
	unsigned int i = 0;

	if(capacity_reference_object != NULL){
		if(obj->rank <= capacity_reference_object->rank){
			gl_set_rank(obj,capacity_reference_object->rank+1);
		}
		if(capacity_reference_object->flags & OF_INIT != OF_INIT){
			char objname[256];
			gl_verbose("auction::init(): deferring initialization on %s", gl_name(obj, objname, 255));
			return 2; // defer
		}
	}

	if(trans_log[0] != 0){
		time_t now = time(NULL);
		trans_file = fopen(trans_log, "w");
		if(trans_file == 0){
			gl_error("%s (auction:%d) unable to open '%s' as a transaction log output file", obj->name?obj->name:"anonymous", obj->id, trans_log.get_string());
			return 0;
		}
		// else write transaction file header, a la recorder file headers
		fprintf(trans_file, "# %s\n", trans_log.get_string());
		fprintf(trans_file, "# %s", asctime(localtime(&now))); // return char already included in asctime
		fprintf(trans_file, "# %s\n", obj->name?obj->name:"anonymous");
		fprintf(trans_file, "# market_id,timestamp,bidder_name,bid_price,bid_quantity,bid_state\n");
		fflush(trans_file);
		trans_log_count = trans_log_max;
	}

	if(curve_log[0] != 0){
		time_t now = time(NULL);
		curve_file = fopen(curve_log, "w");
		if(curve_file == 0){
			gl_error("%s (auction:%d) unable to open '%s' as an aurction curve log output file", obj->name?obj->name:"anonymous", obj->id, trans_log.get_string());
			return 0;
		}
		fprintf(curve_file, "# %s\n", curve_log.get_string());
		fprintf(curve_file, "# %s", asctime(localtime(&now))); // return char already included in asctime
		fprintf(curve_file, "# %s\n", obj->name?obj->name:"anonymous");
		fprintf(curve_file, "# market_id,timestamp,sort_index,bidder_name,bid_quantity,bid_price\n");
		fflush(curve_file);
		curve_log_count = curve_log_max;
	}

	if (pricecap==0){
		pricecap = 9999.0;
	}

	if(dPeriod == 0.0){
		dPeriod = 300.0;
		period = 300; // five minutes
	} else {
		period = (TIMESTAMP)floor(dPeriod + 0.5);
	}

	if(dLatency <= 0.0){
		dLatency = 0.0;
		latency = 0;
	} else {
		latency = (TIMESTAMP)floor(dLatency + 0.5);
	}
	// @new

	// init statistics, vs create statistics
	STATISTIC *statprop;
	for(statprop = stats; statprop != NULL; statprop = statprop->next){
		if(statprop->interval < this->period){
			static int was_warned = 0;
			if(was_warned == 0){
				gl_warning("market statistic '%s' samples faster than the market updates and will be filled with immediate data", statprop->prop->name);
				/* TROUBLESHOOT
					Market statistics are only calculated when the market clears.  Statistics shorter than the market clearing rate wind up being
					equal to either the immediate price or to zero, due to the constant price during the specified interval.
					*/
				was_warned = 0;
			}
			//statprop.interval = (TIMESTAMP)(this->period);
		} else if(statprop->interval % (int64)(this->period) != 0){
			static int was_also_warned = 0;
			gl_warning("market statistic '%s' interval not a multiple of market period, rounding towards one interval", statprop->prop->name);
			/* TROUBLESHOOT
				Market statistics are only calculated when the market clears, and will be calculated assuming intervals that are multiples of the market's period.
				*/
			statprop->interval -= statprop->interval % (int64)(period);
		}
	}
	/* reference object & property */	
	if(capacity_reference_object != NULL){
		if(capacity_reference_propname[0] != 0){
			capacity_reference_property = gl_get_property(capacity_reference_object, capacity_reference_propname.get_string());
			if(capacity_reference_property == NULL){
				gl_error("%s (auction:%d) capacity_reference_object of type '%s' does not contain specified reference property '%s'", obj->name?obj->name:"anonymous", obj->id, capacity_reference_object->oclass->name, capacity_reference_propname.get_string());
				/* TROUBLESHOOT
					capacity_reference_object must contain a property named by capacity_reference_property.  Review the published properties of the object specified
					in capacity_reference_object.
					*/
				return 0;
			}
			if(capacity_reference_property->ptype != PT_double){
				gl_warning("%s (auction:%d) capacity_reference_property '%s' is not a double type property", obj->name?obj->name:"anonymous", obj->id, capacity_reference_propname.get_string());
				/* TROUBLESHOOT
					capacity_reference_property must specify a double property to work properly, and may behave unpredictably should it be pointed at
					non-double properties.
					*/
			}
			if(capacity_reference_object->rank >= obj->rank){
				obj->rank = capacity_reference_object->rank + 1;
			}
		} else {
			gl_error("%s (auction:%d) capacity_reference_object specified without a reference property", obj->name?obj->name:"anonymous", obj->id);
			/* TROUBLESHOOT
				capacity_reference_object can only be used when capacity_reference_property is specified.
				*/
			return 0;
		}
	}

	if(special_mode != MD_NONE){
		if(fixed_quantity < 0.0){
			gl_error("%s (auction:%d) is using a one-sided market with a negative fixed quantity", obj->name?obj->name:"anonymous", obj->id);
			/* TROUBLESHOOT
				capacity_reference_object can only be used when capacity_reference_property is specified.
				*/
			return 0;
		}
	}

	// initialize latency queue
	latency_count = (uint32)(latency / period + 2);
	latency_stride = sizeof(MARKETFRAME) + statistic_count * sizeof(double);
	framedata = (MARKETFRAME *)malloc(latency_stride * latency_count);
	memset(framedata, 0, latency_stride * latency_count);
	for(i = 0; i < latency_count; ++i){
		MARKETFRAME *frameptr;
		int64 addr = latency_stride * i + (int64)framedata;
		int64 stataddr = addr + sizeof(MARKETFRAME);
		int64 nextaddr = addr + latency_stride;
		frameptr = (MARKETFRAME *)(addr);
		if(statistic_count > 0){
			frameptr->statistics = (double *)(stataddr);
		} else {
			frameptr->statistics = 0;
		}
		if(i+1 < latency_count){
			frameptr->next = (MARKETFRAME *)(nextaddr);
		} else if(i+1 == latency_count){
			frameptr->next = framedata;
		}
	}
	latency_front = latency_back = 0;
	// initialize arrays
	if(statistic_count > 0){
		statdata = (double *)malloc(sizeof(double) * statistic_count);
	}
	if(longest_statistic > 0){
		history_count = (uint32)longest_statistic / (uint32)(this->period) + 2;
		new_prices = (double *)malloc(sizeof(double) * history_count);
	} else {
		history_count = 1;
		new_prices = (double *)malloc(sizeof(double));
	}
	price_index = 0;
	price_count = 0;
	for(i = 0; i < history_count; ++i){
		new_prices[i] = init_price;
	}

	if(init_stdev < 0.0){
		gl_error("auction init_stdev is negative!");
		/* TROUBLESHOOT
			By mathematical definition, real standard deviations cannot be negative.
			*/
		return 0;
	}
	STATISTIC *stat;
	for(stat = stats; stat != NULL; stat = stat->next){
		double check = 0.0;
		if(stat->stat_type == SY_STDEV){
			check = *gl_get_double(obj, stat->prop);
			if(check == -1.0){
				gl_set_value(obj, stat->prop, init_stdev);
			}
		} else if(stat->stat_type == SY_MEAN){
			check = *gl_get_double(obj, stat->prop);
			if(check == -1.0){
				gl_set_value(obj, stat->prop, init_price);
			}
		}
	}
	if(clearing_scalar <= 0.0){
		clearing_scalar = 0.5;
	}
	if(clearing_scalar >= 1.0){
		clearing_scalar = 0.5;
	}
	current_frame.clearing_price = init_price;
	past_frame.clearing_price = init_price;
	if(latency > 0)
		next_frame.clearing_price = init_price;
	return 1; /* return 1 on success, 0 on failure */
}

int auction::init_statistics(){
	STATISTIC *sp = 0;
	STATISTIC *tail = 0;
	STATISTIC statprop;
	PROPERTY *prop = oclass->pmap;
	OBJECT *obj = OBJECTHDR(this);
	for(prop = oclass->pmap; prop != NULL; prop = prop->next){
		char frame[32], price[32], stat[32], period[32], period_unit[32];
		memset(&statprop, 0, sizeof(STATISTIC));
		period_unit[0] = 0;
		if(sscanf(prop->name, "%[^\n_]_%[^\n_]_%[^\n_]_%[0-9]%[A-Za-z]", frame, price, stat, period, period_unit) >= 4){
			if(strcmp(price, "price") != 0){
				continue;
			}
			if(strcmp(stat, "mean") == 0){
				statprop.stat_type = SY_MEAN;
			} else if(strcmp(stat, "stdev") == 0){
				statprop.stat_type = SY_STDEV;
			} else {
				continue; 
			}
			if(strcmp(frame, "past") == 0){
				statprop.stat_mode = ST_PAST;
			} else if(strcmp(frame, "current") == 0){
				statprop.stat_mode = ST_CURR;
			} else {
				continue;
			}
			// parse period
			statprop.interval = strtol(period, 0, 10);
			if(statprop.interval <= 0){
				gl_warning("market statistic interval for '%s' is not positive, skipping", prop->name);
				/* TROUBLESHOOT
					Negative market statistic intervals are nonsensical.
					*/
				continue;
			}
			// scale by period_unit, if any
			if(period_unit[0] == 0){
				; // none? continue!
			} else if(period_unit[0] == 'm'){
				statprop.interval *= 60; // minutes
			} else if(period_unit[0] == 'h'){
				statprop.interval *= 3600;
			} else if(period_unit[0] == 'd'){
				statprop.interval *= 86400;
			} else if(period_unit[0] == 'w'){
				statprop.interval *= 604800;
			} else {
				gl_warning("market statistic period scalar '%c' not recognized, statistic ignored", period_unit[0]);
			} // months and years are of varying length
			// enqueue a new STATPROP instance
			sp = (STATISTIC *)malloc(sizeof(STATISTIC));
			memcpy(sp, &statprop, sizeof(STATISTIC));
			strcpy(sp->statname, prop->name);
			sp->prop = prop;
			sp->value = 0;
			if(stats == 0){
				stats = sp;
				tail = stats;
			} else {
				tail->next = sp;
				tail = sp;
			}

			++statistic_count;
			if(statprop.interval > longest_statistic){
				longest_statistic = statprop.interval;
			}
		}
	}
	memset(&cleared_frame, 0, latency_stride);
	memset(&current_frame, 0, latency_stride);
	return 1;
}

int auction::update_statistics(){
	OBJECT *obj = OBJECTHDR(this);
	STATISTIC *current = 0;
	uint32 sample_need = 0;
	unsigned int start = 0, stop = 0;
	unsigned int i = 0;
	unsigned int idx = 0;
	unsigned int skipped = 0;
	double mean = 0.0;
	double stdev = 0.0;
	if(statistic_count < 1){
		return 1; // no statistics
	}
	if(new_prices == 0){
		return 0;
	}
	if(statdata == 0){
		return 0;
	}
	if(stats == 0){
		return 1; // should've been caught with statistic_count < 1
	}
	for(current = stats; current != 0; current = current->next){
		mean = 0.0;
		sample_need = (uint32)(current->interval / this->period);
		if(current->stat_mode == ST_CURR){
			stop = price_index;
		} else if(current->stat_mode == ST_PAST){
			stop = price_index - 1;
		}
		start = (unsigned int)((history_count + stop - sample_need) % history_count); // one off for initial period delay
		for(i = 0; i < sample_need; ++i){
			idx = (start + i + history_count) % history_count;
			if( (ignore_pricecap == IP_FALSE) || 
				((new_prices[idx] != pricecap) && (new_prices[idx] != -pricecap))){
				mean += new_prices[idx];
			} else {
				++skipped;
			}
		}
		if(skipped != sample_need){
			mean /= sample_need;
		} else {
			mean = 0; // problem!
		}
		if(use_future_mean_price)
			mean = future_mean_price;
		if(current->stat_type == SY_MEAN){
			current->value = mean;
		} else if(current->stat_type == SY_STDEV){
			double x = 0.0;
			if(sample_need + (current->stat_mode == ST_PAST ? 1 : 0) > total_samples){ // extra sample for 'past' values
				//	still in initial period, use init_stdev
				current->value = init_stdev;
			} else {
				stdev = 0.0;
				for(i = 0; i < sample_need; ++i){
					idx = (start + i + history_count) % history_count;
					if( (ignore_pricecap == IP_FALSE) || 
						((new_prices[idx] != pricecap) && (new_prices[idx] != -pricecap))){
							x = new_prices[idx] - mean;
							stdev += x * x;
					}
				}
				if(skipped != sample_need){
					stdev /= sample_need;
				} else {
					stdev = 0; // problem!
				}
				current->value = sqrt(stdev);
			}
		}
		if(statistic_mode == ST_ON){
			if(latency == 0){
				gl_set_value(obj, current->prop, current->value);
			}
		}
	}
	return 1;
}

/*	Take the current market values and enqueue them on the end of the latency frame queue. */
int auction::push_market_frame(TIMESTAMP t1){
	MARKETFRAME *frame = 0;
	OBJECT *obj = OBJECTHDR(this);
	STATISTIC *stat = stats;
	double *stats = 0;
	int64 frame_addr = latency_stride * latency_back + (int64)framedata;
	uint32 i = 0;
	if((latency_back + 1) % latency_count == latency_front){
		gl_error("market latency queue is overwriting as-yet unused data, so is not long enough or is not consuming data");
		/* TROUBLESHOOT
			This is indicative of problems with how the internal circular queue is being handled.  Please report this error on the GridLAB-D
			TRAC site.
			*/
		return 0;
	}
	frame = (MARKETFRAME *)frame_addr;
	stats = frame->statistics;
	// set market details
	frame->market_id = cleared_frame.market_id;
	frame->start_time = cleared_frame.start_time;
	frame->end_time = cleared_frame.end_time;
	frame->clearing_price = cleared_frame.clearing_price;
	frame->clearing_quantity = cleared_frame.clearing_quantity;
	frame->clearing_type = cleared_frame.clearing_type;
	frame->marginal_quantity = cleared_frame.marginal_quantity;
	frame->total_marginal_quantity = cleared_frame.total_marginal_quantity;
	frame->seller_total_quantity = cleared_frame.seller_total_quantity;
	frame->buyer_total_quantity = cleared_frame.buyer_total_quantity;
	frame->seller_min_price = cleared_frame.seller_min_price;
	frame->marginal_frac = cleared_frame.marginal_frac;
	frame->cap_ref_unrep = cleared_frame.cap_ref_unrep;
	// set stats
	for(i = 0, stat = this->stats; i < statistic_count && stat != 0; ++i, stat = stat->next){
		stats[i] = stat->value;
	}
	if(back != 0){
		back->next = frame;
	}
	back = frame;

	latency_back = (latency_back + 1) % latency_count;
	if(latency > 0){
		++total_samples;
	}
	return 1;
}

int auction::check_next_market(TIMESTAMP t1){
	MARKETFRAME *frame = 0;
	frame = (MARKETFRAME *)(latency_front * this->latency_stride + (int64)framedata);
	if(frame->start_time > t1 && frame->start_time <= t1 + period){
		OBJECT *obj = OBJECTHDR(this);
		MARKETFRAME *nframe = frame;
		unsigned int i = 0;
		STATISTIC *stat;
		next_frame.market_id = nframe->market_id;
		next_frame.start_time = nframe->start_time;
		next_frame.end_time = nframe->end_time;
		next_frame.clearing_price = nframe->clearing_price;
		next_frame.clearing_quantity = nframe->clearing_quantity;
		next_frame.clearing_type = nframe->clearing_type;
		next_frame.marginal_quantity = nframe->marginal_quantity;
		next_frame.total_marginal_quantity = nframe->total_marginal_quantity;
		next_frame.seller_total_quantity = nframe->seller_total_quantity;
		next_frame.buyer_total_quantity = nframe->buyer_total_quantity;
		next_frame.seller_min_price = nframe->seller_min_price;
		next_frame.marginal_frac = nframe->marginal_frac;
		next_frame.cap_ref_unrep = nframe->cap_ref_unrep;
		if(statistic_mode == ST_ON){
			for(i = 0, stat = this->stats; i < statistic_count, stat != 0; ++i, stat = stat->next){
				gl_set_value(obj, stat->prop, frame->statistics[i]);
			}
		}
		return 1;
	}
	return 0;
}

/*	Fill in the exposed current market values with those within the */
TIMESTAMP auction::pop_market_frame(TIMESTAMP t1){
	MARKETFRAME *frame = 0;
	OBJECT *obj = OBJECTHDR(this);
	STATISTIC *stat = stats;
	double *stats = 0;
	uint32 i = 0;
	if(latency_front == latency_back){
		gl_verbose("market latency queue has no data");
		return TS_NEVER;
	}

	frame = (MARKETFRAME *)(latency_front * this->latency_stride + (int64)framedata);

	if(t1 < frame->start_time){
		gl_verbose("market latency queue data is not yet applicable");
		return frame->start_time - (latency > 0 ? period : 0);
	}
	// valid, time-applicable data
	// ~ copy current data to past_frame
	memcpy(&past_frame, &current_frame, sizeof(MARKETFRAME)); // not copying lagging stats
	// ~ copy new data in
	current_frame.market_id = frame->market_id;
	current_frame.start_time = frame->start_time;
	current_frame.end_time = frame->end_time;
	current_frame.clearing_price = frame->clearing_price;
	current_frame.clearing_quantity = frame->clearing_quantity;
	current_frame.clearing_type = frame->clearing_type;
	current_frame.marginal_quantity = frame->marginal_quantity;
	current_frame.total_marginal_quantity = frame->total_marginal_quantity;
	current_frame.seller_total_quantity = frame->seller_total_quantity;
	current_frame.buyer_total_quantity = frame->buyer_total_quantity;
	current_frame.seller_min_price = frame->seller_min_price;
	current_frame.marginal_frac = frame->marginal_frac;
	// copy statistics
	if(statistic_mode == ST_ON){
		for(i = 0, stat = this->stats; i < statistic_count, stat != 0; ++i, stat = stat->next){
			gl_set_value(obj, stat->prop, frame->statistics[i]);
		}
	}
	// having used this index, push the index forward
	latency_front = (latency_front + 1) % latency_count;
	return TS_NEVER;
}


/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP auction::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	if (clearat==TS_ZERO)
	{
		clearat = nextclear();
		DATETIME dt;
		gl_localtime(clearat,&dt);
		update_statistics();
		char buffer[256];
		char myname[64];
		if (verbose) gl_output("   ...%s first clearing at %s", gl_name(OBJECTHDR(this),myname,sizeof(myname)), gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");
	}
	else
	{
		/* if clock has advanced to a market clearing time */
		if (t1>t0 && ((t1/TS_SECOND) % period)==0)
		{
			/* save the last clearing and reset the next clearing */
			next.from = NULL; /* in the context of a clearing, from is the marginal resource */
			next.quantity = next.price = 0;
		}
	}

	if (t1>=clearat)
	{
		DATETIME dt;
		gl_localtime(clearat,&dt);
		char buffer[256];
		char myname[64];
		if (verbose) gl_output("   ...%s clearing process started at %s", gl_name(OBJECTHDR(this),myname,sizeof(myname)), gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");

		/* clear market */
		thishr = dt.hour;
		double thismin = dt.minute;
		clear_market();

		// advance market_id
		++market_id;

		char name[64];
		clearat = nextclear();
		// kick this over every hour to prevent odd behavior
		checkat = gl_globalclock + (TIMESTAMP)(3600.0 - fmod(gl_globalclock+3600.0,3600.0));
		gl_localtime(clearat,&dt);
		if (verbose) gl_output("   ...%s opens for clearing of market_id %d at %s", gl_name(OBJECTHDR(this),name,sizeof(name)), (int32)market_id, gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");
	}

	return -clearat; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP auction::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	return -clearat; /* soft return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

void auction::record_curve(double bu, double su){
	char name[64];
	char timestr[64];
	DATETIME dt;
	unsigned int i = 0;

	gl_localtime(gl_globalclock, &dt);
	gl_strtime(&dt, timestr, 63);

	if(CO_EXTRA == curve_log_info)
		fprintf(curve_file, "# supply curve at %s: %f total and %f unresponsive\n", timestr, cleared_frame.seller_total_quantity, su);
	for (i=0; i<offers.getcount(); i++){
		fprintf(curve_file, "%d,%s,%4d,%s,%.3f,%.6f\n",(int32)market_id,timestr,i,gl_name(offers.getbid(i)->from,name,sizeof(name)), offers.getbid(i)->quantity,offers.getbid(i)->price);
	}

	if(CO_EXTRA == curve_log_info)
		fprintf(curve_file, "# demand curve at %s: %f total and %f unresponsive\n", timestr, cleared_frame.buyer_total_quantity, bu);
	for (i=0; i<asks.getcount(); i++){
		fprintf(curve_file, "%d,%s,%4d,%s,%.3f,%.6f\n",(int32)market_id,timestr,i,gl_name(asks.getbid(i)->from,name,sizeof(name)), -asks.getbid(i)->quantity,asks.getbid(i)->price);
	}

	if(CO_EXTRA == curve_log_info){
		char tstr[256];
		switch(cleared_frame.clearing_type){
			case CT_NULL:
				sprintf(tstr, "null");
				break;
			case CT_SELLER:
				sprintf(tstr, "marginal_seller");
				break;
			case CT_BUYER:
				sprintf(tstr, "marginal_buyer");
				break;
			case CT_PRICE:
				sprintf(tstr, "marginal_price");
				break;
			case CT_EXACT:
				sprintf(tstr, "exact_p_and_q");
				break;
			case CT_FAILURE:
				sprintf(tstr, "failure");
				break;
		}
		fprintf(curve_file, "# marginal quantity of %f %s (%f %%)\n", cleared_frame.marginal_quantity, this->unit.get_string(), cleared_frame.marginal_frac);
		fprintf(curve_file, "# curve cleared %f at $%f with %s type\n", cleared_frame.clearing_quantity, cleared_frame.clearing_price, tstr);
	}

	if(curve_log_max > 0){
		--curve_log_count;
		if (curve_log_count == 0){
			fclose(curve_file);
			curve_file = 0;
		}
	} else {
		fflush(curve_file);
	}
}

void auction::clear_market(void)
{
	unsigned int sph24 = (unsigned int)(3600/period*24);
	BID unresponsive;
	extern double bid_offset;
	double cap_ref_unrep = 0.0;

	memset(&unresponsive, 0, sizeof(unresponsive));

	/* handle unbidding capacity */
	if(capacity_reference_property != NULL && special_mode != MD_FIXED_BUYER){
		char name[256];

		double total_unknown = asks.get_total() - asks.get_total_on() - asks.get_total_off();
		double *pRefload = gl_get_double(capacity_reference_object, capacity_reference_property);
		double refload;
		
		if(pRefload == NULL){
			char msg[256];
			sprintf(msg, "unable to retreive property '%s' from capacity reference object '%s'", capacity_reference_property->name, capacity_reference_object->name);
			throw msg;
			/* TROUBLESHOOT
				The specified capacity reference property cannot be retrieved as a double.  Verify that it is a double type property.
			*/
		} else {
			refload = *pRefload;
		}

		if(strcmp(unit, "") != 0){
			if(capacity_reference_property->unit != 0){
				if(gl_convert(capacity_reference_property->unit->name,unit.get_string(),&refload) == 0){
					char msg[256];
					sprintf(msg, "capacity_reference_property %s uses units of %s and is incompatible with auction units (%s)", gl_name(linkref,name,sizeof(name)), capacity_reference_property->unit->name, unit.get_string());
					throw msg;
					/* TROUBLESHOOT
						If capacity_reference_property has units specified, the units must be convertable to the units used by its auction object.
						*/
				} else if (verbose){
					gl_output("capacity_reference_property converted %.3f %s to %.3f %s", *pRefload, capacity_reference_property->unit->name, refload, unit.get_string());
				}
			} // else assume same units
		}
		if (total_unknown > 0.001){ // greater than one mW ~ allows rounding errors.  Threshold may be one kW given a MW-unit'ed market.
			gl_warning("total_unknown is %.0f -> some controllers are not providing their states with their bids", total_unknown);
		}
		//unresponsive.from = linkref;
		unresponsive.from = capacity_reference_object;
		unresponsive.price = pricecap;
		unresponsive.state = BS_UNKNOWN;
		unresponsive.quantity = (refload - asks.get_total_on() - total_unknown/2); /* estimate load on as 1/2 unknown load */
		cap_ref_unrep = unresponsive.quantity;
		if (unresponsive.quantity < -0.001)
		{
			gl_warning("capacity_reference_property %s has negative unresponsive load--this is probably due to improper bidding", gl_name(linkref,name,sizeof(name)), unresponsive.quantity);
		}
		else if (unresponsive.quantity > 0.001)
		{
			submit_nolock(capacity_reference_object, -unresponsive.quantity, unresponsive.price, -1, BS_ON);
			gl_verbose("capacity_reference_property %s has %.3f unresponsive load", gl_name(linkref,name,sizeof(name)), -unresponsive.quantity);
		}
	}


	/* handle bidding capacity reference */
	if(capacity_reference_object != NULL && special_mode == MD_NONE) {
		double *pCaprefq = gl_get_double(capacity_reference_object, capacity_reference_property);
		double caprefq;
		if(pCaprefq == NULL) {
			char msg[256];
			sprintf(msg, "unable to retreive property '%s' from capacity reference object '%s'", capacity_reference_property->name, capacity_reference_object->name);
			throw msg;
		}
		caprefq = *pCaprefq;
		if(strcmp(unit, "") != 0) {
			if (capacity_reference_property->unit != 0) {
				if(gl_convert(capacity_reference_property->unit->name,unit.get_string(),&caprefq) == 0) {
					char msg[256];
					sprintf(msg, "capacity_reference_property %s uses units of %s and is incompatible with auction units (%s)", capacity_reference_property->name, capacity_reference_property->unit->name, unit.get_string());
					throw msg;
				} else {
					submit_nolock(OBJECTHDR(this), max_capacity_reference_bid_quantity, capacity_reference_bid_price, -1, BS_ON);
					if (verbose) gl_output("Capacity reference object: %s bids %.2f at %.2f", capacity_reference_object->name, max_capacity_reference_bid_quantity, capacity_reference_bid_price);
				}
			}
		}
	}

	double single_quantity = 0.0;
	double single_price = 0.0;
	/* sort the bids */
	switch(special_mode){
		char name[64];
		case MD_SELLERS:
			offers.sort(false);
			if (verbose){
				gl_output("   ...  supply curve");
			}
			for (unsigned int i=0; i<offers.getcount(); i++){
				if (verbose){
					gl_output("   ...  %4d: %s offers %.3f %s at %.2f $/%s",i,gl_name(offers.getbid(i)->from,name,sizeof(name)), offers.getbid(i)->quantity,unit.get_string(),offers.getbid(i)->price,unit.get_string());
				}
			}
			if(fixed_price * fixed_quantity != 0.0){
				gl_warning("fixed_price and fixed_quantity are set in the same single auction market ~ only fixed_price will be used");
			}
			if(fixed_quantity > 0.0){
				for(unsigned int i = 0; i < offers.getcount() && single_quantity < fixed_quantity; ++i){
					single_price = offers.getbid(i)->price;
					single_quantity += offers.getbid(i)->quantity;
				}
				if(single_quantity > fixed_quantity){
					single_quantity = fixed_quantity;
					clearing_type = CT_SELLER;
				} else if(single_quantity == fixed_quantity){
					clearing_type = CT_EXACT;
				} else {
					clearing_type = CT_FAILURE;
					single_quantity = 0.0;
					single_price = offers.getbid(0)->price - bid_offset;
				}
			} else if(fixed_quantity < 0.0){
				GL_THROW("fixed_quantity is negative");
			} else {
				single_price = fixed_price;
				for(unsigned int i = 0; i < offers.getcount(); ++i){
					if(offers.getbid(i)->price <= fixed_price){
						single_quantity += offers.getbid(i)->quantity;
					} else {
						break;
					}
				}
				if(single_quantity > 0.0){
					clearing_type = CT_EXACT;
				} else {
					clearing_type = CT_NULL;
				}
			} 
			next.quantity = single_quantity;
			next.price = single_price;
			break;
		case MD_BUYERS:
			asks.sort(true);
			if (verbose){
				gl_output("   ...  demand curve");
			}
			for (unsigned int i=0; i<asks.getcount(); i++){
				if (verbose){
					gl_output("   ...  %4d: %s asks %.3f %s at %.2f $/%s",i,gl_name(asks.getbid(i)->from,name,sizeof(name)), asks.getbid(i)->quantity,unit.get_string(),asks.getbid(i)->price,unit.get_string());
				}
			}
			if(fixed_price * fixed_quantity != 0.0){
				gl_warning("fixed_price and fixed_quantity are set in the same single auction market ~ only fixed_price will be used");
			}
			if(fixed_quantity > 0.0){
				for(unsigned int i = 0; i < asks.getcount() && single_quantity < fixed_quantity; ++i){
					single_price = asks.getbid(i)->price;
					single_quantity += asks.getbid(i)->quantity;
				}
				if(single_quantity > fixed_quantity){
					single_quantity = fixed_quantity;
					clearing_type = CT_BUYER;
				} else if(single_quantity == fixed_quantity){
					clearing_type = CT_EXACT;
				} else {
					clearing_type = CT_FAILURE;
					single_quantity = 0.0;
					single_price = asks.getbid(0)->price + bid_offset;
				}
			} else if(fixed_quantity < 0.0){
				GL_THROW("fixed_quantity is negative");
			} else {
				single_price = fixed_price;
				for(unsigned int i = 0;  i < asks.getcount(); ++i){
					if(asks.getbid(i)->price >= fixed_price){
						single_quantity += asks.getbid(i)->quantity;
					} else {
						break;
					}
				}
				if(single_quantity > 0.0){
					clearing_type = CT_EXACT;
				} else {
					clearing_type = CT_NULL;
				}
			}
			next.quantity = single_quantity;
			next.price = single_price;
			break;
		case MD_FIXED_SELLER:
			offers.sort(false);
			if(asks.getcount() > 0){
				gl_warning("Seller-only auction was given purchasing bids");
			}
			asks.clear();
			submit(OBJECTHDR(this), -fixed_quantity, fixed_price, -1, BS_ON);
			break;
		case MD_FIXED_BUYER:
			asks.sort(true);
			if(offers.getcount() > 0){
				gl_warning("Buyer-only auction was given offering bids");
			}
			offers.clear();
			submit(OBJECTHDR(this), fixed_quantity, fixed_price, -1, BS_ON);
			break;
		case MD_NONE:
			offers.sort(false);
			asks.sort(true);
			break;
	}

	if(special_mode == MD_SELLERS || special_mode == MD_BUYERS){
		char name[64];
		char buffer[256];
		DATETIME dt;
		TIMESTAMP submit_time = gl_globalclock;
		gl_localtime(submit_time,&dt);
		if (verbose){
			gl_output("   ...  %s clears %.2f %s at $%.2f/%s at %s", gl_name(OBJECTHDR(this),name,sizeof(name)),
				next.quantity, unit.get_string(), next.price, unit.get_string(), gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");
		}
	} else if ((asks.getcount()>0) && offers.getcount()>0)
	{
		TIMESTAMP submit_time = gl_globalclock;
		DATETIME dt;
		gl_localtime(submit_time,&dt);
		char buffer[256];
		/* clear market */
		unsigned int i=0, j=0;
		BID *buy = asks.getbid(i), *sell = offers.getbid(j);
		BID clear = {NULL,0,0};
		double demand_quantity = 0, supply_quantity = 0;
		double a=this->pricecap, b=-pricecap;
		bool check=false;
		
		// dump curves
		if (1)
		{
			char name[64];
			unresponsive_sell = 0.0;
			unresponsive_buy = 0.0;
			responsive_sell = 0.0; //
			responsive_buy = 0.0; //
			if (verbose){
				gl_output("   ...  supply curve");
			}
			for (i=0; i<offers.getcount(); i++){
				if (verbose){
					gl_output("   ...  %4d: %s offers %.3f %s at %.2f $/%s",i,gl_name(offers.getbid(i)->from,name,sizeof(name)), offers.getbid(i)->quantity,unit.get_string(),offers.getbid(i)->price,unit.get_string());
				}
				if(offers.getbid(i)->price == -pricecap){
					unresponsive_sell += offers.getbid(i)->quantity;
				} else {
					responsive_sell += offers.getbid(i)->quantity;
				}
			}
			total_sell = responsive_sell + unresponsive_sell;
			if (verbose){
				gl_output("   ...  demand curve");
			}
			for (i=0; i<asks.getcount(); i++){
				if (verbose){
					gl_output("   ...  %4d: %s asks %.3f %s at %.2f $/%s",i,gl_name(asks.getbid(i)->from,name,sizeof(name)), asks.getbid(i)->quantity,unit.get_string(),asks.getbid(i)->price,unit.get_string());
				}
				if(asks.getbid(i)->price == pricecap){
					unresponsive_buy += asks.getbid(i)->quantity;
				} else {
					responsive_buy += asks.getbid(i)->quantity;
				}
			}

			total_buy = responsive_buy + unresponsive_buy;
			
		}

		i = j = 0;
		clearing_type = CT_NULL;
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
				clearing_type = CT_BUYER;
			}
			else if (buy_quantity < sell_quantity)
			{
				clear.quantity = demand_quantity = buy_quantity;
				a = b = sell->price;
				buy = asks.getbid(++i);
				check = false;
				clearing_type = CT_SELLER;
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
	
		if(a == b){
			clear.price = a;
		}
		if(check){ /* there was price agreement or quantity disagreement */
			clear.price = a;
			if(supply_quantity == demand_quantity){
				if(i == asks.getcount() || j == offers.getcount()){
					if(i == asks.getcount() && j == offers.getcount()){ // both sides exhausted at same quantity
						if(a == b){
							clearing_type = CT_EXACT;
						} else {
							clearing_type = CT_PRICE;
						}
					} else if (i == asks.getcount() && b == sell->price){ // exhausted buyers, sellers unsatisfied at same price
						clearing_type = CT_SELLER;
					} else if (j == offers.getcount() && a == buy->price){ // exhausted sellers, buyers unsatisfied at same price
						clearing_type = CT_BUYER;
					} else { // both sides satisfied at price, but one side exhausted
						if(a == b){
							clearing_type = CT_EXACT;
						} else {
							clearing_type = CT_PRICE;
						}
					}
				} else {
					if(a != buy->price && b != sell->price && a == b){
						clearing_type = CT_EXACT; // price changed in both directions
					} else if (a == buy->price && b != sell->price){
						// sell price increased ~ marginal buyer since all sellers satisfied
						clearing_type = CT_BUYER;
					} else if (a != buy->price && b == sell->price){
						// buy price increased ~ marginal seller since all buyers satisfied
						clearing_type = CT_SELLER;
						clear.price = b; // use seller's price, not buyer's price
					} else if(a == buy->price && b == sell->price){
						// possible when a == b, q_buy == q_sell, and either the buyers or sellers are exhausted
						if(i == asks.getcount() && j == offers.getcount()){
							clearing_type = CT_EXACT;
						} else if (i == asks.getcount()){ // exhausted buyers
							clearing_type = CT_SELLER;
						} else if (j == offers.getcount()){ // exhausted sellers
							clearing_type = CT_BUYER;
						}
					} else {
						clearing_type = CT_PRICE; // marginal price
					}
				}
			}
			if(clearing_type == CT_PRICE){
				double avg, dHigh, dLow;
				avg = (a+b) / 2;
				dHigh = (i == asks.getcount() ? a : buy->price);
				dLow = (j == offers.getcount() ? b : sell->price);
				// needs to be just off such that it does not trigger any other bids
				if(a == pricecap && b != -pricecap){
					clear.price = (buy->price > b ? buy->price + bid_offset : b);
				} else if(a != pricecap && b == -pricecap){
					clear.price = (sell->price < a ? sell->price - bid_offset : a);
				} else if(a == pricecap && b == -pricecap){
					if(i == asks.getcount() && j == offers.getcount()){
						clear.price = 0; // no additional bids on either side
					} else if(i == asks.getcount()){ // buyers left
						clear.price = buy->price + bid_offset;
					} else if(j == asks.getcount()){ // sellers left
						clear.price = sell->price - bid_offset;
					} else { // additional bids on both sides, just no clearing
						clear.price = (dHigh + dLow) / 2;
					}
				} else {
					if(i != asks.getcount() && buy->price == a){
						clear.price = a;
					} else if (j != offers.getcount() && sell->price == b){
						clear.price = b;
					} else if(i != asks.getcount() && avg < buy->price){
						clear.price = dHigh + bid_offset;
					} else if(j != offers.getcount() && avg > sell->price){
						clear.price = dLow - bid_offset;
					} else {
						clear.price = avg;
					}
				}
			}
		}
	
		/* check for zero demand but non-zero first unit sell price */
		if (clear.quantity==0)// && offers.getcount()>0)
		{
			clearing_type = CT_NULL;
			if(offers.getcount() > 0 && asks.getcount() == 0){
				clear.price = offers.getbid(0)->price - bid_offset;
			} else if(offers.getcount() == 0 && asks.getcount() > 0){
				clear.price = asks.getbid(0)->price + bid_offset;
			} else {
				if(offers.getbid(0)->price == pricecap){
					clear.price = asks.getbid(0)->price + bid_offset;
				} else if (asks.getbid(0)->price == -pricecap){
					clear.price = offers.getbid(0)->price - bid_offset;
				} else {
					clear.price = offers.getbid(0)->price + (asks.getbid(0)->price - offers.getbid(0)->price) * clearing_scalar;
				}
			}
			
		} else if (clear.quantity < unresponsive_buy){
			clearing_type = CT_FAILURE;
			clear.price = pricecap;
		} else if (clear.quantity < unresponsive_sell){
			clearing_type = CT_FAILURE;
			clear.price = -pricecap;
		} else if (clear.quantity == unresponsive_buy && clear.quantity == unresponsive_sell){
			// only cleared unresponsive loads
			clearing_type = CT_PRICE;
			clear.price = 0.0;
		}
	
		/* post the price */
		char name[64];
		if (verbose) gl_output("   ...  %s clears %.2f %s at $%.2f/%s at %s", gl_name(OBJECTHDR(this),name,sizeof(name)), clear.quantity, unit.get_string(), clear.price, unit.get_string(), gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");
		next.price = clear.price;
		next.quantity = clear.quantity;
	}
	else
	{
		char name[64];
		if(offers.getcount() > 0 && asks.getcount() == 0){
			next.price = offers.getbid(0)->price - bid_offset;
		} else if(offers.getcount() == 0 && asks.getcount() > 0){
			next.price = asks.getbid(0)->price + bid_offset;
		} else if(asks.getcount() > 0 && offers.getcount() > 0){
			next.price = offers.getbid(0)->price + (asks.getbid(0)->price - offers.getbid(0)->price) * clearing_scalar;
		} else if(asks.getcount() == 0 && offers.getcount() == 0){
			next.price = 0.0;
		}
		next.quantity = 0;
		clearing_type = CT_NULL;
		gl_warning("market '%s' fails to clear due to missing %s", gl_name(OBJECTHDR(this),name,sizeof(name)), asks.getcount()==0?(offers.getcount()==0?"buyers and sellers":"buyers"):"sellers");
	}
	
	double marginal_total = 0.0;
	double marginal_quantity = 0.0;
	double marginal_frac = 0.0;
	if(clearing_type == CT_BUYER){
		unsigned int i = 0;
		double marginal_subtotal = 0.0;
		for(i = 0; i < asks.getcount(); ++i){
			if(asks.getbid(i)->price > next.price){
				marginal_subtotal += asks.getbid(i)->quantity;
			} else {
				break;
			}
		}
		marginal_quantity = next.quantity - marginal_subtotal;
		for(; i < asks.getcount(); ++i){
			if(asks.getbid(i)->price == next.price)
				marginal_total += asks.getbid(i)->quantity;
			else
				break;
		}
		marginal_frac = marginal_quantity / marginal_total;
	} else if (clearing_type == CT_SELLER){
		unsigned int i = 0;
		double marginal_subtotal = 0.0;
		for(i = 0; i < offers.getcount(); ++i){
			if(offers.getbid(i)->price < next.price){
				marginal_subtotal += offers.getbid(i)->quantity;
			} else {
				break;
			}
		}
		marginal_quantity = next.quantity - marginal_subtotal;
		for(; i < offers.getcount(); ++i){
			if(offers.getbid(i)->price == next.price)
				marginal_total += offers.getbid(i)->quantity;
			else
				break;
		}	
		marginal_frac = marginal_quantity / marginal_total;
	} else {
		marginal_quantity = 0.0;
		marginal_frac = 0.0;
	}

	if(history_count > 0){
		if(price_index == history_count){
			price_index = 0;
		}
		new_prices[price_index] = next.price;
		//int update_rv = update_statistics();
		++price_index;
	}

	/* limit price */
	if (next.price<-pricecap) next.price = -pricecap;
	else if (next.price>pricecap) next.price = pricecap;

	// update cleared_frame data
	cleared_frame.market_id = this->market_id;
	cleared_frame.start_time = gl_globalclock + latency;
	cleared_frame.end_time = gl_globalclock + latency + period;
	cleared_frame.clearing_price = next.price;
	cleared_frame.clearing_quantity = next.quantity;
	cleared_frame.clearing_type = clearing_type;
	cleared_frame.marginal_quantity = marginal_quantity;
	cleared_frame.total_marginal_quantity = marginal_total;
	cleared_frame.buyer_total_quantity = asks.get_total();
	cleared_frame.seller_total_quantity = offers.get_total();
	cleared_frame.seller_min_price = offers.get_min();
	cleared_frame.marginal_frac = marginal_frac;
	cleared_frame.buyer_total_unrep = unresponsive_buy;
	cleared_frame.cap_ref_unrep = cap_ref_unrep;

	if(latency > 0){
		TIMESTAMP rt;
		rt = pop_market_frame(gl_globalclock);
		update_statistics();
		push_market_frame(gl_globalclock);
		check_next_market(gl_globalclock);
	} else {
		STATISTIC *stat = 0;
		OBJECT *obj = OBJECTHDR(this);
		memcpy(&past_frame, &current_frame, sizeof(MARKETFRAME)); // just the frame
		// ~ copy new data in
		current_frame.market_id = cleared_frame.market_id;
		current_frame.start_time = cleared_frame.start_time;
		current_frame.end_time = cleared_frame.end_time;
		current_frame.clearing_price = cleared_frame.clearing_price;
		current_frame.clearing_quantity = cleared_frame.clearing_quantity;
		current_frame.clearing_type = cleared_frame.clearing_type;
		current_frame.marginal_quantity = cleared_frame.marginal_quantity;
		current_frame.total_marginal_quantity = cleared_frame.total_marginal_quantity;
		current_frame.seller_total_quantity = cleared_frame.seller_total_quantity;
		current_frame.buyer_total_quantity = cleared_frame.buyer_total_quantity;
		current_frame.seller_min_price = cleared_frame.seller_min_price;
		current_frame.marginal_frac = cleared_frame.marginal_frac;
		current_frame.buyer_total_unrep = cleared_frame.buyer_total_unrep;
		current_frame.cap_ref_unrep = cleared_frame.cap_ref_unrep;
		++total_samples;
		update_statistics();
		
	}

	// record the results
	if(curve_file != 0){
		record_curve(unresponsive_buy, unresponsive_sell);
	}

	/* clear the bid lists */
	asks.clear();
	offers.clear();

	if(trans_file){
		fflush(trans_file);
	}
}

void auction::record_bid(OBJECT *from, double quantity, double real_price, BIDDERSTATE state){
	char name_buffer[256];
	char *unkState = "unknown";
	char *offState = "off";
	char *onState = "on";
	char *unk = "unknown time";
	char buffer[256];
	char bigbuffer[1024];
	char *pState;
	char *tStr;
	DATETIME dt;
	TIMESTAMP submit_time = gl_globalclock;
	if(trans_file){ // copied from version below
		if((this->trans_log_max <= 0) || (trans_log_count > 0)){
			gl_localtime(submit_time,&dt);
			switch(state){
				case BS_UNKNOWN:
					pState = unkState;
					break;
				case BS_OFF:
					pState = offState;
					break;
				case BS_ON:
					pState = onState;
					break;
			}
			tStr = (gl_strtime(&dt,buffer,sizeof(buffer)) ? buffer : unk);
			sprintf(bigbuffer, "%d,%s,%s,%f,%f,%s", (int32)market_id, tStr, gl_name(from, name_buffer, 256), real_price, quantity, pState);
			fprintf(trans_file, "%s\n", bigbuffer);
			--trans_log_count;
		} else {
			fprintf(trans_file, "# end of file \n");
			fclose(trans_file);
			trans_file = 0;
		}
	}
}

KEY auction::submit(OBJECT *from, double quantity, double real_price, KEY key, BIDDERSTATE state)
{
	gld_wlock lock(my());
	return submit_nolock(from,quantity,real_price,key,state);
}
KEY auction::submit_nolock(OBJECT *from, double quantity, double real_price, KEY key, BIDDERSTATE state)
{
	char myname[64];
	TIMESTAMP submit_time = gl_globalclock;
	DATETIME dt;
	double price;
	gl_localtime(submit_time,&dt);
	char buffer[256];
	BIDDEF biddef;

	/* suppress demand bidding until market stabilizes */
	unsigned int sph24 = (unsigned int)(3600/period*24);
	if(real_price > pricecap){
		gl_warning("%s received a bid above the price cap, truncating", gl_name(OBJECTHDR(this),myname,sizeof(myname)));
		price = pricecap;
	} else {
		price = real_price;
	}
	if (total_samples<sph24 && quantity<0 && warmup)
	{
		if (verbose) gl_output("   ...  %s ignoring demand bid during first 24 hours", gl_name(OBJECTHDR(this),myname,sizeof(myname)));
		return -1;
	}

	/* translate key */
	if(key == -1 || key == 0xccccccccffffffffULL){ // new bid ~ rebuild key
		//write_bid(key, market_id, -1, BID_UNKNOWN);
		biddef.bid = -1;
		biddef.bid_type = BID_UNKNOWN;
		biddef.market = -1;
		biddef.raw = -1;
	} else {
		if((key & 0xFFFFFFFF00000000ULL) == 0xCCCCCCCC00000000ULL){
			key &= 0x00000000FFFFFFFFULL;
		}
		translate_bid(biddef, key);
	}

	if (biddef.market > market_id)
	{	// future market
		gl_error("bidding into future markets is not yet supported");
		/* TROUBLESHOOT
			Tracking bids input markets other than the immediately open one will be supported in the future.
			*/
	}
	else if (biddef.market == market_id) // resubmit
	{
		char biddername[64];
		KEY out;
		if (verbose){
			gl_output("   ...  %s resubmits %s from object %s for %.2f %s at $%.2f/%s at %s", 
				gl_name(OBJECTHDR(this),myname,sizeof(myname)), quantity<0?"ask":"offer", gl_name(from,biddername,sizeof(biddername)), 
				fabs(quantity), unit.get_string(), price, unit.get_string(), gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");
		}
		BID bid = {from,fabs(quantity),price,state};
		if (biddef.bid_type == BID_BUY){
			out = asks.resubmit(&bid,biddef.bid);
		} else if (biddef.bid_type == BID_SELL){
			out = offers.resubmit(&bid,biddef.bid);
		} else {
			;
		}
		record_bid(from, quantity, real_price, state);
		return biddef.raw;
	}
	else if (biddef.market < 0 || biddef.bid_type == BID_UNKNOWN){
		char myname[64];
		char biddername[64];
		KEY out;
		if (verbose){
			gl_output("   ...  %s receives %s from object %s for %.2f %s at $%.2f/%s at %s", 
				gl_name(OBJECTHDR(this),myname,sizeof(myname)), quantity<0?"ask":"offer", gl_name(from,biddername,sizeof(biddername)), 
				fabs(quantity), unit.get_string(), price, unit.get_string(), gl_strtime(&dt,buffer,sizeof(buffer))?buffer:"unknown time");
		}
		BID bid = {from,fabs(quantity),price,state};
		if (quantity<0){
			out = asks.submit(&bid);
		} else if (quantity>0){
			out = offers.submit(&bid);
		} else {
			char name[64];
			gl_debug("zero quantity bid from %s is ignored", gl_name(from,name,sizeof(name)));
			return -1;
		}
		biddef.bid = (int16)out;
		biddef.market = market_id;
		biddef.bid_type = (quantity > 0 ? BID_SELL : BID_BUY);
		write_bid(out, biddef.market, biddef.bid, biddef.bid_type);
		// interject transaction log file writing here
		record_bid(from, quantity, real_price, state);
		biddef.raw = out;
		return biddef.raw;
	} else { // key between cleared market and 'market_id' ~ points to an old market
		if(verbose){
			char myname[64];
			char biddername[64];
			gl_output(" ... %s receives %s from object %s for a previously cleared market",
				gl_name(OBJECTHDR(this),myname,sizeof(myname)),quantity<0?"ask":"offer",
				gl_name(from,biddername,sizeof(biddername)));
		}
		return 0;
	}
	return 0;
}

TIMESTAMP auction::nextclear(void) const
{
	return gl_globalclock + (TIMESTAMP)(period - (gl_globalclock+period) % period);
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
		else
			return 0;
	}
	CREATE_CATCHALL(auction);
}

EXPORT int init_auction(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,auction)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(auction);
}

EXPORT int isa_auction(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,auction)->isa(classname);
	} else {
		return 0;
	}
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
		return t2;
	}
	SYNC_CATCHALL(auction);
}

