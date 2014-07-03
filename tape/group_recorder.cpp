#include "group_recorder.h"

CLASS *group_recorder::oclass = NULL;
CLASS *group_recorder::pclass = NULL;
group_recorder *group_recorder::defaults = NULL;

void new_group_recorder(MODULE *mod){
	new group_recorder(mod);
}

group_recorder::group_recorder(MODULE *mod){
	if(oclass == NULL)
		{
#ifdef _DEBUG
		gl_debug("construction group_recorder class");
#endif
		oclass = gl_register_class(mod,"group_recorder",sizeof(group_recorder), PC_POSTTOPDOWN);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);
        
        if(gl_publish_variable(oclass,
			PT_char256, "file", PADDR(filename), PT_DESCRIPTION, "output file name",
			PT_char1024, "group", PADDR(group_def), PT_DESCRIPTION, "group definition string",
			PT_double, "interval[s]", PADDR(dInterval), PT_DESCRIPTION, "recordering interval (0 'every iteration', -1 'on change')",
			PT_double, "flush_interval[s]", PADDR(dFlush_interval), PT_DESCRIPTION, "file flush interval (0 never, negative on samples)",
			PT_bool, "strict", PADDR(strict), PT_DESCRIPTION, "causes the group_recorder to stop the simulation should there be a problem opening or writing with the group_recorder",
			PT_bool, "print_units", PADDR(print_units), PT_DESCRIPTION, "flag to append units to each written value, if applicable",
			PT_char256, "property", PADDR(property_name), PT_DESCRIPTION, "property to record",
			PT_int32, "limit", PADDR(limit), PT_DESCRIPTION, "the maximum number of lines to write to the file",
			PT_enumeration, "complex_part", PADDR(complex_part), PT_DESCRIPTION, "the complex part to record if complex properties are gathered",
				PT_KEYWORD, "NONE", NONE,
				PT_KEYWORD, "REAL", REAL,
				PT_KEYWORD, "IMAG", IMAG,
				PT_KEYWORD, "MAG", MAG,
				PT_KEYWORD, "ANG_DEG", ANG,
				PT_KEYWORD, "ANG_RAD", ANG_RAD,
		NULL) < 1){
			;//GL_THROW("unable to publish properties in %s",__FILE__);
		}
		defaults = this;
		memset(this, 0, sizeof(group_recorder));
    }
}

int group_recorder::create(){
	memcpy(this, defaults, sizeof(group_recorder));
	return 1;
}

int group_recorder::init(OBJECT *obj){
	OBJECT *gr_obj = 0;

	// check for group
	if(0 == group_def[0]){
		if(strict){
			gl_error("group_recorder::init(): no group defined");
			/* TROUBLESHOOT
				group_recorder must define a group in "group_def".
			 */
			return 0;
		} else {
			gl_warning("group_recorder::init(): no group defined");
			tape_status = TS_ERROR;
			return 1; // nothing more to do
		}
	}

	// check for filename
	if(0 == filename[0]){
		// if no filename, auto-generate based on ID
		if(strict){
			gl_error("group_recorder::init(): no filename defined in strict mode");
			return 0;
		} else {
			sprintf(filename, "%256s-%256i.csv", oclass->name, obj->id);
			gl_warning("group_recorder::init(): no filename defined, auto-generating '%s'", filename.get_string());
			/* TROUBLESHOOT
				group_recorder requires a filename.  If none is provided, a filename will be generated
				using a combination of the classname and the core-assigned ID number.
			*/
		}
	}

	// check valid write interval
	write_interval = (int64)(dInterval);
	if(-1 > write_interval){
		gl_error("group_recorder::init(): invalid write_interval of %i, must be -1 or greater", write_interval);
		/* TROUBLESHOOT
			The group_recorder interval must be -1, 0, or a positive number of seconds.
		*/
		return 0;
	}

	// all flush intervals are valid
	flush_interval = (int64)dFlush_interval;
	
	
	// build group
	//	* invariant?
	//	* non-empty set?
	items = gl_find_objects(FL_GROUP, group_def.get_string());
	if(0 == items){
		if(strict){
			gl_error("group_recorder::init(): unable to construct a set with group definition");
			/* TROUBLESHOOT
				An error occured while attempting to build and populate the find list with the specified group definition.
			 */
			return 0;
		} else {
			gl_warning("group_recorder::init(): unable to construct a set with group definition");
			tape_status = TS_ERROR;
			return 1; // nothing more to do
		}
	}
	if(1 > items->hit_count){
		if(strict){
			gl_error("group_recorder::init(): the defined group returned an empty set");
			/* TROUBLESHOOT
				Placeholder.
			 */
			return 0;
		} else {
			gl_warning("group_recorder::init(): the defined group returned an empty set");
			tape_status = TS_ERROR;
			return 1;
		}
	}
	
	// open file
	rec_file = fopen(filename.get_string(), "w");
	if(0 == rec_file){
		if(strict){
			gl_error("group_recorder::init(): unable to open file '%s' for writing", filename.get_string());
			return 0;
		} else {
			gl_warning("group_recorder::init(): unable to open file '%s' for writing", filename.get_string());
			/* TROUBLESHOOT
				If the group_recorder cannot open the specified output file, it will 
			 */
			tape_status = TS_ERROR;
			return 1;
		}
	}

	// turn list into objlist, count items
	obj_count = 0;
	for(gr_obj = gl_find_next(items, 0); gr_obj != 0; gr_obj = gl_find_next(items, gr_obj) ){
		prop_ptr = gl_get_property(gr_obj, property_name.get_string());
		// might make this a 'strict-only' issue in the future
		if(prop_ptr == NULL){
			gl_error("group_recorder::init(): unable to find property '%s' in an object of type '%s'", property_name.get_string(), gr_obj->oclass->name);
			/* TROUBLESHOOT
				An error occured while reading the specified property in one of the objects.
			 */
			return 0;
		}
		++obj_count;
		if(obj_list == 0){
			obj_list = new quickobjlist(gr_obj, prop_ptr);
		} else {
			obj_list->tack(gr_obj, prop_ptr);
		}
	}

	// check if we should expunge the units from our copied PROP structs
	if(!print_units){
		quickobjlist *itr = obj_list;
		for(; itr != 0; itr = itr->next){
			itr->prop.unit = NULL;
		}
	}

	tape_status = TS_OPEN;
	if(0 == write_header()){
		gl_error("group_recorder::init(): an error occured when writing the file header");
		/* TROUBLESHOOT
			Unexpected IO error.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	

	return 1;
}

TIMESTAMP group_recorder::postsync(TIMESTAMP t0, TIMESTAMP t1){
	// if we are strict and an error has occured, stop the simulation

	// if eventful interval, read
	if(0 == write_interval){//
		if(0 == read_line()){
			gl_error("group_recorder::sync");
			/* TROUBLESHOOT
				Placeholder.
			 */
			return 0;
		}
	} else if(0 < write_interval){
		// recalculate next_time, since we know commit() will fire
		if(last_write + write_interval <= t1){
			interval_write = true;
			last_write = t1;
			next_write = t1 + write_interval;
		}
		return next_write;
	} else {
		// on-change intervals simply short-circuit
		return TS_NEVER;
	}
	// if every iteration, write
	if(0 == write_interval){
		if(0 == write_line(t1) ){
			gl_error("group_recorder::sync(): error when writing the values to the file");
			/* TROUBLESHOOT
				Placeholder.
			 */
			return 0;
		}
		if(flush_interval < 0){
			if( ((write_count + 1) % (-flush_interval)) == 0 ){
				flush_line();
			}
		}
	}
	
	// the interval recorders have already return'ed out, earlier in the sequence.
	return TS_NEVER;
}

int group_recorder::commit(TIMESTAMP t1){
	// short-circuit if strict & error
	if((TS_ERROR == tape_status) && strict){
		gl_error("group_recorder::commit(): the object has error'ed and is halting the simulation");
		/* TROUBLESHOOT
			In strict mode, any group_recorder logic errors or input errors will
			halt the simulation.
		 */
		return 0;
	}

	// short-circuit if not open
	if(TS_OPEN != tape_status){
		return 1;
	}

	// if periodic interval, check for write
	if(write_interval > 0){
		if(interval_write){
			if(0 == read_line()){
				gl_error("group_recorder::commit(): error when reading the values");
				return 0;
			}
			if(0 == write_line(t1)){
				gl_error("group_recorder::commit(): error when writing the values to the file");
				return 0;
			}
			last_write = t1;
			interval_write = false;
		}
	}

	// if every change,
	//	* compare to last values
	//	* if different, write
	if(-1 == write_interval){
		if(0 == read_line()){
			if(0 == read_line()){
				gl_error("group_recorder::commit(): error when reading the values");
				return 0;
			}
			if(0 != strcmp(line_buffer, prev_line_buffer) ){
				if(0 == write_line(t1)){
					gl_error("group_recorder::commit(): error when writing the values to the file");
					return 0;
				}
			}
		}

	}

	// if periodic flush, check for flush
	if(flush_interval > 0){
		if(last_flush + flush_interval <= t1){
			last_flush = t1;
		}
	} else if(flush_interval < 0){
		if( ((write_count + 1) % (-flush_interval)) == 0 ){
			flush_line();
		}
	} // if 0, no flush

	// check if write limit
	if(limit > 0 && write_count >= limit){
		// write footer
		write_footer();
		fclose(rec_file);
		rec_file = 0;
		free(line_buffer);
		line_buffer = 0;
		line_size = 0;
		tape_status = TS_DONE;
	}

	// check if strict & error ... a second time in case the periodic behavior failed.
	if((TS_ERROR == tape_status) && strict){
		gl_error("group_recorder::commit(): the object has error'ed and is halting the simulation");
		/* TROUBLESHOOT
			In strict mode, any group_recorder logic errors or input errors will
			halt the simulation.
		 */
		return 0;
	}

	return 1;
}

int group_recorder::isa(char *classname){
	return (strcmp(classname, oclass->name) == 0);
}

/**
	@return 0 on failure, 1 on success
 **/
int group_recorder::write_header(){
//	size_t name_size;
	time_t now = time(NULL);
	quickobjlist *qol = 0;
	OBJECT *obj=OBJECTHDR(this);

	if(TS_OPEN != tape_status){
		// could be ERROR or CLOSED
		return 0;
	}
	if(0 == rec_file){
		gl_error("group_recorder::write_header(): the output file was not opened");
		/* TROUBLESHOOT
			group_recorder claimed to be open and attempted to write to a file when
			a file had not successfully	opened.
		 */
		tape_status = TS_ERROR;
		return 0; // serious problem
	}

	// write model file name
	if(0 > fprintf(rec_file,"# file...... %s\n", filename.get_string())){ return 0; }
	if(0 > fprintf(rec_file,"# date...... %s", asctime(localtime(&now)))){ return 0; }
#ifdef WIN32
	if(0 > fprintf(rec_file,"# user...... %s\n", getenv("USERNAME"))){ return 0; }
	if(0 > fprintf(rec_file,"# host...... %s\n", getenv("MACHINENAME"))){ return 0; }
#else
	if(0 > fprintf(rec_file,"# user...... %s\n", getenv("USER"))){ return 0; }
	if(0 > fprintf(rec_file,"# host...... %s\n", getenv("HOST"))){ return 0; }
#endif
	if(0 > fprintf(rec_file,"# group..... %s\n", group_def.get_string())){ return 0; }
	if(0 > fprintf(rec_file,"# property.. %s\n", property_name.get_string())){ return 0; }
	if(0 > fprintf(rec_file,"# limit..... %d\n", limit)){ return 0; }
	if(0 > fprintf(rec_file,"# interval.. %d\n", write_interval)){ return 0; }

	// write list of properties
	if(0 > fprintf(rec_file, "# timestamp")){ return 0; }
	for(qol = obj_list; qol != 0; qol = qol->next){
		if(0 != qol->obj->name){
			if(0 > fprintf(rec_file, ",%s", qol->obj->name)){ return 0; }
		} else {
			if(0 > fprintf(rec_file, ",%s:%i", qol->obj->oclass->name, qol->obj->id)){ return 0; }
		}
	}
	if(0 > fprintf(rec_file, "\n")){ return 0; }
	return 1;
}

/**
	@return 0 on failure, 1 on success
 **/
int group_recorder::read_line(){
	size_t index = 0, offset = 0, unit_len = 0;
	quickobjlist *curr = 0;
	char *swap_ptr = 0;
	char buffer[128];
	char objname[128];

	if(TS_OPEN != tape_status){
		// could be ERROR or CLOSED
		return 0;
	}

	// pre-calculate buffer needs
	if(line_size <= 0 || line_buffer == 0){
		size_t prop_size;
		
		// in v2.3, there's no measure of the property's string representation size.
		//	this value *is* present in 3.0.
		prop_size = 48;

		line_size = (prop_size + 1) * obj_count + 1;
		line_buffer = (char *)malloc(line_size);
		if(0 == line_buffer){
			return 0;
		}
		memset(line_buffer, 0, line_size);
		if(-1 == write_interval){ // 'on change', will need second buffer
			prev_line_buffer = (char *)malloc(line_size);
			if(0 == prev_line_buffer){
				gl_error("group_recorder::read_line(): malloc failure");
				/* TROUBLESHOOT
					Memory allocation failure.
				*/
				return 0;
			}
			memset(prev_line_buffer, 0, line_size);
		}
	}

	// if we need the previous buffer to compare against, swap the buffers
	if(-1 == write_interval){
		swap_ptr = prev_line_buffer;
		prev_line_buffer = line_buffer;
		line_buffer = swap_ptr;
	}
	memset(line_buffer, 0, line_size);
	for(curr = obj_list; curr != 0; curr = curr->next){
		// GETADDR is a macro defined in object.h
		if(curr->prop.ptype == PT_complex && complex_part != NONE){
			double part_value = 0.0;
			complex *cptr = 0;
			// get value as a complex
			cptr = gl_get_complex(curr->obj, &(curr->prop));
			if(0 == cptr){
				gl_error("group_recorder::read_line(): unable to get complex property '%s' from object '%s'", curr->prop.name, gl_name(curr->obj, objname, 127));
				/* TROUBLESHOOT
					Could not read a complex property as a complex value.
				 */
				return 0;
			}
			// switch on part
			switch(complex_part){
				case NONE:
					// didn't we test != NONE just a few lines ago?
					gl_error("group_recorder::read_line(): inconsistant complex_part states!");
					return 0;
				case REAL:
					part_value = cptr->Re();
					break;
				case IMAG:
					part_value = cptr->Im();
					break;
				case MAG:
					part_value = cptr->Mag();
					break;
				case ANG:
					part_value = cptr->Arg() * 180/PI;
					break;
				case ANG_RAD:
					part_value = cptr->Arg();
					break;
			}
			sprintf(buffer, "%f", part_value);
			offset = strlen(buffer);
		} else {
			offset = gl_get_value(curr->obj, GETADDR(curr->obj, &(curr->prop)), buffer, 127, &(curr->prop));
			if(0 == offset){
				gl_error("group_recorder::read_line(): unable to get value for '%s' in object '%s'", curr->prop.name, curr->obj->name);
				/* TROUBLESHOOT
					An error occured while reading the specified property in one of the objects.
				 */
				return 0;
			}
		}
		// check line_buffer space
		if( (index + offset + 1) > line_size ){
			gl_error("group_recorder::read_line(): potential buffer overflow from a too-short automatically sized output value buffer");
			/* TROUBLESHOOT
				A potential buffer overflow was caught, most likely due to incorrect property
				representation assumptions for buffer allocation.
			 */
			return 0;
		}
		// write to line_buffer
		// * lead with a comma on all entries, assume leading timestamp will NOT print a comma
		if(0 >= sprintf(line_buffer+index, ",%s", buffer)){return 0;}
		index += (offset + 1); // add the comma
	}
	// assume write_line will add newline character

	return 1;
}

/**
	@return 1 on successful write, 0 on unsuccessful write, error, or when not ready
 **/
int group_recorder::write_line(TIMESTAMP t1){
	char time_str[64];
	DATETIME dt;

	if(TS_OPEN != tape_status){
		gl_error("group_recorder::write_line(): trying to write line when the tape is not open");
		// could be ERROR or CLOSED, should not have happened
		return 0;
	}
	if(0 == rec_file){
		gl_error("group_recorder::write_line(): no output file open and state is 'open'");
		/* TROUBLESHOOT
			group_recorder claimed to be open and attempted to write to a file when
			a file had not successfully	opened.
		 */
		tape_status = TS_ERROR;
		return 0;
	}

	// check that buffer needs were pre-calculated
	if(line_size <= 0 || line_buffer == 0){
		gl_error("group_recorder::write_line(): output buffer not initialized (read_line() not called)");
		/* TROUBLESHOOT
			read_line was not called before write_line, indicating an internal logic error.
		 */
		tape_status = TS_ERROR;
		return 0;
	}

	// write time_str
	// recorder.c uses multiple formats, in the sense of "formatted or not".  This does not.
	if(0 == gl_localtime(t1, &dt)){
		gl_error("group_recorder::write_line(): error when converting the sync time");
		/* TROUBLESHOOT
			Unprintable timestamp.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	if(0 == gl_strtime(&dt, time_str, sizeof(time_str) ) ){
		gl_error("group_recorder::write_line(): error when writing the sync time as a string");
		/* TROUBLESHOOT
			Error printing the timestamp.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	// print line to file
	if(0 >= fprintf(rec_file, "%s%s\n", time_str, line_buffer)){
		gl_error("group_recorder::write_line(): error when writing to the output file");
		/* TROUBLESHOOT
			File I/O error.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	++write_count;
	
	return 1;
}

/**
	@return 0 on failure, 1 on success
 **/
int group_recorder::flush_line(){
	if(TS_OPEN != tape_status){
		gl_error("group_recorder::flush_line(): tape is not open");
		// could be ERROR or CLOSED, should not have happened
		return 0;
	}
	if(0 == rec_file){
		gl_error("group_recorder::flush_line(): output file is not open");
		/* TROUBLESHOOT
			group_recorder claimed to be open and attempted to flush to a file when
			a file had not successfully	opened.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	if(0 != fflush(rec_file)){
		gl_error("group_recorder::flush_line(): unable to flush output file");
		/* TROUBLESHOOT
			An IO error has occured.
		 */
		tape_status = TS_ERROR;
		return 0;
	}
	return 1;
}

/**
	@return 0 on failure, 1 on success
 **/
int group_recorder::write_footer(){
	if(TS_OPEN != tape_status){
		gl_error("group_recorder::write_footer(): tape is not open");
		// could be ERROR or CLOSED, should not have happened
		return 0;
	}
	if(0 == rec_file){
		gl_error("group_recorder::write_footer(): output file is not open");
		/* TROUBLESHOOT
			group_recorder claimed to be open and attempted to write to a file when
			a file had not successfully	opened.
		 */
		tape_status = TS_ERROR;
		return 0;
	}

	// not a lot to this one.
	if(0 >= fprintf(rec_file, "# end of file\n")){ return 0; }

	return 1;
}

//////////////////////////////


EXPORT int create_group_recorder(OBJECT **obj, OBJECT *parent){
	int rv = 0;
	try {
		*obj = gl_create_object(group_recorder::oclass);
		if(*obj != NULL){
			group_recorder *my = OBJECTDATA(*obj, group_recorder);
			gl_set_parent(*obj, parent);
			rv = my->create();
		}
	}
	catch (char *msg){
		gl_error("create_group_recorder: %s", msg);
	}
	catch (const char *msg){
		gl_error("create_group_recorder: %s", msg);
	}
	catch (...){
		gl_error("create_group_recorder: unexpected exception caught");
	}
	return rv;
}

EXPORT int init_group_recorder(OBJECT *obj){
	group_recorder *my = OBJECTDATA(obj, group_recorder);
	int rv = 0;
	try {
		rv = my->init(obj->parent);
	}
	catch (char *msg){
		gl_error("init_group_recorder: %s", msg);
	}
	catch (const char *msg){
		gl_error("init_group_recorder: %s", msg);
	}
	return rv;
}

EXPORT TIMESTAMP sync_group_recorder(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass){
	group_recorder *my = OBJECTDATA(obj, group_recorder);
	TIMESTAMP rv = 0;
	try {
		switch(pass){
			case PC_PRETOPDOWN:
				rv = TS_NEVER;
				break;
			case PC_BOTTOMUP:
				rv = TS_NEVER;
				break;
			case PC_POSTTOPDOWN:
				rv = my->postsync(obj->clock, t0);
				obj->clock = t0;
				break;
			default:
				throw "invalid pass request";
		}
	}
	catch(char *msg){
		gl_error("sync_group_recorder: %s", msg);
	}
	catch(const char *msg){
		gl_error("sync_group_recorder: %s", msg);
	}
	return rv;
}

EXPORT int commit_group_recorder(OBJECT *obj){
	int rv = 0;
	group_recorder *my = OBJECTDATA(obj, group_recorder);
	try {
		rv = my->commit(obj->clock);
	}
	catch (char *msg){
		gl_error("commit_group_recorder: %s", msg);
	}
	catch (const char *msg){
		gl_error("commit_group_recorder: %s", msg);
	}
	return rv;
}

EXPORT int isa_group_recorder(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj, group_recorder)->isa(classname);
}

// EOF
