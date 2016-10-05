/** $Id: line_sensor.h 4738 2014-07-03 00:55:39Z dchassin $

 General purpose line_sensor objects

 **/

#ifndef _LINESENSOR_H
#define _LINESENSOR_H

#include "gridlabd.h"
#include "../powerflow/powerflow_object.h"

class line_sensor : public gld_object {
public:
	GL_ATOMIC(enumeration,measured_phase);
	GL_ATOMIC(complex,measured_voltage);
	GL_ATOMIC(complex,measured_current);
	GL_ATOMIC(complex,measured_power);
	GL_ATOMIC(double,location);
	GL_STRUCT(double_array,covariance);
private:
	// TODO: private variables
	gld_property from_voltage_A;
	gld_property from_voltage_B;
	gld_property from_voltage_C;
	gld_property to_voltage_A;
	gld_property to_voltage_B;
	gld_property to_voltage_C;
	gld_property from_current_A;
	gld_property from_current_B;
	gld_property from_current_C;
	gld_property to_current_A;
	gld_property to_current_B;
	gld_property to_current_C;
	double w; // from-to weighting factor
	double L[4][4]; // Cholesky of covariance matrix
public:
	/* required implementations */
	line_sensor(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP postsync(TIMESTAMP t1);
	inline TIMESTAMP presync(TIMESTAMP t1) { return TS_NEVER; };
	inline TIMESTAMP sync(TIMESTAMP t1) { return TS_NEVER; };

public:
	static CLASS *oclass;
	static line_sensor *defaults;
};

#endif // _LINESENSOR_H
