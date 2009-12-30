// $Id: restoration.h 2009-11-09 15:00:00Z d3x593 $
//	Copyright (C) 2009 Battelle Memorial Institute

#ifndef _RESTORATION_H
#define _RESTORATION_H

#include "powerflow.h"
#include "powerflow_library.h"

#define CONN_NONE 0		//No connectivity
#define CONN_LINE 1		//Normal "line" - OH, UH, Triplex, transformer
#define CONN_FUSE 2		//Fuse connection
#define CONN_SWITCH 3	//Switch connection - controllable via "status"
#define CONN_XFRM 4		//Transformer connection or regulator

class restoration : public powerflow_library
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	char1024 configuration_file;
	int **Connectivity_Matrix;

	int reconfig_attempts;		//Number of reconfigurations/timestep to try before giving up
	int reconfig_iter_limit;	//Number of iterations to let PF go before flagging this as a bad reconfiguration
	int reconfig_number;		//Number of reconfigurations that have been attempted at this timestep
	int reconfig_iterations;	//Number of iterations that have been performed on this reconfiguration

	restoration(MODULE *mod);
	inline restoration(CLASS *cl=oclass):powerflow_library(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);
	void CreateConnectivity(void);
	void PopulateConnectivity(int frombus, int tobus, OBJECT *linkingobj);
	void Perform_Reconfiguration(OBJECT *faultobj, TIMESTAMP t0);
	bool VoltageCheck(void);

private:
	TIMESTAMP prev_time;	//Previous timestamp - mainly for intialization
};

#endif // _RESTORATION_H
