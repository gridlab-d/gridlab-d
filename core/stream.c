/* $Id$
 *
 */

#include "output.h"
#include "stream.h"
#include "module.h"
#include "globals.h"
#include "schedule.h"

/* stream name - this should never be changed */
#define STREAM_NAME "GRIDLABD"

/* stream version - change this when the structure of the stream changes */
#define STREAM_VERSION 1

/* stream format - binary is normal, undefine BINARY to debug the stream visually */
#define BINARY

/* output macros */
static unsigned char b,t;
static unsigned char c;
static unsigned short s;
static unsigned long l;
static unsigned int64 q;
static unsigned int64 n;

#ifdef BINARY
#define COMPRESS(S,L) {if ((n=stream_compress(fp,S,L))<=0) return -1; else count+=n;} 
#define PUTD(S,L) {n=(L);if(fwrite(&n,2,1,fp)!=1 || fwrite((S),1,L,fp)!=L) return -1; else count+=n+2;}
#define PUTC(C) {c=(C);PUTD(&c,1)}
#define PUTS(S) {s=(S);PUTD(&s,2)}
#define PUTL(L) {l=(L);PUTD(&l,4)}
#define PUTQ(Q) {q=(Q);PUTD(&q,8)}
#define PUTX(T,X) {if ((n=stream_out_##T(fp,(X)))<0) return -1; else count+=n;}
#define PUTT(B,T) {b=SB_##B;t=ST_##T;s=(unsigned short)((b<<8)|t);PUTD(&s,2)}
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

#define GETD(S,L) {char _buf[4096]; if(fread(&n,2,1,fp)!=1 || n!=(L) || n>sizeof(_buf) || fread(_buf,1,n,fp)!=n) return -1; else count+=n+2;}
#define GETC(C) {c=(C);GETD(&c,1)}
#define GETS(S) {s=(S);GETD(&s,2)}
#define GETL(L) {l=(L);GETD(&l,4)}
#define GETQ(Q)
#define GETX(T,X)
#define GETT(B,T)

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
void stream_error(char *format, ...)
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,format,ptr);
	va_end(ptr);
	output_error("- stream(%d:%d) - %s", b,t,buffer);
	return;
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
					count+=3;

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
				count+=3;

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
	output_debug("stream_compress(): %d kB -> %d kB (%.1f%%)", original/1000+1, count/1000+1, (double)count*100/(double)original);
	return count;
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
	char module_name[1024]; 
	unsigned short major, minor;
	MODULE *m;

	GETT(MODULE,BEGIN);

	GETT(MODULE,NAME);
	GETD(module_name,sizeof(module_name));

	m = module_load(module_name,0,NULL);
	if (m==NULL)
	{
		stream_error("module %s version is not found", module_name);
		return -1;
	}

	GETT(MODULE,VERSION);
	GETS(major);
	GETS(minor);

	GETT(MODULE,END);

	if (m->major!=major || m->minor!=minor)
	{
		stream_error("module %s version %d.%02d specified does not match version %d.%02d found", module_name, major, minor, m->major, m->minor);
		return -1;
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
 * UNIT
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

/*******************************************************
 * GLOBALS 
 */
int64 stream_out_global(FILE *fp, GLOBALVAR *v)
{
	int64 count=0;
	char value[4096];

	if (global_getvar(v->name,value,sizeof(value))==NULL)
		return -1;

	PUTT(GLOBAL,BEGIN);
	
	PUTT(GLOBAL,NAME);
	PUTD(v->name,strlen(v->name));

	PUTT(GLOBAL,PROPERTY);
	PUTX(property, v->prop);

	PUTT(GLOBAL,VALUE);
	PUTD(value,strlen(value));

	PUTT(GLOBAL,END);
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

				PUTT(BEGIN,PROPERTY);
				extended = TRUE;
			}

			PUTT(CLASS,PROPERTY);
			PUTX(property,prop);
		}
	}

	/* close class if extended */
	if (extended)
	{
		PUTT(END,PROPERTY)
		PUTT(CLASS,END);
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

	PUTT(OBJECT,CLASS);
	PUTD(obj->oclass->name,strlen(obj->oclass->name));

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
			object_get_value_by_addr(obj,addr,value,sizeof(value),p);
			
			PUTT(OBJECT,PROPERTY);
			PUTL((unsigned long)(p->addr));

			PUTT(OBJECT,VALUE);
			PUTD(value,strlen(value));
		}
	}

	PUTT(OBJECT,END);
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
	COMPRESS(sch,sizeof(SCHEDULE));
#else
	PUTT(SCHEDULE,NAME);
	PUTD(sch->name,strlen(sch->name));

	PUTT(SCHEDULE,DEFINITION);
	PUTD(sch->definition,strlen(sch->definition));
#endif

	PUTT(SCHEDULE,END);

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

			PUTT(OBJECT,ID);
			PUTL(obj->id);

			PUTT(TRANSFORM,PROPERTY);
			PUTL((unsigned long)(prop->addr));
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

	PUTT(TRANSFORM,OBJECT);
	PUTL(xform->target_obj->id);

	PUTT(TRANSFORM,PROPERTY);
	PUTL((unsigned long)(xform->target_prop->addr));

	PUTT(TRANSFORM,SCALE);
	PUTQ(xform->scale);

	PUTT(TRANSFORM,BIAS);
	PUTQ(xform->bias);

	PUTT(TRANSFORM,END);
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

	/* modules (not optional) */
	PUTT(BEGIN,MODULE);
	for (mod=module_get_first(); (flags&SF_MODULES) && mod!=NULL; mod=mod->next)
		PUTX(module,mod);
	PUTT(END,MODULE);

	/* globals */
	PUTT(BEGIN,MODULE);
	while ((flags&SF_GLOBALS) && (gvar=global_getnext(gvar))!=NULL)
		PUTX(global,gvar);
	PUTT(END,MODULE);

	/* classes */
	PUTT(BEGIN,CLASS);
	for (oclass=class_get_first_class(); (flags&SF_CLASSES) && oclass!=NULL; oclass=oclass->next)
		PUTX(class,oclass);
	PUTT(END,CLASS);

	/* schedules */
	PUTT(BEGIN,SCHEDULE);
	while ((sch=schedule_getnext(sch))!=NULL)
		PUTX(schedule,sch);
	PUTT(END,SCHEDULE);

	/* objects */
	PUTT(BEGIN,OBJECT);
	for (obj=object_get_first(); (flags&SF_OBJECTS) && obj!=NULL; obj=obj->next)
		PUTX(object,obj);
	PUTT(END,OBJECT);

	/* schedules */
	PUTT(BEGIN,TRANSFORM);
	while ((xform=scheduletransform_getnext(xform))!=NULL)
		PUTX(transform,xform);
	PUTT(END,TRANSFORM);

	return count;
}

int64 stream_in(FILE *fp, int flags)
{
	int64 count = 0;
	GETD(STREAM_NAME,strlen(STREAM_NAME));

	return count;
}
