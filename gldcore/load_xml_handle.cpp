/* gld_load.h
 * Copyright (C) 2008 Battelle Memorial Institute
 * Authors:
 *	Matthew Hauer <matthew.hauer@pnl.gov>, 6 Nov 07 -
 *
 * Versions:
 *	1.0 - MH - initial version
 *
 * Credits:
 *	adapted from SAX2Print.h
 *
 *	@file load_xml_handle.cpp
 *	@addtogroup load XML file loader
 *	@ingroup core
 *
 */

#include "platform.h"
#ifdef HAVE_XERCES
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include "load_xml_handle.h"

#define _CRT_SECURE_NO_DEPRECATE 1

gld_loadHndl::gld_loadHndl() : fFormatter("LATIN1", 0, this, XMLFormatter::NoEscapes, (const XMLFormatter::UnRepFlags) 0),
		fExpandNS ( false ){
	depth = 0;
	first = last = -1;
	stack_state = EMPTY;

	load_state = false;
	module_name[0] = 0;
	obj = NULL;
	stack_ptr = NULL;
	object_count = 0;
	class_count = 0;
}

gld_loadHndl::gld_loadHndl(const char* const encodingName, const XMLFormatter::UnRepFlags unRepFlags, const bool expandNamespaces)
	:	fFormatter(encodingName, 0, this, XMLFormatter::NoEscapes, unRepFlags),
		fExpandNS ( expandNamespaces ){
	depth = 0;
	first = last = -1;
	obj = NULL;
	stack_state = EMPTY;

	load_state = false;
	module_name[0] = 0;
	stack_ptr = NULL;
	object_count = 0;
	class_count = 0;
}

gld_loadHndl::~gld_loadHndl(){
	;
}

void gld_loadHndl::setDocumentLocator(const Locator *const loc){
	this->locator = loc;
}

void gld_loadHndl::writeChars(const XMLByte* const toWrite){
	;
}

#if XERCES_VERSION_MAJOR < 3
void gld_loadHndl::writeChars(const XMLByte* const toWrite, const unsigned int count, XMLFormatter* const formatter){
#else
void gld_loadHndl::writeChars(const XMLByte* const toWrite, const XMLSize_t count, XMLFormatter* const formatter){
#endif
	//printf("%s", (char *)toWrite);
	;
}

void gld_loadHndl::error(const SAXParseException& e){
	output_error("XML_Load: %ls(%i:char %i)\n Message: %ls", (char *)e.getSystemId(), e.getLineNumber(), e.getColumnNumber(), (char *)e.getMessage());
	load_state = false;
}

void gld_loadHndl::fatalError(const SAXParseException& e){
	output_error("XML_Load: %ls(%i:char %i)\n Message: %ls", (char *)e.getSystemId(), e.getLineNumber(), e.getColumnNumber(), (char *)e.getMessage());
	load_state = false;
}

void gld_loadHndl::warning(const SAXParseException& e){
	output_warning("XML_Load: %ls(%i:char %i)\n Message: %ls", (char *)e.getSystemId(), e.getLineNumber(), e.getColumnNumber(), (char *)e.getMessage());
	//load_state = false;
}


void gld_loadHndl::notationDecl(const  XMLCh* const name, const XMLCh* const publicId, const XMLCh* const systemId){
    ;
}

/** parse_property() breaks a string into its propname and propval before handing both to the module getvar/setvar methods.
*/
void gld_loadHndl::parse_property(char *buffer){
	;
}

char *gld_loadHndl::read_module_prop(char *buffer, size_t len){
	//printf("read_module_prop (%s)\n", buffer);
	if(strcmp(propname, "major") == 0){
		char temp[8];
		int major_in = atoi(buffer);
		int major = 0;
		module_getvar(this->module, "major", temp, 8);
		major = atoi(temp);
		if(major == 0)					//	don't care if major = 0
			return NULL;
		if(major == major_in){			//	current version, continue
			return NULL;
		} else if(major > major_in){	//	old version
			output_warning("The input file was built using an older major version of the module.");
			return NULL;
		} else if(major < major_in){	//	newer version
			output_warning("The file was built using a newer major version of the module loading it!");
			return NULL;
		}
	} else if(strcmp(propname, "minor") == 0){
		char temp[8];
		int minor_in = atoi(buffer);
		int minor = 0;
		int major = 0;
		module_getvar(this->module, "major", temp, 8);
		major = atoi(temp);
		module_getvar(this->module, "minor", temp, 8);
		minor = atoi(temp);
		if(major == 0)	//	don't care if major = 0
			return NULL;
		if(minor == minor_in){			//	current version, continue
			return NULL;
		} else if(minor > minor_in){	//	old version
			output_warning("XML::read_module_prop(): The input file was built using an older minor version of the module.");
		} else if(minor < minor_in){	//	new version
			output_warning("XML::read_module_prop(): The file was built using a newer minor version of the module loading it!");
		}
	} else {
		global_setvar(propname, buffer);
	}
	return NULL;
}

char *gld_loadHndl::read_global_prop(char *buffer, size_t len){
	return 0;
}

char *gld_loadHndl::read_clock_prop(char* buffer, size_t len){
	TIMESTAMP tsval = 0;
	if(strcmp(propname, "tick") == 0){
		;
	} else if(strcmp(propname, "timezone") == 0){
		if (timestamp_set_tz(buffer)==NULL){
			sprintf(errmsg, "timezone %s is not defined", timezone);
			return errmsg;
		}
	} else if(strcmp(propname, "timestamp") == 0){
		if(time_value_datetime(buffer, &tsval)){
			global_clock = tsval;
		} else if(time_value_datetimezone(buffer, &tsval)){
			global_clock = tsval;
		} else {
			sprintf(errmsg, "timestamp \"%s\" could not be resolved", buffer);
			return errmsg;
		}
	} else {
		sprintf(errmsg, "Unrecognized keyword in start_element_clock(%s)", buffer);
		return errmsg;
	}
	return NULL;
}

char *gld_loadHndl::read_object_prop(char *buffer, size_t len){
	//wchar_t wbuff[1024];
	//char tbuff[1024];
	char *rand_ptr = NULL;
	char *rand_mode_ptr = NULL;
	char *unit_ptr = NULL;
	double realval = 0.0;
	UNIT *unit=NULL;
	SAXParseException *e = NULL;
	void *addr = object_get_addr(obj, propname); /* get the & to set later */
	if(this->obj == NULL){
		sprintf(errmsg, "Null object pointer in read_object_prop(%s)", buffer);
		return errmsg;	//	no object
	}
	if(this->prop == NULL){
		if (strcmp(propname, "parent")==0){
			if (strcmp(propname, "root")==0){
				obj->parent = NULL;
			} else {
				add_unresolved(obj,PT_object,(void*)&obj->parent,oclass,buffer,"XML",42,UR_RANKS); 
			}
		} else if (strcmp(propname, "rank")==0){
			obj->rank = atoi(buffer);
		} else if (strcmp(propname, "clock")==0){
			obj->clock = atoi64(buffer);
		} else if (strcmp(propname, "latitude")==0){
			obj->latitude = load_latitude(buffer);
		} else if (strcmp(propname, "longitude")==0){
			obj->longitude = load_longitude(buffer);
		} else if (strcmp(propname, "in")==0){
			obj->in_svc = convert_to_timestamp(buffer);
		} else if (strcmp(propname, "out")==0){
			obj->out_svc = convert_to_timestamp(buffer);
		} else {
			sprintf(errmsg, "Null property pointer in read_object_prop(%s)", buffer);
			return errmsg;
		}
		return NULL;
	}
	//	determine property type
	switch(prop->ptype){
		case PT_double:
			//	scan for "random"
			if(strncmp("random.", buffer, 7) == 0){
				char temp[256];
				char *modep = NULL;
				double first = 0.0, second = 0.0;
				RANDOMTYPE rt;
				strncpy(temp, buffer, 256);
				modep = strtok(temp+7, "(");
				if(modep == NULL){
					//output_error("XML_Load: misformed random() value");
					load_state = false;
					sprintf(errmsg, "Misformed random() value in read_object_prop(%s)", buffer);
					return errmsg;
				}
				rt = random_type(modep);
				if(rt == 0){
					//output_message("XML_Load: '%s' ~ %s is not a valid random distribution", buffer, modep);
					load_state = false;
					sprintf(errmsg, "Invalid random distribution in read_object_prop(%s)", buffer);
					return errmsg;
				} else {
					first = atof(strtok(NULL, ","));
					second = atof(strtok(NULL, ")"));
					realval = random_value(rt, first, second);
				}
				if(strlen(strchr(buffer, ')')+1) > 0){ /* look for units */
					unit = unit_find(strchr(buffer, ')') + 2);
					if (unit!=NULL && prop->unit!=NULL && unit_convert_ex(unit,prop->unit,&realval)==0){
						sprintf(errmsg, "Cannot convert units from %s to %s in read_object_prop(%s)", unit->name,prop->unit->name, buffer);
						load_state = false;
						return errmsg;
					}
				}
			} else {
				unit_ptr = NULL;
				realval = strtod(buffer, &unit_ptr);
				if(unit_ptr != NULL){
					while(*unit_ptr == ' ') ++unit_ptr;
					unit = unit_find(unit_ptr);
					if(strlen(unit_ptr) > 0){
						if (unit!=NULL && prop->unit!=NULL && unit_convert_ex(unit,prop->unit,&realval)==0){
							sprintf(errmsg, "Cannot convert units from %s to %s in read_object_prop(%s)", unit->name,prop->unit->name, buffer);
							load_state = false;
							return errmsg;
						}
					}
				}
			}
			/* if((unit_ptr != NULL) && (*unit_ptr != '\0')){;} */
			if(object_set_double_by_name(obj, propname, realval) == 0){
				sprintf(errmsg, "Could not set \"%s\" to %f in read_object_prop(%s)", propname, realval, buffer);
				load_state = false;
				return errmsg;
			} else {
				return NULL; /* success */
			}
			break;
		case PT_object:
			if(add_unresolved(obj,PT_object,(void*)addr,oclass,buffer,"XML",42,UR_NONE) == NULL){
				sprintf(errmsg, "Failure with add_unresolved() in read_object_prop(%s)", buffer);
				return errmsg;
			}
			break;
		default:
			if(prop->ptype < _PT_LAST){	//	set value by name
				if (object_set_value_by_name(obj, propname, buffer)==0)	{
					//output_error("XML_Load: property %s of %s:%s could not be set to '%s'", propname, obj->oclass->name, obj->id, buffer);
					sprintf(errmsg, "Property %s of %s:%i could not be set to \"%s\" in read_object_prop()", propname, obj->oclass->name, obj->id, buffer);
					load_state = false;
					return errmsg;
				} else {
					;
				}
			} else {
				sprintf(errmsg, "Invalid property id = %i in read_object_prop(%s)", prop->ptype, buffer);
				return errmsg;
			}
	}
	return 0;
}

/**	characters() is where the raw text between the tags is handled.
*/
void gld_loadHndl::characters(const XMLCh* const chars, const unsigned int length){
    char buffer[1024];
	char tbuff[1024];
	wchar_t wbuff[1024];
	SAXParseException *e = NULL;
	char *unit_ptr = NULL;
	unsigned int i = 0;
	size_t len = 0;
	char *retval = NULL;	//	negative logic
	switch(stack_state){
		case MODULE_PROP:
		case GLOBAL_PROP:
		case OBJECT_PROP:
		case CLOCK_PROP:
			//	get prop
			if((len = wcslen((const wchar_t *)chars)) < 1){
				sprintf(tbuff, "Unable to get length of characters in characters()");
				mbstowcs(wbuff, tbuff, 1024);
				e = new SAXParseException((const XMLCh *)wbuff, *(this->locator));
				error(*e);
				delete e;
				return;
			}
			if(len != wcstombs(buffer, (const wchar_t *)chars, 1024)){
				sprintf(tbuff, "Unable to convert wcs to char in characters()");
				mbstowcs(wbuff, tbuff, 1024);
				e = new SAXParseException((const XMLCh *)wbuff, *(this->locator));
				error(*e);
				delete e;
				return;
			}
			break;
		default:
			return;	//	ignore whatever characters we read, we don't use them here.
	}
	if(len > 0){
		int j = 0;
		int i = 0;
		for(i = 0; i < (int)len; ++i){
			if(isspace(buffer[i])){
				++j;
			}
		}
		if(i == j){
			output_verbose("XML_Load: ignored: %i spaces.", length);
			retval = 0;
			return;
		}
	} else {
		output_verbose("XML_Load: ignored empty characters() call");
		retval = 0;
		return;
	}
	if(buffer[0] == '"' && buffer[1] == '"'){
		output_verbose("XML_Load: ignored empty doublequote characters() call");
		retval = 0;
		return;
	}
	switch(stack_state){
		case MODULE_PROP:
			retval = this->read_module_prop(buffer, len);
			break;
		case GLOBAL_PROP:
			retval = this->read_global_prop(buffer, len);
			break;
		case OBJECT_PROP:
			retval = this->read_object_prop(buffer, len);
			//	get prop
			break;
		case CLOCK_PROP:
			retval = this->read_clock_prop(buffer, len);
	}
	if(retval != NULL){
		stack_state = EMPTY;	//	stop processing
		output_error("Error reading the XML file");
		mbstowcs(wbuff, errmsg, 1024);
		e = new SAXParseException((const XMLCh *)wbuff, *(this->locator));
		error(*e);
		delete e;
		load_state = 0;
	}

}

/**	endDocument() is called when the end of the file is reached.  Any post-processing within the parser should occur here.
*/
void gld_loadHndl::endDocument(){
	OBJECT *tempobj = object_get_first();

	/* resolve_list(); */
	if(load_resolve_all() == FAILED){
		output_error("XML_Load: unable to resolve object linkings!");
		load_state = false;
	}
	/* "establish ranks" */

	for (; obj!=NULL; obj=obj->next)
		object_set_parent(obj,obj->parent);
	output_verbose("XML_Load: %d object%s loaded.", object_get_count(), object_get_count() > 0 ? "s" : "");
	if(object_count > 0)
		if(object_get_count() != this->object_count)
			output_warning("XML_Load: we expected %i objects instead!", this->object_count);
	output_verbose("XML_Load: %d class%s loaded.", class_get_count(), class_get_count() > 0 ? "es" : "");
	if(class_count > 0)
		if(class_get_count() != this->class_count)
			output_warning("XML_Load: we expected %i classes instead!", this->class_count);
}

char *gld_loadHndl::end_element_empty(char *buffer, size_t len){	//	why are we here?
	return NULL;
}

char *gld_loadHndl::end_element_load(char *buffer, size_t len){
	if(strcmp("load", buffer) == 0)
		stack_state = EMPTY;
	return NULL;
}

char *gld_loadHndl::end_element_module(char *buffer, size_t len){
	if(strcmp("module", buffer) == 0)
		stack_state = LOAD;
	return NULL;
}

char *gld_loadHndl::end_element_module_prop(char *buffer, size_t len){
	if(strcmp("properties", buffer) == 0){
		memset(propname, 0, len);
		stack_state = MODULE_STATE;
	}
	return NULL;
}

char *gld_loadHndl::end_element_object(char *buffer, size_t len){
	if(strcmp("object", buffer) == 0)
		stack_state = MODULE_STATE;
	return NULL;
}

char *gld_loadHndl::end_element_object_prop(char *buffer, size_t len){
	if(strcmp(propname, buffer) == 0){
		memset(propname, 0, len);
		stack_state = OBJECT_STATE;
	}
	return NULL;
}

char *gld_loadHndl::end_element_global(char *buffer, size_t len){
	if(strcmp("global", buffer) == 0)
		stack_state = LOAD;
	return NULL;
}

char *gld_loadHndl::end_element_global_prop(char *buffer, size_t len){
	if(strcmp(propname, buffer) == 0){
		memset(propname, 0, len);
		stack_state = GLOBAL_STATE;
	}
	return NULL;
}

char *gld_loadHndl::end_element_clock_prop(char *buffer, size_t len){
	if(strcmp(propname, buffer) == 0){
		memset(propname, 0, len);
		stack_state = CLOCK_STATE;
	}
	return NULL;
}

char *gld_loadHndl::end_element_clock(char *buffer, size_t len){
	if(strcmp("clock", buffer) == 0)
		stack_state = LOAD;
	return NULL;
}

/**	endElement() is called for every tag, and should zero whatever level of the stack corresponds to the tag.
*/
void gld_loadHndl::endElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname){
	char buffer[128];
	int i = 0;
	size_t len = 0;
	gldStack *temp_stack = NULL;
	char *retval = NULL;	// negative logic
	
	--depth;

	len = wcslen((const wchar_t *)qname);
	wcstombs(buffer, (const wchar_t *)qname, 128);
	
	switch(stack_state){
		case EMPTY:
			retval = end_element_empty(buffer, len);
			break;
		case LOAD:
			retval = end_element_load(buffer, len);
			break;
		case MODULE_STATE:
			retval = end_element_module(buffer, len);
			break;
		case MODULE_PROP:
			retval = end_element_module_prop(buffer, len);
			break;
		case OBJECT_STATE:
			retval = end_element_object(buffer, len);
			break;
		case OBJECT_PROP:
			retval = end_element_object_prop(buffer, len);
			break;
		case GLOBAL_STATE:
			retval = end_element_global(buffer, len);
			break;
		case GLOBAL_PROP:
			retval = end_element_global_prop(buffer, len);
			break;
		case CLOCK_STATE:
			retval = end_element_clock(buffer, len);
			break;
		case CLOCK_PROP:
			retval = end_element_clock_prop(buffer, len);
			break;
	}
}


void gld_loadHndl::ignorableWhitespace(const XMLCh* const chars, const unsigned int length){
	output_verbose("XML_Load: ignored: %i spaces.", length);
}


void gld_loadHndl::processingInstruction(const  XMLCh* const target, const XMLCh* const data){
	/* put GLD macros in here? -mh */
	return;
}

/**	startDocument() is the
*/
void gld_loadHndl::startDocument(){
	depth = 0;
	module_name[0] = 0;
	load_state = true;
}

char *gld_loadHndl::start_element_empty(char *buffer, size_t len, const Attributes& attributes){
	if(strcmp(buffer, "load") == 0){
		stack_state = LOAD;
	} // cut some slack, don't break here.
	return NULL;
}

char *gld_loadHndl::start_element_load(char *buffer, size_t len, const Attributes& attributes){
	static wchar_t wcs_type[5], wcs_minor[6], wcs_major[6];
	static size_t wcs_type_len = mbstowcs(wcs_type, "type", 5);
	static size_t wcs_major_len = mbstowcs(wcs_major, "major", 6);
	static size_t wcs_minor_len = mbstowcs(wcs_minor, "minor", 6);
	XMLCh *_major = NULL, *_minor = NULL;
	int32 major = 0, minor = 0;
	if(strcmp(buffer, "global") == 0){
		stack_state = GLOBAL_STATE;
	} else if(strcmp(buffer, "module") == 0){	//	LOAD THE MODULE
		wcstombs(module_name, (const wchar_t *)attributes.getValue((const XMLCh *)wcs_type), 1024);
		_major = (XMLCh *)attributes.getValue((const XMLCh *)wcs_major);
		_minor = (XMLCh *)attributes.getValue((const XMLCh *)wcs_minor);
		if(_major != NULL)
			major = wcstol((const wchar_t *)_major, NULL, 10);
		if(_minor != NULL)
			minor = wcstol((const wchar_t *)_minor, NULL, 10);
		
		if((module = module_load(module_name, 0, NULL)) == NULL){
			sprintf(errmsg, "Unable to load module \"%s\" in start_element_load()", module_name);
			return errmsg;
		} else {
			strcpy(module_name, buffer);
			stack_state = MODULE_STATE;
			if(major != 0){
				char temp[8];
				module_getvar(this->module, "major", temp, 8);
				int major_in = atoi(temp);
				module_getvar(this->module, "minor", temp, 8);
				int minor_in = atoi(temp);
				if(major < major_in){
					output_warning("XML::read_module_prop(): The input file was built using an older major version of the module.");
					return NULL; /* avoid minor var complaints */
				}
				if(minor == minor_in){			//	current version, continue
					return NULL;
				} else if(minor > minor_in){	//	old version
					output_warning("XML::read_module_prop(): The input file was built using an older minor version of the module.");
				} else if(minor < minor_in){	//	new version
					output_warning("XML::read_module_prop(): The file was built using a newer minor version of the module loading it!");
				}
			}
		}
	} else if(strcmp(buffer, "clock") == 0){
		stack_state = CLOCK_STATE;
	} else {
		sprintf(errmsg, "Unrecognized keyword in start_element_load(%s)", buffer);
		return errmsg;
	}
	return NULL;
}

char *gld_loadHndl::start_element_module_build_object(const Attributes &attributes){
	static wchar_t str_id[3],
			str_name[5],
			str_type[5];
	static size_t str_id_len = mbstowcs(str_id, "id", 3),
			str_name_len = mbstowcs(str_name, "name", 5),
			str_type_len = mbstowcs(str_type, "type", 5);
	char object_type[64],
		object_id[64],
		object_name[64];
	char *temp = NULL;
	char *retval = NULL;
	int first = -1,
		last = -1;
	if(attributes.getIndex((const XMLCh *) str_type) < 0){
		sprintf(errmsg, "object tag without a type in start_element_module_build_object()");
		return errmsg;
	} else {
		wcstombs(object_type, (const wchar_t *)attributes.getValue((const XMLCh *) str_type), 64);
		oclass = class_get_class_from_classname_in_module(object_type, module);
		if(oclass == NULL){
			sprintf(errmsg, "Class \"%s\" for module \"%s\"is not recognized in start_element_module_build_object()", object_type, module->name);
			return errmsg;
		} else {
			; /* recognized class type */
		}
	}
	if(attributes.getIndex((const XMLCh *) str_id) < 0){
		/* anonymous objects are legit - mh */
		last = first = -1;
	} else {
		wcstombs(object_id, (wchar_t *)attributes.getValue((const XMLCh *) str_id), 64);
		temp = strchr(object_id, '.');
		if(temp == NULL){ /* no "..", just a number*/
			first = strtol(object_id, NULL, 10);
			if(first < 0){
				sprintf(errmsg, "Invalid object id of %i in start_element_module_build_object()", first);
				return errmsg;
			} else {
				last = first;
			}
		} else {
			if(temp[0] == temp[1]){
				if(temp == object_id){ /* "..x", count */
					last = strtol(object_id+2, NULL, 10);
					if(last < 1){
						sprintf(errmsg, "Invalid object id of %i in start_element_module_build_object()", first);
						return errmsg;
					} else {
						first = -1;
					}
				} else { /* "x..y", range */
					*temp = 0;
					first = strtol(object_id, NULL, 10);
					last = strtol(temp+2, NULL, 10);
					*temp = '.';
					if(first < 0){
						output_error("XML_Load: first ID < 0 !");
						sprintf(errmsg, "Invalid object id of %i in start_element_module_build_object()", first);
						return errmsg;
					}
					if(last < 1){
						output_error("XML_Load: last ID < 1 !");
						sprintf(errmsg, "Invalid object id of %i in start_element_module_build_object()", first);
						return errmsg;
					}
					if(first >= last){
						output_error("XML_Load: first id >= last id!");
						sprintf(errmsg, "Invalid object id of %i in start_element_module_build_object()", first);
						return errmsg;
					}
				}
			} else {
				sprintf(errmsg, "Invalid ID format in start_element_module_build_object()");
				return errmsg;
			}
		}
	}
	if((retval = build_object_vect(first, last)) != 0){ /* unable to call constructors */
		sprintf(errmsg, "Unable to create objects in start_element_module_build_object()");
		return errmsg;
	} else {
		;
	}
	if(attributes.getIndex((const XMLCh *) str_name) < 0){
		strcpy(object_name, "(none)");
	} else {
		wcstombs(object_name, (wchar_t *)attributes.getValue((const XMLCh *) str_name), 64);
		object_set_name(obj, object_name);
	}
	//printf("object: type = %s, id = %s, name = %s\n", object_type, object_id, object_name);
	stack_state = OBJECT_STATE;
	return NULL;
}

char *gld_loadHndl::start_element_module(char *buffer, size_t len, const Attributes& attributes){
	//	check generic props
	if(0 == strcmp(buffer, "object")){
		strcpy(propname, "object");
		return start_element_module_build_object(attributes);
	}/* else if(0 == strcmp(buffer, "major")){
		strcpy(propname, "major");
		stack_state = MODULE_PROP;
	} else if(0 == strcmp(buffer, "minor")){
		strcpy(propname, "minor");
		stack_state = MODULE_PROP;
	} */else if(0 == strcmp(buffer, "properties")){	//	check for module props ~ note plural tag
		strcpy(propname, buffer);
		stack_state = MODULE_PROP;
	} else {
		sprintf(errmsg, "Unrecognized keyword in start_element_module(%s)", buffer);
		return errmsg;
	}
	return NULL;
}

char *gld_loadHndl::start_element_module_prop(char *buffer, size_t len, const Attributes& attributes){
	char pname[323];
	sprintf(pname, "%s::%s", this->module->name, buffer);
	if(global_find(pname)){
		strcpy(propname, pname);
	} else {
		output_warning("XML: start_element_module_prop: property \"\" not found, initializing");
		strcpy(propname, pname);
	}
	return NULL;
}

char *gld_loadHndl::start_element_object(char *buffer, size_t len, const Attributes& attributes){
	//	check generic props
	if(0 == strcmp(buffer, "parent")){
		strcpy(propname, "parent");
		stack_state = OBJECT_PROP;
	} else if(0 == strcmp(buffer, "root")){
		strcpy(propname, "root");
		stack_state = OBJECT_PROP;
	} else if(0 == strcmp(buffer, "rank")){
		strcpy(propname, "rank");
		stack_state = OBJECT_PROP;
	} else if(0 == strcmp(buffer, "clock")){
		strcpy(propname, "clock");
		stack_state = OBJECT_PROP;
	} else if(0 == strcmp(buffer, "latitude")){
		strcpy(propname, "latitude");
		stack_state = OBJECT_PROP;
	} else if(0 == strcmp(buffer, "longitude")){
		strcpy(propname, "longitude");
		stack_state = OBJECT_PROP;
	} else if(0 == strcmp(buffer, "in")){
		strcpy(propname, "in");
		stack_state = OBJECT_PROP;
	} else if(0 == strcmp(buffer, "out")){
		strcpy(propname, "out");
		stack_state = OBJECT_PROP;
	} else if(0 == strcmp(buffer, "library")){
		strcpy(propname, "library");	//	uhm?  an object library?
		stack_state = OBJECT_PROP;
	}	//	check specific props
	else if((prop = class_find_property(oclass, buffer)) != NULL){
		strcpy(propname, buffer);
		stack_state = OBJECT_PROP;
	} else {
		sprintf(errmsg, "Unrecognized property in start_element_object(%s)", buffer);
		return errmsg;
	}
	prop = class_find_property(obj->oclass, buffer);	//	prop = NULL is valid for hardcoded properties. -MH
	return NULL;
}

char *gld_loadHndl::start_element_object_prop(char *buffer, size_t len, const Attributes& attributes){
	//	we shouldn't be here.
	return NULL;
}

char *gld_loadHndl::start_element_global(char *buffer, size_t len, const Attributes& attributes){
	//	check generic props
	if(0 == strcmp(buffer, "version_major")){
		strcpy(propname, "version_major");
		stack_state = GLOBAL_PROP;
	} else if(0 == strcmp(buffer, "version_minor")){
		strcpy(propname, "version_minor");
		stack_state = GLOBAL_PROP;
	} else if(0 == strcmp(buffer, "savefilename")){
		strcpy(propname, "savefilename");
		stack_state = GLOBAL_PROP;
	} else if(0 == strcmp(buffer, "class_count")){
		strcpy(propname, "class_count");
		stack_state = GLOBAL_PROP;
	} else if(0 == strcmp(buffer, "object_count")){
		strcpy(propname, "object_count");
		stack_state = GLOBAL_PROP;
	} else if(0 == strcmp(buffer, "property")){	//	check specific props
		strcpy(propname, "property");
		stack_state = GLOBAL_PROP;
	}
	//	anything else, we don't care about.
	return NULL;
}

char *gld_loadHndl::start_element_global_prop(char *buffer, size_t len, const Attributes& attributes){
	//	we may not supposed to be here?
	return NULL;
}

char *gld_loadHndl::start_element_clock(char *buffer, size_t len, const Attributes& attributes){
	if(strcmp(buffer, "tick") == 0){
		stack_state = CLOCK_PROP;
		strcpy(propname, "tick");
	} else if(strcmp(buffer, "timezone") == 0){
		stack_state = CLOCK_PROP;
		strcpy(propname, "timezone");
	} else if(strcmp(buffer, "timestamp") == 0){
		stack_state = CLOCK_PROP;
		strcpy(propname, "timestamp");
	} else if(strcmp(buffer, "starttime") == 0){
		stack_state = CLOCK_PROP;
		strcpy(propname, "timestamp");
	} else if(strcmp(buffer, "stoptime") == 0){
		stack_state = CLOCK_PROP;
		strcpy(propname, "stoptime");
	} else {	//	bad keyword
		sprintf(errmsg, "Unrecognized keyword in start_element_clock(%s)", buffer);
		return errmsg;
	}
	return NULL;
}

char *gld_loadHndl::start_element_clock_prop(char *buffer, size_t len, const Attributes& attributes){
	return NULL;
}

#define ADD_KEYWORD(A) 	stack_ptr = new gldStack(); \
						strcpy(stack_ptr->object_type, "module"); \
						strcpy(stack_ptr->keyword, (A)); \

void gld_loadHndl::startElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname,
								const Attributes& attributes){
	char buffer[128];
	char *temp = NULL;
	unsigned int i = 0;
	size_t len = 0;
	char *retval = 0;	//	negative logic
	gldStack *currStack = NULL;
	wchar_t str_id[3],	/* = "id", */
			str_name[5],/* = "name", */
			str_type[5];/* = "type"; */

	mbstowcs(str_id, "id", 3);
	mbstowcs(str_name, "name", 5);
	mbstowcs(str_type, "type", 5);

	if((len = wcslen((const wchar_t *)qname)) < 1){
		output_error("startElement: Unable to parse element tag length!");
		load_state = 0;
		return;
	}
	if(wcstombs(buffer, (const wchar_t *)qname, 128) < 1){
		output_error("startElement: Unable to convert element tag name from wchar to char!");
		load_state = 0;
		return;
	}
	switch(this->stack_state){
		case EMPTY:
			retval = start_element_empty(buffer, len, attributes);
			break;
		case LOAD:
			retval = start_element_load(buffer, len, attributes);
			break;
		case MODULE_STATE:
			retval = start_element_module(buffer, len, attributes);
			break;
		case MODULE_PROP:
			retval = start_element_module_prop(buffer, len, attributes);
			break;
		case OBJECT_STATE:
			retval = start_element_object(buffer, len, attributes);
			break;
		case OBJECT_PROP:
			retval = start_element_object_prop(buffer, len, attributes);
			break;
		case GLOBAL_STATE:
			retval = start_element_global(buffer, len, attributes);
			break;
		case GLOBAL_PROP:
			retval = start_element_global_prop(buffer, len, attributes);
			break;
		case CLOCK_STATE:
			retval = start_element_clock(buffer, len, attributes);
			break;
		case CLOCK_PROP:
			retval = start_element_clock_prop(buffer, len, attributes);
			break;
	}
	if(retval != NULL){
		char tbuff[256];
		wchar_t wbuff[256];
		SAXParseException *e = NULL;
		sprintf(tbuff, "Error in start_element with tag \"%s\": %s", buffer, retval);
		mbstowcs(wbuff, tbuff, 1024);
		e = new SAXParseException((const XMLCh *)wbuff, *(this->locator));
		error(*e);
		delete e;
	}
}

#undef ADD_KEYWORD

/** Stuffs a number of new objects into obj_vect based on our inputs.
	@param start first index for the objects, -1 if anonymous
	@param end last index for the objects, if -1, assume 1 object
	@return positive logic ~ 1 success, 0 failure
*/
char *gld_loadHndl::build_object_vect(int start, int end){
	int count = 0, i = 0;
	obj_vect.clear();
	if(start == end){
		if((*oclass->create)(&obj, NULL) == 0){
			//output_error("XML_Load: Unable to create a lone object with ID = %i.", start);
			sprintf(errmsg, "Unable to create a lone object with ID = %i in build_object_vect(%i, %i)", start, start, end);
			load_state = false;
			return errmsg;
		}
		if(start != -1){
			if(load_set_index(obj, (OBJECTNUM)end) == 0){
				sprintf(errmsg, "Unable to index an item in build_object_vect(%i, %i)", start, end);
				load_state = false;
				return errmsg;
			} else {
				; /* success*/
			}
		}
		/* successfully created object */
		return NULL;
	}
	if(start == -1){ /* create unindexed objects */
		count = end;
	} else {
		if(last > first){
			count = last - first + 1;
		} else {
			sprintf(errmsg, "last < first, aborting build_object_vect(%i, %i)", start, end);
			load_state = false;
			return errmsg;
		}
	}
	obj_vect.reserve(count);
	for(i = (start == -1) ? 0 : start; i <= last; ++i){ /* "if start == -1, use 0, else use start" */
		if((*oclass->create)((obj_vect[i]), NULL) != 0){
			if(start != -1){
				if(load_set_index(obj_vect[i], (OBJECTNUM)i) == 0){
					sprintf(errmsg, "Unable to index a batch item in build_object_vect(%i, %i)", start, end);
					load_state = false;
					// cleanup all items?
					return errmsg;
				} 
			}
		} else { /* failed */
			sprintf(errmsg, "Unable to create an object in build_object_vect(%i, %i)", start, end);
			load_state = false;
			/* cleanup existing objects, if any */
			return errmsg;
		}
	}
	return NULL; /* positive logic */
}

#undef _CRT_SECURE_NO_DEPRECATE

#endif HAVE_XERCES
/* EOF */
