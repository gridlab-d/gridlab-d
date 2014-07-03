/** $Id: multi_recorder.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file tape.c
	@addtogroup recorder Recorders
	@ingroup tapes

	Tape recorders use the following additional properties
	- \p format specifies whether to use raw timestamp instead of date-time format
	- \p interval specifies the sampling interval to use (0 means every pass, -1 means only transients)
	- \p limit specifies the maximum length limit for the number of samples taken
	- \p trigger specifies a trigger condition on a property to start recording
	the condition is specified in the format \p property \p comparison \p value
	- \p The \p loop property is not available in recording.
 @{
 **/

 /*
  *	The idea behind the multi-recorder is that it is easier to reuse the recorder structure with logic
  *	that anticipates the use of a different mechanism for determining the object to associate with each
  *	property listed for the recorder.
  *	The differences assumed by the multi-recorder are that the parent is not used to determine the object
  *	that each property is associated with, or that the recorder is associated with, that the property
  *	list will include the name of the object that the property will correspond to (and corresponds with
  *	the parent object if no object is parsed from the string), and that the parent property may be
  * null.  The multi-recorder will also use the RECORDER_MAP struct in order to store the target
  *	properties, rather than the PROPERTY struct.
  *		-Matt Hauer <d3p988>
  */
 
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include "gridlabd.h"
#include "object.h"
#include "aggregate.h"

#include "tape.h"
#include "file.h"
#include "odbc.h"

#ifndef WIN32
#define strtok_s strtok_r
#endif

CLASS *multi_recorder_class = NULL;
static OBJECT *last_recorder = NULL;

EXPORT int create_multi_recorder(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(multi_recorder_class);
	if (*obj!=NULL)
	{
		struct recorder *my = OBJECTDATA(*obj,struct recorder);
		last_recorder = *obj;
		gl_set_parent(*obj,parent);
		strcpy(my->file,"");
		strcpy(my->multifile,"");
		strcpy(my->filetype,"txt");
		strcpy(my->delim,",");
		strcpy(my->mode, "file");
		strcpy(my->property,"(undefined)");
		my->interval = -1; /* transients only */
		my->dInterval = -1.0;
		my->last.ts = -1;
		strcpy(my->last.value,"");
		my->limit = 0;
		my->samples = 0;
		my->status = TS_INIT;
		my->trigger[0]='\0';
		my->format = 0;
		strcpy(my->plotcommands,"");
		my->target = gl_get_property(*obj,my->property,NULL);
		my->header_units = HU_DEFAULT;
		my->line_units = LU_DEFAULT;
		return 1;
	}
	return 0;
}

static int multi_recorder_open(OBJECT *obj)
{
	char32 type="file";
	char1024 fname="";
	char32 flags="w";
	struct recorder *my = OBJECTDATA(obj,struct recorder);
	TAPEFUNCS *tf = 0;

	my->interval = (int64)(my->dInterval/TS_SECOND);
	/* if prefix is omitted (no colons found) */
	if (sscanf(my->file,"%32[^:]:%1024[^:]:%[^:]",type,fname,flags)==1)
	{
		/* filename is file by default */
		strcpy(fname,my->file);
		strcpy(type,"file");
	}

	/* if no filename given */
	if (strcmp(fname,"")==0)

		/* use object name-id as default file name */
		sprintf(fname,"%s-%d.%s",obj->parent->oclass->name,obj->parent->id, my->filetype);

	/* open multiple-run input file & temp output file */
	if(my->type == FT_FILE && my->multifile[0] != 0){
		if(my->interval < 1){
			gl_error("multirecorder: transient recorders cannot use multi-run output files");
			return 0;
		}
		sprintf(my->multitempfile, "temp_%s", my->file);
		my->multifp = fopen(my->multitempfile, "w");
		if(my->multifp == NULL){
			gl_error("multirecorder: unable to open \'%s\' for multi-run output", my->multitempfile);
		} else {
			time_t now=time(NULL);

			my->inputfp = fopen(my->multifile, "r");

			// write header into temp file
			fprintf(my->multifp,"# file...... %s\n", my->file);
			fprintf(my->multifp,"# date...... %s", asctime(localtime(&now)));
#ifdef WIN32
			fprintf(my->multifp,"# user...... %s\n", getenv("USERNAME"));
			fprintf(my->multifp,"# host...... %s\n", getenv("MACHINENAME"));
#else
			fprintf(my->multifp,"# user...... %s\n", getenv("USER"));
			fprintf(my->multifp,"# host...... %s\n", getenv("HOST"));
#endif
			if(obj->parent){
				fprintf(my->multifp,"# target.... %s %d\n", obj->parent->oclass->name, obj->parent->id);
			} else {
				fprintf(my->multifp,"# target.... (none)\n");
			}
			fprintf(my->multifp,"# trigger... %s\n", my->trigger[0]=='\0'?"(none)":my->trigger);
			fprintf(my->multifp,"# interval.. %d\n", my->interval);
			fprintf(my->multifp,"# limit..... %d\n", my->limit);
			fprintf(my->multifp,"# property.. %s\n", my->property);
			//fprintf(my->multifp,"# timestamp,%s\n", my->property);
		}
		if(my->inputfp != NULL){
			char1024 inbuffer;
			char *data;
			int get_col = 0;
			do{
				if(0 != fgets(inbuffer, 1024, my->inputfp)){
					char *end = strchr(inbuffer, '\n');
					data = inbuffer+strlen("# file...... ");
					if(end != 0){
						*end = 0; // trim the trailing newline
					}
					//					   "# columns... "
					if(strncmp(inbuffer, "# file", strlen("# file")) == 0){
						; // ignore
					} else if(strncmp(inbuffer, "# date", strlen("# date")) == 0){
						; // ignore
					} else if(strncmp(inbuffer, "# user", strlen("# user")) == 0){
						; // ignore
					} else if(strncmp(inbuffer, "# host", strlen("# host")) == 0){
						; // ignore
					} else if(strncmp(inbuffer, "# target", strlen("# target")) == 0){
						// verify same target
						char256 target;
						if(obj->parent != 0){
							sprintf(target, "%s %d", obj->parent->oclass->name, obj->parent->id);
							if(0 != strncmp(target, data, strlen(data))){
								gl_error("multirecorder:%i: re-recording target mismatch: was %s, now %s", obj->id, data, target);
							}
						} else {
							if(strncmp("(none)", target, 6)){
								gl_error("multirecorder:%i: re-recording target mismatch: was %s, now blank", obj->id, data);
							}
						}
					} else if(strncmp(inbuffer, "# trigger", strlen("# trigger")) == 0){
						// verify same trigger, or absence thereof
						;
					} else if(strncmp(inbuffer, "# interval", strlen("# interval")) == 0){
						// verify same interval
						int interval = atoi(data);
						if(interval != my->interval){
							gl_error("multirecorder:%i: re-recording interval mismatch: was %i, now %i", obj->id, interval, my->interval);
						}
					} else if(strncmp(inbuffer, "# limit", strlen("# limit")) == 0){
						// verify same limit
						int limit = atoi(data);
						if(limit != my->limit){
							gl_error("multirecorder:%i: re-recording limit mismatch: was %i, now %i", obj->id, limit, my->limit);
						}
					} else if(strncmp(inbuffer, "# property", strlen("# property")) == 0){
						// verify same columns
						if(0 != strncmp(my->property, data, strlen(my->property))){
							gl_error("multirecorder:%i: re-recording property mismatch: was %s, now %s", obj->id, data, my->property);
						}
						// next line is full header column list
						get_col = 1;
					}
				} else {
					gl_error("multirecorder: error reading multi-read input file \'%s\'", my->multifile);
					break;
				}
			} while(inbuffer[0] == '#' && get_col == 0);
			// get full column list
			if(0 != fgets(inbuffer, 1024, my->inputfp)){
				int rep=0;
				int replen = (int)strlen("# repetition");
				int len, lenmax = 1024, i = 0;
				char1024 propstr, shortstr;
				PROPERTY *tprop = my->target;
				gl_verbose("read last buffer");
				if(strncmp(inbuffer, "# repetition", replen) == 0){
					char *trim;
					rep = atoi(inbuffer + replen + 1); // skip intermediate space
					++rep;
					fprintf(my->multifp, "# repetition %i\n", rep);
					fgets(inbuffer, 1024, my->inputfp);
					trim = strchr(inbuffer, '\n');
					if(trim) *trim = 0;
				} else { // invalid input file or somesuch, we could error ... or we can trample onwards with our output file.
					rep = 0;
					fprintf(my->multifp, "# repetition %i\n", rep);
				}
				// following block matches below
				while(tprop != NULL){
					sprintf(shortstr, ",%s(%i)", tprop->name, rep);
					len = (int)strlen(shortstr);
					if(len > lenmax){
						gl_error("multirecorder: multi-run recorder output full property list is larger than the buffer, please start a new file!");
						break; // will still print everything up to this one
					}
					strncpy(propstr+i, shortstr, len+1);
					i += len;
					tprop = tprop->next;
				}
				fprintf(my->multifp, "%s%s\n", inbuffer, propstr);
			}
		} else { /* new file, so write repetition & properties with (0) */
			char1024 propstr, shortstr;
/*							*
 *		UPDATE THIS PART	*
 *							*/
			int len, lenmax = 1024, i = 0;
			PROPERTY *tprop = my->target;
			fprintf(my->multifp, "# repetition 0\n");
			// no string from previous runs to append new props to
			sprintf(propstr, "# timestamp");
			len = (int)strlen(propstr);
			lenmax-=len;
			i = len;
			// following block matches above
			while(tprop != NULL){
				sprintf(shortstr, ",%s(0)", tprop->name);
				len = (int)strlen(shortstr);
				if(len > lenmax){
					gl_error("multirecorder: multi-run recorder output full property list is larger than the buffer, please start a new file!");
					break; // will still print everything up to this one
				}
				strncpy(propstr+i, shortstr, len+1);
				i += len;
				tprop = tprop->next;
			}
			fprintf(my->multifp, "%s\n", propstr);
/*							*
 *		End update part		*
 *							*/
		}
	}

	/* if type is file or file is stdin */
	tf = get_ftable(type);
	if(tf == NULL)
		return 0;
	my->ops = tf->recorder;
	if(my->ops == NULL)
		return 0;
	set_csv_options();

	// set out_property here
	{size_t offset = 0;
		char unit_buffer[1024];
		char *token = 0, *prop_ptr = 0, *unit_ptr = 0, *obj_ptr = 0;
		char objstr[1024], bigpropstr[1024], propstr[1024], unitstr[64];
		PROPERTY *prop = 0;
		UNIT *unit = 0;
		int first = 1;
		OBJECT *myobj = 0;
		switch(my->header_units){
			case HU_DEFAULT:
				strcpy(my->out_property, my->property);
				break;
			case HU_ALL:
				strcpy(unit_buffer, my->property);
				for(token = strtok(unit_buffer, ","); token != NULL; token = strtok(NULL, ",")){
					unit = 0;
					prop = 0;
					unitstr[0] = 0;
					propstr[0] = 0;
					objstr[0] = 0;
					prop_ptr = strchr(token, ':');
					if(prop_ptr != 0){
						unit_ptr = strchr(prop_ptr, '[');
					} else {
						unit_ptr = strchr(token, '[');
					}
					// detect if this points at a different object and split accordingly
					if(prop_ptr == 0){
						prop_ptr = token;
						myobj = obj->parent;
						strcpy(bigpropstr, token);
					} else {
						sscanf(token, "%[^:]:%[^\n\r\t;]", objstr, bigpropstr);
						myobj = gl_get_object(objstr);
						if(myobj == 0){
							gl_error("multi_recorder:%d: unable to find object '%s'", obj->id, objstr);
							return 0;
						}
					}
					// split unit from property, if present
					if(unit_ptr == 0){
						// no explicit unit
						prop = gl_get_property(myobj, bigpropstr,NULL);
						if(prop == 0){
							gl_error("multi_recorder:%d: unable to find property '%s' for object '%s'", obj->id, propstr, myobj->name);
							return 0;
						}

						if(prop->ptype == PT_double){
							strcpy(unitstr, prop->unit->name);
						} else {
							; // not double, no unit, nothing to do
						}
						strcpy(propstr, bigpropstr);
					} else {
						// has explicit unit
						if(2 == sscanf(bigpropstr, "%[A-Za-z0-9_.][%[^]\n,\0]", propstr, unitstr)){
							unit = gl_find_unit(unitstr);
							if(unit == 0){
								gl_error("multi_recorder:%d: unable to find unit '%s' for property '%s'", obj->id, unitstr, propstr);
								return 0;
							}
							prop = gl_get_property(myobj, propstr,NULL);
							if(prop == 0){
								gl_error("multi_recorder:%d: unable to find property '%s' for object '%s'", obj->id, propstr, myobj->name);
								return 0;
							}
						} else {
							gl_error("oops");
						}
					}
					// check if property exists in object
					// find property

/*					// breakpoint
					if(3 == sscanf(token, "%[A-Za-z0-9_.][%[^]\n,\0]:%[A-Za-z0-9_.][%[^]\n,\0]", objstr, propstr, unitstr)){
						myobj = gl_get_object(objstr);
					} else if(2 == sscanf(token, "%[A-Za-z0-9_.][%[^]\n,\0]", propstr, unitstr)){
						unit = gl_find_unit(unitstr);
						if(unit == 0){
							gl_error("multi_recorder:%d: unable to find unit '%s' for property '%s'", obj->id, unitstr, propstr);
							return 0;
						}
					}
					prop = gl_get_property(obj->parent, propstr);
					if(prop->unit != 0 && unit == 0){
						unit = prop->unit;
					}*/
					// print the property, and if there is one, the unit
					if(myobj != obj->parent){
						// need to include target object name in string
						if(unit != 0){
							sprintf(my->out_property+offset, "%s%s:%s[%s]", (first ? "" : ","), myobj->name, propstr, (unitstr[0] ? unitstr : unit->name));
							offset += strlen(propstr) + (first ? 0 : 1) + 2 + strlen(unitstr[0] ? unitstr : unit->name) + strlen(myobj->name) + 1;
						} else {
							sprintf(my->out_property+offset, "%s%s:%s", (first ? "" : ","), myobj->name, propstr);
							offset += strlen(propstr) + (first ? 0 : 1 + strlen(myobj->name) + 1);
						}
					} else {
						// parent object, so no explicit object name
						if(unit != 0){
							sprintf(my->out_property+offset, "%s%s[%s]", (first ? "" : ","), propstr, (unitstr[0] ? unitstr : unit->name));
							offset += strlen(propstr) + (first ? 0 : 1) + 2 + strlen(unitstr[0] ? unitstr : unit->name);
						} else {
							sprintf(my->out_property+offset, "%s%s", (first ? "" : ","), propstr);
							offset += strlen(propstr) + (first ? 0 : 1);
						}
					}
					first = 0;
				}
				break;
			case HU_NONE:
				strcpy(unit_buffer, my->property);
				for(token = strtok(unit_buffer, ","); token != NULL; token = strtok(NULL, ",")){
					if(2 == sscanf(token, "%[A-Za-z0-9_:.][%[^]\n,\0]", propstr, unitstr)){
						; // no logic change
					}
					// print just the property, regardless of type or explicitly declared property
					sprintf(my->out_property+offset, "%s%s", (first ? "" : ","), propstr);
					offset += strlen(propstr) + (first ? 0 : 1);
					first = 0;
				}
				break;
			default:
				// error
				break;
		}
	}
	return my->ops->open(my, fname, flags);
}

static int write_multi_recorder(struct recorder *my, char *ts, char *value)
{
	return my->ops->write(my, ts, value);
}

static void close_multi_recorder(struct recorder *my)
{
	if (my->ops){
		my->ops->close(my);
	}
	if(my->multifp){
		if(0 != fclose(my->multifp)){
			gl_error("multirecorder: unable to close multi-run temp file \'%s\'", my->multitempfile);
			perror("fclose(): ");
		}

		my->multifp = 0; // since it's closed

		if(my->inputfp != NULL){
			fclose(my->inputfp);
			if(0 != remove(my->multifile)){ // old file
				gl_error("multirecorder: unable to remove out-of-data multi-run file \'%s\'", my->multifile);
				perror("remove(): ");
			}
		}
		if(0 != rename(my->multitempfile, my->multifile)){
			gl_error("multirecorder: unable to rename multi-run file \'%s\' to \'%s\'", my->multitempfile, my->multifile);
			perror("rename(): ");
		}
		
	}
}

static TIMESTAMP multi_recorder_write(OBJECT *obj)
{
	struct recorder *my = OBJECTDATA(obj,struct recorder);
	char ts[64]="0"; /* 0 = INIT */
	if (my->format==0)
	{
		if (my->last.ts>TS_ZERO)
		{
			DATETIME dt;
			gl_localtime(my->last.ts,&dt);
			gl_strtime(&dt,ts,sizeof(ts));
		}
		/* else leave INIT in the buffer */
	}
	else
		sprintf(ts,"%" FMT_INT64 "d", my->last.ts);
	if ((my->limit>0 && my->samples > my->limit) /* limit reached */
		|| write_multi_recorder(my, ts, my->last.value)==0) /* write failed */
	{
		close_multi_recorder(my);
		my->status = TS_DONE;
	}
	else
		my->samples++;

	/* at this point we've written the sample to the normal recorder output */

	// if file based
	if(my->multifp != NULL){
		char1024 inbuffer;
		char1024 outbuffer;
		char *lasts = 0;
		char *in_ts = 0;
		char *in_tok = 0;

		memset(inbuffer, 0, sizeof(inbuffer));
		
		if(my->inputfp != NULL){
			// read line
			do{
				if(0 == fgets(inbuffer, 1024, my->inputfp)){
					if(feof(my->inputfp)){
						// if there is no more data to append rows to, we're done with the aggregate 
						//fclose(my->multifp); // happens in close()
						//fclose(my->inputfp); // one-over read never happens
						return TS_NEVER;
					} else {
						gl_error("multirecorder: error reading past recordings file");
						my->status = TS_ERROR;
						return TS_NEVER;
					}
				}
				// if first char == '#', re-read
			} while(inbuffer[0] == '#');
			
			// NOTE: this is not thread safe!
			// split on first comma
			in_ts = strtok_s(inbuffer, ",\n", &lasts);
			in_tok = strtok_s(NULL, "\n", &lasts);

			if(in_ts == NULL){
				gl_error("multirecorder: unable to indentify a timestamp within the line read from ");
			}

			// compare timestamps if my->format == 0
				//	warn if timestamps mismatch
			if(strcmp(in_ts, ts) != 0){
				gl_warning("multirecorder: timestamp mismatch between current input line and simulation time");
			}
			sprintf(outbuffer, "%s,%s", in_tok, my->last.value);
		} else { // no input file ~ write normal output
			strcpy(outbuffer, my->last.value);
		}
		// fprintf 
		fprintf(my->multifp, "%s,%s\n", ts, outbuffer);
	}
	return TS_NEVER;
}

RECORDER_MAP *link_multi_properties(OBJECT *obj, char *property_list)
{
	char *itemptr, *item;
	char objstr[128];
	char itemstr[128];
	RECORDER_MAP *first=NULL, *last=NULL, *rmap;
	OBJECT *target_obj = NULL;
	UNIT *unit = NULL;
	char1024 list;
	complex oblig;
	int partres = 0;
	char name[128];
	char256 pstr, ustr;
	char *cpart = 0;
	int64 cid = -1;
	PROPERTY *prop = NULL;
	PROPERTY *target = NULL;
	double scale = 1.0;

	strcpy(list,property_list); /* avoid destroying orginal list */
	for (itemptr = strtok(list,","); itemptr != NULL; itemptr = strtok(NULL,","))
	{
		cpart = 0;
		cid = -1;
		prop = NULL;
		target = NULL;
		scale = 1.0;
		ustr[0] = 0;
		pstr[0] = 0;
		unit = NULL;

		//if(2 == sscanf(itemptr, "%[^:]:%[^\n\r\0]", objstr, itemstr)){
		if(2 == sscanf(itemptr, "%[^:]:%s", objstr, itemstr)){	//changed this line because of conflicts in rh5
			item = itemstr;
			target_obj = gl_get_object(objstr);
			if(target_obj == 0){
				gl_error("multirecorder: unable to find object '%s'", objstr);
				return 0;
			}
		} else { // only the one part
			if(obj == 0){
				gl_error("multirecorder: no parent object and no specified target object in '%s'", itemstr);
				return 0;
			}
			target_obj = obj;
			item = objstr;
		}

		// everything that looks like a property name, then read units up to ]
		while (isspace(*item)) item++;
		if(2 == sscanf(item,"%[A-Za-z0-9_.][%[^]\n,\0]", pstr, ustr)){
			unit = gl_find_unit(ustr);
			if(unit == NULL){
				gl_error("multirecorder: unable to find unit '%s' for property '%s' in object '%s %i'", ustr,pstr,target_obj->oclass->name, target_obj->id);
				return NULL;
			}
			item = pstr;
		}
		rmap = (RECORDER_MAP *)malloc(sizeof(RECORDER_MAP));
		memset(rmap, 0, sizeof(RECORDER_MAP));
		
		/* branch: test to see if we're trying to split up a complex property */
		/* must occur w/ *cpart=0 before gl_get_property in order to properly reformat the property name string */
		cpart = strchr(item, '.');
		if(cpart != NULL){
			if(strcmp("imag", cpart+1) == 0){
				cid = (int)((int64)&(oblig.i) - (int64)&oblig);
				*cpart = 0;
			} else if(strcmp("real", cpart+1) == 0){
				cid = (int)((int64)&(oblig.r) - (int64)&oblig);
				*cpart = 0;
			} else {
				;
			}
		}

		target = gl_get_property(target_obj,item,NULL);

		if (rmap != NULL && target != NULL)
		{
			if(unit != NULL && target->unit == NULL){
				gl_error("recorder:%d: property '%s' is unitless, ignoring unit conversion", obj->id, item);
			}
			else if(unit != NULL && 0 == gl_convert_ex(target->unit, unit, &scale))
			{
				gl_error("recorder:%d: unable to convert property '%s' units to '%s'", obj->id, item, ustr);
				return NULL;
			}
			if(first == NULL){
				first = rmap;
			} else {
				last->next=rmap;
			}
			last = rmap;
			rmap->obj = target_obj;
			memcpy(&(rmap->prop), target, sizeof(PROPERTY));
			rmap->prop.unit = unit;
			rmap->next = NULL;
		}
		else
		{
			gl_name_object(target_obj, name, 128);
			gl_error("multirecorder: property '%s' not found in object '%s'", item, name);
			return NULL;
		}
		if(cid >= 0){ /* doing the complex part thing */
			rmap->prop.ptype = PT_double;
			(rmap->prop.addr) = (PROPERTYADDR)((int64)(rmap->prop.addr) + cid);
		}
	}
	return first;
}

int read_multi_properties(struct recorder *my, OBJECT *obj, RECORDER_MAP *rmap, char *buffer, int size)
{
	RECORDER_MAP *r;
	int offset = 0;
	int count = 0;
	double value;
	PROPERTY *p2 = 0;
	PROPERTY fake;
	memset(&fake, 0, sizeof(PROPERTY));
	fake.ptype = PT_double;
	fake.unit = 0;
	for(r = rmap; r != NULL && offset < size - 33; r = r->next){
		if(offset > 0){
			strcpy(buffer+offset++,",");
		}
//		offset += gl_get_value(r->obj, GETADDR(r->obj, &(r->prop)), buffer + offset, size - offset - 1, &(r->prop)); /* pointer => int64 */
		if(r->prop.ptype == PT_double){
			switch(my->line_units){
				case LU_ALL:
					// cascade into 'default', as prop->unit should've been set, if there's a unit available.
				case LU_DEFAULT:
					offset+=gl_get_value(r->obj,GETADDR(r->obj,&(r->prop)),buffer+offset,size-offset-1,&(r->prop)); /* pointer => int64 */
					break;
				case LU_NONE:
					// copy value into local value, use fake PROP, feed into gl_get_vaule
					value = *gl_get_double(r->obj, &(r->prop));
					p2 = gl_get_property(r->obj, r->prop.name,NULL);
					if(p2 == 0){
						gl_error("unable to locate %s.%s for LU_NONE", r->obj, r->prop.name);
						return 0;
					}
					if(r->prop.unit != 0 && p2->unit != 0){
						if(0 == gl_convert_ex(p2->unit, r->prop.unit, &value)){
							gl_error("unable to convert %s to %s for LU_NONE", r->prop.unit, p2->unit);
						} else { // converted
							offset+=gl_get_value(r->obj,&value,buffer+offset,size-offset-1,&fake); /* pointer => int64 */;
						}
					} else {
						offset+=gl_get_value(r->obj,GETADDR(r->obj,&(r->prop)),buffer+offset,size-offset-1,&(r->prop)); /* pointer => int64 */;
					}
					break;
				default:
					break;
			}
		} else {
		  //offset += gl_get_value(obj,    GETADDR(obj,    p),          buffer+offset, size-offset-1, p); /* pointer => int64 */
			offset += gl_get_value(r->obj, GETADDR(r->obj, &(r->prop)), buffer+offset, size-offset-1, &(r->prop)); /* pointer => int64 */
		}
		buffer[offset] = '\0';
		count++;
	}
	return count;
}

EXPORT TIMESTAMP sync_multi_recorder(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	struct recorder *my = OBJECTDATA(obj,struct recorder);
	typedef enum {NONE='\0', LT='<', EQ='=', GT='>'} COMPAREOP;
	COMPAREOP comparison;
	char1024 buffer = "";
	
	if (my->status==TS_DONE)
	{
		close_multi_recorder(my); /* note: potentially called every sync pass for multiple timesteps, catch fp==NULL in tape ops */
		return TS_NEVER;
	}

	/* legitimate to lack a parent by the definition of a multi-recorder.  -MH */
	/*
	if(obj->parent == NULL){
		char tb[32];
		sprintf(buffer, "'%s' lacks a parent object", obj->name ? obj->name : tb, sprintf(tb, "recorder:%i", obj->id));
		close_recorder(my);
		my->status = TS_ERROR;
		goto Error;
	}*/

	if(my->last.ts < 1 && my->interval != -1)
		my->last.ts = t0;

	/* connect to property */
	if (my->rmap==NULL){
		my->rmap = link_multi_properties(obj->parent,my->property); // allowable use of obj->parent
	}
	/*	invalid target object must be handled individually */
	/*if (my->target==NULL)
	{
		sprintf(buffer,"'%s' contains a property of %s %d that is not found", my->property, obj->parent->oclass->name, obj->parent->id);
		close_multi_recorder(my);
		my->status = TS_ERROR;
		goto Error;
	}*/

	if (my->rmap==NULL)
	{
		sprintf(buffer,"'%s' contains a property reference that was not found", my->property);
		close_multi_recorder(my);
		my->status = TS_ERROR;
		goto Error;
	}

	// update clock
	if ((my->status==TS_OPEN) && (t0 > obj->clock)) 
	{	
		obj->clock = t0;
		// if the recorder is clock-based, write the value
		if((my->interval > 0) && (my->last.ts < t0) && (my->last.value[0] != 0)){
			multi_recorder_write(obj);
			my->last.value[0] = 0; // once it's been finalized, dump it
		}
	}

	/* update property value */
	if ((my->rmap != NULL) && (my->interval == 0 || my->interval == -1)){	
		if(read_multi_properties(my, obj->parent,my->rmap,buffer,sizeof(buffer))==0) // vestigal use of parent
		{
			//sprintf(buffer,"unable to read property '%s' of %s %d", my->property, obj->parent->oclass->name, obj->parent->id);
			sprintf(buffer,"unable to read a property");
			close_multi_recorder(my);
			my->status = TS_ERROR;
		}
	}
	if ((my->rmap != NULL) && (my->interval > 0)){
		if((t0 >=my->last.ts + my->interval) || (t0 == my->last.ts)){
			if(read_multi_properties(my, obj->parent,my->rmap,buffer,sizeof(buffer))==0)
			{
				//sprintf(buffer,"unable to read property '%s' of %s %d", my->property, obj->parent->oclass->name, obj->parent->id);
				sprintf(buffer,"unable to read a property");
				close_multi_recorder(my);
				my->status = TS_ERROR;
			}
			my->last.ts = t0;
		}
	}

	/* check trigger, if any */
	comparison = (COMPAREOP)my->trigger[0];
	if (comparison!=NONE)
	{
		int desired = comparison==LT ? -1 : (comparison==EQ ? 0 : (comparison==GT ? 1 : -2));

		/* if not trigger or can't get access */
		int actual = strcmp(buffer,my->trigger+1);
		if (actual!=desired || (my->status==TS_INIT && !multi_recorder_open(obj)))
		{
			/* better luck next time */
			return (my->interval==0 || my->interval==-1) ? TS_NEVER : t0+my->interval;
		}
	}
	else if (my->status==TS_INIT && !multi_recorder_open(obj))
	{
		close_multi_recorder(my);
		return TS_NEVER;
	}

	if(my->last.ts < 1 && my->interval != -1)
		my->last.ts = t0;

	/* write tape */
	if (my->status==TS_OPEN)
	{	
		if (my->interval==0 /* sample on every pass */
			|| ((my->interval==-1) && my->last.ts!=t0 && strcmp(buffer,my->last.value)!=0) /* sample only when value changes */
			)

		{
			strncpy(my->last.value,buffer,sizeof(my->last.value));
			my->last.ts = t0;
			multi_recorder_write(obj);
		} else if (my->interval > 0 && my->last.ts == t0){
			strncpy(my->last.value,buffer,sizeof(my->last.value));
		}
	}
Error:
	if (my->status==TS_ERROR)
	{
		gl_error("recorder %d %s\n",obj->id, buffer);
		close_multi_recorder(my);
		my->status=TS_DONE;
		return TS_NEVER;
	}

	if (my->interval==0 || my->interval==-1) 
	{
		return TS_NEVER;
	}
	else
	{
		return my->last.ts+my->interval;
	}
}

/**@}*/
