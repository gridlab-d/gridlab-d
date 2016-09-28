/** $Id$

 Abstract multizone/central-plant commercial building model

 **/

#ifndef _BUILDING_H
#define _BUILDING_H

#include "gridlabd.h"

class building : public gld_object {
public:
	static double maximum_timestep; ///< maximum allowed timestep
	static double_array enduse_composition;
	typedef enum {NONE=0, OFF=1, VENT=2, HEAT=3, COOL=4, AUX=5, ECON=6, } HVACMODE;

public:
//	GL_STRUCT(enduse,load);
	GL_ATOMIC(int16,N); ///< number of zones
	GL_STRUCT(double_array,T); ///< array of N temperatures
	GL_STRUCT(double_array,U); ///< array of NxN conductances
	GL_STRUCT(double_array,Q); ///< array of N heat flows
	GL_STRUCT(double_array,C); ///< array of N heat capacities
	GL_STRUCT(double_array,Qh); ///< array of N heating heat gains
	GL_STRUCT(double_array,Qc); ///< array of N cooling heat losses
	GL_STRUCT(double_array,Qf); ///< array of N fan heat gains
	GL_STRUCT(double_array,Qs); ///< array of N solar heat gains
	GL_STRUCT(double_array,Qi); ///< array of N internal heat gains
	GL_STRUCT(double_array,Qhc); ///< array of N heating capacities
	GL_STRUCT(double_array,Qcc); ///< array of N cooling capacities
	GL_STRUCT(double_array,Qfl); ///< array of N low fan heat gains
	GL_STRUCT(double_array,Qfh); ///< array of N high fan heat gains
	GL_STRUCT(double_array,Ts); ///< array of N thermostat mode (0=NONE, 1=OFF, 2=VENT, 3=HEAT, 4=COOL, 5=AUX, 6=ECON)
	GL_STRUCT(double_array,Vm); ///< array of N minimum ventilation rates
	GL_STRUCT(double_array,Th); ///< array of N heating set-points
	GL_STRUCT(double_array,Tc); ///< array of N cooling set-points
	GL_STRUCT(double_array,Td); ///< array of N set-point deadbands
	GL_ATOMIC(double,tl); ///< thermostat lockout time (seconds)
	GL_ATOMIC(bool,autosize); ///< flag to enable autosizing of arrays

private:
	double t_last; // time of last update
	double t_next; // time of next update
	double *Tlast; // temperatures at last update
	double *dT; // temperature change since last update

public:
	/* required implementations */
	building(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	STATUS precommit(TIMESTAMP t);
	inline TIMESTAMP presync(TIMESTAMP t) { return TS_NEVER; }
	TIMESTAMP sync(TIMESTAMP t);
	inline TIMESTAMP postsync(TIMESTAMP t) { return TS_NEVER; }
	TIMESTAMP commit(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP plc(TIMESTAMP t);

private:
	bool is_ventilating(unsigned int n);

public:
	static CLASS *oclass;
	static building *defaults;
};
#endif // _BUILDING_H