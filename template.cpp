#include "gridlabd.h"

#ifndef _class_template_h_
#define _class_template_h_

class classtemplate{
public:
// required
	classtemplate(MODULE *module);
	int create();
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	static CLASS *oclass;
	static classtemplate *defaults
// optional
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMPT t0, TIMESTAMP t1);
	int commit();
	int notify(NOTIFYMODULE type, PROPERTY *prop); // type = {NM_PREUPDATE, NM_POSTUPDATE, NM_RESET}
};

#endif

CLASS *classtemplate::oclass = NULL;
classtemplate *classtemplate::defaults = NULL;

classtemplate::classtemplate(MODULE *module){
	if (oclass==NULL)
		{
			// register the class definition
			oclass = gl_register_class(mod,"house",sizeof(house_e),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);

			if (oclass==NULL)
				GL_THROW("unable to register object class implemented by %s",__FILE__);

			// publish the class properties
		if (gl_publish_variable(oclass,
			// insert properties here
			NULL) < 1)
			GL_THROW("unable to publish properties in %s",__FILE__);
		}
}

int classtemplate::create(OBJECT *parent){
	return 1;
}

EXPORT int create_classtemplate(OBJECT **obj, OBJECT *parent){
	*obj = gl_create_object(classtemplate::oclass);
	if(*obj != 0){
		classtemplate *my = OBJECTDATA(*obj, classtemplate);
		gl_set_parent(*obj, parent);
		my->create();
		return 1;
	}
	return 0;
}

TIMESTAMP classtemplate::presync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

TIMESTAMP classtemplate::sync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

TIMESTAMP classtemplate::postsync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

EXPORT TIMESTAMP classtemplate_sync(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass){
		classtemplate *my = OBJECTDATA(obj, classtemplate);
		TIMESTAMP t1 = TS_NEVER;
		if (obj->clock <= ROUNDOFF)
			obj->clock = t0;  //set the object clock if it has not been set yet

		try {
			switch (pass)
			{
				case PC_PRETOPDOWN:
					t1 = my->presync(obj->clock, t0);
					break;

				case PC_BOTTOMUP:
					t1 = my->sync(obj->clock, t0);
					obj->clock = t0;
					break;
				case PC_POSTTOPDOWN:
					t1 = my->postsync(obj->clock, t0);
					obj->clock = t0;
					break;
				default:
					gl_error("classtemplate::sync- invalid pass configuration");
					t1 = TS_INVALID; // serious error in exec.c
			}
		}
		catch (char *msg)
		{
			gl_error("classtemplate::sync exception caught: %s", msg);
			t1 = TS_INVALID;
		}
		catch (...)
		{
			gl_error("classtemplate::sync exception caught: no info");
			t1 = TS_INVALID;
		}
	return t1;
}

int classtemplate::init(OBJECT *parent){
	return 1;
}

EXPORT int classtemplate_init(OBJECT *obj){
	try{
		classtemplate *my = OBJECTDATA(obj, classtemplate);
		return my->init(obj->parent);
	}
	catch (char *msg){
		gl_error("classtemplate:%d (%s) %s", obj->id, obj->name ? obj->name : "anonymous", msg);
		return 0;
	}
}

int classtemplate::commit(TIMESTAMP t){
	return 1;
}

EXPORT int classtemplate_commit(OBJECT *obj){
	classtemplate *my = OBJECTDATA(obj, classtemplate);
	try {
		return my->commit(obj->clock);
	} catch(const char *msg){
		GL_THROW("%s (classtemplate:%d): %s", obj->name, obj->id, msg);
		return 0;
	}
}

int classtemplate::isa(char *classname){
	return strcmp(oclass->name, classname);
}

EXPORT int isa_classtemplate(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,classtemplate)->isa(classname);
	} else {
		return 0;
	}
}

int classtemplate::prenotify(PROPERTY *prop){
	return 1;
}

int classtemplate::postnotify(PROPERTY *prop){
	return 1;
}

EXPORT int notify_classtemplate(OBJECT *obj, NOTIFYMODULE nm, PROPERTY *prop){
	classtemplate *my = OBJECTDATA(obj, classtemplate);
	switch(nm){
		case NM_PREUPDATE:
			return my->prenotify(prop);
		case NM_POSTUPDATE:
			return my->postnotify(prop);
		}
	return 0;
}

// eof
