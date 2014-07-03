// $Id: transformer.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRANSFORMER_H
#define _TRANSFORMER_H

#include "powerflow.h"
#include "link.h"
#include "transformer_configuration.h"

class transformer : public link_object
{
public:
	OBJECT *configuration;
	//Thermal model parameters
	OBJECT *climate;
	double thermal_capacity;	// Wh/degrees C.
	double K;					// ratio of the current load to the rated load, per unit.
	double m;					// empirically derived exponent used to calculate the variation of dtheta_H with changes in load.
	double n;					// empirically derived exponent used to calculate the variation of dtheta_TO with changes in load.
	double R;					// ratio of full load loss to no-load loss.
	double *ptheta_A;			// pointer to the outside temperature, degrees F.
	double temp_A;				// ambient temperature in degrees F.
	double amb_temp;			// ambient temperature in degrees C.
	double theta_H;				// winding hottest-spot temperature, degrees C.
	double theta_TO;			// top-oil hottest-spot temperature, degrees C.
	double dtheta_H;			// winding hottest-spot rise over top-oil temperature, degrees C.
	double dtheta_H_i;			// initial winding hottest-spot rise over top-oil temperature, degrees C.
	double dtheta_H_R;			// winding hottest-spot rise over top-oil temperature at rated load, degrees C.
	double dtheta_H_U;			// ultimate winding hottest-spot rise over top-oil temperature for current load, degrees C.
	double dtheta_TO;			// top-oil hottest-spot rise over ambient temperature, degrees C.
	double dtheta_TO_i;			// initial top-oil hottest-spot rise over ambient temperature, degrees C.
	double dtheta_TO_U;			// ultimate top-oil hottest-spot rise over ambient temperature for current load, degrees C.
	double t_TOR;				// the oil time constant at rated load with dtheta_TO_i = 0 degrees C, hrs.
	double t_TO;				// the oil time constant for any load and for any difference between dtheta_TO_U and dtheta_TO_i, hrs.
	bool use_thermal_model;		// boolean to enable use of thermal model.
	//Aging model
	double B_age;				// the aging rate slope for the transformer insulation. 
	double F_AA;				// the aging acceleration factor.
	double F_EQA;				// the equivalent aging acceleration factor.
	double dt;					// change in time in hours.;
	double sumdt;
	double sumF_AAdt;
	double inst_ins_life;		// the installed insulation life of the transformer.
	double life_loss;			// the percent loss of life.
	double transformer_replacements;
	TIMESTAMP time_before;		// the previous timestamp.
	double aging_step;			// maximum timestep before updating thermal and aging model in seconds.
	TIMESTAMP return_at;
	double last_temp;


	transformer_configuration *config;

	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP postsync(TIMESTAMP t0);

	static CLASS *oclass;
	static CLASS *pclass;
	int isa(char *classname);

	transformer(MODULE *mod);
	inline transformer(CLASS *cl=oclass):link_object(cl){};

private:
	TIMESTAMP simulation_start_time;
	void fetch_double(double **prop, char *name, OBJECT *parent);
};
EXPORT void power_calculation(OBJECT *thisobj);
#endif // _TRANSFORMER_H
