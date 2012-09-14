/**
	Copyright (C) 2012 Battelle Memorial Institute
	@file		pw_load.cpp
	@author		Matt Hauer d3p988
	@version	3.0

	An interface with PowerWorld's load object.  Uses SimAuto v10 to
	exchange data via a COM object provided by PowerWorld.
 **/

#include "pw_load.h"

// let's see if this *doesn't* confuse the compiler.  Fingers crossed.
/**
 	Scans the argument to validate its VARIANT type, and to perform basic
 	  error checking.
 	@param	output	expected to be a two-element variant of types
					VT_VARIANT | VT_ARRAY ("array of variants")
 	@return	positive logic: 1 if input reports success, 0 if input reports failure
 **/
int check_COM_output_load(_variant_t output){
	BSTR bHolder;
	_variant_t element;
	LONG indices[1] = {0};
	char *ptr = 0;
	HRESULT hr;
	SAFEARRAY *output_array;

	if(output.vt != (VT_VARIANT | VT_ARRAY)){
		gl_error("check_COM_output_load: COM call did not return an array of variants");
		/* TROUBLESHOOTING */
		return 0;
	}
	output_array = output.parray;
	hr = SafeArrayGetElement(output_array, indices, &element);
	switch(hr){
		case S_OK:
			break; //okay
		case DISP_E_BADINDEX: // bad entry in indices
			gl_error("check_COM_output_load: bad index in SafeArrayGetElement");
			/* TROUBLESHOOTING */
			return 0;
			break; 
		case E_INVALIDARG: 
			gl_error("check_COM_output_load: invalid arguement in SafeArrayGetElement");
			/* TROUBLESHOOTING */
			return 0;
			break; // one of the args was invalid (?)
		case E_OUTOFMEMORY: 
			gl_error("check_COM_output_load: ran out of memory during SafeArrayGetElement");
			/* TROUBLESHOOTING */
			return 0;
			break; // ran out of memory
	}
	bHolder = element.bstrVal;
	ptr = _com_util::ConvertBSTRToString(bHolder);
	if(strlen(ptr) > 0){
		gl_error("check_COM_output_load: %s", ptr);
		/* TROUBLESHOOTING */
		return 0;
	}
	return 1; // "good"
}

// DC's magic GLD smoke jars.  Don't breathe this!
EXPORT_CREATE(pw_load);
EXPORT_INIT(pw_load);
//EXPORT_SYNC(pw_load);

EXPORT TIMESTAMP sync_pw_load(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass) { 
	pw_load *p;
	try {
			TIMESTAMP t1=TS_NEVER; p=OBJECTDATA(obj,pw_load); 
		switch (pass) { 
		case PC_PRETOPDOWN: t1 = p->presync(t0); break; 
		case PC_BOTTOMUP: t1 = p->sync(t0); break; 
		case PC_POSTTOPDOWN: t1 = p->postsync(t0); break; 
		default: throw "invalid pass request"; break; } 
		if ( (obj->oclass->passconfig&(PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN)&(~pass)) <= pass ) obj->clock = t0; 
		return t1; 
	} 
	SYNC_CATCHALL(pw_load); }


EXPORT_ISA(pw_load);

CLASS *pw_load::oclass = NULL;
pw_load *pw_load::defaults = NULL;

pw_load::pw_load(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"pw_load",sizeof(pw_load),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL){
			throw "unable to register class pw_load";
			/* TROUBLESHOOTING */
		} else {
			oclass->trl = TRL_PROVEN;
		}

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_object, "model", get_model_offset(),PT_DESCRIPTION, "pw_model object for the PowerWorld model this load is in",
			PT_int32, "powerworld_bus_num", get_powerworld_bus_num_offset(),PT_DESCRIPTION, "bus_num key field of the load",
			PT_char1024, "powerworld_load_id", get_powerworld_load_id_offset(),PT_DESCRIPTION, "load_id key field of the load",
			PT_double, "power_threshold[MVA]", get_power_threshold_offset(),PT_DESCRIPTION, "power threshold for this object to trigger a model update",
			PT_double, "power_diff[MVA]", get_power_diff_offset(),PT_DESCRIPTION, "power threshold for this object to trigger a model update",
			PT_complex, "load_power[MVA]", get_load_power_offset(),PT_DESCRIPTION, "constant power draw on the load",
			PT_complex, "load_impedance[MVA]", get_load_impedance_offset(),PT_DESCRIPTION, "constant impedance draw on the load",
			PT_complex, "load_current[MVA]", get_load_current_offset(),PT_DESCRIPTION, "constant current draw on the load",
			PT_complex, "last_load_power[MVA]", get_last_load_power_offset(),PT_DESCRIPTION, "constant power draw on the load",
			PT_complex, "last_load_impedance[MVA]", get_last_load_impedance_offset(),PT_DESCRIPTION, "constant impedance draw on the load",
			PT_complex, "last_load_current[MVA]", get_last_load_current_offset(),PT_DESCRIPTION, "constant current draw on the load",
			PT_complex, "load_voltage[V]", get_load_voltage_offset(),PT_DESCRIPTION, "transmission system voltage on this load",
//			PT_complex, "next_load_power", get_next_load_power_offset(),
			PT_complex, "pw_load_mw", get_pw_load_mw_offset(),
			PT_double, "pw_load_mva", get_pw_load_mva_offset(),
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
				/* TROUBLESHOOTING */
		}

		memset(this,0,sizeof(pw_load));
		powerworld_bus_num = -1;
	}
}

int pw_load::create(){
	return 1;
}

// it would be faster to cache some of these in the object rather than re-allocating them each iteration. -mh
/**
	Builds a set of SafeArray ARRAY|VARIANT defining desired fields from the PowerWorld model, then uses SimAuto to
	retrieve the voltage, the total complex load, and the total load magnitude.
	@return	Returns 0 on success, nonzero on failure.
 **/
int pw_load::get_powerworld_voltage(){
	// source strongly borrowed from PowerWorld
	_variant_t HUGEP *presults, *pvariant;
	HRESULT hr;
	SAFEARRAYBOUND bounds[1];
	pw_model *cModel;
	_variant_t fields, values, results;
	char *tempstr = 0;
	BSTR tempbstr = 0;

	double load_voltage_d = 0.0;
	double load_mva = 0.0, load_mvr = 0.0, load_mw = 0.0;

	cModel = OBJECTDATA(model, pw_model);

//	gl_output("get_voltage(): before 1 mw = %f, mva = (%f, %f)", this->pw_load_mw, this->pw_load_mva.Re(), this->pw_load_mva.Im());

	try {
		ISimulatorAutoPtr SimAuto(cModel->A);
			
		bounds[0].cElements = 8;
		bounds[0].lLbound = 0;

		fields.vt = VT_ARRAY | VT_VARIANT;
		fields.parray = SafeArrayCreate(VT_VARIANT, 1, bounds);
		if (!fields.parray){
			throw _com_error(E_OUTOFMEMORY);
		}
		values.vt = VT_ARRAY | VT_VARIANT;
		values.parray = SafeArrayCreate(VT_VARIANT, 1, bounds);
		if (!values.parray){
			throw _com_error(E_OUTOFMEMORY);
		}
		
		hr = SafeArrayAccessData(fields.parray, (void HUGEP **)&pvariant);
		if (FAILED(hr)){
			throw _com_error(hr);
		}
		pvariant[0] = "BusNum";
		pvariant[1] = "LoadID";
		pvariant[2] = "BusKVVolt";
		pvariant[3] = "LoadMVA";
		pvariant[4] = "LoadMW";
		pvariant[5] = "LoadMVR";
		pvariant[6] = "LoadSMW";
		pvariant[7] = "LoadSMVR";
		SafeArrayUnaccessData(fields.parray);

		hr = SafeArrayAccessData(values.parray, (void HUGEP **)&pvariant);
		if (FAILED(hr)){
			throw _com_error(hr);
		}
		char pbn_str[32];
		sprintf(pbn_str, "%i", powerworld_bus_num);
		//pvariant[0] = this->powerworld_bus_num;
		pvariant[0] = _com_util::ConvertStringToBSTR(pbn_str);
		pvariant[1] = tempbstr = _com_util::ConvertStringToBSTR(this->powerworld_load_id);
		pvariant[2] = _variant_t();
		pvariant[3] = _variant_t();
		pvariant[4] = _variant_t();
		pvariant[5] = _variant_t();
		pvariant[6] = _variant_t();
		pvariant[7] = _variant_t();

		SysFreeString(tempbstr);
		SafeArrayUnaccessData(values.parray);

		results = SimAuto->GetParametersSingleElement(L"Load", fields, values);

		hr = SafeArrayAccessData(results.parray, (void HUGEP **)&presults);
		if (FAILED(hr)){
			throw _com_error(hr);
		}
		if (((_bstr_t)(_variant_t)presults[0]).length()){
			tempstr = _com_util::ConvertBSTRToString((_bstr_t)(_variant_t)presults[0]);
			gl_error("Error from GetParametersSingleElement(): %s", tempstr);
			/* TROUBLESHOOTING 
				The call to GetParametersSingleElement failed.  Please review the error message and respond accordingly.
				Addition COM-related error handling may be found on the MSDN website.
			 */
			delete [] tempstr;
			tempstr = 0;
			SafeArrayDestroy(fields.parray);
			SafeArrayDestroy(values.parray);
			return 0;
		} else {
			double load_pi, load_pr;
			
			hr = SafeArrayAccessData(presults[1].parray, (void HUGEP **)&pvariant);
			if (FAILED(hr)){
				throw _com_error(hr);
			}
			load_voltage_d = wcstod(pvariant[2].bstrVal, 0);
			load_mva = wcstod(pvariant[3].bstrVal, 0);
			load_mw = wcstod(pvariant[4].bstrVal, 0);
			load_mvr = wcstod(pvariant[5].bstrVal, 0);
			load_pr = wcstod(pvariant[6].bstrVal, 0);
			load_pi = wcstod(pvariant[7].bstrVal, 0);
			SafeArrayUnaccessData(presults[1].parray);

//			printf("get_voltage(): power = %f + %f\n", load_pr, load_pi);
		}
		SafeArrayUnaccessData(results.parray);
		load_voltage = complex(load_voltage_d * 1000.0, 0.0, I);
		pw_load_mw = load_mva;
		pw_load_mva = complex(load_mw, load_mvr, I);

//		gl_output("get_voltage(): after 2 mw = %f, mva = (%f, %f)", this->pw_load_mw, this->pw_load_mva.Re(), this->pw_load_mva.Im());
		
		SafeArrayDestroy(fields.parray);
		SafeArrayDestroy(values.parray);
	}
	catch (_com_error err) {
		// @TODO this needs to be a gl_error, but err.ErrorMessage returns a TCHAR*
		std::cout << "!!! " << err.ErrorMessage() << "\n";
		return 1; // failure
	}
	catch(...){
		std::cout << "Unknown excetpion in get_pwd_voltage!";
	}
	return 0; // success
}

/**
	Builds a set of SafeArray ARRAY|VARIANT defining desired fields from the PowerWorld model, then uses SimAuto to
	post the power load, the current load, and the impedance load.
	@return	Returns 0 on success
 **/
int pw_load::post_powerworld_current(){
	// source strongly borrowed from PowerWorld
	_variant_t HUGEP *presults, *pvariant;
	HRESULT hr;
	SAFEARRAYBOUND bounds[1];
	pw_model *cModel;
	_variant_t fields, values, results;
	BSTR tempbstr;

	cModel = OBJECTDATA(model, pw_model);

	try {
		ISimulatorAutoPtr SimAuto(cModel->A);

		bounds[0].cElements = 8;
		bounds[0].lLbound = 0;

		fields.vt = VT_ARRAY | VT_VARIANT;
		fields.parray = SafeArrayCreate(VT_VARIANT, 1, bounds);
		if (!fields.parray){
			throw _com_error(E_OUTOFMEMORY);
		}
		values.vt = VT_ARRAY | VT_VARIANT;
		values.parray = SafeArrayCreate(VT_VARIANT, 1, bounds);
		if (!values.parray){
			throw _com_error(E_OUTOFMEMORY);
		}
			
		hr = SafeArrayAccessData(fields.parray, (void HUGEP **)&pvariant);
		if (FAILED(hr)){
			throw _com_error(hr);
		}
		pvariant[0] = "BusNum";
		pvariant[1] = "LoadID";
		pvariant[2] = "LoadSMW";
		pvariant[3] = "LoadSMVA";
		pvariant[4] = "LoadIMW";
		pvariant[5] = "LoadIMVA";
		pvariant[6] = "LoadZMW";
		pvariant[7] = "LoadZMVA";
		SafeArrayUnaccessData(fields.parray);

		hr = SafeArrayAccessData(values.parray, (void HUGEP **)&pvariant);
		if (FAILED(hr)){
			throw _com_error(hr);
		}
		pvariant[0] = this->powerworld_bus_num;
		pvariant[1] = tempbstr = _com_util::ConvertStringToBSTR(this->powerworld_load_id); // NULL
		pvariant[2] = load_power.Re();
		pvariant[3] = load_power.Im();
		pvariant[4] = load_current.Re();
		pvariant[5] = load_current.Im();
		pvariant[6] = load_impedance.Re();
		pvariant[7] = load_impedance.Im();
		SysFreeString(tempbstr);
		SafeArrayUnaccessData(values.parray);

//		printf("post_power(): Z=%f+%fi\n", next_load_power.Re(), next_load_power.Im());
		results = SimAuto->ChangeParametersSingleElement(L"Load", fields, values);

		hr = SafeArrayAccessData(results.parray, (void HUGEP **)&presults);
		if (FAILED(hr)){
			throw _com_error(hr);
		}
		if (((_bstr_t)(_variant_t)presults[0]).length()){
			char *tempstr = _com_util::ConvertBSTRToString((_bstr_t)(_variant_t)presults[0]);
			gl_error("Error from SetParametersSingleElement(): %s",  tempstr);
			/* TROUBLESHOOTING 
				The call to SetParametersSingleElement failed.  Please review the error message and respond accordingly.
				Addition COM-related error handling may be found on the MSDN website.
			 */
			delete [] tempstr;
		} else {
			// else success, only the one return element
		}
		SafeArrayUnaccessData(results.parray);
	}
	catch (_com_error err) {
		// @TODO this needs to be a gl_error, but err.ErrorMessage returns a TCHAR*
		std::cout << "!!! " << err.ErrorMessage() << "\n";
	}
	return 0; // success
}

/**
	Initializes a pw_load object.  On successful initialization, the object will have an pw_model defined as
	its target model, 
	@return 0 on failure, 1 on success, 2 if model object has not initialized yet
 **/
int pw_load::init(OBJECT *parent){
	_bstr_t otype;
	_variant_t plist;
	_variant_t vlist;
	_variant_t output;
//	SAFEARRAYBOUND bounds[1];
//	_variant_t plistNames, vlistNames;
//	LPSAFEARRAY plistArray, vlistArray;
//	VARIANT *plistNameArray, *vlistNameArray; // slightly inaccurate nomenclature but aiming for consistency

	// defer on model object
	if(0 == model){
		char objname[256];
		gl_error("pw_load::init(): object \'%s\' does not specify a model object", gl_name(model, objname, 255));
		/* TROUBLESHOOT
			pw_load objects require that a model object is specified, refering to a pw_model object.
		 */
		return 0;
	}

	if(!gl_object_isa(model, "pw_model")){
		char objname[256], modelname[256];
		gl_error("pw_load::init(): object \'%s\' specifies a model object \'%s\' that is not a pw_model object", gl_name(model, objname, 255), gl_name(model, modelname, 255));
		/* TROUBLESHOOT
			pw_load objects require that the specified model object refers to a pw_model object.  Other types are not supported.
		 */
		return 0;
	}

	if((model->flags & OF_INIT) != OF_INIT){
		char objname[256];
		gl_verbose("pw_load::init(): deferring initialization on %s", gl_name(model, objname, 255));
		return 2; // defer
	}

	// capture the model class pointer, we'll need it for later
	cModel = OBJECTDATA(model, pw_model);

	// must have bus_num and load_id
	if(powerworld_bus_num < 0){
		gl_error("powerworld_bus_num must be a non-negative integer");
		/* TROUBLESHOOT
			PowerWorld uses positive integers for bus numbers.  The pw_load object must connect to a valid bus, requiring a valid bus number.
		 */
		return 0;
	}
	
	if(powerworld_load_id[0] == 0){
		gl_error("powerworld_load_id must be set");
		/* TROUBLESHOOT
			PowerWorld uses a short string for load IDs.  The pw_load object must connect to a valid load, requiring a load ID be defined.
		 */
		return 0;
	}

	if(0 != get_powerworld_voltage()){
		char objname[256];
		gl_error("pw_load::init(): unable to get PowerWorld bus voltage from Bus #%i, Load ID %s, for the pw_load \'%s\'", powerworld_bus_num, powerworld_load_id, gl_name(OBJECTHDR(this), objname, 255));
		/* TROUBLESHOOT
			An error occured while calling the function that retrieves data from PowerWorld.  The Bus/Load pair may be incorrect, there may have
			been a COM error with SimAuto, or other unexpected results may have occured.
		 */
		return 0;
	}

	// power_threshold must be positive, else default
	if(power_threshold < 0.0){
		gl_warning("pw_load::init(): power_threshold is negative, reseting to 0 VA");
		power_threshold = 0.0;
	}

	return 1;
}

/**
	presync checks if the PowerWorld model is in a valid state, then will retrieve the bus voltage if the state is valid.
	@return TS_NEVER on success, TS_INVALID if unable to retrieve the bus voltage
 **/
TIMESTAMP pw_load::presync(TIMESTAMP t1){
	if(0 == cModel){
		gl_error("pw_load::presync(): cModel is null (is deferred initialization enabled?)");
		return TS_INVALID;
	}
	if(!cModel->get_valid_flag()){
		gl_verbose("not fetching voltage due to invalid model state");
	} else {
		if(0 != get_powerworld_voltage()){
			gl_error("pw_load::presync(): get_powerworld_voltage failed");
			/*	TROUBLESHOOT
			 */
			return TS_INVALID;
		}
	}
	// substation will perform conversion on load_voltage for powerflow module

	return TS_NEVER;
}

/**
	Updates the change in loads, which should have been updated on the 'way up' by the substation object
	posting its load values to the pw_load.  If the load changes by a sufficient quantity, the pw_load will
	set the 'update' flag in its pw_model object to 'true'.
	@return TS_NEVER.
 **/
TIMESTAMP pw_load::sync(TIMESTAMP t1){
	// check if load_power/imped/current aggregate magnitude change exceeds threshold
	//	* if so, set model->update_flag
	double z_diff, i_diff, p_diff;
	z_diff = load_power.Mag() - last_load_power.Mag();
	i_diff = load_current.Mag() - last_load_current.Mag();
	p_diff = load_impedance.Mag() - last_load_impedance.Mag();
	power_diff = fabs(z_diff) + fabs(i_diff) + fabs(p_diff);
	// @TODO once we hook the substation up, need to verify that the flag isn't set for adequately small changes
	//if(power_diff > power_threshold){
		cModel->set_update_flag(true, gld_wlock(model));
	//}
	return TS_NEVER;
}

/**
	If the model has been flagged for an update, the load values are posted and the last_load members are updated.
	@return TS_NEVER on success, TS_INVALID if unable to post the load.
 **/
TIMESTAMP pw_load::postsync(TIMESTAMP t1){
	// if model->update_flag is set,
	//	* send aggregate load to model
	
	if(cModel->get_update_flag()){
		if(0 != post_powerworld_current()){
			return TS_INVALID;
		}
	
		last_load_power = load_power;
		last_load_current = load_current;
		last_load_impedance = load_impedance;
	}
	return TS_NEVER;
}

int pw_load::isa(char *classname){
	return (0 == strcmp(classname, oclass->name));
}


// EOF
