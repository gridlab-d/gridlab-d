/** $Id: stubauction.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file stubauction.h
	@addtogroup auction
	@ingroup market

 @{
 **/

#ifndef _stubauction_H
#define _stubauction_H

#include <stdarg.h>
#include "gridlabd.h"

class stubauction : public gld_object {
public:
	bool verbose;
protected:
public:
	char32 unit;		/**< unit of quantity (see unitfile.txt) */
	double period;		/**< time period of auction closing (s) */
	double latency;		/**< delay after closing before unit commitment (s) */
	int64 market_id;	/**< id of market to clear */
	TIMESTAMP clearat;	/**< next clearing time */
	TIMESTAMP checkat;	/**< next price check time */

	double last_price;
	double next_price;
	double clear_price;
	double avg24;		/**< daily average of price */
	double std24;		/**< daily stdev of price */
	double avg72;
	double std72;
	double avg168;		/**< weekly average of price */
	double std168;		/**< weekly stdev of price */
	double prices[168]; /**< price history */
	int64 count;		/**< number of prices in history */
	int16 lasthr, thishr;
	typedef enum {
		CON_NORMAL=0,
		CON_DISABLED=1,
	} AVGCONTROL;
	enumeration control_mode;
	int retry;
public:
	TIMESTAMP nextclear() const;
public:
	/* required implementations */
	stubauction(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static stubauction *defaults;
};

#endif

/**@}*/
