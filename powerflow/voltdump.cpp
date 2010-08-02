// $Id: voltdump.cpp 1182 2008-12-22 22:08:36Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file voltdump.cpp

	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "voltdump.h"
#include "node.h"

//////////////////////////////////////////////////////////////////////////
// voltdump CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* voltdump::oclass = NULL;

voltdump::voltdump(MODULE *mod)
{
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"voltdump",sizeof(voltdump),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_char32,"group",PADDR(group),PT_DESCRIPTION,"the group ID to output data for (all nodes if empty)",
			PT_timestamp,"runtime",PADDR(runtime),PT_DESCRIPTION,"the time to check voltage data",
			PT_char32,"filename",PADDR(filename),PT_DESCRIPTION,"the file to dump the voltage data into",
			PT_int32,"runcount",PADDR(runcount),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the number of times the file has been written to",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		
	}
}


int voltdump::create(void)
{
	memset(group, 0, sizeof(char32));
	runtime = TS_NEVER;
	runcount = 0;
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
	char namestr[64];
	char timestr[64];
	FINDLIST *nodes = NULL;
	OBJECT *obj = NULL;
	FILE *outfile = NULL;
	node *pnode;
//	CLASS *nodeclass = NULL;
//	PROPERTY *vA, *vB, *vC;

	if(group[0] == 0){
		nodes = gl_find_objects(FL_NEW,FT_MODULE,SAME,"powerflow",FT_END);
	} else {
		nodes = gl_find_objects(FL_NEW,FT_MODULE,SAME,"powerflow",AND,FT_GROUPID,SAME,group,FT_END);
	}

	if(nodes == NULL){
		gl_warning("no nodes were found to dump");
		return;
	}

	outfile = fopen(filename, "w");
	if(outfile == NULL){
		gl_error("voltdump unable to open %s for output", filename);
		return;
	}

	//nodeclass = node::oclass;
	//vA=gl_find_property(nodeclass, "

	/* print column names */
	gl_printtime(t, timestr, 64);
	fprintf(outfile,"# %s run at %s on %i nodes\n", filename, timestr, nodes->hit_count);
	fprintf(outfile,"node_name,voltA_real,voltA_imag,voltB_real,voltB_imag,voltC_real,voltC_imag\n");
	while (obj=gl_find_next(nodes,obj)){
		if(gl_object_isa(obj, "node", "powerflow")){
			pnode = OBJECTDATA(obj,node);
			if(obj->name == NULL){
				sprintf(namestr, "%s:%i", obj->oclass->name, obj->id);
			}
			fprintf(outfile,"%s,%f,%f,%f,%f,%f,%f\n",(obj->name ? obj->name : namestr),pnode->voltage[0].Re(),pnode->voltage[0].Im(),pnode->voltage[1].Re(),pnode->voltage[1].Im(),pnode->voltage[2].Re(),pnode->voltage[2].Im());
		}
	}
	fclose(outfile);
}

int voltdump::commit(TIMESTAMP t){
	if(runtime == 0){
		runtime = t;
	}
	if((t == runtime || runtime == TS_NEVER) && (runcount < 1)){
		/* dump */
		dump(t);
		++runcount;
	}
	return 1;
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
		if (*obj!=NULL)
		{
			voltdump *my = OBJECTDATA(*obj,voltdump);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_voltdump: %s", msg);
	}
	return 0;
}

EXPORT int init_voltdump(OBJECT *obj)
{
	voltdump *my = OBJECTDATA(obj,voltdump);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s (voltdump:%d): %s", obj->name, obj->id, msg);
		return 0; 
	}
}

EXPORT TIMESTAMP sync_voltdump(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	voltdump *my = OBJECTDATA(obj,voltdump);
	TIMESTAMP rv;
	obj->clock = t1;
	rv = my->runtime > t1 ? my->runtime : TS_NEVER;
	return rv;
}

EXPORT int commit_voltdump(OBJECT *obj){
	voltdump *my = OBJECTDATA(obj,voltdump);
	try {
		return my->commit(obj->clock);
	} catch(const char *msg){
		gl_error("%s (voltdump:%d): %s", obj->name, obj->id, msg);
		return 0; 
	}
}

EXPORT int isa_voltdump(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,voltdump)->isa(classname);
}

/**@}*/
