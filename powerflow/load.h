// $Id: load.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _LOAD_H
#define _LOAD_H

#include "node.h"

EXPORT SIMULATIONMODE interupdate_load(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
EXPORT STATUS update_load_values(OBJECT *obj);

//KML export
EXPORT int load_kmldata(OBJECT *obj,int (*stream)(const char*,...));

class load : public node
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
private:
	gld::complex prev_shunt[3];

	bool base_load_val_was_nonzero[3];		///< Tracking variable to make ZIP-fraction loads check for zero conditions (but not already zeroed)

	gld::complex prev_load_values[3][3];			///< Tracking variable for accumulators - make loads behave more like nodes
	gld::complex prev_load_values_dy[3][6];		///< Tracking variable for accumulators - full connection - make loads behave more like nodes.

public:
	gld::complex measured_voltage_A;	///< measured voltage
	gld::complex measured_voltage_B;
	gld::complex measured_voltage_C;
	gld::complex measured_voltage_AB;	
	gld::complex measured_voltage_BC;
	gld::complex measured_voltage_CA;
	gld::complex measured_total_power;
	gld::complex measured_power[3];
	gld::complex constant_power[3];		// power load
	gld::complex constant_current[3];	// current load
	gld::complex constant_impedance[3];	// impedance load
	gld::complex constant_power_dy[6];		// Power load, explicitly specified wye and delta -- delta first, then wye
	gld::complex constant_current_dy[6];	// Current load, explicitly specified wye and delta -- delta first, then wye
	gld::complex constant_impedance_dy[6];	// Impedance load, explicitly specified wye and delta -- delta first, then wye
	INRUSHINTMETHOD inrush_int_method_inductance;	//Individual mode selection
	INRUSHINTMETHOD inrush_int_method_capacitance;

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
	enum {DISCRETIONARY=0, PRIORITY, CRITICAL};
	enumeration load_priority;

	int create(void);
	int init(OBJECT *parent);
	
	void load_update_fxn(void);
	void load_delete_update_fxn(void);
	SIMULATIONMODE inter_deltaupdate_load(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

	load(MODULE *mod);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	TIMESTAMP presync(TIMESTAMP t0);
	inline load(CLASS *cl=oclass):node(cl){};
	int isa(char *classname);
	int notify(int update_mode, PROPERTY *prop, char *value);
	int kmldata(int (*stream)(const char*,...));
};

#endif // _LOAD_H
