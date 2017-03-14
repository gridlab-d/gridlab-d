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

	//class node *get_from(void) const;
	//class node *get_to(void) const;
	//set get_flow(class node **from, class node **to) const; /* determine flow direction (return phases on which flow is reverse) */

	int alloc_freq_arrays(double delta_t_val);
	void initialParameters(void);
	void vfdCoreCalculations(double currFrequencyVal, double settleTime, double phasorVal[3], node *tNode, node *fNode, double currEfficiency, complex prev_current[3]);
	complex complex_exp(double angle);
	STATUS VFD_current_injection(void);
	
public:
	double ratedRPM;
	double motorPoles;
	double desiredRPM;
	double stableTime;
	double voltageLLRating;
	double horsePowerRatedVFD;
	double nominal_output_frequency;
	double settleTime;
	complex powerOutElectrical;
	complex powerLosses;
	complex powerInElectrical;
	double currEfficiency;
	double driveFrequency;
	complex calc_current_in[3];
	complex currentOut[3];
	complex calc_volt_in[3];
	complex settleVoltOut[3];

private:
	//Variable objects that are only used internally
	double *settleFreq;
	double prevDesiredFreq;
	double *freqArray;
	double nominal_output_radian_freq;
	double torqueRated;
	double VbyF;
	double HPbyF;
	int vfdState;
	double stayTime;
	double settleVolt;
	bool force_array_realloc;
	unsigned int settleFreq_length;
	double phasorVal[3];
	int curr_array_position;
	complex prev_current[3];

};

#endif // VFD_H
/**@}**/

