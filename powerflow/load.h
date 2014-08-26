// $Id: load.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _LOAD_H
#define _LOAD_H

#include "node.h"

EXPORT SIMULATIONMODE interupdate_load(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class load : public node
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	complex measured_voltage_A;	///< measured voltage
	complex measured_voltage_B;
	complex measured_voltage_C;
	complex measured_voltage_AB;	
	complex measured_voltage_BC;
	complex measured_voltage_CA;
	complex constant_power[3];		// power load
	complex constant_current[3];	// current load
	complex constant_impedance[3];	// impedance load
	complex constant_power_dy[6];		// Power load, explicitly specified wye and delta -- delta first, then wye
	complex constant_current_dy[6];	// Current load, explicitly specified wye and delta -- delta first, then wye
	complex constant_impedance_dy[6];	// Impedance load, explicitly specified wye and delta -- delta first, then wye

	double base_power[3];
	double power_pf[3];
	double current_pf[3];
	double impedance_pf[3];
	double power_fraction[3];
	double impedance_fraction[3];
	double current_fraction[3];
	bool three_phase_protect;

	enum {LC_UNKNOWN=0, LC_RESIDENTIAL, LC_COMMERCIAL, LC_INDUSTRIAL, LC_AGRICULTURAL};
	enumeration load_class;

	int create(void);
	int init(OBJECT *parent);
	
	void load_update_fxn(bool fault_mode);
	SIMULATIONMODE inter_deltaupdate_load(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

	load(MODULE *mod);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	TIMESTAMP presync(TIMESTAMP t0);
	inline load(CLASS *cl=oclass):node(cl){};
	int isa(char *classname);
	int notify(int update_mode, PROPERTY *prop, char *value);
};

#endif // _LOAD_H

