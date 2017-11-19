#include "commercial.h"
#include "../powerflow/node.h"
#include <iostream>
#include <ctime>

CLASS* CommercialLoad::oclass = NULL;

double CommercialLoad::defaultTemperature = 64.4; //deg F

CommercialLoad::CommercialLoad(MODULE *mod) {
	if (oclass == NULL) {
		oclass = gld_class::create(mod, "commercial_load", sizeof(CommercialLoad), PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		if (gl_publish_variable(oclass,
			PT_int32, "building_type", PADDR(buildingType),
			PT_int32, "climate_zone", PADDR(climateZone),
			PT_int32, "naics_code", PADDR(naicsCode),
			PT_double, "power_pf_A[pu]",PADDR(power_pf[0]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase A constant power portion of load",
			PT_double, "current_pf_A[pu]",PADDR(current_pf[0]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase A constant current portion of load",
			PT_double, "impedance_pf_A[pu]",PADDR(impedance_pf[0]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase A constant impedance portion of load",
			PT_double, "power_pf_B[pu]",PADDR(power_pf[1]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase B constant power portion of load",
			PT_double, "current_pf_B[pu]",PADDR(current_pf[1]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase B constant current portion of load",
			PT_double, "impedance_pf_B[pu]",PADDR(impedance_pf[1]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase B constant impedance portion of load",
			PT_double, "power_pf_C[pu]",PADDR(power_pf[2]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase C constant power portion of load",
			PT_double, "current_pf_C[pu]",PADDR(current_pf[2]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase C constant current portion of load",
			PT_double, "impedance_pf_C[pu]",PADDR(impedance_pf[2]),PT_DESCRIPTION,"in similar format as ZIPload, this is the power factor of the phase C constant impedance portion of load",
			PT_double, "power_fraction_A[pu]",PADDR(power_fraction[0]),PT_DESCRIPTION,"this is the constant power fraction of base power on phase A",
			PT_double, "current_fraction_A[pu]",PADDR(current_fraction[0]),PT_DESCRIPTION,"this is the constant current fraction of base power on phase A",
			PT_double, "impedance_fraction_A[pu]",PADDR(impedance_fraction[0]),PT_DESCRIPTION,"this is the constant impedance fraction of base power on phase A",
			PT_double, "power_fraction_B[pu]",PADDR(power_fraction[1]),PT_DESCRIPTION,"this is the constant power fraction of base power on phase B",
			PT_double, "current_fraction_B[pu]",PADDR(current_fraction[1]),PT_DESCRIPTION,"this is the constant current fraction of base power on phase B",
			PT_double, "impedance_fraction_B[pu]",PADDR(impedance_fraction[1]),PT_DESCRIPTION,"this is the constant impedance fraction of base power on phase B",
			PT_double, "power_fraction_C[pu]",PADDR(power_fraction[2]),PT_DESCRIPTION,"this is the constant power fraction of base power on phase C",
			PT_double, "current_fraction_C[pu]",PADDR(current_fraction[2]),PT_DESCRIPTION,"this is the constant current fraction of base power on phase C",
			PT_double, "impedance_fraction_C[pu]",PADDR(impedance_fraction[2]),PT_DESCRIPTION,"this is the constant impedance fraction of base power on phase C",
			PT_double, "average_daily_usage[kVAh]",PADDR(average_usage),PT_DESCRIPTION,"average daily usage for load",
			PT_double, "nominal_voltage[V]", PADDR(nominal_voltage),
			PT_complex, "current_A[A]", PADDR(current[0]),PT_DESCRIPTION,"bus current injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_B[A]", PADDR(current[1]),PT_DESCRIPTION,"bus current injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "current_C[A]", PADDR(current[2]),PT_DESCRIPTION,"bus current injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_A[VA]", PADDR(power[0]),PT_DESCRIPTION,"bus power injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_B[VA]", PADDR(power[1]),PT_DESCRIPTION,"bus power injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "power_C[VA]", PADDR(power[2]),PT_DESCRIPTION,"bus power injection (in = positive), this an accumulator only, not a output or input variable",
			PT_complex, "shunt_A[S]", PADDR(shunt[0]),PT_DESCRIPTION,"bus shunt admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_B[S]", PADDR(shunt[1]),PT_DESCRIPTION,"bus shunt admittance, this an accumulator only, not a output or input variable",
			PT_complex, "shunt_C[S]", PADDR(shunt[2]),PT_DESCRIPTION,"bus shunt admittance, this an accumulator only, not a output or input variable",
			PT_double, "temperature", PADDR(temperature), PT_DESCRIPTION, "Current temperature in Celsius (debug only)",
			PT_set, "phases", PADDR(phases),
				PT_KEYWORD, "A", (set)PHASE_A,
				PT_KEYWORD, "B", (set)PHASE_B,
				PT_KEYWORD, "C", (set)PHASE_C,
			NULL) < 1) {
				GL_THROW("unable to publish properties in %s",__FILE__);
		}
	}
}

int CommercialLoad::create(){
	buildingType = 1;
	climateZone = 6;
	naicsCode = -1;

	average_usage = 0.0;
	for (int i = 0; i < 3; i++) {
		power_pf[i] = 1.0;
		current_pf[i] = 1.0;
		impedance_pf[i] = 1.0;
		power_fraction[i] = 1.0;
		impedance_fraction[i] = 0.0;
		current_fraction[i] = 0.0;
	}
	phases = (set)PHASE_A;
	char filepath[1025];
	nominal_voltage = 240.0;
	gl_global_getvar("commercial::coefficient_file", filepath, sizeof(filepath));

	int result = setCoefficientPath(filepath);

	switch (result) {
	case COMMERCIAL_COEFFICIENTS_FILE_NOT_SPECIFIED:
		GL_THROW("Commercial coefficients file not specified");
		return 0;
	case COMMERCIAL_COEFFICIENTS_ACCESS_DENIED:
		GL_THROW("Unable to access commercial coefficients file '%s'", filepath);
		return 0;
	}
	return 1;
}

int CommercialLoad::init(OBJECT* parent) {
	commObj = COMMHANDLE(getCommercialObject(climateZone, buildingType, naicsCode, average_usage), std::mem_fun(&ICommercialObject::release));
	phaseCount = 0;
	if (hasPhase(PHASE_A)) phaseCount++;
	if (hasPhase(PHASE_B)) phaseCount++;
	if (hasPhase(PHASE_C)) phaseCount++;

	if (parent == NULL) {
		gl_warning("Commercial load object %s has no parent", my()->name);
	} else if (gl_object_isa(parent, "node", "powerflow")) {
	} else {
		GL_THROW("Commercial load object %s has a parent that is not of type powerflow::node", my()->name);
	}

	if (nominal_voltage <= 0.0) {
		GL_THROW("Commercial load object %s has an invalid nominal voltage.", my()->name);
	}

	last_child_power[0][0] = last_child_power[0][1] = last_child_power[0][2] = complex(0,0);
	last_child_power[1][0] = last_child_power[1][1] = last_child_power[1][2] = complex(0,0);
	last_child_power[2][0] = last_child_power[2][1] = last_child_power[2][2] = complex(0,0);

	initClimate();

	return 1;
}

int CommercialLoad::isa(char *classname){
	return strcmp(classname,"commercial_load")==0;
}

TIMESTAMP CommercialLoad::presync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

TIMESTAMP CommercialLoad::sync(TIMESTAMP t0, TIMESTAMP t1){
	DATETIME dt;
	gl_localtime(t1, &dt);
	int dayofweek = dt.weekday;
	int hour = dt.hour;
	int month = dt.month;
	int day = dt.day;
	int year = dt.year;

	// Calculate returns values in kW (kWh actually, but we'll assume an average value of power for
	// the hour
	temperature = (*outdoorTemp - 32.0) * 5.0 / 9.0;
	commObj->setAvgUsage(average_usage);
	double load = commObj->calculate(temperature, dayofweek, hour, month) * 1000.0;
	// Divide the load evenly over the attached phases...
	double phaseload = load / (double)phaseCount;

	unsigned phaseList[3] = {PHASE_A, PHASE_B, PHASE_C};
	for (int i = 0; i < 3; i++) {
		if (hasPhase(phaseList[i])) {
			if (power_fraction[i] != 0.0) {
				double real_power, imag_power;
				double pf = power_pf[i];
				real_power = phaseload * power_fraction[i] * fabs(pf);
				imag_power = real_power * sqrt(1.0 / (pf * pf) - 1.0);
				if (pf < 0) {
					imag_power *= -1.0;
				}
				constant_power[i] = complex(real_power, imag_power);
			} else {
				constant_power[i] = complex(0, 0);
			}

			if (current_fraction[i] != 0.0) {
				double real_power, imag_power;
				double pf = current_pf[i];
				real_power = phaseload * power_fraction[i] * fabs(pf);
				imag_power = real_power * sqrt(1.0 / (pf * pf) - 1.0);
				if (pf < 0) {
					imag_power *= -1.0;
				}
				// There may need to be additional modifications of the angle here...
				constant_current[i] = ~complex(real_power, imag_power) / complex(nominal_voltage, 0);
			} else {
				constant_current[i] = complex(0, 0);
			}

			if (impedance_fraction[i] != 0.0) {
				double real_power, imag_power;
				double pf = impedance_pf[i];
				real_power = phaseload * power_fraction[i] * fabs(pf);
				imag_power = real_power * sqrt(1.0 / (pf * pf) - 1.0);
				if (pf < 0) {
					imag_power *= -1.0;
				}
				constant_impedance[i] = ~(complex(nominal_voltage * nominal_voltage, 0) / complex(real_power, imag_power));
			} else {
				constant_impedance[i] = complex(0, 0);
			}
			power[i] = constant_power[i];
			current[i] = constant_current[i];
			shunt[i] = constant_impedance[i].IsZero() ? 0.0 : complex(1.0) / constant_impedance[i];
		}
	}

	postValuesToParent();

	TIMESTAMP updateTime = t1 + (TIMESTAMP)(60 * TS_SECOND);

	return updateTime;
}

TIMESTAMP CommercialLoad::postsync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

EXPORT int create_commercial_load(OBJECT **obj, OBJECT *parent)
{
	try {
		*obj = gl_create_object(CommercialLoad::oclass);
		if (*obj!=NULL)
		{
			CommercialLoad *my = OBJECTDATA(*obj,CommercialLoad);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(commercial_load);
}

EXPORT int init_commercial_load(OBJECT *obj)
{
	try {
		CommercialLoad *my = OBJECTDATA(obj,CommercialLoad);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(CommercialLoad);
}

EXPORT int isa_commercial_load(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,CommercialLoad)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_commercial_load(OBJECT *obj, TIMESTAMP t1)
{
	CommercialLoad *my = OBJECTDATA(obj,CommercialLoad);
	try {
		TIMESTAMP t2 = my->sync(obj->clock, t1);
		obj->clock = t1;
		return t2;
	}
	SYNC_CATCHALL(commercial_load);
}

bool CommercialLoad::hasPhase(set p) {
	return (phases & p) != 0;
}

void CommercialLoad::postValuesToParent() {
	gld_object *parent = get_parent();

	node* loadParent = (node*)parent;
	LOCK_OBJECT(parent->my());
	for (int i = 0; i < 3; i++) {
		loadParent->power[i] += power[i] - last_child_power[0][i];
		loadParent->shunt[i] += shunt[i] - last_child_power[1][i];
		loadParent->current[i] += current[i] - last_child_power[2][i];

		last_child_power[0][i] = power[i];
		last_child_power[1][i] = shunt[i];
		last_child_power[2][i] = current[i];
	}
	UNLOCK_OBJECT(parent->my());
}

void CommercialLoad::initClimate() {
	static FINDLIST *climates = NULL;
	if (climates == NULL) {
		climates = gl_find_objects(FL_NEW, FT_CLASS, SAME, "climate", FT_END);		
		if (climates == NULL) {
			gl_warning("commercial_load: no climate data found, using default data");
			outdoorTemp = &defaultTemperature;
		}
	}

	if (climates != NULL) {
		if (climates->hit_count == 0) {
			outdoorTemp = &defaultTemperature;
		} else {
			OBJECT* obj = gl_find_next(climates, NULL);
			OBJECT* hdr = OBJECTHDR(this);
			if (obj->rank <= hdr->rank) {
				gl_set_dependent(obj, hdr);
			}
			outdoorTemp = (double*)GETADDR(obj, gl_get_property(obj, "temperature"));
		}
	}
}