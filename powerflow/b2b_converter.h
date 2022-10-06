/** $Id: b2b_converter.h
	Copyright (C) 2022 Battelle Memorial Institute
	@file b2b_converter.h
 @{
 **/

#ifndef B2B_CONVERTER_H
#define B2B_CONVERTER_H

#include "powerflow.h"
#include "link.h"
#include "node.h"

EXPORT STATUS current_injection_update_b2bc(OBJECT *obj);
EXPORT SIMULATIONMODE interupdate_b2bc(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
EXPORT STATUS postupdate_b2bc(OBJECT *obj);

class b2bc : public link_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;

public:
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);	//Legacy FBS coding in here - may not be needed in future
	b2bc(MODULE *mod);
	inline b2bc(CLASS *cl=oclass):link_object(cl){};
	int isa(char *classname);

	STATUS alloc_freq_arrays(double delta_t_val);
	void CheckParameters(void);
	complex complex_exp(double angle);
	STATUS b2bc_current_injection(void);
	
	SIMULATIONMODE inter_deltaupdate_b2bc(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
	STATUS post_deltaupdate_b2bc(void);

public:
	double ratedRPM;
	double motorPoles;
	double desiredRPM;
	double stableTime;
	double voltageLLRating;
	double horsePowerRatedb2bc;
	double nominal_output_frequency;
	double settleTime;

	enum {
		b2bc_OFF=0,			/**< b2bc is off */
		b2bc_CHANGING=1,		/**< b2bc is changing frequency */
		b2bc_STEADY=2	/**< b2bc has reached desired frequency */
	};
	enumeration b2bcState;
	enumeration prev_b2bcState;

	double currEfficiency;
	double desiredFrequency;
	double currentFrequency;

private:
	node *fNode;
	node *tNode;

	double *settleFreq;
	unsigned int settleFreq_length;
	unsigned int curr_array_position;
	bool force_array_realloc;

	double nominal_output_radian_freq;
	double VbyF;
	double HPbyF;

	double phasorVal[3];
	complex prev_power[3];
	double prev_desiredRPM;
	double efficiency_coeffs[8];
	double curr_time_value;
	double prev_time_value;
};

#endif // B2B_CONVERTER_H
/**@}**/

