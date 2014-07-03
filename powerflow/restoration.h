// $Id: restoration.h 4738 2014-07-03 00:55:39Z dchassin $
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

typedef struct  {
	int *Data;					//Pointer for the array
	unsigned int DataLength;	//Length of the allocated space
	unsigned int IdxVar;					//Variable for misc tracking - like how much space actually used
} VECTARRAY;

class restoration : public powerflow_library
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	int **Connectivity_Matrix;

	unsigned int reconfig_switch_number; // Number of switches that will be operated during a reconfiguration;
	unsigned int reconfig_attempts;		//Number of reconfigurations/timestep to try before giving up
	unsigned int reconfig_iter_limit;	//Number of iterations to let PF go before flagging this as a bad reconfiguration
	unsigned int reconfig_number;		//Number of reconfigurations that have been attempted at this timestep
	unsigned int reconfig_iterations;	//Number of iterations that have been performed on this reconfiguration
	unsigned int feeder_number; // Total number of feeders in the multi-feeder system
	unsigned int sub_transmission_node; // Record the sub_transmission_node
	bool populate_tree;			//Flag to populate Parent/Child tree structure

	restoration(MODULE *mod);
	inline restoration(CLASS *cl=oclass):powerflow_library(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);
	void CreateConnectivity(void);
	void PopulateConnectivity(int frombus, int tobus, OBJECT *linkingobj);
	void Perform_Reconfiguration(OBJECT *faultobj, TIMESTAMP t0);
	void Feeder_id_Mark(int initial_node, int feeder);
    int Search_sec_switches(int initial_node);
	bool VoltageCheck(double MagV_PU);
	double VoltagePerUnit(int node_id, int node_phase);
	bool connected_path(unsigned int node_int, unsigned int node_end, unsigned int last_node);

private:
	TIMESTAMP prev_time;	//Previous timestamp - mainly for intialization
	double *min_V;
	double max_V;
	int *node_id_minV;
	int node_id_maxV; 
	int *feeder_id; // Each node is belong to one feeder in the original system. The feeder id for each node is recorded.
	int *candidate_sec_switch;
	bool *visited_flag; // Visited_flag used in the DepthFirstSearch function
	int *unsupported_node;  // All the unsupported_node in the system 
	int *tie_switch; // All the tie_switch in the system
	int *sec_switch; // sectionalizing switch
	bool VoltageCheckResult;  // Voltage check result
    int *feeder_initial_node; //feeder_inital_node record the initial node connecting to the the subtransmission node for each feeder;
};

#endif // _RESTORATION_H
