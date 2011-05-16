/* $Id$
 *
 */

#include "output.h"
#include "stream.h"
#include "module.h"
#include "globals.h"
#include "schedule.h"
#include "class.h"
#include "object.h"

/* stream name - this should never be changed */
#define STREAM_NAME "GRIDLABD"

/* stream version - change this when the structure of the stream changes */
#define STREAM_VERSION 1

/* stream format - binary is normal, undefine BINARY to debug the stream visually */
#define BINARY
#ifndef _DEBUG
#ifndef BINARY
#error Binary must be defined for release versions of stream.c
#endif
#endif

/* output macros */
static unsigned char b,t;
static unsigned char c;
static unsigned short s;
static uint32 l;
static unsigned int64 q;
static unsigned short n;
static unsigned int64 nn;

#ifdef BINARY
#define COMPRESS(S,L) {if ((nn=stream_compress(fp,(S),(L)))<=0) return -1; else count+=nn;} 
#define PUTD(S,L) {n=(L);if(fwrite(&n,sizeof(n),1,fp)!=1 || fwrite((S),1,L,fp)!=L) return -1; else count+=n+2;}
#define PUTC(C) {c=(unsigned char)(C);PUTD(&c,sizeof(c))}
#define PUTS(S) {s=(unsigned short)(S);PUTD(&s,sizeof(s))}
#define PUTL(L) {l=(uint32)(L);PUTD(&l,sizeof(l))}
#define PUTQ(Q) {q=(unsigned int64)(Q);PUTD(&q,sizeof(q))}
#define PUTX(T,X) {if ((n=stream_out_##T(fp,(X)))<0) return -1; else count+=n;}
#define PUTT(B,T) {unsigned short w=0; b=SB_##B; t=ST_##T; if (fwrite(&w,2,1,fp)!=1 || fwrite(&b,1,1,fp)!=1 || fwrite(&t,1,1,fp)!=1) return -1; else count+=4;}
#else
static char indent[256]="";
void indent_more() { if (strlen(indent)<sizeof(indent)/sizeof(indent[0])) strcat(indent," ");}
void indent_less() { if (indent[0]!='\0') indent[strlen(indent)-1] = '\0';}
#define PUTD(S,L) {n=(L);if(fprintf(fp,"%sD%d/%s:%s\n",indent,L,#S,S)<=0) return -1; else count+=n+2;}
#define PUTC(C) {c=(C);if(fprintf(fp,"%sC/%s:%d\n",indent,#C,C)<=0) return -1; else count+=3;}
#define PUTS(S) {s=(S);if(fprintf(fp,"%sS/%s:%d\n",indent,#S,S)<=0) return -1; else count+=4;}
#define PUTL(L) {l=(L);if(fprintf(fp,"%sL/%s:%d\n",indent,#L,L)<=0) return -1; else count+=6;}
#define PUTQ(Q) {q=(Q);if(fprintf(fp,"%sQ/%s:%d\n",indent,#Q,Q)<=0) return -1; else count+=10;}
#define PUTX(T,X) {if ((n=stream_out_##T(fp,(X)))<0) return -1; else count+=n;}
#define PUTT(B,T) {if (SB_##B==SB_BEGIN || ST_##T==ST_BEGIN) indent_more(); if(fprintf(fp,"%s[%s %s]\n",indent,#B,#T)<=0) return -1; else count+=2; if (SB_##B==SB_END || ST_##T==ST_END) indent_less();}
#endif

#define DECOMPRESS(S,L) {if ((nn=stream_decompress(fp,(S),(L)))<=0) return -1; else count+=nn;}
#define GETD(S,L) {if(fread(&n,sizeof(n),1,fp)!=1 || n>(L) || fread((S),n,1,fp)!=1) return -1; else count+=n+2;}
#define GETC(C) GETD(&(C),sizeof(c))
#define GETS(S) GETD(&(S),sizeof(s))
#define GETL(L) GETD(&(L),sizeof(l))
#define GETQ(Q) GETD(&(Q),sizeof(q))
static int ok=1;
#define GETBT (ok?(ok=0,fread(&n,sizeof(n),1,fp)==1 && n==0 && fread(&b,1,1,fp)==1 && fread(&t,1,1,fp)==1):1)
#define OK (ok=1)
#define B(X) (b==SB_##X)
#define T(X) (t==ST_##X)

/* stream block tokens */
enum {
	SB_BEGIN=0x00,

	SB_MODULE,
	SB_GLOBAL,
	SB_CLASS,
	SB_OBJECT,
	SB_UNIT,
	SB_KEYWORD,
	SB_PROPERTY,
	SB_SCHEDULE,
	SB_TRANSFORM,

	SB_END=0xff,
};

/* stream type tokens */
enum {
	ST_BEGIN=0x00,

	/* these should match stream block tokens */
	ST_MODULE,
	ST_GLOBAL,
	ST_CLASS,
	ST_OBJECT,
	ST_UNIT,
	ST_KEYWORD,
	ST_PROPERTY,
	ST_SCHEDULE,
	ST_TRANSFORM,

	/* these do not have corresponding stream block tokens */
	ST_VERSION,
	ST_ID,
	ST_NAME,
	ST_GROUP,
	ST_PARENT,
	ST_RANK,
	ST_CLOCK,
	ST_VALIDTO,
	ST_LATITUDE,
	ST_LONGITUDE,
	ST_INSVC,
	ST_OUTSVC,
	ST_FLAGS,
	ST_TYPE,
	ST_SIZE,
	ST_ACCESS,
	ST_VALUE,
	ST_DEFINITION,
	ST_BIAS,
	ST_SCALE,
	ST_DATA,

	ST_END=0xff,
};

char *stream_context()
{
	static char buffer[64];
	sprintf(buffer,"block %d, token %d",b,t);
	return buffer;
}
int stream_error(char *format, ...)
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,format,ptr);
	va_end(ptr);
	output_error("- stream(%d:%d) - %s", b,t,buffer);
	return -1;
}

int stream_warning(char *format, ...)
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,format,ptr);
	va_end(ptr);
	output_warning("- stream(%d:%d) - %s", b,t,buffer);
	return -1;
}

/** stream_compress

	Format of compressed stream 

	[US/len] [bit15 runlen flag, bit 0-14 data len]
	
	Bit 15 clear : raw data follows for len given
	[UC/data ... ]

	Bit 15 set : compressed data 
	[SC/delta] differential to apply to each value
	[SC/value] initial value

 **/
size_t stream_compress(FILE *fp, char *buf, size_t len)
{
	size_t count = 0, original = len;
	char *p = buf; // current position in input buffer
	char *raw = p; // start of raw buffer
	unsigned short rawlen = 0; // len of raw buffer
	char *run = p; // start of run buffer
	unsigned short runlen = 0; // len of run buffer
	char diff = 0; // current differential run value
	enum {RAW=0, RUNLEN=1} state = RAW; // state of compression
	for ( p = buf ; len-->0 ; p++ ) 
	{
		int dp = p[1]-p[0];

		if (state == RAW)
		{
			if (dp==diff) // pattern repeats
				runlen++;
			else
				runlen=0;

			// if raw buffer in progress and run is long enough to use
			if (runlen==8)
			{
				// dump raw buffer
				if (rawlen>runlen)
				{
					rawlen -= runlen; // don't include new run data
					if (fwrite(&rawlen,1,sizeof(rawlen),fp)<0) return -1;
					if (fwrite(raw,1,rawlen,fp)<0) return -1;
					count += rawlen+2;
				}

				// change to run buffer
				state = RUNLEN;
				rawlen = 0;
				run = p-runlen;
			}

			else if (rawlen==32767)	// long raw buffer
			{
				// dump raw buffer
				if (fwrite(&rawlen,1,sizeof(rawlen),fp)<0) return -1;
				if (fwrite(raw,1,rawlen,fp)<0) return -1;
				count += rawlen+2;

				// restart raw buffer
				rawlen = 0;
				raw = p;
			}
			else
				rawlen++;

		}
		else if (state==RUNLEN)
		{
			if (dp==diff) // run continues
			{
				runlen++;

				// if run buffer is too long
				if (runlen==32767)
				{
					// mark run buffer
					runlen |= 0x8000;

					// dump run buffer
					if (fwrite(&runlen,1,sizeof(runlen),fp)<0) return -1;
					if (fwrite(&diff,1,sizeof(diff),fp)<0) return -1;
					if (fputc(*run,fp)<0) return -1; 
					count+=4;

					// start over with a new run
					runlen = 0;
					run = p;
				}
			}

			else // run ends
			{
				// mark run buffer
				runlen |= 0x8000;

				// dump run buffer
				if (fwrite(&runlen,1,sizeof(runlen),fp)<0) return -1;
				if (fwrite(&diff,1,sizeof(diff),fp)<0) return -1;
				if (fputc(*run,fp)<0) return -1; 
				count+=4;

				// change to a raw buffer
				state = RAW;
				raw = p;
				rawlen = 0;

				// get run candidate
				runlen = 0;
			}
		}

		// get next run candidate
		diff = dp;
	}
	
	// terminate compressed stream
	rawlen=0;
	if (fwrite(&rawlen,sizeof(rawlen),1,fp)<0) return -1; else count+=2;

	// stream len confirmation code
	if (fwrite(&count,sizeof(count),1,fp)<0) return -1; else count+=2;

	output_debug("stream_compress(): %d kB -> %d kB (%.1f%%)", original/1000+1, count/1000+1, (double)count*100/(double)original);
	return count;
}

/** stream_compress

	Format of compressed stream 

	[US/len] [bit15 runlen flag, bit 0-14 data len]
	
	Bit 15 clear : raw data follows for len given
	[UC/data ... ]

	Bit 15 set : compressed data 
	[SC/delta] differential to apply to each value
	[SC/value] initial value

 **/
size_t stream_decompress(FILE *fp, char *buf, const size_t len)
{
	char *ptr = buf;
	size_t count=0, buflen=0;
	size_t confirm=0;
	struct {
		unsigned int runlen:15;
		unsigned int is_compressed:1;
	} runstate;
	for (buflen=0; buflen<len; buflen+=runstate.runlen)
	{
		if (fread(&runstate,2,1,fp)!=1)
			return stream_error("stream_decompress(): failed to read runlen");
		else
			count+=2;

		// check for end of compressed stream
		if (runstate.runlen==0)
			break;

		// handle run data
		if (runstate.is_compressed) // compression flag set
		{
			char delta;
			unsigned char value;
			if (fread(&delta,1,1,fp)!=1 || fread(&value,1,1,fp)!=1) 
				return stream_error("stream_decompress(): failed to read delta/value");
			else
				count += 2;
			if (delta==0)
			{
				memset(ptr,value,runstate.runlen+1);
				ptr += runstate.runlen+1;
			}
			else
			{
				unsigned short run = runstate.runlen+1;
				while (run-->0)
				{
					*ptr++ = value;
					value += delta;
				}
			}
		}
		else // no compression
		{
			if (fread(ptr,1,runstate.runlen,fp)!=runstate.runlen)
				return stream_error("stream_decompress(): failed to read raw data");
			else
			{
				ptr += runstate.runlen;
				count += runstate.runlen;
			}
		}
	}

	// check for overrun
	if (buflen>len)
		return stream_error("stream_decompress(): stream overrun--possible invalid stream");

	// read confirmation code
	if (fread(&confirm,sizeof(confirm),1,fp)==1 && confirm==count)
		return count;
	else
		return stream_error("stream_decompress(): stream confirmation code mismatched--probable invalid stream");
}

/*******************************************************
 * MODULES 
 */
int64 stream_out_module(FILE *fp, MODULE *m)
{
	int64 count=0;

	PUTT(MODULE,BEGIN);

	PUTT(MODULE,NAME);
	PUTD(m->name,strlen(m->name));

	PUTT(MODULE,VERSION);
	PUTS(m->major);
	PUTS(m->minor);

	PUTT(MODULE,END);

	return count;
}
int64 stream_in_module(FILE *fp)
{
	int64 count=0;
	MODULE *m;

	while (GETBT && B(MODULE) && T(BEGIN))
	{
		char name[1024]; 
		memset(name,0,sizeof(name));
		OK;
		while (GETBT && B(MODULE) && !T(END))
		{
			OK;
			if T(NAME) 
			{
				GETD(name,sizeof(name));
				m = module_load(name,0,NULL);
				if (m==NULL)
					return stream_error("module %s version is not found", name);
			}
			else if T(VERSION) 
			{
				unsigned short major, minor;
				GETS(major);
				GETS(minor);
				if (m->major!=major || m->minor!=minor)
					return stream_error("module %s version %d.%02d specified does not match version %d.%02d found", name, major, minor, m->major, m->minor);
			}
			else
				stream_warning("ignoring token %d in module stream", t);
		}
		OK;
	}
	return count;
}

/*******************************************************
 * UNIT
 */
int64 stream_out_unit(FILE *fp, UNIT *u)
{
	int64 count=0;
	PUTT(UNIT,BEGIN);

	PUTT(UNIT,NAME);
	PUTD(u->name,strlen(u->name));

	PUTT(UNIT,END);
	return count;
}

/*******************************************************
 * KEYWORD
 */
int64 stream_out_keyword(FILE *fp, KEYWORD *k)
{
	int64 count=0;
	PUTT(KEYWORD,BEGIN);

	PUTT(KEYWORD,NAME);
	PUTD(k->name,strlen(k->name));

	PUTT(KEYWORD,VALUE);
	PUTQ(k->value);

	PUTT(KEYWORD,END);
	return count;
}

/*******************************************************
 * PROPERTIES 
 */
int64 stream_out_property(FILE *fp, PROPERTY *p)
{
	int64 count=0;
	KEYWORD *key=NULL;
	PUTT(PROPERTY,BEGIN);

	if (strlen(p->name)>0)
	{
		PUTT(PROPERTY,NAME);
		PUTD(p->name,strlen(p->name));
	}

	PUTT(PROPERTY,TYPE);
	PUTL(p->ptype);

	PUTT(PROPERTY,ACCESS);
	PUTL(p->access);

	PUTT(PROPERTY,SIZE);
	PUTL(p->size);

	if (p->unit)
	{
		PUTT(PROPERTY,UNIT);
		PUTX(unit,p->unit);
	}

	PUTT(BEGIN,KEYWORD);
	for (key=p->keywords;key!=NULL;key=key->next)
		PUTX(keyword,key);
	PUTT(END,KEYWORD);

	PUTT(PROPERTY,END);
	return count;
}
int64 stream_in_property(FILE *fp, PROPERTY *p)
{
	return 0;
}

/*******************************************************
 * GLOBALS 
 */
int64 stream_out_global(FILE *fp, GLOBALVAR *v)
{
	int64 count=0;
	char value[4096];

	if (global_getvar(v->prop->name,value,sizeof(value))==NULL)
		return -1;

	PUTT(GLOBAL,BEGIN);
	
	PUTT(GLOBAL,NAME);
	PUTD(v->prop->name,strlen(v->prop->name));

	PUTT(GLOBAL,VALUE);
	PUTD(value,strlen(value));

	PUTT(GLOBAL,END);
	return count;
}
int64 stream_in_global(FILE *fp)
{
	int64 count=0;
	GLOBALVAR *v;

	while (GETBT && B(GLOBAL) && T(BEGIN))
	{
		char name[1024]; 
		char value[1024]; 
		memset(name,0,sizeof(name));
		memset(value,0,sizeof(value));
		OK;
		while (GETBT && B(GLOBAL) && !T(END))
		{
			OK;
			if T(NAME) 
			{
				GETD(name,sizeof(name));
				v = global_find(name);
			}
			else if T(VALUE) 
			{
				GETD(value,sizeof(value));
				if (v->prop->access==PA_PUBLIC && strcmp(value,"")!=0 && strcmp(value,"\"\"")!=0) /* only public variables are loaded */
					global_setvar(name,value,NULL);
			}
			else
				stream_warning("ignoring token %d in global stream", t);
		}
		OK;
	}
	return count;
}

/*******************************************************
 * CLASS
 */
int64 stream_out_class(FILE *fp, CLASS *oclass)
{
	int64 count=0;
	PROPERTY *prop=NULL;
	int extended=FALSE;

	for (prop=oclass->pmap; prop!=NULL; prop=prop->next)
	{
		if ((prop->flags&PT_EXTENDED)==PT_EXTENDED)
		{
			/* open the class if needed */
			if (!extended)
			{
				PUTT(CLASS,BEGIN);
				PUTT(CLASS,NAME);
				PUTD(oclass->name,strlen(oclass->name));

				extended = TRUE;
			}

			PUTT(CLASS,PROPERTY);
			PUTD(prop->name,strlen(prop->name));

			PUTT(CLASS,TYPE);
			PUTL(prop->ptype);

			if (prop->unit)
			{
				PUTT(CLASS,UNIT);
				PUTD(prop->unit->name,strlen(prop->unit->name));
			}

			/* TODO */
			//PUTT(CLASS,CODE);
			//PUTD(code,sizeof(code));
		}
	}

	/* close class if extended */
	if (extended)
	{
		PUTT(CLASS,END);
	}
	return count;
}
int stream_in_class(FILE *fp)
{
	int64 count=0;
	CLASS *c;

	while (GETBT && B(CLASS) && T(BEGIN))
	{
		char cname[1024]; 
		char pname[1024]; 
		char ptype[1024];
		char punit[1024];
		char code[65536];
		memset(cname,0,sizeof(cname));
		memset(pname,0,sizeof(pname));
		memset(ptype,0,sizeof(ptype));
		memset(punit,0,sizeof(punit));
		memset(code,0,sizeof(code));
		OK;
		while (GETBT && B(CLASS) && !T(END))
		{
			OK;
			if T(NAME) 
			{
				GETD(cname,sizeof(cname));
			}
			else if T(PROPERTY) 
			{
				GETD(pname,sizeof(pname));
			}
			else if T(TYPE)
			{
				GETD(ptype,sizeof(ptype));
			}
			else if T(UNIT)
			{
				GETD(punit,sizeof(punit));
			}
			// TODO
			//else if T(CODE)
			//{
			//	GETD(code,sizeof(code));
			//}
			else
				stream_warning("ignoring token %d in class stream", t);
		}
		OK;
	}
	return count;
}

/*******************************************************
 * OBJECT
 */
int64 stream_out_object(FILE *fp, OBJECT *obj)
{
	int64 count=0;
	PUTT(OBJECT,BEGIN);

	PUTT(OBJECT,CLASS);
	PUTD(obj->oclass->name,strlen(obj->oclass->name));

	PUTT(OBJECT,ID);
	PUTL(obj->id);

	if (obj->name)
	{
		PUTT(OBJECT,NAME);
		PUTD(obj->name,strlen(obj->name));
	}

	if (strlen(obj->groupid)>0)
	{
		PUTT(OBJECT,GROUP);
		PUTD(obj->groupid,strlen(obj->groupid));
	}

	if (obj->parent)
	{
		PUTT(OBJECT,PARENT);
		PUTL(obj->parent->id);
	}

	PUTT(OBJECT,RANK);
	PUTL(obj->rank);

	PUTT(OBJECT,CLOCK);
	PUTQ(obj->clock);

	PUTT(OBJECT,VALIDTO);
	PUTQ(obj->valid_to);

	PUTT(OBJECT,LATITUDE);
	PUTD(&(obj->latitude),sizeof(obj->latitude));

	PUTT(OBJECT,LONGITUDE);
	PUTD(&(obj->longitude),sizeof(obj->longitude));

	PUTT(OBJECT,INSVC);
	PUTQ(obj->in_svc);

	PUTT(OBJECT,OUTSVC);
	PUTQ(obj->out_svc);

	PUTT(OBJECT,FLAGS);
	PUTL(obj->flags);

	if (obj->oclass->pmap)
	{
		PROPERTY *p;
		for (p=obj->oclass->pmap; p!=NULL; p=p->next)
		{
			char value[4096]="";
			void *addr = (void*)((char*)(obj+1)+(unsigned int64)p->addr);
			PUTT(OBJECT,PROPERTY);
			PUTL(p->ptype);
			PUTS(p->addr);
			if (p->ptype==PT_object)
			{
				OBJECT *ref = *(OBJECT**)addr;
				uint32 id = ref==NULL ? -1 : ref->id;
				PUTL(id);
			}
			else
			{
				PUTD(addr,property_size(p));
			}
		}
	}

	PUTT(OBJECT,END);
	return count;
}
int64 stream_in_object(fp)
{
	int64 count=0;
	OBJECT *obj = NULL;
	char name[65536];
	memset(name,0,sizeof(name));

	while (GETBT && B(OBJECT) && T(BEGIN))
	{
		OK;
		while (GETBT && B(OBJECT) && !T(END))
		{
			char buffer[1024];
			memset(buffer,0,sizeof(buffer));
			OK;
			if T(CLASS)
			{
				CLASS *oclass;
				GETD(buffer,sizeof(buffer));
				oclass = class_get_class_from_classname(buffer);
				if (oclass==NULL)
					return stream_error("class '%s' not found", buffer);
				if (oclass->create==NULL)
					return stream_error("class '%s' does not implement object creation", buffer);
				if (oclass->create(&obj,NULL)==0)
					return stream_error("failed to create object of class '%s'", buffer);
			}
			else if T(ID)
			{
				uint32 id;
				GETL(id);
				if (id!=obj->id)
					return stream_error("object id %d mismatched--object sequence is not valid", id);
			}
			else if T(NAME)
			{
				GETD(buffer,sizeof(buffer));
				obj->name = malloc(strlen(buffer)+1);
				strcpy(obj->name,buffer);
			}
			else if T(GROUP)
			{
				GETD(obj->groupid,sizeof(obj->groupid));
			}
			else if T(PARENT)
			{
				GETL(obj->parent);
				// this gets fixed after all objects are loaded
			}
			else if T(RANK)
			{
				GETL(obj->rank);
			}
			else if T(CLOCK)
			{
				GETQ(obj->clock);
			}
			else if T(VALIDTO)
			{
				GETQ(obj->valid_to);
			}
			else if T(LATITUDE)
			{
				GETD(&(obj->latitude),sizeof(obj->latitude));
			}
			else if T(LONGITUDE)
			{
				GETD(&(obj->longitude),sizeof(obj->longitude));
			}
			else if T(INSVC)
			{
				GETQ(obj->in_svc);
			}
			else if T(OUTSVC)
			{
				GETQ(obj->out_svc);
			}
			else if T(FLAGS)
			{
				GETL(obj->flags);
			}
			else if T(PROPERTY)
			{
				unsigned short addr;
				PROPERTYTYPE ptype;
				GETL(ptype);
				GETS(addr);
				GETD((char*)(obj+1)+addr,property_size_by_type(ptype));
			}
			else
				stream_warning("ignoring token %d in object stream", t);
		}
		OK;

		// TODO indexing, see load.c:load_set_index()
		obj = NULL;
	}
	return count;
}

/*******************************************************
 * SCHEDULE
 */
int64 stream_out_schedule(FILE *fp, SCHEDULE *sch)
{
	int64 count=0;

	PUTT(SCHEDULE,BEGIN);

#ifdef COMPRESS
	PUTT(SCHEDULE,DATA);
	COMPRESS((char*)sch,sizeof(SCHEDULE));
#else
	PUTT(SCHEDULE,NAME);
	PUTD(sch->name,strlen(sch->name));

	PUTT(SCHEDULE,DEFINITION);
	PUTD(sch->definition,strlen(sch->definition));
#endif

	PUTT(SCHEDULE,END);

	return count;
}
int64 stream_in_schedule(fp)
{
	int64 count=0;

	while (GETBT && B(SCHEDULE) && T(BEGIN))
	{
		OK;
		while (GETBT && B(SCHEDULE) && !T(END))
		{
			OK;
			if T(DATA) 
			{
				SCHEDULE *sch = schedule_new();
				DECOMPRESS((char*)sch,sizeof(SCHEDULE));
				schedule_add(sch);
			}
			// TODO other terms
			else
				stream_warning("ignoring token %d in schedule stream", t);
		}
		OK;
	}
	return count;
}

/*******************************************************
 * TRANSFORM
 */
int64 stream_out_transform(FILE *fp, SCHEDULEXFORM *xform)
{
	int64 count=0;
	OBJECT *obj = NULL;
	PROPERTY *prop = NULL;

	PUTT(TRANSFORM,BEGIN);

	PUTT(TRANSFORM,TYPE);
	PUTL(xform->source_type);

	switch (xform->source_type) {
	case XS_SCHEDULE:
		PUTT(TRANSFORM,SCHEDULE);
		PUTD(((SCHEDULE*)(xform->source_addr))->name,strlen(((SCHEDULE*)(xform->source_addr))->name));
		break;
	case XS_DOUBLE:
	case XS_COMPLEX:
	case XS_LOADSHAPE:
	case XS_ENDUSE:
		if (object_locate_property(xform->source,&obj,&prop))
		{
			PUTT(TRANSFORM,OBJECT);
			PUTL(obj->id);
			PUTL((uint32)(prop->addr));
		}
		else
		{
			stream_error("transform is unable to source for value at %x", xform->source);
			return -1;
		}
		break;
	default:
		stream_error("transform uses undefined source type (%d)", xform->source_type);
		return -1;
	}
	PUTL(xform->target_obj->id);
	PUTL(xform->target_prop->addr);

	PUTT(TRANSFORM,SCALE);
	PUTQ(xform->scale);

	PUTT(TRANSFORM,BIAS);
	PUTQ(xform->bias);

	PUTT(TRANSFORM,END);
	return count;
}
int64 stream_in_transform(fp)
{
	int64 count=0;

	while (GETBT && B(TRANSFORM) && T(BEGIN))
	{
		SCHEDULEXFORM *xform = (SCHEDULEXFORM*)malloc(sizeof(SCHEDULEXFORM));
		OK;
		while (GETBT && B(TRANSFORM) && !T(END))
		{
			OK;
			if T(TYPE) 
			{
				GETL(xform->source_type);
			}
			else if T(SCHEDULE)
			{
				SCHEDULE *sch;
				char schedule_name[64];
				memset(schedule_name,0,sizeof(schedule_name));
				if (xform->source_type!=XS_SCHEDULE)
					return stream_error("stream_in_transform(): schedule not expected");
				GETD(schedule_name,sizeof(schedule_name));
				sch = schedule_find_byname(schedule_name);
				if (sch==NULL)
					return stream_error("stream_in_transform(): schedule '%s' not found", schedule_name);
				else
					xform->source_addr = sch;

				// target
				GETL(xform->target_obj);
				// id will get fixed up later
				GETL(xform->target_prop);
				// addr will get fixed up later
			}
			else if T(OBJECT)
			{
				uint32 object_id;
				uint32 prop_addr;
				if (xform->source_type!=XS_DOUBLE && xform->source_type!=XS_COMPLEX && xform->source_type!=XS_LOADSHAPE && xform->source_type!=XS_ENDUSE)
					return stream_error("stream_in_transform(): property not expected");

				// source
				GETL(object_id);
				GETL(prop_addr);
				xform->source = (char*)(object_find_by_id(object_id)+1)+prop_addr;
				if (xform->source==NULL)
					return stream_error("stream_in_transform(): source object id=%d not found", object_id);

				// target
				GETL(object_id);
				xform->target_obj = object_find_by_id(object_id);
				if (xform->target_obj == NULL)
					return stream_error("stream_in_transform(): target object id=%d not found", object_id);
				GETL(prop_addr);
				for (xform->target_prop = class_get_first_property(xform->target_obj->oclass); xform->target_prop!=NULL; xform->target_prop = class_get_next_property(xform->target_prop))
				{
					if (xform->target_prop->addr == prop_addr)
						break;
				}
				if (xform->target_prop==NULL)
					return stream_error("stream_in_transform(): target property at addr %d not found", prop_addr);
			}
			else if T(SCALE)
			{
				GETQ(xform->scale);
			}
			else if T(BIAS)
			{
				GETQ(xform->bias);
			}
			else
				stream_warning("ignoring token %d in object stream", t);
		}
		OK;
	}
	return count;
}


/*******************************************************
 * OUTPUT STREAM
 */
int64 stream_out(FILE *fp, int flags)
{
	int64 count = 0;
	MODULE *mod=NULL;
	GLOBALVAR *gvar=NULL;
	CLASS *oclass=NULL;
	OBJECT *obj=NULL;
	SCHEDULE *sch=NULL;
	SCHEDULEXFORM *xform=NULL;

	/* stream header */
	PUTD(STREAM_NAME,strlen(STREAM_NAME));

	/* stream level 1 */
	PUTS(STREAM_VERSION);

	/* higher stream level data goes here... */

	/* schedules */
	while ((sch=schedule_getnext(sch))!=NULL)
		PUTX(schedule,sch);

	/* modules (not optional) */
	for (mod=module_get_first(); (flags&SF_MODULES) && mod!=NULL; mod=mod->next)
		PUTX(module,mod);

	/* globals */
	while ((flags&SF_GLOBALS) && (gvar=global_getnext(gvar))!=NULL)
		PUTX(global,gvar);

	/* classes */
	for (oclass=class_get_first_class(); (flags&SF_CLASSES) && oclass!=NULL; oclass=oclass->next)
		PUTX(class,oclass);

	/* objects */
	for (obj=object_get_first(); (flags&SF_OBJECTS) && obj!=NULL; obj=obj->next)
		PUTX(object,obj);

	/* transforms */
	while ((xform=scheduletransform_getnext(xform))!=NULL)
		PUTX(transform,xform);

	PUTT(END,END);
	output_verbose("stream_out() sent %"FMT_INT64"d bytes", count);
	return count;
}

int64 stream_in(FILE *fp, int flags)
{
	int64 count = 0;
	OBJECT *obj;
	SCHEDULEXFORM *xform=NULL;
	
	{	char stream_name[] = STREAM_NAME;
		GETD(stream_name,sizeof(stream_name));
		if (strcmp(stream_name,STREAM_NAME)!=0)
			return stream_error("stream_in(): stream name '%s' mismatch", stream_name);
	}

	{	unsigned short stream_version = STREAM_VERSION;
		GETS(stream_version);
		if (stream_version!=STREAM_VERSION)
			return stream_error("stream_in(): stream version %d mismatch", stream_version);
	}

	while (!feof(fp) && !ferror(fp))
	{
		int c=0;
		if (GETBT && B(END) && T(END))
			goto Done;
		c += stream_in_object(fp);
		c += stream_in_global(fp);
		c += stream_in_schedule(fp);
		c += stream_in_module(fp);
		c += stream_in_class(fp);
		c += stream_in_transform(fp);
		if (c==0)
			return stream_error("stream_in(): unable to continue");
		else
			count += c;
	}

	if (!feof(fp) || !ok)
		return stream_error("stream_in(): unhandled block/token at position %d", count);
	else if (ferror(fp))
		return stream_error("stream_in(): %s", strerror(errno));

Done:
	// fixup object parents and object properties
	for (obj=object_get_first(); obj!=NULL; obj=obj->next)
	{
		PROPERTY *p;
		obj->parent = object_find_by_id(obj->parent);
		for (p=class_get_first_property(obj->oclass); p!=NULL; p=class_get_next_property(p))
		{
			if (p->ptype==PT_object)
			{
				OBJECT **ref = (OBJECT*)((char*)(obj+1)+(unsigned short)p->addr);
				*ref = object_find_by_id((uint32)(*ref));
			}
		}
	}

	// fixup transform targets
	while ((xform=scheduletransform_getnext(xform))!=NULL)
	{
		PROPERTY *p;
		xform->target_obj = object_find_by_id(xform->target_obj);
		for (p=class_get_first_property(xform->target_obj->oclass); p!=NULL; p=class_get_next_property(p))
		{
			if (p->addr == xform->target_prop)
			{
				xform->target_prop = p;
				xform->target = (double*)((char*)(xform->target_obj+1)+(uint32)p->addr);
			}
		}
	}

	output_verbose("stream_in() received %"FMT_INT64"d bytes", count);
	return count;
}


/*******************************************************
 * OUTPUT PROPERTIES
 */

int stream_out_double(FILE *fp,void *ptr,PROPERTY *prop)
{
	int count=0;
	PUTQ(*(double*)ptr);
	return count;
}
int stream_in_double(FILE *fp,void *ptr,PROPERTY *prop)
{
	int count=0;
	GETQ(*(double*)ptr);
	return count;
}
