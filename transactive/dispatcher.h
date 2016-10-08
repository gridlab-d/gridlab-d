// $Id$

#include "gridlabd.h"

#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

class dispatcher : public gld_object {
public:

private:
	double supply_ramp; ///< generator ramp rate [MW/s]
	double demand_ramp; ///< demand response ramp rate [MW/s]
public:
// required
	DECL_IMPLEMENT(dispatcher);
	DECL_SYNC;
};

#endif
