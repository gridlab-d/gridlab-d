/** $Id: init.cpp 4738 2014-07-03 00:55:39Z dchassin $
	@file init.cpp
	@defgroup matlab_objects Matlab objects
	@ingroup modules

	Matlab objects

	Objects can be defined in Matlab using the \p matlab module.  Matlab
	classes are defined using the module GLM statement \verbatim module matlab::CLASSNAME; \endverbatim
	To define an object class in Matlab, you then refer to the class name, omitting the
	\p matlab:: module prefix of the name.  The follow methods must be defined for each
	object used in an \p matlab module:
	- \e classname where \e classname is the name you give to the Matlab class (see Matlab \p class syntax).
	  This is used as the default constructor as well as the converter from char.
	- \p create is used to create a new Matlab object for GridLAB-D
	- \p init is used to initialize a new Matlab object for GridLAB-D
	- \p presync is used to perform \p PRETOPDOWN synchronization for GridLAB-D
	- \p sync is used to perform \p BOTTOMUP synchronization for GridLAB-D
	- \p postsync is used to perform \p POSTTOPDOWN synchronization for GridLAB-D
	- \p char is used to convert to char
	- \p display is used to show the object's data during debugging

	The constructor must be implemented as follows (replace \p CLASSNAME with the classes true name)
	@verbatim
	function r = CLASSNAME(varargin)

	if (nargsin==0) % setup call from core
		% establish the pass configuration
		global passconfig;
		passconfig = 'PRETOPDOWN|BOTTOMUP|POSTTOPDOWN'; % omit those you don't need

		% create all your variables and assign them default values
		r.var1 = 1.23;
		r.var2 = 'Defaults set';
	else
		% create object directly from varargin (char must be supported)
	end

	% set the class
	r = class(r,'CLASSNAME');

	end	@endverbatim

	The \p create method must be implemented as follows
	@verbatim
	function r = create(my)

	% create your context-free initial values
	my.var1 = my.var1 + 0.11;
	my.var2 = 'Create done';

	r = my;
	end		@endverbatim

	The \p init method must be implemented as follows
	@verbatim
	function r = init(my,parent)

	% set your context-dependent values
	my.var1 = my.var1 + 0.11;
	my.var2 = 'Initialized';

	r = my;
	end	@endverbatim

	The \p presync, \p sync, and \p postsync methods must be implemented and must be implemented
	if defined in \p passconfig.  Implementation of \p {pre,post}sync is always as follows

	@verbatim
	function [r t2] = sync(my,t0,t1) % define separate methods for each type of sync

	% update variables as appropriate
	my.var1 = my.var1+0.11;
	my.var2 = 'Sync';

	r = my;
	t2 = NEVER; % set t2 as appropriate (in seconds)
	end	@endverbatim

	The \p char implementation is required by GridLAB-D's internal data conversion routines and 
	must convert the object's data to a character string that can be embedded in GLM and XML files.
	The implementation must be as follows

	@verbatim
	function s = char(my)
	
	s = sprintf('var1="%g kV"; var2="%s";',my.var1,my.var2); % adjust for your variables
	
	end	@endverbatim

	The \p display implementation is necessary for Matlab's internal data conversion routines and
	must convert the object's data to a sequences of output lines that can be displayed on the
	screen.  The implementation must be as follows

	@verbatim
	function display(obj)

	disp(' ');
	% display your variables in gridlab debugger style 
	disp(['object ' inputname(1), ' {']);
	disp(['  var1 ' sprintf('%g',obj.var1) ' kV;']);
	disp(['  var2 "' obj.var2 '";']);
	disp('}');
	disp(' ');

	end	@endverbatim

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include "gridlabd.h"

#include "matlab.h"
#include "engine.h"

// matlab implementation data
Engine *engine=NULL;
char output[4096];
PASSCONFIG passconfig;
CLASS *oclass=NULL;
mxArray *defaults = NULL;
char *matlab_server = ""; // use "standalone" to engage single use servers for each run
bool debugmode = false;

//#define TOSERIAL(DT) (719529.0+(double)(DT)/(double)TS_SECOND/86400.0)
//#define FROMSERIAL(DN) ((TIMESTAMP)((DN-719529)*86400*TS_SECOND))
#define TOSERIAL(DT) ((double)DT/(double)TS_SECOND)
#define FROMSERIAL(DN) (TIMESTAMP)((double)DN*(double)TS_SECOND)

void gl_matlab_output(void)
{
	if (strncmp(output,"???",3)==0)
		gl_error("[MATLAB] %s", output+4);
	else if (output[0]!='\0')
		gl_debug("[MATLAB] %s", output);
}

int object_from_string(void *addr, char *value)
{
	mxArray **data = (mxArray**)addr;
	return 0;
}
int object_to_string(void *addr, char *value, int size)
{
	mxArray **my = (mxArray**)addr;
	if (*my!=NULL)
	{
		engPutVariable(engine,"object",*my);
		engEvalString(engine,"disp(char(object));");
		output[strlen(output)-1]='\0'; // trim CR off end
		return _snprintf(value,size-1,"[[%s]]",output);
	}
	else
		return 0;
}

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	// open a connection to the Matlab engine
	int status=0;
	static char server[1024];
	if (gl_global_getvar("matlab_server",server,sizeof(server)))
		matlab_server = server;
	if (strcmp(matlab_server,"standalone")==0)
		engine = engOpenSingleUse(NULL,NULL,&status);
	else
		engine = engOpen(matlab_server);
	if (engine==NULL)
	{
		gl_error("unable to start Matlab engine (code %d)",status);
		return NULL;
	}

	// prepare session
	char debug[8];
	if (gl_global_getvar("debug",debug,sizeof(debug)))
		debugmode = (atoi(debug)==1);
	engSetVisible(engine,debugmode?1:0);
	engEvalString(engine,"clear all;");
	char env[1024];
	_snprintf(env,sizeof(env),"NEVER=%g;INVALID=%g;",TOSERIAL(TS_NEVER),TOSERIAL(TS_INVALID));
	engEvalString(engine,env);

	// collect output from Matlab
	engOutputBuffer(engine,output,sizeof(output)); 

	// setup the Matlab module and run the class constructor
	engEvalString(engine,"global passconfig;");
	if (engEvalString(engine,argv[0])!=0)
		gl_error("unable to evaluate function '%s' in Matlab", argv[0]);
	else
		gl_matlab_output();

	// read the pass configuration
	mxArray *pcfg= engGetVariable(engine,"passconfig");
	if (pcfg && mxIsChar(pcfg))
	{
		char passinfo[1024];
		KEYWORD keys[] = {
			{"NOSYNC",PC_NOSYNC,keys+1},
			{"PRETOPDOWN",PC_PRETOPDOWN,keys+2},
			{"BOTTOMUP",PC_BOTTOMUP,keys+3},
			{"POSTTOPDOWN",PC_POSTTOPDOWN,NULL},
		};
		PROPERTY pctype = {0,"passconfig",PT_set,1,PA_PUBLIC,NULL,&passconfig,NULL,keys,NULL};
		set passdata;
		if (mxGetString(pcfg,passinfo,sizeof(passinfo))==0 && callback->convert.string_to_property(&pctype,&passdata,passinfo)>0)
		{
			passconfig = (PASSCONFIG)passdata;
			oclass=gl_register_class(module,argv[0],passconfig);
			if (oclass==NULL)
				gl_error("unable to register '%s' as a class",argv[0]);

			DELEGATEDTYPE *pDelegate = new DELEGATEDTYPE;
			pDelegate->oclass = oclass;
			strncpy(pDelegate->type,"matlab",sizeof(pDelegate->type));
			pDelegate->from_string = object_from_string;
			pDelegate->to_string = object_to_string;
			if (gl_publish_variable(oclass,PT_delegated,pDelegate,"data",0,NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		}
		else
			gl_error("passconfig is invalid (expected set of NOSYNC, PRETOPDOWN, BOTTOMUP, and POSTTOPDOWN)", passinfo);
	}
	else
		gl_error("passconfig not specified");

	// read the pass configuration
	mxArray *ans= engGetVariable(engine,"ans");
	if (ans && mxIsStruct(ans))
	{
		defaults = mxDuplicateArray(ans);

		// process the answer
		int nFields = mxGetNumberOfFields(ans), i;
		for (i=0; i<nFields; i++)
		{
			const char *name = mxGetFieldNameByNumber(ans,i);
			mxArray *data = mxGetFieldByNumber(ans,0,i);
			// @todo publish the structure
		}
	}
	else
		gl_error("result of call to matlab::%s did not return a structure", argv[0]);

#ifdef OPTIONAL
	/* TODO: publish global variables (see class_define_map() for details) */
	gl_global_create(char *name, ..., NULL);
	/* TODO: use gl_global_setvar, gl_global_getvar, and gl_global_find for access */
#endif

	/* always return the first class registered */
	return oclass;
}

EXPORT int create_matlab(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(oclass,sizeof(mxArray*));
		if (*obj!=NULL)
		{
			gl_set_parent(*obj,parent);
			char createcall[1024];

			if (engPutVariable(engine,"object",defaults))
			{
				gl_error("matlab::%s_create(...) unable to set defaults for object", oclass->name);
				throw "create failed";
			}
			if (parent)
			{
				// @todo transfer parent data in matlab create call
				mxArray *pParent = mxCreateStructMatrix(0,0,0,NULL);
				engPutVariable(engine,"parent",pParent);
				sprintf(createcall,"create(object,parent)");
			}
			else
				sprintf(createcall,"create(object,[])");
			if (engEvalString(engine,createcall)!=0)
			{
				gl_error("matlab::%s_create(...) unable to evaluate '%s' in Matlab", oclass->name, createcall);
				throw "create failed";
			}
			else
				gl_matlab_output();
			mxArray *ans= engGetVariable(engine,"ans");
			mxArray **my = OBJECTDATA(*obj,mxArray*);
			if (ans && mxIsClass(ans,oclass->name))
				*my = engGetVariable(engine,"ans");
			else
			{
				*my = NULL;
				gl_error("matlab::@%s/create(...) failed to return an object of class %s",oclass->name,oclass->name);
				throw "create failed";
			}
			return 1;
		}
		else
			throw "create failed due to memory allocation failure";
	} 
	catch (char *msg) 
	{
		gl_error("create_matlab(): %s", msg);
	}
	return 0;
}

EXPORT int init_matlab(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
		{
			char initcall[1024];
			// @todo transfer parent data in matlab init call
			mxArray **my = OBJECTDATA(obj,mxArray*);
			if (engPutVariable(engine,"object",*my))
			{
				gl_error("%s_init() unable to send object data", oclass->name);
				return 0;
			}
			if (parent)
			{
				// @todo transfer parent data in matlab init call
				mxArray *pParent = mxCreateStructMatrix(0,0,0,NULL);
				engPutVariable(engine,"parent",pParent);
				sprintf(initcall,"init(object,parent)",oclass->name);
			}
			else
				sprintf(initcall,"init(object,[])",oclass->name);
			if (engEvalString(engine,initcall)!=0)
				gl_error("unable to evaluate '%s' in Matlab", initcall);
			else
				gl_matlab_output();
			mxArray *ans= engGetVariable(engine,"ans");
			if (ans && mxIsClass(ans,oclass->name))
			{
				mxArray **my = OBJECTDATA(obj,mxArray*);
				if (*my!=NULL)
					mxFree(*my);
				*my = mxDuplicateArray(ans);
				return 1;
			}
			else
			{
				gl_error("matlab::@%s/create(...) failed to return an object of class %s",oclass->name,oclass->name);
				throw "create failed";
			}
		}
	}
	catch (char *msg)
	{
		gl_error("init_matlab(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 0;
}

EXPORT TIMESTAMP sync_matlab(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	CLASSNAME *my = OBJECTDATA(obj,CLASSNAME);
	try
	{
		char synccall[1024];
		
		switch (pass) {
		case PC_PRETOPDOWN:
			sprintf(synccall,"[ans t2]=presync(object,%g,%g)",TOSERIAL(obj->clock),TOSERIAL(t1));
			if ((passconfig&(PC_BOTTOMUP|PC_POSTTOPDOWN))==0)
				obj->clock = t1;		
			break;
		case PC_BOTTOMUP:
			sprintf(synccall,"[ans t2]=sync(object,%g,%g)",TOSERIAL(obj->clock),TOSERIAL(t1));
			if ((passconfig&PC_POSTTOPDOWN)==0)
				obj->clock = t1;		
			break;
		case PC_POSTTOPDOWN:
			sprintf(synccall,"[ans t2]=postsync(object,%g,%g)",TOSERIAL(obj->clock),TOSERIAL(t1));
			obj->clock = t1;		
			break;
		default:
			throw("invalid pass request");
			break;
		}

		// send object data
		mxArray **my = OBJECTDATA(obj,mxArray*);
		if (engPutVariable(engine,"object",*my))
		{
			gl_error("%s unable to send object data", oclass->name);
			return 0;
		}

		// perform sync
		if (engEvalString(engine,synccall)!=0)
			gl_error("%s call failed", synccall);
		else
			gl_matlab_output();

		// get object data
		mxArray *ans= engGetVariable(engine,"ans");
		if (ans && mxIsClass(ans,oclass->name))
		{
			if (mxIsClass(ans,oclass->name))
			{
				mxArray **my = OBJECTDATA(obj,mxArray*);
				if (*my!=NULL)
					mxFree(*my);
				*my = mxDuplicateArray(ans);
			}
			else
				throw "sync return must have two values";
		}

		else
		{
			gl_error("matlab::%s: %s failed to return an object of class %s",oclass->name,synccall,oclass->name);
			throw "sync failed";
		}

		// get t2
		mxArray *pT2 = engGetVariable(engine,"t2");
		if (pT2 && mxIsNumeric(pT2))
		{
			t2 = FROMSERIAL(mxGetScalar(pT2));
		}
		else
		{
			gl_error("matlab::@%s/%s failed to return a sync time",oclass->name,synccall);
			throw "sync failed";
		}
	}
	catch (char *msg)
	{
		gl_error("sync_matlab(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return TS_INVALID;
	}
	return t2;
}


CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	return 0;
}

