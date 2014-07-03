#include "pw_recorder.h"

#ifdef HAVE_POWERWORLD
#ifndef PWX64

#include <iostream>

EXPORT_CREATE(pw_recorder);
EXPORT_INIT(pw_recorder);
EXPORT_SYNC(pw_recorder);
EXPORT_ISA(pw_recorder);
EXPORT_COMMIT(pw_recorder);
EXPORT_PRECOMMIT(pw_recorder);

CLASS *pw_recorder::oclass = NULL;
pw_recorder *pw_recorder::defaults = NULL;

int pw_recorder::get_pw_values(){
	return 1;
}

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
			PT_char1024, "outfile", get_outfile_name_offset(), PT_DESCRIPTION, "Output file name",
			PT_char256, "obj_class", get_obj_classname_offset(), PT_DESCRIPTION, "PowerWorld object class of object to record",
			PT_char1024, "key_strings", get_key_strings_offset(), PT_DESCRIPTION, "Key string values for PowerWorld object to record",
			PT_char1024, "key_values", get_key_values_offset(), PT_DESCRIPTION, "Key values for PowerWorld object to record",
			PT_char1024, "properties", get_properties_offset(), PT_DESCRIPTION, "Properties desired to record from PowerWorld object",
			PT_int64, "interval", get_interval_offset(), PT_DESCRIPTION, "Interval of output for pw_recorder object",
			PT_int64, "limit", get_limit_offset(),PT_DESCRIPTION, "Number of data points to write before stopping",
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
	char objname[64];
	// check nonzero model
	//  * if null, check for parent, fill model from that
	if(0 == model){
		if(0 == parent){
			gl_error("pw_recorder::init(): object \'%s\' does not specify a model object", gl_name(model, objname, 63));
			/*  TROUBLESHOOT
			A model object was not explicitly specified or connected as a parent to the pw_recorder object.  Please make
			this connection and try again.
			*/
			return 0;
		} else {
			// isa parent pw_model?
			if(!gl_object_isa(parent, "pw_model")){
				gl_error("pw_recorder::init(): parent of '%s' is not a pw_model", gl_name(model, objname, 63) );
				/*  TROUBLESHOOT
				The pw_recorder object does not have the model field specified and is not parented to a pw_model
				object.  Please specify a pw_model object in the model field or parent the pw_recorder to one and try again.
				*/
				return 0;
			}
			model = parent;
		}
	} else {
		// isa model pw_model?
		if(!gl_object_isa(model, "pw_model")){
			gl_error("pw_recorder::init(): model of '%s' is not a pw_model", gl_name(model, objname, 63) );
			/*  TROUBLESHOOT
			The object specified in the model field of the pw_recorder is not a pw_model object.
			*/
			return 0;
		}
	}

	// check if model is initialized
	//	* if not, defer
	if((model->flags & OF_INIT) != OF_INIT){
		gl_verbose("pw_recorder::init(): deferring initialization on '%s'", gl_name(model, objname, 63));
		return 2; // defer
	}
	

	// assert key_strings not null
	if(0 == key_strings[0]){
		gl_error("pw_recorder::init(): key_strings not defined for '%s'", gl_name(model, objname, 63));
		/*  TROUBLESHOOT
		The key strings required for the pw_recorder are not specified.  Please specify these values and try again.
		*/
		return 0;
	}

	// assert key_values not null
	if(0 == key_values[0]){
		gl_error("pw_recorder::init(): key_values not defined for '%s'", gl_name(model, objname, 63));
		/*  TROUBLESHOOT
		The key value field required for the pw_recorder is not specified.  Please specify these values and try again.
		*/
		return 0;
	}

	// assert properties not null
	if(0 == properties[0]){
		gl_error("pw_recorder::init(): properties not defined for '%s'", gl_name(model, objname, 63));
		/*  TROUBLESHOOT
		The recording properties list is not defined for the pw_recorder object.  Please specify these properties and try again.
		*/
		return 0;
	}

	// generate GetParametersSingleElement VARIANT array
	// populate GPSE key values
	if(0 == build_keys()){
		gl_error("pw_recorder::init(): error when building keys for '%s'", gl_name(model, objname, 63));
		/*  TROUBLESHOOT
		An error occurred while building the keys for the pw_recorder.  Ensure your property values, key strings, and
		key values are correct and try again.
		*/
		return 0;
	}

	// set cModel
	cModel = OBJECTDATA(model, pw_model);

	// call GPSE
	// check values
	//	* if GPSE failed, mark 'ERROR' and return success ~ nonfatal error
	if(0 == GPSE()){
		gl_error("pw_recorder::init(): error when calling GetParameterSingleElement wrapper function for %s", gl_name(model, objname, 63));
		/*  TROUBLESHOOT
		An error occurred while trying to extract a value from PowerWorld.  Please check that your key string, key values, and property
		values are correct and try again.
		*/
		return 0;
	}

	// assert positive interval
	if(interval < -1){
		gl_error("pw_recorder::init(): invalid interval in '%s'", gl_name(model, objname, 63));
		/*  TROUBLESHOOT
		An invalid interval was specified in the pw_recorder.  Please specify a value in seconds, 0 (all iterations), or -1 (on change).
		*/
		return 0;
	}

	// prime the interval
	if(interval > 0){
		last_write = gl_globalclock - interval;
	}

	// check non-positive limit
	if(limit < 1){
		gl_verbose("pw_recorder::init(): '%s' will perform unlimited writes", gl_name(my(), objname, 63));
		limit = -1;
	}
	

	// check if outfile defined
	//	* if not, auto-generate
	if(0 == outfile_name[0]){
		sprintf(outfile_name, "%s-%u.csv", gl_name(my(), objname, 63), my()->id);
		gl_verbose("pw_recorder::init(): '%s' does not define a filename, auto-generating '%s'");
	}
	
	// open outfile
	//	* if unable, abort
	outfile = fopen(outfile_name, "w");
	if(0 == outfile){
		gl_error("pw_recorder::init(): unable to open outfile '%s' for writing", outfile_name);
		/*  TROUBLESHOOT
		pw_recorder was unable to open the output file.  Please ensure it isn't open in another program
		and try again.
		*/
		return 0;
	} else {
		if(!write_header()){
			gl_error("pw_recorder::init(): unable to write header for '%s'", gl_name(my(), objname, 63));
			/*  TROUBLESHOOT
			pw_recorder was unable to write the header in the output file.  Please ensure the file isn't
			open in another program and try again.
			*/
			return 0;
		}
		is_ready = true;
	}

	return 1;
}

int pw_recorder::precommit(TIMESTAMP t1){

	return 1;
}

TIMESTAMP pw_recorder::presync(TIMESTAMP t1){
	if(!cModel->get_valid_flag()){
		gl_verbose("not fetching voltage due to invalid model state");
	} else {
		if(0 == GPSE()){
			gl_error("pw_load::presync(): GPSE failed");
			/*	TROUBLESHOOT
			pw_recorder encountered an error when attempting to retrieve a parameter from PowerWorld.
			Please try again.  If the error persists, please submit your code an an error report via the
			trac website.
			 */
			return TS_INVALID;
		}
	}
	// substation will perform conversion on load_voltage for powerflow module

	return TS_NEVER;
}

TIMESTAMP pw_recorder::sync(TIMESTAMP t1){
	TIMESTAMP rt;
	if ((interval > 0) && is_ready && outfile){
		if(t1 >= (last_write + interval) ){
			interval_write = true;
			rt = last_write + interval + interval; // last_write is updated in commit()
		} else {
			interval_write = false;
			rt = last_write + interval;
		}
	} else {
		rt = TS_NEVER;
	}
	return rt;
}

TIMESTAMP pw_recorder::postsync(TIMESTAMP t1){
	if(0 == interval){
		write_line(t1);
	}
	return TS_NEVER;
}

TIMESTAMP pw_recorder::commit(TIMESTAMP t1, TIMESTAMP t2){
	if(outfile && is_ready){
		if(interval > 0 && interval_write){
			write_line(t1);
			interval_write = false;
		}
		if(interval == -1){
			if(strcmp(line_output, last_line_output) != 0){
				write_line(t1);
			}
		}
		if(limit > 0 && write_ct >= limit){
			is_ready = false;
		}
//		return last_write+interval;	// now handled by sync(), for the most part
	}
	return TS_NEVER;
}

int pw_recorder::isa(char *classname){
	return 1;
}

/**
	build_keys
	Validates the contents of key_strings, key_values, and properties, then
	converts them into a VARIANT array for later use in GetParameterSingleElement().
	@return		zero failure, nonzero success
 **/
int pw_recorder::build_keys(){
	char objname[256];
	int key_str_ct = 1, key_val_ct = 1, prop_str_ct = 1, i = 0;
	int index = 0, len = 0;
	char **string_ptrs = 0, **value_ptrs = 0, **prop_ptrs = 0, *context = 0;
	_variant_t HUGEP *field_data, HUGEP *value_data;
	HRESULT hr;

	// count key_strings
	len = (int)strlen(key_strings);
	for(index = 0; index < len; ++index){
		if(',' == key_strings[index]){
			++key_str_ct;
		}
	}
	
	// count key_values
	len = (int)strlen(key_values);
	for(index = 0; index < len; ++index){
		if(',' == key_values[index]){
			++key_val_ct;
		}
	}

	// compare counts
	if(0 == key_str_ct){
		gl_error("pw_recorder::build_keys(): '%s' did not parse any key strings", gl_name(my(), objname, 255));
		/*  TROUBLESHOOT
		pw_recorder was unable to parse the key strings specified.  Please ensure they are correct and try again.
		*/
		return 0;
	}
	if(0 == key_val_ct){
		gl_error("pw_recorder::build_keys(): '%s' did not parse any key values", gl_name(my(), objname, 255));
		/*  TROUBLESHOOT
		pw_recorder was unable to parse the key values specified.  Please ensure they are correct and try again.
		*/
		return 0;
	}
	if(key_str_ct != key_val_ct){
		gl_error("pw_recorder::build_keys(): '%s' has %i key properties and %i key values listed", gl_name(my(), objname, 255), key_str_ct, key_val_ct);
		/*  TROUBLESHOOT
		pw_recorder was unable to parse the key properties specified.  Please ensure they are correct and try again.
		*/
		return 0;
	}

	// prepare & allocate space for tokens
	strncpy(key_strings_copy, key_strings, 1024);
	strncpy(key_values_copy, key_values, 1024);

	string_ptrs = (char **)malloc(sizeof(char *) * key_str_ct);
	value_ptrs = (char **)malloc(sizeof(char *) * key_val_ct);

	/* didn't spot a portable 'count chr' function, using this as a quick-fix */
	// tokenize key_strings
	string_ptrs[0] = strtok_s(key_strings_copy, ",\n\r\t", &context);
	for(i = 1; i < key_str_ct; ++i){
		string_ptrs[i] = strtok_s(NULL, ",\n\r\t", &context);
	}

	// tokenize key_values
	value_ptrs[0] = strtok_s(key_values_copy, ",\n\r\t", &context);
	for(i = 1; i < key_val_ct; ++i){
		value_ptrs[i] = strtok_s(NULL, ",\n\r\t", &context);
	}
	
	key_count = key_str_ct;

	// count properties
	len = (int)strlen(properties);
	for(index = 0; index < len; ++index){
		if(',' == properties[index]){
			++prop_str_ct;
		}
	}

	prop_count = prop_str_ct;

	strncpy(props_copy, properties, 1024);

	prop_ptrs = (char **)malloc(sizeof(char *) * prop_count);
	out_values = (char **)malloc(prop_count * 64);

	// tokenize properties
	prop_ptrs[0] = strtok_s(props_copy, ",\n\r\t", &context);
	for(i = 1; i < prop_str_ct; ++i){
		prop_ptrs[i] = strtok_s(NULL, ",\n\r\t", &context);
	}
	
	// with everything tokenized, crunch it into VARIANTs
	// set bounds
	bounds[0].lLbound = 0;
	bounds[0].cElements = key_count + prop_count;

	type_bstr = _com_util::ConvertStringToBSTR(obj_classname);

	try {
		// allocate space for field names
		fields.vt = VT_ARRAY | VT_VARIANT;
		fields.parray = SafeArrayCreate(VT_VARIANT, 1, bounds);
		if(!fields.parray){
			throw _com_error(E_OUTOFMEMORY);
		}

		// write field names
		hr = SafeArrayAccessData(fields.parray, (void HUGEP **)&field_data);
		if(FAILED(hr)){
			throw _com_error(E_OUTOFMEMORY);
		}
		// key fields
		for(index = 0; index < key_count; ++index){
			field_data[index] = string_ptrs[index];
		}
		// then 'read' property fields
		for(; index < key_count + prop_count; ++index){
			field_data[index] = prop_ptrs[index-key_count];
		}
		SafeArrayUnaccessData(fields.parray);
	}
	catch(_com_error err){
		// @TODO this needs to be a gl_error, but err.ErrorMessage returns a TCHAR*
		std::cout << "!!! " << err.ErrorMessage() << "\n";
		return 0; // failure
	}
	catch(...){
		gl_error("Unknown exception in pw_recorder::build_keys()!");
		/*  TROUBLESHOOT
		An unknown exception was encountered by the pw_recorder object.  Please try again.
		If the error persists, please submit your code and a bug report via the trac website.
		*/
		return 0;
	}

	try {
		// allocate space for the values
		values.vt = VT_ARRAY | VT_VARIANT;
		values.parray = SafeArrayCreate(VT_VARIANT, 1, bounds);
		if(!values.parray){
			throw _com_error(E_OUTOFMEMORY);
		}

		// write key values
		hr = SafeArrayAccessData(values.parray, (void HUGEP **)&value_data);
		if(FAILED(hr)){
			throw _com_error(E_OUTOFMEMORY);
		}
		// key values
		for(index = 0; index < key_count; ++index){
			value_data[index] = value_ptrs[index];
		}
		// zero out 'read' property fields
		for(; index < key_count + prop_count; ++index){
			value_data[index] = _variant_t();
		}
		SafeArrayUnaccessData(values.parray);
		
	}
	catch(_com_error err){
		// @TODO this needs to be a gl_error, but err.ErrorMessage returns a TCHAR*
		std::cout << "!!! " << err.ErrorMessage() << "\n";
		return 0; // failure
	}
	catch(...){
		gl_error("Unknown exception in pw_recorder::build_keys()!");
		/*  TROUBLESHOOT
		An unknown exception was encountered by the pw_recorder object.  Please try again.
		If the error persists, please submit your code and a bug report via the trac website.
		*/
		return 0;
	}

	// success?

	return 1;
}

/**
	GPSE
	Wrapper for SimAuto's GetParameterSingleElement.  Fetches a stack of properties
	and writes the results to the specified output file.
	@return 0 on error, 1 on success
 **/
int pw_recorder::GPSE(){
	_variant_t HUGEP *pResults, HUGEP *pData;
	_variant_t results, data;
	char output[2048];
	char *tempstr;
	HRESULT hr;
	int i;
	bool first = true;
	// result = SimAuto->GPSE(L"Load", fields, values)
	memset(output, 0, sizeof(output));
	try{
		ISimulatorAutoPtr SimAuto(cModel->A);
		results = SimAuto->GetParametersSingleElement(type_bstr, fields, values);

		hr = SafeArrayAccessData(results.parray, (void HUGEP **)&pResults);

		if (((_bstr_t)(_variant_t)pResults[0]).length()){
			tempstr = _com_util::ConvertBSTRToString((_bstr_t)(_variant_t)pResults[0]);
			gl_error("Error from GetParametersSingleElement(): %s", tempstr);
			/* TROUBLESHOOT 
				The call to GetParametersSingleElement failed.  Please review the error message and respond accordingly.
				Addition COM-related error handling may be found on the MSDN website.
			 */
			delete [] tempstr;
			tempstr = 0;
			SafeArrayDestroy(fields.parray);
			SafeArrayDestroy(values.parray);
			return 0;
		}

		hr = SafeArrayAccessData(pResults[1].parray, (void HUGEP **)&pData);
		first = true;
		for(i = key_count; i < prop_count + key_count; ++i){
			tempstr = _com_util::ConvertBSTRToString(pData[i].bstrVal);
			if(!first){
				strcat(output, ",");
			} else {
				first = false;
			}
			strcat(output, tempstr);
			delete [] tempstr;
		}
		SafeArrayUnaccessData(pResults[1].parray);
		SafeArrayUnaccessData(results.parray);
	}
	catch(...){
		;
	}
	strncpy(line_output, output, sizeof(line_output));
	return 1;
}
/**
	write_header
	Writes the recorder header into the open outfile.
	@return		zero failure, nonzero success
 **/
int pw_recorder::write_header(){
	char namebuf[64];
	time_t now = time(NULL);

	fprintf(outfile, "# file...... %s\n", (const char*)outfile_name);
	fprintf(outfile, "# date...... %s", asctime(localtime(&now))); // adds its own newline
#ifdef WIN32
	fprintf(outfile, "# user...... %s\n", getenv("USERNAME"));
	fprintf(outfile, "# host...... %s\n", getenv("MACHINENAME"));
#else
	fprintf(outfile, "# user...... %s\n", getenv("USER"));
	fprintf(outfile, "# host...... %s\n", getenv("HOST"));
#endif
	fprintf(outfile, "# model..... %s\n", gl_name(model, namebuf, 63));
	fprintf(outfile, "# interval.. %d\n", interval);
	fprintf(outfile, "# limit..... %d\n", limit);
	fprintf(outfile, "# key_str... %s\n", (const char*)key_strings);
	fprintf(outfile, "# key_val... %s\n", (const char*)key_values);
	fprintf(outfile, "# timestamp,%s\n", (const char*)properties);
	return 1;
}

int pw_recorder::write_line(TIMESTAMP t1){
	DATETIME dt;
	char256 time_output;

	gl_localtime(t1, &dt);
	gl_strtime(&dt, time_output, sizeof(time_output));
	fprintf(outfile, "%s,%s\n",(const char *)time_output, (const char*)line_output);
	fflush(outfile);
	last_line_output.copy_from(line_output);

	++write_ct;
	last_write = t1;
	return 1;
}

#endif //PWX64
#endif //HAVE_POWERWORLD
// EOF
