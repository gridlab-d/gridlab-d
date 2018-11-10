// powernet/house.h
// Copyright (C) 2016, Stanford University
// Author: dchassin@slac.stanford.edu

#ifndef _HOUSE_H
#define _HOUSE_H

#include "powernet.h"

DECL_METHOD(house,connect);

typedef struct s_enduseload {
	OBJECT *obj;
	enum {PC_NONE=0, PC_ONE=1, PC_TWO=2, PC_BOTH=3} circuit;
	double Zf; // constant impedance fraction (pu)
	double If; // constant current fraction (pu)
	double Pf; // constant power fraction (pu)
	double Mf; // motor power fraction (pu)
	double Ef; // electronic power fraction (pu)
	double Lw; // total load (kW)
	double F; // power factor
	double Qh; // heat gain to indoor space (kW)
} ENDUSELOAD;

class house : public gld_object {
public:
	typedef enum {SM_OFF=0, SM_HEAT=1, SM_AUX=2, SM_COOL=3, N_SYSTEMMODES} SYSTEMMODE;
	typedef enum {CT_NONE=0, CT_ACONLY=1, CT_GASHEAT=2, CT_ACPLUSGASHEAT=3, CT_HEATPUMP=4, N_SYSTEMTYPES} SYSTEMTYPE;
	GL_ATOMIC(object,weather);
	GL_ATOMIC(double,Uair);
	GL_ATOMIC(double,Cair);
	GL_ATOMIC(double,Umass);
	GL_ATOMIC(double,Cmass);
	GL_ATOMIC(double,Qheat);
	GL_ATOMIC(double,Qaux);
	GL_ATOMIC(double,Qcool);
	GL_ATOMIC(double,COPheat);
	GL_ATOMIC(double,COPcool);
	GL_ATOMIC(double,Tair);
	GL_ATOMIC(double,Tmass);
	GL_ATOMIC(enumeration,SystemMode); // see SYSTEMMODE
	GL_ATOMIC(enumeration,SystemType); // see SYSTEMTYPE
	GL_ATOMIC(double,Qint);
	GL_ATOMIC(double,Qvent);
	GL_ATOMIC(double,Qsolar);
	GL_ATOMIC(double,Qloads);
	GL_METHOD(house,connect);
private:
	std::list<ENDUSELOAD*> *loads;
	ENDUSELOAD *add_load(OBJECT *obj, bool is220=false);
	gld_property Tout;
	gld_property sell;
public:
	/* required implementations */
	house(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);

public:
	static CLASS *oclass;
	static house *defaults;
};

#endif // _HOUSE_H
