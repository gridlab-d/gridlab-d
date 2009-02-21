// $Id: histogram.cpp 1186 2009-01-02 18:15:30Z dchassin $
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

#include "histogram.h"

//initialize pointers
CLASS* histogram::oclass = NULL;
CLASS* histogram::pclass = NULL;
histogram *histogram::defaults = NULL;

//////////////////////////////////////////////////////////////////////////
// histogram CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

histogram::histogram(MODULE *mod)
{
	if(oclass == NULL)
	{
		oclass = gl_register_class(mod,"histogram",sizeof(histogram), PC_PRETOPDOWN);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);
        
        if(gl_publish_variable(oclass,
			PT_char1024, "filename", PADDR(filename),
			PT_char1024, "group", PADDR(group),
			PT_char1024, "bins", PADDR(bins),
			PT_char32, "property", PADDR(property),
			PT_double, "min", PADDR(min),
			PT_double, "max", PADDR(max),
			PT_int32, "samplerate", PADDR(sampling_interval),
			PT_int32, "countrate", PADDR(counting_interval),
			PT_int32, "bin_count", PADDR(bin_count),
			PT_int32, "count", PADDR(line_count),
            NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(filename, 0, 1025);
		memset(group, 0, 1025);
		memset(bins, 0, 1025);
		memset(property, 0, 33);
		min = 0;
		max = -1;
		bin_count = -1;
		sampling_interval = -1;
		counting_interval = -1;
		line_count = 0;
		bin_list = NULL;
		group_list = NULL;
		binctr = NULL;
    }
}

int histogram::create()
{
	memcpy(this, defaults, sizeof(histogram));
	return 0;
}


/* consume spaces */
void eat_white(char **pos){
	while(**pos == ' ')
		++(*pos);
}

/**
*	Token parsing to extract bin range
*	@return 0 on error
*/
int parse_bin_val(char *tok, BIN *bin){
	char *pos = tok;
	int low_set = 0, high_set = 0;
	
	/* check for bracket or parenthesis */
	if(*pos == '['){
		++pos;
		bin->low_inc = 1;
	} else if (*pos == '('){
		++pos;
		bin->low_inc = 0;
	} else {
		bin->low_inc = 0;	/* default inclusive on lower bound */
	}
	
	/* consume whitespace */
	eat_white(&pos);
	
	/* check for ellipsis or for value */
	if(isdigit(*pos)){
		bin->low_val = strtod(pos, &pos);
		low_set = 1;
	} else if(*pos == '.' && pos[1] == '.'){ /* "everything less than the other value" */
		pos += 2;
		bin->low_val = -DBL_MAX;
		bin->low_inc = 0;
		low_set = 1;
	}
	
	eat_white(&pos);
	
	if(low_set && *pos == '.' && pos[1] == '.'){
		pos+=2;
	} else if (low_set && *pos == '-'){
		++pos;
	} else {
		gl_error("parse_bin failure");
		return 0;
	}
	
	eat_white(&pos);
	
	if(*pos == 0 || *pos == '\n' || *pos == ';'){
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

	if(parent == NULL) /* better have a group... */
	{
		OBJECT *group_obj = NULL;
		if(group[0] = 0){
			throw("Histogram has no parent and no group");
			return 0;
		}
		group_list = gl_find_objects(FL_GROUP,group);
		if(group_list == NULL){
			throw("Histogram group could not be parsed");
			return 0;
		}
		if(group_list->hit_count < 1){
			throw("Histogram group is an empty set");
			return 0;
		}
		/* non-empty set */
		while(group_obj = gl_find_next(group_list, group_obj)){
			if(gl_find_property(group_obj->oclass, property) == NULL){
				throw("Histogram group is unable to find prop '%s' in class '%d' for group '%s'", property, group_obj->oclass->name, group);
				return 0;
			}
		}
	} else { /* if we have a parent, we only focus on that one object */
		prop = gl_find_property(parent->oclass, property);
		if(prop == NULL){
			throw("Histogram parent '%s' of class '%s' does not contain property '%s'", parent->name ? parent->name : "(anon)", parent->oclass->name, property);
			return 0;
		}
	}
	/* generate bins */
	/* - min, max, bincount?*/
	if((bin_count > 0) && (min < max))
	{
		int i=0;
		double range = max - min;
		double step = range/bin_count;
		bin_list = (BIN *)malloc(sizeof(BIN) * bin_count);
		if(bin_list == NULL){
			throw("Histogram malloc error: unable to alloc %i * %i bytes for %s", bin_count, sizeof(BIN), obj->name ? obj->name : "(anon. histogram)");
			return 0;
		}
		memset(bin_list, 0, sizeof(BIN) * bin_count);
		for(i = 0; i < bin_count; i++){
			bin_list[i].low_val = min + i * step;
			bin_list[i].high_val = bin_list[i].low_val + step;
			bin_list[i].low_inc = 1;
			bin_list[i].high_inc = 0;
		}
		bin_list[i].high_inc = 1;	/* tail value capture */
	}
	else if (bins[0] != 0)
	{
		char *cptr = bins;
		char1024 bincpy;
		int i = 0;
		while(cptr != 0)
			if(*cptr == ';') ++bin_count;
		bin_list = (BIN *)malloc(sizeof(BIN) * bin_count);
		if(bin_list == NULL){
			throw("Histogram malloc error: unable to alloc %i * %i bytes for %s", bin_count, sizeof(BIN), obj->name ? obj->name : "(anon. histogram)");
			return 0;
		}
		memset(bin_list, 0, sizeof(BIN) * bin_count);
		memcpy(bincpy, bins, 1024);
		cptr = strtok(bincpy, ";\n");
		if(prop->ptype == PT_double || prop->ptype == PT_int16 || prop->ptype == PT_int32 || prop->ptype == PT_int64 || prop->ptype == PT_float || || prop->ptype == PT_real){
			for(i = 0; i < bin_count && cptr != NULL; ++i){
				if(parse_bin_val(cptr, bin_list+i) == 0){
					throw("Histogram unable to parse \'%s\' in %s", cptr, obj->name ? obj->name : "(unnamed histogram)");
				}
			}
		} else if (prop->ptype == PT_enum){
			for(i = 0; i < bin_count && cptr != NULL; ++i){
				if(parse_bin_enum(cptr, bin_list+i) == 0){
					throw("Histogram unable to parse \'%s\' in %s", cptr, obj->name ? obj->name : "(unnamed histogram)");
				}
			}
		} else if (prop->ptype == PT_set){
			;
		}

		if(i < bin_count){
			throw("Histrogram encountered a problem parsing bins for %s", obj->name ? obj->name : "(unnamed histogram)");
		}
	}
	return 1;
}

TIMESTAMP histogram::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	int i = 0;
	
	if(t1 >= next_sample){
		if(prop->ptype == PT_double || prop->ptype == PT_complex)
		{
			double value = 0.0;
			/* get value */
			for(i = 0; i < bin_count; ++i){
				if(value > bin_list[i].low_val && value < bin_list[i].high_val){
					++binctr[i];
				} else if(bin_list[i].low_inc && bin_list[i].low_val == value){
					++binctr[i];
				} else if(bin_list[i].high_inc && bin_list[i].high_val == value){
					++binctr[i];
				}
			}
		}
		else if(prop->ptype == PT_int16 || prop->ptype == PT_int32 || prop->ptype == PT_int64 || prop->ptype == PT_enum || prop->ptype == PT_set)
		{
			int value = 0;
			/* get value */
			for(i = 0; i < bin_count; ++i){
				if(value > bin_list[i].low_val && value < bin_list[i].high_val){
					++binctr[i];
				} else if(bin_list[i].low_inc && bin_list[i].low_val == value){
					++binctr[i];
				} else if(bin_list[i].high_inc && bin_list[i].high_val == value){
					++binctr[i];
				}
			}
		}
		next_sample += sampling_interval;
	}

	if(t1 >= next_count){
		char1024 line;
		char ts[64];
		int off, i;
		DATETIME dt;

		/* write the timestamp */
		gl_localtime(t1,&dt);
		gl_strtime(&dt,ts,64);
		
		/* write bins */
		for(i = 0; i < bin_count; ++i){
			off += sprintf(line, "%i", binctr[i]);
			if(i != bin_count){
				off += sprintf(line, ",");
			}
		}

		next_count += counting_interval;
	}
	return ( next_count < next_sample ? next_count : next_sample );
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
