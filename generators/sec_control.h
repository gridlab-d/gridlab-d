#ifndef GLD_GENERATORS_SEC_CONTROL_H_
#define GLD_GENERATORS_SEC_CONTROL_H_

#include <vector>

#include "generators.h"

EXPORT int isa_sec_control(OBJECT *obj, char *classname);
EXPORT STATUS preupdate_sec_control(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time);
EXPORT SIMULATIONMODE interupdate_sec_control(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
EXPORT STATUS postupdate_sec_control(OBJECT *obj, complex *useful_value, unsigned int mode_pass);

//sec_control class
class sec_control : public gld_object
{
private:
	bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags
	bool first_sync_delta_enabled;

	double prev_timestamp_dbl;	//deltamode - tracks last timestep for reiterations and time tracking
	SIMULATIONMODE desired_simulation_mode; //deltamode desired simulation mode after corrector pass - prevents starting iterations again

	////**************** Sample variable - published property for sake of doing so ****************//
	complex test_published_property;

public:
	/* utility functions */
	gld_property *map_complex_value(OBJECT *obj, char *name);
	gld_property *map_double_value(OBJECT *obj, char *name);

	/* required implementations */
	sec_control(MODULE *module);
	int isa(char *classname);

	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	STATUS pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time);
	SIMULATIONMODE inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val);
	STATUS post_deltaupdate(complex *useful_value, unsigned int mode_pass);

public:
	static CLASS *oclass;
	static sec_control *defaults;
	static CLASS *plcass;
};

#endif // GLD_GENERATORS_SEC_CONTROL_H_
