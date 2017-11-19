#include "gridlabd.h"
#include <vector>
#include <memory>
#include "commercial_library.hpp"

#ifndef COMMERCIAL_H_
#define COMMERCIAL_H_

typedef std::shared_ptr<ICommercialObject> COMMHANDLE;

class CommercialLoad : public gld_object {
public:
	static CLASS *oclass;
	static CLASS *pclass;

	int32 buildingType;
	int32 climateZone;
	int32 naicsCode;
	set phases;

	static double defaultTemperature;

	// Load settings. Blatantly stealing this from the powerflow::load object
	double power_pf[3];
	double current_pf[3];
	double impedance_pf[3];
	double power_fraction[3];
	double impedance_fraction[3];
	double current_fraction[3]; 
	double average_usage;
	double nominal_voltage;
	double temperature;

	complex constant_power[3];		// power load
	complex constant_current[3];	// current load
	complex constant_impedance[3];	// impedance load
	complex power[3];
	complex current[3];
	complex shunt[3];

	CommercialLoad(MODULE * module);
	int create();
	int isa(char* classname);
	int init(OBJECT *parent);

	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
private:
	int phaseCount;
	std::vector<double> coefficients;
	double* outdoorTemp;
	complex last_child_power[3][3];
	COMMHANDLE commObj;
	
	bool hasPhase(set p);
	void postValuesToParent();
	void initClimate();
};

#endif // COMMERCIAL_H_