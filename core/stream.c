/* $Id$
 *
 */

#include "output.h"
#include "stream.h"
#include "module.h"
#include "globals.h"

/* stream name - this should never be changed */
#define STREAM_NAME "GRIDLABD"

/* stream version - change this when the structure of the stream changes */
#define STREAM_VERSION 1

/* output macros */
static unsigned char b,t;
static unsigned char c;
static unsigned short s;
static unsigned long l;
static unsigned int64 q;
static unsigned int64 n;
#define PUTD(S,L) {n=(L);if(fwrite(&n,2,1,fp)!=1 || fwrite((S),1,L,fp)!=L) return -1; else count+=n+2;}
#define PUTC(C) {c=(C);PUTD(&c,1)}
#define PUTS(S) {s=(S);PUTD(&s,2)}
#define PUTL(L) {l=(L);PUTD(&l,4)}
#define PUTQ(Q) {q=(Q);PUTD(&q,8)}
#define PUTX(T,X) {if ((n=stream_out_##T(fp,(X)))<0) return -1; else count+=n;}
#define PUTT(B,T) {b=SB_##B;t=ST_##T;s=(unsigned short)((b<<8)|t);PUTD(&s,2)}

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

	ST_END=0xff,
};

char *stream_context()
{
	static char buffer[64];
	sprintf(buffer,"block %d, token %d",b,t);
	return buffer;
}

/*******************************************************
 * MODULES 
 */
int64 stream_out_module(FILE *fp, MODULE *m)
{
	int64 count=0;

	/* begin */
	PUTT(MODULE,BEGIN);

	/* name */
	PUTT(MODULE,NAME);
	PUTD(m->name,strlen(m->name));

	/* version */
	PUTT(MODULE,VERSION);
	PUTS(m->major);
	PUTS(m->minor);

	/* end */
	PUTT(MODULE,END);
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
			PUTQ((unsigned int64)(p->addr));
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

	PUTT(SCHEDULE,NAME);
	PUTD(sch->name,strlen(sch->name));

	PUTT(SCHEDULE,DEFINITION);
	PUTD(sch->definition,strlen(sch->definition));

	return count;
}

/*******************************************************
 * SCHEDULE
 */
int64 stream_out_transform(FILE *fp, SCHEDULEXFORM *xform)
{
	int64 count=0;

	/* TODO output the transform definition */

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
	output_fatal("stream_in not implemented");
	return -1;
}
