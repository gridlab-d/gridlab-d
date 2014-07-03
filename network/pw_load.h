/** $Id: pw_load.h 4738 2014-07-03 00:55:39Z dchassin $

 PowerWorld load object proxy.

 **/


#ifndef _PW_LOAD_H_
#define _PW_LOAD_H_

#include "gridlabd.h"
#include "network.h"

#ifdef HAVE_POWERWORLD
#ifndef PWX64

#include <iostream>

class pw_model;

class pw_load : public gld_object {
public:
	pw_load(MODULE *module);

	static CLASS *oclass;
	static pw_load *defaults;
	int create();
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t1);
	int isa(char *classname);

public:
	//	inputs
	GL_ATOMIC(int32, powerworld_bus_num);
	GL_STRING(char1024, powerworld_load_id);
	GL_ATOMIC(double, power_threshold);
	GL_ATOMIC(double, power_diff);
	GL_ATOMIC(complex, load_power);
	GL_ATOMIC(complex, load_impedance);
	GL_ATOMIC(complex, load_current);
	GL_ATOMIC(complex, last_load_power);
	GL_ATOMIC(complex, last_load_impedance);
	GL_ATOMIC(complex, last_load_current);
//	GL_ATOMIC(complex, next_load_power);
	//	outputs
	GL_ATOMIC(double, load_voltage_mag);
	GL_ATOMIC(complex, load_voltage);
	GL_ATOMIC(double, pw_load_mw);
	GL_ATOMIC(complex, pw_load_mva);
	GL_ATOMIC(double, bus_nom_volt);
	GL_ATOMIC(double, bus_volt_angle);
private:
	pw_model *cModel; // internal use

	int get_powerworld_nomvolt();
	int get_powerworld_busangle();
	int get_powerworld_voltage();
	int post_powerworld_current();
};
#endif // PWX64
#endif // HAVE_POWERWORLD
#endif // _PW_LOAD_H_

// EOF
