/// $Id$
/// @file scheduler.h

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

// scheduling methods
#define SM_NONE 0x00 ///< no scheduling
#define SM_PLAY 0x01 ///< schedule played in
#define SM_MATPOWER 0x02 ///< schedule from matpower (must be installed)

// scheduling options
#define SO_NONE 0x00 ///< no options set
#define SO_NORAMP 0x01 ///< dispatcher do not ramp schedules (ramptime ignored)
#define SO_ALL 0x01 ///< all options set

class scheduler : public gld_object {
public:
	GL_ATOMIC(double,interval); ///< scheduling interval (s)
	GL_ATOMIC(double,ramptime); ///< dispatch ramp time (s)
	GL_BITFLAGS(enumeration,method); ///< scheduling method
	GL_BITFLAGS(set,options); ///< scheduling options
	GL_ATOMIC(double,supply); ///< scheduled supply (MW)
	GL_ATOMIC(double,demand); ///< scheduled demand (MW)
	GL_ATOMIC(double,losses); ///< schedule losses (MW)
	GL_ATOMIC(double,cost); ///< base supply and losses cost ($/h)
	GL_ATOMIC(double,price); ///< base demand price ($/MWh)
private:
	gld_object *system; ///< interconnection for this scheduler
	double supply_ramp; ///< generator ramp rate [MW/s]
	double demand_ramp; ///< demand response ramp rate [MW/s]
	double supply_cost; ///< cost of supply ($/h)
	double losses_cost; ///< cost of losses ($/h)
private:
	void schedule_play(TIMESTAMP t1);
	void schedule_matpower(TIMESTAMP t1);
public:
// required
	DECL_IMPLEMENT(scheduler);
	DECL_SYNC;
}; ///< scheduler class

#endif
