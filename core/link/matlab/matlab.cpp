// $Id: matlab.cpp
// Copyright (C) 2012 Battelle Memorial Institute

#include <stdlib.h>
#include <ctype.h>

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

static mxArray* create_mxproperty(gld_property *prop)
{
	mxArray *value=NULL;
	switch ( prop->get_type() ) {
	case PT_double:
		value = mxCreateDoubleScalar(*(double*)prop->get_addr());
		break;
	case PT_complex:
		{
			value = mxCreateDoubleMatrix(1,1,mxCOMPLEX);
			complex *v = (complex*)prop->get_addr();
			*mxGetPr(value) = v->Re();
			*mxGetPi(value) = v->Im();
		}
		break;
	case PT_int16:
		value = mxCreateDoubleScalar((double)*(int16*)prop->get_addr());
		break;
	case PT_enumeration:
	case PT_int32:
		value = mxCreateDoubleScalar((double)*(int32*)prop->get_addr());
		break;
	case PT_set:
	case PT_int64:
		value = mxCreateDoubleScalar((double)*(int64*)prop->get_addr());
		break;
	case PT_timestamp:
		value = mxCreateDoubleScalar((double)*(TIMESTAMP*)prop->get_addr());
		break;
	case PT_bool:
		value = mxCreateDoubleScalar((double)*(bool*)prop->get_addr());
		break;
	case PT_char8:
	case PT_char32:
	case PT_char256:
	case PT_char1024:
		{
			const char *str[] = {(char*)prop->get_addr()};
			value = mxCreateCharMatrixFromStrings(mwSize(1),str); 
		}
		break;
	default:
		value = NULL;
		break;
	}
	return value;
}

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

	// build gridlabd data
	mwSize dims[] = {1,1};
	mxArray *gridlabd_struct = mxCreateStructArray(2,dims,0,NULL);

	///////////////////////////////////////////////////////////////////////////
	// build global data
	LINKLIST *item;
	mxArray *global_struct = mxCreateStructArray(2,dims,0,NULL);
	for ( item=mod->get_globals() ; item!=NULL ; item=mod->get_next(item) )
	{
		char *name = mod->get_name(item);
		GLOBALVAR *var = mod->get_globalvar(item);
		if ( var==NULL ) continue;

		// do not map module or structured globals
		if ( var!=NULL && strchr(var->prop->name,':')==0 && strchr(var->prop->name,'.')==0 )
		{
			gld_property prop(var);
			mwIndex var_index = mxAddField(global_struct,prop.get_name());
			mxArray *var_struct = create_mxproperty(&prop);
			if ( var_struct!=NULL )
			{
				mod->add_copyto(var->prop->addr,mxGetData(var_struct));
				mxSetFieldByNumber(global_struct,0,var_index,var_struct);
			}
		}
	}

	// add globals structure to gridlabd structure
	mwIndex gridlabd_index = mxAddField(gridlabd_struct,"global");
	mxSetFieldByNumber(gridlabd_struct,0,gridlabd_index,global_struct);

	///////////////////////////////////////////////////////////////////////////
	// build the object data
	dims[0] = 0;
	for ( item=mod->get_objects() ; item!=NULL ; item=mod->get_next(item) )
		dims[0]++;
	dims[1] = 1;
	const char *objfields[] = {"id","name","class","parent","rank","clock","valid_to","schedule_skew",
		"latitude","longitude","in","out","rng_state","heartbeat","lock","flags"};
	mxArray *object_struct = mxCreateStructArray(2,dims,sizeof(objfields)/sizeof(objfields[0]),objfields);
	for ( item=mod->get_objects() ; item!=NULL ; item=mod->get_next(item) )
	{
		OBJECT *obj = mod->get_object(item);
		if ( obj==NULL ) continue;
		const char *objname[] = {obj->name&&isdigit(obj->name[0])?NULL:obj->name};
		const char *oclassname[] = {obj->oclass->name};

		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,0,mxCreateDoubleScalar((double)obj->id+1));
		if (obj->name) mxSetFieldByNumber(object_struct,(mwIndex)obj->id,1,mxCreateCharMatrixFromStrings(mwSize(1),objname));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,2,mxCreateCharMatrixFromStrings(mwSize(1),oclassname));
		if (obj->parent) mxSetFieldByNumber(object_struct,(mwIndex)obj->id,3,mxCreateDoubleScalar((double)obj->parent->id+1));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,4,mxCreateDoubleScalar((double)obj->rank));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,5,mxCreateDoubleScalar((double)obj->clock));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,6,mxCreateDoubleScalar((double)obj->valid_to));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,7,mxCreateDoubleScalar((double)obj->schedule_skew));
		if ( isfinite(obj->latitude) ) mxSetFieldByNumber(object_struct,(mwIndex)obj->id,8,mxCreateDoubleScalar((double)obj->latitude));
		if ( isfinite(obj->longitude) ) mxSetFieldByNumber(object_struct,(mwIndex)obj->id,9,mxCreateDoubleScalar((double)obj->longitude));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,10,mxCreateDoubleScalar((double)obj->in_svc));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,11,mxCreateDoubleScalar((double)obj->out_svc));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,12,mxCreateDoubleScalar((double)obj->rng_state));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,13,mxCreateDoubleScalar((double)obj->heartbeat));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,14,mxCreateDoubleScalar((double)obj->lock));
		mxSetFieldByNumber(object_struct,(mwIndex)obj->id,15,mxCreateDoubleScalar((double)obj->flags));
	}
	gridlabd_index = mxAddField(gridlabd_struct,"object");
	mxSetFieldByNumber(gridlabd_struct,0,gridlabd_index,object_struct);

	///////////////////////////////////////////////////////////////////////////
	// build class data
	dims[0] = dims[1] = 1;
	mxArray *class_struct = mxCreateStructArray(2,dims,0,NULL);
	gridlabd_index = mxAddField(gridlabd_struct,"class");
	mxSetFieldByNumber(gridlabd_struct,0,gridlabd_index,class_struct);

	// add classes
	for ( CLASS *oclass = callback->class_getfirst() ; oclass!=NULL ; oclass=oclass->next )
	{
		mxArray *runtime_struct = mxCreateStructArray(2,dims,0,NULL);

		// add class 
		mwIndex class_index = mxAddField(class_struct,oclass->name);
		mxSetFieldByNumber(class_struct,0,class_index,runtime_struct);

		// add properties to classes
		for ( PROPERTY *prop=oclass->pmap ; prop!=NULL && prop->oclass==oclass ; prop=prop->next )
		{
			mxArray *property_struct = mxCreateStructArray(2,dims,0,NULL);
			mwIndex runtime_index = mxAddField(runtime_struct,prop->name);
			mxSetFieldByNumber(runtime_struct,0,runtime_index,property_struct);
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// build module data
	dims[0] = dims[1] = 1;
	mxArray *module_struct = mxCreateStructArray(2,dims,0,NULL);

	// add modules
	for ( MODULE *module = callback->module.getfirst() ; module!=NULL ; module=module->next )
	{
		mxArray *module_data = mxCreateStructArray(2,dims,0,NULL);
		mwIndex module_index = mxAddField(module_struct,module->name);
		mxSetFieldByNumber(module_struct,0,module_index,module_data);
	}
	gridlabd_index = mxAddField(gridlabd_struct,"module");
	mxSetFieldByNumber(gridlabd_struct,0,gridlabd_index,module_struct);

	///////////////////////////////////////////////////////////////////////////
	// post the gridlabd structure
	engPutVariable(matlab->engine,"gridlabd",gridlabd_struct);

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
