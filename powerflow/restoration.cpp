/** $Id: restoration.cpp 2009-11-09 15:00:00Z d3x593 $
	Copyright (C) 2009 Battelle Memorial Institute
**/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace std;

#include "restoration.h"

//////////////////////////////////////////////////////////////////////////
// restoration CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* restoration::oclass = nullptr;

CLASS* restoration::pclass = nullptr;

restoration::restoration(MODULE *mod) : powerflow_library(mod)
{
	if(oclass == nullptr)
	{
		oclass = gl_register_class(mod,"restoration",sizeof(restoration),0x00);
		if (oclass==nullptr)
			throw "unable to register class restoration";
		else
			oclass->trl = TRL_PROTOTYPE;

		if(gl_publish_variable(oclass,
			PT_int32,"reconfig_attempts",PADDR(reconfig_attempts),PT_DESCRIPTION,"Number of reconfigurations/timestep to try before giving up",
			PT_int32,"reconfig_iteration_limit",PADDR(reconfig_iter_limit),PT_DESCRIPTION,"Number of iterations to let PF go before flagging this as a bad reconfiguration",
			PT_object,"source_vertex",PADDR(sourceVerObj),PT_DESCRIPTION,"Source vertex object for reconfiguration",
			PT_object,"faulted_section",PADDR(faultSecObj),PT_DESCRIPTION,"Faulted section for reconfiguration",
			PT_char1024,"feeder_power_limit",PADDR(feeder_power_limit_Pub),PT_DESCRIPTION,"Comma-separated power limit (VA) for feeders during reconfiguration",
			PT_char1024,"feeder_power_links",PADDR(feeder_power_link_Pub),PT_DESCRIPTION,"Comma-separated list of link-based objects for monitoring power through",
			PT_char1024,"feeder_vertex_list",PADDR(fVerPub),PT_DESCRIPTION,"Comma-separated object list that defines the feeder vertices",
			PT_char1024,"microgrid_power_limit",PADDR(microgrid_limit_Pub),PT_DESCRIPTION,"Comma-separated power limit (complex VA) for microgrids during reconfiguration",
			PT_char1024,"microgrid_power_links",PADDR(microgrid_power_link_Pub),PT_DESCRIPTION,"Comma-separated list of link-based objects for monitoring power through",
			PT_char1024,"microgrid_vertex_list",PADDR(mVerPub),PT_DESCRIPTION,"Comma-separated object list that defines the microgrid vertices",
			PT_double,"lower_voltage_limit[pu]",PADDR(voltage_limit[0]),PT_DESCRIPTION,"Lower voltage limit for the reconfiguration validity checks - per unit",
			PT_double,"upper_voltage_limit[pu]",PADDR(voltage_limit[1]),PT_DESCRIPTION,"Upper voltage limit for the reconfiguration validity checks - per unit",
			PT_char1024,"output_filename",PADDR(logfile_name),PT_DESCRIPTION,"Output text file name to describe final or attempted switching operations",
			PT_bool,"generate_all_scenarios",PADDR(stop_and_generate),PT_DESCRIPTION,"Flag to determine if restoration reconfiguration and continues, or explores the full space",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		if (gl_publish_function(oclass,	"perform_restoration", (FUNCTIONADDR)perform_restoration)==nullptr)
			GL_THROW("Unable to publish restoration function");
    }
}

int restoration::isa(char *classname)
{
	return strcmp(classname,"restoration")==0;
}

int restoration::create(void)
{
	reconfig_attempts = 20;			//Arbitrary
	reconfig_iter_limit = 50;		//Arbitrary
	file_output_desired = false;	//By default, no output

	stop_and_generate = false;		//By default, just reconfigure until we're happy

	feeder_power_limit = nullptr;
	microgrid_limit = nullptr;
	mVerObjList = nullptr;
	fVerObjList = nullptr;
	fLinkObjList = nullptr;
	fLinkPowerFunc = nullptr;
	mLinkObjList = nullptr;
	mLinkPowerFunc = nullptr;
	feeder_power_link_value = nullptr;
	microgrid_power_link_value = nullptr;

	sourceVerObj = nullptr;
	faultSecObj = nullptr;
	numfVer = 0;
	numMG = 0;

	MGIdx.currSize = 0;
	MGIdx.maxSize = 0;
	MGIdx.data = nullptr;

	MGIdx_1.currSize = 0;
	MGIdx_1.maxSize = 0;
	MGIdx_1.data = nullptr;

	MGIdx_2.currSize = 0;
	MGIdx_2.maxSize = 0;
	MGIdx_2.data = nullptr;

	feederVertices.currSize = 0;
	feederVertices.maxSize = 0;
	feederVertices.data = nullptr;

	feederVertices_1.currSize = 0;
	feederVertices_1.maxSize = 0;
	feederVertices_1.data = nullptr;

	feederVertices_2.currSize = 0;
	feederVertices_2.maxSize = 0;
	feederVertices_2.data = nullptr;

	top_ori = nullptr;
	top_sim_1 = nullptr;
	top_sim_2 = nullptr;
	top_res = nullptr;
	top_tmp = nullptr;

	tie_swi.currSize = 0;
	tie_swi.maxSize = 0;
	tie_swi.data_1 = nullptr;
	tie_swi.data_2 = nullptr;

	tie_swi_1.currSize = 0;
	tie_swi_1.maxSize = 0;
	tie_swi_1.data_1 = nullptr;
	tie_swi_1.data_2 = nullptr;

	tie_swi_2.currSize = 0;
	tie_swi_2.maxSize = 0;
	tie_swi_2.data_1 = nullptr;
	tie_swi_2.data_2 = nullptr;

	sec_swi.currSize = 0;
	sec_swi.maxSize = 0;
	sec_swi.data_1 = nullptr;
	sec_swi.data_2 = nullptr;

	sec_swi_1.currSize = 0;
	sec_swi_1.maxSize = 0;
	sec_swi_1.data_1 = nullptr;
	sec_swi_1.data_2 = nullptr;

	sec_swi_2.currSize = 0;
	sec_swi_2.maxSize = 0;
	sec_swi_2.data_1 = nullptr;
	sec_swi_2.data_2 = nullptr;

	sec_swi_map.currSize = 0;
	sec_swi_map.maxSize = 0;
	sec_swi_map.data_1 = nullptr;
	sec_swi_map.data_2 = nullptr;

	sec_swi_map_1.currSize = 0;
	sec_swi_map_1.maxSize = 0;
	sec_swi_map_1.data_1 = nullptr;
	sec_swi_map_1.data_2 = nullptr;

	voltage_limit[0] = 0.95;	//Arbitrary values - use C84.1 ANSI limits?
	voltage_limit[1] = 1.05;

	tie_swi_loc.currSize = 0;
	tie_swi_loc.maxSize = 0;
	tie_swi_loc.data_1 = nullptr;
	tie_swi_loc.data_2 = nullptr;
	tie_swi_loc.data_3 = nullptr;
	tie_swi_loc.data_4 = nullptr;

	sec_swi_loc.currSize = 0;
	sec_swi_loc.maxSize = 0;
	sec_swi_loc.data_1 = nullptr;
	sec_swi_loc.data_2 = nullptr;
	sec_swi_loc.data_3 = nullptr;
	sec_swi_loc.data_4 = nullptr;

	f_sec.from_vert = -1;
	f_sec.to_vert = -1;
	f_sec_1.from_vert = -1;
	f_sec_1.to_vert = -1;
	f_sec_2.from_vert = -1;
	f_sec_2.to_vert = -1;
	
	s_ver = -1;
	s_ver_1 = -1;
	s_ver_2 = -1;

	ver_map_1.currSize = 0;
	ver_map_1.maxSize = 0;
	ver_map_1.data = nullptr;

	ver_map_2.currSize = 0;
	ver_map_2.maxSize = 0;
	ver_map_2.data = nullptr;

	candidateSwOpe.currSize = 0;
	candidateSwOpe.maxSize = 0;
	candidateSwOpe.data_1 = nullptr;
	candidateSwOpe.data_2 = nullptr;
	candidateSwOpe.data_3 = nullptr;
	candidateSwOpe.data_4 = nullptr;
	candidateSwOpe.data_5 = nullptr;
	candidateSwOpe.data_6 = nullptr;
	candidateSwOpe.data_7 = nullptr;

	candidateSwOpe_1.currSize = 0;
	candidateSwOpe_1.maxSize = 0;
	candidateSwOpe_1.data_1 = nullptr;
	candidateSwOpe_1.data_2 = nullptr;
	candidateSwOpe_1.data_3 = nullptr;
	candidateSwOpe_1.data_4 = nullptr;
	candidateSwOpe_1.data_5 = nullptr;
	candidateSwOpe_1.data_6 = nullptr;
	candidateSwOpe_1.data_7 = nullptr;

	candidateSwOpe_2.currSize = 0;
	candidateSwOpe_2.maxSize = 0;
	candidateSwOpe_2.data_1 = nullptr;
	candidateSwOpe_2.data_2 = nullptr;
	candidateSwOpe_2.data_3 = nullptr;
	candidateSwOpe_2.data_4 = nullptr;
	candidateSwOpe_2.data_5 = nullptr;
	candidateSwOpe_2.data_6 = nullptr;
	candidateSwOpe_2.data_7 = nullptr;

	voltage_storage = nullptr;

	fault_check_fxn = nullptr;

	return 1;
}

int restoration::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	int working_int_val, indexval;

	if (solver_method == SM_NR)
	{
		if (restoration_object == nullptr)
		{
			restoration_object = obj;	//Set up the global
			
			//Check limits - post warnings as necessary
			if (reconfig_attempts==0)
			{
				GL_THROW("Infinite reconfigurations set.");
				/*  TROUBLESHOOT
				The restoration object can have the number of reconfiguration per timestep set via the
				reconfig_attempts property.  If not set, or set to 0, the system will perform reconfigurations
				forever and lock up the computer (if an acceptable solution is never met).
				*/
			}

			if (reconfig_iter_limit==0)
			{
				gl_warning("Infinite reconfiguration iterations set.");
				/*  TROUBLESHOOT
				The restoration object can have the number of iterations powerflow can perform before trying another
				reconfiguration set.  This is set via teh reconfig_iteration_limit property.  If not set, or set to 0, the
				system will perform iterations on this reconfiguration until the overall powerflow iteration limit is reached,
				or the system solves and moves to a new reconfiguration.
				*/
			}
		}
		else
		{
			GL_THROW("Only one restoration object is permitted per model file");
			/*  TROUBLESHOOT
			The restoration object manipulates the configuration of the system.  In order
			to prevent possible conflicts, only one restoration object is permitted per
			model.
			*/
		}

		//Parse the defined input values

		//Read in the feeder power limit values
		feeder_power_limit = ParseDoubleString(&feeder_power_limit_Pub[0],&numfVer);

		//Read in the feeder vertex object list
		fVerObjList = ParseObjectString(&fVerPub[0],&working_int_val);

		//Make sure the counts match, or throw an error
		if (numfVer != working_int_val)
		{
			GL_THROW("restoration: The number of feeder limits and feeder vertices must be the same!");
			/*  TROUBLESHOOT
			While specifying the feeder_power_limit and feeder_vertex_list, the same number of feeders must be represented.
			*/
		}

		//Read in the feeder "power flow" object list
		fLinkObjList = ParseObjectString(&feeder_power_link_Pub[0],&working_int_val);

		//Do another check to make sure counts match
		if (numfVer != working_int_val)
		{
			GL_THROW("restoration: The number of feeder links and feeder vertices must be the same!");
			/*  TROUBLESHOOT
			While specifying the feeder_power_links and feeder_vertex_list, the same number of feeders must be represented.
			*/
		}

		//Allocate the "function map" array
		fLinkPowerFunc = (FUNCTIONADDR *)gl_malloc(numfVer*sizeof(FUNCTIONADDR));

		//Check it
		if (fLinkPowerFunc == nullptr)
		{
			GL_THROW("Restoration: failed to allocate memory for link functions");
			/*  TROUBLESHOOT
			While attempting to allocate memory for the feeder/microgrid power link objects, an
			error occurred.  Please try again.  If the error persists, please submit your code and a bug
			report via the ticketing system.
			*/
		}

		//Do the same for the value
		feeder_power_link_value = (gld::complex **)gl_malloc(numfVer*sizeof(gld::complex *));

		//Check it
		if (feeder_power_link_value == nullptr)
		{
			GL_THROW("Restoration: failed to allocate memory for calculated power values");
			/*  TROUBLESHOOT
			While attempting to allocate memory for pointers to the calculated power of links,
			an error occurred.  Please try again, assuming proper inputs.  If the error persists,
			please submit your code and a bug report via the ticketing system.
			*/
		}

		//Now try to map the power update functions for each of these objects
		for (indexval=0; indexval<numfVer; indexval++)
		{
			//Try to map the function
			fLinkPowerFunc[indexval] = (FUNCTIONADDR)(gl_get_function(fLinkObjList[indexval],"update_power_pwr_object"));

			//Check to see if it worked -- would let it be NULL, but will cause problems later
			if (fLinkPowerFunc[indexval] == nullptr)
			{
				GL_THROW("Restoration: failed to find power calculation function for link:%s",fLinkObjList[indexval]->name ? fLinkObjList[indexval]->name : "Unnamed");
				/*  TROUBLESHOOT
				While attempting to map the update_power_pwr_object function for a link object, a failure occurred.
				Make sure the object is a link object and try again.  If the error persists, please submit your
				code and a bug report via the ticketing system.
				*/
			}

			//Link up the actual power value too, while we're here
			feeder_power_link_value[indexval] = gl_get_complex_by_name(fLinkObjList[indexval],"power_out");

			//Make sure it worked
			if (feeder_power_link_value[indexval] == nullptr)
			{
				GL_THROW("Restoration: failed to find calculated power value for link:s",fLinkObjList[indexval]->name ? fLinkObjList[indexval]->name : "Unnamed");
				/*  TROUBLESHOOT
				While attempting to map up the location of the complex, calculated power of a link, an error occurred.
				Please try again.  If the error persists, please submit your code and a bug report via the ticketing system.
				*/
			}
		}

		//Read in the microgrid power limits (complex)
		microgrid_limit = ParseComplexString(&microgrid_limit_Pub[0],&numMG);

		//Read in the microgrid vertex list
		mVerObjList = ParseObjectString(&mVerPub[0],&working_int_val);

		//Check to make sure these are proper too
		if (numMG != working_int_val)
		{
			GL_THROW("restoration: The number of microgrid limits and microgrid vertices must be the same!");
			/*  TROUBLESHOOT
			While specifying the microgrid_power_limit and microgrid_vertex_list, the same number of microgrids must
			be represented.
			*/
		}

		//Read in the microgrid "power flow" object list
		mLinkObjList = ParseObjectString(&microgrid_power_link_Pub[0],&working_int_val);

		//Do another check to make sure counts match
		if (numMG != working_int_val)
		{
			GL_THROW("restoration: The number of microgrid links and microgrid vertices must be the same!");
			/*  TROUBLESHOOT
			While specifying the microgrid_power_links and microgrid_vertex_list, the same number of microgrids must be represented.
			*/
		}

		//Allocate the "function map" array
		mLinkPowerFunc = (FUNCTIONADDR *)gl_malloc(numMG*sizeof(FUNCTIONADDR));

		//Check it
		if (mLinkPowerFunc == nullptr)
		{
			GL_THROW("Restoration: failed to allocate memory for link functions");
			//Defined above
		}

		//Do the same for the value
		microgrid_power_link_value = (gld::complex **)gl_malloc(numMG*sizeof(gld::complex *));

		//Check it
		if (microgrid_power_link_value == nullptr)
		{
			GL_THROW("Restoration: failed to allocate memory for calculated power values");
			//Defined above
		}

		//Now try to map the power update functions for each of these objects
		for (indexval=0; indexval<numMG; indexval++)
		{
			//Try to map the function
			mLinkPowerFunc[indexval] = (FUNCTIONADDR)(gl_get_function(mLinkObjList[indexval],"update_power_pwr_object"));

			//Check to see if it worked -- would let it be NULL, but will cause problems later
			if (mLinkPowerFunc[indexval] == nullptr)
			{
				GL_THROW("Restoration: failed to find power calculation function for link:%s",mLinkObjList[indexval]->name ? mLinkObjList[indexval]->name : "Unnamed");
				//Defined above
			}

			//Link up the actual power value too, while we're here
			microgrid_power_link_value[indexval] = gl_get_complex_by_name(mLinkObjList[indexval],"power_out");

			//Make sure it worked
			if (microgrid_power_link_value[indexval] == nullptr)
			{
				GL_THROW("Restoration: failed to find calculated power value for link:s",mLinkObjList[indexval]->name ? mLinkObjList[indexval]->name : "Unnamed");
				//Defined above
			}
		}

		//Make sure the voltage limits aren't reversed
		if (voltage_limit[1] <= voltage_limit[0])
		{
			GL_THROW("restoration: the upper and lower voltage limits are either equal, or backwards");
			/*  TROUBLESHOOT
			The lower_voltage_limit must be lower than the upper_voltage_limit for the restoration object to properly work.
			*/
		}

		//Create an empty output file by "recreating it", if desired
		if (logfile_name[0] != '\0')
		{
			//Open it to empty it
			FILE *FPOutput = fopen(logfile_name,"wt");

			//Close it now
			fclose(FPOutput);

			//Flag us for output
			file_output_desired = true;
		}
	}//End NR solver
	else
	{
		GL_THROW("Restoration control is only valid for the Newton-Raphson solver at this time.");
		/*  TROUBLESHOOT
		The restoration object only works with the Newton-Raphson powerflow solver at this time.
		Other solvers may be integrated at a future date.
		*/
	}

	return 1;
}

//Internal version of exposed restoration call
int restoration::PerformRestoration(int faulting_link)
{
	unsigned int indexval;
	int num_swi_open, num_swi_closed, secindexval, IdxSW;
	BRANCHVERTICES fsec;

	//Set global flag active
	restoration_checks_active = true;

	if (fault_check_object != nullptr)
	{
		//See if we've mapped the function yet
		if (fault_check_fxn == nullptr)
		{
			//Map the function
			fault_check_fxn = (FUNCTIONADDR)(gl_get_function(fault_check_object,"reliability_alterations"));
			
			//Make sure it was found
			if (fault_check_fxn == nullptr)
			{
				GL_THROW("Unable to update objects for reliability effects");
				//Defined somewhere else
			}
		}
		//Default else, already mapped
	}
	else	//Throw an error
	{
		GL_THROW("Restoration: fault_check object not found.");
		/*  TROUBLESHOOT
		While attemping to map a function for the fault_check object, no fault_check
		object could be found.  Restoration requires one to function.  Please add one and try
		again.
		*/
	}

	//Make sure the source vertex is defined
	if (sourceVerObj == nullptr)
	{
		gl_warning("restoration: The source vertex for the reconfiguration is not defined, defaulting to a swing bus");
		/*  TROUBLESHOOT
		To properly operate, the reconfiguration needs the sources vertex of the system defined.  If left blank, the first
		SWING bus will be utilized instead.
		*/

		//Grab the powerflow designated "master swing bus"
		sourceVerObj = NR_swing_bus;
	}

	//Simplified readNode_2 code -- create a new LinkedUndigraph item for the number of nodes
	//Steal the NR-based numbering schemes, since they're already done
	top_ori = (LinkedUndigraph *)gl_malloc(sizeof(LinkedUndigraph));	//Allocate the storage item

	//Make sure it worked
	if (top_ori == nullptr)
	{
		GL_THROW("Restoration: failed to allocte graph theory object");
		/*  TROUBLESHOOT
		While attempting to allocate memory for one of the graph theory objects inside the
		restoration object, an error was encountered.  Please try again.  If the error persists,
		please submit your code and a bug report via the ticketing system.
		*/
	}

	//Call the constructor for the number of NR nodes
	new (top_ori) LinkedUndigraph(NR_bus_count);


	//Populate the microgrid edges -- allocate first
	INTVECTalloc(&MGIdx,numMG);
	INTVECTalloc(&MGIdx_1,numMG);
	INTVECTalloc(&MGIdx_2,numMG);

	//Loop and populate the list - do reversed, since may exit faster that way
	for (secindexval=0; secindexval<numMG; secindexval++)
	{
		//Loop through buses to see if we have a match
		for (indexval=0; indexval<NR_bus_count; indexval++)
		{
			if (mVerObjList[secindexval] == NR_busdata[indexval].obj)
			{
				MGIdx.data[MGIdx.currSize] = indexval;
				MGIdx.currSize++;
				break;	//Next loop
			}
		}//End bus traversion
	}//End microgrid edge traversion

	//Do readEdges_2 routine and part of readSwitches_2 routine
	//Simplified to loop the NR_branchdata array and populate almost everything
	//Switches that are open are apparently removed, later

	//Performed up here so source-connected switch can be removed -- this was done in a separate routine before
	//Set the source vertex
	for (indexval=0; indexval<NR_bus_count; indexval++)
	{
		if (sourceVerObj == NR_busdata[indexval].obj)
		{
			s_ver = indexval;
			break;
		}
	}

	//Check it, for grins
	if (s_ver == -1)
	{
		GL_THROW("Restoration: failed to find the source vertex!");
		/*  TROUBLESHOOT
		While attempting to map the source vertex of the graph, the node in question
		could not be located.  Be sure it is part of the main system island and try again.
		If the error persists, please submit your code and a bug report via the ticketing system.
		*/
	}

	//Zero the accumulators
	num_swi_open = 0;
	num_swi_closed = 0;

	for (indexval=0; indexval<NR_branch_count; indexval++)
	{
		//Add them in, unless it is an open fuse/open switch/open sectionalizer
		if ((NR_branchdata[indexval].lnk_type == 3) || (NR_branchdata[indexval].lnk_type == 4) || (NR_branchdata[indexval].lnk_type == 5))	//Normal switch considered sectionalizer, as well
		{
			//Make sure it is closed (if open, just don't add it)
			if (*NR_branchdata[indexval].status == LS_CLOSED)
			{
				top_ori->addEdge(NR_branchdata[indexval].from,NR_branchdata[indexval].to);
			}
			//Default else, don't add it

			//Update the count of what it is (if not a fuse)
			if (NR_branchdata[indexval].lnk_type != 3)	//Must be a switch or sectionalizer
			{
				if (*NR_branchdata[indexval].status == LS_CLOSED)	//Closed switches
				{
					//See if one side is connected to the source vertex -- if so, don't count it (WSU algorithm does this separately)
					//if ((NR_branchdata[indexval].from != s_ver) && (NR_branchdata[indexval].to != s_ver))
					{
						num_swi_closed++;
					}
					//Default else - connected to source vertex, so don't count
				}
				else	//Must be open
				{
					num_swi_open++;
				}
			}
			//Else a fuse, no count needed (not used anywhere)
		}//End is a fuse, switch, or rsectionalizer
		else	//Not a fuse, switch, or sectionalizer just add it, for now (reclosers always assumed closed)
		{
			top_ori->addEdge(NR_branchdata[indexval].from,NR_branchdata[indexval].to);

			//See if we're a recloser
			if (NR_branchdata[indexval].lnk_type == 6)
			{
				num_swi_closed++;
			}
			//default else -- something else
		}//End not a fuse, switch, or sectionalizer
	}//End NR_branchdata parse

	//Allocate tracking arrays - allocate them all to the same size (multiple passes)
	CHORDSETalloc(&tie_swi,num_swi_open);
	CHORDSETalloc(&tie_swi_1,num_swi_open);
	CHORDSETalloc(&tie_swi_2,num_swi_open);
	LOCSETalloc(&tie_swi_loc,num_swi_open);

	CHORDSETalloc(&sec_swi,num_swi_closed);
	CHORDSETalloc(&sec_swi_1,num_swi_closed);
	//CHORDSETalloc(&sec_swi_2,num_swi_closed);
	LOCSETalloc(&sec_swi_loc,num_swi_closed);

	//Now populate these arrays (requires a second pass)
	for (indexval=0; indexval<NR_branch_count; indexval++)
	{
		//See if we're a normal switch or sectionalizer
		if ((NR_branchdata[indexval].lnk_type == 4) || (NR_branchdata[indexval].lnk_type == 5))
		{
			//Check out status
			if (*NR_branchdata[indexval].status == LS_CLOSED)
			{
				//Make sure we're not source-vertex connected, just like above
				//if ((NR_branchdata[indexval].from != s_ver) && (NR_branchdata[indexval].to != s_ver))
				{
					//Switch closed - sectionalizing -- add it to the list
					sec_swi.data_1[sec_swi.currSize] = NR_branchdata[indexval].from;
					sec_swi.data_2[sec_swi.currSize] = NR_branchdata[indexval].to;
					sec_swi.currSize++;

					//Populate the location matrix thing
					sec_swi_loc.data_1[sec_swi_loc.currSize] = indexval;	//This was originally line number in the GLM.  Not sure this is needed anymore
					sec_swi_loc.data_2[sec_swi_loc.currSize] = 0;
					sec_swi_loc.data_3[sec_swi_loc.currSize] = 0;
					sec_swi_loc.data_4[sec_swi_loc.currSize] = 0;
					sec_swi_loc.currSize++;
				}
				//Default else -- closed and connected to the source vertex - ignore it (WSU requirement)
			}
			else
			{
				//Switch open - tie
				tie_swi.data_1[tie_swi.currSize] = NR_branchdata[indexval].from;
				tie_swi.data_2[tie_swi.currSize] = NR_branchdata[indexval].to;
				tie_swi.currSize++;

				//Populate the location matrix thing
				tie_swi_loc.data_1[tie_swi_loc.currSize] = indexval;	//This was originally line number in the GLM.  Not sure this is needed anymore
				tie_swi_loc.data_2[tie_swi_loc.currSize] = 0;
				tie_swi_loc.data_3[tie_swi_loc.currSize] = 0;
				tie_swi_loc.data_4[tie_swi_loc.currSize] = 0;
				tie_swi_loc.currSize++;

			}
		}//End is a switch, or rsectionalizer
		else if (NR_branchdata[indexval].lnk_type == 6)	//If it's a recloser, automatically a sectionalizing switch
		{
			//No open/closed test for these -- all go into the same category
			sec_swi.data_1[sec_swi.currSize] = NR_branchdata[indexval].from;
			sec_swi.data_2[sec_swi.currSize] = NR_branchdata[indexval].to;
			sec_swi.currSize++;

			//Populate the location matrix thing
			sec_swi_loc.data_1[sec_swi_loc.currSize] = indexval;	//This was originally line number in the GLM.  Not sure this is needed anymore
			sec_swi_loc.data_2[sec_swi_loc.currSize] = 0;
			sec_swi_loc.data_3[sec_swi_loc.currSize] = 0;
			sec_swi_loc.data_4[sec_swi_loc.currSize] = 0;
			sec_swi_loc.currSize++;
		}//End is recloser
		//Default else -- normal link and we don't care
	}//Second NR_branch parse

	//Do whatever this readLoads_2 thing does - forms load_matrix, which appears to just represent constant_xxx_x portions, so we're skipping it

	//Source vertex setting is above here -- needed for removing source-connected switches for WSU reconfiguration

	//Do a check to set the fault section -- see if we're reading the GLM or fault_check
	if (faulting_link == -99)
	{
		//Read the GLM -- make sure it is valid
		if (faultSecObj == nullptr)
		{
			//Not set, hard-code to zero (swing fault?  Default setting of original MATLAB code)
			f_sec.from_vert = 0;
			f_sec.to_vert = 0;
		}
		else
		{	//Proceed as normal

			//Set the fault section
			for (indexval=0; indexval<NR_branch_count; indexval++)
			{
				if (faultSecObj == NR_branchdata[indexval].obj)
				{
					f_sec.from_vert = NR_branchdata[indexval].from;
					f_sec.to_vert = NR_branchdata[indexval].to;
					break;
				}
			}
		}
	}
	else	//Read the index
	{
		f_sec.from_vert = NR_branchdata[faulting_link].from;
		f_sec.to_vert = NR_branchdata[faulting_link].to;
	}

	//Do a check
	if ((f_sec.from_vert == -1) || (f_sec.to_vert == -1))
	{
		GL_THROW("Restoration: failed to find the fault vertex");
		/*  TROUBLESHOOT
		While attempting to map the faulted line section to the graph, the edge in
		question could not be located.  Please try again.  If the error persists,
		please submit your code and a bug report via the ticketing system.
		*/
	}

	//Original MATLAB trimmed excess portions here (shouldn't need to be done - should be GLM level)

	//Allocate the feeder vertices
	INTVECTalloc(&feederVertices,numfVer);

	//Loop and populate - do reversed, since may exit faster that way
	for (secindexval=0; secindexval<numfVer; secindexval++)
	{
		//Loop through buses to see if we have a match
		for (indexval=0; indexval<NR_bus_count; indexval++)
		{
			if (fVerObjList[secindexval] == NR_busdata[indexval].obj)
			{
				feederVertices.data[feederVertices.currSize] = indexval;
				feederVertices.currSize++;
				break;	//Next loop
			}
		}//End bus traversion
	}//End feeder edge traversion

	//simplifyTop_1
	simplifyTop_1();

	//Reduced feeder vertices (ver_map_1 on feederVertices)
	INTVECTalloc(&feederVertices_1,ver_map_1.currSize);
	
	//Copy the mapping
	for (secindexval=0; secindexval<feederVertices.currSize; secindexval++)
	{
		feederVertices_1.data[secindexval] = ver_map_1.data[feederVertices.data[secindexval]];
		feederVertices_1.currSize++;
	}

	//Load in the 1st simplified topology
	//This step is skipped, because it is effectively handled natively by GLD

	//Second simplification
	simplifyTop_2();

	//Feeder vertices
	setFeederVertices_2();

	//Microgrid vertices
	setMicrogridVertices();

	//Removes the switches from source and feeders as switches -- tried to do this at the GLM level, but it breaks other things
	modifySecSwiList();

	//Form the map
	formSecSwiMap();

	//Allocate other items
	top_res = (LinkedUndigraph *)gl_malloc(sizeof(LinkedUndigraph));
	top_tmp = (LinkedUndigraph *)gl_malloc(sizeof(LinkedUndigraph));

	//Check them
	if ((top_res == nullptr) || (top_tmp == nullptr))
	{
		GL_THROW("Restoration: failed to allocte graph theory object");
		//Defined elsewhere
	}

	//Call the constructors
	new (top_res) LinkedUndigraph(0);
	new (top_tmp) LinkedUndigraph(0);

	top_tmp->copy(top_sim_1);

	//Create the powerflow backup
	PowerflowSave();

	//Create a copy of sec_swi for looping/examining
	CHORDSETalloc(&sec_list,sec_swi.maxSize);

	//Copy the values in -- do it with a memcpy, just to be different
	memcpy(sec_list.data_1,sec_swi.data_1,sec_swi.maxSize*sizeof(int));
	memcpy(sec_list.data_2,sec_swi.data_2,sec_swi.maxSize*sizeof(int));
	sec_list.currSize = sec_swi.currSize;	//Pull pointer too

	//Loop through the sec_list items
	for (secindexval=0; secindexval<sec_list.currSize; secindexval++)
	{
		//Make extract fsec value
		fsec.from_vert = sec_list.data_1[secindexval];
		fsec.to_vert = sec_list.data_2[secindexval];

		//RenewFaultLocation for current sectionalizer
		renewFaultLocation(&fsec);

		//Perform spanningTreeSearch()
		IdxSW = spanningTreeSearch();

		//Print any outputs
		if (file_output_desired)
		{
			printResult(IdxSW);
		}

		//Check mode of operation
		if (!stop_and_generate)	//"GridLAB-D" mode - exit and onward
		{
			break;	//Get out of this loop
		}
		//Default else -- Full report mode, so continue loop
	}

	//Deflag the global value
	restoration_checks_active = false;

	return 1;	//Always succeeds now, or something (unless it fails earlier - yeah, how's that for logic!?)
}


//Utility functions - string parsing
//Parse comma-separated list of doubles into array
//Assumes 1024-character input array
double *restoration::ParseDoubleString(char *input_string,int *num_items_found)
{
	int index, num_items, item_count;
	char *working_token, *parsing_token;
	double *output_array;

	//Init the pointer, for good measure
	output_array = nullptr;

	//Map up the original token
	parsing_token = input_string;
	working_token = input_string;

	//Figure out the number of tokens we have
	index = 0;
	num_items = 0;

	//Parse it
	while ((*working_token != '\0') && (index < 1024))
	{
		if (*working_token == ',')	//Found a comma!
			num_items++;			//Increment the number of tokens present
		
		index++;	//Increment the pointer
		working_token++;	//Increment the token pointer
	}

	//General checks
	if ((num_items>=0)  && (index != 0))	//Moved, but didn't find a comma - assume singular
	{
		num_items++;	//One more than commas detected
	}
	//Defaulted else - must be zero

	//Only proceed if non-zero
	if (num_items != 0)
	{
		//Allocate the array -- assume it's empty (otherwise, oops)
		output_array = (double *)gl_malloc(num_items*sizeof(double));

		//Check it
		if (output_array == nullptr)
		{
			gl_error("restoration: Failed to allocate space for double array!");
			/*  TROUBLESHOOT
			While attempting to allocate space for a double array for parsing a comma-separated list into, an
			error occurred.  Please try again.  If the error persists, please submit your code and a bug report via the
			ticketing system.
			*/
			
			//Return negative for an error
			*num_items_found = -1;
			return nullptr;
		}

		//Zero it, for good measure
		for (index=0; index<num_items; index++)
		{
			output_array[index] = 0.0;
		}

		//Now start the parsing
		item_count = 0;
		working_token = input_string;

		//If more than 1, parse differently
		if (num_items > 1)
		{
			//Loop through
			while (item_count < (num_items - 1))
			{
				//Look for a comma in the input value
				parsing_token = strchr(working_token,',');	//Look for commas, or none

				//Check it, even though we shouldn't need to
				if (parsing_token == nullptr)
				{
					gl_error("restoration: Attempting to parse a character token failed");
					/*  TROUBLESHOOT
					While attempting to parse a string of numbers, an unexpected state was encountered.
					Please check your inputs and try again.
					*/

					//Flag us as a fail
					*num_items_found = -1;
					return nullptr;
				}
				else	//Continue like normal
				{
					//Replace the comma with an end character
					*parsing_token = '\0';

					//Read the double value
					output_array[item_count] = strtod(working_token,NULL);

					//Increment counters/pointers
					item_count++;
					working_token = ++parsing_token;
				}
			}//End while
		}//End more than one
		//Defaulted else -- only 1

		//Last item/singular item handled the same
		output_array[item_count] = strtod(working_token,NULL);
	}
	else	//No items, warn and exit
	{
		gl_warning("restoration: Attempt to parse double CSV array resulted in an empty array - check your inputs");
		/*  TROUBLESHOOT
		While attempting to parse a comma-separated string into a double array, the array appears to be empty.  Ensure
		this is the correct behavior.
		*/
	}

	//Update values
	*num_items_found = num_items;

	//Return the pointer
	return output_array;
}


//Utility functions - string parsing
//Parse comma-separated list of complex values into array
//Assumes 1024-character input array
gld::complex *restoration::ParseComplexString(char *input_string,int *num_items_found)
{
	int index, num_items, item_count;
	char *working_token, *parsing_token, *complex_token, *offset_token;
	gld::complex *output_array;
	double temp_value;

	//Init the pointer, for good measure
	output_array = nullptr;

	//Map up the original token
	parsing_token = input_string;
	working_token = input_string;

	//Figure out the number of tokens we have
	index = 0;
	num_items = 0;

	//Parse it
	while ((*working_token != '\0') && (index < 1024))
	{
		if (*working_token == ',')	//Found a comma!
			num_items++;			//Increment the number of tokens present
		
		index++;	//Increment the pointer
		working_token++;	//Increment the token pointer
	}

	//General checks
	if ((num_items>=0)  && (index != 0))	//Moved, but didn't find a comma - assume singular
	{
		num_items++;	//One more than commas detected
	}
	//Defaulted else - must be zero

	//Only proceed if non-zero
	if (num_items != 0)
	{
		//Allocate the array -- assume it's empty (otherwise, oops)
		output_array = (gld::complex *)gl_malloc(num_items*sizeof(gld::complex));

		//Check it
		if (output_array == nullptr)
		{
			gl_error("restoration: Failed to allocate space for complex array!");
			/*  TROUBLESHOOT
			While attempting to allocate space for a complex array for parsing a comma-separated list into, an
			error occurred.  Please try again.  If the error persists, please submit your code and a bug report via the
			ticketing system.
			*/
			
			//Return negative for an error
			*num_items_found = -1;
			return nullptr;
		}

		//Zero it, for good measure
		for (index=0; index<num_items; index++)
		{
			output_array[index] = gld::complex(0.0,0.0);
		}

		//Now start the parsing
		item_count = 0;
		working_token = input_string;

		//If more than 1, parse differently
		if (num_items > 1)
		{
			//Loop through
			while (item_count < (num_items - 1))
			{
				//Look for a comma in the input value
				parsing_token = strchr(working_token,',');	//Look for commas, or none

				//Check it, even though we shouldn't need to
				if (parsing_token == nullptr)
				{
					gl_error("restoration: Attempting to parse a character token failed");
					/*  TROUBLESHOOT
					While attempting to parse a string of numbers, an unexpected state was encountered.
					Please check your inputs and try again.
					*/

					//Flag us as a fail
					*num_items_found = -1;
					return nullptr;
				}
				else	//Continue like normal
				{
					//Replace the comma with an end character
					*parsing_token = '\0';

					//Skip over first character, in case we were explicit
					offset_token = working_token;
					offset_token++;

					//Look for the complex split
					complex_token = strpbrk(offset_token,"+-");

					//see if a complex portion was specified
					if (complex_token == nullptr)	//No complex indicated, so we're either full real or full complex - fail
					{
						gl_error("restoration: Attempting to parse a complex character token failed");
						/*  TROUBLESHOOT
						While attempting to parse a string of numbers, an unexpected state was encountered.
						Please check your inputs and try again.
						*/

						//Flag us as a fail
						*num_items_found = -1;
						return nullptr;
					}
					else	//Complex, get the two parts
					{
						//Read complex part first, so we can "break it"

						//Remove the j
						offset_token = parsing_token;
						*--offset_token = '\0';
						offset_token = complex_token;

						//Read it
						temp_value = strtod(complex_token,NULL);

						//Store it
						output_array[item_count].SetImag(temp_value);

						//Remove the +/-
						*offset_token = '\0';

						//Read the real part
						temp_value = strtod(working_token,NULL);

						//Store it
						output_array[item_count].SetReal(temp_value);
					}

					//Increment counters/pointers
					item_count++;
					working_token = ++parsing_token;
				}
			}//End while
		}//End more than one
		//Defaulted else -- only 1

		//Last item/singular item handled the same
		//Skip over first character, in case we were explicit
		offset_token = working_token;
		offset_token++;

		//Look for the complex split
		complex_token = strpbrk(offset_token,"+-");

		//see if a complex portion was specified
		if (complex_token == nullptr)	//No complex indicated, so we're either full real or full complex - fail
		{
			gl_error("restoration: Attempting to parse a complex character token failed");
			//Defined above

			//Flag us as a fail
			*num_items_found = -1;
			return nullptr;
		}
		else	//Complex, get the two parts
		{
			//Read complex part first, so we can "break it"

			//Remove the j
			//offset_token = parsing_token;
			//*--offset_token = '\0';
			offset_token = complex_token;

			//Read it
			temp_value = strtod(complex_token,NULL);

			//Store it
			output_array[item_count].SetImag(temp_value);

			//Remove the +/-
			*offset_token = '\0';

			//Read the real part
			temp_value = strtod(working_token,NULL);

			//Store it
			output_array[item_count].SetReal(temp_value);
		}//End complex parse
	}
	else	//No items, warn and exit
	{
		gl_warning("restoration: Attempt to parse complex CSV array resulted in an empty array - check your inputs");
		/*  TROUBLESHOOT
		While attempting to parse a comma-separated string into a complex array, the array appears to be empty.  Ensure
		this is the correct behavior.
		*/
	}

	//Update values
	*num_items_found = num_items;

	//Return the pointer
	return output_array;
}

//Utility functions - string parsing
//Parse comma-separated list of complex values into array
//Assumes 1024-character input array
OBJECT **restoration::ParseObjectString(char *input_string,int *num_items_found)
{
	int index, num_items, item_count;
	char *working_token, *parsing_token;
	OBJECT **output_array;

	//Init the pointer, for good measure
	output_array = nullptr;

	//Map up the original token
	parsing_token = input_string;
	working_token = input_string;

	//Figure out the number of tokens we have
	index = 0;
	num_items = 0;

	//Parse it
	while ((*working_token != '\0') && (index < 1024))
	{
		if (*working_token == ',')	//Found a comma!
			num_items++;			//Increment the number of tokens present
		
		index++;	//Increment the pointer
		working_token++;	//Increment the token pointer
	}

	//General checks
	if ((num_items>=0)  && (index != 0))	//Moved, but didn't find a comma - assume singular
	{
		num_items++;	//One more than commas detected
	}
	//Defaulted else - must be zero

	//Only proceed if non-zero
	if (num_items != 0)
	{
		//Allocate the array -- assume it's empty (otherwise, oops)
		output_array = (OBJECT **)gl_malloc(num_items*sizeof(OBJECT *));

		//Check it
		if (output_array == nullptr)
		{
			gl_error("restoration: Failed to allocate space for object array!");
			/*  TROUBLESHOOT
			While attempting to allocate space for a object array for parsing a comma-separated list into, an
			error occurred.  Please try again.  If the error persists, please submit your code and a bug report via the
			ticketing system.
			*/
			
			//Return negative for an error
			*num_items_found = -1;
			return nullptr;
		}

		//Zero it, for good measure
		for (index=0; index<num_items; index++)
		{
			output_array[index] = nullptr;
		}

		//Now start the parsing
		item_count = 0;
		working_token = input_string;

		//If more than 1, parse differently
		if (num_items > 1)
		{
			//Loop through
			while (item_count < (num_items - 1))
			{
				//Look for a comma in the input value
				parsing_token = strchr(working_token,',');	//Look for commas, or none

				//Check it, even though we shouldn't need to
				if (parsing_token == nullptr)
				{
					gl_error("restoration: Attempting to parse a character token failed");
					/*  TROUBLESHOOT
					While attempting to parse a string of numbers, an unexpected state was encountered.
					Please check your inputs and try again.
					*/

					//Flag us as a fail
					*num_items_found = -1;
					return nullptr;
				}
				else	//Continue like normal
				{
					//Replace the comma with an end character
					*parsing_token = '\0';

					//Read the object handle value
					output_array[item_count] = gl_get_object(working_token);

					//Increment counters/pointers
					item_count++;
					working_token = ++parsing_token;
				}
			}//End while
		}//End more than one
		//Defaulted else -- only 1

		//Last item/singular item handled the same
		output_array[item_count] = gl_get_object(working_token);
	}
	else	//No items, warn and exit
	{
		gl_warning("restoration: Attempt to parse object CSV array resulted in an empty array - check your inputs");
		/*  TROUBLESHOOT
		While attempting to parse a comma-separated string into a object array, the array appears to be empty.  Ensure
		this is the correct behavior.
		*/
	}

	//Update values
	*num_items_found = num_items;

	//Return the pointer
	return output_array;
}

//Allocation functions
//INTVECT allocate
void restoration::INTVECTalloc(INTVECT *inititem, int allocsizeval)
{
	int indexval;

	//Initial check -- see if the array already has something -- if so, free it first
	//Not sure any of these ever get reused, but this will make sure this potential
	//memory hole is closed
	if (inititem->data != nullptr)
	{
		gl_free(inititem->data);

		//Null it
		inititem->data = nullptr;
	}

	inititem->currSize = 0;	//Reset initial pointer, to be safe
	inititem->maxSize = allocsizeval;	//Set max size

	//Allocate
	inititem->data = (int *)gl_malloc(allocsizeval*sizeof(int));

	//Check it
	if (inititem->data == nullptr)
	{
		GL_THROW("Restoration: failed to allocte graph theory object");
		//Defined elsewhere
	}

	//Init them all to -1, just cause
	for (indexval=0; indexval<allocsizeval; indexval++)
	{
		inititem->data[indexval] = -1;
	}
}

//INTVECT free-er
void restoration::INTVECTfree(INTVECT *inititem)
{
	//Remove all of our malloced components - if allocated (check, to prevent overzealous failures)
	if (inititem->data != nullptr)
	{
		gl_free(inititem->data);

		//Null it
		inititem->data = nullptr;
	}

	//Set variables, just in case this is used in a meaninful way
	inititem->currSize = 0;
	inititem->maxSize = 0;
}



//CHORDSET allocate
void restoration::CHORDSETalloc(CHORDSET *inititem, int allocsizeval)
{
	int indexval;

	//Initial check -- see if the array already has something -- if so, free it first
	//Not sure any of these ever get reused, but this will make sure this potential
	//memory hole is closed
	if (inititem->data_1 != nullptr)
	{
		gl_free(inititem->data_1);

		//NULL it
		inititem->data_1 = nullptr;
	}

	if (inititem->data_2 != nullptr)
	{
		gl_free(inititem->data_2);

		//NULL it
		inititem->data_2 = nullptr;
	}

	inititem->currSize = 0;	//Reset initial pointer, to be safe
	inititem->maxSize = allocsizeval;	//Set max size

	//Allocate
	inititem->data_1 = (int *)gl_malloc(allocsizeval*sizeof(int));
	inititem->data_2 = (int *)gl_malloc(allocsizeval*sizeof(int));

	//Check it
	if ((inititem->data_1 == nullptr) || (inititem->data_2 == nullptr))
	{
		GL_THROW("Restoration: failed to allocte graph theory object");
		//Defined elsewhere
	}

	//Init them all to -1, just cause
	for (indexval=0; indexval<allocsizeval; indexval++)
	{
		inititem->data_1[indexval] = -1;
		inititem->data_2[indexval] = -1;
	}
}

//CHORDSET free-er
void restoration::CHORDSETfree(CHORDSET *inititem)
{
	//Remove all of our malloced components - if allocated (check, to prevent overzealous failures)
	if (inititem->data_1 != nullptr)
	{
		gl_free(inititem->data_1);

		//NULL it
		inititem->data_1 = nullptr;
	}

	if (inititem->data_2 != nullptr)
	{
		gl_free(inititem->data_2);

		//NULL it
		inititem->data_2 = nullptr;
	}

	//Set variables, just in case this is used in a meaninful way
	inititem->currSize = 0;
	inititem->maxSize = 0;
}

//LOCSET allocate
void restoration::LOCSETalloc(LOCSET *inititem, int allocsizeval)
{
	int indexval;

	//Initial check -- see if the array already has something -- if so, free it first
	//Not sure any of these ever get reused, but this will make sure this potential
	//memory hole is closed
	if (inititem->data_1 != nullptr)
	{
		gl_free(inititem->data_1);

		//NULL it
		inititem->data_1 = nullptr;
	}

	if (inititem->data_2 != nullptr)
	{
		gl_free(inititem->data_2);

		//NULL it
		inititem->data_2 = nullptr;
	}

	if (inititem->data_3 != nullptr)
	{
		gl_free(inititem->data_3);

		//NULL it
		inititem->data_3 = nullptr;
	}

	if (inititem->data_4 != nullptr)
	{
		gl_free(inititem->data_4);

		//NULL it
		inititem->data_4 = nullptr;
	}

	inititem->currSize = 0;	//Reset initial pointer, to be safe
	inititem->maxSize = allocsizeval;	//Set max size

	//Allocate
	inititem->data_1 = (int *)gl_malloc(allocsizeval*sizeof(int));
	inititem->data_2 = (int *)gl_malloc(allocsizeval*sizeof(int));
	inititem->data_3 = (int *)gl_malloc(allocsizeval*sizeof(int));
	inititem->data_4 = (int *)gl_malloc(allocsizeval*sizeof(int));

	//Check it
	if ((inititem->data_1 == nullptr) || (inititem->data_2 == nullptr) || (inititem->data_3 == nullptr) || (inititem->data_4 == nullptr))
	{
		GL_THROW("Restoration: failed to allocte graph theory object");
		//Defined elsewhere
	}

	//Init them all to -1, just cause
	for (indexval=0; indexval<allocsizeval; indexval++)
	{
		inititem->data_1[indexval] = -1;
		inititem->data_2[indexval] = -1;
		inititem->data_3[indexval] = -1;
		inititem->data_4[indexval] = -1;
	}
}

//Function to free up LOCSET allocations
void restoration::LOCSETfree(LOCSET *inititem)
{
	//Check individual items for existance
	if (inititem->data_1 != nullptr)
	{
		gl_free(inititem->data_1);

		//NULL it
		inititem->data_1 = nullptr;
	}

	if (inititem->data_2 != nullptr)
	{
		gl_free(inititem->data_2);

		//NULL it
		inititem->data_2 = nullptr;
	}

	if (inititem->data_3 != nullptr)
	{
		gl_free(inititem->data_3);

		//NULL it
		inititem->data_3 = nullptr;
	}

	if (inititem->data_4 != nullptr)
	{
		gl_free(inititem->data_4);

		//NULL it
		inititem->data_4 = nullptr;
	}

	//Set values, just to be safe
	inititem->currSize = 0;
	inititem->maxSize = 0;
}

//CANDSWOP allocate
void restoration::CANDSWOPalloc(CANDSWOP *inititem, int allocsizeval)
{
	int indexval;

	//Initial check -- see if the array already has something -- if so, free it first
	//Not sure any of these ever get reused, but this will make sure this potential
	//memory hole is closed
	if (inititem->data_1 != nullptr)
	{
		gl_free(inititem->data_1);

		//NULL it
		inititem->data_1 = nullptr;
	}

	if (inititem->data_2 != nullptr)
	{
		gl_free(inititem->data_2);

		//NULL it
		inititem->data_2 = nullptr;
	}

	if (inititem->data_3 != nullptr)
	{
		gl_free(inititem->data_3);

		//NULL it
		inititem->data_3 = nullptr;
	}

	if (inititem->data_4 != nullptr)
	{
		gl_free(inititem->data_4);

		//NULL it
		inititem->data_4 = nullptr;
	}

	if (inititem->data_5 != nullptr)
	{
		gl_free(inititem->data_5);

		//NULL it
		inititem->data_5 = nullptr;
	}

	if (inititem->data_6 != nullptr)
	{
		gl_free(inititem->data_6);

		//NULL it
		inititem->data_6 = nullptr;
	}

	if (inititem->data_7 != nullptr)
	{
		gl_free(inititem->data_7);

		//NULL it
		inititem->data_7 = nullptr;
	}

	inititem->currSize = 0;	//Reset initial pointer, to be safe
	inititem->maxSize = allocsizeval;	//Set max size

	//Allocate ints
	inititem->data_1 = (int *)gl_malloc(allocsizeval*sizeof(int));
	inititem->data_2 = (int *)gl_malloc(allocsizeval*sizeof(int));
	inititem->data_3 = (int *)gl_malloc(allocsizeval*sizeof(int));
	inititem->data_4 = (int *)gl_malloc(allocsizeval*sizeof(int));
	inititem->data_5 = (int *)gl_malloc(allocsizeval*sizeof(int));
	inititem->data_7 = (int *)gl_malloc(allocsizeval*sizeof(int));

	//Check first 3 ints
	if ((inititem->data_1 == nullptr) || (inititem->data_2 == nullptr) || (inititem->data_3 == nullptr))
	{
		GL_THROW("Restoration: failed to allocte graph theory object");
		//Defined elsewhere
	}

	//Check second 3 ints
	if ((inititem->data_4 == nullptr) || (inititem->data_5 == nullptr) || (inititem->data_7 == nullptr))
	{
		GL_THROW("Restoration: failed to allocte graph theory object");
		//Defined elsewhere
	}

	//Allocate double
	inititem->data_6 = (double *)gl_malloc(allocsizeval*sizeof(double));

	//Check double
	if (inititem->data_6 == nullptr)
	{
		GL_THROW("Restoration: failed to allocte graph theory object");
		//Defined elsewhere
	}

	//Init them all to -1, just cause
	for (indexval=0; indexval<allocsizeval; indexval++)
	{
		inititem->data_1[indexval] = -1;
		inititem->data_2[indexval] = -1;
		inititem->data_3[indexval] = -1;
		inititem->data_4[indexval] = -1;
		inititem->data_5[indexval] = -1;
		inititem->data_7[indexval] = -1;

		//Double
		inititem->data_6[indexval] = 0.0;
	}
}

//Function to free up CANDSWOP allocations
void restoration::CANDSWOPfree(CANDSWOP *inititem)
{
	//Check individual items for existance
	if (inititem->data_1 != nullptr)
	{
		gl_free(inititem->data_1);

		//NULL it
		inititem->data_1 = nullptr;
	}

	if (inititem->data_2 != nullptr)
	{
		gl_free(inititem->data_2);

		//NULL it
		inititem->data_2 = nullptr;
	}

	if (inititem->data_3 != nullptr)
	{
		gl_free(inititem->data_3);

		//NULL it
		inititem->data_3 = nullptr;
	}

	if (inititem->data_4 != nullptr)
	{
		gl_free(inititem->data_4);

		//NULL it
		inititem->data_4 = nullptr;
	}

	if (inititem->data_5 != nullptr)
	{
		gl_free(inititem->data_5);

		//NULL it
		inititem->data_5 = nullptr;
	}

	if (inititem->data_6 != nullptr)
	{
		gl_free(inititem->data_6);

		//NULL it
		inititem->data_6 = nullptr;
	}

	if (inititem->data_7 != nullptr)
	{
		gl_free(inititem->data_7);

		//NULL it
		inititem->data_7 = nullptr;
	}

	//Set values, just to be safe
	inititem->currSize = 0;
	inititem->maxSize = 0;
}

//1st simplification -- all non-switch edges are deleted
//The faulted edge must not be deleted
void restoration::simplifyTop_1(void)
{
	int idx, v, s, jIndex, iIndex;
	int top_ori_VerNum, top_ori_VerNum_2;

    // Initialize the map for the 1st simplification
		top_ori_VerNum = top_ori->getVerNum();
		INTVECTalloc(&ver_map_1,top_ori_VerNum);

		//Now populate it
		for (idx=0; idx<ver_map_1.maxSize; idx++)
		{
			ver_map_1.data[idx] = idx;
		}

		//Set top variable
		ver_map_1.currSize = ver_map_1.maxSize;
    
    // Initialize top_sim_1 -- allocate first
		top_sim_1 = (LinkedUndigraph *)gl_malloc(sizeof(LinkedUndigraph));

		//Check it
		if (top_sim_1 == nullptr)
		{
			GL_THROW("Restoration: failed to allocte graph theory object");
			//Defined elsewhere
		}

		//Call the constructor
		new (top_sim_1) LinkedUndigraph(0);
	    //top_sim_1 = LinkedUndigraph();
	
	top_sim_1->copy(top_ori);
    
    // Merge the two ends (vertices) of non-switch devices
    // Initialize the iterator
	top_ori->initializePos();

    // Form the map of vertices
	top_ori_VerNum = top_ori->getVerNum();
	for (idx=0; idx<top_ori_VerNum; idx++)
	{
		v = top_ori->beginVertex(idx);

		while (v != -1)
		{
			if ((v>idx) && !isSwitch(idx,v) && ((v!=f_sec.from_vert) || (idx!=f_sec.to_vert)) && ((v!=f_sec.to_vert) || (idx!=f_sec.from_vert)))
			{
				//iIndex = min(resObj.ver_map_1(idx),resObj.ver_map_1(v));
				//jIndex = max(resObj.ver_map_1(idx),resObj.ver_map_1(v));
				if (ver_map_1.data[idx] < ver_map_1.data[v])
				{
					iIndex = ver_map_1.data[idx];
					jIndex = ver_map_1.data[v];
				}
				else
				{
					iIndex = ver_map_1.data[v];
					jIndex = ver_map_1.data[idx];
				}

				top_ori_VerNum_2 = top_ori->getVerNum();
				for (s=0; s<top_ori_VerNum_2; s++)
				{
					if (ver_map_1.data[s] == jIndex)
					{
						ver_map_1.data[s] = iIndex;
					}
					else if (ver_map_1.data[s] > jIndex)
					{
						ver_map_1.data[s] = ver_map_1.data[s] - 1;
					}
				}
			}
			v = top_ori->nextVertex(idx);
		}
	}

    //Deactivate the iterator
	top_ori->deactivatePos();

    //Merge vertices
	top_sim_1->mergeVer_2(&ver_map_1);
    
    //Update the index of switches, faulted section and source
    //vertex
    //Sectionalizing switches
	for (idx=0; idx<sec_swi.currSize; idx++)
	{
		sec_swi_1.data_1[idx] = ver_map_1.data[sec_swi.data_1[idx]];
		sec_swi_1.data_2[idx] = ver_map_1.data[sec_swi.data_2[idx]];
	}

	//Update index
	sec_swi_1.currSize = sec_swi.currSize;

    //Tie switches
	for (idx=0; idx<tie_swi.currSize; idx++)
	{
		tie_swi_1.data_1[idx] = ver_map_1.data[tie_swi.data_1[idx]];
		tie_swi_1.data_2[idx] = ver_map_1.data[tie_swi.data_2[idx]];
	}

	//Update index
	tie_swi_1.currSize = tie_swi.currSize;

    //Faulted section
	f_sec_1.from_vert = ver_map_1.data[f_sec.from_vert];
	f_sec_1.to_vert = ver_map_1.data[f_sec.to_vert];

	// Source Vertex
	s_ver_1 = ver_map_1.data[s_ver];
}

// The 2nd Simplification, all vertices whose degree equal to 1 or 2
// are deleted. The source node, vertices with tie switches or the
// faulted section should not be deleted.
void restoration::simplifyTop_2(void)
{
	int idx, uIdx, vIdx;
	int top_sim_2_getVerNum;
	int allocsizeval;
	CHAINNODE *v;

    // Initialize top_sim_2 -- allocate first
		top_sim_2 = (LinkedUndigraph *)gl_malloc(sizeof(LinkedUndigraph));

		//Check it
		if (top_sim_2 == nullptr)
		{
			GL_THROW("Restoration: failed to allocte graph theory object");
			//Defined elsewhere
		}

		//Call the constructor
		new (top_sim_2) LinkedUndigraph(0);
	    //resObj.top_sim_2 = LinkedUndigraph();

	top_sim_2->copy(top_sim_1);

	//Allocate ver_map_2 -- guess that it will be based on ver_map_1's size
	//************ Arbitrary guess here **************/
	INTVECTalloc(&ver_map_2,ver_map_1.maxSize);

	// Simplify top_sim_2
	top_sim_2->simplify(&tie_swi_1, &f_sec_1, s_ver_1, &feederVertices_1, &ver_map_2);

	// Update the index of switches, faulted section and source vertex
	// Tie switches
	for (idx=0; idx<tie_swi_1.currSize; idx++)
	{
		tie_swi_2.data_1[idx] = ver_map_2.data[tie_swi_1.data_1[idx]];
		tie_swi_2.data_2[idx] = ver_map_2.data[tie_swi_1.data_2[idx]];
	}

	//Update size
	tie_swi_2.currSize = tie_swi_1.currSize;

	// Faulted section
	f_sec_2.from_vert = ver_map_2.data[f_sec_1.from_vert];
	f_sec_2.to_vert = ver_map_2.data[f_sec_1.to_vert];

	// Source Vertex
	s_ver_2 = ver_map_2.data[s_ver_1];

	// Sectionalizing switches
	allocsizeval = top_sim_2->getVerNum();	//Arbitrary change, since it was blowing the array size below - was getEdgeNum - 1 - this aligns with FOR loop below
	CHORDSETalloc(&sec_swi_2,allocsizeval);

	top_sim_2_getVerNum = top_sim_2->getVerNum();
	for (uIdx=0; uIdx<top_sim_2_getVerNum; uIdx++)
	{
		v = top_sim_2->adjList[uIdx]->first;

		while (v != nullptr)
		{
			vIdx = v->data;
			if (!(vIdx > uIdx)  && ((((uIdx==f_sec_2.from_vert) && (vIdx==f_sec_2.to_vert)) || ((uIdx==f_sec_2.to_vert) && (vIdx==f_sec_2.from_vert)))))
			{
				sec_swi_2.data_1[sec_swi_2.currSize] = uIdx;
				sec_swi_2.data_2[sec_swi_2.currSize] = vIdx;
				sec_swi_2.currSize++;

				if (sec_swi_2.currSize >= sec_swi_2.maxSize)	//Check, since we had problems with the original code, as written
				{
					//Being equal is just of note for debugging (if we come back, we'll be upset)
					if (sec_swi_2.currSize == sec_swi_2.maxSize)
					{
						gl_verbose("sec_swi_2 just hit maximum length");
					}
					else	//Must be greater, we're already broken
					{
						GL_THROW("restoration: allocated array size exceeded!");
						/*  TROUBLESHOOT
						While performing an operation, the maximum size of the array allocated was exceeded.  Please submit your
						code and a bug report via the ticketing system.
						*/
					}
				}
			}
			v = v->link;
		}
	}
}

//Set feeder vertices
void restoration::setFeederVertices_2(void)
{
	int idx, curNode, nodeSetIdx;
	CHAINNODE *tNode;
	INTVECT nodeSet;

	//Empty it, to be safe
	nodeSet.data = nullptr;

	//Allocate the temporary vector - assume won't be bigger than feederVertices_1? (failed horribly on feederVertices)
	INTVECTalloc(&nodeSet,feederVertices_1.maxSize);

	//Allocate the feederVertices_2 vector
	INTVECTalloc(&feederVertices_2,feederVertices_1.maxSize);
	
	//Arbitrarily size, since "NULL/-1" seems to be a status vector of this???
	feederVertices_2.currSize = feederVertices_2.maxSize;

	for (idx=0; idx<feederVertices.currSize; idx++)
	{
		//Start from zero, again
		nodeSetIdx = 0;
		nodeSet.data[nodeSetIdx] = feederVertices_1.data[idx];
		nodeSet.currSize = 1;

		while ((nodeSet.data[nodeSetIdx] != -1) && (nodeSetIdx < nodeSet.currSize))
		{
			curNode = nodeSet.data[nodeSetIdx];
			nodeSetIdx++;
			
			if (ver_map_2.data[curNode] != -1)
			{
				feederVertices_2.data[idx] = ver_map_2.data[curNode];
				break;
			}
			else
			{
				if (top_sim_1->adjList[curNode] != nullptr)
				{
					tNode = top_sim_1->adjList[curNode]->first;
				}
				else
				{
					tNode = nullptr;
				}

				while (tNode != nullptr)
				{
					if (tNode->data != s_ver_1)
					{
						nodeSet.data[nodeSet.currSize] = tNode->data;
						nodeSet.currSize++;
					}
					tNode = tNode->link;
				}
			}
		}
	}

	//De-allocate the working variable
	gl_free(nodeSet.data);
}

//Set microgrid vertices
void restoration::setMicrogridVertices(void)
{
	int idxval;

	//Do first transfer
	for (idxval=0; idxval<MGIdx.currSize; idxval++)
	{
		MGIdx_1.data[idxval] = ver_map_1.data[MGIdx.data[idxval]];
	}
	MGIdx_1.currSize = MGIdx.currSize;

	//Do second transfer
	for (idxval=0; idxval<MGIdx_1.currSize; idxval++)
	{
		MGIdx_2.data[idxval] = ver_map_2.data[MGIdx_1.data[idxval]];
	}
	MGIdx_2.currSize = MGIdx_1.currSize;
}

// The line between source vertex and feeder vertices are modeled as
// sectionalizing switches in gridlab-d model. But actually they are
// not. This function remove these line from the list of
// sectionalizing swithces
//
//Do this in a much more simplified manner - just look for it matching and remove it (GLD approach)
//This isn't an elegant solution, but works for now (can make it pretty later)
void restoration::modifySecSwiList(void)
{
	CHORDSET sec_swi_temp, sec_swi_1_temp, sec_swi_2_temp;
	LOCSET sec_swi_loc_temp;
	int indexval;

	//Null them all, just in case (paranoia)
	sec_swi_temp.data_1 = nullptr;
	sec_swi_temp.data_2 = nullptr;
	sec_swi_1_temp.data_1 = nullptr;
	sec_swi_1_temp.data_2 = nullptr;
	sec_swi_2_temp.data_1 = nullptr;
	sec_swi_2_temp.data_2 = nullptr;
	sec_swi_loc_temp.data_1 = nullptr;
	sec_swi_loc_temp.data_2 = nullptr;
	sec_swi_loc_temp.data_3 = nullptr;
	sec_swi_loc_temp.data_4 = nullptr;

	//Allocate up two new arrays, temporarily
	CHORDSETalloc(&sec_swi_temp,sec_swi.maxSize);
	CHORDSETalloc(&sec_swi_1_temp,sec_swi_1.maxSize);
	CHORDSETalloc(&sec_swi_2_temp,sec_swi_2.maxSize);
	LOCSETalloc(&sec_swi_loc_temp,sec_swi_loc.maxSize);

	//Copy the old ones into these
	memcpy(sec_swi_temp.data_1,sec_swi.data_1,sec_swi.maxSize*sizeof(int));
	memcpy(sec_swi_temp.data_2,sec_swi.data_2,sec_swi.maxSize*sizeof(int));
	memcpy(sec_swi_1_temp.data_1,sec_swi_1.data_1,sec_swi_1.maxSize*sizeof(int));
	memcpy(sec_swi_1_temp.data_2,sec_swi_1.data_2,sec_swi_1.maxSize*sizeof(int));
	memcpy(sec_swi_2_temp.data_1,sec_swi_2.data_1,sec_swi_2.maxSize*sizeof(int));
	memcpy(sec_swi_2_temp.data_2,sec_swi_2.data_2,sec_swi_2.maxSize*sizeof(int));

	//Including the useless locset thing
	memcpy(sec_swi_loc_temp.data_1,sec_swi_loc.data_1,sec_swi_loc.maxSize*sizeof(int));
	memcpy(sec_swi_loc_temp.data_2,sec_swi_loc.data_2,sec_swi_loc.maxSize*sizeof(int));
	memcpy(sec_swi_loc_temp.data_3,sec_swi_loc.data_3,sec_swi_loc.maxSize*sizeof(int));
	memcpy(sec_swi_loc_temp.data_4,sec_swi_loc.data_4,sec_swi_loc.maxSize*sizeof(int));

	//Set currsize, just for tracking
	sec_swi_temp.currSize = sec_swi.currSize;
	sec_swi_1_temp.currSize = sec_swi_1.currSize;
	sec_swi_2_temp.currSize = sec_swi_2.currSize;
	sec_swi_loc_temp.currSize = sec_swi_loc.currSize;

	//Zero the old, just because I say so - could theoretically just "-1" out the existing entry
	//But this code would probably not like that and I'm too lazy to track down the instances
	//Assume sec_swi_loc is the same size
	for (indexval=0; indexval<sec_swi.maxSize; indexval++)
	{
		sec_swi.data_1[indexval] = -1;
		sec_swi.data_2[indexval] = -1;

		sec_swi_loc.data_1[indexval] = -1;
		sec_swi_loc.data_2[indexval] = -1;
		sec_swi_loc.data_3[indexval] = -1;
		sec_swi_loc.data_4[indexval] = -1;
	}

	//Set location
	sec_swi.currSize = 0;
	sec_swi_loc.currSize = 0;

	//Loop through and store values, if not the source vertex
	for (indexval=0; indexval<sec_swi_temp.currSize; indexval++)
	{
		if ((sec_swi_temp.data_1[indexval] != s_ver) && (sec_swi_temp.data_2[indexval] != s_ver))
		{
			//Not a source vertex, allow it
			sec_swi.data_1[sec_swi.currSize] = sec_swi_temp.data_1[indexval];
			sec_swi.data_2[sec_swi.currSize] = sec_swi_temp.data_2[indexval];

			//Same for loc stuff
			sec_swi_loc.data_1[sec_swi_loc.currSize] = sec_swi_loc_temp.data_1[indexval];
			sec_swi_loc.data_2[sec_swi_loc.currSize] = sec_swi_loc_temp.data_2[indexval];
			sec_swi_loc.data_3[sec_swi_loc.currSize] = sec_swi_loc_temp.data_3[indexval];
			sec_swi_loc.data_4[sec_swi_loc.currSize] = sec_swi_loc_temp.data_4[indexval];

			//Update
			sec_swi.currSize++;
			sec_swi_loc.currSize++;
		}
		//Default else -- we need to be excluded.  Poor us :(
	}

	//Do the same for sec_swi_1
	for (indexval=0; indexval<sec_swi_1.maxSize; indexval++)
	{
		sec_swi_1.data_1[indexval] = -1;
		sec_swi_1.data_2[indexval] = -1;
	}

	//Set location
	sec_swi_1.currSize = 0;

	//Loop through and store values, if not the source vertex
	for (indexval=0; indexval<sec_swi_1_temp.currSize; indexval++)
	{
		if ((sec_swi_1_temp.data_1[indexval] != s_ver_1) && (sec_swi_1_temp.data_2[indexval] != s_ver_1))
		{
			//Not a source vertex, allow it
			sec_swi_1.data_1[sec_swi_1.currSize] = sec_swi_1_temp.data_1[indexval];
			sec_swi_1.data_2[sec_swi_1.currSize] = sec_swi_1_temp.data_2[indexval];

			//Update
			sec_swi_1.currSize++;
		}
		//Default else -- we need to be excluded.  Poor us :(
	}

	//Do the same for sec_swi_2
	for (indexval=0; indexval<sec_swi_2.maxSize; indexval++)
	{
		sec_swi_2.data_1[indexval] = -1;
		sec_swi_2.data_2[indexval] = -1;
	}

	//Set location
	sec_swi_2.currSize = 0;

	//Loop through and store values, if not the source vertex
	for (indexval=0; indexval<sec_swi_2_temp.currSize; indexval++)
	{
		if ((sec_swi_2_temp.data_1[indexval] != s_ver_2) && (sec_swi_2_temp.data_2[indexval] != s_ver_2))
		{
			//Not a source vertex, allow it
			sec_swi_2.data_1[sec_swi_2.currSize] = sec_swi_2_temp.data_1[indexval];
			sec_swi_2.data_2[sec_swi_2.currSize] = sec_swi_2_temp.data_2[indexval];

			//Update
			sec_swi_2.currSize++;
		}
		//Default else -- we need to be excluded.  Poor us :(
	}

	//Free up stuffs
	CHORDSETfree(&sec_swi_temp);
	CHORDSETfree(&sec_swi_1_temp);
	CHORDSETfree(&sec_swi_2_temp);
	LOCSETfree(&sec_swi_loc_temp);
}

//No description in the MATLAB
void restoration::formSecSwiMap(void)
{
	int idx;
	BRANCHVERTICES sSW, sSW_1, inputBranch;

	//Allocate items
	CHORDSETalloc(&sec_swi_map,sec_swi_2.currSize);
	CHORDSETalloc(&sec_swi_map_1,sec_swi_2.currSize);

	for (idx=0; idx<sec_swi_2.currSize; idx++)
	{
		inputBranch.from_vert = sec_swi_2.data_1[idx];
		inputBranch.to_vert = sec_swi_2.data_2[idx];
		mapSecSwi(&inputBranch,&sSW,&sSW_1);
		
		sec_swi_map.data_1[idx] = sSW.from_vert;
		sec_swi_map.data_2[idx] = sSW.to_vert;
		sec_swi_map_1.data_1[idx] = sSW_1.from_vert;
		sec_swi_map_1.data_2[idx] = sSW_1.to_vert;
	}

	//Update size pointers
	sec_swi_map.currSize = sec_swi_2.currSize;
	sec_swi_map_1.currSize = sec_swi_2.currSize;
}

//No MATLAB description for function -- presumably located Sectionalizing switches in some manner
void restoration::mapSecSwi(BRANCHVERTICES *sSW_2, BRANCHVERTICES *sSW, BRANCHVERTICES *sSW_1)
{
	int idx, uIdx, vIdx, temp, detDistInt;
	double detDist;

	// sSW_1
	// An edge in top_sim_2 may present more than 1 edge in top_sim_1
	// BFS for top_sim_1
	top_sim_1->BFS(s_ver_1);
	// 

	//Do the seaches as kludgy for loops for now -- single solutions expected
	uIdx = -1;
	vIdx = -1;
	
	//uIdx = find(resObj.ver_map_2==sSW_2(1));
		for (idx=0; idx<ver_map_2.currSize; idx++)
		{
			if (ver_map_2.data[idx] == sSW_2->from_vert)
			{
				uIdx = idx;
				break;
			}
		}

		//Check for failure
		if (uIdx == -1)
		{
			GL_THROW("Restoration: failed to find desired vertex in mapping list");
			/*  TROUBLESHOOT
			While parsing a mapping list of vertices for the graph theory analysis, a desired
			vertex could not be found.  Please try again.  If the error persists, please submit your
			code and a bug report via the ticketing system.
			*/
		}

	//vIdx = find(resObj.ver_map_2==sSW_2(2));
		for (idx=0; idx<ver_map_2.currSize; idx++)
		{
			if (ver_map_2.data[idx] == sSW_2->to_vert)
			{
				vIdx = idx;
				break;
			}
		}

		//Check for failure
		if (vIdx == -1)
		{
			GL_THROW("Restoration: failed to find desired vertex in mapping list");
			//Defined above
		}

	if (top_sim_1->dist[uIdx] > top_sim_1->dist[vIdx])
	{
		temp = uIdx;
		uIdx = vIdx;
		vIdx = temp;
	}
	//Combined this with the next line in the original MATLAB
	detDist = (top_sim_1->dist[vIdx] - top_sim_1->dist[uIdx])/2.0;
	detDistInt = (int)detDist;

	for (idx=0; idx<detDistInt; idx++)
	{
		vIdx = top_sim_1->parent_value[vIdx];
	}
	sSW_1->to_vert = vIdx;
	sSW_1->from_vert = top_sim_1->parent_value[vIdx];

	// sSW
	idx = 0;

	//Was while (1), but that seems like it could cause problems - bound by size
	while (idx<sec_swi_1.currSize)
	{
		if ((sec_swi_1.data_1[idx] == sSW_1->from_vert) && (sec_swi_1.data_2[idx] == sSW_1->to_vert))
		{
			sSW->from_vert = sec_swi.data_1[idx];
			sSW->to_vert = sec_swi.data_2[idx];
			break;
		}
		if ((sec_swi_1.data_1[idx] == sSW_1->to_vert) && (sec_swi_1.data_2[idx] == sSW_1->from_vert))
		{
			sSW->from_vert = sec_swi.data_2[idx];
			sSW->to_vert = sec_swi.data_1[idx];
			break;
		}
		idx++;
	}
}

//No MATLAB description for function -- presumably locates tie switches in some manner
void restoration::mapTieSwi(BRANCHVERTICES *tSW_2, BRANCHVERTICES *tSW, BRANCHVERTICES *tSW_1)
{
	int idx;

	idx = 0;

	//Initialize to bad values - for giggles (may check later -- was absent from MATLAB)
	tSW->from_vert = -1;
	tSW->to_vert = -1;
	tSW_1->from_vert = -1;
	tSW_1->to_vert = -1;

	//Was while (1), but that could cause problems
	while (idx<tie_swi_2.currSize)
	{
		if ((tie_swi_2.data_1[idx]==tSW_2->from_vert) && (tie_swi_2.data_2[idx]==tSW_2->to_vert))
		{
			tSW->from_vert = tie_swi.data_1[idx];
			tSW->to_vert = tie_swi.data_2[idx];
			tSW_1->from_vert = tie_swi_1.data_1[idx];
			tSW_1->to_vert = tie_swi_1.data_2[idx];
			break;
		}

		if ((tie_swi_2.data_2[idx]==tSW_2->from_vert) && (tie_swi_2.data_1[idx]==tSW_2->to_vert))
		{
			tSW->from_vert = tie_swi.data_2[idx];
			tSW->to_vert = tie_swi.data_1[idx];
			tSW_1->from_vert = tie_swi_1.data_2[idx];
			tSW_1->to_vert = tie_swi_1.data_1[idx];
			break;
		}
		
		idx++;
	}
}

//No description from MATLAB
void restoration::renewFaultLocation(BRANCHVERTICES *faultsection)
{
	// Set fault section
	f_sec.from_vert = faultsection->from_vert;
	f_sec.to_vert = faultsection->to_vert;

	// Faulted section
	f_sec_1.from_vert = ver_map_1.data[f_sec.from_vert];
	f_sec_1.to_vert = ver_map_1.data[f_sec.to_vert];

	// 2nd simplification
	simplifyTop_2();

	// Feeder vertices
	setFeederVertices_2();

	// Microgrid vertices
	setMicrogridVertices();

	// modify lists of sectionalizing switches
	//Removes the switches from source and feeders as switches -- tried to do this at the GLM level, but it breaks other things
	modifySecSwiList();

	//Form the map
	formSecSwiMap();

	// Graph after restoration
	//These were theoretically allocated earlier
	if (top_res != nullptr)	//As an attempt at fixing possible memory leaks, try to empty them first
	{
		top_res->delAllVer();
	}

	if (top_tmp != nullptr)
	{
		top_tmp->delAllVer();
	}

	//Now call the constructors again
	// ********************* may get upset about this, so may need to destroy them completely first

	new (top_res) LinkedUndigraph(0);
	new (top_tmp) LinkedUndigraph(0);

	//Copy the values in
	top_tmp->copy(top_sim_1);
}

// Full Restoration by spanning tree search
// Perform full restoration to the system for given fault
// If outage loads can be full restored, return the index for the,
// last switch operation, and save switching sequence in swi_seq; 
// otherwise, return 0.a.
int restoration::spanningTreeSearch(void)
{
	int idx, counter, preCounter, feederID, feeder_overloaded, k, tvi, startIdx, allocsize;
	int powerflow_result;
	CHORDSET FCutSet, FCutSet_1, FCutSet_2, FCutSet_2_1, FCutSet_2_2, new_tie_swi;
	BRANCHVERTICES FCutSetentry, FCutSet_1entry, FCutSet_2entry;
	BRANCHVERTICES SW_to_Open, SW_to_Open_1, SW_to_Open_2;
	BRANCHVERTICES sec_swi_2entry;
	bool feasible, swiInFeederBool;
	double overLoad;

	//Initialize local variables -- just in case
	FCutSet.data_1 = nullptr;
	FCutSet.data_2 = nullptr;
	FCutSet_1.data_1 = nullptr;
	FCutSet_1.data_2 = nullptr;
	FCutSet_2.data_1 = nullptr;
	FCutSet_2.data_2 = nullptr;
	FCutSet_2_1.data_1 = nullptr;
	FCutSet_2_1.data_2 = nullptr;
	FCutSet_2_2.data_1 = nullptr;
	FCutSet_2_2.data_2 = nullptr;
	new_tie_swi.data_1 = nullptr;
	new_tie_swi.data_2 = nullptr;

	//Feasibility flag - default to infeasible
	feasible = false;

	//Allocate chord sets -- make them maximally big
	CHORDSETalloc(&FCutSet_2_2,tie_swi_2.maxSize);
	CHORDSETalloc(&FCutSet_2_1,tie_swi_2.maxSize);
	CHORDSETalloc(&FCutSet_2,tie_swi_2.maxSize);
	CHORDSETalloc(&FCutSet_1,tie_swi_2.maxSize);
	CHORDSETalloc(&FCutSet,tie_swi_2.maxSize);

	// Find the fundamental cut set of the fault section
	top_sim_2->BFS(s_ver_2);

	top_sim_2->findFunCutSet(&tie_swi_2,&f_sec_2,&FCutSet_2);

	for (idx=0; idx<FCutSet_2.currSize; idx++)
	{
		FCutSet_2entry.from_vert = FCutSet_2.data_1[idx];
		FCutSet_2entry.to_vert = FCutSet_2.data_2[idx];

		mapTieSwi(&FCutSet_2entry,&FCutSetentry, &FCutSet_1entry);

		//Store values
		FCutSet.data_1[idx] = FCutSetentry.from_vert;
		FCutSet.data_2[idx] = FCutSetentry.to_vert;
		FCutSet_1.data_1[idx] = FCutSet_1entry.from_vert;
		FCutSet_1.data_2[idx] = FCutSet_1entry.to_vert;
	}

	//Set size for later items
	FCutSet.currSize = FCutSet_2.currSize;
	FCutSet_1.currSize = FCutSet_2.currSize;

	// Save all candidate solutions
		//Allocate the switch operation fields - go on assumption of for loop below
		//Make as big as fundamental switch set

		//Still not sure how this is being sized -- this guess seems to work
		//Revisit this!! Seems like it should be FCutSet_2.currSize
		if (numMG > 0)
		{
			allocsize = 2*numMG*numfVer*(tie_swi.currSize + sec_swi.currSize);	//Arbitrarily sized - MATLAB is sloppy
		}
		else
		{
			allocsize = 2*numfVer*(tie_swi.currSize + sec_swi.currSize);	//Arbitrarily sized - MATLAB is sloppy
		}

		CANDSWOPalloc(&candidateSwOpe,allocsize);
		CANDSWOPalloc(&candidateSwOpe_1,allocsize);
		CANDSWOPalloc(&candidateSwOpe_2,allocsize);

	for (idx=0; idx<FCutSet_2.currSize; idx++)
	{
		candidateSwOpe_2.data_1[idx] = f_sec_2.from_vert;
		candidateSwOpe_2.data_2[idx] = f_sec_2.to_vert;
		candidateSwOpe_2.data_3[idx] = FCutSet_2.data_1[idx];
		candidateSwOpe_2.data_4[idx] = FCutSet_2.data_2[idx];
		candidateSwOpe_2.data_5[idx] = 0;
		candidateSwOpe_2.data_6[idx] = 0.0;
		candidateSwOpe_2.data_7[idx] = 0;

		candidateSwOpe_1.data_1[idx] = f_sec_1.from_vert;
		candidateSwOpe_1.data_2[idx] = f_sec_1.to_vert;
		candidateSwOpe_1.data_3[idx] = FCutSet_1.data_1[idx];
		candidateSwOpe_1.data_4[idx] = FCutSet_1.data_2[idx];
		candidateSwOpe_1.data_5[idx] = 0;
		candidateSwOpe_1.data_6[idx] = 0.0;
		candidateSwOpe_1.data_7[idx] = 0;

		candidateSwOpe.data_1[idx] = f_sec.from_vert;
		candidateSwOpe.data_2[idx] = f_sec.to_vert;
		candidateSwOpe.data_3[idx] = FCutSet.data_1[idx];
		candidateSwOpe.data_4[idx] = FCutSet.data_2[idx];
		candidateSwOpe.data_5[idx] = 0;
		candidateSwOpe.data_6[idx] = 0.0;
		candidateSwOpe.data_7[idx] = 0;
	}

	//Set sizes for later
	candidateSwOpe.currSize = FCutSet_2.currSize;
	candidateSwOpe_1.currSize = FCutSet_2.currSize;
	candidateSwOpe_2.currSize = FCutSet_2.currSize;

	// Search for spanning trees without duplication
	counter = 0;	//Was 1 in the MATLAB, but it's an index

	while (counter < candidateSwOpe.currSize)
	{
		if (candidateSwOpe.data_5[counter] == 0)
		{
			//Adjustment from WSU code below - just run a powerflow
			//If it fails, then modifyModel again (should de-toggle all of what was just toggled)

				//Initialize storage variables, in case we fail
				overLoad = 0.0;
				feederID = 0;

				//Perform the modification
				modifyModel(counter);

				// Run power flow
				powerflow_result = runPowerFlow();
				
				//See if it even worked -- if not, modifyModel again and set as a "false"
				if (powerflow_result == -1)
				{
					return -2;	//Serious error occurred, so flag us as "really bad"
					//basically, the state of the system may be corrupted, so any subsequent powerflows can't be trusted
				}
				else if (powerflow_result == 0)
				{
					//Call the modify function again, to undo what we just did
					modifyModel(counter);

					//Set us as invalid
					feasible=false;

					//Restore voltage for next pass
					PowerflowRestore();
				}
				else	//Success!?
				{
					//Check results
					checkPF2(&feasible, &overLoad, &feederID);

					//Check feasible again -- if not feasible, undo the operations again
					if (!feasible)
					{
						modifyModel(counter);	//Undo it by calling it again

						//Restore voltage for next pass
						PowerflowRestore();
					}
				}
	        
			// If feasible restoration scheme is found
			if (feasible)
			{
				//Check our desired "approach" on this -- see if we need to undo for the next section or not
				if (stop_and_generate)
				{
					//Undo it by calling it again
					modifyModel(counter);

					//Restore voltage for next pass
					PowerflowRestore();
				}
				//Default else -- "GridLAB-D" mode, just exit and go onward

				//IdxSW = counter;
				//Before exiting, free up some of our junk
				CHORDSETfree(&FCutSet);
				CHORDSETfree(&FCutSet_1);
				CHORDSETfree(&FCutSet_2);
				CHORDSETfree(&FCutSet_2_1);
				CHORDSETfree(&FCutSet_2_2);

				return counter;
			}

			// If feasible restoration scheme is not found
			// Save the mount of load need shedding for partial restoration
			candidateSwOpe_2.data_6[counter] = overLoad;
			candidateSwOpe_1.data_6[counter] = overLoad;
			candidateSwOpe.data_6[counter] = overLoad;

			candidateSwOpe_2.data_7[counter] = feederID;
			candidateSwOpe_1.data_7[counter] = feederID;
			candidateSwOpe.data_7[counter] = feederID;
	        
			// Form the graph after restoration
			top_res->copy(top_sim_2);
			top_res->deleteEdge(candidateSwOpe_2.data_1[counter],candidateSwOpe_2.data_2[counter]);
			top_res->addEdge(candidateSwOpe_2.data_3[counter],candidateSwOpe_2.data_4[counter]);
			top_res->BFS(s_ver_2);

			// Form the tie switch list for new graph
				//Allocate new_tie_swi -- just in case sizes change, this will realloc it (not quite as elegantly)
				CHORDSETalloc(&new_tie_swi,tie_swi_2.currSize);

			//Copy results in
			memcpy(new_tie_swi.data_1,tie_swi_2.data_1,tie_swi_2.currSize*sizeof(int));
			memcpy(new_tie_swi.data_2,tie_swi_2.data_2,tie_swi_2.currSize*sizeof(int));
			new_tie_swi.currSize = tie_swi_2.currSize;

			for (idx=0; idx<new_tie_swi.currSize; idx++)
			{
				if (((new_tie_swi.data_1[idx]==candidateSwOpe_2.data_3[counter]) && (new_tie_swi.data_2[idx]==candidateSwOpe_2.data_4[counter])) || ((new_tie_swi.data_1[idx]==candidateSwOpe_2.data_4[counter]) && (new_tie_swi.data_2[idx]==candidateSwOpe_2.data_3[counter])))
				{
					new_tie_swi.data_1[idx] = candidateSwOpe_2.data_1[counter];
					new_tie_swi.data_2[idx] = candidateSwOpe_2.data_2[counter];
				}
			}
			//
			for (idx=0; idx<sec_swi_2.currSize; idx++)
			{
				// Open the sectionalizing switch
				SW_to_Open_2.from_vert = sec_swi_2.data_1[idx];
				SW_to_Open_2.to_vert = sec_swi_2.data_2[idx];

				SW_to_Open.from_vert = sec_swi_map.data_1[idx];
				SW_to_Open.to_vert = sec_swi_map.data_2[idx];

				SW_to_Open_1.from_vert = sec_swi_map_1.data_1[idx];
				SW_to_Open_1.to_vert = sec_swi_map_1.data_2[idx];
	            
				// If the switch to open is not in the overloaded
				// feeder, move on to the next candidate switch.
				feeder_overloaded = candidateSwOpe_2.data_7[counter];
				swiInFeederBool = isSwiInFeeder(&SW_to_Open_2,feeder_overloaded);
				if ((feeder_overloaded != 01) && !swiInFeederBool)
				{
					continue;
				}
	            
				// Fundamental cut set of the sectionalizing switch
				sec_swi_2entry.from_vert = sec_swi_2.data_1[idx];
				sec_swi_2entry.to_vert = sec_swi_2.data_2[idx];
				top_sim_2->findFunCutSet(&tie_swi_2,&sec_swi_2entry,&FCutSet_2_1);
				top_res->findFunCutSet(&new_tie_swi,&sec_swi_2entry,&FCutSet_2_2);

				CHORDSETintersect(&FCutSet_2_1, &FCutSet_2_2, &FCutSet_2);

				for (k=0; k<FCutSet_2.currSize; k++)
				{
					//Pull the variables into BRANCHVERTICES
					FCutSet_2entry.from_vert = FCutSet_2.data_1[k];
					FCutSet_2entry.to_vert = FCutSet_2.data_2[k];

					mapTieSwi(&FCutSet_2entry,&FCutSetentry,&FCutSet_1entry);

					//Store them back
					FCutSet.data_1[k] = FCutSetentry.from_vert;
					FCutSet.data_2[k] = FCutSetentry.to_vert;
					FCutSet_1.data_1[k] = FCutSet_1entry.from_vert;
					FCutSet_1.data_2[k] = FCutSet_1entry.to_vert;
				}

				//Find offset
				//All theoretically the same index (from earlier)
				tvi=candidateSwOpe_2.currSize;

				// Save all candidate solutions -- these get appended to the previous ones (no overwritten)
				for (k=0; k<FCutSet_2.currSize; k++)
				{
					//General error check
					if ((tvi+k) >= candidateSwOpe.maxSize)
					{
						GL_THROW("Maximum size exceeded!");
						//Add later
					}

					candidateSwOpe_2.data_1[tvi+k]=SW_to_Open_2.from_vert;
					candidateSwOpe_2.data_2[tvi+k]=SW_to_Open_2.to_vert;
					candidateSwOpe_2.data_3[tvi+k]=FCutSet_2.data_1[k];
					candidateSwOpe_2.data_4[tvi+k]=FCutSet_2.data_2[k];
					candidateSwOpe_2.data_5[tvi+k]=counter;
					candidateSwOpe_2.data_6[tvi+k]=0.0;
					candidateSwOpe_2.data_7[tvi+k]=0;

					candidateSwOpe_1.data_1[tvi+k]=SW_to_Open_1.from_vert;
					candidateSwOpe_1.data_2[tvi+k]=SW_to_Open_1.to_vert;
					candidateSwOpe_1.data_3[tvi+k]=FCutSet_1.data_1[k];
					candidateSwOpe_1.data_4[tvi+k]=FCutSet_1.data_2[k];
					candidateSwOpe_1.data_5[tvi+k]=counter;
					candidateSwOpe_1.data_6[tvi+k]=0.0;
					candidateSwOpe_1.data_7[tvi+k]=0;

					candidateSwOpe.data_1[tvi+k]=SW_to_Open.from_vert;
					candidateSwOpe.data_2[tvi+k]=SW_to_Open.to_vert;
					candidateSwOpe.data_3[tvi+k]=FCutSet.data_1[k];
					candidateSwOpe.data_4[tvi+k]=FCutSet.data_2[k];
					candidateSwOpe.data_5[tvi+k]=counter;
					candidateSwOpe.data_6[tvi+k]=0.0;
					candidateSwOpe.data_7[tvi+k]=0;
				}

				//Update the size pointers
				tvi += FCutSet_2.currSize;
				candidateSwOpe_2.currSize = tvi;
				candidateSwOpe_1.currSize = tvi;
				candidateSwOpe.currSize = tvi;
			}
		}
		else
		{
			//Adjustment from WSU code below - just run a powerflow
			//If it fails, then modifyModel again (should de-toggle all of what was just toggled)
				//Perform the modification
				modifyModel(counter);

				// Run power flow
				powerflow_result = runPowerFlow();
				
				//See if it even worked -- if not, modifyModel again and set as a "false"
				if (powerflow_result == -1)
				{
					return -2;	//Serious error occurred, so flag us as "really bad"
					//basically, the state of the system may be corrupted, so any subsequent powerflows can't be trusted
				}
				else if (powerflow_result == 0)
				{
					//Call the modify function again, to undo what we just did
					modifyModel(counter);

					//Set us as invalid
					feasible=false;

					//Restore voltage for next pass
					PowerflowRestore();
				}
				else	//Success!?
				{
					//Check results
					checkPF2(&feasible, &overLoad, &feederID);

					//Check feasible again -- if not feasible, undo the operations again
					if (!feasible)
					{
						modifyModel(counter);	//Undo it by calling it again

						//Restore voltage for next pass
						PowerflowRestore();
					}
				}
	        
			// If feasible restoration scheme is found
			if (feasible)
			{
				//Check our desired "approach" on this -- see if we need to undo for the next section or not
				if (stop_and_generate)
				{
					//Undo it by calling it again
					modifyModel(counter);

					//Restore voltage for next pass
					PowerflowRestore();
				}
				//Default else -- "GridLAB-D" mode, just exit and go onward

				//IdxSW = counter;
				//Before exiting, free up some of our junk
				CHORDSETfree(&FCutSet);
				CHORDSETfree(&FCutSet_1);
				CHORDSETfree(&FCutSet_2);
				CHORDSETfree(&FCutSet_2_1);
				CHORDSETfree(&FCutSet_2_2);

				return counter;
			}

			// If feasible restoration scheme is not found
			candidateSwOpe_2.data_6[counter] = overLoad;
			candidateSwOpe_1.data_6[counter] = overLoad;
			candidateSwOpe.data_6[counter] = overLoad;

			candidateSwOpe_2.data_7[counter] = feederID;
			candidateSwOpe_1.data_7[counter] = feederID;
			candidateSwOpe.data_7[counter] = feederID;

			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			//%%% If counter > 300, we can assume full restoration
			//%%% is failed.
			//Adjust this to the real size, to stop things from breaking horribly
			if (counter >= (allocsize-1))
			{
				//If caught here, the stuff below should be correct, by default
				//resObj.candidateSwOpe = resObj.candidateSwOpe(1:300,:);
				//resObj.candidateSwOpe_1 = resObj.candidateSwOpe_1(1:300,:);
				//resObj.candidateSwOpe_2 = resObj.candidateSwOpe_2(1:300,:);

				//Before exiting, free up some of our junk
				CHORDSETfree(&FCutSet);
				CHORDSETfree(&FCutSet_1);
				CHORDSETfree(&FCutSet_2);
				CHORDSETfree(&FCutSet_2_1);
				CHORDSETfree(&FCutSet_2_2);

				//Send effectively, an error
				return -1;
			}
			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	        
			// If more than one feeder are overloaded OR
			// one or more microgrids are overloaded OR
			// move on to the next candidate switching operation
			if (feederID == -1)
			{
				counter++;
				continue;
			}

			// Form new graph
			top_res->copy(top_sim_2);
			top_res->deleteEdge(candidateSwOpe_2.data_1[counter],candidateSwOpe_2.data_2[counter]);
			top_res->addEdge(candidateSwOpe_2.data_3[counter],candidateSwOpe_2.data_4[counter]);

			preCounter = candidateSwOpe_2.data_5[counter];
			while (preCounter != 0)
			{
				top_res->deleteEdge(candidateSwOpe_2.data_1[preCounter],candidateSwOpe_2.data_2[preCounter]);
				top_res->addEdge(candidateSwOpe_2.data_3[preCounter],candidateSwOpe_2.data_4[preCounter]);
				preCounter=candidateSwOpe_2.data_5[preCounter];
			}
			top_res->BFS(s_ver_2);

			// Form the tie switch list for new graph
				//Allocate new_tie_swi -- just in case sizes change, this will realloc it (not quite as elegantly)
				CHORDSETalloc(&new_tie_swi,tie_swi_2.currSize);

			//Copy results in
			memcpy(new_tie_swi.data_1,tie_swi_2.data_1,tie_swi_2.currSize*sizeof(int));
			memcpy(new_tie_swi.data_2,tie_swi_2.data_2,tie_swi_2.currSize*sizeof(int));
			new_tie_swi.currSize = tie_swi_2.currSize;

			for (idx=0; idx<new_tie_swi.currSize; idx++)
			{
				if (((new_tie_swi.data_1[idx]==candidateSwOpe_2.data_3[counter]) && (new_tie_swi.data_2[idx]==candidateSwOpe_2.data_4[counter])) || ((new_tie_swi.data_1[idx]==candidateSwOpe_2.data_4[counter]) && (new_tie_swi.data_2[idx]==candidateSwOpe_2.data_3[counter])))
				{
					new_tie_swi.data_1[idx] = candidateSwOpe_2.data_1[counter];
					new_tie_swi.data_2[idx] = candidateSwOpe_2.data_2[counter];
				}
			}

			preCounter = candidateSwOpe_2.data_5[counter];
			while (preCounter != 0)
			{
				for (idx=0; idx<new_tie_swi.currSize; idx++)
				{
					if (((new_tie_swi.data_1[idx]==candidateSwOpe_2.data_3[preCounter]) && (new_tie_swi.data_2[idx]==candidateSwOpe_2.data_4[preCounter])) || ((new_tie_swi.data_1[idx]==candidateSwOpe_2.data_4[preCounter]) && (new_tie_swi.data_2[idx]==candidateSwOpe_2.data_3[preCounter])))
					{
						new_tie_swi.data_1[idx] = candidateSwOpe_2.data_1[preCounter];
						new_tie_swi.data_2[idx] = candidateSwOpe_2.data_2[preCounter];
					}
				}
				preCounter = candidateSwOpe_2.data_5[preCounter];
			}
			//
			for (idx=0; idx<sec_swi_2.currSize; idx++)
			{
				if (((sec_swi_2.data_1[idx]==candidateSwOpe_2.data_1[counter]) && (sec_swi_2.data_2[idx]==candidateSwOpe_2.data_2[counter])) || ((sec_swi_2.data_1[idx]==candidateSwOpe_2.data_2[counter]) && (sec_swi_2.data_2[idx]==candidateSwOpe_2.data_1[counter])))
				{
					startIdx = idx + 1;
					break;
				}
			}
			//
			for (idx=startIdx; idx<sec_swi_2.currSize; idx++)
			{
				// Open the sectionalizing switch
				SW_to_Open_2.from_vert = sec_swi_2.data_1[idx];
				SW_to_Open_2.to_vert = sec_swi_2.data_2[idx];
				SW_to_Open.from_vert = sec_swi_map.data_1[idx];
				SW_to_Open.to_vert = sec_swi_map.data_2[idx];
				SW_to_Open_1.from_vert = sec_swi_map_1.data_1[idx];
				SW_to_Open_1.to_vert = sec_swi_map_1.data_2[idx];

				// If the switch to open is not in the overloaded
				// feeder, move on to the next candidate switch.
				feeder_overloaded = candidateSwOpe_2.data_7[counter];

				swiInFeederBool = isSwiInFeeder(&SW_to_Open_2,feeder_overloaded);
				if ((feeder_overloaded != 0) && !swiInFeederBool)
				{
					continue;
				}

				// Fundamental cut set of the sectionalizing switch
				sec_swi_2entry.from_vert = sec_swi_2.data_1[idx];
				sec_swi_2entry.to_vert = sec_swi_2.data_2[idx];
				top_sim_2->findFunCutSet(&tie_swi_2,&sec_swi_2entry,&FCutSet_2_1);
				top_res->findFunCutSet(&new_tie_swi,&sec_swi_2entry,&FCutSet_2_2);

				CHORDSETintersect(&FCutSet_2_1, &FCutSet_2_2, &FCutSet_2);

				for (k=0; k<FCutSet_2.currSize; k++)
				{
					//Pull the variables into BRANCHVERTICES
					FCutSet_2entry.from_vert = FCutSet_2.data_1[k];
					FCutSet_2entry.to_vert = FCutSet_2.data_2[k];

					mapTieSwi(&FCutSet_2entry,&FCutSetentry,&FCutSet_1entry);

					//Store them back
					FCutSet.data_1[k] = FCutSetentry.from_vert;
					FCutSet.data_2[k] = FCutSetentry.to_vert;
					FCutSet_1.data_1[k] = FCutSet_1entry.from_vert;
					FCutSet_1.data_2[k] = FCutSet_1entry.to_vert;
				}

				//Find offset
				//All theoretically the same index (from earlier)
				tvi=candidateSwOpe_2.currSize;

				// Save all candidate solutions -- these get appended to the previous ones (no overwritten)
				for (k=0; k<FCutSet_2.currSize; k++)
				{
					//General error check
					if ((tvi+k) >= candidateSwOpe.maxSize)
					{
						GL_THROW("Maximum size exceeded!");
						//Add later
					}

					candidateSwOpe_2.data_1[tvi+k]=SW_to_Open_2.from_vert;
					candidateSwOpe_2.data_2[tvi+k]=SW_to_Open_2.to_vert;
					candidateSwOpe_2.data_3[tvi+k]=FCutSet_2.data_1[k];
					candidateSwOpe_2.data_4[tvi+k]=FCutSet_2.data_2[k];
					candidateSwOpe_2.data_5[tvi+k]=counter;
					candidateSwOpe_2.data_6[tvi+k]=0.0;
					candidateSwOpe_2.data_7[tvi+k]=0;

					candidateSwOpe_1.data_1[tvi+k]=SW_to_Open_1.from_vert;
					candidateSwOpe_1.data_2[tvi+k]=SW_to_Open_1.to_vert;
					candidateSwOpe_1.data_3[tvi+k]=FCutSet_1.data_1[k];
					candidateSwOpe_1.data_4[tvi+k]=FCutSet_1.data_2[k];
					candidateSwOpe_1.data_5[tvi+k]=counter;
					candidateSwOpe_1.data_6[tvi+k]=0.0;
					candidateSwOpe_1.data_7[tvi+k]=0;

					candidateSwOpe.data_1[tvi+k]=SW_to_Open.from_vert;
					candidateSwOpe.data_2[tvi+k]=SW_to_Open.to_vert;
					candidateSwOpe.data_3[tvi+k]=FCutSet.data_1[k];
					candidateSwOpe.data_4[tvi+k]=FCutSet.data_2[k];
					candidateSwOpe.data_5[tvi+k]=counter;
					candidateSwOpe.data_6[tvi+k]=0.0;
					candidateSwOpe.data_7[tvi+k]=0;
				}

				//Update the size pointers
				tvi += FCutSet_2.currSize;
				candidateSwOpe_2.currSize = tvi;
				candidateSwOpe_1.currSize = tvi;
				candidateSwOpe.currSize = tvi;
			}
		}
		counter = counter + 1;
	}

	//Before exiting, free up some of our junk
	CHORDSETfree(&FCutSet);
	CHORDSETfree(&FCutSet_1);
	CHORDSETfree(&FCutSet_2);
	CHORDSETfree(&FCutSet_2_1);
	CHORDSETfree(&FCutSet_2_2);

	return -1;
}

//Function to replicate MATLAB intersect capability
//for CHORDSET-based objects
//It is assumed all sets are preallocated
//MATLAB does this as a sort and is probably more efficient.  This goes "brute force route"
void restoration::CHORDSETintersect(CHORDSET *set_1, CHORDSET *set_2, CHORDSET *intersect)
{
	int indexa, indexb, idx;
	CHORDSET *leftside, *rightside;

	//Reset output CHORDSET
	for (idx=0; idx<intersect->maxSize; idx++)
	{
		intersect->data_1[idx] = -1;
		intersect->data_2[idx] = -1;
	}

	//Reset pointer
	intersect->currSize = 0;

	//Match against the smallest one
	if (set_1->currSize < set_2->currSize)
	{
		//Assign pointers
		leftside=set_1;
		rightside=set_2;
	}
	else
	{
		//Assign pointers
		leftside=set_2;
		rightside=set_1;
	}

	//Perform exhaustive search - not necessarily efficient, but meh
	for (indexa=0; indexa<leftside->currSize; indexa++)	//Main loop is on the smaller one, since it will finish faster
	{
		//Loop through the other side
		for (indexb=0; indexb<rightside->currSize; indexb++)
		{
			//See if they match
			if ((leftside->data_1[indexa] == rightside->data_1[indexb]) && (leftside->data_2[indexa] == rightside->data_2[indexb]))
			{
				//Store the value in the output
				intersect->data_1[intersect->currSize] = leftside->data_1[indexa];
				intersect->data_2[intersect->currSize] = leftside->data_2[indexa];

				//Increment pointer
				intersect->currSize++;

				//Break out of inner loop
				//Idea is rows are all unique, so once you match one, just go to next
				break;
			}
			//Default else, next set
		}
	}
}

//Modification function
//this was modifyGlmFile_3, but we're in GLD, so no sense modifying a GLM
void restoration::modifyModel(int counter)
{
	CHORDSET swi_to_open, swi_to_close;
	int preCounter, idx, k, newsizeval, idxstart;
	LOCSET loc_sec, loc_tie;
	INTVECT locations_temp, locations;
	FUNCTIONADDR switching_fxn;
	int return_val;
	double return_val_double;
	OBJECT *thisobj = OBJECTHDR(this);
	OBJECT *swobj;
	bool switch_occurred, return_is_int_val;

	//Initialize temporary variables, just in case
	swi_to_open.data_1 = nullptr;
	swi_to_open.data_2 = nullptr;
	swi_to_close.data_1 = nullptr;
	swi_to_close.data_2 = nullptr;
	locations_temp.data = nullptr;
	locations.data = nullptr;
	loc_sec.data_1 = nullptr;
	loc_sec.data_2 = nullptr;
	loc_sec.data_3 = nullptr;
	loc_sec.data_4 = nullptr;
	loc_tie.data_1 = nullptr;
	loc_tie.data_2 = nullptr;
	loc_tie.data_3 = nullptr;
	loc_tie.data_4 = nullptr;
	
	//Allocate size - base off of the size of the switching operations
	CHORDSETalloc(&swi_to_open,candidateSwOpe.currSize);
	CHORDSETalloc(&swi_to_close,candidateSwOpe.currSize);
	
	// Switching operations
	swi_to_open.data_1[swi_to_open.currSize] = candidateSwOpe.data_1[counter];
	swi_to_open.data_2[swi_to_open.currSize] = candidateSwOpe.data_2[counter];
	swi_to_open.currSize++;

	swi_to_close.data_1[swi_to_close.currSize] = candidateSwOpe.data_3[counter];
	swi_to_close.data_2[swi_to_close.currSize] = candidateSwOpe.data_4[counter];
	swi_to_close.currSize++;

	preCounter = candidateSwOpe.data_5[counter];

	while (preCounter != 0)
	{
		swi_to_open.data_1[swi_to_open.currSize] = candidateSwOpe.data_1[preCounter];
		swi_to_open.data_2[swi_to_open.currSize] = candidateSwOpe.data_2[preCounter];
		swi_to_open.currSize++;

		swi_to_close.data_1[swi_to_close.currSize] = candidateSwOpe.data_3[preCounter];
		swi_to_close.data_2[swi_to_close.currSize] = candidateSwOpe.data_4[preCounter];
		swi_to_close.currSize++;

		preCounter = candidateSwOpe.data_5[preCounter];
	}

	//Allocate loc_sec
	LOCSETalloc(&loc_sec,swi_to_open.currSize);

	for (idx=0; idx<swi_to_open.currSize; idx++)
	{
		for (k=0; k<sec_swi.currSize; k++)
		{
			if (((swi_to_open.data_1[idx]==sec_swi.data_1[k]) && (swi_to_open.data_2[idx]==sec_swi.data_2[k])) || ((swi_to_open.data_1[idx]==sec_swi.data_2[k]) && (swi_to_open.data_2[idx]==sec_swi.data_1[k])))
			{
				loc_sec.data_1[idx] = sec_swi_loc.data_1[k];
				loc_sec.data_2[idx] = sec_swi_loc.data_2[k];
				loc_sec.data_3[idx] = sec_swi_loc.data_3[k];
				loc_sec.data_4[idx] = sec_swi_loc.data_4[k];
			}
		}
	}

	//Assume max size, since it may be intermittent
	loc_sec.currSize = swi_to_open.currSize;

	//Allocate loc_tie
	LOCSETalloc(&loc_tie,swi_to_close.currSize);

	for (idx=0; idx<swi_to_close.currSize; idx++)
	{
		for (k=0; k<tie_swi.currSize; k++)
		{
			if (((swi_to_close.data_1[idx]==tie_swi.data_1[k]) && (swi_to_close.data_2[idx]==tie_swi.data_2[k])) || ((swi_to_close.data_1[idx]==tie_swi.data_2[k]) && (swi_to_close.data_2[idx]==tie_swi.data_1[k])))
			{
				loc_tie.data_1[idx] = tie_swi_loc.data_1[k];
				loc_tie.data_2[idx] = tie_swi_loc.data_2[k];
				loc_tie.data_3[idx] = tie_swi_loc.data_3[k];
				loc_tie.data_4[idx] = tie_swi_loc.data_4[k];
			}
		}
	}

	//Assume max size, since it may be intermittent
	loc_tie.currSize = swi_to_close.currSize;

	//locations = [loc_sec; loc_tie];
		//Allocate it
		newsizeval = (loc_sec.currSize + loc_tie.currSize);
		INTVECTalloc(&locations_temp,newsizeval);

		//Allocate the actual output
		INTVECTalloc(&locations,newsizeval);

		//Copy values into the input -- just copy the first column, since it is the index to what we care about
		memcpy(locations_temp.data,loc_sec.data_1,loc_sec.currSize*sizeof(int));
		memcpy(&locations_temp.data[loc_sec.currSize],loc_tie.data_1,loc_tie.currSize*sizeof(int));

		//Set size
		locations_temp.currSize = locations_temp.maxSize;

	//Find unique values
	unique_int(&locations_temp,&locations);

	//Free up the working vector, before I forget
	INTVECTfree(&locations_temp);

	if (locations.data[0] == -1)	//Check to see if any invalids snuck in here
	{
		idxstart = 1;
	}
	else
	{
		idxstart = 0;
	}

	//Start with no alterations
	switch_occurred = false;

	//Now loop through these locations - if they were closed, open them.  If open, close them.
	for (idx=idxstart; idx<locations.currSize; idx++)
	{
		//Null the function handler
		switching_fxn = nullptr;

		//Pull the object header, just cause
		swobj = NR_branchdata[locations.data[idx]].obj;

		if (NR_branchdata[locations.data[idx]].lnk_type == 4)	//Normal switch
		{
			//See if we can find the switching function
			switching_fxn = (FUNCTIONADDR)(gl_get_function(swobj,"change_switch_state"));

			//Set the return type
			return_is_int_val = true;
		}
		else if (NR_branchdata[locations.data[idx]].lnk_type == 6)	//Recloser
		{
			//See if we can find the switching function
			switching_fxn = (FUNCTIONADDR)(gl_get_function(swobj,"change_recloser_state"));

			//Set the return type
			return_is_int_val = false;
		}
		else if (NR_branchdata[locations.data[idx]].lnk_type == 5)	//Sectionalizer
		{
			//See if we can find the switching function
			switching_fxn = (FUNCTIONADDR)(gl_get_function(swobj,"change_sectionalizer_state"));

			//Set the return type
			return_is_int_val = false;
		}
		else	//How'd we get here!?
		{
			GL_THROW("Restoration: switch actions were attempted on a non-switch device!");
			/*  TROUBLESHOOT
			While attempting a reconfiguration action, an object that is not a switch, sectionalizer, or
			recloser was attempted to be switched.  This should not occur.  Please try again.  If the error
			persists, pleas submit your code and a bug report via the ticketing system.
			*/
		}

		//Check it
		if (switching_fxn == nullptr)
		{
			GL_THROW("Restoration: Unable to map switch change function");
			/*  TROUBLESHOOT
			While attempting to map the switch adjustment function for the restoration code, an error occurred.  Please
			try again.  If the error persists, please submit your code and a bug report via the ticketing system.
			*/
		}

		if (return_is_int_val)	//Switches, basically
		{
			//See what our status was, and do the opposite
			if (*NR_branchdata[locations.data[idx]].status == LS_OPEN)	//Was open, close it
			{
				return_val = ((int (*)(OBJECT *,unsigned char,bool))(*switching_fxn))(swobj,0x07,true);
			}
			else	//Must be closed, open it
			{
				return_val = ((int (*)(OBJECT *,unsigned char,bool))(*switching_fxn))(swobj,0x07,false);
			}
		}
		else	//Double return - reclosers and sectionalizers
		{
			//See what our status was, and do the opposite
			if (*NR_branchdata[locations.data[idx]].status == LS_OPEN)	//Was open, close it
			{
				return_val_double = ((double (*)(OBJECT *,unsigned char,bool))(*switching_fxn))(swobj,0x07,true);
			}
			else	//Must be closed, open it
			{
				return_val_double = ((double (*)(OBJECT *,unsigned char,bool))(*switching_fxn))(swobj,0x07,false);
			}

			//Check our values - general error checks
			if (return_val_double != 0.0)
			{
				return_val = 1;	//Something returned, so call us a success
			}
			else
			{
				return_val = 0;
			}
		}
		
		//Check it for errors, even though I think it always successed
		if (return_val != 1)
		{
			GL_THROW("Restoration: Unable to perform switching action on %s!",swobj->name?swobj->name:"unnamed");
			/*  TROUBLESHOOT
			While attempting to change the state on a switching device, an error occurred.  Please try again.  If the
			error persists, please submit your code and a bug report via the ticketing system.
			*/
		}
		else	//Occurred successfully
		{
			switch_occurred = true;
		}
	}//End adjustment location for loop

	//Call the support check alterations, if there was at least one switching action
	if (switch_occurred)
	{
		//Call "reliability_alterations" - "link" value of -99 or -77 matter?
		return_val = ((int (*)(OBJECT *, int, bool))(*fault_check_fxn))(fault_check_object,-99,true);

		//Check it
		if (return_val != 1)
		{
			GL_THROW("Restoration: problem occurred altering topology");
			/*  TROUBLESHOOT
			While attempting to modify the system topology based on a switching action, an error occurred.
			Please try again.  If the error persists, please submit your code and a bug report via the
			ticketing system.
			*/
		}
	}

	//Free up some stuff
	INTVECTfree(&locations);
	LOCSETfree(&loc_sec);
	LOCSETfree(&loc_tie);
}


//Execute a powerflow call
//basically call the NR solver routine and catch any errors
//This won't necessarily reflect load changes (since they may not
//have occurred in the update structure yet).
//
//Return codes - -1 = error, 0 = divergence/failure to converge, 1 = converged
int restoration::runPowerFlow(void)
{
	int64 PFresult;
	int overallresult;
	bool bad_computation;
	NRSOLVERMODE powerflow_type;
#ifdef GLD_USE_EIGEN
    static auto NR_Solver = std::make_unique<NR_Solver_Eigen>();
#endif
	
	//Start out assuming that we're not a bad computation
	bad_computation = false;

	//Depending on operation mode, call solver appropriately
	if (enable_subsecond_models)	//Deltamode is flagged on a module level, which means powerflow should really be going that route
	{
		powerflow_type = PF_DYNCALC;	//Built on an overarching assumption (to be implemented elsewhere) that reconfiguration NEVER occurs at the 0th timestep
	}//End deltamode
	else	//Normal mode
	{
		powerflow_type = PF_NORMAL;
	}

	//Put the powerflow call in a try/catch, since GL_THROWs may occur
	try {

		//Copied, more or less, from node.cpp call to solver_nr
		//Call the powerflow routine
#ifndef GLD_USE_EIGEN
		PFresult = solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, &NR_powerflow, powerflow_type, nullptr, &bad_computation);
#else
		PFresult = NR_Solver.solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, &NR_powerflow, powerflow_type, nullptr, &bad_computation);
#endif

		//De-flag the change - otherwise will cause un-neccesary reallocs
		NR_admit_change = false;

		if (bad_computation)
		{
			overallresult = 0;		//Flag as a failure to converge -- may be singular, NaN, etc, but is invalid
		}
		else if (PFresult<0)	//Failure to converge
		{
			overallresult = 0;		//Flag as a failure to converge (since that's what happened here)
		}
		else
		{
			overallresult = 1;		//"Succeeded" (solved a powerflow, at least)
		}
	}
	catch (...)	//Don't really care about the error - presumably it is associated with whatever bad powerflow we just did
	{
		overallresult = -1;		//Some type of "bad error" occurred - either a throw, or something worse
	}

	//Return out "status code"
	return overallresult;
}

//Function to check the results of the powerflow solution
void restoration::checkPF2(bool *flag, double *overLoad, int *feederID)
{
	bool vFlag, fFlag, mFlag, lFlag;
	double overLoad1, overLoad2, overLoad3;
	int oFeederID, microgridID, lineID;

	vFlag = checkVoltage();
	checkFeederPower(&fFlag, &overLoad1, &oFeederID);
	checkMG(&mFlag, &overLoad2, &microgridID);
	checkLinePower(&lFlag,&overLoad3,&lineID);

	if (vFlag && fFlag && mFlag && lFlag)
	{
		*flag = true;
	}
	else
	{
		*flag = false;
	}

	if (!vFlag)
	{
		*overLoad = INFINITY;
	}
	else
	{
		*overLoad = overLoad1 + overLoad2 + overLoad3;
	}

	if (((oFeederID != 0) && (microgridID != 0)) || (oFeederID == -1) || (microgridID == -1))
	{
		*feederID = -1;
	}
	else if (microgridID != 0)
	{
		*feederID = microgridID + 100;
	}
	else	//Not in MATLAB, just set to value of oFeederID
	{
		*feederID = oFeederID;
	}
}

//Check voltage routine
bool restoration::checkVoltage(void)
{
	bool flag;
	unsigned int index;
	double perunitvalue;

	//Start assuming we're all hokey-dorey
	flag = true;

	//See if any are outside the limits
	for (index=0; index<NR_bus_count; index++)
	{
		//Make sure we aren't triplex first, since that breaks things
		if ((NR_busdata[index].phases & 0x80) == 0x80)	//Are triples
		{
			//Check each leg individually against nominal -- assumes nominal is 120 V (or suitable equivalent based on 1N/2N, not 12)
			perunitvalue = (NR_busdata[index].V[0].Mag())/NR_busdata[index].volt_base;	//1N

			//Check and see if it violates or not
			if ((perunitvalue < voltage_limit[0]) || (perunitvalue > voltage_limit[1]))	//1N check
			{
				//Outside of range - set flag and break us
				flag = false;
				break;
			}

			perunitvalue = (NR_busdata[index].V[1].Mag())/NR_busdata[index].volt_base;	//2N

			//Check and see if it violates or not
			if ((perunitvalue < voltage_limit[0]) || (perunitvalue > voltage_limit[1]))	//2N check
			{
				//Outside of range - set flag and break us
				flag = false;
				break;
			}
		}
		else	//"Normal"
		{
			//Check against individual phases -- check as a bit mask so only "in-service" are caught
			if ((NR_busdata[index].phases & 0x04) == 0x04)	//Has phase A
			{
				//Calculate the perunit value for A
				perunitvalue = (NR_busdata[index].V[0].Mag())/NR_busdata[index].volt_base;

				//Check and see if it violates or not
				if ((perunitvalue < voltage_limit[0]) || (perunitvalue > voltage_limit[1]))
				{
					//Outside of range - set flag and break us
					flag = false;
					break;
				}
			}//End phase A

			//Check against individual phases -- check as a bit mask so only "in-service" are caught
			if ((NR_busdata[index].phases & 0x02) == 0x02)	//Has phase B
			{
				//Calculate the perunit value for A
				perunitvalue = (NR_busdata[index].V[1].Mag())/NR_busdata[index].volt_base;

				//Check and see if it violates or not
				if ((perunitvalue < voltage_limit[0]) || (perunitvalue > voltage_limit[1]))
				{
					//Outside of range - set flag and break us
					flag = false;
					break;
				}
			}//End phase B

			//Check against individual phases -- check as a bit mask so only "in-service" are caught
			if ((NR_busdata[index].phases & 0x01) == 0x01)	//Has phase C
			{
				//Calculate the perunit value for A
				perunitvalue = (NR_busdata[index].V[2].Mag())/NR_busdata[index].volt_base;

				//Check and see if it violates or not
				if ((perunitvalue < voltage_limit[0]) || (perunitvalue > voltage_limit[1]))
				{
					//Outside of range - set flag and break us
					flag = false;
					break;
				}
			}//End phase C

		}//End "normal"
	}//End For loop

	//Send back the value
	return flag;
}

//Check feeder power levels
void restoration::checkFeederPower(bool *fFlag, double *overLoad, int *feederID)
{
	int indexval, ret_value;
	double feeder_power_Mag;

	//Assume we're okay, by default
	*fFlag = true;

	//Set other default
	*overLoad = 0.0;
	*feederID = 0;

	//Loop through the "known feeders" and see how their values compare
	for (indexval=0; indexval<numfVer; indexval++)
	{
		//Call the power computation function first
		ret_value = ((int (*)(OBJECT *))(*fLinkPowerFunc[indexval]))(fLinkObjList[indexval]);

		//Check the value -- make sure it worked
		if (ret_value != 1)
		{
			GL_THROW("Restoration: failure to calculate power from link:%s", fLinkObjList[indexval]->name ? fLinkObjList[indexval]->name : "Unnamed");
			/*  TROUBLESHOOT
			While trying to calculate the power on a link in the system, an error occurred.  Please try again.  If the error
			persists, please submit your code and a bug report via the ticketing system.
			*/
		}

		//Now pull the value
		feeder_power_Mag = feeder_power_link_value[indexval]->Mag();

		//Check against the limits
		if (feeder_power_Mag > feeder_power_limit[indexval])
		{
			if (*feederID == 0)
			{
				*feederID = indexval + 1;
				*overLoad = feeder_power_Mag - feeder_power_limit[indexval];
			}
			else
			{
				*feederID = -1;
				*overLoad += (feeder_power_Mag - feeder_power_limit[indexval]);
			}

			//set the flag - overloaded
			*fFlag = false;
		}
	}//End feeder traversion
}

//Check microgrid power levels
void restoration::checkMG(bool *mFlag, double *overLoad, int *microgridID)
{
	int indexval, ret_value;
	gld::complex microgrid_power_over;

	//Assume we're okay, by default
	*mFlag = true;

	//Set other default
	*overLoad = 0.0;
	*microgridID = 0;

	//Loop through the "known microgrids" and see how their values compare
	for (indexval=0; indexval<numMG; indexval++)
	{
		//Call the power computation function first
		ret_value = ((int (*)(OBJECT *))(*mLinkPowerFunc[indexval]))(mLinkObjList[indexval]);

		//Check the value -- make sure it worked
		if (ret_value != 1)
		{
			GL_THROW("Restoration: failure to calculate power from link:%s", mLinkObjList[indexval]->name ? mLinkObjList[indexval]->name : "Unnamed");
			//Defined above
		}

		//Check limits - do explicitly
		if ((microgrid_power_link_value[indexval]->Re() > microgrid_limit[indexval].Re()) || (microgrid_power_link_value[indexval]->Im() > microgrid_limit[indexval].Im()))
		{
			*mFlag = false;
			// Record the ID of overload Microgrid
			if (*microgridID == 0)
			{
				*microgridID = indexval + 1;
			}
			else
			{
				*microgridID = -1;
			}

			// Calculate the amount of overload
			microgrid_power_over = *microgrid_power_link_value[indexval] - microgrid_limit[indexval];

			//Accumulate it
			*overLoad += microgrid_power_over.Mag();
		}//End overload
	}//End microgrid traversion
}

//Check line loading levels
void restoration::checkLinePower(bool *lFlag, double *overLoad, int *lineID)
{
	int ret_value;
	unsigned int indexval;
	bool lineFlag;
	double lineOverload;

	//Set initial variable
	*lFlag = true;
	*lineID = 0;
	*overLoad = 0.0;

	//Check to see if line limits are being checked -- if they aren't, just skip this
	if (use_link_limits)
	{
		//Call the function linked in NR_branchdata for each line to check itself
		for (indexval=0; indexval<NR_branch_count; indexval++)
		{
			//Call the function
			ret_value = ((int (*)(OBJECT *, double *, bool *))(*NR_branchdata[indexval].limit_check))(NR_branchdata[indexval].obj,&lineOverload,&lineFlag);

			//Do a general success check
			if (ret_value != 1)
			{
				GL_THROW("Restoration: failure to calculate limits from link:%s", NR_branchdata[indexval].name ? NR_branchdata[indexval].name : "Unnamed");
				/*  TROUBLESHOOT
				While trying to calculate the amount over a limit on a link in the system, an error occurred.  Please try again.  If the error
				persists, please submit your code and a bug report via the ticketing system.
				*/
			}

			//See if it is overloaded
			if (lineFlag)
			{
				//Set the flag
				*lFlag = false;

				if (*lineID == 0)
				{
					*lineID = indexval + 1;
					*overLoad = lineOverload;
				}
				else
				{
					*lineID = -1;
					*overLoad += lineOverload;
				}
			}//End line overloaded
			//Default else -- not overloaded, go to the next one
		}
	}
	//Default else -- no line limits being checked, so just skip
}

//Print the results to a text file -- similar to the MATLAB output (adjusted slightly)
void restoration::printResult(int IdxSW)
{
	FILE *FPOutput;
	TIMESTAMP tval;
	int delta_microseconds_value;
	double delta_tval, loadShedding, seconds_dbl_value;
	DATETIME res_event_time;
	int retvalue, indexvalue;

	//Get current timestamp - pull the deltamode clock, just cause
	tval = gl_globalclock;
	delta_tval = gl_globaldeltaclock;

	//Convert to a printable time
	delta_microseconds_value = (int)((delta_tval - (int)(delta_tval))*1000000+0.5);	// microseconds roll-over - biased upward (by 0.5)
	retvalue = gl_localtime(tval,&res_event_time);	//Could check to make sure this worked, but meh

	//Create the seconds counter
	seconds_dbl_value = (double)(res_event_time.second) + (double)(delta_microseconds_value)/1000000.0;

	//Open the file handle
	FPOutput = fopen(logfile_name,"at");

	// Case infomation
	//Simple print mechanism
	if (res_event_time.second < 10)	//10 or below, add a zero for giggles
	{
		fprintf(FPOutput,"-- Restoration session of %4d-%02d-%02d %02d:%02d:0%2.6f --\n\n",res_event_time.year,res_event_time.month,res_event_time.day,res_event_time.hour,res_event_time.minute,seconds_dbl_value);
	}
	else	//10 or above, 2 digit print works
	{
		fprintf(FPOutput,"-- Restoration session of %4d-%02d-%02d %02d:%02d:%2.6f --\n\n",res_event_time.year,res_event_time.month,res_event_time.day,res_event_time.hour,res_event_time.minute,seconds_dbl_value);
	}
	fprintf(FPOutput, "Fault section: %d - %d (in original topology), %d - %d (in simplified topology)\n", f_sec.from_vert, f_sec.to_vert, f_sec_1.from_vert, f_sec_1.to_vert);
	fprintf(FPOutput, "Fault section: %s - %s (in original topology)\n", NR_busdata[f_sec.from_vert].name, NR_busdata[f_sec.to_vert].name);
	if (faultSecObj != nullptr)
	{
		fprintf(FPOutput, "Fault section: %s\n",faultSecObj->name ? faultSecObj->name : "Unnamed");
	}
	else
	{
		fprintf(FPOutput, "Fault section: Not set, assumed SWING -- %s\n",NR_busdata[0].name ? NR_busdata[0].name : "Unnamed");
	}

	// Restoration results
	if (IdxSW >= 0) // Full restoration is successful	-- was "1" in MATLAB, but that is a counter.  -1 values coded as bad for GLD
	{
		fprintf(FPOutput, "Full restoration is successful.\n");
		fprintf(FPOutput, "The optimal switching sequence is as follows \n");
		printSOs(FPOutput,IdxSW);
	}
	else if (candidateSwOpe.data_6 != nullptr) // Partial restoration                
	{
		//Find the minimum value - start with first value
		IdxSW = 0;
		loadShedding = candidateSwOpe.data_6[0];

		//Loop through the rest
		for (indexvalue=1; indexvalue<candidateSwOpe.currSize; indexvalue++)
		{
			if (candidateSwOpe.data_6[indexvalue] < loadShedding)
			{
				loadShedding = candidateSwOpe.data_6[indexvalue];
				IdxSW = indexvalue;
			}
		}

		fprintf(FPOutput, "Full restoration failed. Partial restoration performed.\n");
		fprintf(FPOutput, "%f kVA load should be shed.\n", (loadShedding/1000.0));
		fprintf(FPOutput, "The optimal switching sequence is as follows. \n");
		printSOs(FPOutput,IdxSW);
	}
	else
	{
		// Fail to restore all outage load
		fprintf(FPOutput, "Outage load cannot be restored!!!\n");
	}

	//Extra line breaks, for good measure
	fprintf(FPOutput,"\n\n");

	//Close the output file
	fclose(FPOutput);
}

//Print swith operations, recursively
void restoration::printSOs(FILE *FPHandle, int IdxSW)
{
	//Overall size check
	if (IdxSW<candidateSwOpe.currSize)
	{
		if (candidateSwOpe.data_5[IdxSW] != 0)
		{
			//Recursive call to ourselves
			printSOs(FPHandle,candidateSwOpe.data_5[IdxSW]);
		}
		printCIO(FPHandle,IdxSW);
	}
}

//Print cyclic interchange operations
void restoration::printCIO(FILE *FPHandle, int IdxSW)
{
	int indexval, idx_microgrid;

	fprintf(FPHandle, "Open: %d - %d (in original topology), %d - %d (in simplified topology)\n",candidateSwOpe.data_1[IdxSW],candidateSwOpe.data_2[IdxSW],candidateSwOpe_1.data_1[IdxSW],candidateSwOpe_1.data_2[IdxSW]);
	fprintf(FPHandle, "Open: %s - %s (in original topology)\n",NR_busdata[candidateSwOpe.data_1[IdxSW]].name,NR_busdata[candidateSwOpe.data_2[IdxSW]].name);
	
	if ((candidateSwOpe.data_3[IdxSW] != s_ver) && (candidateSwOpe.data_4[IdxSW] != s_ver))
	{
		fprintf(FPHandle, "Close: %d - %d (in original topology), %d - %d (in simplified topology)\n",candidateSwOpe.data_3[IdxSW],candidateSwOpe.data_4[IdxSW],candidateSwOpe_1.data_3[IdxSW],candidateSwOpe_1.data_4[IdxSW]);
		fprintf(FPHandle, "Close: %s - %s (in original topology)\n",NR_busdata[candidateSwOpe.data_3[IdxSW]].name,NR_busdata[candidateSwOpe.data_4[IdxSW]].name);
	}
	else
	{
		if (candidateSwOpe.data_3[IdxSW] == s_ver)
		{
			idx_microgrid = -1;	//Set to a default value

			//Loop and see if we can find a match
			for (indexval=0; indexval<MGIdx.currSize; indexval++)
			{
				if (MGIdx.data[indexval] == candidateSwOpe.data_4[IdxSW])
				{
					idx_microgrid = indexval;

					//Assume we only want one, based on MATLAB code
					break;
				}
			}

			//Only print if it worked
			if (idx_microgrid != -1)
			{
				fprintf(FPHandle, "Close: %d - Microgrid%d (in original topology), %d - Microgrid%d (in simplified topology)\n",candidateSwOpe.data_4[IdxSW],idx_microgrid, candidateSwOpe_1.data_4[IdxSW], idx_microgrid);
				fprintf(FPHandle, "Close: %s - Microgrid%d (in original topology)\n",NR_busdata[candidateSwOpe.data_4[IdxSW]].name,idx_microgrid);
			}
			//Default else, just don't do anything (not sure it ever gets here)
		}
	}
}

//Check an edge (i,j) in original graph is a switch or not
//If yes, flag = 1, else flag = 0
bool restoration::isSwitch(int iIndex, int jIndex)
{
	bool flag;
	int idx;

	flag = false;

	for (idx=0; idx<sec_swi.currSize; idx++)
	{
		if (((iIndex==sec_swi.data_1[idx]) && (jIndex==sec_swi.data_2[idx])) || ((jIndex==sec_swi.data_1[idx]) && (iIndex==sec_swi.data_2[idx])))
		{
            flag = true;
            break;
		}
	}

	return flag;
}

// Check if a sectionalizing switch is in a given feeder,
// Assume BFS is already performed for top_res
bool restoration::isSwiInFeeder(BRANCHVERTICES *swi_2, int feederID_overload)
{
	bool flag;
	int curNode, feederID_current, microgridID_current;
	int idxval, tempval, count_check;

	curNode = swi_2->from_vert;      
    if (feederID_overload < 100) // If a feeder overloaded
	{
		feederID_current = -1;

		//Loop through
		for (idxval=0; idxval<feederVertices_2.currSize; idxval++)
		{
			if (feederVertices_2.data[idxval] == curNode)
			{
				feederID_current = idxval;
				break;
			}
		}

		//"Stuck" detector -- not sure why it does sometimes, but this is a kludgy way to prevent it
		count_check = NR_bus_count;	//Arbitrary assignment

        while ((feederID_current != -1) && (count_check > 0))
		{
			tempval = -1;
			for (idxval=0;idxval<MGIdx_2.currSize; idxval++)
			{
				if (MGIdx_2.data[idxval]==curNode)
				{
					tempval = idxval;
				}
			}

			if ((tempval != -1) && (top_res->parent_value[curNode] == s_ver_2))
			{
                flag = false;
				return flag;
			}

			curNode = top_res->parent_value[curNode];
			feederID_current = -1;

			//Loop through
			for (idxval=0; idxval<feederVertices_2.currSize; idxval++)
			{
				if (feederVertices_2.data[idxval] == curNode)
				{
					feederID_current = idxval;
					break;
				}
			}

			//Stuck counter decrement
			count_check--;
		}
        if ((feederID_current+1) == feederID_overload)
		{
            flag = true;
		}
        else
		{
            flag = false;
		}
	}
    else // If a microgrid overloaded
	{
		microgridID_current = -1;

		//Loop through
		for (idxval=0; idxval<MGIdx_2.currSize; idxval++)
		{
			if (MGIdx_2.data[idxval] == curNode)
			{
				microgridID_current = idxval;
				break;
			}
		}

		//"Stuck" detector -- not sure why it does sometimes, but this is a kludgy way to prevent it
		count_check = NR_bus_count;	//Arbitrary assignment

        while ((microgridID_current == -1) && (count_check > 0))
		{
			curNode = top_res->parent_value[curNode];

            if (curNode == -1)
			{
                flag = false;
                return flag;
			}

			//Do search again
			microgridID_current = -1;

			//Loop through
			for (idxval=0; idxval<MGIdx_2.currSize; idxval++)
			{
				if (MGIdx_2.data[idxval] == curNode)
				{
					microgridID_current = idxval;
					break;
				}
			}

			//Stuck counter decrement
			count_check--;
		}
        if ((microgridID_current+1) == (feederID_overload - 100))
		{
            flag = true;
		}
        else
		{
            flag = false;
		}
	}

	return flag;
}


//Powerflow saving function
//Saves voltages at pre-restoration point
//Basically to avoid if the restoration creates an INF/NAN
//and corrupts all answers
void restoration::PowerflowSave(void)
{
	unsigned int indexval;

	//See if we're already allocated - we shouldn't be, but check
	if (voltage_storage != nullptr)
	{
		//Loop and free our components first
		for (indexval=0; indexval<NR_bus_count; indexval++)
		{
			gl_free(voltage_storage[indexval]);
		}

		//Free us up too
		gl_free(voltage_storage);
	}

	//Allocate us
	voltage_storage = (gld::complex **)gl_malloc(NR_bus_count*sizeof(gld::complex *));

	//Make sure it worked
	if (voltage_storage == nullptr)
	{
		GL_THROW("Restoration: failed to allocate memory for base voltage values!");
		/*  TROUBLESHOOT
		While attempting to allocate memory for storing the fallback voltage values,
		an error occurred.  Please try again, assuming proper inputs.  If the error persists,
		please submit your code and a bug report via the ticketing system.
		*/
	}

	//Now loop for allocate/copy
	for (indexval=0; indexval<NR_bus_count; indexval++)
	{
		//Allocate this entry
		voltage_storage[indexval] = (gld::complex *)gl_malloc(3*sizeof(gld::complex));

		//Check it
		if (voltage_storage[indexval] == nullptr)
		{
			GL_THROW("Restoration: failed to allocate memory for base voltage values!");
			//Defined above
		}

		//Copy the voltage values
		voltage_storage[indexval][0] = NR_busdata[indexval].V[0];
		voltage_storage[indexval][1] = NR_busdata[indexval].V[1];
		voltage_storage[indexval][2] = NR_busdata[indexval].V[2];
	}
}

//Converse to saving function -- puts all the saved voltages back
void restoration::PowerflowRestore(void)
{
	unsigned int indexval;

	//Loop through and put the values back
	for (indexval=0; indexval<NR_bus_count; indexval++)
	{
		NR_busdata[indexval].V[0] = voltage_storage[indexval][0];
		NR_busdata[indexval].V[1] = voltage_storage[indexval][1];
		NR_busdata[indexval].V[2] = voltage_storage[indexval][2];
	}
}

//****************************************/
//****     Chain class functions      ****/
//****************************************/
//delete all nodes of the linked list
void Chain::delAllNodes(void)
{
	CHAINNODE *next;

	while (first != nullptr)
	{
		next = first->link;

		//Remove it
		gl_free(first);

		first = next;
	}
	first = nullptr;
	last = nullptr;
}

//if the linked list is empty, return 1.  Else return 0
bool Chain::isempty(void)
{
	bool flag;

	if (first == nullptr)
	{
		flag = true;
	}
	else
	{
		flag = false;
	}

	return flag;
}

//return the length of the linked list
int Chain::getLength(void)
{
	CHAINNODE *currentNode;
	int length;
	
	length = 0;
	currentNode = first;

	while (currentNode != nullptr)
	{
		length = length + 1;
		currentNode = currentNode->link;
	}

	return length;
}

//search the linked list for given data.  If found, return index of 1st node.  If not found, return 0
int Chain::search(int sData)
{
	int index;
	CHAINNODE *currentNode;

	index = 0;
	currentNode = first;

	while (currentNode != nullptr)
	{
		if (currentNode->data != sData)
		{
			currentNode = currentNode->link;
			index = index + 1;
		}
		else
		{
			break;
		}
	}

	if (currentNode == nullptr)
	{
		index = -1;
	}
	
	return index;
}

//modifies node data.  Only 1st found data is modified.  If oldData is found, modify it and return 1
//if oldData is not found, do nothing and return 0
bool Chain::modify(int oldData, int newData)
{
	CHAINNODE *currentNode;
	bool flag;

	flag = false;
	currentNode = first;

	while (currentNode != nullptr)
	{
		if (currentNode->data != oldData)
		{
			currentNode = currentNode->link;
		}
		else
		{
			break;
		}
	}

	//if oldData is found:
	if (currentNode != nullptr)
	{
		if (currentNode->data == oldData)
		{
			currentNode->data = newData;
			flag = true;
		}
		//Default else -- leave it false
	}

	//if oldData is not found:
	if (currentNode == nullptr)
	{
		flag = false;
	}

	return flag;
}

//adds a new node after the kth node in the linked list.  If k is out of bounds, do nothing.  
//if k = 0, add the new node as the first node
void Chain::addNode(int kIndex, int newData)
{
	CHAINNODE *currentNode, *newNode;
	int currentIndex;

	//check bounds
	if (kIndex < 0)
	{
		GL_THROW("The index is out of bounds.");
	}

	//find kth node
	currentIndex = 0;	//Adjusted from 1
	currentNode = first;
	while ((currentIndex < kIndex) && (currentNode != nullptr))
	{
		currentIndex = currentIndex + 1;
		currentNode = currentNode->link;
	}

	//out of bounds
	if ((kIndex > 0) && (currentNode==nullptr))
	{
		GL_THROW("The index is out of bounds.");
	}

	//insert new node
	//Malloc it
	newNode = (CHAINNODE *)gl_malloc(sizeof(CHAINNODE));

	//See if it worked
	if (newNode == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new node in chain");
		/*  TROUBLESHOOT
		While attempting to allocate a new node in one of the restoration program's graph theory chains,
		an error was encountered.  Please try again.  If the error persists, please submit your code and a
		bug report via the ticket system.
		*/
	}

	//Set the new data
	newNode->data = newData;

	if (kIndex > 0)
	{
		newNode->link = currentNode->link;
		currentNode->link = newNode;
	} 
	else
	{
		newNode->link = first;
		first = newNode;
	}
	
	if (newNode->link == nullptr)
	{
		last = newNode;
	}
}

//Deletes the first found node found with data = dData
//does nothing if dData is not found
void Chain::deleteNode(int dData)
{
	CHAINNODE *currentNode, *trail;

	currentNode = first;
	trail = nullptr;					//the node before currentNode
	
	while (currentNode != nullptr)
	{
		if (currentNode->data != dData)
		{
			trail = currentNode;
			currentNode = currentNode->link;
		}
		else
		{
			break;
		}
	}

	if (currentNode == nullptr)
	{
		GL_THROW("The data cannot be found.");
	}

	//if node with dData is found, delete that node
	if (trail != nullptr)
	{
		trail->link = currentNode->link;
	}
	else
	{
		first = currentNode->link;
	}

	if (currentNode->link == nullptr)
	{
		last = trail;
	}

	//remove the current node
	gl_free(currentNode);
}

//make a copy of llist
void Chain::copy(Chain *ilist)
{
	CHAINNODE *currNode;

	//delete all nodes in llist2
	if (first != nullptr)
	{
		delAllNodes();
	}

	//copy nodes
	if (ilist != nullptr)
	{
		currNode = ilist->first;
		while (currNode != nullptr)
		{
			append(currNode->data);
			currNode = currNode->link;
		}
	}
	//Default else, it was empty, just ignore it?
}

//add a new node at the end of the linked list llist
void Chain::append(int newData)
{
	CHAINNODE *newNode;
	
	//Create a new node
	newNode = (CHAINNODE *)gl_malloc(sizeof(CHAINNODE));

	//Make sure it worked
	if (newNode == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new node in chain");
		//Defined above
	}

	newNode->data = newData;

	//Set initial link
	newNode->link = nullptr;

	if (first != nullptr)
	{
		last->link = newNode;
		last = newNode;
	}
	else {
		first = newNode;
		last = newNode;
	}
}

//****************************************/
//****  LinkedQueue class functions   ****/
//****************************************/
// Add a new node at the end of the queue
void LinkedQueue::enQueue(int newData)
{
	CHAINNODE *newNode;

	//Create one
	newNode = (CHAINNODE *)gl_malloc(sizeof(CHAINNODE));

	//Make sure it worked
	if (newNode == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new node in chain");
		//Defined above
	}

	newNode->link = nullptr;
	newNode->data = newData;
	
	if (first != nullptr)
	{
		last->link = newNode;
		last = newNode;
	}
    else
	{
		first = newNode;
		last = newNode;
	}
}

// Delete the first node of the queue and return its data
int LinkedQueue::deQueue(void)
{
	int fData;
	CHAINNODE *tempNode;

	if (isempty())
	{
		GL_THROW("Restoration: The queue is empty!");
	}

	fData = first->data;
	tempNode = first;

	first = first->link;

	//Free it
	gl_free(tempNode);

	return fData;
}

//****************************************/
//**** ChainIterator class functions  ****/
//****************************************/
//initializes the iterator, takes as argument lList of class Chain
int ChainIterator::initialize(Chain *lList)			
{
	int nData;

	location = lList->first;

	if (location != nullptr)
	{
		nData = location->data;
	}
	else
	{
		nData = -1;
	}
	
	return nData;
}


//move pointer to next node
int ChainIterator::next(void)
{
	int nData;

	if (location == nullptr)
	{
		return -1;
	}

	location = location->link;
	
	if (location != nullptr)
	{
		nData = location->data;
	}
	else
	{
		nData = -1;
	}

	return nData;
}

//****************************************/
//**** LinkedBase class functions  ****/
//****************************************/
//Constructor
LinkedBase::LinkedBase(int nVer)
{
	int indexvar;

	//Common zeros
	iterPos = nullptr;
	source = 0;
	dfs_time = 0;
	numEdges = 0;

	if (nVer == 0)
	{
		numVertices = 0;
		numEdges = 0;
		adjList = nullptr;
		status_value = nullptr;
		parent_value = nullptr;
		dist = nullptr;
		dTime = nullptr;
		fTime = nullptr;
	}
	else
	{
		numVertices = nVer;

		//Allocate the list
		adjList = (Chain **)gl_malloc(numVertices*sizeof(Chain *));

		//See if it worked
		if (adjList == nullptr)
		{
			GL_THROW("Restoration:Failed to allocate new chain");
			/*  TROUBLESHOOT
			While attempting to allocate a new chain in one of the restoration program's graph theory chains,
			an error was encountered.  Please try again.  If the error persists, please submit your code and a
			bug report via the ticket system.
			*/
		}

		//NULL out the chain iterator - gets populated elsewhere, theoretically
		iterPos = nullptr;

		//Create the default vectors - initialize at the end (all same size)
		status_value = (int *)gl_malloc(numVertices*sizeof(int));

		//Check it
		if (status_value == nullptr)
		{
			GL_THROW("Restoration:Failed to allocate new chain");
			//Defined above
		}

		parent_value = (int *)gl_malloc(numVertices*sizeof(int));

		if (parent_value == nullptr)
		{
			GL_THROW("Restoration:Failed to allocate new chain");
			//Defined above
		}

		dist = (double *)gl_malloc(numVertices*sizeof(double));

		if (dist == nullptr)
		{
			GL_THROW("Restoration:Failed to allocate new chain");
			//Defined above
		}

		dTime = (TIMESTAMP *)gl_malloc(numVertices*sizeof(TIMESTAMP));

		if (dTime == nullptr)
		{
			GL_THROW("Restoration:Failed to allocate new chain");
			//Defined above
		}

		fTime = (TIMESTAMP *)gl_malloc(numVertices*sizeof(TIMESTAMP));

		if (fTime == nullptr)
		{
			GL_THROW("Restoration:Failed to allocate new chain");
			//Defined above
		}

		//Loop through and do the initializations
		for (indexvar=0; indexvar<numVertices; indexvar++)
		{
			//Begin with constructor for Chains
			//Call the constructor for everything (since doing explicit memory management)
			//Allocate each new object
			adjList[indexvar] = (Chain *)gl_malloc(sizeof(Chain));

			//Make sure it worked
			if (adjList[indexvar] == nullptr)
			{
				GL_THROW("Restoration:Failed to allocate new chain");
				//Defined above
			}

			//Call the construcotr
			new (adjList[indexvar]) Chain();

			//Now do the other items
			status_value[indexvar] = 0;
			parent_value[indexvar] = -1;
			dist[indexvar] = INFINITY;
			dTime[indexvar] = 0;
			fTime[indexvar] = 0;
		}//End init loop
	}//End non-zero constructor
}

//Delete all elements in the adjacency list
void LinkedBase::delAllVer(void)
{
	int indexvar;

	if (numVertices != 0)
	{
		//Loop through and remove chain items
		for (indexvar = 0; indexvar < numVertices; indexvar++)
		{
			gl_free(adjList[indexvar]);

			//NULL it
			adjList[indexvar] = nullptr;
		}

		//Now de-allocate everything else
		gl_free(adjList);	//Main pointer
		//Null it too
		adjList = nullptr;

		numVertices = 0;
		numEdges = 0;
		dfs_time = 0;
		
		//Call iterposes explicit delte
		deactivatePos();

		source = 0;
		gl_free(status_value);
		gl_free(parent_value);
		gl_free(dist);
		gl_free(dTime);
		gl_free(fTime);

		//NULL em all
		status_value = nullptr;
		parent_value = nullptr;
		dist = nullptr;
		dTime = nullptr;
		fTime = nullptr;
	}
}

//Return the number of vertices
int LinkedBase::getVerNum(void)
{
	return numVertices;
}

//Return the number of edges
int LinkedBase::getEdgNum(void)
{
	return numEdges;
}

//Return the out degree of the given vertex
int LinkedBase::getOutDeg(int index)
{
	if ((index <0) || (index >= numVertices))
	{
		GL_THROW("Restoration: The vertex number is out of bounds!");
	}

	return adjList[index]->getLength();
}

//Initialize 'iterPos'
void LinkedBase::initializePos(void)
{
	int indexvar;

	//Allocate up the array
	iterPos = (ChainIterator **)gl_malloc(numVertices*sizeof(ChainIterator *));

	//See if it worked
	if (iterPos == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	//Loop through and do the initializations
	for (indexvar=0; indexvar<numVertices; indexvar++)
	{
		//Begin with constructor for Chains
		//Call the constructor for everything (since doing explicit memory management)
		//Allocate each new object
		iterPos[indexvar] = (ChainIterator *)gl_malloc(sizeof(ChainIterator));

		//Make sure it worked
		if (iterPos[indexvar] == nullptr)
		{
			GL_THROW("Restoration:Failed to allocate new chain");
			//Defined above
		}

		//Call the constructor
		new (iterPos[indexvar]) ChainIterator();

	}
}

//delete 'iterPos'
void LinkedBase::deactivatePos(void)
{
	int indexvar;

	if (numVertices != 0)
	{
		//Loop through and remove chain items
		for (indexvar = 0; indexvar < numVertices; indexvar++)
		{
			gl_free(iterPos[indexvar]);

			//Null it - for paranoia (killed below anyways)
			iterPos[indexvar] = nullptr;
		}

		//Now de-allocate the mainpointer
		gl_free(iterPos);

		//Null it for good measure
		iterPos = nullptr;
	}
}

//Return the first vertex that is connected to the given vertex
int LinkedBase::beginVertex(int index)
{
	if ((index <0) || (index >= numVertices))
	{
		GL_THROW("Restoration: The vertex number is out of bounds!");
	}
	
	return iterPos[index]->initialize(adjList[index]);
}

//Return the next vertex that is connected to the given vertex
int LinkedBase::nextVertex(int index)
{
	if ((index <0) || (index >= numVertices))
	{
		GL_THROW("Restoration: The vertex number is out of bounds!");
	}

	return iterPos[index]->next();
}

// Breadth-first search
void LinkedBase::BFS(int s)
{
	int index, u, v;
	LinkedQueue *Q;

    // Intialization
    for (index=0; index<numVertices; index++)
	{
		status_value[index] = 0;
		parent_value[index] = -1;
		dist[index] = INFINITY;
	}

	source = s;
	status_value[s] = 1;
	dist[s] = 0.0;

	// Use the queue to manager vertices whose status is '1'
	//Create a temporary Q
	Q = (LinkedQueue *)gl_malloc(sizeof(LinkedQueue));

	//Make sure it worked
	if (Q == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new LinkedQueue");
	}

	//Call the constructor, just in case
	new (Q) LinkedQueue();

	Q->enQueue(s);
    
	// Initialize the iterator
	initializePos();

	// Search process
	while (!Q->isempty())
	{
		u = Q->deQueue();
        v = beginVertex(u);

		while (v != -1)
		{
            if (status_value[v] == 0) // v has not been discovered yet
			{
				status_value[v] = 1;
				dist[v] = dist[u] + 1.0;
				parent_value[v] = u;
				Q->enQueue(v);
			}

			v = nextVertex(u);
		}

		status_value[u] = 2;
	}
	
	// Deactive the iterator
	deactivatePos();
}

// Depth-first search I: create a depth-first forest
void LinkedBase::DFS1(void)
{
	int index, u;

    // Initialization
    for (index = 0; index<numVertices; index++)
	{
		status_value[index] = 0;
		parent_value[index] = -1;
	}

	dfs_time = 0;

	// Initialize the iterator
	initializePos();

	// Search from each vertex that is not discovered
    for (u = 0; u < numVertices; u++)
	{
        if (status_value[u] == 0)
		{
            DFSVisit(u);
		}
	}
    // Deactive the iterator
    deactivatePos();
}

// Depth-first search II: create a depth-first tree with a specified root.
// Only for a connected graph.
void LinkedBase::DFS2(int s)
{
	int index;

    // Initialization
    for (index = 0; index<numVertices; index++)
	{
		status_value[index] = 0;
		parent_value[index] = -1;
	}

	dfs_time = 0;

    // Initialize the iterator
	initializePos();

	// Search from the source
	source = s;
	DFSVisit(s);

	// Deactive the iterator
	deactivatePos();
}

//visits vertices in depth first search and marks them as visited; u is a vertex
void LinkedBase::DFSVisit(int u)
{
	int v;

	status_value[u] = 1;

	dfs_time = dfs_time + 1;

	dTime[u] = dfs_time;

	v = beginVertex(u);
    
	while (v != -1) // Explore eage (u, v)
	{
        if (status_value[v] == 0)
		{
			parent_value[v] = u;
			DFSVisit(v);
		}

		v = nextVertex(u);
	}

	status_value[u] = 2;	//Finished
	dfs_time = dfs_time + 1;
	fTime[u] = dfs_time;
}

// Copy graph: make graph2 a copy of graph1 
// Only the number of vertices and edges, and adjacency list are copied
// Properties for the graph iterator and graph traveral will not be copied
void LinkedBase::copy(LinkedBase *graph1)
{
	int idx;

	delAllVer();

	// add vertices without edges
	addAIsoVer(graph1->numVertices);	//This whole routine strikes me as odd, but this is theoretically verbatim from MATLAB
	numEdges = graph1->numEdges;

    // copy adjacency list
    for (idx = 0; idx < numVertices; idx++)
	{
		adjList[idx]->copy(graph1->adjList[idx]);
	}
}

// add n isolated vertice into a graph
void LinkedBase::addAIsoVer(int n)
{
	int indexvar, oldNumVertices;
	Chain **tempBase;

	//Store old number
	oldNumVertices = numVertices;

	//Store old list
	tempBase = adjList;

	//Update to new
	numVertices = numVertices + n;

	//Allocate a new array for these
	adjList = (Chain **)gl_malloc(numVertices*sizeof(Chain *));

	//See if it worked
	if (adjList == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined elsewhere
	}

	//Now copy in the values for the first part
	for (indexvar=0; indexvar<oldNumVertices; indexvar++)
	{
		adjList[indexvar] = tempBase[indexvar];
	}

	//Now that it is copied, destroy the old "main list" (elements stay alloced)
	gl_free(tempBase);

	//NULL it, for giggles
	tempBase = nullptr;
    
	//See if interPos has anything -- if it does, delete it first (just to be thorough)
	//Theoretically, only this references it, so prevents orphaning -- may need to revisit in the future if doesn't work
	if (iterPos != nullptr)
	{
		deactivatePos();
	}
    
	source = 0;
	dfs_time = 0;

	//Appears to zero things for new vector sizes -- check the first one and release, if needed
	if (status_value != nullptr)
	{
		//Free everyone first
		gl_free(status_value);
		gl_free(parent_value);
		gl_free(dist);
		gl_free(dTime);
		gl_free(fTime);

		//NULL us, to be safe
		status_value = nullptr;
		parent_value = nullptr;
		dist = nullptr;
		dTime = nullptr;
		fTime = nullptr;
	}

	//Allocate new space - basically copied from above (constructor)

	//Create the default vectors - initialize at the end (all same size)
	status_value = (int *)gl_malloc(numVertices*sizeof(int));

	//Check it
	if (status_value == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	parent_value = (int *)gl_malloc(numVertices*sizeof(int));

	if (parent_value == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	dist = (double *)gl_malloc(numVertices*sizeof(double));

	if (dist == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	dTime = (TIMESTAMP *)gl_malloc(numVertices*sizeof(TIMESTAMP));

	if (dTime == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	fTime = (TIMESTAMP *)gl_malloc(numVertices*sizeof(TIMESTAMP));

	if (fTime == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	//Update initializations to reflect the increased size (others already alloced and copied in above)
	for (indexvar=oldNumVertices; indexvar<numVertices; indexvar++)
	{
		//Begin with constructor for Chains
		//Call the constructor for everything (since doing explicit memory management)
		//Allocate each new object
		adjList[indexvar] = (Chain *)gl_malloc(sizeof(Chain));

		//Make sure it worked
		if (adjList[indexvar] == nullptr)
		{
			GL_THROW("Restoration:Failed to allocate new chain");
			//Defined above
		}

		//Call the construcotr
		new (adjList[indexvar]) Chain();
	}

	//Loop through and do the initializations for "all new" variables
	for (indexvar=0; indexvar<numVertices; indexvar++)
	{
		//Now do the other items
		status_value[indexvar] = 0;
		parent_value[indexvar] = -1;
		dist[indexvar] = INFINITY;
		dTime[indexvar] = 0;
		fTime[indexvar] = 0;
	}//End init loop
}

//****************************************/
//**** LinkedDigraph class functions  ****/
//****************************************/
// Decide whether the edge <i, j> exists in the graph
// If it exists, return 1; otherwise, return -1.
bool LinkedDigraph::isEdgeExisting(int iIndex, int jIndex)
{
	bool flag;

	flag = false;

    if ((iIndex < 0) || (jIndex < 0) || (iIndex >= numVertices) || (jIndex >= numVertices))
	{
		GL_THROW("Restoration: The vertex Number(s) is(are) out of bounds!");
	}
    else
	{
		if (adjList[iIndex]->search(jIndex) == -1)
		{
            flag = false;
		}
        else
		{
            flag = true;
		}
	}

	return flag;
}

// Add a new edge into the graph
void LinkedDigraph::addEdge(int iIndex, int jIndex)
{
    if ((iIndex < 0) || (jIndex < 0) || (iIndex >= numVertices) || (jIndex >= numVertices))
	{
		GL_THROW("Restoration: The vertex Number(s) is(are) out of bounds!");
	}
    if (iIndex == jIndex)
	{
        GL_THROW("Restoration: Error: Two vertexs of an edge can not be the same!");
	}
    if (isEdgeExisting(iIndex, jIndex))
	{
        GL_THROW("Restoration: Error: Can not add an edge that is already existing!");
	}
	adjList[iIndex]->addNode(0, jIndex);
	numEdges = numEdges + 1;
}

// Delete an edge from the graph
void LinkedDigraph::deleteEdge(int iIndex, int jIndex)
{
    if ((iIndex < 0) || (jIndex < 0) || (iIndex >= numVertices) || (jIndex >= numVertices))
	{
		GL_THROW("Restoration: The vertex Number(s) is(are) out of bounds!");
	}
	
	adjList[iIndex]->deleteNode(jIndex);

	numEdges = numEdges - 1;
}

// Return the in degree of the given vertex
int LinkedDigraph::getInDeg(int index)
{
	int curIndex, inDeg;

    if ((index < 0) || (index >= numVertices))
	{
        GL_THROW("Restoration: The vertex Number is out of bounds!");
	}

	inDeg = 0;

    for (curIndex=0; curIndex<numVertices; curIndex++)
	{
		if (adjList[curIndex]->search(index) != -1)
		{
            inDeg = inDeg + 1;
		}
	}

	return inDeg;
}

// Return the out degree of a given vertex
int LinkedDigraph::getOutDeg(int index)
{
	return LinkedBase::getOutDeg(index);
}

//****************************************/
//**** LinkedUndigraph class functions ***/
//****************************************/
// Add a new edge into the graph
void LinkedUndigraph::addEdge(int iIndex, int jIndex)
{
    if ((iIndex < 0) || (jIndex < 0) || (iIndex >= numVertices) || (jIndex >= numVertices))
	{
        GL_THROW("Restoration: The node Number(s) is(are) out of bounds!");
	}
    if (iIndex == jIndex)
	{
        GL_THROW("Restoration: Two nodes of an edge can not be the same!");
	}
	if (isEdgeExisting(iIndex,jIndex))
	{
        // error('Error: Can not add an edge that is already existing!');
		gl_warning("Restoration: (addEdge): Edge (%d,%d) is existing",iIndex,jIndex);
		return;
	}
	adjList[iIndex]->addNode(0, jIndex);
	adjList[jIndex]->addNode(0, iIndex);
    numEdges = numEdges + 1;
}

// Delete an edge from the graph
void LinkedUndigraph::deleteEdge(int iIndex, int jIndex)
{
	LinkedDigraph::deleteEdge(iIndex,jIndex);
	numEdges = numEdges + 1; // compensation
	LinkedDigraph::deleteEdge(jIndex,iIndex);
}

// Return the in degree of the given vertex
int LinkedUndigraph::getInDeg(int index)
{
	return LinkedDigraph::getInDeg(index);
}

// Return the out degree of the given vertex
int LinkedUndigraph::getOutDeg(int index)
{
	return LinkedDigraph::getOutDeg(index);
}

// Return the degree of the given vertex
int LinkedUndigraph::getDeg(int index)
{
    return getOutDeg(index);
}

// Find fundamental cut set
void LinkedUndigraph::findFunCutSet(CHORDSET *chordSet, BRANCHVERTICES *tBranch, CHORDSET *cutSet)
{
	int iIndex, jIndex, idx, curIndex, child_back, parent_back, k, debug_counter;
	bool isCutSetEle, is_bad_loop;

	//Original comments
	// uTree is the give undirected tree, assume DFS or BFS is already performed.
	// chordSet is a n*2 matrix, each row (i, j) corresponds to a chord.
	// tBranch is a 1*2 vector (i, j), denoting a tree edge of uTree
	// cutSet is the fundmental cut set

	//Empty the cutset, but default (assumes already allocated
	cutSet->currSize = 0;

	for (idx=0; idx<cutSet->maxSize; idx++)
	{
		cutSet->data_1[idx] = -1;
		cutSet->data_2[idx] = -1;
	}

	// Cut the given tree edge
	iIndex = tBranch->from_vert;
	jIndex = tBranch->to_vert;

	if (parent_value[iIndex] == jIndex)
	{
		parent_value[iIndex] = -1;
		// backup for resuming
		child_back = iIndex;
		parent_back = jIndex;
	}
	else
	{
		parent_value[jIndex] = -1;
		// backup for resuming
		child_back = jIndex; 
		parent_back = iIndex;
	}

	// Fine fundmental cut set
	for (idx=0; idx<chordSet->currSize; idx++)
	{
		// Set the status of all vertices to '0'
		for (k=0; k<numVertices; k++)
		{
			status_value[k] = 0;
		}

		iIndex = chordSet->data_1[idx];
		jIndex = chordSet->data_2[idx];

		// Mark the ancestors of vectex i as 1
		curIndex = iIndex;
		debug_counter = 0;
		is_bad_loop = false;

		while (curIndex != -1)
		{
			status_value[curIndex] = 1;
			curIndex = parent_value[curIndex];

			debug_counter++;	//Added to prevent it from getting stuck (circular loops)

			//Check the debug counter -- arbitrary check -- these seem like they should be upper limits
			if ((debug_counter > numVertices) && (debug_counter > numEdges))
			{
				is_bad_loop = true;
				break;	//Death to the loop!
			}
		}

		//Check for badness
		if (is_bad_loop)
		{
			continue;	//Just move on in lieu of doing anything better
		}

		//Race variables
		debug_counter = 0;

		// Check the status of vectex j's ancestors
		isCutSetEle = true;
		curIndex = jIndex;
		while (curIndex != -1)
		{
			if (status_value[curIndex] == 1)
			{
				isCutSetEle = false; // (i,j) is not a element of the fundmental cut set
				break;
			}
			curIndex = parent_value[curIndex];

			debug_counter++;	//Added to prevent it from getting stuck (circular loops)

			//Check the debug counter -- arbitrary check -- these seem like they should be upper limits
			if ((debug_counter > numVertices) && (debug_counter > numEdges))
			{
				isCutSetEle = false;	//Short cirucit next part - effectively a "continue"
				break;	//Death to the loop!
			}
		}

		// if (i,j) is a element of the fundmental cut set, add it to cutSet
		if (isCutSetEle)
		{
			//Add it to the cutSet
			cutSet->data_1[cutSet->currSize] = chordSet->data_1[idx];
			cutSet->data_2[cutSet->currSize] = chordSet->data_2[idx];

			//Increment the pointer
			cutSet->currSize++;

			//Check for size issues
			if (cutSet->currSize >= cutSet->maxSize)
			{
				if (cutSet->currSize == cutSet->maxSize)
				{
					gl_verbose("cutSet just hit maximum length");
				}
				else	//Must be greater, we're already broken
				{
					GL_THROW("restoration: allocated array size exceeded!");
					//Defined elsewhere
				}
			}
		}
	}

	// Resume uTree
	parent_value[child_back] = parent_back;
}

// Graph simplify
// Delete vertices whose degree is less than 3
// Vertices connected with chords (tie switches) should be retained
// Vertices connected with the edge corresponding to fault should be retained
// The vertex denoting source should be retained
// Feeder vertices should not be deleted.
//
//vMap is assumed to be the unique output (pre-allocated outside this call)
void LinkedUndigraph::simplify(CHORDSET *chordset, BRANCHVERTICES *fBranch, int source, INTVECT *fVer, INTVECT *vMap)
{
	int newsize, idxval, baseidx, resultval;

	INTVECT tempvect;
	INTVECT iVer;

	//Initialize local versions, just in case
	tempvect.data = nullptr;
	iVer.data = nullptr;

	//Original comments
    // uTree is the orignal tree, assume BFS is already performed.
    // chordSet is a n*2 matrix, each row (i, j) corresponds to a chord
    // fBranch is a 1*2 vector (i, j), denoting the edge corresponding to fault
    // source is the vertex denoting source

	//Get size of the big sorty vector - 3 = fBranch (2) + source (1)
	newsize = chordset->currSize + chordset->currSize + 3 + fVer->currSize;

	//Malloc it up - use the heap, just cause
	tempvect.currSize = 0;
	tempvect.maxSize = newsize;
	tempvect.data = (int *)gl_malloc(newsize*sizeof(int));

	//Make sure it worked
	if (tempvect.data == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		//Defined elsewhere
	}

	//Loop through and populate it - first chordset
	for (idxval=0; idxval<chordset->currSize; idxval++)
	{
		tempvect.data[idxval] = chordset->data_1[idxval];
		tempvect.data[idxval+chordset->currSize] = chordset->data_2[idxval];
	}

	//update base
	baseidx = chordset->currSize + chordset->currSize;

	//Add in fBranch portions
	tempvect.data[baseidx] = fBranch->from_vert;
	tempvect.data[baseidx+1] = fBranch->to_vert;

	//Update pointer
	baseidx = baseidx + 2;

	//Store the source
	tempvect.data[baseidx] = source;

	//Update pointer, for grins
	baseidx++;

	//Loop through and populate - add fVer
	for (idxval=0; idxval<fVer->currSize; idxval++)
	{
		tempvect.data[baseidx+idxval] = fVer->data[idxval];
	}
	
	//Set current size, as matter of principle
	tempvect.currSize = baseidx + fVer->currSize - 1;

	//Allocate the "unique" output vector -- assume it will never be bigger than vMap's largest size (assumed to be total number of nodes)
	iVer.data = (int *)gl_malloc(vMap->maxSize*sizeof(int));

	//Make sure it worked
	if (iVer.data == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		//Defined elsewhere
	}

	//Set variables
	iVer.currSize = 0;
	iVer.maxSize = vMap->maxSize;

    // Form a set for vertices that can not be deleted (important vertices)
	//Call the uniquifier
	unique_int(&tempvect,&iVer);

    // simplify
	//Do first call to explicitly do the while loop (don't trust other syntax)
	resultval = isolateDeg12Ver(&iVer);

	//Loop -- still not convinced this isn't a single execution or a race condition - we'll see (******************)
	while (resultval != 0)
	{
		resultval = isolateDeg12Ver(&iVer);
	}

	//Now that done, call the last item
	deleteIsoVer(vMap);

	//Free up our mess
	gl_free(tempvect.data);
	gl_free(iVer.data);
}

//Find function
//Parses a vector and extracts the indices of any matches of findval (to the maximum number in numfind)
//and places them into foundvect.  If numfind is -1, it finds all matches, otherwise it stops after numfind entries
//foundvect currsize variable will be a test for success (+1 = number of entries found, so -1 = none found)
void LinkedUndigraph::find_int(INTVECT *inputvect, INTVECT *foundvect, int findval, int numfind)
{
	int indexval;

	//Reset output vector as a matter of principle - set it to -1 (only for numfind, unless it is -1)
	if (numfind != -1)
	{
		for (indexval=0; indexval<numfind; indexval++)
		{
			foundvect->data[indexval] = -1;
		}
	}
	else	//Init the whole thing, just in case
	{
		for (indexval=0; indexval<foundvect->maxSize; indexval++)
		{
			foundvect->data[indexval] = -1;
		}
	}

	//Reset tracker
	foundvect->currSize = 0;

	//Reset loop variable
	indexval = 0;

	//Loop
	while (indexval < inputvect->currSize)
	{
		//Check the entry
		if (inputvect->data[indexval] == findval)
		{
			//Store the value
			foundvect->data[foundvect->currSize] = indexval;

			//Increment the pointer (will fix later)
			foundvect->currSize++;

			//See if we've hit a limit or not
			if (numfind != -1)
			{
				if (foundvect->currSize >= numfind)	//Just in case someone puts zero in
				{
					break;	//outta the while loop
				}
			}
			//Default else, just keep going

		}
		//Default else - continue
			
		indexval++;
	}

	//Decrement the find output -- will serve as a "success or not" indicator
	foundvect->currSize--;
}

// a step for simulifying the original graph
// Isolate vertices whose degree is 1 or 2
int LinkedUndigraph::isolateDeg12Ver(INTVECT *iVer)
{
	int iNum, idx, u, v, v1, v2;
	int *iMark;

    // uGraph is the original graph, which will be directly modified
    // iVer is a set of vertices which can not be isolated (important vertices)
    // iVer is a 1*n vector
    // iNum is the number of vertices that has been isolated
    
    // Mark important vertices
	iMark = (int *)gl_malloc(numVertices*sizeof(int));

	//Make sure it worked
	if (iMark == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		/*  TROUBLESHOOT
		While attempting to allocate space for a new temporary variable, an error was encountered.
		Please try again.  If the error persists, please submit your code via the ticketing system.
		*/
	}

	//Loop through and zero it
	for (idx=0; idx<numVertices; idx++)
	{
		iMark[idx] = 0;
	}

	//Loop through the size
	for (idx=0; idx<iVer->currSize; idx++)
	{
		iMark[iVer->data[idx]] = 1;
	}
    
    // Isolation       
    iNum = 0;
    for (u=0; u<numVertices; u++)
	{
        if (iMark[u] == 0) // not an important vertex
		{
            if (getDeg(u) == 1)	//  Isolate vertices whose degree is 1
			{
                // find the vertex connected to u
				v = adjList[u]->first->data;
                // delete edge (u, v)
				deleteEdge(u,v);
                iNum = iNum + 1;
                continue;
			}

            if (getDeg(u) == 2) //  Isolate vertices whose degree is 2
			{
                // find the vertices connected to u
				v1 = adjList[u]->first->data;
				v2 = adjList[u]->first->link->data;
                // delete edge (u, v1) and (u, v2)
				deleteEdge(u,v1);
				deleteEdge(u,v2);
                // add edge (v1, v2)
				addEdge(v1,v2);
                iNum = iNum + 1;
                continue;
			}
		}
	}

	//Free up iMark, we're done with it
	gl_free(iMark);

	return iNum;
}

// Delete all isolated vertices (degree = 0)
void LinkedUndigraph::deleteIsoVer(INTVECT *vMap)
{
	int idx, idx2, numIsoVer, newArraySize;
	CHAINNODE *v;
	INTVECT isoVerArray;
	Chain **tempList;
	bool allocation_needed;

	//Initialize local variables, just in case
	isoVerArray.data = nullptr;
	allocation_needed = true;

    // vMap is a map between old number and new number of vertices
    // 'vMap(i) = j' means that vertex i in the original graph is
    // mapped to vertex j in the new graph
    // If vertex i in the original graph is deleted, vMap(i) = 0

	//Check and see if vMap is already allocated -- if it is, remove it first
	//Just in case a variable is reused outside
	if (vMap->data != nullptr)
	{
		//See if it is 
		if (vMap->maxSize != numVertices)
		{
			//Free it
			gl_free(vMap->data);

			//No need to NULL it, we're just going to allocate it again in a second
			//Changed my mind, NULL it anyways
			vMap->data = nullptr;

			//Flag
			allocation_needed = true;
		}
		else	//Same size, just overwrite it
		{
			allocation_needed = false;
		}
	}

	//Allocate, if needed
	if (allocation_needed)
	{
		//Allocate vMap and make the initial one
		vMap->data = (int *)gl_malloc(numVertices*sizeof(int));

		//Make sure it worked
		if (vMap->data == nullptr)
		{
			GL_THROW("Restoration:Failed to allocate new temp variable");
			/*  TROUBLESHOOT
			While attempting to allocate space for a new temporary variable, an error was encountered.
			Please try again.  If the error persists, please submit your code via the ticketing system.
			*/
		}
	}//End allocation needed

	//Initialize the extra parameters, for tracking
	vMap->currSize = numVertices;
	vMap->maxSize = numVertices;

    // Initial vMap
	for (idx=0; idx<numVertices; idx++)
	{
		vMap->data[idx] = idx;
	}
    
    // Form vMap
    numIsoVer = 0; // Number of isolated vertices

	//Allocate isoVerArray to be max size of nodes (scale back later)
	//isoVerArray =  A set of isolated vertices
	isoVerArray.data = (int *)gl_malloc(numVertices*sizeof(int));

	//Make sure it worked
	if (isoVerArray.data == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		//Defined above
	}

	//Set the max size and "currsize"
	isoVerArray.maxSize = numVertices;
	isoVerArray.currSize = 0;

	//For giggles, loop it and -1 it (arbitrary)
	for (idx=0; idx<numVertices; idx++)
	{
		isoVerArray.data[idx] = -1;
	}

    for (idx=0; idx<numVertices; idx++)
	{
        if (getDeg(idx) == 0)
		{
			vMap->data[idx] = -1;

			for (idx2=(idx+1); idx2<numVertices; idx2++)
			{
				vMap->data[idx2] = vMap->data[idx2] - 1;
			}
            
			numIsoVer = numIsoVer + 1;
            
			isoVerArray.data[isoVerArray.currSize] = idx;
			isoVerArray.currSize++;
		}
	}
    
    // Modify index of vertices
    for (idx=0; idx<numVertices; idx++)
	{
		v = adjList[idx]->first;

		while (v != nullptr)
		{
			v->data = vMap->data[v->data];
            v = v->link;
		}
	}
    
    // Delete isolated vertices
    // Free memory
    for (idx=0; idx<numIsoVer; idx++)
	{
		//Remove items no longer needed
		gl_free(adjList[isoVerArray.data[idx]]);

		//Null it, to be safe -- theoretically done, but be paranoid
		adjList[isoVerArray.data[idx]] = nullptr;
	}

    // Delete from adjacency list
    for (idx=0; idx<numVertices; idx++)
	{
		if (vMap->data[idx] != -1)
		{
			adjList[vMap->data[idx]] = adjList[idx];
		}
	}

	//Calculate new array size for "fixing"
	newArraySize = numVertices - numIsoVer;

	//Copy the pointer to the temporary list
	tempList = adjList;

	//Allocate new master array
	adjList = (Chain **)gl_malloc(newArraySize*sizeof(Chain *));

	//See if it worked
	if (adjList == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined elsewhere
	}

	//Now copy in the values for the first part
	for (idx=0; idx<newArraySize; idx++)
	{
		adjList[idx] = tempList[idx];
	}

	//Now that it is copied, destroy the old "main list" (elements stay alloced)
	//Theoretically, any of the "extended" items have already been dealloced above in the adjacency list for loop (previous for)
	gl_free(tempList);

	//NULL us, out of paranoia
	tempList = nullptr;
    
    // modify the number of vertices
    numVertices = newArraySize;
    
    // modify the length of other properties
	if (status_value != nullptr)	//Free them first
	{
		gl_free(status_value);
		gl_free(parent_value);
		gl_free(dist);
		gl_free(dTime);
		gl_free(fTime);

		//NULL em all
		status_value = nullptr;
		parent_value = nullptr;
		dist = nullptr;
		dTime = nullptr;
		fTime = nullptr;
	}

	//Alloc again (copy from elsewhere)
	//Allocate new space - basically copied from above (constructor)

	//Create the default vectors - initialize at the end (all same size)
	status_value = (int *)gl_malloc(numVertices*sizeof(int));

	//Check it
	if (status_value == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	parent_value = (int *)gl_malloc(numVertices*sizeof(int));

	if (parent_value == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	dist = (double *)gl_malloc(numVertices*sizeof(double));

	if (dist == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	dTime = (TIMESTAMP *)gl_malloc(numVertices*sizeof(TIMESTAMP));

	if (dTime == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	fTime = (TIMESTAMP *)gl_malloc(numVertices*sizeof(TIMESTAMP));

	if (fTime == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	//Loop through and do the initializations for "all new" variables
	for (idx=0; idx<numVertices; idx++)
	{
		//Now do the other items
		status_value[idx] = 0;
		parent_value[idx] = -1;
		dist[idx] = INFINITY;
		dTime[idx] = 0;
		fTime[idx] = 0;
	}//End init loop

	//Free up isoVerArray, since it is no longer needed
	gl_free(isoVerArray.data);
}

// Merge vertices according to the map of vertices
void LinkedUndigraph::mergeVer_2(INTVECT *vMap)
{
	int idx, tempidx, n_V_new, k, numIsoVer, newArraySize, numFound;
	INTVECT V_new, isoVerArray, resVerArray, merge_V, tempoutVect;
	CHAINNODE *cur_ver, *next_ver;
	Chain **tempList;

	//Initialize local variables, just in case
	V_new.data = nullptr;
	isoVerArray.data = nullptr;
	resVerArray.data = nullptr;
	merge_V.data = nullptr;
	tempoutVect.data = nullptr;
	tempList = nullptr;

	//Allocate up V_new -- assume it won't be any bigger than vMap
	V_new.currSize = vMap->currSize;
	V_new.maxSize = vMap->maxSize;
	V_new.data = (int *)gl_malloc(V_new.maxSize*sizeof(int));

	//Check it
	if (V_new.data == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		//Defined above
	}

	//Find the unique pairs
	unique_int(vMap,&V_new);

	n_V_new = V_new.currSize;

	//Allocate three working arrays -- assume it won't be any bigger than vMap for now (may need to be revisited)
	isoVerArray.currSize = 0;
	resVerArray.currSize = 0;
	merge_V.currSize = 0;
	tempoutVect.currSize = 0;
	
	isoVerArray.maxSize = vMap->maxSize;
	resVerArray.maxSize = vMap->maxSize;
	merge_V.maxSize = vMap->maxSize;	//This should be extremely overkill, but better safe than sorry
	tempoutVect.maxSize = vMap->maxSize;	//Again overkill, but oh well

	isoVerArray.data = (int *)gl_malloc(isoVerArray.maxSize*sizeof(int));
	resVerArray.data = (int *)gl_malloc(resVerArray.maxSize*sizeof(int));
	merge_V.data = (int *)gl_malloc(merge_V.maxSize*sizeof(int));
	tempoutVect.data = (int *)gl_malloc(tempoutVect.maxSize*sizeof(int));

	//Make sure they worked
	if (isoVerArray.data == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		//Defined above
	}

	if (resVerArray.data == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		//Defined above
	}

	if (merge_V.data == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		//Defined above
	}

	if (tempoutVect.data == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		//Defined above
	}

    numIsoVer = 0;

    // Merge vertices
    for (idx=0; idx<n_V_new; idx++)
	{
		//Search for matches
		find_int(vMap,&merge_V,V_new.data[idx],-1);
        // **** Seems like there should be an empty check here, but seems to be handled by changes below

		//Add found values into array
		numFound = merge_V.currSize + 1;

		//Add to resVerArray
		resVerArray.data[resVerArray.currSize] = merge_V.data[0];
		resVerArray.currSize++;

		//Copy the rest in
		for (tempidx=1; tempidx<numFound; tempidx++)
		{
			//Store it
			isoVerArray.data[isoVerArray.currSize] = merge_V.data[tempidx];

			//Increment storage pointer
			isoVerArray.currSize++;
		}

		//Update numIsoVer, even though it is already in the vector
		numIsoVer = isoVerArray.currSize;

		//Check for empty
		if (merge_V.data[0] == -1)	//Empty
		{
			cur_ver = nullptr;
		}
		else
		{
			cur_ver = adjList[merge_V.data[0]]->first;
		}

		while (cur_ver != nullptr)
		{
			if (cur_ver->link != nullptr)
			{
				next_ver = cur_ver->link;
			}
			else
			{
				next_ver = nullptr;
			}

			//See if something can be found
			find_int(&merge_V,&tempoutVect,cur_ver->data,1);

			//Check
			if (tempoutVect.currSize != -1)
			{
				deleteEdge(merge_V.data[0],cur_ver->data);
			}
            cur_ver = next_ver;
		}
        
        for (k=1; k<numFound; k++)	//Starts 1 index higher (started at 2 in ML)
		{
			//Initial check
			if (merge_V.data[k] != -1)	//Empty
			{
				cur_ver = adjList[merge_V.data[k]]->first;
			}
			else	//Empty
			{
				cur_ver = nullptr;
			}

            while (cur_ver != nullptr)
			{
				if (cur_ver->link != nullptr)
				{
					next_ver = cur_ver->link;
				}
				else
				{
					next_ver = nullptr;
				}

				//See if anything more is found
				find_int(&merge_V,&tempoutVect,cur_ver->data,1);

				//Check
				if (tempoutVect.currSize != -1)
				{
					deleteEdge(merge_V.data[k],cur_ver->data);
				}
                else
				{
					addEdge(merge_V.data[0],cur_ver->data);
					deleteEdge(merge_V.data[k],cur_ver->data);
				}
                cur_ver = next_ver;
			}
		}
	}

    // Modify indexes of vertices
    for (idx=0; idx<numVertices; idx++)
	{
		cur_ver = adjList[idx]->first;
        while (cur_ver != nullptr)
		{
			cur_ver->data = vMap->data[cur_ver->data];
            cur_ver = cur_ver->link;
		}
	}

    // Delete isolated vertices
    // Free memory
    for (idx=0; idx<numIsoVer; idx++)
	{
		//Remove items no longer needed
		gl_free(adjList[isoVerArray.data[idx]]);

		//Null it, to be safe -- theoretically done, but be paranoid
		adjList[isoVerArray.data[idx]] = nullptr;
	}

    // Delete from adjacency list
	unique_int(&resVerArray,&V_new);
	for (idx=0; idx<V_new.currSize; idx++)
	{
		//Copy the values in
		adjList[idx] = adjList[V_new.data[idx]];
	}

	//Calculate new array size for "fixing"
	newArraySize = numVertices - numIsoVer;

	//Copy the pointer to the temporary list
	tempList = adjList;

	//Allocate new master array
	adjList = (Chain **)gl_malloc(newArraySize*sizeof(Chain *));

	//See if it worked
	if (adjList == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined elsewhere
	}

	//Now copy in the values for the first part
	for (idx=0; idx<newArraySize; idx++)
	{
		adjList[idx] = tempList[idx];
	}

	//Now that it is copied, destroy the old "main list" (elements stay alloced)
	//Theoretically, any of the "extended" items have already been dealloced above in the adjacency list for loop (previous for)
	gl_free(tempList);

	//NULL it, to be safe
	tempList = nullptr;
    
    // modify the number of vertices
    numVertices = newArraySize;
    
    // modify the length of other properties
	if (status_value != nullptr)	//Free them first
	{
		gl_free(status_value);
		gl_free(parent_value);
		gl_free(dist);
		gl_free(dTime);
		gl_free(fTime);

		//NULL them all
		status_value = nullptr;
		parent_value = nullptr;
		dist = nullptr;
		dTime = nullptr;
		fTime = nullptr;
	}

	//Alloc again (copy from elsewhere)
	//Allocate new space - basically copied from above (constructor)

	//Create the default vectors - initialize at the end (all same size)
	status_value = (int *)gl_malloc(numVertices*sizeof(int));

	//Check it
	if (status_value == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	parent_value = (int *)gl_malloc(numVertices*sizeof(int));

	if (parent_value == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	dist = (double *)gl_malloc(numVertices*sizeof(double));

	if (dist == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	dTime = (TIMESTAMP *)gl_malloc(numVertices*sizeof(TIMESTAMP));

	if (dTime == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	fTime = (TIMESTAMP *)gl_malloc(numVertices*sizeof(TIMESTAMP));

	if (fTime == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new chain");
		//Defined above
	}

	//Loop through and do the initializations for "all new" variables
	for (idx=0; idx<numVertices; idx++)
	{
		//Now do the other items
		status_value[idx] = 0;
		parent_value[idx] = -1;
		dist[idx] = INFINITY;
		dTime[idx] = 0;
		fTime[idx] = 0;
	}//End init loop

	//Free up isoVerArray, since it is no longer needed
	gl_free(isoVerArray.data);

	//Free up resVerArray, since it was just a working array
	gl_free(resVerArray.data);

	//Free up temporary vectors
	gl_free(V_new.data);
	gl_free(merge_V.data);
	gl_free(tempoutVect.data);
}

//******************** General functions ***********************//

// Unique function
// Examines elements of inputvect and returns an order-sorted as outputvect
void unique_int(INTVECT *inputvect, INTVECT *outputvect)
{
	INTVECT workmatrix, workmatrix_input;
	int leftidx, rightidx;

	//Allocate working matrix of the same size as the input vector
	workmatrix.currSize = inputvect->currSize;
	workmatrix.maxSize = inputvect->maxSize;
	workmatrix.data = (int *)gl_malloc(workmatrix.maxSize*sizeof(int));

	//Make sure it worked
	if (workmatrix.data == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		//Defined elsewhere
	}

	//Allocate a working input matrix too -- otherwise we potentially screw up the order elsewhere (keep it separate)
	workmatrix_input.currSize = inputvect->currSize;
	workmatrix_input.maxSize = inputvect->maxSize;
	workmatrix_input.data = (int *)gl_malloc(workmatrix_input.maxSize*sizeof(int));

	//Make sure it worked
	if (workmatrix_input.data == nullptr)
	{
		GL_THROW("Restoration:Failed to allocate new temp variable");
		//Defined elsewhere
	}

	//Copy the input into the work matrix - do a memcpy, just to be clever - only copy the current size (we don't need more)
	memcpy(workmatrix_input.data,inputvect->data,inputvect->currSize*sizeof(int));

	//Perform the merge sort
	merge_sort_int(workmatrix_input.data,workmatrix_input.currSize, workmatrix.data);

	//Now loop through the vectors and only keep the unique values
	leftidx = 1;
	rightidx = 0;

	//First value is unique, by default
	outputvect->data[0] = workmatrix_input.data[0];

	while (leftidx < workmatrix_input.currSize)
	{
		if (workmatrix_input.data[leftidx] != outputvect->data[rightidx])	//Unique
		{
			//Increment pointer first
			rightidx++;

			//Do a check, for good measure
			if (rightidx>outputvect->maxSize)
			{
				GL_THROW("Restoration: Larger number of unique entries than expected found!");
				/*  TROUBLESHOOT
				While looking for unique vertices in the restoration algorithm, more unique values than expected
				were found and the vector is mis-sized.  Please submit your code and a bug report via the ticketing system.
				*/
			}

			//Store the value
			outputvect->data[rightidx] = workmatrix_input.data[leftidx];

		}
		else	//Not unique, increment
		{
			leftidx++;
		}
	}

	//Set the output size
	outputvect->currSize = rightidx + 1;

	//Free the working matrix
	gl_free(workmatrix.data);
	gl_free(workmatrix_input.data);
}

//Merge sort - same as in solver_NR (replicated for typing)
void merge_sort_int(int *Input_Array, unsigned int Alen, int *Work_Array)
{
	unsigned int split_point;
	unsigned int right_length;
	int *leftside, *rightside;
	int *Final_P;

	if (Alen>0)	//Only occurs if over zero
	{
		split_point = Alen/2;	//Find the middle point
		right_length = Alen - split_point;	//Figure out how big the right hand side is

		//Make the appropriate pointers
		leftside = Input_Array;
		rightside = Input_Array+split_point;

		//Check to see what condition we are in (keep splitting it)
		if (split_point>1)
			merge_sort_int(leftside,split_point,Work_Array);

		if (right_length>1)
			merge_sort_int(rightside,right_length,Work_Array);

		//Point at the first location
		Final_P = Work_Array;

		//Merge them now
		do {
			if (*leftside < *rightside)
				*Final_P++ = *leftside++;
			else	//Equal or greater, so arbitrarily keep
				*Final_P++ = *rightside++;
		} while ((leftside<(Input_Array+split_point)) && (rightside<(Input_Array+Alen)));	//Sort the list until one of them empties

		while (leftside<(Input_Array+split_point))	//Put any remaining entries into the list
			*Final_P++ = *leftside++;

		while (rightside<(Input_Array+Alen))		//Put any remaining entries into the list
			*Final_P++ = *rightside++;

		memcpy(Input_Array,Work_Array,sizeof(int)*Alen);	//Copy the result back into the input
	}	//End length > 0
}



//Export for external object function calls
EXPORT int perform_restoration(OBJECT *thisobj, int faulting_link)
{
	restoration *temp_rest_obj = OBJECTDATA(thisobj,restoration);

	return temp_rest_obj->PerformRestoration(faulting_link);
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: restoration
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_restoration(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(restoration::oclass);
		if (*obj!=nullptr)
		{
			restoration *my = OBJECTDATA(*obj,restoration);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(restoration);
}

EXPORT int init_restoration(OBJECT *obj, OBJECT *parent)
{
	try {
		return OBJECTDATA(obj,restoration)->init(parent);
	}
	INIT_CATCHALL(restoration);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_restoration(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	return TS_NEVER;
}
EXPORT int isa_restoration(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,restoration)->isa(classname);
}

/**@}**/
