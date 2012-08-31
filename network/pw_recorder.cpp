#include "pw_recorder.h"

EXPORT_CREATE(pw_recorder);
EXPORT_INIT(pw_recorder);
EXPORT_SYNC(pw_recorder);
EXPORT_ISA(pw_recorder);
EXPORT_COMMIT(pw_recorder);
EXPORT_PRECOMMIT(pw_recorder);

CLASS *pw_recorder::oclass = NULL;
pw_recorder *pw_recorder::defaults = NULL;

pw_recorder::pw_recorder(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"pw_recorder",sizeof(pw_recorder),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class pw_recorder";
		else
			oclass->trl = TRL_PROVEN;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_object, "model", get_model_offset(), PT_DESCRIPTION, "pw_model object for the PowerWorld model this recorder is monitoring",
			PT_char1024, "outfile", get_outfile_name_offset(), PT_DESCRIPTION, "stuff",
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		memset(this,0,sizeof(pw_recorder));

	}
}

int pw_recorder::create(){
	return 1;
}

int pw_recorder::init(OBJECT *parent){

	return 1;
}

int pw_recorder::precommit(TIMESTAMP t1){

	return 1;
}

TIMESTAMP pw_recorder::presync(TIMESTAMP t1){

	return TS_NEVER;
}

TIMESTAMP pw_recorder::sync(TIMESTAMP t1){

	return TS_NEVER;
}

TIMESTAMP pw_recorder::postsync(TIMESTAMP t1){
	
	return TS_NEVER;
}

TIMESTAMP pw_recorder::commit(TIMESTAMP t1, TIMESTAMP t2){
	return TS_NEVER;
}

int pw_recorder::isa(char *classname){
	return 1;
}



// EOF
