// $Id: matlab.cpp
// Copyright (C) 2012 Battelle Memorial Institute

#include <stdlib.h>

// you must have matlab installed and ensure matlab/extern/include is in include path
#include <matrix.h>
#include <engine.h>

// you must have gridlabd installed and ensure core is in include path
#include <gridlabd.h>
#include <link.h>

CALLBACKS *callback = NULL;

typedef struct {
	Engine *engine;
	int visible;
	char *init;
	char *sync;
	char *term;
	int status;
} MATLABLINK;

EXPORT bool create(link *mod, CALLBACKS *fntable)
{
	callback = fntable;
	MATLABLINK *matlab = new MATLABLINK;
	memset(matlab,0,sizeof(MATLABLINK));
	mod->set_data(matlab);
	return true;
}

EXPORT bool settag(link *mod, char *tag, char *data)
{
	MATLABLINK *matlab = (MATLABLINK*)mod->get_data();
	if ( strcmp(tag,"visible")==0 )
	{
		matlab->visible = atoi(data);
	}
	else if ( strcmp(tag,"on_init")==0 )
	{
		size_t len = strlen(data);
		if ( len>0 )
		{
			matlab->init = new char(len+1);
			strcpy(matlab->init,data);
		}
	}
	else if ( strcmp(tag,"on_sync")==0 )
	{
		size_t len = strlen(data);
		if ( len>0 )
		{
			matlab->sync = new char(len+1);
			strcpy(matlab->sync,data);
		}
	}
	else if ( strcmp(tag,"on_term")==0 )
	{
		size_t len = strlen(data);
		if ( len>0 )
		{
			matlab->term = new char(len+1);
			strcpy(matlab->term,data);
		}
	}
	else
	{
		gl_output("tag '%s' not valid for matlab target", tag);
		return false;
	}
	return true;
}

EXPORT bool init(link *mod)
{
	// initialize matlab engine
	MATLABLINK *matlab = (MATLABLINK*)mod->get_data();
	matlab->engine = engOpenSingleUse(NULL,NULL,&matlab->status);
	if ( matlab->engine==NULL )
		return false;

	// setup matlab engine
	engSetVisible(matlab->engine,matlab->visible);
	if ( matlab->init ) engEvalString(matlab->engine,matlab->init);
//	engEvalString(matlab->engine,"(now-datenum(1970,1,1,0,0,0))*86400;");
//	mxArray *ans = engGetVariable(matlab->engine,"ans");
//	if ( ans && mxIsDouble(ans) )
//	{
//		time_t t = time(NULL);
//		double *pVal = (double*)mxGetData(ans);
//		if ( pVal && *pVal!=time(NULL) )
//			gl_warning("matlab clock skew is %f second", *pVal - time(NULL));
//		else
//			gl_warning("unable to read matlab clock");
//	}


	// build global data
	LINKLIST *item;
	mwSize dims[] = {1,1};
	const char *fields[] = {""};
	mxArray *globals = mxCreateStructArray(2,dims,0,fields);
	engPutVariable(matlab->engine,"globals",globals);
	for ( item=mod->get_globals() ; item!=NULL ; item=mod->get_next(item) )
	{
		char *name = mod->get_name(item);
		GLOBALVAR *var = mod->get_globalvar(item);
		int field = mxAddField(globals,var->prop->name);
		switch ( var->prop->ptype ) {
		case PT_int32:
			mxArray *value = mxCreateDoubleScalar((double)*(int32*)var->prop->addr);
			mod->add_copyto(var->prop->addr,mxGetData(value));
			mxSetFieldByNumber(globals,0,field,value);
			break;
		}
	}

//	char data[] = "TODO";
//	char objname[256], propertyname[64];
//	if ( sscanf(data,"%[^.].%s",objname,propertyname)!=2 )
//	{
//		gl_output("argument '%s' is not a valid object.property specification", data);
//		return false;
//	}

//	OBJECT *obj = object_find_name(objname);
//	if ( obj==NULL )
//	{
//		gl_output("object '%s' not found", objname);
//		return false;
//	}
	
//	PROPERTY *prop = class_find_property(obj->oclass,propertyname);
//	if ( prop==NULL )
//	{
//		gl_output("property '%s' not found in object '%s'", propertyname, objname);
//		return false;
//	}
	return true;
}

EXPORT TIMESTAMP sync(link* mod,TIMESTAMP t0)
{
	TIMESTAMP t1 = TS_NEVER;
	MATLABLINK *matlab = (MATLABLINK*)mod->get_data();

	// TODO process copy to list

	if ( matlab->sync ) engEvalString(matlab->engine,matlab->sync);
	mxArray *ans = engGetVariable(matlab->engine,"ans");
	if ( ans && mxIsDouble(ans) )
	{
		double *pVal = (double*)mxGetData(ans);
		t1 = floor(*pVal);
	}

	// TODO process copy from list
	return TS_NEVER;
}

EXPORT bool term(link* mod)
{
	// close matlab engine
	MATLABLINK *matlab = (MATLABLINK*)mod->get_data();
	if ( matlab->term ) engEvalString(matlab->engine,matlab->term);
	return engClose(matlab->engine)==0;
}
