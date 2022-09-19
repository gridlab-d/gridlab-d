/** $Id: vfd.h
	Copyright (C) 2017 Battelle Memorial Institute
	@file vfd.h
 @{
 **/

#ifndef VFD_H
#define VFD_H

#include "powerflow.h"
#include "link.h"
#include "node.h"

EXPORT STATUS current_injection_update_VFD(OBJECT *obj);
EXPORT SIMULATIONMODE interupdate_vfd(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
EXPORT STATUS postupdate_vfd(OBJECT *obj);

class vfd : public link_object
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
	vfd(MODULE *mod);
	inline vfd(CLASS *cl=oclass):link_object(cl){};
	int isa(char *classname);

	STATUS alloc_freq_arrays(double delta_t_val);
	void CheckParameters(void);
	gld::complex complex_exp(double angle);
	STATUS VFD_current_injection(void);
	
	SIMULATIONMODE inter_deltaupdate_vfd(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
	STATUS post_deltaupdate_vfd(void);

public:
	double ratedRPM;
	double motorPoles;
	double desiredRPM;
	double stableTime;
	double voltageLLRating;
	double horsePowerRatedVFD;
	double nominal_output_frequency;
	double settleTime;

	enum {
		VFD_OFF=0,			/**< VFD is off */
		VFD_CHANGING=1,		/**< VFD is changing frequency */
		VFD_STEADY=2	/**< VFD has reached desired frequency */
	};
	enumeration vfdState;
	enumeration prev_vfdState;

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
	gld::complex prev_power[3];
	double prev_desiredRPM;
	double efficiency_coeffs[8];
	double curr_time_value;
	double prev_time_value;
};

#endif // VFD_H
/**@}**/

