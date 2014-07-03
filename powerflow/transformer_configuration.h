// $Id: transformer_configuration.h 4738 2014-07-03 00:55:39Z dchassin $
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
	enum {WYE_WYE=1, DELTA_DELTA, DELTA_GWYE, SINGLE_PHASE, SINGLE_PHASE_CENTER_TAPPED};
	enumeration connect_type;   ///< connect type enum: Wye-Wye, single-phase, etc.
	enum {POLETOP=1, PADMOUNT, VAULT};
	enumeration install_type; ///< Defines location of the transformer installation
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
	double RX;						// the reactance to resistance ratio
	// thermal model input
	enum {MINERAL_OIL=1, DRY=2};
	enumeration coolant_type; ///< coolant type, used in life time model
	enum {OA=1, FA=2, NDFOA=3, NDFOW=4, DFOA=5, DFOW=6};
	enumeration cooling_type; ///< type of coolant used in life time model
	double core_coil_weight;		// The weight of the core and coil assembly in pounds.
	double tank_fittings_weight;	// The weight of the tank and fittings in pounds.
	double oil_vol;					// The number of gallons of oil in the transformer.
	double t_W;						// The rated winding time constant in hours.
	double dtheta_TO_R;			// top-oil hottest-spot rise over ambient temperature at rated load, degrees C.
	double dtheta_H_AR;			// winding hottest-spot rise over ambient temperature at rated load, degrees C.
	double installed_insulation_life;	//the normal lifetime of the transformer insulation at rated load, hours.

	transformer_configuration(MODULE *mod);
	inline transformer_configuration(CLASS *cl=oclass):powerflow_library(cl){};
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
};

#endif // _TRANSFORMERCONFIGURATION_H
