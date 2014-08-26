// $Id: fault_check.h 2009-12-15 15:00:00Z d3x593 $
//	Copyright (C) 2009 Battelle Memorial Institute

#ifndef _FAULT_CHECK_H
#define _FAULT_CHECK_H

#include "powerflow.h"

class fault_check : public powerflow_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	//Move typedef in here so reliabilty-related items can access the enumeration
	typedef enum {
			SINGLE=0,		//Runs one fault_check, right at the beginning of powerflow
			ONCHANGE=1,	//Runs fault_check everytime a Jacobian reconfiguration is requested
			ALLT=2			//Runs fault_check on every iteration
			} FCSTATE;

	typedef struct s_obj_supp_check {
		unsigned char phases_altered;
		unsigned char fault_phases;
		int altering_object;
		s_obj_supp_check *next;
	};	//Structure for tracking how objects have been removed

	unsigned int **Supported_Nodes;		//Nodes with source support (connected to swing somehow)
	char *Alteration_Nodes;		//Similar to Supported_Nodes, but used to track alteration progression (namely, has the side been handled)
	char *Alteration_Links;		//Similar to Supported_Nodes, but used to track alteration progression in links
	unsigned char *valid_phases;	//Nodes with source support, specific to individual phases (hex mapped) -- used for mesh-based check
	s_obj_supp_check *Altered_Node_Track;	//Structure for tracking node alterations, mainly for restoration operations
	s_obj_supp_check *Altered_Link_Track;	//Structure for tracking link alterations, mainly for restoration operations

	enumeration fcheck_state;		//Mode variable
	char1024 output_filename;	//File name to output unconnected bus values
	bool reliability_mode;		//Flag for reliability implementation
	bool reliability_search_mode;	//Flag for how the object removal search occurs - basically assuming radial versus not
	OBJECT *rel_eventgen;		//Eventgen object in reliability - allows "unscheduled" faults

	fault_check(MODULE *mod);
	fault_check(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);
	void search_links(int node_int);	//Function to check connectivity and support of nodes
	void search_links_mesh(int node_int,int buslink_fault,unsigned char fault_phases);		//Function to check connectivity and support of nodes, but more in the "mesh" sense
	void support_check(int swing_node_int, bool rest_pop_tree);	//Function that performs the connectivity check - this way so can be easily externally accessed
	void support_check_mesh(int swing_node_int,int buslink_fault,unsigned char *temp_phases);	//Function that performs the connectivity check for not-so-radial systems
	void reset_support_check(void);		//Function to re-init the support matrix
	void write_output_file(TIMESTAMP tval);		//Function to write out "unsupported" items

	void support_check_alterations(int baselink_int, bool rest_mode);	//Function to update powerflow for "no longer supported" devices
	void reset_alterations_check(void);									//Function to re-init the alterations support matrix - mainly used to see if ends have been touched or not
	bool output_check_supported_mesh(void);								//Function to check and see if anything is unsupported and set overall flag (file outputs)

	void altered_device_add(s_obj_supp_check *base_item, int fault_cause, unsigned char phases_alter, unsigned char phases_fault);
	bool altered_device_search(s_obj_supp_check *base_item, int fault_case, unsigned char phases_fault, unsigned char *phases_alter, bool remove_mode);

	void support_search_links(int node_int, int node_start, bool impact_mode);	//Function to check connectivity and support of nodes and remove/restore components as necessary
	void support_search_links_mesh(int baselink_int, bool impact_mode, unsigned char *phases_adjust);	//Function to parse bus list and remove anything that isn't supported (mesh systems)
	void momentary_activation(int node_int);	//Function to progress down a branch and flag momentary outages
	void special_object_alteration_handle(int branch_idx);	//Functionalized special device alteration code

	TIMESTAMP sync(TIMESTAMP t0);

private:
	TIMESTAMP prev_time;	//Previous timestamp - mainly for intialization
};

EXPORT int powerflow_alterations(OBJECT *thisobj, int baselink,bool rest_mode);
EXPORT double handle_sectionalizer(OBJECT *thisobj, int sectionalizer_number);

#endif // _FAULT_CHECK_H
