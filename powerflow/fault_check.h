// $Id: fault_check.h 2009-12-15 15:00:00Z d3x593 $
//	Copyright (C) 2009 Battelle Memorial Institute

#ifndef _FAULT_CHECK_H
#define _FAULT_CHECK_H

#include "powerflow.h"

#define TIME_BUF_SIZE 64 // TODO: this ought to be in gridlabd.h, which has hard-coded value of 15 that is apparently too small

class fault_check : public powerflow_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	//Move typedef in here so reliabilty-related items can access the enumeration
	typedef enum {
			SINGLE=0,		//Runs one fault_check, right at the beginning of powerflow
			ONCHANGE=1,		//Runs fault_check everytime a Jacobian reconfiguration is requested
			ALLT=2,			//Runs fault_check on every iteration
			SINGLE_DEBUG=3,	//Runs one fault_check, but will terminate the simulation -- bypasses some phase check errors
			SWITCHING=4 // Runs every time a new Jacobian is requested, does not require supported nodes
			} FCSTATE;

	unsigned int **Supported_Nodes;			//Nodes with source support (connected to swing somehow)
	char *Alteration_Nodes;					//Similar to Supported_Nodes, but used to track alteration progression (namely, has the side been handled)
	char *Alteration_Links;					//Similar to Supported_Nodes, but used to track alteration progression in links
	unsigned char *valid_phases;			//Nodes with source support, specific to individual phases (hex mapped) -- used for mesh-based check

	enumeration fcheck_state;		//Mode variable
	char1024 output_filename;		//File name to output unconnected bus values
	bool reliability_mode;			//Flag for reliability implementation
	bool reliability_search_mode;	//Flag for how the object removal search occurs - basically assuming radial versus not
	bool grid_association_mode;		//Flag to see if fault_check should be checking for multiple grids, or just go on the "master swing" idea
	bool full_print_output;			//Flag to determine if both supported and unsupported nodes get written to the output file
	OBJECT *rel_eventgen;			//Eventgen object in reliability - allows "unscheduled" faults

	fault_check(MODULE *mod);
	fault_check(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);
	void search_links(int node_int);							//Function to check connectivity and support of nodes
	void search_links_mesh(int node_int);						//Function to check connectivity and support of nodes, but more in the "mesh" sense
	void support_check(int swing_node_int);						//Function that performs the connectivity check - this way so can be easily externally accessed
	void support_check_mesh(void);								//Function that performs the connectivity check for not-so-radial systems
	void reset_support_check(void);								//Function to re-init the support matrix
	void write_output_file(TIMESTAMP tval, double tval_delta);	//Function to write out "unsupported" items

	void support_check_alterations(int baselink_int, bool rest_mode);	//Function to update powerflow for "no longer supported" devices
	void reset_alterations_check(void);									//Function to re-init the alterations support matrix - mainly used to see if ends have been touched or not
	void allocate_alterations_values(bool reliability_mode_bool);		//Function to allocate the various memory items associated with reliability
	bool output_check_supported_mesh(void);								//Function to check and see if anything is unsupported and set overall flag (file outputs)

	void support_search_links(int node_int, int node_start, bool impact_mode);	//Function to check connectivity and support of nodes and remove/restore components as necessary
	void support_search_links_mesh(void);										//Function to parse bus list and remove anything that isn't supported (mesh systems)
	void momentary_activation(int node_int);									//Function to progress down a branch and flag momentary outages
	void special_object_alteration_handle(int branch_idx);						//Functionalized special device alteration code

	void reset_associated_grid(void);										//Function to reset/allocate "grid association" array
	void associate_grids(void);												//Function to look for the various swing nodes in the system, then associate the grids
	void search_associated_grids(unsigned int node_int, int grid_counter);	//Function to perform the "grid association" and populate the array

	STATUS disable_island(int island_number);				//Function to remove/disable an island if it diverged in powerflow
	STATUS rescan_topology(int bus_that_called_reset);		//Function to do the grid association and rescan islands

	TIMESTAMP sync(TIMESTAMP t0);

private:
	TIMESTAMP prev_time;	//Previous timestamp - mainly for intialization
	FUNCTIONADDR restoration_fxn;	// Function address for restoration object reconfiguration call
	bool force_reassociation;	//Flag to force the island reassociation -- used if an island was removed to renumber them
	char time_buf[TIME_BUF_SIZE];  // to format verbose time stamp output (note: fault_check is a singleton object)
};

EXPORT int powerflow_alterations(OBJECT *thisobj, int baselink,bool rest_mode);
EXPORT double handle_sectionalizer(OBJECT *thisobj, int sectionalizer_number);
EXPORT STATUS powerflow_disable_island(OBJECT *thisobj, int island_number);
EXPORT STATUS powerflow_rescan_topo(OBJECT *thisobj,int bus_that_called_reset);

#endif // _FAULT_CHECK_H
