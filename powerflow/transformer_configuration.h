// $Id: transformer_configuration.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRANSFORMERCONFIGURATION_H
#define _TRANSFORMERCONFIGURATION_H

#include "powerflow.h"
#include "powerflow_library.h"
#include "link.h"

class transformer_configuration : public powerflow_library
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	enum {WYE_WYE=1, DELTA_DELTA, DELTA_GWYE, SINGLE_PHASE, SINGLE_PHASE_CENTER_TAPPED} connect_type;   // connect type enum: Wye-Wye, single-phase, etc.
	enum {POLETOP=1, PADMOUNT, VAULT} install_type;
	double V_primary;     // primary voltage level in L-L value kV
	double V_secondary;   // secondary voltage level kV
	double kVA_rating;     // kVA rating of transformer  @todo remove when everything is implemented for separate phase ratings.
	double phaseA_kVA_rating;     // kVA rating of transformer
	double phaseB_kVA_rating;     // kVA rating of transformer
	double phaseC_kVA_rating;     // kVA rating of transformer
	complex impedance;				// Series impedance
	complex impedance1;				// Series impedance (only used when you want to define each individual 
	complex impedance2;				//    winding seperately
	complex shunt_impedance;		// Shunt impedance - all values are summed and reflected back to the primary
	double no_load_loss;			// Another method of specifying transformer impedances
	double full_load_loss;			//  -- both are defined as unit values
	
	transformer_configuration(MODULE *mod);
	inline transformer_configuration(CLASS *cl=oclass):powerflow_library(cl){};
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
};

#endif // _TRANSFORMERCONFIGURATION_H
