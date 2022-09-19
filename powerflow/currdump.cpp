// $Id: currdump.cpp 4738 2014-07-03 00:55:39Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file currdump.cpp

	@{
*/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "currdump.h"

//////////////////////////////////////////////////////////////////////////
// currdump CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* currdump::oclass = nullptr;

currdump::currdump(MODULE *mod)
{
	if (oclass==nullptr)
	{
		// register the class definition
		oclass = gl_register_class(mod,"currdump",sizeof(currdump),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==nullptr)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_char32,"group",PADDR(group),PT_DESCRIPTION,"the group ID to output data for (all links if empty)",
			PT_timestamp,"runtime",PADDR(runtime),PT_DESCRIPTION,"the time to check current data",
			PT_char256,"filename",PADDR(filename),PT_DESCRIPTION,"the file to dump the current data into",
			PT_int32,"runcount",PADDR(runcount),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the number of times the file has been written to",
			PT_enumeration, "mode", PADDR(mode),
				PT_KEYWORD, "RECT", (enumeration)CDM_RECT,
				PT_KEYWORD, "POLAR", (enumeration)CDM_POLAR,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		
	}
}


int currdump::create(void)
{
	group.erase();
	runtime = TS_NEVER;
	runcount = 0;
	mode = CDM_RECT;
	return 1;
}

int currdump::init(OBJECT *parent)
{
	return 1;
}

int currdump::isa(char *classname)
{
	return strcmp(classname,"currdump")==0;
}

void currdump::dump(TIMESTAMP t){
	char namestr[128];
	char timestr[64];
	FINDLIST *links = nullptr;
	OBJECT *obj = nullptr;
	FILE *outfile = nullptr;

	gld_property *link_current_value_link[3];
	gld::complex link_current_values[3];

	if(group[0] == 0){
		links = gl_find_objects(FL_NEW,FT_MODULE,SAME,"powerflow",FT_END);
	} else {
		links = gl_find_objects(FL_NEW,FT_MODULE,SAME,"powerflow",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
	}

	if(links == nullptr){
		gl_warning("no links were found to dump");
		return;
	}

	outfile = fopen(filename, "w");
	if(outfile == nullptr){
		gl_error("currdump unable to open %s for output", filename.get_string());
		return;
	}

	//nodeclass = node::oclass;
	//vA=gl_find_property(nodeclass, "

	/* print column names */
	gl_printtime(t, timestr, 64);
	fprintf(outfile,"# %s run at %s on %i links\n", filename.get_string(), timestr, links->hit_count);
	if(mode == CDM_RECT){
		fprintf(outfile,"link_name,currA_real,currA_imag,currB_real,currB_imag,currC_real,currC_imag\n");
	}
	else if (mode == CDM_POLAR){
		fprintf(outfile,"link_name,currA_mag,currA_angle,currB_mag,currB_angle,currC_mag,currC_angle\n");
	}
	obj = 0;
	while (obj=gl_find_next(links,obj)){
		if(gl_object_isa(obj, "link", "powerflow")){

			//Map the properties of interest - first current
			link_current_value_link[0] = new gld_property(obj,"current_in_A");

			//Check it
			if (!link_current_value_link[0]->is_valid() || !link_current_value_link[0]->is_complex())
			{
				GL_THROW("currdump - Unable to map current property of link:%d - %s",obj->id,(obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While the currdump object attempted to map the current_in_A, current_in_B, or current_in_C, an error
				occurred.  Please try again.  If the error persists, please submit your code via the ticketing and issues system.
				*/
			}

			//Map the properties of interest - second current
			link_current_value_link[1] = new gld_property(obj,"current_in_B");

			//Check it
			if (!link_current_value_link[1]->is_valid() || !link_current_value_link[1]->is_complex())
			{
				GL_THROW(const_cast<char*>("currdump - Unable to map current property of link:%d - %s"),obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map the properties of interest - third current
			link_current_value_link[2] = new gld_property(obj,"current_in_C");

			//Check it
			if (!link_current_value_link[2]->is_valid() || !link_current_value_link[2]->is_complex())
			{
				GL_THROW(const_cast<char*>("currdump - Unable to map current property of link:%d - %s"),obj->id,(obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Pull the values
			link_current_values[0] = link_current_value_link[0]->get_complex();
			link_current_values[1] = link_current_value_link[1]->get_complex();
			link_current_values[2] = link_current_value_link[2]->get_complex();

			if(obj->name == nullptr){
				sprintf(namestr, "%s:%i", obj->oclass->name, obj->id);
			}
			if(mode == CDM_RECT){
				fprintf(outfile,"%s,%f,%f,%f,%f,%f,%f\n",(obj->name ? obj->name : namestr),link_current_values[0].Re(),link_current_values[0].Im(),link_current_values[1].Re(),link_current_values[1].Im(),link_current_values[2].Re(),link_current_values[2].Im());
			} else if (mode == CDM_POLAR){
				fprintf(outfile,"%s,%f,%f,%f,%f,%f,%f\n",(obj->name ? obj->name : namestr),link_current_values[0].Mag(),link_current_values[0].Arg(),link_current_values[1].Mag(),link_current_values[1].Arg(),link_current_values[2].Mag(),link_current_values[2].Arg());
			}

			//Clear the properties found
			delete link_current_value_link[0];
			delete link_current_value_link[1];
			delete link_current_value_link[2];
		}
	}
	fclose(outfile);

	//Free the list
	gl_free(links);
}

TIMESTAMP currdump::commit(TIMESTAMP t){
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
// IMPLEMENTATION OF CORE LINKAGE: currdump
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_currdump(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(currdump::oclass);
		if (*obj!=nullptr)
		{
			currdump *my = OBJECTDATA(*obj,currdump);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_currdump: %s", msg);
	}
	return 0;
}

EXPORT int init_currdump(OBJECT *obj)
{
	currdump *my = OBJECTDATA(obj,currdump);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s (currdump:%d): %s", obj->name, obj->id, msg);
		return 0; 
	}
}

EXPORT TIMESTAMP sync_currdump(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	currdump *my = OBJECTDATA(obj,currdump);
	TIMESTAMP rv;
	obj->clock = t1;
	rv = my->runtime > t1 ? my->runtime : TS_NEVER;
	return rv;
}

EXPORT TIMESTAMP commit_currdump(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	currdump *my = OBJECTDATA(obj,currdump);
	try {
		return my->commit(t1);
	} catch(const char *msg){
		gl_error("%s (currdump:%d): %s", obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT int isa_currdump(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,currdump)->isa(classname);
}

/**@}*/
