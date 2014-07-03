/** $Id: cmex.c 4738 2014-07-03 00:55:39Z dchassin $
	@file cmex.c
	@addtogroup cmex CMEX module
	@ingroup matlab

	The CMEX module \p gl.dll is created for Matlab users.  

	To add GridLAB-D CMEX module to Matlab's library, use
	the Matlab \p setpath command.

	For a list of supported function, give the command 
	\verbatim >> gl('help') \endverbatim 
	at the Matlab command prompt.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <conio.h>
#include <stdarg.h>
#include <ctype.h>
#include "mex.h"

#define _MAIN_C

#include "globals.h"
#include "module.h"
#include "load.h"
#include "exec.h"
#include "legal.h"
#include "output.h"
#include "object.h"
#include "convert.h"
#include "find.h"

#define MAXNAME 256
#define MATLABDATETIME(X) ((double)(X)/TS_SECOND/86400 + 719529)

typedef enum {MH_GLOBAL=0x19c3, MH_OBJECT=0x82a3, MH_CLASS=0x5d37, MH_MODULE=0xb40f} MEXHANDLETYPE; /* number carefully chosen to mean nothing */

static mxArray *make_handle(MEXHANDLETYPE type, void *ptr)
{
	mxArray *handle = mxCreateNumericMatrix(1,2,mxINT64_CLASS,mxREAL);
	unsigned int64 *data = (unsigned int64*)mxGetPr(handle);
	data[0] = type;
	data[1] = (unsigned int64)ptr;
	return handle;
}

static char *make_fieldname(char *str)
{
	char *p;
	for (p=str; *p!='\0'; p++)
		if (!isalnum(*p))
			*p = '_';
	return str;
}

static OBJECT *find_object(mxArray *handle)
{
	unsigned int64 *data = (unsigned int64*)mxGetPr(handle);
	return data[0]==MH_OBJECT ? (OBJECT*)data[1] : NULL;
}

static GLOBALVAR *find_global(mxArray *handle)
{
	unsigned int64 *data = (unsigned int64*)mxGetPr(handle);
	return data[0]==MH_GLOBAL ? (GLOBALVAR*)data[1] : NULL;
}

static CLASS *find_class(mxArray *handle)
{
	unsigned int64 *data = (unsigned int64*)mxGetPr(handle);
	return data[0]==MH_CLASS ? (CLASS*)data[1] : NULL;
}

static MODULE *find_module(mxArray *handle)
{
	unsigned int64 *data = (unsigned int64*)mxGetPr(handle);
	return data[0]==MH_MODULE ? (MODULE*)data[1] : NULL;
}

static mxArray *get_object_data(OBJECT *obj)
{
	mxArray *plhs[1];

	/* set the standard info */
#define ERROR "(error)"
#define NONE "(none)"
	char *fnames[1024] = {"id","class","parent","rank","clock","latitude","longitude","in_svc","out_svc","flags",NULL}; // };
	int nFields = 0;
	int nData = 0;
	char value[1024];
	PROPERTY *prop;
	mxArray *pId = mxCreateString(convert_from_object(value,sizeof(value),&obj,NULL)?value:ERROR);
	mxArray *pClass = mxCreateString(obj->oclass->name);
	mxArray *pParent = mxCreateString(obj->parent!=NULL&&convert_from_object(value,sizeof(value),&(obj->parent),NULL)?value:NONE);
	mxArray *pRank = mxCreateNumericMatrix(1,1,mxUINT32_CLASS,mxREAL);
	mxArray *pClock = mxCreateString(convert_from_timestamp(obj->clock,value,sizeof(value))?value:ERROR);
	mxArray *pLatitude = mxCreateString(convert_from_latitude(obj->latitude,value,sizeof(value))?value:NONE);
	mxArray *pLongitude = mxCreateString(convert_from_longitude(obj->longitude,value,sizeof(value))?value:NONE);
	mxArray *pInSvc = mxCreateString(convert_from_timestamp(obj->in_svc,value,sizeof(value))?value:ERROR);
	mxArray *pOutSvc = mxCreateString(convert_from_timestamp(obj->out_svc,value,sizeof(value))?value:ERROR);
	mxArray *pFlags = mxCreateString(convert_from_set(value,sizeof(value),(void*)&obj->flags,object_flag_property())?value:ERROR);

	*(OBJECTRANK*)mxGetPr(pRank) = obj->rank;

	/* count the number of header items */
	while (fnames[nFields]!=NULL) nFields++;

	/* count the number of object properties and assign the field names */
	for (prop=class_get_first_property(obj->oclass);  prop!=NULL; prop=class_get_next_property(prop))
		/** @todo don't damage the original fieldname when making it safe for Matlab */
		fnames[nFields+nData++] = make_fieldname(prop->name);

	/* construct the return value */
	plhs[0] = mxCreateStructMatrix(1,1,nFields+nData,fnames);

	/* construct the header fields */
	mxSetFieldByNumber(plhs[0],0,0,pId);
	mxSetFieldByNumber(plhs[0],0,1,pClass);
	mxSetFieldByNumber(plhs[0],0,2,pParent);
	mxSetFieldByNumber(plhs[0],0,3,pRank);
	mxSetFieldByNumber(plhs[0],0,4,pClock);
	mxSetFieldByNumber(plhs[0],0,5,pLatitude);
	mxSetFieldByNumber(plhs[0],0,6,pLongitude);
	mxSetFieldByNumber(plhs[0],0,7,pInSvc);
	mxSetFieldByNumber(plhs[0],0,8,pOutSvc);
	mxSetFieldByNumber(plhs[0],0,9,pFlags);

	/* construct the data fields */
	for (prop=class_get_first_property(obj->oclass);  prop!=NULL; nFields++,prop=class_get_next_property(prop))
	{
		mxArray *pValue;
		if (prop->ptype==PT_double)
		{
			pValue = mxCreateDoubleMatrix(1,1,mxREAL);
			*(double*)mxGetPr(pValue) = *object_get_double(obj,prop);
		}
		else if (prop->ptype==PT_int32)
		{
			pValue = mxCreateDoubleMatrix(1,1,mxREAL);
			*(double*)mxGetPr(pValue) = (double)*object_get_int32(obj,prop);
		}
		else if (prop->ptype==PT_complex)
		{
			complex *pData = object_get_complex(obj,prop);
			pValue = mxCreateDoubleMatrix(1,1,mxCOMPLEX);
			*(double*)mxGetPr(pValue) = pData->r;
			*(double*)mxGetPi(pValue) = pData->i;
		}
		else 
		{
			pValue = mxCreateString(object_get_value_by_name(obj,prop->name,value,sizeof(value))?value:ERROR);
		}
		mxSetFieldByNumber(plhs[0],0,nFields,pValue);
	}
	return plhs[0];
}

static void set_object_data(const mxArray *data)
{
	OBJECT *obj;
	mxArray *pId = mxGetField(data,0,"id");
	char id[256];
	if (pId==NULL)
		output_error("set_object_data(const mxArray *data={...}) did not find a required object id field");
	else if (mxGetString(pId,id,sizeof(id)))
		output_error("set_object_data(const mxArray *data={...}) couldn't read the object id field");
	else if ((obj=object_find_name(id))==NULL)
		output_error("set_object_data(const mxArray *data={id='%s',...}) couldn't find object id", id);
	else
	{
		int i;
		for (i=0; i<mxGetNumberOfFields(data); i++)
		{	
			const char *name;
			const mxArray *pField = mxGetFieldByNumber(data,0,i);
			char value[4096];
			if ((name=mxGetFieldNameByNumber(data,i))==NULL)
				output_error("set_object_data(const mxArray *data={id='%s',...}) couldn't read the name of field %d", id, i);
			else if (strcmp(name,"id")==0 || strcmp(name,"class")==0)
			{	/* these may not be changed */ }
			else if (pField==NULL)
				output_error("set_object_data(const mxArray *data={id='%s',...}) couldn't read the object field '%s' for object '%s'", id, name);
			else if (mxIsChar(pField) && mxGetString(pField,value,sizeof(value)))
				output_error("set_object_data(const mxArray *data={id='%s',...}) couldn't read the string value '%s' from field '%s'", id, value, name);
			else if (mxIsDouble(pField) && sprintf(value,"%lg",*(double*)mxGetPr(pField))<1)
				output_error("set_object_data(const mxArray *data={id='%s',...}) couldn't read the double value '%lg' from field '%s'", id, *(double*)mxGetPr(pField), name);
			else if (mxIsComplex(pField) && sprintf(value,"%lg%+lgi",*(double*)mxGetPr(pField),*(double*)mxGetPi(pField))<1)
				output_error("set_object_data(const mxArray *data={id='%s',...}) couldn't read the complex value '%lg%+lgi' from field '%s'", id, *(double*)mxGetPr(pField), *(double*)mxGetPi(pField), name);
			else if (mxIsUint32(pField) && sprintf(value,"%lu",*(unsigned int*)mxGetPr(pField))<1)
				output_error("set_object_data(const mxArray *data={id='%s',...}) couldn't read the uint32 value '%lu' from field '%s'", id, *(unsigned int*)mxGetPr(pField), name);
			else if (strcmp(value,ERROR)==0)
				output_error("set_object_data(const mxArray *data={id='%s',...}) couldn't use error value '%s'", id, value);
			else if (strcmp(value,NONE)==0 && strcpy(value,"")==NULL)
				output_error("set_object_data(const mxArray *data={id='%s',...}) couldn't clear empty value '%s'", id, value);
			else if (!object_set_value_by_name(obj,name,value))
				output_error("set_object_data(const mxArray *data={id='%s',...}) couldn't read the value '%s' into property '%s'", id, value, name);
		}
	}
}

static int cmex_printerr(char *format, ...)
{
	int count=0;
	static char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	count += vsprintf(buffer,format,ptr);
	va_end(ptr);
	if (strncmp(buffer,"ERROR",5)==0 || strncmp(buffer,"FATAL",5)==0)
		mexErrMsgTxt(buffer);
	else if (strncmp(buffer,"WARN",4)==0)
		mexWarnMsgTxt(buffer);
	else
		mexPrintf("%s",buffer);
	return count;
}

void cmex_object_list(int nlhs, mxArray *plhs[], /**< entlist */
					  int nrhs, const mxArray *prhs[] ) /**< () */
{
	OBJECT *obj;
	char criteria[1024]="(undefined)";
	FINDPGM *search = NULL;
	char *fields[] = {"name","class","parent","flags","location","service","rank","clock","handle"};
	FINDLIST *list = NULL;
	if (nrhs>0 && mxGetString(prhs[0],criteria,sizeof(criteria))!=0)
		output_error("gl('list',type='object'): unable to read search criteria (arg 2)");
	else if (nrhs>0 && (search=find_mkpgm(criteria))==NULL)
		output_error("gl('list',type='object'): unable to run search '%s'",criteria);
	else if (search==NULL && (list=find_objects(NULL,NULL))==NULL)
		output_error("gl('list',type='object'): unable to obtain default list");
	else if (list==NULL && (list=find_runpgm(NULL,search))==NULL)
		output_error("gl('list',type='object'): unable search failed");
	else if ((plhs[0] = mxCreateStructMatrix(list->hit_count,1,sizeof(fields)/sizeof(fields[0]),fields))==NULL)
		output_error("gl('list',type='object'): unable to allocate memory for result list");
	else
	{
		unsigned int n;
		for (n=0, obj=find_first(list); obj!=NULL; n++, obj=find_next(list,obj))
		{
			char tmp[1024];
			mxArray *data;
			double *pDouble;
			unsigned int *pInt;
			mxSetFieldByNumber(plhs[0], n, 0, mxCreateString(object_name(obj)));
			mxSetFieldByNumber(plhs[0], n, 1, mxCreateString(obj->oclass->name));
			mxSetFieldByNumber(plhs[0], n, 2, mxCreateString(obj->parent?object_name(obj->parent):NONE));
			mxSetFieldByNumber(plhs[0], n, 3, mxCreateString(convert_from_set(tmp,sizeof(tmp),&(obj->flags),object_flag_property())?tmp:ERROR));
			pDouble = mxGetPr(data=mxCreateDoubleMatrix(1,2,mxREAL)); pDouble[0] = obj->longitude; pDouble[1] = obj->latitude;
			mxSetFieldByNumber(plhs[0], n, 4, data);
			pDouble = mxGetPr(data=mxCreateDoubleMatrix(1,2,mxREAL)); pDouble[0] = (double)obj->in_svc/TS_SECOND; pDouble[1] = (double)obj->out_svc/TS_SECOND;
			mxSetFieldByNumber(plhs[0], n, 5, data);
			pInt = (unsigned int*)mxGetPr(data=mxCreateNumericMatrix(1,1,mxINT32_CLASS,mxREAL)); pInt[0] = obj->rank;
			mxSetFieldByNumber(plhs[0], n, 6, data);
			pDouble = mxGetPr(data=mxCreateDoubleMatrix(1,1,mxREAL)); pDouble[0] = (double)obj->clock/TS_SECOND; 
			mxSetFieldByNumber(plhs[0], n, 7, data);
			mxSetFieldByNumber(plhs[0], n, 8, make_handle(MH_OBJECT,obj));
		}
	}
}

/** Get a list of entities
	\verbatim entlist = gl('list',type[, criteria]) \endverbatim
 **/
void cmex_list(int nlhs, mxArray *plhs[], /**< entlist */
			   int nrhs, const mxArray *prhs[] ) /**< () */
{
	char type[32];
	if (nlhs>1)
		output_error("gl('list',type=...): returns only one value");
	else if (nlhs<1)
		return; /* nothing to do */
	else if (nrhs<1)
		output_error("gl('list',type=...): needs a type (arg 1)");
	else if (mxGetString(prhs[0],type,sizeof(type))==1)
		output_error("gl('list',type=...): type (arg 1) should be a string");
	else if (strcmp(type,"object")==0)
		cmex_object_list(nlhs,plhs,nrhs-1,prhs+1);
	else if (strcmp(type,"global")==0)
		output_error("gl('list',type='%s'): type (arg 1) not implemented",type);
	else if (strcmp(type,"class")==0)
		output_error("gl('list',type='%s'): type (arg 1) not implemented",type);
	else if (strcmp(type,"module")==0)
		output_error("gl('list',type='%s'): type (arg 1) not implemented",type);
	else
		output_error("gl('list',type='%s'): type (arg 1) not recognized",type);

	return;
}

/** Get the version of GridLAB-D CMEX running
	\verbatim [major minor] = gl('version') \endverbatim
 **/
void cmex_version(int nlhs, mxArray *plhs[], /**< [major minor] */
				  int nrhs, const mxArray *prhs[] ) /**< () */
{
	/* setup result array */
	if (nlhs>0)
	{
		double *res = NULL;
		plhs[0] = mxCreateDoubleMatrix(1, 2, mxREAL);
		res = mxGetPr(plhs[0]);
		*res++ = global_version_major;
		*res++ = global_version_minor;
	}
	else
		legal_notice();

	return;
}

/** Define a new global variable
	\verbatim gl('global',name,value) \endverbatim
 **/
void cmex_global(int nlhs, mxArray *plhs[], /**< () */
				   int nrhs, const mxArray *prhs[] )  /**< (name,value) */
{
	char name[256];
	size_t nDims;
	const mwSize *dim;
	if (nlhs!=0)
		output_error("global does not return a value");
	else if (nrhs!=2)
		output_error("global requires a name (arg 1) and an array (arg 2)");
	else if (!mxIsChar(prhs[0]) || mxGetString(prhs[0],name,sizeof(name))==1)
		output_error("global name (arg 1) must be a string");
	else if ((nDims=(size_t)mxGetNumberOfDimensions(prhs[1]))<2)
		output_error("dimensions of array (arg 2) are not valid");
	else if ((dim=mxGetDimensions(prhs[1]))==NULL)
		output_error("dimensions of array (arg 2) are not available");
	else
	{
		size_t size=1;
		while (nDims-->0) 
			size*=dim[nDims];
		if (mxIsChar(prhs[1]))
		{
			char *buffer = malloc(1024);
			if(mxGetString(prhs[1],buffer,1024) && global_create(name,PT_char1024,buffer,PT_SIZE,1,NULL)==NULL)
				output_error("unable to register string variable '%s' in globals", name);
		}
		else if (mxIsDouble(prhs[1]))
		{
			double *x = malloc(sizeof(double)*size);
			unsigned int i;
			memset(x,0,sizeof(double)*size);
			for (i=0; i<size; i++)
				x[i] = ((double*)mxGetPr(prhs[1]))[i];
			if (global_create(name,PT_double,x,PT_SIZE,size,NULL)==NULL)
				output_error("unable to register double array variable '%s' in globals", name);
		}
		else if (mxIsComplex(prhs[1]))
		{
			complex *x = (complex*)malloc(sizeof(complex)*size);
			unsigned int i;
			memset(x,0,sizeof(complex)*size);
			for (i=0; i<size; i++)
			{
				x[i].r = ((double*)mxGetPr(prhs[1]))[i];
				x[i].i = ((double*)mxGetPi(prhs[1]))[i];
				x[i].f = I;
			}
			if (global_create(name,PT_complex,x,PT_SIZE,size,NULL)==NULL)
				output_error("unable to register complex array variable '%s' in globals", name);
			free(x);
		}
		else if (mxIsStruct(prhs[1]))
		{
			output_error("unable to register struct variable '%s' in globals", name);
		}
		else 
			output_error("array (arg 2) type is not supported");
	}
}

/** Set an environment variable
	\verbatim gl('setenv',name,value) \endverbatim
 **/
void cmex_setenv(int nlhs, mxArray *plhs[], /**< () */
				   int nrhs, const mxArray *prhs[] ) /**< (name, value) */
{
	if (nrhs>1)
	{
		char name[1024];
		char value[1024];
		if (!mxIsChar(prhs[0]))
			output_error("variable name is not a string");
		else if (!mxIsChar(prhs[1]))
			output_error("value is not a string");
		else if (nlhs>0)
			output_error("setenv does not return a value");
		else if (mxGetString(prhs[0],name,sizeof(name))!=0)
			output_error("variable name too long");
		else if (mxGetString(prhs[1],value,sizeof(value))!=0)
			output_error("value too long");
		else 
		{
			char env[2050];
			if (sprintf(env,"%s=%s", name, value)<0)
				output_error("unable to make environment value");
			else if (putenv(env)<0)
				output_error("unable to save environment value");
		}
	}
	else
		output_error("environment name and value not specified");
	return;
}

/** Get an environment variable
	\verbatim gl('getenv',name) \endverbatim
 **/
void cmex_getenv(int nlhs, mxArray *plhs[], /**< () */
				   int nrhs, const mxArray *prhs[] ) /**< (name, value) */
{
	char name[1024];
	if (!mxIsChar(prhs[0]))
		output_error("variable name is not a string");
	else if (mxGetString(prhs[0],name,sizeof(name))!=0)
		output_error("variable name too long");
	else 
	{
		char *value = getenv(name);
		if (value==NULL)
			output_error("%s is not defined",name);
		else if (nlhs==0)
			output_message("%s='%s'",name,value);
		else if (nlhs==1)
			plhs[0] = mxCreateString(value);
		else
			output_error("getenv does not return more than one value");
	}
	return;
}

/** Create an object
    \verbatim gl('create', class, property, value, ...) \endverbatim
 **/
void cmex_create(int nlhs, mxArray *plhs[], /**< {properties} */
				 int nrhs, const mxArray *prhs[]) /**< (class, property1, value1, property2, value2, ..., propertyN, valueN) */
{
	CLASS *oclass;
	OBJECT *obj;
	CLASSNAME classname;
	unsigned int num_properties = (nrhs-1)/2;

	/* check return value */
	if (nlhs>1)
		output_error("create only returns one struct");
	/* check class name */
	else if (!mxIsChar(prhs[0]))
		output_error("class name (arg 1) must be a string");

	/* get class name */
	else if (mxGetString(prhs[0],classname,sizeof(classname)))
		output_error("unable to read class name (arg 1)");

	/* check the number of properties */
	else if (num_properties!=(nrhs/2))
		output_error("an object property value is missing");

	/* create object */
	else if ((oclass=class_get_class_from_classname(classname))==NULL)
		output_error("class '%s' is not registered", classname);

	/* create the object */
	else if (oclass->create(&obj,NULL)==FAILED)
		output_error("unable to create object of class %s", classname);

	/* copy properties */
	else
	{
		unsigned int i;
		for (i=0; i<num_properties; i++)
		{
			char name[256];
			char value[1024];
			unsigned int n=i*2+1;
			const mxArray *p[] = {prhs[n],prhs[n+1]};
			if (!mxIsChar(p[0]))
				output_error("property name (arg %d) must be a string", n);
			else if (mxGetString(p[0],name,sizeof(name)))
				output_error("property name (arg %d) couldn't be read", n);
			else if (mxIsChar(p[1]) && mxGetString(p[1],value,sizeof(value)))
				output_error("property %s (arg %d) value couldn't be read", name, n);
			else if (mxIsDouble(p[1]) && sprintf(value,"%lg",*(double*)mxGetPr(p[1]))<1)
				output_error("property %s (arg %d) value couldn't be converted from double", name, n);
			else if (mxIsComplex(p[1]) && sprintf(value,"%lg%+lgi",*(double*)mxGetPr(p[1]),*(double*)mxGetPi(p[1]))<1)
				output_error("property %s (arg %d) value couldn't be converted from complex", name, n);
			else if (object_set_value_by_name(obj,name,value)==0)
				output_error("property %s (arg %d) couldn't be set to '%s'", name, n, value);
			else if (nlhs>0 && (plhs[0]=get_object_data(obj))==NULL)
				output_error("couldn't get object data for %s", name);
		}
	}
}

/** Import an GLM or XML model
	\verbatim gl('load',filename) \endverbatim
 **/
void cmex_load(int nlhs, mxArray *plhs[], /**< () */
			   int nrhs, const mxArray *prhs[] ) /**< (filename) */
{
	if (nrhs>0)
	{
		char fname[1024];
		if (!mxIsChar(prhs[0]))
			output_error("Model name is not a string");
		else if (nlhs>0)
			output_error("load does not return a value");
		else if (mxGetString(prhs[0],fname,sizeof(fname))!=0)
			output_error("Model name too long");
		else if (loadall(fname)==FAILED)
			output_error("Model load failed");
		else 
			output_message("Model %s loaded ok", fname);
	}
	else
		output_error("Module not specified");
	return;
}

/** Start the GridLAB-D simulation executive
	\verbatim gl('start') \endverbatim
 **/
void cmex_start(int nlhs, mxArray *plhs[], /**< () */
				  int nrhs, const mxArray *prhs[] ) /**< () */
{
	global_keep_progress = 1;
	if (exec_start()==FAILED)
		output_error("Simulation failed!");
	return;
}
/** Force a module to be loaded immediately (modules are loaded automatically)
	\verbatim gl('module',modulename) \endverbatim
 **/

void cmex_module(int nlhs, mxArray *plhs[], /**< () */
				   int nrhs, const mxArray *prhs[] ) /**< (modulename) */
{
	if (nrhs>0)
	{
		char fname[1024];
		MODULE *mod;
		if (!mxIsChar(prhs[0]))
			output_error("Module name is not a string");
		else if (nlhs>1)
			output_error("Only one return value is possible");
		else if (mxGetString(prhs[0],fname,sizeof(fname))!=0)
			output_error("Module name too long");
		else if ((mod=module_find(fname))==NULL && (mod=module_load(fname,0,NULL))==NULL)
			output_error("Module load failed");
		else if (nlhs=0)
			output_message("Module '%s(%d.%d)' loaded ok", mod->name, mod->major, mod->minor);
		else
		{
			/* set the standard info */
			char *fnames[256] = {"handle","name","major","minor"};
			mxArray *name = mxCreateString(mod->name);
			mxArray *handle = mxCreateNumericMatrix(1,1,sizeof(int32)==sizeof(int)?mxUINT32_CLASS:mxUINT64_CLASS,mxREAL);
			mxArray *major = mxCreateNumericMatrix(1,1,mxUINT8_CLASS,mxREAL);
			mxArray *minor = mxCreateNumericMatrix(1,1,mxUINT8_CLASS,mxREAL);
			mxArray *value[256];
			unsigned int64 *pHandle = mxGetData(handle);
			unsigned char *pMajor = mxGetData(major);
			unsigned char *pMinor = mxGetData(minor);
			char32 varname="";
			int nFields=4;
			char vnames[256][32];
			*pHandle = (unsigned int64)mod->hLib;
			*pMajor = (unsigned char)mod->major;
			*pMinor = (unsigned char)mod->minor;

			/* get the module data */
			while (module_getvar(mod,varname,NULL,0))
			{
				char32 buffer;
				if (module_getvar(mod,varname,buffer,sizeof(buffer)) && nFields<sizeof(fname)/sizeof(fname[0]))
				{
					double *pVal;
					output_verbose("module variable %s = '%s'", varname, buffer);
					value[nFields] = mxCreateDoubleMatrix(1,1,mxREAL);
					pVal = mxGetPr(value[nFields]);
					*pVal = atof(buffer);
					strcpy(vnames[nFields],varname);
					fnames[nFields] = vnames[nFields];
					nFields++;
				}
			}

			/* construct the result */
			plhs[0] = mxCreateStructMatrix(1,1,nFields,fnames);
			mxSetFieldByNumber(plhs[0],0,0,handle);
			mxSetFieldByNumber(plhs[0],0,1,name);
			mxSetFieldByNumber(plhs[0],0,2,major);
			mxSetFieldByNumber(plhs[0],0,3,minor);
			while (nFields-->4)
				mxSetFieldByNumber(plhs[0],0,nFields,value[nFields]);

		}
	}
	else
		output_error("Module not specified");
	return;
}

/** Set a value
	\verbatim gl('set',id,property,value) \endverbatim
 **/
void cmex_set(int nlhs, mxArray *plhs[], /**< () */
				int nrhs, const mxArray *prhs[]) /**< (object_id,property_name,value) */
{
	if (nlhs>0)
		output_error("set does not return a value");
	else if (nrhs==1 && mxIsStruct(prhs[0]))
		set_object_data(prhs[0]);
	else if (nrhs!=3)
		output_error("set requires either a structure, or object id, property name and value");
	else if (!mxIsChar(prhs[0]))
		output_error("object id (arg 0) must be a string");
	else if (mxIsChar(prhs[1]))
	{
		char value[1024];
		PROPERTY* prop;
		OBJECT *obj=NULL;
		GLOBALVAR *var=NULL;
		if (mxGetString(prhs[0],value,sizeof(value))!=0)
			output_error("object name (arg 0) too long");
		else if (strcmp(value,"global")==0)
		{
			if (!mxIsChar(prhs[1]))
				output_error("global variable name is not a string");
			else if (!mxIsChar(prhs[2]))
				output_error("global value is not is not a string");
			else
			{
				char name[32], value[1024];
				mxGetString(prhs[1],name,sizeof(name));
				mxGetString(prhs[2],value,sizeof(value));
				if (global_setvar(name,value)!=SUCCESS)
					output_error("unable to set global '%s' to '%s'", name,value);
			}
		}
		else if (convert_to_object(value,&obj,NULL)==0)
			output_error("object (arg 0) %s not found",value);
		else if (mxGetString(prhs[1],value,sizeof(value))!=0)
			output_error("property name (arg 1) too long");
		else if ((prop=object_get_property(obj,value))==NULL)
			output_error("property name (arg 1) %s not found in object %s:%d", value,obj->oclass->name, obj->id);
		else if (mxIsChar(prhs[2]))
		{
			if (mxGetString(prhs[2],value,sizeof(value))!=0)
				output_error("value (arg 2) is too long");
			else if (object_set_value_by_name(obj,prop->name,value)==0)
				output_error("unable to set %s:%d/%s to %s", obj->oclass->name, obj->id, prop->name, value);
			else
				return; /* ok */
		}
		else if (mxIsDouble(prhs[2]) && prop->ptype==PT_double)
		{
			double v = *mxGetPr(prhs[2]);
			if (prop->ptype==PT_double && object_set_double_by_name(obj,prop->name,v)==0)
				output_error("unable to set %s:%d/%s to %lg", obj->oclass->name, obj->id, prop->name, v);
		}
		else if (mxIsComplex(prhs[2]) || mxIsDouble(prhs[2]))
		{
			sprintf(value,"%lg%+lgi",*mxGetPr(prhs[2]),mxIsComplex(prhs[2])?*mxGetPi(prhs[2]):0);
			if (object_set_value_by_name(obj,prop->name,value)==0)
				output_error("unable to set %s:%d/%s to %s", obj->oclass->name, obj->id, prop->name, value);
		}
		else 
			output_error("value (arg 2) has an unsupported data type");
	}
	else
		output_error("property or data (arg 1) type is not valid");
}

/** Get a value (object, clock, or global)
	\verbatim gl('get',name) \endverbatim
 **/
void cmex_get(int nlhs, mxArray *plhs[], /**< {data} */
				int nrhs, const mxArray *prhs[] ) /**< (name) */
{
	if (nrhs>0)
	{
		char name[1024];
		OBJECT *obj=NULL;
		if (!mxIsChar(prhs[0]))
			output_error("entity name (arg 1) is not a string");
		else if (nlhs>1)
			output_error("only one return value is possible");
		else if (mxGetString(prhs[0],name,sizeof(name))!=0)
			output_error("object name too long");
		else if (strcmp(name,"clock")==0)
		{
			char *fnames[] = {"timestamp","timestring","timezone"};
			char buffer[256];
			mxArray *pTimestamp = mxCreateDoubleMatrix(1,1,mxREAL);
			mxArray *pTimestring = mxCreateString(convert_from_timestamp(global_clock,buffer,sizeof(buffer))?buffer:"(error)");
			mxArray *pTimezone = mxCreateString(timestamp_current_timezone());

			*(double*)mxGetPr(pTimestamp) = ((double)global_clock)/TS_SECOND;
			plhs[0] = mxCreateStructMatrix(1,1,sizeof(fnames)/sizeof(fnames[0]),fnames);
			mxSetFieldByNumber(plhs[0],0,0,pTimestamp);
			mxSetFieldByNumber(plhs[0],0,1,pTimestring);
			mxSetFieldByNumber(plhs[0],0,2,pTimezone);
		}
		else if (strcmp(name,"property")==0 && nrhs>1)
		{
			if (mxGetString(prhs[1],name,sizeof(name))!=0)
				output_error("missing property name");
			else
			{
				char classname[256];
				char propname[256];
				if (sscanf(name,"%[^.].%s",classname,propname)==2)
				{
					CLASS *pClass = class_get_class_from_classname(classname);
					if (pClass)
					{
						PROPERTY *pProp = class_find_property(pClass,propname);
						if (pProp)
						{
							char *fields[] = {"class","name","type","size","access","unit","delegation","keywords"};
							int fn = 0;
							mxArray *oclass = mxCreateString(classname);
							mxArray *prop = mxCreateString(pProp->name);
							mxArray *type = mxCreateString(class_get_property_typename(pProp->ptype));
							mxArray *size = mxCreateDoubleMatrix(1,1,mxREAL); 
							mxArray *access = mxCreateString("(na)"); /** @todo implement get_property access info (ticket #187) */
							mxArray *unit = mxCreateString(pProp->unit->name);
							mxArray *delegation = mxCreateString(pProp->delegation?pProp->delegation->oclass->name:"(none)");
							mxArray *keywords = mxCreateString("(na)"); /** @todo implement get_property keywords (ticket #188) */
							*(mxGetPr(size)) = pProp->size==0?1:pProp->size;
							plhs[0] = mxCreateStructMatrix(1,1,sizeof(fields)/sizeof(fields[0]),fields);
							mxSetFieldByNumber(plhs[0],0,fn++,oclass);
							mxSetFieldByNumber(plhs[0],0,fn++,prop);
							mxSetFieldByNumber(plhs[0],0,fn++,type);
							mxSetFieldByNumber(plhs[0],0,fn++,size);
							mxSetFieldByNumber(plhs[0],0,fn++,access);
							mxSetFieldByNumber(plhs[0],0,fn++,unit);
							mxSetFieldByNumber(plhs[0],0,fn++,delegation);
							mxSetFieldByNumber(plhs[0],0,fn++,keywords);
						}
						else
							output_error("property %s is not found in class %s", propname,classname);
					}
					else
						output_error("class %s is not found");
				}
				else
					output_error("property name not in class.name format");
			}
		}
		else if ((convert_to_object(name,&obj,NULL))==0)
		{
			GLOBALVAR *var = global_find(name);
			if (var==NULL)
				output_error("entity '%s' not found", name);
			else if (var->prop->ptype==PT_double)
			{
				size_t size = var->prop->size?var->prop->size:1;
				plhs[0] = mxCreateDoubleMatrix(size,1,mxREAL);
				memcpy(mxGetPr(plhs[0]),(void*)var->prop->addr,sizeof(double)*size);
			}
			else if (var->prop->ptype==PT_int32)
			{
				size_t size = var->prop->size?var->prop->size:1;
				plhs[0] = mxCreateDoubleMatrix(size,1,mxREAL);

				memcpy(mxGetPr(plhs[0]),(void*)var->prop->addr,sizeof(double)*size);
			}
			else if (var->prop->ptype!=PT_double)
				output_error("cannot retrieve globals that are of type %s",class_get_property_typename(var->prop->ptype));
		}
		else if ((plhs[0]=get_object_data(obj))==NULL)
			output_error("unable to extract %s data", name);
	}
	else
		output_error("object not specified");
	return;
}

/************************************************************
 *
 * MAIN MEX FUNCTION
 *
 ************************************************************/

typedef struct
{
	char *name;
	void (*call)(int, mxArray*[],int,const mxArray*[]);
	char *brief;
	char *detail;
} CMDMAP;
static CMDMAP cmdMap[] = 
{
	{"setenv",cmex_setenv,"Set environment",
		"\tgl('setenv','name','value')\n"
		"  Set an environment variable\n"
	},

	{"getenv",cmex_getenv,"Get environment",
		"\tgl('getenv','name')\n"
		"  Get an environment variable\n"
	},

	{"start",cmex_start,"Start simulation",
		"\tgl('start')\n"
		"  Start the GridLAB simulation.\n"
	},

	{"load",cmex_load,"Import GridLAB model",
		"\tgl('load',filename)\n"
		"  Import the GridLAB model in <filename>.\n"
	},

	{"module",cmex_module,"Load GridLAB module",
		"\tgl('module',modulename)\n"
		"  Load the GridLAB module named <modulename>.\n"
	},

	{"version",cmex_version,"Get GridLAB version information",
		"\tgl('version')\n"
		"  Returns the major, minor, and build numbers of the installed version of GridLAB.\n"
	},

	{"get",cmex_get,"Get an object's data",
		"\tgl('get',name)\n"
		"  Returns the object structure\n"
	},

	{"set",cmex_set,"Set an object's data",
		"\tgl('set',id,property,value)\n"
		"  Returns the object structure\n"
	},
	{"create",cmex_create,"Create an object",
		"\tgl('create',class, property1,value1, property2,value2, ..., propertyN,valueN)\n"
		"  Returns the object structure\n"
	},
	{"global",cmex_global,"Create a global variable",
		"\tgl('global',name,array)\n"
		"  Returns nothing\n"
	},

	{"list",cmex_list,"Obtain a list of entities",
		"\tgl('list',type)\n"
		"  Returns a list of entities\n"
	},
};

void mexFunction( int nlhs, mxArray *plhs[],
		  int nrhs, const mxArray *prhs[] )
{
	static first = 1;
	char key[MAXNAME];
	int i;
	
	if (first==1)
	{
		first = 0;

		/* prevent Matlab from clearing GridLAB */
		mexLock(); 

		/* register Matlab output routines */
		output_set_stdout(mexPrintf);
		output_set_stderr(cmex_printerr);

		/* display legal stuff */
		legal_license();

		/* initialize GridLAB */
		exec_init();
	}

	/* check number of input arguments */
	if (nrhs<1)
	{
		output_error("Use gl('help') for a list of commands.");
		return;
	}
	
	/* check type of first argument */
	if (!mxIsChar(prhs[0]))
	{
		output_error("token must be a string");
		return;
	}
	
	/* read first argument */
	if (mxGetString(prhs[0],key,sizeof(key))!=0)
		output_warning("GridLAB key string too long");
		
	/* scan command map to find call function */
	for (i=0; i<sizeof(cmdMap)/sizeof(cmdMap[0]); i++)
	{
		if (strcmp(key,cmdMap[i].name)==0)
		{
			if (cmdMap[i].call == NULL) /* help request */
			{
				int j;
				if (nrhs==1)
				{
					output_raw("Available top-level commands\n");
					for (j=0; j<sizeof(cmdMap)/sizeof(cmdMap[0]); j++)
						output_raw("\t%s\t%s\n", cmdMap[j].name, cmdMap[j].brief);
					output_raw("Use gl('help',command) for details\n");
					return;
				}
				else if (mxIsChar(prhs[1]))
				{
					char cmd[MAXNAME];
					if (mxGetString(prhs[1],cmd,sizeof(cmd))!=0)
						output_warning("command string too long to read fully");

					for (j=0; j<sizeof(cmdMap)/sizeof(cmdMap[0]); j++)
					{
						if (strcmp(cmd,cmdMap[j].name)==0)
						{
							output_raw("Help for command '%s'\n\n%s\n", cmd, cmdMap[j].detail ? cmdMap[j].detail : "\tNo details available\n");
							return;
						}
					}
					output_error("Command '%s' does not exist", cmd);
					return;
				}
				else
				{
					output_error("command must be a string");
					return;
				}
			}
			else
			{
				(cmdMap[i].call)(nlhs,plhs,nrhs-1,prhs+1);
				return;
			}
		}
	}

	/* function not found */
	{	int nret = nlhs;
		output_error("unrecognized GridLAB operation--gl('help') for list");
		while (nret-->0)
			plhs[nret] = mxCreateDoubleMatrix(0,0,mxREAL);
	}
	return;
}

/** @} */
