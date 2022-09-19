// $Id: triplex_load.h 1182 2012-7-1 2:13 PDT fish334 $
//	Copyright (C) 2013 Battelle Memorial Institute

#ifndef _TRIPLEXLOAD_H
#define _TRIPLEXLOAD_H

#include "triplex_node.h"

EXPORT SIMULATIONMODE interupdate_triplex_load(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
EXPORT STATUS update_triplex_load_values(OBJECT *obj);

class triplex_load : public triplex_node
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
private:
	bool base_load_val_was_nonzero[3];		///< Tracking variable to make ZIP-fraction loads check for zero conditions (but not already zeroed)
	bool ZIP_constant_current[3];			///< Tracking variable for if ZIP-fraction loads have overwritten constant_current_X

	gld::complex prev_load_values[3][3];			///< Tracking variable for accumulators - make loads behave more like nodes

public:
	gld::complex measured_voltage_1;	///< measured voltage
	gld::complex measured_voltage_2;
	gld::complex measured_voltage_12;
	gld::complex measured_total_power;
	gld::complex measured_power[3];
	gld::complex constant_power[3];		// power load
	gld::complex constant_current[3];	// current load
	gld::complex constant_impedance[3];	// impedance load

	double base_power[3];
	double power_pf[3];
	double current_pf[3];
	double impedance_pf[3];
	double power_fraction[3];
	double impedance_fraction[3];
	double current_fraction[3];

	enum {LC_UNKNOWN=0, LC_RESIDENTIAL, LC_COMMERCIAL, LC_INDUSTRIAL, LC_AGRICULTURAL};
	enumeration load_class;
	enum {DISCRETIONARY=0, PRIORITY, CRITICAL};
	enumeration load_priority;

	int create(void);
	int init(OBJECT *parent);
	
	void triplex_load_update_fxn(void);
	void triplex_load_delete_update_fxn(void);

	triplex_load(MODULE *mod);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	TIMESTAMP presync(TIMESTAMP t0);
	inline triplex_load(CLASS *cl=oclass):triplex_node(cl){};
	int isa(char *classname);

	SIMULATIONMODE inter_deltaupdate_triplex_load(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
};

#endif // _TRIPLEX_LOAD_H

