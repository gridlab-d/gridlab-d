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
	complex positive_sequence_voltage;				///< The positive sequence voltage of the substation object
	typedef enum {
		R_PHASE_A,
		R_PHASE_B,
		R_PHASE_C
	};
	enumeration reference_phase;						///< The point of reference for the positive sequence voltage conversion
	complex average_transmission_power_load;		///<the average constant power load to be posted directly to the pw_load object
	complex average_transmission_impedance_load;	///<the average constant impedance load to be posted directly to the pw_load object
	complex average_transmission_current_load;		///<the average constant current load to be posted directly to the pw_load object
	complex distribution_load;				///<The total load of all three phases at the substation object
	complex distribution_power_A;					
	complex distribution_power_B;					
	complex distribution_power_C;	
	complex seq_mat[3];								///<The matrix containing the sequence voltages
	double distribution_real_energy;
	double base_power;
	double power_convergence_value;					///Threshold where convergence is assumed achieved for pw_load-connected items
	int has_parent;									///0 = sequence conversion, 1 = pw_load connected, 2 = normal node
private:
	complex reference_number;						///<The angle to shift the sequence voltages by to shift the matrix by
	complex transformation_matrix[3][3];			///<the transformation matrix that converts sequence voltages to phase voltages
	complex last_power_A;
	complex last_power_B;
	complex last_power_C;
	complex volt_A;
	complex volt_B;
	complex volt_C;
	complex *pPositiveSequenceVoltage;
	complex *pConstantPowerLoad;
	complex *pConstantCurrentLoad;
	complex *pConstantImpedanceLoad;
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
	void fetch_complex(complex **prop, char *name, OBJECT *parent);
	void fetch_double(double **prop, char *name, OBJECT *parent);
	int isa(char *classname);
	SIMULATIONMODE inter_deltaupdate_substation(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);
};

#endif // _SUBSTATION_H

