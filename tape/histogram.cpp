// $Id: histogram.cpp 4738 2014-07-03 00:55:39Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file histogram.cpp
	@addtogroup tape histogram
	@ingroup tape
	
	@{
*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <float.h>

#include "histogram.h"

//initialize pointers
CLASS* histogram::oclass = NULL;
CLASS* histogram::pclass = NULL;
histogram *histogram::defaults = NULL;

//////////////////////////////////////////////////////////////////////////
// histogram CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

void new_histogram(MODULE *mod){
	new histogram(mod);
}

histogram::histogram(MODULE *mod)
{
	if(oclass == NULL)
	{
#ifdef _DEBUG
		gl_debug("construction histogram class");
#endif
		oclass = gl_register_class(mod,"histogram",sizeof(histogram), PC_PRETOPDOWN);
		if (oclass==NULL)
			throw "unable to register class histogram";
		else
			oclass->trl = TRL_PROTOTYPE;
        
        if(gl_publish_variable(oclass,
			PT_char1024, "filename", PADDR(filename),PT_DESCRIPTION,"the name of the file to write",
			PT_char8, "filetype", PADDR(ftype),PT_DESCRIPTION,"the format to output a histogram in",
			PT_char32, "mode", PADDR(mode),PT_DESCRIPTION,"the mode of file output",
			PT_char1024, "group", PADDR(group),PT_DESCRIPTION,"the GridLAB-D group expression to use for this histogram",
			PT_char1024, "bins", PADDR(bins),PT_DESCRIPTION,"the specific bin values to use",
			PT_char256, "property", PADDR(property),PT_DESCRIPTION,"the property to sample",
			PT_double, "min", PADDR(min),PT_DESCRIPTION,"the minimum value of the auto-sized bins to use",
			PT_double, "max", PADDR(max),PT_DESCRIPTION,"the maximum value of the auto-sized bins to use",
			PT_double, "samplerate[s]", PADDR(sampling_interval),PT_DESCRIPTION,"the rate at which samples are read",
			PT_double, "countrate[s]", PADDR(counting_interval),PT_DESCRIPTION,"the reate at which bins are counted and written",
			PT_int32, "bin_count", PADDR(bin_count),PT_DESCRIPTION,"the number of auto-sized bins to use",
			PT_int32, "limit", PADDR(limit),PT_DESCRIPTION,"the number of samples to write",
            NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(filename, 0, 1025);
		memset(group, 0, 1025);
		memset(bins, 0, 1025);
		memset(property, 0, 33);
		min = 0;
		max = -1;
		bin_count = -1;
		sampling_interval = -1.0;
		counting_interval = -1.0;
		limit = 0;
		bin_list = NULL;
		group_list = NULL;
		binctr = NULL;
		prop_ptr = NULL;
		next_count = t_count = next_sample = t_sample = TS_ZERO;
		strcpy(mode, "file");
		flags[0]='w';
    }
}

int histogram::create()
{
	memcpy(this, defaults, sizeof(histogram));
	return 1;
}


/* consume spaces */
void eat_white(char **pos){
	while(**pos == ' ' || **pos == '\t')
		++(*pos);
}

/**
*	Token parsing to extract bin range
*	@return 0 on error
*/
int parse_bin_val(char *tok, BIN *bin){
	char *pos = tok;
	int low_set = 0, high_set = 0;
	
	eat_white(&pos);

	/* check for bracket or parenthesis */
	if(pos[0] == '['){
		++pos;
		bin->low_inc = 1;
	} else if (pos[0] == '('){
		++pos;
		bin->low_inc = 0;
	} else {
		bin->low_inc = 1;	/* default inclusive on lower bound */
	}
	
	/* consume whitespace */
	eat_white(&pos);
	
	/* check for ellipsis or for value */
	if(isdigit(*pos)){
		bin->low_val = strtod(pos, &pos);
		low_set = 1;
	} else if(*pos == '.' && pos[1] == '.'){ /* "everything less than the other value" */
		bin->low_val = -DBL_MAX;
		bin->low_inc = 0;
	} else if(*pos == '-') {
		bin->low_val = -DBL_MAX;
		bin->low_inc = 0;

	}
	
	eat_white(&pos);
	
	if(*pos == '.' && pos[1] == '.'){
		pos+=2;
	} else if (*pos == '-'){
		++pos;
	} else {
		gl_error("parse_bin failure");
		return 0;
	}
	
	eat_white(&pos);
	
	if(isdigit(*pos)){
		bin->high_val = strtod(pos, &pos);
		high_set = 1;
		if(low_set == 0)
			bin->high_inc = 1;
	} else {
		bin->high_val = DBL_MAX;
	}

	if(*pos == 0 || *pos == '\n' || *pos == ','){
		return 1;	/* reached the end of the bin definition*/
	} else if (*pos == ')'){
		bin->high_inc = 0;
		return 1;
	} else if (*pos == ']'){
		bin->high_inc = 1;
		return 1;
	}
	return 0;
}

int parse_bin_enum(char *cptr, BIN *bin, PROPERTY *prop){
	/* cptr should just be one token */
	char *pos = cptr;
	KEYWORD *kw = NULL;
	
	if(prop->keywords == NULL){
		gl_error("parse_bin_enum error: property has no keywords");
		return 0;
	}
	
	eat_white(&pos);
	
	for(kw = prop->keywords; kw != NULL; kw = kw->next){
		if(strcmp(kw->name, pos) == 0){
			bin->low_inc = bin->high_inc = 1;
			bin->low_val = bin->high_val = (double)(kw->value);
			return 1;
		}
	}
	
	gl_error("parse_bin_enum error: unable to find property \'%s\'", pos);
	return 0;
}

/**
 *	Local convenience function, used to extract the complex part of a property string (if any) and
 *	add a null character to the property string accordingly.
 *	@param tprop a pointer to the buffer for the property name
 *	@param tpart a pointer to the buffer for the property complex type
 */
void histogram::test_for_complex(char *tprop, char *tpart){

	if(sscanf(property.get_string(), "%[^.\n\t ].%s", tprop, tpart) == 2){
		if(0 == memcmp(tpart, "real", 4)){comp_part = REAL;}
		else if(0 == memcmp(tpart, "imag", 3)){comp_part = IMAG;}
		else if(0 == memcmp(tpart, "mag", 3)){comp_part = MAG;}
		else if(0 == memcmp(tpart, "ang", 3)){comp_part = ANG;}
		else {
			comp_part = NONE;
			throw("Unable to resolve complex part for \'%s\'", property.get_string());
			return;
		}
		strtok(property, "."); /* "quickly" replaces the dot with a space */
	}
}

/**
* Object initialization is called once after all object have been created
*
* @param parent a pointer to this object's parent
* @return 1 on success, 0 on error
*/
int histogram::init(OBJECT *parent)
{
	PROPERTY *prop = NULL;
	OBJECT *obj = OBJECTHDR(this);
	char tprop[64], tpart[8];
	int e = 0;
	TAPEFUNCS *tf = 0;
	tprop[0]=0;
	tpart[0] = 0;

	if(parent == NULL) /* better have a group... */
	{
		OBJECT *group_obj = NULL;
		CLASS *oclass = NULL;
		if(group[0] == NULL){
			gl_error("Histogram has no parent and no group");
			return 0;
		}
		group_list = gl_find_objects(FL_GROUP,group.get_string());
		if(group_list == NULL){
			gl_error("Histogram group could not be parsed");
			return 0;
		}
		if(group_list->hit_count < 1){
			gl_error("Histogram group is an empty set");
			return 0;
		}
		/* non-empty set */

		/* parse complex part of property */
		test_for_complex(tprop, tpart);
	
		while(group_obj = gl_find_next(group_list, group_obj)){
			prop = gl_find_property(group_obj->oclass, property.get_string());
			if(prop == NULL){
				gl_error("Histogram group is unable to find prop '%s' in class '%s' for group '%s'", property.get_string(), group_obj->oclass->name, group.get_string());
				return 0;
			}
			/* check to see if all the group objects are in the same class, allowing us to cache the target property */
			if (oclass == NULL){
				oclass = group_obj->oclass;
				prop_ptr = prop;
			}
			if(oclass != group_obj->oclass){
				prop_ptr = NULL;
			} 
			
		}
	} else { /* if we have a parent, we only focus on that one object */

		test_for_complex(tprop, tpart);

		prop = gl_find_property(parent->oclass, property.get_string());
		
		if(prop == NULL){
			gl_error("Histogram parent '%s' of class '%s' does not contain property '%s'", parent->name ? parent->name : "(anon)", parent->oclass->name, property.get_string());
			return 0;
		} else {
			prop_ptr = prop; /* saved for later */
		}
	}

	// initialize first timesteps
	if(counting_interval > 0.0)
		next_count = gl_globalclock + counting_interval;
	if(sampling_interval > 0.0)
		next_sample = gl_globalclock + sampling_interval;
	/*
	 *	This will create a uniform partition over the specified range
	 */
	if((bin_count > 0) && (min < max))
	{
		int i=0;
		double range = max - min;
		double step = range/bin_count;
		//throw("Histogram bin_count is temporarily disabled.");
		bin_list = (BIN *)gl_malloc(sizeof(BIN) * bin_count);
		if(bin_list == NULL){
			gl_error("Histogram malloc error: unable to alloc %i * %i bytes for %s", bin_count, sizeof(BIN), obj->name ? obj->name : "(anon. histogram)");
			return 0;
		}
		memset(bin_list, 0, sizeof(BIN) * bin_count);
		for(i = 0; i < bin_count; i++){
			bin_list[i].low_val = min + i * step;
			bin_list[i].high_val = bin_list[i].low_val + step;
			bin_list[i].low_inc = 1;
			bin_list[i].high_inc = 0;
		}
		bin_list[i-1].high_inc = 1;	/* tail value capture */
		binctr = (int *)gl_malloc(sizeof(int) * bin_count);
		memset(binctr, 0, sizeof(int) * bin_count);
	}
	else if (bins[0] != 0)
	/*
	 *	This will parse the bin strings as specified.  The valid basic form is "(a..b)".  Brackets are optional, [ & ] are inclusive.  A dash may replace the '..'.
	 *	If a or b is not present, the bin will fill in a positve/negative infinity.
	 */
	{
		char *cptr = bins;
		char bincpy[1025];
		int i = 0;
		bin_count = 1; /* assume at least one */
		/* would be better to count the number of times strtok succeeds, but this should work -mh */
		//while(*cptr != 0){
		for(cptr = bins; *cptr != 0; ++cptr){
			if(*cptr == ',' && cptr[1] != 0){
				++bin_count;
			}
		//	++cptr;
		}
		bin_list = (BIN *)gl_malloc(sizeof(BIN) * bin_count);
		if(bin_list == NULL){
			gl_error("Histogram malloc error: unable to alloc %i * %i bytes for %s", bin_count, sizeof(BIN), obj->name ? obj->name : "(anon. histogram)");
			return 0;
		}
		memset(bin_list, 0, sizeof(BIN) * bin_count);
		memcpy(bincpy, bins, 1024);
		cptr = strtok(bincpy, ",\t\r\n\0");
		if(prop->ptype == PT_complex || prop->ptype == PT_double || prop->ptype == PT_int16 || prop->ptype == PT_int32 || prop->ptype == PT_int64 || prop->ptype == PT_float || prop->ptype == PT_real){
			for(i = 0; i < bin_count && cptr != NULL; ++i){
				if(parse_bin_val(cptr, bin_list+i) == 0){
					gl_error("Histogram unable to parse \'%s\' in %s", cptr, obj->name ? obj->name : "(unnamed histogram)");
					return 0;
				}
				cptr = strtok(NULL, ",\t\r\n\0"); /* minor efficiency gain to use the incremented pointer from parse_bin */
			}
		} else if (prop->ptype == PT_enumeration || prop->ptype == PT_set){
			for(i = 0; i < bin_count && cptr != NULL; ++i){
				if(parse_bin_enum(cptr, bin_list+i, prop) == 0){
					gl_error("Histogram unable to parse \'%s\' in %s", cptr, obj->name ? obj->name : "(unnamed histogram)");
					return 0;
				}
				cptr = strtok(NULL, ",\t\r\n\0"); /* minor efficiency gain to use the incremented pointer from parse_bin */
			}
		}

		if(i < bin_count){
			gl_error("Histrogram encountered a problem parsing bins for %s", obj->name ? obj->name : "(unnamed histogram)");
			return 0;
		}
		binctr = (int *)malloc(sizeof(int) * bin_count);
		memset(binctr, 0, sizeof(int) * bin_count);
	} else {
		gl_error("Histogram has neither bins or a bin range to work with");
		return 0;
	}

	/* open file ~ copied from recorder.c */
		/* if prefix is omitted (no colons found) */
//	if (sscanf(filename,"%32[^:]:%1024[^:]:%[^:]",ftype,fname,flags)==1)
//	{
		/* filename is file by default */
		strcpy(fname,filename);
//		strcpy(ftype,"file");
//	}
	/* if no filename given */
	if (strcmp(fname,"")==0)

		/* use object name-id as default file name */
		sprintf(fname,"%s-%d.%s",obj->parent->oclass->name,obj->parent->id, ftype.get_string());

	/* if type is file or file is stdin */
	tf = get_ftable(mode);
	if(tf == NULL)
		return 0;
	ops = tf->histogram; /* same mentality as a recorder, 'cept for the header properties */
	if(ops == NULL)
		return 0;
	return ops->open(this, fname, flags);
}

int histogram::feed_bins(OBJECT *obj){
	double value = 0.0;
	complex cval = 0.0; //gl_get_complex(obj, ;
	int64 ival = 0;
	int i = 0;

	switch(prop_ptr->ptype){
		case PT_complex:
			cval = (prop_ptr ? *gl_get_complex(obj, prop_ptr) : *gl_get_complex_by_name(obj, property) );
			switch(this->comp_part){
				case REAL:
					value = cval.Re();
					break;
				case IMAG:
					value = cval.Im();
					break;
				case MAG:
					value = cval.Mag();
					break;
				case ANG:
					value = cval.Arg();
					break;
				default:
					gl_error("Complex property with no part defined in %s", (obj->name ? obj->name : "(unnamed)"));
			}
			ival = 1;
			/* fall through */
		case PT_double:
			if(ival == 0) 
				value = (prop_ptr ? *gl_get_double(obj, prop_ptr) : *gl_get_double_by_name(obj, property.get_string()) );
			for(i = 0; i < bin_count; ++i){
				if(value > bin_list[i].low_val && value < bin_list[i].high_val){
					++binctr[i];
				} else if(bin_list[i].low_inc && bin_list[i].low_val == value){
					++binctr[i];
				} else if(bin_list[i].high_inc && bin_list[i].high_val == value){
					++binctr[i];
				}
			}
			break;
		case PT_int16:
			ival = (prop_ptr ? *gl_get_int16(obj, prop_ptr) : *gl_get_int16_by_name(obj, property.get_string()) );
			value = 1.0;
		case PT_int32:
			if(value == 0.0){
				ival = (prop_ptr ? *gl_get_int32(obj, prop_ptr) : *gl_get_int32_by_name(obj, property.get_string()) );
				value = 1.0;
			}
		case PT_int64:
			if(value == 0.0){
				ival = (prop_ptr ? *gl_get_int64(obj, prop_ptr) : *gl_get_int64_by_name(obj, property.get_string()) );
				value = 1.0;
			}
		case PT_enumeration:
			if(value == 0.0){
				ival = (prop_ptr ? *gl_get_enum(obj, prop_ptr) : *gl_get_enum_by_name(obj, property.get_string()) );
				value = 1.0;
			}
		case PT_set:
			if(value == 0.0){
				ival = (prop_ptr ? *gl_get_set(obj, prop_ptr) : *gl_get_set_by_name(obj, property.get_string()) );
				value = 1.0;
			}
			
			/* may be prone to fractional errors */
			for(i = 0; i < bin_count; ++i){
				if(ival > bin_list[i].low_val && ival < bin_list[i].high_val){
					++binctr[i];
				} else if(bin_list[i].low_inc && bin_list[i].low_val == ival){
					++binctr[i];
				} else if(bin_list[i].high_inc && bin_list[i].high_val == ival){
					++binctr[i];
				}
			}
			break;
	}

	return 0;
}

TIMESTAMP histogram::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	int i = 0;
	double value = 0.0;
	OBJECT *obj = OBJECTHDR(this);

	if((sampling_interval == -1.0 && t_count > t1) ||
		sampling_interval == 0.0 ||
		(sampling_interval > 0.0 && t1 >= next_sample))
	{
		if(group_list == NULL){
			feed_bins(obj->parent);
		} else {
			OBJECT *obj = gl_find_next(group_list, NULL);
			for(; obj != NULL; obj = gl_find_next(group_list, obj)){
				feed_bins(obj);
			}
		}
		t_sample = t1;
		if(sampling_interval > 0.0001){
			next_sample = t1 + (int64)(sampling_interval/TS_SECOND);
		} else {
			next_sample = TS_NEVER;
		}
	}

	if((counting_interval == -1.0 && t_count < t1) ||
		counting_interval == 0.0 ||
		(counting_interval > 0.0 && t1 >= next_count))
	{
		char line[1025];
		char ts[64];
		int off=0, i=0;
		DATETIME dt;

		/* write the timestamp */
		gl_localtime(t1,&dt);
		gl_strtime(&dt,ts,64);
		
		/* write bins */
		for(i = 0; i < bin_count; ++i){
			off += sprintf(line+off, "%i", binctr[i]);
			if(i != bin_count){
				off += sprintf(line+off, ",");
			}
		}

		/* write line */
		ops->write(this, ts, line);
		
		/* cleanup */
		for(i = 0; i < bin_count; ++i){
			binctr[i] = 0;
		}
		t_count = t1;
		if(counting_interval > 0){
			next_count = t_count + (int64)(counting_interval/TS_SECOND);
		} else {
			next_count = TS_NEVER;
		}


		if(--limit < 1){
			ops->close(this);
			next_count = TS_NEVER;
			next_sample = TS_NEVER;
		}
	}
	return ( next_count < next_sample && counting_interval > 0.0 ? next_count : next_sample );
}

int histogram::isa(char *classname)
{
	return strcmp(classname,"histogram")==0;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: histogram
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_histogram(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(histogram::oclass);
		if (*obj!=NULL)
		{
			histogram *my = OBJECTDATA(*obj,histogram);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	} 
	catch (char *msg)
	{
		gl_error("create_histogram: %s", msg);
	}
	return 0;
}

EXPORT int init_histogram(OBJECT *obj)
{
	histogram *my = OBJECTDATA(obj,histogram);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		GL_THROW("%s (histogram:%d): %s", (obj->name ? obj->name : "(unnamed)"), obj->id, msg);
		return 0; 
	}
}

EXPORT TIMESTAMP sync_histogram(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	histogram *pObj = OBJECTDATA(obj,histogram);
	try {
		if (pass == PC_PRETOPDOWN)
			return pObj->sync(obj->clock, t0);
	} catch (const char *error) {
		GL_THROW("%s (histogram:%d): %s", (obj->name ? obj->name : "(unnamed)"), obj->id, error);
		return 0; 
	} catch (...) {
		GL_THROW("%s (histogram:%d): %s", (obj->name ? obj->name : "(unnamed)"), obj->id, "unknown exception");
		return 0;
	}
	return TS_NEVER;
}

/**
* Allows the core to discover whether obj is a subtype of this class.
*
* @param obj a pointer to this object
* @param classname the name of the object the core is testing
*
* @return true (1) if obj is a subtype of this class
*/
EXPORT int isa_histogram(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,histogram)->isa(classname);
}

/**@}*/
