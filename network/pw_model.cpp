/**
	Copyright (C) 2012 Battelle Memorial Institute
	@file		pw_model.cpp
	@author		Matt Hauer d3p988
	@version	3.0

	An interface with PowerWorld and a single model file.  Constructs and destructs the SimAuto COM connection.
	Tracks if any of the attached loads has requested a model calculation, and detects if the powerflow could not
	converge to a solution.
 **/

#include "pw_model.h"

#ifdef HAVE_POWERWORLD
#ifndef PWX64

#include "comutil.h"
#include "atlbase.h"
#pragma comment(lib, "comsuppw.lib")

EXPORT_CREATE(pw_model);
EXPORT_INIT(pw_model);
EXPORT_SYNC(pw_model);
EXPORT_ISA(pw_model);
//EXPORT_FINALIZE(pw_model);

EXPORT int finalize_pw_model(OBJECT *obj) 
{	pw_model *my = OBJECTDATA(obj,pw_model);
	try {
		return obj!=NULL ? my->finalize() : 0;
	} 
	//T_CATCHALL(pw_model,finalize); 
	catch (char *msg) {
		gl_error("finalize_pw_model" "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return TS_INVALID; 
	}
	catch (const char *msg) { 
		gl_error("finalize_pw_model" "(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return TS_INVALID;
	}
	catch (...) {
		gl_error("finalize_pw_model" "(obj=%d;%s): unhandled exception", obj->id, obj->name?obj->name:"unnamed");
		return TS_INVALID;
	}
}

CLASS *pw_model::oclass = NULL;
pw_model *pw_model::defaults = NULL;

pw_model::pw_model(MODULE *module)
{
	if (oclass==NULL)
	{
		//GLOBAL char32 model_name INIT("(unnamed)")
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"pw_model",sizeof(pw_model),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class assert";
		else
			oclass->trl = TRL_PROVEN;

		defaults = this;
		if (gl_publish_variable(oclass,
		//if ((*callback->define_map)(0,
			PT_char1024, "model_name", PADDR(model_name),PT_DESCRIPTION, "the file path for the PowerWorld model to run",
//			PT_char1024, "load_list_file", PADDR(load_list_file), PT_DESCRIPTION, "the optional file to write a list of loads into.  if present, GLD will exit after writing loads.",
//			PT_char1024, "out_file", PADDR(out_file), PT_DESCRIPTION, "the file to write the list of objects into",
//			PT_char32, "out_file_type", PADDR(out_file_type),PT_DESCRIPTION,"the type of object to list in out_file",
//			PT_char1024, "field_file", PADDR(field_file),PT_DESCRIPTION,"the file to write the list of fields of a type of object into",
//			PT_char32, "field_type", PADDR(field_type),PT_DESCRIPTION,"the type of object to list the fields for in field_file",
			PT_bool, "update_flag", get_update_flag_offset(), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "flag set by pw_load objects signaling the loads need updating",
			PT_int32, "exchange_count", get_exchange_count_offset(), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "number of times PowerWorld and GridLAB-D have exchanged data",
			PT_bool, "valid_flag", get_valid_flag_offset(), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "flag set by pw_model signaling if the PowerWorld model solved successfully",
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		memset(this,0,sizeof(pw_model));

	}
}

//	required callback
int pw_model::create(){
	return 1;
}

/**
 	Scans the argument to validate its VARIANT type, and to perform basic
 	  error checking.
 	@param	output	expected to be a two-element variant of types
					VT_VARIANT | VT_ARRAY ("array of variants")
 	@return	positive logic: 1 if input reports success, 0 if input reports failure
 **/

int check_COM_output(_variant_t output){
	BSTR bHolder;
	_variant_t element;
	LONG indices[1] = {0};
	char *ptr = 0;
	HRESULT hr;
	SAFEARRAY *output_array;
	int rv;

//	output = A->RunScriptCommand("EnterMode(RUN);");

	if(output.vt != (VT_VARIANT | VT_ARRAY)){
		gl_error("check_COM_output: COM call did not return an array of variants");
		/* TROUBLESHOOT
		An error was encountered in pw_load while trying to return a required array.  Please try again.
		If the error persists, please submit your code and a bug report via the trac website
		*/
		return 0;
	}
	output_array = output.parray;
	hr = SafeArrayGetElement(output_array, indices, &element);
	switch(hr){
		case S_OK:
			break; //okay
		case DISP_E_BADINDEX: // bad entry in indices
			gl_error("check_COM_output: bad index in SafeArrayGetElement");
			/* TROUBLESHOOT
			A bad index value was encountered while parsing the COM output to PowerWorld.  Please ensure
			all values are correct and try again.
			*/
			return 0;
			break; 
		case E_INVALIDARG: 
			gl_error("check_COM_output: invalid arguement in SafeArrayGetElement");
			/* TROUBLESHOOT
			An invalid argument was encountered while parsing the COM output to PowerWorld.  Please ensure
			all values are correct and try again.
			*/
			return 0;
			break; // one of the args was invalid (?)
		case E_OUTOFMEMORY: 
			gl_error("check_COM_output: ran out of memory during SafeArrayGetElement");
			/* TROUBLESHOOT
			Memory ran out while parsing the COM output to PowerWorld.  Please ensure
			all values are correct and try again.  If the error persists, please submit a bug
			report via the trac website.
			*/
			return 0;
			break; // ran out of memory
	}
	bHolder = element.bstrVal;
	ptr = _com_util::ConvertBSTRToString(bHolder);
	if(strlen(ptr) > 0){
		gl_error("check_COM_output: %s", ptr);
		/* TROUBLESHOOT
		A generic COM error was encountered while interfacing with PowerWorld.  Please check
		MSDN and other resources for what this may mean.
		*/
		rv = 0;
	} else {
		rv = 1;
	}
	delete [] ptr;
	return rv;
}

int pw_model::init(OBJECT *parent){
	_variant_t output;
	_variant_t element;
//	SAFEARRAY *output_array;
	HRESULT rv;
//	BSTR bHolder;
	LONG indices[1] = {0};
	char *ptr = 0;
	bool trouble = false;
	// if model_name not null,
	//VariantInit(&output);
	if(0 != model_name[0]){
		LPCOLESTR clsid_str = L"pwrworld.SimulatorAuto";
		
	//	* initialize COM
		rv = ::CoInitialize(NULL);

		//Set flag to true to uninitialize us upon closing
		startedCOM=true;
		initiatorCOM=OBJECTHDR(this);

	//	* check syntax
	//	* start SIMAuto if not running
		hr = CLSIDFromProgID(clsid_str, &clsid);
		switch(hr){
			case S_OK:
				// CLSID successfully retreived
				break;
			case CO_E_CLASSSTRING:
				gl_error("unable to find clsid \"pwerworld.SimulatorAuto\" at init()");
				/*  TROUBLESHOOT
				GridLAB-D was unable to find the clsid value for PowerWorld SimAuto inside the
				registry.  Please ensure PowerWorld and SimAuto are installed and registered correctly
				and try again.
				*/
				return 0;
				break;
			case REGDB_E_WRITEREGDB:
				gl_error("An error occured writing the CLSID to the registry.");	//I would imagine this should never happen - why are we writing to the registry?
				/*  TROUBLESHOOT
				An error occurred while writing the CLSID value to the registry.  Please submit your
				code and a bug report via the trac website.
				*/
				return 0;
				break;
			default:
				gl_error("unexpected return code from CLSIDFromProdID() in init()");
				return 0;
				break;
		}

		// construct AutoPtr ... COM and smart pointer stuff.
		A = new ISimulatorAutoPtr(__uuidof(SimulatorAuto));
		if(0 == A){
			gl_error("unable to allocate ISimulatorAutoPtr in init()");
			/*  TROUBLESHOOT
			GridLAB-D was unable to allocate one of the necessary variables for the SimAuto interface.
			Please try again.  If the error persists, please submit your code and a bug report via the
			trac website.
			*/
			return 0;
		}
		hr = A.CreateInstance(clsid, NULL, CLSCTX_SERVER);
		//	* if !connect(model_name), fail!
		output = A->OpenCase(model_name.get_string()); // must catch RV in a _variant_t!
		if(0 == check_COM_output(output)){
			gl_error("OpenCase() failed");
			/*  TROUBLESHOOT
			PowerWorld failed to open the case specified.  Please ensure the case exists, is a valid PowerWorld file,
			and that it is located in the proper folder and try again.
			*/
			return 0;
		}
		//gl_output("OpenCase succeeded");
		gl_verbose("OpenCase succeeded");

	//	* run simulation
		output = A->RunScriptCommand("EnterMode(RUN);");
		if(0 == check_COM_output(output)){
			gl_error("RunScriptCommand(EnterMode(RUN);) failed");
			/*	TROUBLESHOOT
			PowerWorld failed to enter the RUN mode.  Please ensure there are no model errors
			and that PowerWorld is functioning correctly and try again.
			*/
			return 0;
		}
		//gl_output("RunScriptCommand(EnterMode(RUN)) succeeded");
		gl_verbose("RunScriptCommand(EnterMode(RUN)) succeeded");

		//	* run simulation
		output = A->RunScriptCommand("SolvePowerFlow;");
		if(0 == check_COM_output(output)){
			gl_error("RunScriptCommand(SolvePowerFlow;) failed");
			/* TROUBLESHOOT
				SimAuto reported a problem when asked to simulate the model.  Review the error message on the console.
			 */
			return 0;
		}
		//gl_output("RunScriptCommand(SolvePowerFlow) succeeded");
		gl_verbose("RunScriptCommand(SolvePowerFlow) succeeded");
	//	* if simulation failed, fail!
	

	} else {
		// model_name is null, there may be trouble
		trouble = true;
	}

	/*  IFDEFing out unpublished code for now */
#ifdef PW_THIS_IS_PUBLISHED
	// not published
	if(out_file[0] != 0){
		FILE *outfile;
		BSTR objtypebstr, filterbstr;

		if(out_file_type == 0){
			gl_error("out_file '%s' did not define an object type");
			return 0;
		}
		outfile = fopen(out_file, "w");
		if(out_file == 0){
			gl_error("out_file '%s' could not be opened for writing");
			return 0;
		}
		
		objtypebstr = _com_util::ConvertStringToBSTR(out_file_type);
		filterbstr = _com_util::ConvertStringToBSTR("");
		output = A->ListOfDevices(objtypebstr, filterbstr);

		if(0 == check_COM_output(output)){
			gl_error("ListOfDevices(%s) failed", out_file_type);
			return 0;
		}
		//gl_output("ListOfDevices(%s) succeeded", out_file_type);
		gl_verbose("ListOfDevices(%s) succeeded", out_file_type);

		// assume 'load'
		if((0 == strcmp("load", out_file_type) ) || (0 == strcmp("Load", out_file_type) ) ){
			// first is I4, second is BSTR;
			;
		} else {
			; // generic ... if ever
		}

		return 0;
	}

	// not published
	if(field_file[0] != 0){
		FILE *fieldfile;
		BSTR fieldtypebstr;
		fieldfile = fopen(field_file, "w");
		fieldtypebstr = _com_util::ConvertStringToBSTR(field_type);

		output = A->GetFieldList(fieldtypebstr);
		SysFreeString(fieldtypebstr);
		if(0 == check_COM_output(output)){
			gl_error("GetFieldList(%s) failed", field_type);
			return 0;
		}
		//gl_output("GetFieldList(%s) succeeded", field_type);
		gl_verbose("GetFieldList(%s) succeeded", field_type);
	}

	// not published.
	// if load_list_file not null,
//	if(load_list_file[0] != 0){
//		FILE *pair_file;
		//	* parse file for BusNum/LoadID pairs
		//	* connect to model
		//	* check BusNum/LoadID pairs in model file
		//	* close connection
		//	* print big pass/fail message
		//	* return 0;
//		return 0;
//	}
#endif

	if(trouble){
		gl_error("pw_model::init(): no PowerWorld model file specified");
		/* TROUBLESHOOT 
			The pw_model requires either a valid PWB model file path, or for an alternate mode to be triggered.  Please
			check the model_name definition, and the path to the desired model file.
		 */
		return 0;
	}

	exchange_count = 0;
	update_flag = false;
	valid_flag = true;
	//model_invalid_state = false;
	return 1;
}

/**
	If the update flag is set, command PowerWorld to find the powerflow solution, then 
	@return TS_NEVER given normal operations, t1 (reiterate) if the PowerWorld model is in an invalid state, TS_INVALID given SimAuto errors.
 **/
TIMESTAMP pw_model::presync(TIMESTAMP t1){
	TIMESTAMP rv = TS_NEVER;
	_variant_t output;

	// check update_flag is/not set
	// if update_flag true,
	if(update_flag){
		//	* run simulation
		// NOTE: copied from init()
		output = A->RunScriptCommand("SolvePowerFlow;");
		if(0 == check_COM_output(output)){
			gl_error("RunScriptCommand(SolvePowerFlow;) failed");
			/*  TROUBLESHOOT
			PowerWorld failed to solve the powerflow correctly.  Please ensure the model is correct and
			the system condition is valid and try again.
			*/
			return TS_INVALID;
		}
		gl_verbose("RunScriptCommand(SolvePowerFlow) succeeded");

		// @TODO determine how to detect if the simulation is in an invalid state
	//	* if simulation failed,
		//if(model_invalid_state){
		if(!valid_flag){
		//	** make note to not copy voltages
		//	** set rv = t1, we WILL reiterate
			rv = t1;
		}
	}
	
	// reset update_flag
	update_flag = false;

	return rv;
}

TIMESTAMP pw_model::sync(TIMESTAMP t1){
	OBJECT *hdr = OBJECTHDR(this);
	if(update_flag){
		return t1;
	}
	return TS_NEVER;
}

TIMESTAMP pw_model::postsync(TIMESTAMP t1){
	return TS_NEVER;
}

int pw_model::isa(char *classname){
	return (strcmp(classname, "pw_model") == 0 ? 1 : 0);
}

/**
	Closes the PowerWorld model, as GridLAB-D is shutting down.
	@return 1
 **/
int pw_model::finalize(){
	_variant_t output;

	output = A->CloseCase();
	if(0 == check_COM_output(output)){
		gl_error("CloseCase() failed");
		/*  TROUBLESHOOT
		PowerWorld encountered an error while trying to close the open case.  Please check
		to ensure your model is correct.
		*/
		return 0;
	}

	gl_verbose("pw_model::finalize(): case closed.");

	return 1;
}

//Functionalized version of COM shutdown items
//Mainly so network::term can call it
void pw_model::pw_close_COM(void)
{
	//Clean up COM stuff
	CoUninitialize();
}

#endif //PWX64
#endif //HAVE_POWERWORLD
// EOF
