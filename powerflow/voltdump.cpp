// $Id: voltdump.cpp 4738 2014-07-03 00:55:39Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute
	@file voltdump.cpp
	@{
*/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "voltdump.h"

//////////////////////////////////////////////////////////////////////////
// voltdump CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* voltdump::oclass = nullptr;

voltdump::voltdump(MODULE *mod)
{
	if (oclass==nullptr)
	{
		// register the class definition
		oclass = gl_register_class(mod,"voltdump",sizeof(voltdump),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class voltdump";
		else
			oclass->trl = TRL_PROVEN;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_char32,"group",PADDR(group),PT_DESCRIPTION,"the group ID to output data for (all nodes if empty)",
			PT_timestamp,"runtime",PADDR(runtime),PT_DESCRIPTION,"the time to check voltage data",
			PT_char256,"filename",PADDR(filename),PT_DESCRIPTION,"the file to dump the voltage data into", // must keep this for compatibility
			PT_char256,"file",PADDR(filename),PT_DESCRIPTION,"the file to dump the voltage data into", // added 2012-07-10, adds convenience
			PT_int32,"runcount",PADDR(runcount),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the number of times the file has been written to",
			PT_enumeration, "mode", PADDR(mode),PT_DESCRIPTION,"dumps the voltages in either polar or rectangular notation",
				PT_KEYWORD, "RECT", (enumeration)VDM_RECT,
				PT_KEYWORD, "POLAR", (enumeration)VDM_POLAR,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		
	}
}


int voltdump::create(void)
{
	group.erase();
	runtime = TS_NEVER;
	runcount = 0;
	mode = VDM_RECT;
	return 1;
}

int voltdump::init(OBJECT *parent)
{
	return 1;
}

int voltdump::isa(char *classname)
{
	return strcmp(classname,"voltdump")==0;
}

void voltdump::dump(TIMESTAMP t){
	char namestr[128];
	char timestr[128];
	FINDLIST *nodes = nullptr;
	OBJECT *obj = nullptr;
	FILE *outfile = nullptr;

	gld_property *node_voltage_value_link[3];
	gld::complex node_voltage_values[3];

	//Find the objects - note that "FT_CLASS" requires an explicit match (not parent classing), so
	//this would have to be replicated for all different node types to get it to work.
	if(group[0] == 0){
		nodes = gl_find_objects(FL_NEW,FT_MODULE,SAME,"powerflow",FT_END);
	} else {
		nodes = gl_find_objects(FL_NEW,FT_MODULE,SAME,"powerflow",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
	}

	if(nodes == nullptr){
		gl_warning("no nodes were found to dump");
		return;
	}

	outfile = fopen(filename, "w");
	if(outfile == nullptr){
		gl_error("voltdump unable to open %s for output", filename.get_string());
		return;
	}

	//nodeclass = node::oclass;
	//vA=gl_find_property(nodeclass, "

	/* print column names */
	gl_printtime(t, timestr, 64);
	fprintf(outfile,"# %s run at %s on %i powerflow objects (not all are nodes)\n", filename.get_string(), timestr, nodes->hit_count);
	if (mode == VDM_RECT)
		fprintf(outfile,"node_name,voltA_real,voltA_imag,voltB_real,voltB_imag,voltC_real,voltC_imag\n");
	else if (mode == VDM_POLAR)
		fprintf(outfile,"node_name,voltA_mag,voltA_angle,voltB_mag,voltB_angle,voltC_mag,voltC_angle\n");
	
	obj = 0;
	while (obj=gl_find_next(nodes,obj))
	{
		if(gl_object_isa(obj, "triplex_node", "powerflow"))
		{

			//Map the properties of interest - first current
			node_voltage_value_link[0] = new gld_property(obj,"voltage_1");

			//Check it
			if (!node_voltage_value_link[0]->is_valid() || !node_voltage_value_link[0]->is_complex())
			{
				GL_THROW("voltdump - Unable to map voltage property of triplex_node:%d - %s",obj->id,(obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While the voltdump object attempted to map the voltage_1, voltage_2, or voltage_N, an error
				occurred.  Please try again.  If the error persists, please submit your code via the ticketing and issues system.
				*/
			}

			//Map the properties of interest - second current
			node_voltage_value_link[1] = new gld_property(obj,"voltage_2");

			//Check it
			if (!node_voltage_value_link[1]->is_valid() || !node_voltage_value_link[1]->is_complex())
			{
				GL_THROW("voltdump - Unable to map voltage property of triplex_node:%d - %s",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map the properties of interest - third current
			node_voltage_value_link[2] = new gld_property(obj,"voltage_N");

			//Check it
			if (!node_voltage_value_link[2]->is_valid() || !node_voltage_value_link[2]->is_complex())
			{
				GL_THROW("voltdump - Unable to map voltage property of triplex_node:%d - %s",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}
		}
		else if(gl_object_isa(obj, "node", "powerflow"))
		{
			//Map the properties of interest - first current
			node_voltage_value_link[0] = new gld_property(obj,"voltage_A");

			//Check it
			if (!node_voltage_value_link[0]->is_valid() || !node_voltage_value_link[0]->is_complex())
			{
				GL_THROW("voltdump - Unable to map voltage property of node:%d - %s",obj->id,(obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While the voltdump object attempted to map the voltage_A, voltage_B, or voltage_C, an error
				occurred.  Please try again.  If the error persists, please submit your code via the ticketing and issues system.
				*/
			}

			//Map the properties of interest - second current
			node_voltage_value_link[1] = new gld_property(obj,"voltage_B");

			//Check it
			if (!node_voltage_value_link[1]->is_valid() || !node_voltage_value_link[1]->is_complex())
			{
				GL_THROW("voltdump - Unable to map voltage property of node:%d - %s",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map the properties of interest - third current
			node_voltage_value_link[2] = new gld_property(obj,"voltage_C");

			//Check it
			if (!node_voltage_value_link[2]->is_valid() || !node_voltage_value_link[2]->is_complex())
			{
				GL_THROW("voltdump - Unable to map voltage property of node:%d - %s",obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}
		}
		else	//Skip -- this is just some other object -- consequence of the findlist restrictions
		{
			//Next
			continue;
		}

		//Pull the values
		node_voltage_values[0] = node_voltage_value_link[0]->get_complex();
		node_voltage_values[1] = node_voltage_value_link[1]->get_complex();
		node_voltage_values[2] = node_voltage_value_link[2]->get_complex();

		if(obj->name == nullptr){
			sprintf(namestr, "%s:%i", obj->oclass->name, obj->id);
		}
		if(mode == VDM_RECT){
			fprintf(outfile,"%s,%f,%f,%f,%f,%f,%f\n",(obj->name ? obj->name : namestr),node_voltage_values[0].Re(),node_voltage_values[0].Im(),node_voltage_values[1].Re(),node_voltage_values[1].Im(),node_voltage_values[2].Re(),node_voltage_values[2].Im());
		} else if(mode == VDM_POLAR){
			fprintf(outfile,"%s,%f,%f,%f,%f,%f,%f\n",(obj->name ? obj->name : namestr),node_voltage_values[0].Mag(),node_voltage_values[0].Arg(),node_voltage_values[1].Mag(),node_voltage_values[1].Arg(),node_voltage_values[2].Mag(),node_voltage_values[2].Arg());
		}

		//Clear the properties found
		delete node_voltage_value_link[0];
		delete node_voltage_value_link[1];
		delete node_voltage_value_link[2];
	}
	
	fclose(outfile);

	//Free the findlist
	gl_free(nodes);
}

TIMESTAMP voltdump::commit(TIMESTAMP t){
	if(runtime == 0){
		runtime = t;
	}
	if((t >= runtime || runtime == TS_NEVER) && (runcount < 1)){
		/* dump */
		dump(t);
		++runcount;
	}
	return TS_NEVER;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: voltdump
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_voltdump(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(voltdump::oclass);
		if (*obj!=nullptr)
		{
			voltdump *my = OBJECTDATA(*obj,voltdump);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(voltdump);
}

EXPORT int init_voltdump(OBJECT *obj)
{
	try {
		voltdump *my = OBJECTDATA(obj,voltdump);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(voltdump);
}

EXPORT TIMESTAMP sync_voltdump(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	try
	{
		voltdump *my = OBJECTDATA(obj,voltdump);
		TIMESTAMP rv;
		obj->clock = t1;
		rv = my->runtime > t1 ? my->runtime : TS_NEVER;
		return rv;
	}
	SYNC_CATCHALL(voltdump);
}

EXPORT TIMESTAMP commit_voltdump(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	try {
		voltdump *my = OBJECTDATA(obj,voltdump);
		return my->commit(t1);
	} 
	I_CATCHALL(commit,voltdump);
}

EXPORT int isa_voltdump(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,voltdump)->isa(classname);
}

/**@}*/
