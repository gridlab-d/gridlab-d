// $Id: substation.h
//	Copyright (C) 2009 Battelle Memorial Institute

#ifndef _SUBSTATION_H
#define _SUBSTATION_H

#include "powerflow.h"
#include "node.h"

EXPORT SIMULATIONMODE interupdate_substation(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class substation : public node
{
protected:
	TIMESTAMP last_t;
public:
	gld::complex positive_sequence_voltage;				///< The positive sequence voltage of the substation object
	enum {
		R_PHASE_A,
		R_PHASE_B,
		R_PHASE_C
	} R_PHASE;
	enumeration reference_phase;						///< The point of reference for the positive sequence voltage conversion
	gld::complex average_transmission_power_load;		///<the average constant power load to be posted directly to the pw_load object
	gld::complex average_transmission_impedance_load;	///<the average constant impedance load to be posted directly to the pw_load object
	gld::complex average_transmission_current_load;		///<the average constant current load to be posted directly to the pw_load object
	gld::complex distribution_load;				///<The total load of all three phases at the substation object
	gld::complex distribution_power_A;					
	gld::complex distribution_power_B;					
	gld::complex distribution_power_C;	
	gld::complex seq_mat[3];								///<The matrix containing the sequence voltages
	double distribution_real_energy;
	double base_power;
	double power_convergence_value;					///Threshold where convergence is assumed achieved for pw_load-connected items
	int has_parent;									///0 = sequence conversion, 1 = pw_load connected, 2 = normal node
private:
	gld::complex reference_number;						///<The angle to shift the sequence voltages by to shift the matrix by
	gld::complex transformation_matrix[3][3];			///<the transformation matrix that converts sequence voltages to phase voltages
	gld::complex last_power_A;
	gld::complex last_power_B;
	gld::complex last_power_C;
	gld::complex volt_A;
	gld::complex volt_B;
	gld::complex volt_C;
	gld::complex *pPositiveSequenceVoltage;
	gld::complex *pConstantPowerLoad;
	gld::complex *pConstantCurrentLoad;
	gld::complex *pConstantImpedanceLoad;
	double *pTransNominalVoltage;					/// Transmission nominal voltage
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	substation(MODULE *mod);
	inline substation(CLASS *cl=oclass):node(cl){};
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	void fetch_complex(gld::complex **prop, const char *name, OBJECT *parent);
	void fetch_double(double **prop, const char *name, OBJECT *parent);
	static int isa(const char *classname);
	SIMULATIONMODE inter_deltaupdate_substation(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
};

#endif // _SUBSTATION_H

