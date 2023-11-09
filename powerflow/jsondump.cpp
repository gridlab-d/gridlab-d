// $Id: jsondump.cpp
/**	Copyright (C) 2017 Battelle Memorial Institute
*/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <fstream>
#include <cstring>
#include <json/json.h> //jsoncpp library
#include <string>
#include <sstream>
using namespace std;

#include "jsondump.h"

//////////////////////////////////////////////////////////////////////////
// jsondump CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* jsondump::oclass = nullptr;


jsondump::jsondump(MODULE *mod)
{
	if (oclass==nullptr)
	{
		// register the class definition
		oclass = gl_register_class(mod,"jsondump",sizeof(jsondump),PC_AUTOLOCK);

		if (oclass == nullptr)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_char32,"group",PADDR(group),PT_DESCRIPTION,"the group ID to output data for (all objects if empty)",
			PT_char256,"filename_dump_system",PADDR(filename_dump_system),PT_DESCRIPTION,"the file to dump the current data into",
			PT_char256,"filename_dump_reliability",PADDR(filename_dump_reliability),PT_DESCRIPTION,"the file to dump the current data into",
			PT_timestamp,"runtime",PADDR(runtime),PT_DESCRIPTION,"the time to check voltage data",
			PT_int32,"runcount",PADDR(runcount),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the number of times the file has been written to",
			PT_bool,"write_system_info",PADDR(write_system),PT_DESCRIPTION,"Flag indicating whether system information will be written into JSON file or not",
			PT_bool,"write_reliability",PADDR(write_reliability),PT_DESCRIPTION,"Flag indicating whether reliabililty information will be written into JSON file or not",
			PT_bool,"write_per_unit",PADDR(write_per_unit),PT_DESCRIPTION,"Output the quantities as per-unit values",
			PT_double,"system_base[VA]",PADDR(system_VA_base),PT_DESCRIPTION,"System base power rating for per-unit calculations",
			PT_double,"min_node_voltage[pu]",PADDR(min_volt_value),PT_DESCRIPTION,"Per-unit minimum voltage level allowed for nodes",
			PT_double,"max_node_voltage[pu]",PADDR(max_volt_value),PT_DESCRIPTION,"Per-unit maximum voltage level allowed for nodes",

			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

int jsondump::create(void)
{
	// Set write flag default as false
	write_system = false;
	write_reliability = false;
	write_per_unit = false;	//By default, do full values
	system_VA_base = -1.0;		//Flag value

	group.erase();
	runcount = 0;

	min_volt_value = 0.8;
	max_volt_value = 1.2;

	return 1;
}

int jsondump::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);

	// Check if we need to dump line and line configuration to JSON file
	if ((filename_dump_system[0] == '\0') && write_system){
		filename_dump_system = "JSON_dump_line.json";
	}

	// Check if we need to dump reliability to JSON file
	if ((filename_dump_reliability[0] == '\0') && write_reliability){
		filename_dump_reliability = "JSON_dump_reliability.json";
	}

	//Check per-unitization
	if ((filename_dump_system[0] != '\0') && write_system && write_per_unit)
	{
		//Make sure the base value is proper
		if (system_VA_base <= 0.0)
		{
			gl_warning("jsondump:%d-%s -- If per-unit impedance is desired, a valid system_base must be specified - defaulting to 100 MVA",hdr->id,(hdr->name ? hdr->name : "Unnamed"));
			/*  TROUBLESHOOT
			To use the per-unit functionality of the jsdondump object, a valid system base must be specified (>0.0).  Either none was specified, or an invalid one was specified.
			As such, the base is adjusted to an assumed base of 100 MVA.  If this is not a valid base, please specify the proper value.
			Please try again.
			*/

			system_VA_base = 100.0e6;
		}
	}

	return 1;
}

STATUS jsondump::dump_system(void)
{
	//TODO: See if the object arrays need to be maintained/are even needed!

	FINDLIST *ohlines, *tplines, *uglines, *switches, *regulators, *regConfs, *lineConfs, *tpLineConfs, *TransformerList, *TransConfsList, *nodes, *meters, *loads, *inverters, *diesels, *fuses;
	FINDLIST *reclosers, *sectionalizers;
	OBJECT *objhdr = OBJECTHDR(this);
	OBJECT *obj = nullptr;
	OBJECT *obj_lineConf = nullptr;
	OBJECT *obj_tplineConf = nullptr;
	char buffer[1024];
	FILE *fn = nullptr;
	int index = 0;
	int phaseCount;
	double per_unit_base, temp_impedance_base, temp_voltage_base, temp_de_pu_base;
	gld::complex temp_complex_voltage_value[3];
	gld::complex temp_complex_power_value[3];
	double temp_voltage_output_value;
	int indexA, indexB, indexC;
	gld::complex *b_mat_pu;
	bool *b_mat_defined;
	gld::complex *b_mat_tp_pu;
	bool  *b_mat_tp_defined;
	gld::complex *b_mat_trans_pu;
	bool *b_mat_trans_defined;
	int *trans_phase_count;
	gld::complex *b_mat_reg_pu;
	bool *b_mat_reg_defined;
	int *reg_phase_count;
	gld::complex b_mat_switch_pu[9];
	bool b_mat_switch_defined;
	int switch_phase_count;
	gld::complex b_mat_fuse_pu[9];
	bool b_mat_fuse_defined;
	int fuse_phase_count;
	set temp_set_value;
	enumeration temp_enum_value;
	bool found_match_config;

	// metrics JSON value
	Json::Value metrics_lines;	// Output dictionary for line and line configuration metrics
	Json::Value node_object;
	Json::Value load_object;
	Json::Value line_object;
	Json::Value line_configuration_object;
	Json::Value jsonArray;
	Json::Value jsonSecondArray;
	Json::Value jsonArray1; // for storing rmatrix and xmatrix
	Json::Value jsonArray2; // for storing rmatrix and xmatrix
	// Start write to file
	Json::StreamWriterBuilder builder;
	builder["commentStyle"] = "None";
	builder["indentation"] = "";

	// Open file for writing
	ofstream out_file;

	//find the link objects
	if(group[0] == '\0'){
		ohlines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"overhead_line",AND,FT_MODULE,SAME,"powerflow",FT_END);				//find all overhead_lines
		tplines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"triplex_line",AND,FT_MODULE,SAME,"powerflow",FT_END);				//find all triplex_lines
		uglines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"underground_line",AND,FT_MODULE,SAME,"powerflow",FT_END);			//find all underground_lines
		switches = gl_find_objects(FL_NEW,FT_CLASS,SAME,"switch",AND,FT_MODULE,SAME,"powerflow",FT_END);					//find all switches
		sectionalizers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"sectionalizer",AND,FT_MODULE,SAME,"powerflow",FT_END);		//find all sectionalizer
		reclosers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"recloser",AND,FT_MODULE,SAME,"powerflow",FT_END);					//find all recloser
		fuses = gl_find_objects(FL_NEW,FT_CLASS,SAME,"fuse",AND,FT_MODULE,SAME,"powerflow",FT_END);							//Find all fuses
		regulators = gl_find_objects(FL_NEW,FT_CLASS,SAME,"regulator",AND,FT_MODULE,SAME,"powerflow",FT_END);				//find all regulators
		regConfs = gl_find_objects(FL_NEW,FT_CLASS,SAME,"regulator_configuration",AND,FT_MODULE,SAME,"powerflow",FT_END);	//find all regulator configurations
		lineConfs = gl_find_objects(FL_NEW,FT_CLASS,SAME,"line_configuration",AND,FT_MODULE,SAME,"powerflow",FT_END);			//find all line configurations
		tpLineConfs = gl_find_objects(FL_NEW,FT_CLASS,SAME,"triplex_line_configuration",AND,FT_MODULE,SAME,"powerflow",FT_END);//find all triplex line configurations
		TransConfsList = gl_find_objects(FL_NEW,FT_CLASS,SAME,"transformer_configuration",AND,FT_MODULE,SAME,"powerflow",FT_END);//find all transformer configurations
		TransformerList = gl_find_objects(FL_NEW,FT_CLASS,SAME,"transformer",AND,FT_MODULE,SAME,"powerflow",FT_END);			//find all transformers
		nodes = gl_find_objects(FL_NEW,FT_CLASS,SAME,"node",AND,FT_MODULE,SAME,"powerflow",FT_END);								//Find all nodes
		meters = gl_find_objects(FL_NEW,FT_CLASS,SAME,"meter",AND,FT_MODULE,SAME,"powerflow",FT_END);							//Find all meters
		loads = gl_find_objects(FL_NEW,FT_CLASS,SAME,"load",AND,FT_MODULE,SAME,"powerflow",FT_END);								//Find all loads

		inverters = gl_find_objects(FL_NEW,FT_CLASS,SAME,"inverter",AND,FT_MODULE,SAME,"generators",FT_END);					//Find all inverters
		diesels = gl_find_objects(FL_NEW,FT_CLASS,SAME,"diesel_dg",AND,FT_MODULE,SAME,"generators",FT_END);						//Find all diesels
	} else {
		ohlines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"overhead_line",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		tplines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"triplex_line",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		uglines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"underground_line",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		switches = gl_find_objects(FL_NEW,FT_CLASS,SAME,"switch",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		sectionalizers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"sectionalizer",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		reclosers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"recloser",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		fuses = gl_find_objects(FL_NEW,FT_CLASS,SAME,"fuse",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		regulators = gl_find_objects(FL_NEW,FT_CLASS,SAME,"regulator",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		regConfs = gl_find_objects(FL_NEW,FT_CLASS,SAME,"regulator_configuration",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		lineConfs = gl_find_objects(FL_NEW,FT_CLASS,SAME,"line_configuration",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		tpLineConfs = gl_find_objects(FL_NEW,FT_CLASS,SAME,"triplex_line_configuration",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		TransConfsList = gl_find_objects(FL_NEW,FT_CLASS,SAME,"transformer_configuration",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		TransformerList = gl_find_objects(FL_NEW,FT_CLASS,SAME,"transformer",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		nodes = gl_find_objects(FL_NEW,FT_CLASS,SAME,"node",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		meters = gl_find_objects(FL_NEW,FT_CLASS,SAME,"meter",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);
		loads = gl_find_objects(FL_NEW,FT_CLASS,SAME,"load",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"powerflow",FT_END);

		inverters = gl_find_objects(FL_NEW,FT_CLASS,SAME,"inverter",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"generators",FT_END);
		diesels = gl_find_objects(FL_NEW,FT_CLASS,SAME,"diesel_dg",AND,FT_GROUPID,SAME,group.get_string(),AND,FT_MODULE,SAME,"generators",FT_END);
	}

	if ((ohlines->hit_count==0) && (tplines->hit_count==0) && (uglines->hit_count==0) && (nodes->hit_count==0) && (meters->hit_count==0) && (loads->hit_count==0))
	{
		gl_error("No line or bus objects were found.");
		/* TROUBLESHOOT
		No line or bus objects were found in your .glm file. if you specified a group id double check to make sure you have line objects with the specified group id.
		*/

		return FAILED;
	}

	//write style sheet info
	metrics_lines["$schema"] = "http://json-schema.org/draft-04/schema#";
	metrics_lines["description"] = "This file describes the system topology information (bus and lines) and line configuration data";

	// Define b_mat_pu and b_mat_tp_pu to store per unit bmatrix values
	if (lineConfs->hit_count > 0)
	{
		// Overhead and underground line configurations
		pLineConf = (OBJECT **)gl_malloc((lineConfs->hit_count)*sizeof(OBJECT*));

		//Check it
		if (pLineConf == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to allocate memory for one of the search and output arrays, an error was
			encountered.  Please try again.  If the error persists, please submit your code and a bug
			report via the ticketing system.
			*/
		}


		//Define b_mat_pu
		b_mat_pu = (gld::complex *)gl_malloc((lineConfs->hit_count)*9*sizeof(gld::complex));

		//Check it
		if (b_mat_pu == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Define b_mat_defined
		b_mat_defined = (bool *)gl_malloc((lineConfs->hit_count)*sizeof(bool));

		//Check it
		if (b_mat_defined == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Loop through and zero everything, just because
		for (indexA=0; indexA < (lineConfs->hit_count); indexA++)
		{
			pLineConf[indexA] = nullptr;
			b_mat_defined[indexA] = false;

			for (indexB=0; indexB<9; indexB++)
			{
				b_mat_pu[indexA*9+indexB] = gld::complex(0.0,0.0);
			}
		}

		//Pull the first line config
		obj_lineConf = gl_find_next(lineConfs,NULL);
		
		//Zero the index
		index = 0;
		
		//Loop
		while (obj_lineConf != nullptr)
		{
			pLineConf[index] = obj_lineConf;

			//increment
			index++;

			//Next object
			obj_lineConf = gl_find_next(lineConfs,obj_lineConf);
		}
	}
	else
	{
		pLineConf = nullptr;
		b_mat_defined = nullptr;
		b_mat_pu = nullptr;
	}

	// Triplex line configurations
	if (tpLineConfs->hit_count > 0)
	{
		pTpLineConf = (OBJECT **)gl_malloc((tpLineConfs->hit_count)*sizeof(OBJECT*));

		//Check it
		if (pTpLineConf == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Define b_mat_tp_pu
		b_mat_tp_pu = (gld::complex *)gl_malloc((tpLineConfs->hit_count)*9*sizeof(gld::complex));

		//Check it
		if (b_mat_tp_pu == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Define b_mat_tp_defined
		b_mat_tp_defined = (bool *)gl_malloc((tpLineConfs->hit_count)*sizeof(bool));

		//Check it
		if (b_mat_tp_defined == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}


		//Loop through and zero everything, just because
		for (indexA=0; indexA < (tpLineConfs->hit_count); indexA++)
		{
			pTpLineConf[indexA] = nullptr;
			b_mat_tp_defined[indexA] = false;

			for (indexB=0; indexB<9; indexB++)
			{
				b_mat_tp_pu[indexA*9+indexB] = gld::complex(0.0,0.0);
			}
		}

		//Pull the first line config
		obj_lineConf = gl_find_next(tpLineConfs,NULL);
		
		//Zero the index
		index = 0;
		
		//Loop
		while (obj_lineConf != nullptr)
		{
			pTpLineConf[index] = obj_lineConf;

			//increment
			index++;

			//Next object
			obj_lineConf = gl_find_next(tpLineConfs,obj_lineConf);
		}
	}
	else
	{
		pTpLineConf = nullptr;
		b_mat_tp_defined = nullptr;
		b_mat_tp_pu = nullptr;
	}

	//Transformer configurations
	if (TransConfsList->hit_count > 0)
	{
		// Define b_mat_pu and b_mat_tp_pu to store per unit bmatrix values
		// 
		pTransConf = (OBJECT **)gl_malloc((TransConfsList->hit_count)*sizeof(OBJECT*));

		//Check it
		if (pTransConf == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Define b_mat_trans_pu
		b_mat_trans_pu = (gld::complex *)gl_malloc((TransConfsList->hit_count)*9*sizeof(gld::complex));

		//Check it
		if (b_mat_trans_pu == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Zero it, to be safe
		for (indexA=0; indexA < (TransConfsList->hit_count*9); indexA++)
		{
			b_mat_trans_pu[indexA] = gld::complex(0.0,0.0);
		}

		//Define b_mat_trans_defined
		b_mat_trans_defined = (bool *)gl_malloc((TransConfsList->hit_count)*sizeof(bool));

		//Check it
		if (b_mat_trans_defined == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Zero it, to be safe
		for (indexA=0; indexA < (TransConfsList->hit_count); indexA++)
		{
			b_mat_trans_defined[indexA] = false;
		}

		//Allocate the phase count matrix
		trans_phase_count = (int *)gl_malloc((TransConfsList->hit_count)*sizeof(int));

		//Make sure it worked
		if (trans_phase_count == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Zero it
		for (indexA=0; indexA < (TransConfsList->hit_count); indexA++)
		{
			trans_phase_count[indexA] = 0;
		}

		//Grab the first object
		obj_lineConf = gl_find_next(TransConfsList,NULL);

		index=0;
		while(obj_lineConf != nullptr)
		{
			pTransConf[index] = obj_lineConf;
			index++;

			//Grab the next one
			obj_lineConf = gl_find_next(TransConfsList,obj_lineConf);
		}
	}
	else //No transformers, just NULL everything
	{
		pTransConf = nullptr;
		b_mat_trans_pu = nullptr;
		b_mat_trans_defined = nullptr;
		trans_phase_count = nullptr;
	}

	//Regulator configurations
	if (regConfs->hit_count > 0)
	{
		// Define b_mat_pu and b_mat_tp_pu to store per unit bmatrix values
		// 
		pRegConf = (OBJECT **)gl_malloc((regConfs->hit_count)*sizeof(OBJECT*));

		//Check it
		if (pRegConf == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Define b_mat_trans_pu
		b_mat_reg_pu = (gld::complex *)gl_malloc((regConfs->hit_count)*9*sizeof(gld::complex));

		//Check it
		if (b_mat_reg_pu == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Zero it, to be safe
		for (indexA=0; indexA < (regConfs->hit_count*9); indexA++)
		{
			b_mat_reg_pu[indexA] = gld::complex(0.0,0.0);
		}

		//Define b_mat_trans_defined
		b_mat_reg_defined = (bool *)gl_malloc((regConfs->hit_count)*sizeof(bool));

		//Check it
		if (b_mat_reg_defined == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Zero it, to be safe
		for (indexA=0; indexA < (regConfs->hit_count); indexA++)
		{
			b_mat_reg_defined[indexA] = false;
		}

		//Allocate the phase count matrix
		reg_phase_count = (int *)gl_malloc((regConfs->hit_count)*sizeof(int));

		//Make sure it worked
		if (reg_phase_count == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Zero it
		for (indexA=0; indexA < (regConfs->hit_count); indexA++)
		{
			reg_phase_count[indexA] = 0;
		}

		//Grab the first object
		obj_lineConf = gl_find_next(regConfs,NULL);

		index=0;
		while(obj_lineConf != nullptr)
		{
			pRegConf[index] = obj_lineConf;
			index++;

			//Grab the next one
			obj_lineConf = gl_find_next(regConfs,obj_lineConf);
		}
	}
	else //No transformers, just NULL everything
	{
		pRegConf = nullptr;
		b_mat_reg_pu = nullptr;
		b_mat_reg_defined = nullptr;
		reg_phase_count = nullptr;
	}

	// clear jsonArray - just in case
	jsonArray.clear();

	//Clear the node array too -- just in case
	node_object.clear();

	//Do generators - start with inverters
	if (inverters->hit_count > 0)
	{
		//Get the first one
		obj = gl_find_next(inverters,NULL);

		//Loop until done
		while (obj != nullptr)
		{
			//Write out the name
			if (obj->name != nullptr)
			{
				node_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"inverter:%d",obj->id);
				node_object["id"] = buffer;
			}

			//Write out our connection's name
			if (obj->parent->name != nullptr)
			{
				node_object["node_id"] = obj->parent->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"%s:%d",obj->parent->oclass->name,obj->parent->id);
				node_object["node_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			node_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//Extract the power values
			temp_complex_power_value[0] = get_complex_value(obj,"power_A");
			temp_complex_power_value[1] = get_complex_value(obj,"power_B");
			temp_complex_power_value[2] = get_complex_value(obj,"power_C");

			//See if we need to be per-unitized
			if (write_per_unit)
			{
				temp_complex_power_value[0] /= (system_VA_base / 3.0);
				temp_complex_power_value[1] /= (system_VA_base / 3.0);
				temp_complex_power_value[2] /= (system_VA_base / 3.0);
			}

			//Write out the real value
			jsonArray2.append(temp_complex_power_value[0].Re());
			jsonArray2.append(temp_complex_power_value[1].Re());
			jsonArray2.append(temp_complex_power_value[2].Re());

			//Write it
			node_object["max_real_phase"] = jsonArray2;

			//clear it
			jsonArray2.clear();

			//Now do reactive power
			//Write out the real value
			jsonArray2.append(temp_complex_power_value[0].Im());
			jsonArray2.append(temp_complex_power_value[1].Im());
			jsonArray2.append(temp_complex_power_value[2].Im());

			//Write it
			node_object["max_reactive_phase"] = jsonArray2;

			//clear it
			jsonArray2.clear();

			//Default values that may be read in later (if populated)
			node_object["microgrid_cost"] = 1.0e30;
      		node_object["microgrid_fixed_cost"] = 0.0;
      		node_object["max_microgrid"] = 0.0;
      		node_object["is_new"] = false;

			//Add the object to the array
			jsonArray.append(node_object);

			//Clear the node
			node_object.clear();

			//Get the next one
			obj = gl_find_next(inverters,obj);
		}//End inverter findlist search
	}//End Inverters

	//Do diesels now too
	if (diesels->hit_count > 0)
	{
		//Get the first one
		obj = gl_find_next(diesels,NULL);

		//Loop until done
		while (obj != nullptr)
		{
			//Write out the name
			if (obj->name != nullptr)
			{
				node_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"diesel_dg:%d",obj->id);
				node_object["id"] = buffer;
			}

			//Write out our connection's name
			if (obj->parent->name != nullptr)
			{
				node_object["node_id"] = obj->parent->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"%s:%d",obj->parent->oclass->name,obj->parent->id);
				node_object["node_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			node_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//Extract the power values
			temp_complex_power_value[0] = get_complex_value(obj,"power_out_A");
			temp_complex_power_value[1] = get_complex_value(obj,"power_out_B");
			temp_complex_power_value[2] = get_complex_value(obj,"power_out_C");

			//See if we need to be per-unitized
			if (write_per_unit)
			{
				temp_complex_power_value[0] /= (system_VA_base / 3.0);
				temp_complex_power_value[1] /= (system_VA_base / 3.0);
				temp_complex_power_value[2] /= (system_VA_base / 3.0);
			}

			//Write out the real value
			jsonArray2.append(temp_complex_power_value[0].Re());
			jsonArray2.append(temp_complex_power_value[1].Re());
			jsonArray2.append(temp_complex_power_value[2].Re());

			//Write it
			node_object["max_real_phase"] = jsonArray2;

			//clear it
			jsonArray2.clear();

			//Now do reactive power
			//Write out the real value
			jsonArray2.append(temp_complex_power_value[0].Im());
			jsonArray2.append(temp_complex_power_value[1].Im());
			jsonArray2.append(temp_complex_power_value[2].Im());

			//Write it
			node_object["max_reactive_phase"] = jsonArray2;

			//clear it
			jsonArray2.clear();

			//Default values that may be read in later (if populated)
			node_object["microgrid_cost"] = 1.0e30;
      		node_object["microgrid_fixed_cost"] = 0.0;
      		node_object["max_microgrid"] = 0.0;
      		node_object["is_new"] = false;

			//Add the object to the array
			jsonArray.append(node_object);

			//Clear the node
			node_object.clear();

			//Get the next one
			obj = gl_find_next(diesels,obj);
		}//End diesel findlist search
	}//End diesels

	//Write the values to the overall JSON
	metrics_lines["properties"]["generators"] = jsonArray;

	//Clear the array and object
	node_object.clear();
	jsonArray.clear();

	//Write nodes
	if (nodes->hit_count > 0)
	{
		//Find the first one
		obj = gl_find_next(nodes,NULL);

		//Loop through nodes list
		while (obj != nullptr)
		{
			//If per-unit - adjust the values
			if (write_per_unit)
			{
				temp_voltage_base = get_double_value(obj,"nominal_voltage");
				temp_de_pu_base = 1.0;	//Don't need to "de-per-unit it"
			}//End per-unit
			else	//No per-unit desired
			{
				temp_voltage_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
				temp_de_pu_base = get_double_value(obj,"nominal_voltage");	//But we do need to get the "real value"
			}

			//Write out the name
			if (obj->name != nullptr)
			{
				node_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"node:%d",obj->id);
				node_object["id"] = buffer;
			}

			//Write min and max voltage values
			node_object["min_voltage"] = min_volt_value * temp_de_pu_base;
			node_object["max_voltage"] = max_volt_value * temp_de_pu_base;

			//Obtain current voltage - assume that's the reference - per-unitize, if necessary
			temp_complex_voltage_value[0] = get_complex_value(obj,"voltage_A");
			temp_complex_voltage_value[1] = get_complex_value(obj,"voltage_B");
			temp_complex_voltage_value[2] = get_complex_value(obj,"voltage_C");

			//Now write it
			jsonArray2.clear();

			for (indexA=0; indexA<3; indexA++)
			{
				//Magnitude and "per-unitize" it, as necessary
				temp_voltage_output_value = temp_complex_voltage_value[indexA].Mag() / temp_voltage_base;

				//Append to the JSON array
				jsonArray2.append(temp_voltage_output_value);
			}
			
			//Actually output it
			node_object["ref_voltage"] = jsonArray2;
			jsonArray2.clear();

			//Check phases
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			node_object["has_phase"] = jsonArray2;
			jsonArray2.clear();
			
			// Append to node array
			jsonArray.append(node_object);

			// clear JSON value
			node_object.clear();

			//Get the next object in the list
			obj = gl_find_next(nodes,obj);
			
		}//End of node list traversion
	}//End of nodes non-empty

	//Search for meters too - replicate the nodes list, since we'll just leave them the same way
	if (meters->hit_count > 0)
	{
		//Find the first one
		obj = gl_find_next(meters,NULL);

		//Loop through meters list
		while (obj != nullptr)
		{
			//If per-unit - adjust the values
			if (write_per_unit)
			{
				temp_voltage_base = get_double_value(obj,"nominal_voltage");
				temp_de_pu_base = 1.0;	//Don't need to "de-per-unit it"
			}//End per-unit
			else	//No per-unit desired
			{
				temp_voltage_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
				temp_de_pu_base = get_double_value(obj,"nominal_voltage");	//But we do need to get the "real value"
			}

			//Write out the name
			if (obj->name != nullptr)
			{
				node_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"meter:%d",obj->id);
				node_object["id"] = buffer;
			}

			//Write min and max voltage values
			node_object["min_voltage"] = min_volt_value * temp_de_pu_base;
			node_object["max_voltage"] = max_volt_value * temp_de_pu_base;

			//Obtain current voltage - assume that's the reference - per-unitize, if necessary
			temp_complex_voltage_value[0] = get_complex_value(obj,"voltage_A");
			temp_complex_voltage_value[1] = get_complex_value(obj,"voltage_B");
			temp_complex_voltage_value[2] = get_complex_value(obj,"voltage_C");

			//Now write it
			jsonArray2.clear();

			for (indexA=0; indexA<3; indexA++)
			{
				//Magnitude and "per-unitize" it, as necessary
				temp_voltage_output_value = temp_complex_voltage_value[indexA].Mag() / temp_voltage_base;

				//Append to the JSON array
				jsonArray2.append(temp_voltage_output_value);
			}
			
			//Actually output it
			node_object["ref_voltage"] = jsonArray2;
			jsonArray2.clear();

			//Check phases
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			node_object["has_phase"] = jsonArray2;
			jsonArray2.clear();
			
			// Append to node array
			jsonArray.append(node_object);

			// clear JSON value
			node_object.clear();

			//Get the next object in the list
			obj = gl_find_next(meters,obj);
			
		}//End of meter list traversion
	}//End of meters non-empty

	//Clear load-related arrays and objects
	load_object.clear();
	jsonSecondArray.clear();

	//Search for loads too - replicate the nodes list, since we'll just leave them the same way
	if (loads->hit_count > 0)
	{
		//Find the first one
		obj = gl_find_next(loads,NULL);

		//Loop through loads list
		while (obj != nullptr)
		{
			//If per-unit - adjust the values
			if (write_per_unit)
			{
				temp_voltage_base = get_double_value(obj,"nominal_voltage");
				temp_de_pu_base = 1.0;	//Don't need to "de-per-unit it"
			}//End per-unit
			else	//No per-unit desired
			{
				temp_voltage_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
				temp_de_pu_base = get_double_value(obj,"nominal_voltage");	//But we do need to get the "real value"
			}

			//Write out the name
			if (obj->name != nullptr)
			{
				node_object["id"] = obj->name;

				//Load array portions
				load_object["node_id"] = obj->name;

				sprintf(buffer,"load_%s",obj->name);
				load_object["id"] = buffer;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"load:%d",obj->id);
				node_object["id"] = buffer;

				//Load array portions
				load_object["node_id"] = buffer;

				sprintf(buffer,"load_object_%d",obj->id);
				load_object["id"] = buffer;
			}

			//Pull the enumeration value for the load - see if we're critical or not
			temp_enum_value = get_enum_value(obj,"load_priority");

			//See if we're "appropriately critical"
			if (temp_enum_value == 2)
			{
				load_object["is_critical"] = true;
			}
			else
			{
				load_object["is_critical"] = false;
			}

			//Write min and max voltage values
			node_object["min_voltage"] = min_volt_value * temp_de_pu_base;
			node_object["max_voltage"] = max_volt_value * temp_de_pu_base;

			//Obtain current voltage - assume that's the reference - per-unitize, if necessary
			temp_complex_voltage_value[0] = get_complex_value(obj,"voltage_A");
			temp_complex_voltage_value[1] = get_complex_value(obj,"voltage_B");
			temp_complex_voltage_value[2] = get_complex_value(obj,"voltage_C");

			//Now write it
			jsonArray2.clear();

			for (indexA=0; indexA<3; indexA++)
			{
				//Magnitude and "per-unitize" it, as necessary
				temp_voltage_output_value = temp_complex_voltage_value[indexA].Mag() / temp_voltage_base;

				//Append to the JSON array
				jsonArray2.append(temp_voltage_output_value);
			}
			
			//Actually output it
			node_object["ref_voltage"] = jsonArray2;
			jsonArray2.clear();

			//Check phases
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			node_object["has_phase"] = jsonArray2;
			load_object["has_phase"] = jsonArray2;
			jsonArray2.clear();
			
			//Now pull the load values - only constant power for now
			temp_complex_power_value[0] = get_complex_value(obj,"constant_power_A");
			temp_complex_power_value[1] = get_complex_value(obj,"constant_power_B");
			temp_complex_power_value[2] = get_complex_value(obj,"constant_power_C");

			//See if we need to be per-unitized
			if (write_per_unit)
			{
				temp_complex_power_value[0] /= (system_VA_base / 3.0);
				temp_complex_power_value[1] /= (system_VA_base / 3.0);
				temp_complex_power_value[2] /= (system_VA_base / 3.0);
			}

			//Write out the real value
			jsonArray2.append(temp_complex_power_value[0].Re());
			jsonArray2.append(temp_complex_power_value[1].Re());
			jsonArray2.append(temp_complex_power_value[2].Re());

			//Write it
			load_object["max_real_phase"] = jsonArray2;

			//clear it
			jsonArray2.clear();

			//Now do reactive power
			//Write out the real value
			jsonArray2.append(temp_complex_power_value[0].Im());
			jsonArray2.append(temp_complex_power_value[1].Im());
			jsonArray2.append(temp_complex_power_value[2].Im());

			//Write it
			load_object["max_reactive_phase"] = jsonArray2;

			//clear it
			jsonArray2.clear();

			// Append to node array
			jsonArray.append(node_object);

			// clear JSON value
			node_object.clear();

			//Do the same for the load value
			jsonSecondArray.append(load_object);

			//Clear the JSON value
			load_object.clear();

			//Get the next object in the list
			obj = gl_find_next(loads,obj);
			
		}//End of load list traversion
	}//End of loads non-empty

	// Write bus quantities metrics_lines dictionary
	metrics_lines["properties"]["buses"] = jsonArray;

	// clear jsonArray
	jsonArray.clear();

	//Now do loads -- print out their special properties

	// Write bus quantities metrics_lines dictionary
	metrics_lines["properties"]["loads"] = jsonSecondArray;

	// clear jsonSecondArray
	jsonSecondArray.clear();
	
	//write transformers
	index = 0;
	line_object.clear();

	if(TransformerList->hit_count > 0)
	{
		//Allocate the master array
		pTransformer = (transformer **)gl_malloc(TransformerList->hit_count*sizeof(transformer*));

		//Check it
		if (pTransformer == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Grab the first object
		obj = gl_find_next(TransformerList,NULL);

		//Zero the index
		index = 0;

		while(obj != nullptr)
		{
			if(index >= TransformerList->hit_count){
				break;
			}

			pTransformer[index] = OBJECTDATA(obj,transformer);
			if(pTransformer[index] == nullptr){
				gl_error("Unable to map object as transformer object.");
				return FAILED;
			}

			// Write the transformer metrics
			// Write the name (not id) - if it exists
			if (obj->name != nullptr)
			{
				line_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"transformer:%d",obj->id);
				line_object["id"] = buffer;
			}

			//write from node name
			if (pTransformer[index]->from->name != nullptr)
			{
				line_object["node1_id"] = pTransformer[index]->from->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pTransformer[index]->from->oclass->name,pTransformer[index]->from->id);
				line_object["node1_id"] = buffer;
			}

			//write to node name
			if (pTransformer[index]->to->name != nullptr)
			{
				line_object["node2_id"] = pTransformer[index]->to->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pTransformer[index]->to->oclass->name,pTransformer[index]->to->id);
				line_object["node2_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Get phase count too
			phaseCount = 0;
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			line_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//write capacity - I wanted to use link emergency limit (A), but its private field, write only default value
			line_object["capacity"] = 1e30;
			
			//write the length
			line_object["length"] = 1.0;

			//write the num_phases
			line_object["num_phases"] = phaseCount;

			//write is_transformer
			line_object["is_transformer"] = true;

			//If per-unit - adjust the values
			if (write_per_unit)
			{
				//Compute the per-unit base - use the nominal value off of the secondary
				per_unit_base = get_double_value(pTransformer[index]->to,"nominal_voltage");

				//Calculate the base impedance
				temp_impedance_base = (per_unit_base * per_unit_base) / (system_VA_base / 3.0);
			}//End per-unit
			else	//No per-unit desired
			{
				temp_impedance_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
			}

			//write line configuration, if we're unique
			found_match_config = false;
			for (indexA=0; indexA<TransConfsList->hit_count; indexA++)
			{
				//Just see if we're a match
				if (pTransformer[index]->configuration == pTransConf[indexA])
				{
					found_match_config = true;
					break;
				}
			}

			//See if it worked - if so, store us
			if (found_match_config)
			{
				if (!b_mat_trans_defined[indexA])
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						for (indexC = 0; indexC < 3; indexC++)
						{
							b_mat_trans_pu[indexA*9+indexB*3+indexC] = pTransformer[index]->b_mat[indexB][indexC]/temp_impedance_base;
						}
					}
					trans_phase_count[indexA] = phaseCount;
					b_mat_trans_defined[indexA] = true;
				}
			}

			//write line_code
			if (pTransformer[index]->configuration->name == nullptr)
			{
				sprintf(buffer,"trans_config:%d",pTransformer[index]->configuration->id);
				line_object["line_code"] = buffer;
			}
			else
			{
				line_object["line_code"] = pTransformer[index]->configuration->name;
			}

			//write construction cost - only default value
			line_object["construction_cost"] = 1e30;
			//write hardening cost - only default cost
			line_object["harden_cost"] = 1e30;
			//write switch cost - only default cost
			line_object["switch_cost"] = 1e30;
			//write flag of new construction - only default cost
			line_object["is_new"] = false;
			//write flag of harden - only default cost
			line_object["can_harden"] = false;
			//write flag of add switch - only default cost
			line_object["can_add_switch"] = false;
			//write flag of switch existence - only default cost
			line_object["has_switch"] = false;
			// End of line codes

			// Append to line array
			jsonArray.append(line_object);

			// clear JSON value
			line_object.clear();

			//Get next value
			obj = gl_find_next(TransformerList,obj);

			index++;
		}//End of transformer
	}

	//Regulators
	if(regulators->hit_count > 0)
	{
		//Allocate the master array
		pRegulator = (regulator **)gl_malloc(regulators->hit_count*sizeof(regulator*));

		//Check it
		if (pRegulator == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Grab the first object
		obj = gl_find_next(regulators,NULL);

		//Zero the index
		index = 0;

		while(obj != nullptr)
		{
			if(index >= regulators->hit_count){
				break;
			}

			pRegulator[index] = OBJECTDATA(obj,regulator);
			if(pRegulator[index] == nullptr){
				gl_error("Unable to map object as regulator object.");
				return FAILED;
			}

			// Write the regulator metrics
			// Write the name (not id) - if it exists
			if (obj->name != nullptr)
			{
				line_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"regulator:%d",obj->id);
				line_object["id"] = buffer;
			}

			//write from node name
			if (pRegulator[index]->from->name != nullptr)
			{
				line_object["node1_id"] = pRegulator[index]->from->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pRegulator[index]->from->oclass->name,pRegulator[index]->from->id);
				line_object["node1_id"] = buffer;
			}

			//write to node name
			if (pRegulator[index]->to->name != nullptr)
			{
				line_object["node2_id"] = pRegulator[index]->to->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pRegulator[index]->to->oclass->name,pRegulator[index]->to->id);
				line_object["node2_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Get phase count too
			phaseCount = 0;
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			line_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//write capacity - I wanted to use link emergency limit (A), but its private field, write only default value
			line_object["capacity"] = 1e30;
			
			//write the length
			line_object["length"] = 1.0;

			//write the num_phases
			line_object["num_phases"] = phaseCount;

			//write is_transformer
			line_object["is_transformer"] = true;

			//If per-unit - adjust the values
			if (write_per_unit)
			{
				//Compute the per-unit base - use the nominal value off of the secondary
				per_unit_base = get_double_value(pRegulator[index]->to,"nominal_voltage");

				//Calculate the base impedance
				temp_impedance_base = (per_unit_base * per_unit_base) / (system_VA_base / 3.0);
			}//End per-unit
			else	//No per-unit desired
			{
				temp_impedance_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
			}

			//write line configuration, if we're unique
			found_match_config = false;
			for (indexA=0; indexA<regConfs->hit_count; indexA++)
			{
				//Just see if we're a match
				if (pRegulator[index]->configuration == pRegConf[indexA])
				{
					found_match_config = true;
					break;
				}
			}

			//See if it worked - if so, store us
			if (found_match_config)
			{
				if (!b_mat_reg_defined[indexA])
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						for (indexC = 0; indexC < 3; indexC++)
						{
							b_mat_reg_pu[indexA*9+indexB*3+indexC] = pRegulator[index]->b_mat[indexB][indexC]/temp_impedance_base;
						}
					}
					reg_phase_count[indexA] = phaseCount;
					b_mat_reg_defined[indexA] = true;
				}
			}

			//write line_code
			if (pRegulator[index]->configuration->name == nullptr)
			{
				sprintf(buffer,"reg_config:%d",pRegulator[index]->configuration->id);
				line_object["line_code"] = buffer;
			}
			else
			{
				line_object["line_code"] = pRegulator[index]->configuration->name;
			}

			//write construction cost - only default value
			line_object["construction_cost"] = 1e30;
			//write hardening cost - only default cost
			line_object["harden_cost"] = 1e30;
			//write switch cost - only default cost
			line_object["switch_cost"] = 1e30;
			//write flag of new construction - only default cost
			line_object["is_new"] = false;
			//write flag of harden - only default cost
			line_object["can_harden"] = false;
			//write flag of add switch - only default cost
			line_object["can_add_switch"] = false;
			//write flag of switch existence - only default cost
			line_object["has_switch"] = false;
			// End of line codes

			// Append to line array
			jsonArray.append(line_object);

			// clear JSON value
			line_object.clear();

			//Get next value
			obj = gl_find_next(regulators,obj);

			index++;
		}//End of regulator
	}

	//write the overhead_lines
	if(ohlines->hit_count > 0)
	{
		pOhLine = (line **)gl_malloc(ohlines->hit_count*sizeof(line*));

		//Check it
		if (pOhLine == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Grab the first object
		obj = gl_find_next(ohlines,NULL);

		//Zero the index
		index = 0;

		while(obj != nullptr)
		{
			if(index >= ohlines->hit_count){
				break;
			}

			pOhLine[index] = OBJECTDATA(obj,line);
			if(pOhLine[index] == nullptr){
				gl_error("Unable to map object as overhead_line object.");
				return FAILED;
			}

			// Write the overhead_line metrics
			// Write the name (not id) - if it exists
			if (obj->name != nullptr)
			{
				line_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"overhead_line:%d",obj->id);
				line_object["id"] = buffer;
			}

			//write from node name
			if (pOhLine[index]->from->name != nullptr)
			{
				line_object["node1_id"] = pOhLine[index]->from->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pOhLine[index]->from->oclass->name,pOhLine[index]->from->id);
				line_object["node1_id"] = buffer;
			}

			//write to node name
			if (pOhLine[index]->to->name != nullptr)
			{
				line_object["node2_id"] = pOhLine[index]->to->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pOhLine[index]->to->oclass->name,pOhLine[index]->to->id);
				line_object["node2_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Get phase count too
			phaseCount = 0;
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			line_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//write capacity - I wanted to use link emergency limit (A), but its private field, write only default value
			line_object["capacity"] = 1e30;
			
			//write the length
			line_object["length"] = pOhLine[index]->length;
			
			//write the num_phases
			line_object["num_phases"] = phaseCount;

			//write is_transformer (now only write line object, so will only be false)
			line_object["is_transformer"] = false;

			//If per-unit - adjust the values
			if (write_per_unit)
			{
				//Compute the per-unit base - use the nominal value off of the secondary
				per_unit_base = get_double_value(pOhLine[index]->to,"nominal_voltage");

				//Calculate the base impedance
				temp_impedance_base = (per_unit_base * per_unit_base) / (system_VA_base / 3.0);
			}//End per-unit
			else	//No per-unit desired
			{
				temp_impedance_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
			}

			//write line configuration, if we're unique
			found_match_config = false;
			for (indexA=0; indexA<lineConfs->hit_count; indexA++)
			{
				//Just see if we're a match
				if (pOhLine[index]->configuration == pLineConf[indexA])
				{
					found_match_config = true;
					break;
				}
			}

			//See if it worked - if so, store us
			if (found_match_config)
			{
				if (!b_mat_defined[indexA])
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						for (indexC = 0; indexC < 3; indexC++)
						{
							b_mat_pu[indexA*9+indexB*3+indexC] = (pOhLine[index]->b_mat[indexB][indexC])/(((pOhLine[index]->length)/5280.0)/temp_impedance_base);
						}
					}
					b_mat_defined[indexA] = true;
				}
			}

			//write line_code
			if (pOhLine[index]->configuration->name == nullptr)
			{
				sprintf(buffer,"line_config:%d",pOhLine[index]->configuration->id);
				line_object["line_code"] = buffer;
			}
			else
			{
				line_object["line_code"] = pOhLine[index]->configuration->name;
			}

			//write construction cost - only default value
			line_object["construction_cost"] = 1e30;
			//write hardening cost - only default cost
			line_object["harden_cost"] = 1e30;
			//write switch cost - only default cost
			line_object["switch_cost"] = 1e30;
			//write flag of new construction - only default cost
			line_object["is_new"] = false;
			//write flag of harden - only default cost
			line_object["can_harden"] = false;
			//write flag of add switch - only default cost
			line_object["can_add_switch"] = false;
			//write flag of switch existence - only default cost
			line_object["has_switch"] = false;
			// End of line codes

			// Append to line array
			jsonArray.append(line_object);

			// clear JSON value
			line_object.clear();

			//Get next value
			obj = gl_find_next(ohlines,obj);

			index++;
		}
	}

	//write the underground lines
	if(uglines->hit_count > 0)
	{
		pUgLine = (line **)gl_malloc(uglines->hit_count*sizeof(line*));

		//Check it
		if (pUgLine == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Grab the first object
		obj = gl_find_next(uglines,NULL);

		//Zero the index
		index = 0;

		while(obj != nullptr)
		{
			if(index >= uglines->hit_count){
				break;
			}

			pUgLine[index] = OBJECTDATA(obj,line);
			if(pUgLine[index] == nullptr){
				gl_error("Unable to map object as underground_line object.");
				return FAILED;
			}

			// Write the underground_line metrics
			// Write the name (not id) - if it exists
			if (obj->name != nullptr)
			{
				line_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"underground_line:%d",obj->id);
				line_object["id"] = buffer;
			}

			//write from node name
			if (pUgLine[index]->from->name != nullptr)
			{
				line_object["node1_id"] = pUgLine[index]->from->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pUgLine[index]->from->oclass->name,pUgLine[index]->from->id);
				line_object["node1_id"] = buffer;
			}

			//write to node name
			if (pUgLine[index]->to->name != nullptr)
			{
				line_object["node2_id"] = pUgLine[index]->to->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pUgLine[index]->to->oclass->name,pUgLine[index]->to->id);
				line_object["node2_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Get phase count too
			phaseCount = 0;
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			line_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//write capacity - I wanted to use link emergency limit (A), but its private field, write only default value
			line_object["capacity"] = 1e30;
			
			//write the length
			line_object["length"] = pUgLine[index]->length;
			
			//write the num_phases
			line_object["num_phases"] = phaseCount;

			//write is_transformer (now only write line object, so will only be false)
			line_object["is_transformer"] = false;

			//If per-unit - adjust the values
			if (write_per_unit)
			{
				//Compute the per-unit base - use the nominal value off of the secondary
				per_unit_base = get_double_value(pUgLine[index]->to,"nominal_voltage");

				//Calculate the base impedance
				temp_impedance_base = (per_unit_base * per_unit_base) / (system_VA_base / 3.0);
			}//End per-unit
			else	//No per-unit desired
			{
				temp_impedance_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
			}

			//write line configuration, if we're unique
			found_match_config = false;
			for (indexA=0; indexA<lineConfs->hit_count; indexA++)
			{
				//Just see if we're a match
				if (pUgLine[index]->configuration == pLineConf[indexA])
				{
					found_match_config = true;
					break;
				}
			}

			//See if it worked - if so, store us
			if (found_match_config)
			{
				if (!b_mat_defined[indexA])
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						for (indexC = 0; indexC < 3; indexC++)
						{
							b_mat_pu[indexA*9+indexB*3+indexC] = (pUgLine[index]->b_mat[indexB][indexC])/(((pUgLine[index]->length)/5280.0)/temp_impedance_base);
						}
					}
					b_mat_defined[indexA] = true;
				}
			}

			//write line_code
			if (pUgLine[index]->configuration->name == nullptr)
			{
				sprintf(buffer,"line_config:%d",pUgLine[index]->configuration->id);
				line_object["line_code"] = buffer;
			}
			else
			{
				line_object["line_code"] = pUgLine[index]->configuration->name;
			}

			//write construction cost - only default value
			line_object["construction_cost"] = 1e30;
			//write hardening cost - only default cost
			line_object["harden_cost"] = 1e30;
			//write switch cost - only default cost
			line_object["switch_cost"] = 1e30;
			//write flag of new construction - only default cost
			line_object["is_new"] = false;
			//write flag of harden - only default cost
			line_object["can_harden"] = false;
			//write flag of add switch - only default cost
			line_object["can_add_switch"] = false;
			//write flag of switch existence - only default cost
			line_object["has_switch"] = false;
			// End of line codes

			// Append to line array
			jsonArray.append(line_object);

			// clear JSON value
			line_object.clear();

			//Get next value
			obj = gl_find_next(uglines,obj);

			index++;
		}
	}//End UgLines traversion

	//write the triplex lines
	if(tplines->hit_count > 0)
	{
		pTpLine = (line **)gl_malloc(tplines->hit_count*sizeof(line*));

		//Check it
		if (pTpLine == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Grab the first object
		obj = gl_find_next(tplines,NULL);

		//Zero the index
		index = 0;

		while(obj != nullptr)
		{
			if(index >= tplines->hit_count){
				break;
			}

			pTpLine[index] = OBJECTDATA(obj,line);
			if(pTpLine[index] == nullptr){
				gl_error("Unable to map object as triplex_line object.");
				return FAILED;
			}

			// Write the triplex_line metrics
			// Write the name (not id) - if it exists
			if (obj->name != nullptr)
			{
				line_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"triplex_line:%d",obj->id);
				line_object["id"] = buffer;
			}

			//write from node name
			if (pTpLine[index]->from->name != nullptr)
			{
				line_object["node1_id"] = pTpLine[index]->from->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pTpLine[index]->from->oclass->name,pTpLine[index]->from->id);
				line_object["node1_id"] = buffer;
			}

			//write to node name
			if (pTpLine[index]->to->name != nullptr)
			{
				line_object["node2_id"] = pTpLine[index]->to->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pTpLine[index]->to->oclass->name,pTpLine[index]->to->id);
				line_object["node2_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Get phase count too
			phaseCount = 0;
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			line_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//write capacity - I wanted to use link emergency limit (A), but its private field, write only default value
			line_object["capacity"] = 1e30;
			
			//write the length
			line_object["length"] = pTpLine[index]->length;
			
			//write the num_phases
			line_object["num_phases"] = phaseCount;

			//write is_transformer (now only write line object, so will only be false)
			line_object["is_transformer"] = false;

			//If per-unit - adjust the values
			if (write_per_unit)
			{
				//Compute the per-unit base - use the nominal value off of the secondary
				per_unit_base = get_double_value(pTpLine[index]->to,"nominal_voltage");

				//Calculate the base impedance
				temp_impedance_base = (per_unit_base * per_unit_base) / (system_VA_base / 3.0);
			}//End per-unit
			else	//No per-unit desired
			{
				temp_impedance_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
			}

			//write line configuration, if we're unique
			found_match_config = false;
			for (indexA=0; indexA<lineConfs->hit_count; indexA++)
			{
				//Just see if we're a match
				if (pTpLine[index]->configuration == pTpLineConf[indexA])
				{
					found_match_config = true;
					break;
				}
			}

			//See if it worked - if so, store us
			if (found_match_config)
			{
				if (!b_mat_tp_defined[indexA])
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						for (indexC = 0; indexC < 3; indexC++)
						{
							b_mat_tp_pu[indexA*9+indexB*3+indexC] = (pTpLine[index]->b_mat[indexB][indexC])/(((pTpLine[index]->length)/5280.0)/temp_impedance_base);
						}
					}
					b_mat_tp_defined[indexA] = true;
				}
			}

			//write line_code
			if (pTpLine[index]->configuration->name == nullptr)
			{
				sprintf(buffer,"line_config:%d",pTpLine[index]->configuration->id);
				line_object["line_code"] = buffer;
			}
			else
			{
				line_object["line_code"] = pTpLine[index]->configuration->name;
			}

			//write construction cost - only default value
			line_object["construction_cost"] = 1e30;
			//write hardening cost - only default cost
			line_object["harden_cost"] = 1e30;
			//write switch cost - only default cost
			line_object["switch_cost"] = 1e30;
			//write flag of new construction - only default cost
			line_object["is_new"] = false;
			//write flag of harden - only default cost
			line_object["can_harden"] = false;
			//write flag of add switch - only default cost
			line_object["can_add_switch"] = false;
			//write flag of switch existence - only default cost
			line_object["has_switch"] = false;
			// End of line codes

			// Append to line array
			jsonArray.append(line_object);

			// clear JSON value
			line_object.clear();

			//Get next value
			obj = gl_find_next(tplines,obj);

			index++;
		}
	}

	//Initial value for swtiches
	b_mat_switch_defined = false;

	//Write switches
	if(switches->hit_count > 0)
	{

		//Allocate the master array
		pSwitch = (switch_object **)gl_malloc(switches->hit_count*sizeof(switch_object*));

		//Check it
		if (pSwitch == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Grab the first object
		obj = gl_find_next(switches,NULL);

		//Zero the index
		index = 0;

		while(obj != nullptr)
		{
			if(index >= switches->hit_count){
				break;
			}

			pSwitch[index] = OBJECTDATA(obj,switch_object);
			if(pSwitch[index] == nullptr){
				gl_error("Unable to map object as switch object.");
				return FAILED;
			}

			// Write the switch metrics
			// Write the name (not id) - if it exists
			if (obj->name != nullptr)
			{
				line_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"switch:%d",obj->id);
				line_object["id"] = buffer;
			}

			//write from node name
			if (pSwitch[index]->from->name != nullptr)
			{
				line_object["node1_id"] = pSwitch[index]->from->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pSwitch[index]->from->oclass->name,pSwitch[index]->from->id);
				line_object["node1_id"] = buffer;
			}

			//write to node name
			if (pSwitch[index]->to->name != nullptr)
			{
				line_object["node2_id"] = pSwitch[index]->to->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pSwitch[index]->to->oclass->name,pSwitch[index]->to->id);
				line_object["node2_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj, "phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Get phase count too
			phaseCount = 0;
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			line_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//write capacity - I wanted to use link emergency limit (A), but its private field, write only default value
			line_object["capacity"] = 1e30;
			
			//write the length
			line_object["length"] = 1.0;

			//write the num_phases
			line_object["num_phases"] = phaseCount;

			//write is_transformer
			line_object["is_transformer"] = false;

			//If per-unit - adjust the values
			if (write_per_unit)
			{
				//Compute the per-unit base - use the nominal value off of the secondary
				per_unit_base = get_double_value(pSwitch[index]->to,"nominal_voltage");

				//Calculate the base impedance
				temp_impedance_base = (per_unit_base * per_unit_base) / (system_VA_base / 3.0);
			}//End per-unit
			else	//No per-unit desired
			{
				temp_impedance_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
			}

			//write line configuration, if we're unique
			if (!b_mat_switch_defined)
			{
				for (indexB = 0; indexB < 3; indexB++)
				{
					for (indexC = 0; indexC < 3; indexC++)
					{
						b_mat_switch_pu[indexB*3+indexC] = pSwitch[index]->b_mat[indexB][indexC]/temp_impedance_base;
					}
				}
				b_mat_switch_defined = true;
				switch_phase_count = phaseCount;	//Just store the first one
			}

			//write line_code
			line_object["line_code"] = "switch_config";

			//write construction cost - only default value
			line_object["construction_cost"] = 1e30;
			//write hardening cost - only default cost
			line_object["harden_cost"] = 1e30;
			//write switch cost - only default cost
			line_object["switch_cost"] = 1e30;
			//write flag of new construction - only default cost
			line_object["is_new"] = false;
			//write flag of harden - only default cost
			line_object["can_harden"] = false;
			//write flag of add switch - only default cost
			line_object["can_add_switch"] = false;
			//write flag of switch existence - only default cost
			line_object["has_switch"] = true;
			// End of line codes

			// Append to line array
			jsonArray.append(line_object);

			// clear JSON value
			line_object.clear();

			//Get next value
			obj = gl_find_next(switches,obj);

			index++;
		}//End of switches
	}

	//Write sectionalizers
	if(sectionalizers->hit_count > 0)
	{
		//Allocate the master array
		pSectionalizer = (sectionalizer **)gl_malloc(sectionalizers->hit_count*sizeof(sectionalizer*));

		//Check it
		if (pSectionalizer == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Grab the first object
		obj = gl_find_next(sectionalizers,NULL);

		//Zero the index
		index = 0;

		while(obj != nullptr)
		{
			if(index >= sectionalizers->hit_count){
				break;
			}

			pSectionalizer[index] = OBJECTDATA(obj,sectionalizer);
			if(pSectionalizer[index] == nullptr){
				gl_error("Unable to map object as sectionalizer object.");
				return FAILED;
			}

			// Write the sectionalizer metrics
			// Write the name (not id) - if it exists
			if (obj->name != nullptr)
			{
				line_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"sectionalizer:%d",obj->id);
				line_object["id"] = buffer;
			}

			//write from node name
			if (pSectionalizer[index]->from->name != nullptr)
			{
				line_object["node1_id"] = pSectionalizer[index]->from->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pSectionalizer[index]->from->oclass->name,pSectionalizer[index]->from->id);
				line_object["node1_id"] = buffer;
			}

			//write to node name
			if (pSectionalizer[index]->to->name != nullptr)
			{
				line_object["node2_id"] = pSectionalizer[index]->to->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pSectionalizer[index]->to->oclass->name,pSectionalizer[index]->to->id);
				line_object["node2_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Get phase count too
			phaseCount = 0;
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			line_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//write capacity - I wanted to use link emergency limit (A), but its private field, write only default value
			line_object["capacity"] = 1e30;
			
			//write the length
			line_object["length"] = 1.0;

			//write the num_phases
			line_object["num_phases"] = phaseCount;

			//write is_transformer
			line_object["is_transformer"] = false;

			//If per-unit - adjust the values
			if (write_per_unit)
			{
				//Compute the per-unit base - use the nominal value off of the secondary
				per_unit_base = get_double_value(pSectionalizer[index]->to,"nominal_voltage");

				//Calculate the base impedance
				temp_impedance_base = (per_unit_base * per_unit_base) / (system_VA_base / 3.0);
			}//End per-unit
			else	//No per-unit desired
			{
				temp_impedance_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
			}

			//write line configuration, if we're unique
			if (!b_mat_switch_defined)
			{
				for (indexB = 0; indexB < 3; indexB++)
				{
					for (indexC = 0; indexC < 3; indexC++)
					{
						b_mat_switch_pu[indexB*3+indexC] = pSectionalizer[index]->b_mat[indexB][indexC]/temp_impedance_base;
					}
				}
				b_mat_switch_defined = true;
				switch_phase_count = phaseCount;	//Just store the first one
			}

			//write line_code
			line_object["line_code"] = "switch_config";

			//write construction cost - only default value
			line_object["construction_cost"] = 1e30;
			//write hardening cost - only default cost
			line_object["harden_cost"] = 1e30;
			//write switch cost - only default cost
			line_object["switch_cost"] = 1e30;
			//write flag of new construction - only default cost
			line_object["is_new"] = false;
			//write flag of harden - only default cost
			line_object["can_harden"] = false;
			//write flag of add switch - only default cost
			line_object["can_add_switch"] = false;
			//write flag of switch existence - only default cost
			line_object["has_switch"] = true;
			// End of line codes

			// Append to line array
			jsonArray.append(line_object);

			// clear JSON value
			line_object.clear();

			//Get next value
			obj = gl_find_next(sectionalizers,obj);

			index++;
		}//End of sectionalizers
	}

	//Write reclosers
	if(reclosers->hit_count > 0)
	{

		//Allocate the master array
		pRecloser = (recloser **)gl_malloc(reclosers->hit_count*sizeof(recloser*));

		//Check it
		if (pRecloser == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Grab the first object
		obj = gl_find_next(reclosers,NULL);

		//Zero the index
		index = 0;

		while(obj != nullptr)
		{
			if(index >= reclosers->hit_count){
				break;
			}

			pRecloser[index] = OBJECTDATA(obj,recloser);
			if(pRecloser[index] == nullptr){
				gl_error("Unable to map object as recloser object.");
				return FAILED;
			}

			// Write the recloser metrics
			// Write the name (not id) - if it exists
			if (obj->name != nullptr)
			{
				line_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"recloser:%d",obj->id);
				line_object["id"] = buffer;
			}

			//write from node name
			if (pRecloser[index]->from->name != nullptr)
			{
				line_object["node1_id"] = pRecloser[index]->from->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pRecloser[index]->from->oclass->name,pRecloser[index]->from->id);
				line_object["node1_id"] = buffer;
			}

			//write to node name
			if (pRecloser[index]->to->name != nullptr)
			{
				line_object["node2_id"] = pRecloser[index]->to->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pRecloser[index]->to->oclass->name,pRecloser[index]->to->id);
				line_object["node2_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Get phase count too
			phaseCount = 0;
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			line_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//write capacity - I wanted to use link emergency limit (A), but its private field, write only default value
			line_object["capacity"] = 1e30;
			
			//write the length
			line_object["length"] = 1.0;

			//write the num_phases
			line_object["num_phases"] = phaseCount;

			//write is_transformer
			line_object["is_transformer"] = false;

			//If per-unit - adjust the values
			if (write_per_unit)
			{
				//Compute the per-unit base - use the nominal value off of the secondary
				per_unit_base = get_double_value(pRecloser[index]->to,"nominal_voltage");

				//Calculate the base impedance
				temp_impedance_base = (per_unit_base * per_unit_base) / (system_VA_base / 3.0);
			}//End per-unit
			else	//No per-unit desired
			{
				temp_impedance_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
			}

			//write line configuration, if we're unique
			if (!b_mat_switch_defined)
			{
				for (indexB = 0; indexB < 3; indexB++)
				{
					for (indexC = 0; indexC < 3; indexC++)
					{
						b_mat_switch_pu[indexB*3+indexC] = pRecloser[index]->b_mat[indexB][indexC]/temp_impedance_base;
					}
				}
				b_mat_switch_defined = true;
				switch_phase_count = phaseCount;	//Just store the first one
			}

			//write line_code
			line_object["line_code"] = "switch_config";

			//write construction cost - only default value
			line_object["construction_cost"] = 1e30;
			//write hardening cost - only default cost
			line_object["harden_cost"] = 1e30;
			//write switch cost - only default cost
			line_object["switch_cost"] = 1e30;
			//write flag of new construction - only default cost
			line_object["is_new"] = false;
			//write flag of harden - only default cost
			line_object["can_harden"] = false;
			//write flag of add switch - only default cost
			line_object["can_add_switch"] = false;
			//write flag of switch existence - only default cost
			line_object["has_switch"] = true;
			// End of line codes

			// Append to line array
			jsonArray.append(line_object);

			// clear JSON value
			line_object.clear();

			//Get next value
			obj = gl_find_next(reclosers,obj);

			index++;
		}//End of reclosers
	}

	//Initial value for fuses
	b_mat_fuse_defined = false;

	//Write fuses
	if(fuses->hit_count > 0)
	{

		//Allocate the master array
		pFuse = (fuse **)gl_malloc(fuses->hit_count*sizeof(fuse*));

		//Check it
		if (pFuse == nullptr)
		{
			GL_THROW("jsdondump:%d %s - Unable to allocate memory",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"));
			//Defined above
		}

		//Grab the first object
		obj = gl_find_next(fuses,NULL);

		//Zero the index
		index = 0;

		while(obj != nullptr)
		{
			if(index >= fuses->hit_count){
				break;
			}

			pFuse[index] = OBJECTDATA(obj,fuse);
			if(pFuse[index] == nullptr){
				gl_error("Unable to map object as fuse object.");
				return FAILED;
			}

			// Write the fuse metrics
			// Write the name (not id) - if it exists
			if (obj->name != nullptr)
			{
				line_object["id"] = obj->name;
			}
			else
			{
				//"Make" a value
				sprintf(buffer,"fuse:%d",obj->id);
				line_object["id"] = buffer;
			}

			//write from node name
			if (pFuse[index]->from->name != nullptr)
			{
				line_object["node1_id"] = pFuse[index]->from->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pFuse[index]->from->oclass->name,pFuse[index]->from->id);
				line_object["node1_id"] = buffer;
			}

			//write to node name
			if (pFuse[index]->to->name != nullptr)
			{
				line_object["node2_id"] = pFuse[index]->to->name;
			}
			else
			{
				//Make value
				sprintf(buffer,"%s:%d",pFuse[index]->to->oclass->name,pFuse[index]->to->id);
				line_object["node2_id"] = buffer;
			}

			//Pull the phases and figure out those
			temp_set_value = get_set_value(obj,"phases");
			
			//Clear the output array
			jsonArray2.clear();

			//Do the tests -- we're in powerflow, so the master definitions still work
			//Get phase count too
			phaseCount = 0;
			//Phase A
			if ((temp_set_value & PHASE_A) == PHASE_A)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}
				
			//Phase B
			if ((temp_set_value & PHASE_B) == PHASE_B)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Phase C
			if ((temp_set_value & PHASE_C) == PHASE_C)
			{
				jsonArray2.append(true);
				phaseCount++;
			}
			else
			{
				jsonArray2.append(false);
			}

			//Append it in
			line_object["has_phase"] = jsonArray2;
			jsonArray2.clear();

			//write capacity - I wanted to use link emergency limit (A), but its private field, write only default value
			line_object["capacity"] = 1e30;
			
			//write the length
			line_object["length"] = 1.0;

			//write the num_phases
			line_object["num_phases"] = phaseCount;

			//write is_transformer
			line_object["is_transformer"] = false;

			//If per-unit - adjust the values
			if (write_per_unit)
			{
				//Compute the per-unit base - use the nominal value off of the secondary
				per_unit_base = get_double_value(pFuse[index]->to,"nominal_voltage");

				//Calculate the base impedance
				temp_impedance_base = (per_unit_base * per_unit_base) / (system_VA_base / 3.0);
			}//End per-unit
			else	//No per-unit desired
			{
				temp_impedance_base = 1.0;	//Divide by unity - does nothing really, but easier to code this way
			}

			//write line configuration, if we're unique
			if (!b_mat_fuse_defined)
			{
				for (indexB = 0; indexB < 3; indexB++)
				{
					for (indexC = 0; indexC < 3; indexC++)
					{
						b_mat_fuse_pu[indexB*3+indexC] = pFuse[index]->b_mat[indexB][indexC]/temp_impedance_base;
					}
				}
				b_mat_fuse_defined = true;
				fuse_phase_count = phaseCount;	//Just store the first one
			}

			//write line_code
			line_object["line_code"] = "fuse_config";

			//write construction cost - only default value
			line_object["construction_cost"] = 1e30;
			//write hardening cost - only default cost
			line_object["harden_cost"] = 1e30;
			//write switch cost - only default cost
			line_object["switch_cost"] = 1e30;
			//write flag of new construction - only default cost
			line_object["is_new"] = false;
			//write flag of harden - only default cost
			line_object["can_harden"] = false;
			//write flag of add switch - only default cost
			line_object["can_add_switch"] = false;
			//write flag of switch existence - only default cost
			line_object["has_switch"] = true;
			// End of line codes

			// Append to line array
			jsonArray.append(line_object);

			// clear JSON value
			line_object.clear();

			//Get next value
			obj = gl_find_next(fuses,obj);

			index++;
		}//End of fuses
	}

	// Write line metrics into metrics_lines dictionary
	metrics_lines["properties"]["lines"] = jsonArray;

	// clear jsonArray
	jsonArray.clear();

	//Write a "fuse config", if it exists - assume that if one exists, this needs written
	if (fuses->hit_count > 0)
	{
		line_configuration_object["line_code"] = "fuse_config";

		//write num_phases
		line_configuration_object["num_phases"] = fuse_phase_count;

		//Clear the arrays, just to be safe
		jsonArray1.clear();
		jsonArray2.clear();

		// write rmatrix
		for (indexA = 0; indexA < 3; indexA++)
		{
			for (indexB = 0; indexB < 3; indexB++)
			{
				jsonArray1.append(b_mat_fuse_pu[indexA*3+indexB].Re());
			}
			jsonArray2.append(jsonArray1);
			jsonArray1.clear();
		}
		line_configuration_object["rmatrix"] = jsonArray2;
		jsonArray2.clear();

		//write xmatrix
		for (indexA = 0; indexA < 3; indexA++)
		{
			for (indexB = 0; indexB < 3; indexB++)
			{
				jsonArray1.append(b_mat_fuse_pu[indexA*3+indexB].Im());
			}
			jsonArray2.append(jsonArray1);
			jsonArray1.clear();
		}
		line_configuration_object["xmatrix"] = jsonArray2;
		jsonArray2.clear();
		// end this line configuration

		// Append to line array
		jsonArray.append(line_configuration_object);

		// clear JSON value
		line_configuration_object.clear();
	}//End fuses "configuration"

	//Write a "switch config", if it exists - assume that if one exists, this needs written
	if ((switches->hit_count > 0) || (reclosers->hit_count > 0) || (sectionalizers->hit_count > 0))
	{
		line_configuration_object["line_code"] = "switch_config";

		//write num_phases -- always three
		line_configuration_object["num_phases"] = switch_phase_count;

		//Clear the arrays, just to be safe
		jsonArray1.clear();
		jsonArray2.clear();

		// write rmatrix
		for (indexA = 0; indexA < 3; indexA++)
		{
			for (indexB = 0; indexB < 3; indexB++)
			{
				jsonArray1.append(b_mat_switch_pu[indexA*3+indexB].Re());
			}
			jsonArray2.append(jsonArray1);
			jsonArray1.clear();
		}
		line_configuration_object["rmatrix"] = jsonArray2;
		jsonArray2.clear();

		//write xmatrix
		for (indexA = 0; indexA < 3; indexA++)
		{
			for (indexB = 0; indexB < 3; indexB++)
			{
				jsonArray1.append(b_mat_switch_pu[indexA*3+indexB].Im());
			}
			jsonArray2.append(jsonArray1);
			jsonArray1.clear();
		}
		line_configuration_object["xmatrix"] = jsonArray2;
		jsonArray2.clear();
		// end this line configuration

		// Append to line array
		jsonArray.append(line_configuration_object);

		// clear JSON value
		line_configuration_object.clear();
	}//End switches "configuration"

	//write all line configurations
	//overhead and underground line configurations
	if(lineConfs->hit_count > 0)
	{
		//Loop through the array - we know how many we should have
		for (index=0; index<lineConfs->hit_count; index++)
		{
			//See if it is valid - just ignore the ones that aren't
			if (b_mat_defined[index])
			{
				//write each line configuration data
				// write line_code
				if (pLineConf[index]->name == nullptr)
				{
					sprintf(buffer,"line_config:%d",pLineConf[index]->id);
					line_configuration_object["line_code"] = buffer;
				}
				else
				{
					line_configuration_object["line_code"] = pLineConf[index]->name;
				}

				//write num_phases
				phaseCount = 0;

				obj = get_object_value(pLineConf[index],"conductor_A");

				if (obj != nullptr)
				{
					phaseCount++;
				}

				obj = get_object_value(pLineConf[index],"conductor_B");

				if (obj != nullptr)
				{
					phaseCount++;
				}

				obj = get_object_value(pLineConf[index],"conductor_C");

				if (obj != nullptr)
				{
					phaseCount++;
				}

				line_configuration_object["num_phases"] = phaseCount;

				//Clear the arrays, just to be safe
				jsonArray1.clear();
				jsonArray2.clear();

				// write rmatrix
				for (indexA = 0; indexA < 3; indexA++)
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						jsonArray1.append(b_mat_pu[index*9+indexA*3+indexB].Re());
					}
					jsonArray2.append(jsonArray1);
					jsonArray1.clear();
				}
				line_configuration_object["rmatrix"] = jsonArray2;
				jsonArray2.clear();

				//write xmatrix
				for (indexA = 0; indexA < 3; indexA++)
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						jsonArray1.append(b_mat_pu[index*9+indexA*3+indexB].Im());
					}
					jsonArray2.append(jsonArray1);
					jsonArray1.clear();
				}
				line_configuration_object["xmatrix"] = jsonArray2;
				jsonArray2.clear();
				// end this line configuration

				// Append to line array
				jsonArray.append(line_configuration_object);

				// clear JSON value
				line_configuration_object.clear();
			}//End valid item
		}//End normal loop
	}

	//write the triplex line configurations
	if(tpLineConfs->hit_count > 0 )
	{
		//Loop through the array - we know how many we should have
		for (index=0; index<tpLineConfs->hit_count; index++)
		{
			//See if it is valid - just ignore the ones that aren't
			if (b_mat_tp_defined[index])
			{
				//write each line configuration data
				// write line_code
				if (pTpLineConf[index]->name == nullptr)
				{
					sprintf(buffer,"line_config:%d",pTpLineConf[index]->id);
					line_configuration_object["line_code"] = buffer;
				}
				else
				{
					line_configuration_object["line_code"] = pTpLineConf[index]->name;
				}

				//write num_phases - should always be two, but we'll let it count
				phaseCount = 0;

				obj = get_object_value(pTpLineConf[index],"conductor_1");

				if (obj != nullptr)
				{
					phaseCount++;
				}

				obj = get_object_value(pTpLineConf[index],"conductor_2");

				if (obj != nullptr)
				{
					phaseCount++;
				}

				line_configuration_object["num_phases"] = phaseCount;

				//Clear the arrays, just to be safe
				jsonArray1.clear();
				jsonArray2.clear();

				// write rmatrix
				for (indexA = 0; indexA < 3; indexA++)
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						jsonArray1.append(b_mat_tp_pu[index*9+indexA*3+indexB].Re());
					}
					jsonArray2.append(jsonArray1);
					jsonArray1.clear();
				}
				line_configuration_object["rmatrix"] = jsonArray2;
				jsonArray2.clear();

				//write xmatrix
				for (indexA = 0; indexA < 3; indexA++)
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						jsonArray1.append(b_mat_tp_pu[index*9+indexA*3+indexB].Im());
					}
					jsonArray2.append(jsonArray1);
					jsonArray1.clear();
				}
				line_configuration_object["xmatrix"] = jsonArray2;
				jsonArray2.clear();
				// end this line configuration

				// Append to line array
				jsonArray.append(line_configuration_object);

				// clear JSON value
				line_configuration_object.clear();
			}//End valid item
		}//End triplex loop
	}//End of triplex line configuration steps

	//Write the transformer configurations
	if(TransConfsList->hit_count > 0 )
	{
		//Loop through the array - we know how many we should have
		for (index=0; index<TransConfsList->hit_count; index++)
		{
			//See if it is valid - just ignore the ones that aren't
			if (b_mat_trans_defined[index])
			{
				// write each line configuration data
				// write line_code
				if (pTransConf[index]->name == nullptr)
				{
					sprintf(buffer,"trans_config:%d",pTransConf[index]->id);
					line_configuration_object["line_code"] = buffer;
				}
				else
				{
					line_configuration_object["line_code"] = pTransConf[index]->name;
				}

				// write num_phases
				line_configuration_object["num_phases"] = trans_phase_count[index];

				//Clear the arrays, just to be safe
				jsonArray1.clear();
				jsonArray2.clear();

				// write rmatrix
				for (indexA = 0; indexA < 3; indexA++)
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						jsonArray1.append(b_mat_trans_pu[index*9+indexA*3+indexB].Re());
					}
					jsonArray2.append(jsonArray1);
					jsonArray1.clear();
				}
				line_configuration_object["rmatrix"] = jsonArray2;
				jsonArray2.clear();

				//write xmatrix
				for (indexA = 0; indexA < 3; indexA++)
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						jsonArray1.append(b_mat_trans_pu[index*9+indexA*3+indexB].Im());
					}
					jsonArray2.append(jsonArray1);
					jsonArray1.clear();
				}
				line_configuration_object["xmatrix"] = jsonArray2;
				jsonArray2.clear();
				// end this line configuration

				// Append to line array
				jsonArray.append(line_configuration_object);

				// clear JSON value
				line_configuration_object.clear();
			}//End valid configuration
		}//End loop of configurations
	}//End Transformer configurations

	//Write the regulator configurations
	if(regConfs->hit_count > 0 )
	{
		//Loop through the array - we know how many we should have
		for (index=0; index<regConfs->hit_count; index++)
		{
			//See if it is valid - just ignore the ones that aren't
			if (b_mat_reg_defined[index])
			{
				// write each line configuration data
				// write line_code
				if (pRegConf[index]->name == nullptr)
				{
					sprintf(buffer,"reg_config:%d",pRegConf[index]->id);
					line_configuration_object["line_code"] = buffer;
				}
				else
				{
					line_configuration_object["line_code"] = pRegConf[index]->name;
				}

				// write num_phases
				line_configuration_object["num_phases"] = reg_phase_count[index];

				//Clear the arrays, just to be safe
				jsonArray1.clear();
				jsonArray2.clear();

				// write rmatrix
				for (indexA = 0; indexA < 3; indexA++)
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						jsonArray1.append(b_mat_reg_pu[index*9+indexA*3+indexB].Re());
					}
					jsonArray2.append(jsonArray1);
					jsonArray1.clear();
				}
				line_configuration_object["rmatrix"] = jsonArray2;
				jsonArray2.clear();

				//write xmatrix
				for (indexA = 0; indexA < 3; indexA++)
				{
					for (indexB = 0; indexB < 3; indexB++)
					{
						jsonArray1.append(b_mat_reg_pu[index*9+indexA*3+indexB].Im());
					}
					jsonArray2.append(jsonArray1);
					jsonArray1.clear();
				}
				line_configuration_object["xmatrix"] = jsonArray2;
				jsonArray2.clear();
				// end this line configuration

				// Append to line array
				jsonArray.append(line_configuration_object);

				// clear JSON value
				line_configuration_object.clear();
			}//End valid configuration
		}//End loop of configurations
	}//End regulator configurations

	// Write line metrics into metrics_lines dictionary
	metrics_lines["properties"]["line_codes"] = jsonArray;

	// Clear jsonArray
	jsonArray.clear();

	// Write JSON files for line and line_codes
	out_file.open (filename_dump_system);
	std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
	writer->write(metrics_lines, &out_file);
	out_file << endl;
	out_file.close();

	//Clean up the mallocs
	if (lineConfs->hit_count > 0)
	{
		gl_free(b_mat_pu);
		gl_free(b_mat_defined);
	}

	if (tpLineConfs->hit_count > 0)
	{
		gl_free(b_mat_tp_pu);
		gl_free(b_mat_tp_defined);
	}

	if (TransConfsList->hit_count > 0)
	{
		gl_free(b_mat_trans_pu);
		gl_free(b_mat_trans_defined);
		gl_free(trans_phase_count);
	}

	if (regConfs->hit_count > 0)
	{
		gl_free(b_mat_reg_pu);
		gl_free(b_mat_reg_defined);
		gl_free(reg_phase_count);
	}

	//Clean up the findlists
	gl_free(ohlines);
	gl_free(tplines);
	gl_free(uglines);
	gl_free(switches);
	gl_free(regulators);
	gl_free(regConfs);
	gl_free(lineConfs);
	gl_free(tpLineConfs);
	gl_free(TransformerList);
	gl_free(TransConfsList);
	gl_free(nodes);
	gl_free(meters);
	gl_free(loads);
	gl_free(fuses);
	gl_free(reclosers);
	gl_free(sectionalizers);
	gl_free(inverters);
	gl_free(diesels);

	return SUCCESS;
}

STATUS jsondump::dump_reliability(void)
{
	FINDLIST *powermetrics, *fuses, *reclosers, *sectionalizers, *relays, *capacitors, *regulators;
	fuse *fuseData;
	recloser *reclData;
	sectionalizer *secData;
	capacitor *capData;
	regulator *regData;
	char* indices1366[] = {const_cast<char*>("SAIFI"),
						const_cast<char*>("SAIDI"),
						const_cast<char*>("CAIDI"),
						const_cast<char*>("ASAI"),
						const_cast<char*>("MAIFI"), NULL};
	int index1366;
	double *temp_double;
	enumeration *temp_emu;
	OBJECT *obj = nullptr;
	char buffer[1024];
	FILE *fn = nullptr;
	int index = 0;

	// metrics JSON value
	Json::Value metrics_reliability;	// Output dictionary for line and line configuration metrics
	Json::Value metrics_1366;
	Json::Value metrics_protection;
	Json::Value metrics_others;
	Json::Value protectionArray; // for storing array of each type of protective device
	Json::Value protect_obj;
	Json::Value otherArray; // for storing array of each type of capacitor and regulator devices
	Json::Value other_obj;
	Json::Value jsonArray; // for storing temperary opening status of devices
	// Start write to file
	Json::StreamWriterBuilder builder;
	builder["commentStyle"] = "None";
	builder["indentation"] = "";

	// Open file for writing
	ofstream out_file;

	// Find the objects to be written in
	powermetrics = gl_find_objects(FL_NEW,FT_CLASS,SAME,"power_metrics",FT_END);	//find all power_metrics
	fuses = gl_find_objects(FL_NEW,FT_CLASS,SAME,"fuse",FT_END);					//find all fuses
	reclosers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"recloser",FT_END);			//find all reclosers
	sectionalizers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"sectionalizer",FT_END);	//find all sectionalizers
//	relays = gl_find_objects(FL_NEW,FT_CLASS,SAME,"relay",FT_END);					//find all relays
	capacitors = gl_find_objects(FL_NEW,FT_CLASS,SAME,"capacitor",FT_END);	//find all capacitors
	regulators = gl_find_objects(FL_NEW,FT_CLASS,SAME,"regulator",FT_END);	//find all regulators

	// Write powermetrics data
	index = 0;
	if(powermetrics != nullptr){

		while(obj = gl_find_next(powermetrics,obj)){

			// Allow only one power metrics
			if(index > 1){
				gl_error("There are more than 1 power metrics defined");
				return FAILED;
			}

			// Loop through the reliability indices to find the value and put into JSON variable
			index1366 = -1;
			while (indices1366[++index1366] != NULL) {

				temp_double = gl_get_double_by_name(obj, indices1366[index1366]);

				if(temp_double == nullptr){
					gl_error("Unable to to find property for %s: %s is NULL", obj->name, indices1366[index1366]);
					return FAILED;
				}

				metrics_1366[indices1366[index1366]] = *temp_double;

			}

			index++;
		}
	}

	// Put 1366 indices into the reliability JSON dictionary
	metrics_reliability["GridLAB-D reliability outputs"] = metrics_1366;

	// Write the protective device data
	// Fuses
	index = 0;
	if(fuses != nullptr){

		while(obj = gl_find_next(fuses,obj)){
			if(index >= fuses->hit_count){
				break;
			}

			// Directly obtain the data information from the object
			fuseData = OBJECTDATA(obj,fuse);

			// Write device name
			protect_obj["Name"] = obj->name;

			// Write device opening status
			// Append opening status to array
			if ((fuseData->phases & 0x04) == 0x04) {
				sprintf(buffer, "%d", (fuseData->phase_A_state == 1)? true:false);
				jsonArray.append(buffer);
			}
			if ((fuseData->phases & 0x02) == 0x02) {
				sprintf(buffer, "%d", ((fuseData->phase_B_state == 1)? true:false));
				jsonArray.append(buffer);
			}
			if ((fuseData->phases & 0x01) == 0x01) {
				sprintf(buffer, "%d", ((fuseData->phase_C_state == 1)? true:false));
				jsonArray.append(buffer);
			}

			// Put array to the fuse dictionary
			protect_obj["Device opening status"] = jsonArray;
			jsonArray.clear();

			// Append fuse to the fuse array
			protectionArray.append(protect_obj);
			protect_obj.clear();

			index++;
		}

		// Put Fuse data into the protection dictionary
		metrics_protection["Fuse"] = protectionArray;
		protectionArray.clear();
	}

	// Reclosers
	index = 0;
	if(reclosers != nullptr){

		while(obj = gl_find_next(reclosers,obj)){
			if(index >= reclosers->hit_count){
				break;
			}

			// Directly obtain the data information from the object
			reclData = OBJECTDATA(obj,recloser);

			// Write device name
			protect_obj["Name"] = obj->name;

			// Write device opening status
			// Append opening status to array
			if ((reclData->phases & 0x04) == 0x04) {
				sprintf(buffer, "%d", ((reclData->phase_A_state == 1)? true:false));
				jsonArray.append(buffer);
			}
			if ((reclData->phases & 0x02) == 0x02) {
				sprintf(buffer, "%d", ((reclData->phase_B_state == 1)? true:false));
				jsonArray.append(buffer);
			}
			if ((reclData->phases & 0x01) == 0x01) {
				sprintf(buffer, "%d", ((reclData->phase_C_state == 1)? true:false));
				jsonArray.append(buffer);
			}

			// Put array to the recloser dictionary
			protect_obj["Device opening status"] = jsonArray;
			jsonArray.clear();

			// Append recloser to the recloser array
			protectionArray.append(protect_obj);
			protect_obj.clear();

			index++;
		}

		// Put recloser data into the protection dictionary
		metrics_protection["Recloser"] = protectionArray;
		protectionArray.clear();
	}

	// Sectionalizer
	index = 0;
	if(sectionalizers != nullptr){

		while(obj = gl_find_next(sectionalizers,obj)){
			if(index >= sectionalizers->hit_count){
				break;
			}

			// Directly obtain the data information from the object
			secData = OBJECTDATA(obj,sectionalizer);

			// Write device name
			protect_obj["Name"] = obj->name;

			// Write device opening status
			// Append opening status to array
			if ((secData->phases & 0x04) == 0x04) {
				sprintf(buffer, "%d", ((secData->phase_A_state == 1)? true:false));
				jsonArray.append(buffer);
			}
			if ((secData->phases & 0x02) == 0x02) {
				sprintf(buffer, "%d", ((secData->phase_B_state == 1)? true:false));
				jsonArray.append(buffer);
			}
			if ((secData->phases & 0x01) == 0x01) {
				sprintf(buffer, "%d", ((secData->phase_C_state == 1)? true:false));
				jsonArray.append(buffer);
			}

			// Put array to the sectionalizer dictionary
			protect_obj["Device opening status"] = jsonArray;
			jsonArray.clear();

			// Append sectionalizer to the sectionalizer array
			protectionArray.append(protect_obj);
			protect_obj.clear();

			index++;
		}

		// Put Sectionalizer data into the protection dictionary
		metrics_protection["Sectionalizer"] = protectionArray;
		protectionArray.clear();
	}

	// Write protection metrics into metrics_reliability dictionary
	metrics_reliability["Protective devices"] = metrics_protection;

	// Clear metrics_protection
	metrics_protection.clear();

	// Write the capacitor and regulator device data
	// Capacitors
	index = 0;
	if(capacitors != nullptr){

		while(obj = gl_find_next(capacitors,obj)){
			if(index >= capacitors->hit_count){
				break;
			}

			// Directly obtain the data information from the object
			capData = OBJECTDATA(obj,capacitor);

			// Write device name
			other_obj["Name"] = obj->name;

			// Write device opening status
			// Append opening status to array
			if ((capData->pt_phase & 0x04) == 0x04) {
				sprintf(buffer, "%d", ((capData->switchA_state == 1)? true:false));
				jsonArray.append(buffer);
			}
			if ((capData->pt_phase & 0x02) == 0x02) {
				sprintf(buffer, "%d", ((capData->switchB_state == 1)? true:false));
				jsonArray.append(buffer);
			}
			if ((capData->pt_phase & 0x01) == 0x01) {
				sprintf(buffer, "%d", ((capData->switchC_state == 1)? true:false));
				jsonArray.append(buffer);
			}

			// Put array to the dictionary
			other_obj["Device opening status"] = jsonArray;
			jsonArray.clear();

			// Append capacitor to the capacitor array
			otherArray.append(other_obj);
			other_obj.clear();

			index++;
		}

		// Put capacitor data into the dictionary
		metrics_others["Capacitor"] = otherArray;
		otherArray.clear();
	}

	// Regulators
	index = 0;
	if(regulators != nullptr){

		while(obj = gl_find_next(regulators,obj)){
			if(index >= regulators->hit_count){
				break;
			}

			// Directly obtain the data information from the object
			regData = OBJECTDATA(obj,regulator);

			// Write device name
			other_obj["Name"] = obj->name;

			// Write device opening status
			// Append tap positions to array
			if ((regData->phases & 0x04) == 0x04) {
				jsonArray.append(regData->tap[0]);
			}
			if ((regData->phases & 0x02) == 0x02) {
				jsonArray.append(regData->tap[1]);
			}
			if ((regData->phases & 0x01) == 0x01) {
				jsonArray.append(regData->tap[2]);
			}

			// Put array to the dictionary
			other_obj["Tap position"] = jsonArray;
			jsonArray.clear();

			// Append capacitor to the regulator array
			otherArray.append(other_obj);
			other_obj.clear();

			index++;
		}

		// Put capacitor data into the dictionary
		metrics_others["Regulator"] = otherArray;
		otherArray.clear();
	}

	// Write other device metrics into metrics_reliability dictionary
	metrics_reliability["Other devices"] = metrics_others;

	// Clear metrics_protection
	metrics_others.clear();

	// Write JSON files for line and line_codes
	out_file.open (filename_dump_reliability);
	std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
	writer->write(metrics_reliability, &out_file);
	out_file << endl;
	out_file.close();

	//Remove the various mallocs/findlists
	gl_free(powermetrics);
	gl_free(fuses);
	gl_free(reclosers);
	gl_free(sectionalizers);
	gl_free(relays);
	gl_free(capacitors);
	gl_free(regulators);

	return SUCCESS;

}

TIMESTAMP jsondump::commit(TIMESTAMP t){
	STATUS rv;

	if(runtime == 0){
		runtime = t;
	}
	if(write_system && ((t == runtime) || (runtime == TS_NEVER)) && (runcount < 1)){
		/* dump */
		rv = dump_system();
		++runcount;
		if(rv == FAILED){
			return TS_INVALID;
		}
		return TS_NEVER;
	} else {
		if(t < runtime){
			return runtime;
		} else {
			return TS_NEVER;
		}
	}
}

STATUS jsondump::finalize(){
	STATUS rv;

	if (write_reliability) {

		rv = dump_reliability();

		if(rv != SUCCESS){
			return FAILED;
		}

	}

	return SUCCESS;
}


int jsondump::isa(char *classname)
{
	return strcmp(classname,"jsondump")==0;
}

//New API value pulls -- functionalized to make it easier to use
//Double value
double jsondump::get_double_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	double output_value;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_double())
	{
		GL_THROW("jsdondump:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//Pull the voltage base value
	output_value = pQuantity->get_double();

	//Now get rid of the item
	delete pQuantity;

	//return it
	return output_value;
}

//Complex value
gld::complex jsondump::get_complex_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	gld::complex output_value;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_complex())
	{
		GL_THROW("jsdondump:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		//Defined above
	}

	//Pull the voltage base value
	output_value = pQuantity->get_complex();

	//Now get rid of the item
	delete pQuantity;

	//return it
	return output_value;
}

//Sets value
set jsondump::get_set_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	set output_value;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_set())
	{
		GL_THROW("jsdondump:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		//Defined above
	}

	//Pull the voltage base value
	output_value = pQuantity->get_set();

	//Now get rid of the item
	delete pQuantity;

	//return it
	return output_value;
}

//Enumeration value
enumeration jsondump::get_enum_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	enumeration output_value;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_enumeration())
	{
		GL_THROW("jsdondump:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		//Defined above
	}

	//Pull the voltage base value
	output_value = pQuantity->get_enumeration();

	//Now get rid of the item
	delete pQuantity;

	//return it
	return output_value;
}

//Object pointers
OBJECT *jsondump::get_object_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	gld_rlock *test_rlock;
	OBJECT *output_value;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_objectref())
	{
		GL_THROW("jsdondump:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		//Defined above
	}

	//Pull the voltage base value
	pQuantity->getp<OBJECT*>(output_value,*test_rlock);

	//Now get rid of the item
	delete pQuantity;

	//return it
	return output_value;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: jsondump
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_jsondump(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(jsondump::oclass);
		if (*obj!=nullptr)
		{
			jsondump *my = OBJECTDATA(*obj,jsondump);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_jsondump: %s", msg);
	}
	return 0;
}

EXPORT int init_jsondump(OBJECT *obj)
{
	jsondump *my = OBJECTDATA(obj,jsondump);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s (jsondump:%d): %s", obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT TIMESTAMP sync_jsondump(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	try
	{
		jsondump *my = OBJECTDATA(obj,jsondump);
		TIMESTAMP rv;
		obj->clock = t1;
		rv = my->runtime > t1 ? my->runtime : TS_NEVER;
		return rv;
	}
	SYNC_CATCHALL(jsondump);
}

EXPORT TIMESTAMP commit_jsondump(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	try {
		jsondump *my = OBJECTDATA(obj,jsondump);
		return my->commit(t1);
	}
	I_CATCHALL(commit,jsondump);
}

EXPORT STATUS finalize_jsondump(OBJECT *obj)
{

	jsondump *my = OBJECTDATA(obj,jsondump);

	try {
			return obj!=nullptr ? my->finalize() : FAILED;
		}
		catch (char *msg) {
			gl_error("finalize_jsondump" "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
			return FAILED;
		}
		catch (const char *msg) {
			gl_error("finalize_jsondump" "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
			return FAILED;
		}
		catch (...) {
			gl_error("finalize_jsondump" "(obj=%d;%s): unhandled exception", obj->id, obj->name?obj->name:"unnamed");
			return FAILED;
		}
}

EXPORT int isa_jsondump(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,jsondump)->isa(classname);
}

/**@}*/
