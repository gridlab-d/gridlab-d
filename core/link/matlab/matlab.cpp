// $Id: matlab.cpp
// Copyright (C) 2012 Battelle Memorial Institute

#include <stdlib.h>
#ifdef WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <ctype.h>

// you must have gridlabd installed and ensure core is in include path
#include "gridlabd.h"
#include "link.h"

CALLBACKS *callback = NULL;

#ifdef HAVE_MATLAB

// you must have matlab installed and ensure matlab/extern/include is in include path
#include "matrix.h"
#include "engine.h"

typedef enum {
	MWD_HIDE, // never show window
	MWD_SHOW, // show only while running
	MWD_KEEP, // show while running and keep open when done
	MWD_ONERROR, // show only on error and keep open when done
	MWD_ONDEBUG, // show only when debugging and keep open when done
} MATLABWINDOWDISPOSITION;

typedef struct {
	Engine *engine;
	char *command;
	MATLABWINDOWDISPOSITION window;
	int keep_onerror;
	char *init;
	char *sync;
	char *term;
	int status;
	char *rootname;
	char workdir[1024];
	size_t output_size;
	char *output_buffer;
	mxArray *root;
} MATLABLINK;

static mxArray* matlab_exec(MATLABLINK *matlab, char *format, ...)
{
	char cmd[4096];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(cmd,format,ptr);
	va_end(ptr);
	engEvalString(matlab->engine,cmd);
	if ( matlab->output_buffer && strcmp(matlab->output_buffer,"")!=0 )
		gl_verbose( "%s", matlab->output_buffer );
	mxArray *ans = engGetVariable(matlab->engine,"ans");
	return ans; // failed
}

static mxArray* matlab_create_value(gld_property *prop)
{
	mxArray *value=NULL;
	switch ( prop->get_type() ) {
	case PT_double:
	case PT_random:
	case PT_enduse:
	case PT_loadshape:
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
	case PT_double_array:
		{
			double_array *data = (double_array*)prop->get_addr();
			size_t n=data->get_rows(), m=data->get_cols();
			value = mxCreateDoubleMatrix(0,0,mxREAL);
			double *copy = (double*)mxMalloc(m*n);
			for ( int c=0 ; c<m ; c++ )
			{
				for ( int r=0 ; r<n ; r++ )
				{
					copy[c*m+r] = data->get_at(r,c);
				}
			}
			mxSetPr(value,copy);
			mxSetM(value,m);
			mxSetN(value,n);
		}
		break;
	case PT_complex_array:
		// TODO
		break;
	default:
		value = NULL;
		break;
	}
	return value;
}
static mxArray* matlab_set_value(mxArray *value, gld_property *prop)
{
	switch ( prop->get_type() ) {
	case PT_double:
	case PT_random:
	case PT_enduse:
	case PT_loadshape:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *ptr = *(double*)prop->get_addr();
		}
		break;
	case PT_complex:
		{
			double *r = mxGetPr(value);
			double *i = mxGetPi(value);
			complex *v = (complex*)prop->get_addr();
			if (r) *r = v->Re();
			if (i) *i = v->Im();
		}
		break;
	case PT_int16:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *ptr = (double)*(int16*)prop->get_addr();
		}
		break;
	case PT_enumeration:
	case PT_int32:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *ptr = (double)*(int32*)prop->get_addr();
		}
		break;
	case PT_set:
	case PT_int64:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *ptr = (double)*(int64*)prop->get_addr();
		}
		break;
	case PT_timestamp:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *ptr = (double)*(TIMESTAMP*)prop->get_addr();
		}
		break;
	case PT_bool:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *ptr = (double)*(bool*)prop->get_addr();
		}
		break;
	case PT_char8:
	case PT_char32:
	case PT_char256:
	case PT_char1024:
		// TODO
		break;
	case PT_double_array:
		// TODO
		break;
	case PT_complex_array:
		// TODO
		break;
	default:
		value = NULL;
		break;
	}
	return value;
}
static mxArray* matlab_get_value(mxArray *value, gld_property *prop)
{
	switch ( prop->get_type() ) {
	case PT_double:
	case PT_random:
	case PT_enduse:
	case PT_loadshape:
		{
			double *ptr = mxGetPr(value);
			*(double*)prop->get_addr() = *ptr;
		}
		break;
	case PT_complex:
		{
			double *r = mxGetPr(value);
			double *i = mxGetPi(value);
			complex *v = (complex*)prop->get_addr();
			if (r) v->Re() = *r;
			if (i) v->Im() = *i;
		}
		break;
	case PT_int16:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *(int16*)prop->get_addr() = (int16)*ptr;
		}
		break;
	case PT_enumeration:
	case PT_int32:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *(int32*)prop->get_addr() = (int32)*ptr;
		}
		break;
	case PT_set:
	case PT_int64:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *(int64*)prop->get_addr() = (int64)*ptr;
		}
		break;
	case PT_timestamp:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *(TIMESTAMP*)prop->get_addr() = (TIMESTAMP)*ptr;
		}
		break;
	case PT_bool:
		{
			double *ptr = mxGetPr(value);
			if (ptr) *(bool*)prop->get_addr() = (bool)*ptr;
		}
		break;
	case PT_char8:
	case PT_char32:
	case PT_char256:
	case PT_char1024:
		// TODO
		break;
	default:
		value = NULL;
		break;
	}
	return value;
}
EXPORT bool glx_create(glxlink *mod, CALLBACKS *fntable)
{
	callback = fntable;
	MATLABLINK *matlab = new MATLABLINK;
	memset(matlab,0,sizeof(MATLABLINK));
	matlab->rootname="gridlabd";
	mod->set_data(matlab);
	return true;
}

EXPORT bool glx_settag(glxlink *mod, char *tag, char *data)
{
	MATLABLINK *matlab = (MATLABLINK*)mod->get_data();
	if ( strcmp(tag,"command")==0 )
	{
		matlab->command = (char*)malloc(strlen(data)+1);
		strcpy(matlab->command,data);
	}
	else if ( strcmp(tag,"window")==0 )
	{
		if ( strcmp(data,"show")==0 )
			matlab->window = MWD_SHOW;
		else if ( strcmp(data,"hide")==0 )
			matlab->window = MWD_HIDE;
		else if ( strcmp(data,"onerror")==0 )
			matlab->window = MWD_ONERROR;
		else if ( strcmp(data,"ondebug")==0 )
			matlab->window = MWD_ONDEBUG;
		else if ( strcmp(data,"keep")==0 )
			matlab->window = MWD_KEEP;
		else
			gl_error("'%s' is not a valid matlab window disposition", data);
	}
	else if ( strcmp(tag,"output")==0 )
	{
		matlab->output_size = atoi(data);
		if ( matlab->output_size>0 )
			matlab->output_buffer = (char*)malloc(matlab->output_size);
		else
			gl_error("'%s' is not a valid output buffer size", data);
	}
	else if ( strcmp(tag,"workdir")==0 )
	{
		strcpy(matlab->workdir,data);
	}
	else if ( strcmp(tag,"root")==0 )
	{
		if ( strcmp(data,"")==0 ) // no root
			matlab->rootname=NULL;
		else 
		{
			matlab->rootname = (char*)malloc(strlen(data)+1);
			sscanf(data,"%s",matlab->rootname); // use scanf to avoid spaces in root name
		}
	}
	else if ( strcmp(tag,"on_init")==0 )
	{
		size_t len = strlen(data);
		if ( len>0 )
		{
			matlab->init = (char*)malloc(len+1);
			strcpy(matlab->init,data);
		}
	}
	else if ( strcmp(tag,"on_sync")==0 )
	{
		size_t len = strlen(data);
		if ( len>0 )
		{
			matlab->sync = (char*)malloc(len+1);
			strcpy(matlab->sync,data);
		}
	}
	else if ( strcmp(tag,"on_term")==0 )
	{
		size_t len = strlen(data);
		if ( len>0 )
		{
			matlab->term = (char*)malloc(len+1);
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

bool window_show(MATLABLINK *matlab)
{
	// return true if window should be visible
	switch ( matlab->window ) {
	case MWD_HIDE: return false;
	case MWD_SHOW: return true;
	case MWD_KEEP: return true;
	case MWD_ONERROR: return true;
	case MWD_ONDEBUG: return true; // TODO read global debug variable
	default: return false;
	}
}
bool window_kill(MATLABLINK *matlab)
{
	// return true if window engine should be shutdown
	switch ( matlab->window ) {
	case MWD_HIDE: return true;
	case MWD_SHOW: return true;
	case MWD_KEEP: return false;
	case MWD_ONERROR: return false; // TODO read exit status
	case MWD_ONDEBUG: return true; // TODO read global debug variable
	default: return true;
	}
}

EXPORT bool glx_init(glxlink *mod)
{
	gl_verbose("initializing matlab link");
	gl_verbose("PATH=%s", getenv("PATH"));

	// initialize matlab engine
	MATLABLINK *matlab = (MATLABLINK*)mod->get_data();
	matlab->status = 0;
#ifdef WIN32
	if ( matlab->command )
		matlab->engine = engOpen(matlab->command);
	else
		matlab->engine = engOpenSingleUse(NULL,NULL,&matlab->status);
	if ( matlab->engine==NULL )
	{
		gl_error("matlab engine start failed, status code is '%d'", matlab->status);
		return false;
	}
#else
	matlab->engine = engOpen(matlab->command);
	if ( matlab->engine==NULL )
	{
		gl_error("matlab engine start failed");
		return false;
	}
#endif

	// set the output buffer
	if ( matlab->output_buffer!=NULL )
		engOutputBuffer(matlab->engine,matlab->output_buffer,(int)matlab->output_size);

	// setup matlab engine
	engSetVisible(matlab->engine,window_show(matlab));

	gl_debug("matlab link is open");

	// special values needed by matlab
	mxArray *ts_never = mxCreateDoubleScalar((double)(TIMESTAMP)TS_NEVER);
	engPutVariable(matlab->engine,"TS_NEVER",ts_never);
	mxArray *ts_error = mxCreateDoubleScalar((double)(TIMESTAMP)TS_INVALID);
	engPutVariable(matlab->engine,"TS_ERROR",ts_error);
	mxArray *gld_ok = mxCreateDoubleScalar((double)(bool)true);
	engPutVariable(matlab->engine,"GLD_OK",gld_ok);
	mxArray *gld_err = mxCreateDoubleScalar((double)(bool)false);
	engPutVariable(matlab->engine,"GLD_ERROR",gld_err);

	// set the workdir
	if ( strcmp(matlab->workdir,"")!=0 )
	{
#ifdef WIN32
		_mkdir(matlab->workdir);
#else
		mkdir(matlab->workdir,0750);
#endif
		if ( matlab->workdir[0]=='/' )
			matlab_exec(matlab,"cd '%s'", matlab->workdir);
		else
			matlab_exec(matlab,"cd '%s/%s'", getcwd(NULL,0),matlab->workdir);
	}

	// run the initialization command(s)
	if ( matlab->init )
	{
		mxArray *ans = matlab_exec(matlab,"%s",matlab->init);
		if ( ans && mxIsDouble(ans) && (bool)*mxGetPr(ans)==false )
		{
			gl_error("matlab init failed");
			return false;
		}
	}

	if ( matlab->rootname!=NULL )
	{
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
			mxArray *var_struct = NULL;
			mwIndex var_index;
			if ( var==NULL ) continue;

			// do not map module or structured globals
			if ( strchr(var->prop->name,':')!=NULL )
			{
				// ignore module globals here
			}
			else if ( strchr(var->prop->name,'.')!=NULL )
			{
				char struct_name[256];
				if ( sscanf(var->prop->name,"%[^.]",struct_name)==0 )
				{
					gld_property prop(var);
					var_index = mxAddField(global_struct,prop.get_name());
					var_struct = matlab_create_value(&prop);
					if ( var_struct!=NULL )
					{
						//mod->add_copyto(var->prop->addr,mxGetData(var_struct));
						mxSetFieldByNumber(global_struct,0,var_index,var_struct);
					}
				}
			}
			else // simple data
			{
				gld_property prop(var);
				var_index = mxAddField(global_struct,prop.get_name());
				var_struct = matlab_create_value(&prop);
				if ( var_struct!=NULL )
				{
					//mod->add_copyto(var->prop->addr,mxGetData(var_struct));
					mxSetFieldByNumber(global_struct,0,var_index,var_struct);
				}
			}

			// update export list
			if ( var_struct!=NULL )
			{
				mod->set_addr(item,(void*)var_struct);
				mod->set_index(item,(size_t)var_index);
			}
		}

		// add globals structure to gridlabd structure
		mwIndex gridlabd_index = mxAddField(gridlabd_struct,"global");
		mxSetFieldByNumber(gridlabd_struct,0,gridlabd_index,global_struct);

		///////////////////////////////////////////////////////////////////////////
		// build module data
		dims[0] = dims[1] = 1;
		mxArray *module_struct = mxCreateStructArray(2,dims,0,NULL);

		// add modules
		for ( MODULE *module = callback->module.getfirst() ; module!=NULL ; module=module->next )
		{
			// create module info struct
			mwIndex dims[] = {1,1};
			mxArray *module_data = mxCreateStructArray(2,dims,0,NULL);
			mwIndex module_index = mxAddField(module_struct,module->name);
			mxSetFieldByNumber(module_struct,0,module_index,module_data);
			
			// create version info struct
			const char *version_fields[] = {"major","minor"};
			mxArray *version_data = mxCreateStructArray(2,dims,sizeof(version_fields)/sizeof(version_fields[0]),version_fields);
			mxArray *major_data = mxCreateDoubleScalar((double)module->major);
			mxArray *minor_data = mxCreateDoubleScalar((double)module->minor);
			mxSetFieldByNumber(version_data,0,0,major_data);
			mxSetFieldByNumber(version_data,0,1,minor_data);

			// attach version info to module info
			mwIndex version_index = mxAddField(module_data,"version");
			mxSetFieldByNumber(module_data,0,version_index,version_data);

		}
		gridlabd_index = mxAddField(gridlabd_struct,"module");
		mxSetFieldByNumber(gridlabd_struct,0,gridlabd_index,module_struct);

		///////////////////////////////////////////////////////////////////////////
		// build class data
		dims[0] = dims[1] = 1;
		mxArray *class_struct = mxCreateStructArray(2,dims,0,NULL);
		gridlabd_index = mxAddField(gridlabd_struct,"class");
		mxSetFieldByNumber(gridlabd_struct,0,gridlabd_index,class_struct);
		mwIndex class_id[1024]; // index into class struct
		memset(class_id,0,sizeof(class_id));

		// add classes
		for ( CLASS *oclass = callback->class_getfirst() ; oclass!=NULL ; oclass=oclass->next )
		{
			// count objects in this class
			mwIndex dims[] = {0,1};
			for ( item=mod->get_objects() ; item!=NULL ; item=mod->get_next(item) )
			{
				OBJECT *obj = mod->get_object(item);
				if ( obj==NULL || obj->oclass!=oclass ) continue;
				dims[0]++;
			}
			if ( dims[0]==0 ) continue;
			mxArray *runtime_struct = mxCreateStructArray(2,dims,0,NULL);

			// add class 
			mwIndex class_index = mxAddField(class_struct,oclass->name);
			mxSetFieldByNumber(class_struct,0,class_index,runtime_struct);

			// add properties to class
			for ( PROPERTY *prop=oclass->pmap ; prop!=NULL && prop->oclass==oclass ; prop=prop->next )
			{
				mwIndex dims[] = {1,1};
				mxArray *property_struct = mxCreateStructArray(2,dims,0,NULL);
				mwIndex runtime_index = mxAddField(runtime_struct,prop->name);
				mxSetFieldByNumber(runtime_struct,0,runtime_index,property_struct);
			}

			// add objects to class
			for ( item=mod->get_objects() ; item!=NULL ; item=mod->get_next(item) )
			{
				OBJECT *obj = mod->get_object(item);
				if ( obj==NULL || obj->oclass!=oclass ) continue;
				mwIndex index = class_id[obj->oclass->id]++;
				
				// add properties to class
				for ( PROPERTY *prop=oclass->pmap ; prop!=NULL && prop->oclass==oclass ; prop=prop->next )
				{
					gld_property p(obj,prop);
					mxArray *data = matlab_create_value(&p);
					mxSetField(runtime_struct,index,prop->name,data);
				}

				// update export list
				mod->set_addr(item,(void*)runtime_struct);
				mod->set_index(item,(size_t)index);
			}
		}

		///////////////////////////////////////////////////////////////////////////
		// build the object data
		dims[0] = 0;
		for ( item=mod->get_objects() ; item!=NULL ; item=mod->get_next(item) )
		{
			if ( mod->get_object(item)!=NULL ) dims[0]++;
		}
		dims[1] = 1;
		memset(class_id,0,sizeof(class_id));
		const char *objfields[] = {"name","class","id","parent","rank","clock","valid_to","schedule_skew",
			"latitude","longitude","in","out","rng_state","heartbeat","lock","flags"};
		mxArray *object_struct = mxCreateStructArray(2,dims,sizeof(objfields)/sizeof(objfields[0]),objfields);
		mwIndex n=0;
		for ( item=mod->get_objects() ; item!=NULL ; item=mod->get_next(item) )
		{
			OBJECT *obj = mod->get_object(item);
			if ( obj==NULL ) continue;
			class_id[obj->oclass->id]++; // index into class struct

			const char *objname[] = {obj->name&&isdigit(obj->name[0])?NULL:obj->name};
			const char *oclassname[] = {obj->oclass->name};

			if (obj->name) mxSetFieldByNumber(object_struct,n,0,mxCreateCharMatrixFromStrings(mwSize(1),objname));
			mxSetFieldByNumber(object_struct,n,1,mxCreateCharMatrixFromStrings(mwSize(1),oclassname));
			mxSetFieldByNumber(object_struct,n,2,mxCreateDoubleScalar((double)class_id[obj->oclass->id]));
			if (obj->parent) mxSetFieldByNumber(object_struct,n,3,mxCreateDoubleScalar((double)obj->parent->id+1));
			mxSetFieldByNumber(object_struct,n,4,mxCreateDoubleScalar((double)obj->rank));
			mxSetFieldByNumber(object_struct,n,5,mxCreateDoubleScalar((double)obj->clock));
			mxSetFieldByNumber(object_struct,n,6,mxCreateDoubleScalar((double)obj->valid_to));
			mxSetFieldByNumber(object_struct,n,7,mxCreateDoubleScalar((double)obj->schedule_skew));
			if ( isfinite(obj->latitude) ) mxSetFieldByNumber(object_struct,n,8,mxCreateDoubleScalar((double)obj->latitude));
			if ( isfinite(obj->longitude) ) mxSetFieldByNumber(object_struct,n,9,mxCreateDoubleScalar((double)obj->longitude));
			mxSetFieldByNumber(object_struct,n,10,mxCreateDoubleScalar((double)obj->in_svc));
			mxSetFieldByNumber(object_struct,n,11,mxCreateDoubleScalar((double)obj->out_svc));
			mxSetFieldByNumber(object_struct,n,12,mxCreateDoubleScalar((double)obj->rng_state));
			mxSetFieldByNumber(object_struct,n,13,mxCreateDoubleScalar((double)obj->heartbeat));
			mxSetFieldByNumber(object_struct,n,14,mxCreateDoubleScalar((double)obj->lock));
			mxSetFieldByNumber(object_struct,n,15,mxCreateDoubleScalar((double)obj->flags));
			n++;
		}
		gridlabd_index = mxAddField(gridlabd_struct,"object");
		mxSetFieldByNumber(gridlabd_struct,0,gridlabd_index,object_struct);

		///////////////////////////////////////////////////////////////////////////
		// post the gridlabd structure
		matlab->root = gridlabd_struct;
		engPutVariable(matlab->engine,matlab->rootname,matlab->root);
	}

	///////////////////////////////////////////////////////////////////////////
	// build the import/export data
	for ( LINKLIST *item=mod->get_exports() ; item!=NULL ; item=mod->get_next(item) )
	{
		OBJECTPROPERTY *objprop = mod->get_export(item);
		if ( objprop==NULL ) continue;

		// add to published items
		gld_property prop(objprop->obj,objprop->prop);
		item->addr = (mxArray*)matlab_create_value(&prop);
		engPutVariable(matlab->engine,item->name,(mxArray*)item->addr);
	}
	for ( LINKLIST *item=mod->get_imports() ; item!=NULL ; item=mod->get_next(item) )
	{
		OBJECTPROPERTY *objprop = mod->get_import(item);
		if ( objprop==NULL ) continue;

		// check that not already in export list
		LINKLIST *export_item;
		bool found=false;
		for ( export_item=mod->get_exports() ; export_item!=NULL ; export_item=mod->get_next(export_item) )
		{
			OBJECTPROPERTY *other = mod->get_export(item);
			if ( memcmp(objprop,other,sizeof(OBJECTPROPERTY)) )
				found=true;
		}
		if ( !found )
		{
			gld_property prop(objprop->obj,objprop->prop);
			item->addr = (mxArray*)matlab_create_value(&prop);
			engPutVariable(matlab->engine,item->name,(mxArray*)item->addr);
		}
	}

	static int32 matlab_flag = 1;
	gl_global_create("MATLAB",PT_int32,&matlab_flag,PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"indicates that MATLAB is available",NULL);

	return true;
}

bool copy_exports(glxlink *mod)
{
	MATLABLINK *matlab = (MATLABLINK*)mod->get_data();
	LINKLIST *item;

	if ( matlab->rootname!=NULL )
	{
		// update globals
		for ( item=mod->get_globals() ; item!=NULL ; item=mod->get_next(item) )
		{
			mxArray *var_struct = (mxArray*)mod->get_addr(item);
			if ( var_struct!=NULL )
			{
				mwIndex var_index = (mwIndex)mod->get_index(item);
				GLOBALVAR *var = mod->get_globalvar(item);
				gld_property prop(var);
				matlab_set_value(var_struct,&prop);
			}
		}

		// update classes
		// TODO

		// update objects
		for ( item=mod->get_objects() ; item!=NULL ; item=mod->get_next(item) )
		{
			OBJECT *obj = mod->get_object(item);
			if ( obj==NULL ) continue;
			mwIndex index = mod->get_index(item);
			mxArray *runtime_struct = (mxArray*)mod->get_addr(item); 
			
			// add properties to class
			CLASS *oclass = obj->oclass;
			for ( PROPERTY *prop=oclass->pmap ; prop!=NULL && prop->oclass==oclass ; prop=prop->next )
			{
				gld_property p(obj,prop);
				mxArray *data = mxGetField(runtime_struct,index,prop->name);
				matlab_set_value(data,&p);
			}
		}

		// update root data
		engPutVariable(matlab->engine,matlab->rootname,matlab->root);
	}

	// update exports
	for ( item=mod->get_exports() ; item!=NULL ; item=mod->get_next(item) )
	{
		OBJECTPROPERTY *objprop = mod->get_export(item);
		if ( objprop==NULL ) continue;
		gld_property prop(objprop->obj,objprop->prop);
		item->addr = matlab_set_value((mxArray*)item->addr,&prop);
		engPutVariable(matlab->engine,item->name,(mxArray*)item->addr);
	}

	// update imports
	for ( item=mod->get_imports() ; item!=NULL ; item=mod->get_next(item) )
	{
		OBJECTPROPERTY *objprop = mod->get_import(item);
		if ( objprop==NULL ) continue;
		gld_property prop(objprop->obj,objprop->prop);
		item->addr = matlab_set_value((mxArray*)item->addr,&prop);
		engPutVariable(matlab->engine,item->name,(mxArray*)item->addr);
	}

	return true;
}

bool copy_imports(glxlink *mod)
{
	MATLABLINK *matlab = (MATLABLINK*)mod->get_data();
	LINKLIST *item;

	if ( matlab->rootname!=NULL )
	{
		// update globals
		for ( item=mod->get_globals() ; item!=NULL ; item=mod->get_next(item) )
		{
			mxArray *var_struct = (mxArray*)mod->get_addr(item);
			if ( var_struct!=NULL )
			{
				mwIndex var_index = (mwIndex)mod->get_index(item);
				GLOBALVAR *var = mod->get_globalvar(item);
				gld_property prop(var);
				matlab_get_value(var_struct,&prop);
			}
		}
	}

	// update imports
	for ( item=mod->get_imports()==NULL?mod->get_exports():mod->get_imports() ; item!=NULL ; item=mod->get_next(item) )
	{
		OBJECTPROPERTY *objprop = mod->get_import(item);
		if ( objprop==NULL ) continue;
		gld_property prop(objprop->obj,objprop->prop);
		mxArray *data = engGetVariable(matlab->engine,item->name);
		if ( data )
			matlab_get_value(data,&prop);
		else
			gl_warning("unable to read '%s' from matlab", item->name);
		mxDestroyArray(data);
	}

	return true;
}

EXPORT TIMESTAMP glx_sync(glxlink* mod,TIMESTAMP t0)
{
	TIMESTAMP t1 = TS_NEVER;
	MATLABLINK *matlab = (MATLABLINK*)mod->get_data();

	if ( !copy_exports(mod) )
		return TS_INVALID; // error

	if ( matlab->sync )
	{
		mxArray *ans = matlab_exec(matlab,"%s",matlab->sync);
		if ( ans && mxIsDouble(ans) )
		{
				
			double *pVal = (double*)mxGetData(ans);
			if ( pVal!=NULL ) t1 = floor(*pVal);
			if ( t1<TS_INVALID ) t1=TS_NEVER;
		}
	}

	if ( !copy_imports(mod) )
		return TS_INVALID; // error

	return t1;
}

EXPORT bool glx_term(glxlink* mod)
{
	// close matlab engine
	MATLABLINK *matlab = (MATLABLINK*)mod->get_data();
	if ( matlab && matlab->engine )
	{
		if ( matlab->term ) 
		{
			mxArray *ans = matlab_exec(matlab,"%s",matlab->term);
			if ( ans && mxIsDouble(ans) && (bool)*mxGetPr(ans)==false )
			{
				gl_error("matlab term failed");
				return false;
			}
		}
		if ( window_kill(matlab) ) engClose(matlab->engine)==0;
	}
	return true;
}

#else

EXPORT bool glx_create(glxlink *mod, CALLBACKS *fntable)
{
	callback=fntable;
	gl_error("matlab link was not built on system that had matlab installed");
	return false;
}
EXPORT bool glx_settag(glxlink *mod, char *tag, char *data)
{
	gl_error("matlab link was not built on system that had matlab installed");
	return false;
}
EXPORT bool glx_init(glxlink *mod)
{
	gl_error("matlab link was not built on system that had matlab installed");
	return false;
}
EXPORT TIMESTAMP glx_sync(glxlink* mod,TIMESTAMP t0)
{
	gl_error("matlab link was not built on system that had matlab installed");
	return TS_INVALID;
}
EXPORT bool glx_term(glxlink* mod)
{
	gl_error("matlab link was not built on system that had matlab installed");
	return false;
}
#endif // HAVE_MATLAB
