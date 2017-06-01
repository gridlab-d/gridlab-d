/** $Id: bus.h 4738 2014-07-03 00:55:39Z dchassin $
	@file bus.h
	@addtogroup bus
	@ingroup MODULENAME
	author: Kyle Anderson (GridSpice Project),  kyle.anderson@stanford.edu
	Released in Open Source to Gridlab-D Project


 @{
 **/

#ifndef _bus_H
#define _bus_H

#include <stdarg.h>
#include "gridlabd.h"
#include "matpower.h"

#define MAXSIZE 10000

class bus {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	//double length;
	static CLASS *oclass;
	static bus *defaults;
	
	//variable
	int 	BUS_I;
	int	BUS_TYPE;
	double 	PD;
	double 	QD;
	double 	GS;
	double 	BS;
	int	BUS_AREA;
	double 	VM;
	double 	VA;
	double	BASE_KV;
	int	ZONE;
	double 	VMAX;
	double 	VMIN;
	// only for the output
	double	LAM_P; //Lagrange multiplier on real power mismatch (u/MW)
	double 	LAM_Q; //Lagrange multiplier on reactive power mismatch (u/MVAr)
	double 	MU_VMAX; //Kuhn-Tucker multiplier on upper voltage limit (u/p.u.)
	double 	MU_VMIN; //Kuhn-Tucker multiplier on lower voltage limit (u/p.u)
	int	BASE_MVA;

	// feed header	
	int 	ifheader;
	//char	header_name[MAXSIZE]; 
	complex CVoltageA; // complex voltage: Cvoltage.Mag() = VM, Cvoltage.Arg() = VA;
	complex CVoltageB; // complex voltage: Cvoltage.Mag() = VM, Cvoltage.Arg() = VA;
	complex CVoltageC; // complex voltage: Cvoltage.Mag() = VM, Cvoltage.Arg() = VA;
	double  V_nom;	   // nomial voltage: VM * 1000;	

	// feed power insert
	/*
	double feeder1_PD;
	double feeder1_QD;
	double feeder2_PD;
	double feeder2_QD;
	double feeder3_PD;
	double feeder3_QD;
	double feeder4_PD;
	double feeder4_QD;
	double feeder5_PD;
	double feeder5_QD;
	double feeder6_PD;
	double feeder6_QD;
	double feeder7_PD;
	double feeder7_QD;
	double feeder8_PD;
	double feeder8_QD;
	double feeder9_PD;
	double feeder9_QD;
	double feeder10_PD;
	double feeder10_QD;
	*/
	complex feeder0;
	complex feeder1;
	complex feeder2;
	complex feeder3;
	complex feeder4;
	complex feeder5;
	complex feeder6;
	complex feeder7;
	complex feeder8;
	complex feeder9;

	
public:
	/* required implementations */
	bus(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);


	
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif

};

#endif

/**@}*/
