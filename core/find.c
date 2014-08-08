/** $Id: find.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file find.c
	@author David P. Chassin
	@addtogroup find Searching for objects
	@ingroup core

 @{
 **/

#include <ctype.h>
#include <stdio.h>
#ifdef WIN32
#	define snprintf _snprintf
#endif
#include "globals.h"
#include "output.h"
#include "find.h"
#include "aggregate.h"
#include "module.h"
#include "timestamp.h"

static FINDTYPE invar_types[] = {FT_ID, FT_SIZE, FT_CLASS, FT_PARENT, FT_RANK, FT_NAME, FT_LAT, FT_LONG, FT_INSVC, FT_OUTSVC, FT_MODULE, FT_ISA, 0};

static int compare_int(int64 a, FINDOP op, int64 b)
{
	switch (op) {
	case EQ: return a==b;
	case LT: return a<b;
	case GT: return a>b;
	case LE: return a<=b;
	case GE: return a>=b;
	case NE: return a!=b;
	default:
		output_error("compare op %d not supported on integers", op);
		/* TROUBLESHOOT
			This error is caused when an object find procedure uses a comparison operator
			that isn't allowed on a that type of property.  Make sure the property type
			and the comparison operator are compatible and try again.  If your GLM file
			isn't the cause of the problem, try reducing the complexity of the GLM file 
			you are using to isolate which module is causing the error and file a report 
			with the GLM file attached.
		 */
		return 0;
	}
}

static int compare_int64(int64 a, FINDOP op, int64 b){
	return compare_int(a, op, b);
}

static int compare_int32(int32 a, FINDOP op, int64 b)
{
	switch (op) {
	case EQ: return a==b;
	case LT: return a<b;
	case GT: return a>b;
	case LE: return a<=b;
	case GE: return a>=b;
	case NE: return a!=b;
	default:
		output_error("compare op %d not supported on integers", op);
		/* TROUBLESHOOT
			This error is caused when an object find procedure uses a comparison operator
			that isn't allowed on a that type of property.  Make sure the property type
			and the comparison operator are compatible and try again.  If your GLM file
			isn't the cause of the problem, try reducing the complexity of the GLM file 
			you are using to isolate which module is causing the error and file a report 
			with the GLM file attached.
		 */
		return 0;
	}
}

static int compare_int16(int16 a, FINDOP op, int64 b)
{
	switch (op) {
	case EQ: return a==b;
	case LT: return a<b;
	case GT: return a>b;
	case LE: return a<=b;
	case GE: return a>=b;
	case NE: return a!=b;
	default:
		output_error("compare op %d not supported on integers", op);
		/* TROUBLESHOOT
			This error is caused when an object find procedure uses a comparison operator
			that isn't allowed on a that type of property.  Make sure the property type
			and the comparison operator are compatible and try again.  If your GLM file
			isn't the cause of the problem, try reducing the complexity of the GLM file 
			you are using to isolate which module is causing the error and file a report 
			with the GLM file attached.
		 */
		return 0;
	}
}

static int compare_double(double a, FINDOP op, double b)
{
	switch (op) {
	case EQ: return a==b;
	case LT: return a<b;
	case GT: return a>b;
	case LE: return a<=b;
	case GE: return a>=b;
	case NE: return a!=b;
	default:
		output_error("compare op %d not supported on doubles", op);
		/* TROUBLESHOOT
			This error is caused when an object find procedure uses a comparison operator
			that isn't allowed on a that type of property.  Make sure the property type
			and the comparison operator are compatible and try again.  If your GLM file
			isn't the cause of the problem, try reducing the complexity of the GLM file 
			you are using to isolate which module is causing the error and file a report 
			with the GLM file attached.
		 */
		return 0;
	}
}

static int compare_string(char *a, FINDOP op, char *b)
{
	switch (op) {
	case SAME: op=EQ; goto CompareInt;
	case BEFORE: op=LT; goto CompareInt;
	case AFTER: op=GT; goto CompareInt;
	case DIFF: op=NE; goto CompareInt;
	case LIKE: return match(a, b);
	case EQ:
	case LT:
	case GT:
	case LE:
	case GE:
	case NE:
		if (!(isdigit(a[0])||a[0]=='+'||a[0]=='-')&&!(isdigit(b[0])||b[0]=='+'||b[0]=='-'))
			output_warning("compare of strings using numeric op usually does not work");
			/* TROUBLESHOOT
				An attempt to compare two numbers using a string comparison operation is not generally going to work as expected.
				Check that this is truly the desired comparison and correct it if not.  If so and the warning error is not desired
				you may suppress warnings using <code>#set warning=0</code> in the model file.
			 */
		return compare_double(atof(a),op,atof(b));
	default:
		output_error("compare op %d not supported on strings", op);
		/* TROUBLESHOOT
			This error is caused when an object find procedure uses a comparison operator
			that isn't allowed on a that type of property.  Make sure the property type
			and the comparison operator are compatible and try again.  If your GLM file
			isn't the cause of the problem, try reducing the complexity of the GLM file 
			you are using to isolate which module is causing the error and file a report 
			with the GLM file attached.
		 */
		return 0;
	}
CompareInt:
	return compare_int((int64)strcmp(a,b),op,0);

}

static int compare_property(OBJECT *obj, char *propname, FINDOP op, void *value)
{
	/** @todo comparisons should type based and not using string representation (ticket #20) */
	char buffer[1024];
	char *propval = object_property_to_string(obj,propname, buffer, 1023);
	if (propval==NULL) return 0;
	return compare_string(propval,op,(char*)value);
}

/**	Fetches the property requested and uses the appropriate op on the value.
	@return boolean value
**/
static int compare_property_alt(OBJECT *obj, char *propname, FINDOP op, void *value){
	complex *complex_target = NULL;
	char *char_target = NULL;
	int16 *int16_target = NULL;
	int32 *int32_target = NULL;
	int64 *int64_target = NULL;
	PROPERTY *prop = object_get_property(obj, propname,NULL);

	if(prop == NULL){
		/* property not found in object ~ normal operation */
		return 0;
	}

	switch(prop->ptype){
		case PT_void:
			return 0;	/* no comparsion to be made */
		case PT_double:
			break;
		case PT_complex:
			complex_target = object_get_complex(obj, prop);
			if(complex_target == NULL)
				return 0; /* error value */
			break;
		case PT_enumeration:
		case PT_set:
			break;		/* not 100% sure how to make these cooperate yet */
		case PT_int16:
			int16_target = (int16 *)object_get_int16(obj, prop);
			if(int16_target == NULL)
				return 0;
			return compare_int16(*int16_target, op, *(int64 *)value);
		case PT_int32:
			int32_target = (int32 *)object_get_int32(obj, prop);
			return compare_int32(*int32_target, op, *(int64 *)value);
			break;
		case PT_int64:
			int64_target = (int64 *)object_get_int64(obj, prop);
			return compare_int64(*int64_target, op, *(int64 *)value);
			break;
		case PT_char8:
		case PT_char32:
		case PT_char256:
		case PT_char1024:
			char_target = (char *)object_get_string(obj, prop);
			if(char_target == NULL)
				return 0;
			return compare_string(char_target, op, value);
			break;
		case PT_object:

			break;
		case PT_bool:
			break;
		case PT_timestamp:
		case PT_double_array:
		case PT_complex_array:
			break;
#ifdef USE_TRIPLETS
		case PT_triple:
		case PT_triplex:
			break;
#endif
		default:
			output_error("comparison operators not supported for property type %s", class_get_property_typename(prop->ptype));
			/* TROUBLESHOOT
				This error is caused when an object find procedure uses a comparison operator
			that isn't allowed on a that type of property.  Make sure the property type
			and the comparison operator are compatible and try again.  If your GLM file
			isn't the cause of the problem, try reducing the complexity of the GLM file 
			you are using to isolate which module is causing the error and file a report 
			with the GLM file attached.
			 */
			return 0;
	}
}

static int compare(OBJECT *obj, FINDTYPE ftype, FINDOP op, void *value, char *propname)
{
	switch (ftype) {
	case FT_ID: return compare_int((int64)obj->id,op,(int64)*(OBJECTNUM*)value);
	case FT_SIZE: return compare_int((int64)obj->oclass->size,op,(int64)*(int*)value);
	case FT_CLASS: return compare_string((char*)obj->oclass->name,op,(char*)value);
	case FT_ISA: return object_isa(obj,(char*)value);
	case FT_MODULE: return compare_string((char*)obj->oclass->module->name,op,(char*)value);
	case FT_GROUPID: return compare_string((char*)obj->groupid,op,(char*)value);
	case FT_RANK: return compare_int((int64)obj->rank,op,(int64)*(int*)value);
	case FT_CLOCK: return compare_int((int64)obj->clock,op,(int64)*(TIMESTAMP*)value);
	//case FT_PROPERTY: return compare_property_alt(obj,propname,op,value);
	case FT_PROPERTY: return compare_property(obj,propname,op,value);
	default:
		output_error("findtype %s not supported", ftype);
		/* TROUBLESHOOT
			This error is caused when an object find procedure uses a comparison operator
			that isn't allowed on a that header item.  Make sure the header item
			and the comparison operator are compatible and try again.  If your GLM file
			isn't the cause of the problem, try reducing the complexity of the GLM file 
			you are using to isolate which module is causing the error and file a report 
			with the GLM file attached.
		 */
		return 0;
	}
}

FINDLIST *new_list(unsigned int n)
{
	unsigned int size = (n>>3)+1;
	FINDLIST *list = module_malloc(sizeof(FINDLIST)+size-1);
	if (list==NULL)
	{
		errno=ENOMEM;
		return NULL;
	}
	memset(list->result,0,size);
	list->result_size = size;
	list->hit_count = 0;
	return list;
}
#define SIZE(L) ((L).result_size)
#define COUNT(L) ((L).hit_count)
#define FOUND(L,N) (((L).result[(N)>>3]&(1<<((N)&0x7)))!=0)
#define ADDOBJ(L,N) (!FOUND((L),(N))?((L).result[(N)>>3]|=(1<<((N)&0x7)),++((L).hit_count)):(L).hit_count)
#define DELOBJ(L,N) (FOUND((L),(N))?((L).result[(N)>>3]&=~(1<<((N)&0x7)),--((L).hit_count)):(L).hit_count)
#define ADDALL(L) ((L).hit_count=object_get_count(),memset((L).result,0xff,(L).result_size))
#define DELALL(L) ((L).hit_count=object_get_count(),memset((L).result,0x00,(L).result_size))

FINDLIST *find_runpgm(FINDLIST *list, FINDPGM *pgm);
FINDPGM *find_mkpgm(char *expression);

/** Search for objects that match criteria
	\p start may be a previous search result, or \p FT_NEW.
	\p FT_NEW starts a new search (starting with all objects)

	Searches criteria may be as follows:

		find_objects(\p FT_NEW, \p ftype, \p compare, \p value[, ...], \p NULL);

	and may be grouped using \p AND and \p OR.

	The criteria list must be terminated by \p NULL or \p FT_END.

	Values of \p ftype are:
		- \p FT_ID		 compares object ids (expects \e long value)
		- \p FT_SIZE	 compares object size excluding header (expects \e long value)
		- \p FT_CLASS	 compares object class name (expects \e char* value)
		- \p FT_PARENT	 uses parent for comparison (must be followed by another \p ftype)
		- \p FT_RANK	 compares object rank by number (expects \e long value)
		- \p FT_CLOCK	 compares object clock (expects \e TIMESTAMP value)
		- \p FT_PROPERTY compares property (expects \e char* value)
		- \p FT_MODULE	 compares module name
		- \p FT_ISA		 compares object class name including parent classes (expected \e char* value)

	Extended values of \p ftype are:
		- \p CF_NAME	looks for a particular name
		- \p CF_LATT	compares object lattitudes
		- \p CF_LONG	compares object longitudes
		- \p CF_INSVC	checks in-service timestamp
		- \p CF_OUTSVC	checks out-service timestamp

	Values of \p compare are:
		- \p EQ			equal
		- \p NE			not equal
		- \p LT			less than
		- \p GT			greater than
		- \p LE			less than or equal
		- \p GE			greather than or equal
		- \p NOT		opposite of test
		- \p BETWEEN	in between
		- \p SAME		same string
		- \p BEFORE	    alphabetic before
		- \p AFTER		alphabetic after
		- \p DIFF		alphabetic differ
		- \p MATCH		matches \e regex
		- \p LIKE		matches \e regex
		- \p UNLIKE		matches "not" \e regex

	Conjunctions are \e AND and \e OR, and can be used to do complex searches

	OBJECT *find_first(FINDLIST *list) returns the first object in the result list

	OBJECT *find_next(FINDLIST *list, OBJECT *previous) returns the next object in the result list

	@return a pointer for FINDLIST structure used by find_first(), find_next, and find_makearray(), will return NULL if an error occurs.
**/
FINDLIST *find_objects(FINDLIST *start, ...)
{	
	int ival;
	char *sval;
	OBJECTNUM oval;
	TIMESTAMP tval;
	double rval;
	OBJECT *obj;
	FINDLIST *result = start;
	if (start==FL_NEW || start==FL_GROUP)
	{
		result=new_list(object_get_count());
		ADDALL(*result);
	}
	/* FL_GROUP is something of an interrupt option that constructs a program by parsing string input. */
	if (start==FL_GROUP)
	{
		FINDPGM *pgm;
		va_list(ptr);
		va_start(ptr,start);
		pgm = find_mkpgm(va_arg(ptr,char*));
		if (pgm!=NULL){
			return find_runpgm(result,pgm);
		} else {
			va_end(ptr);
			DELALL(*result); /* pgm == NULL */
			return result;
		}
	}
	/* if we're not using FL_GROUP, we break apart the va_arg list, taking data inputs in the "correct" type. */
	for (obj=object_get_first(); obj!=NULL; obj=obj->next)
	{
		FINDTYPE ftype;
		va_list(ptr);
		va_start(ptr,start);
		while ((ftype=va_arg(ptr,FINDTYPE)) != FT_END)
		{
			int invert=0;
			int parent=0;
			FINDOP conj=AND;
			FINDOP op;
			char *propname = NULL;
			void *value;
			OBJECT *target=obj;

			/* conjunction */
			if (ftype==AND || ftype==OR)
			{	/* expect another op */
				conj=ftype;
				ftype = va_arg(ptr, FINDTYPE);
			}

			/* follow to parent */
			while (ftype==FT_PARENT)
			{
				ftype=va_arg(ptr,FINDTYPE);
				parent++;
			}

			/* property option require property name */
			if (ftype==FT_PROPERTY)
				propname=va_arg(ptr,char*);

			/* read operation */
			op = va_arg(ptr,FINDOP);

			/* negation */
			if (op==NOT)
			{	/* expect another op */
				invert=1;
				op = va_arg(ptr,FINDOP);
			}

			/* read value */
			switch (ftype) {
			case FT_PARENT:
			case FT_ID:
				oval = va_arg(ptr,OBJECTNUM);
				value = &oval;
				break;
			case FT_SIZE:
			case FT_RANK:
				ival = va_arg(ptr,int);
				value = &ival;
				break;
			case FT_INSVC:
			case FT_OUTSVC:
			case FT_CLOCK:
				tval = va_arg(ptr,TIMESTAMP);
				value = &tval;
				break;
			case FT_LAT:
			case FT_LONG:
				rval = va_arg(ptr, double);
				value = &rval;
				break;
			case FT_CLASS:
			case FT_NAME:
			case FT_PROPERTY:
			case FT_MODULE:
			case FT_GROUPID:
			case FT_ISA:
				sval = va_arg(ptr,char*);
				value = sval;
				break;
			default:
				output_error("invalid findtype means search is specified incorrectly or FT_END is probably missing");
				/*	TROUBLESHOOT
					The search rule is invalid because it contains a term that isn't recognized or the 
					search rule is not correctly terminated by the FT_END term.  Check the syntax of
					the search rule and try again.  If the search rule comes from a module, report the 
					problem.
				 */
				if (start == FL_NEW) module_free(result);
				return NULL;
			}

			/* follow target to parent */
			while (parent-- > 0 && target != NULL)
				target = target->parent;

			/* if target exists */
			if (target != NULL)
			{
				/* match */
				if (compare(target,ftype,op,value,propname)!=invert)
				{
					if (conj==OR) 
						ADDOBJ(*result,obj->id);
				}
				else
				{
					if (conj==AND) 
						DELOBJ(*result,obj->id);
				}
			}
		}
		va_end(ptr);
	}
	return result;
}

/** Makes an array from the objects currently in
	the search list result.
	@return The number of objects found
 **/
int find_makearray(FINDLIST *list, /**< the search list to scan */
				   OBJECT ***objs) /**< the object found */
{
	OBJECT *obj=find_first(list);
	int n = list->hit_count, i;
	if (n<=0 || obj==NULL)
		return 0;
	(*objs) = (OBJECT**)module_malloc(sizeof(OBJECT*)*(n+1)); /* one extra for handy NULL to terminate list */
	for ( i=0 ; i<n && obj!=NULL; obj=find_next(list,obj),i++)
		(*objs)[i] = obj;
	objs[n]=NULL;/* NULL to terminate list */
	return n;
}

/** Returns the first object currently in the
	the search list return
	@return a pointer to the first object
 **/
OBJECT *find_first(FINDLIST *list) /**< the search list to scan */
{
	return find_next(list,NULL);
}

/** Return the next object in the currenet search list
	@return a pointer to the next object
 **/
OBJECT *find_next(FINDLIST *list, /**< the search list to scan */
				  OBJECT *obj) /**< the current object */
{
	if (obj==NULL)
		obj = object_get_first();
	else
		obj = obj->next;
	while (obj!=NULL && !FOUND(*list,obj->id))
		obj=obj->next;

	return obj;
}

/**************************************************************
 * FIND EXPRESSIONS AND FIND MACHINES
 **************************************************************/

#define VALUE(X,T,M) ((X).isconstant?(X).value.constant.M:*((X).value.ref.T))

int compare_pointer_eq(void *a, FINDVALUE b) { return *(void**)a==b.pointer;}
int compare_pointer_ne(void *a, FINDVALUE b) { return *(void**)a!=b.pointer;}
int compare_pointer_lt(void *a, FINDVALUE b) { return *(void**)a<b.pointer;}
int compare_pointer_gt(void *a, FINDVALUE b) { return *(void**)a>b.pointer;}
int compare_pointer_le(void *a, FINDVALUE b) { return *(void**)a<=b.pointer;}
int compare_pointer_ge(void *a, FINDVALUE b) { return *(void**)a>=b.pointer;}

int compare_integer16_eq(void *a, FINDVALUE b) { return *(int16*)a==(int16)b.integer;}
int compare_integer16_ne(void *a, FINDVALUE b) { return *(int16*)a!=(int16)b.integer;}
int compare_integer16_lt(void *a, FINDVALUE b) { return *(int16*)a<(int16)b.integer;}
int compare_integer16_gt(void *a, FINDVALUE b) { return *(int16*)a>(int16)b.integer;}
int compare_integer16_le(void *a, FINDVALUE b) { return *(int16*)a<=(int16)b.integer;}
int compare_integer16_ge(void *a, FINDVALUE b) { return *(int16*)a>=(int16)b.integer;}

int compare_integer32_eq(void *a, FINDVALUE b) { return *(int32*)a==(int32)b.integer;}
int compare_integer32_ne(void *a, FINDVALUE b) { return *(int32*)a!=(int32)b.integer;}
int compare_integer32_lt(void *a, FINDVALUE b) { return *(int32*)a<(int32)b.integer;}
int compare_integer32_gt(void *a, FINDVALUE b) { return *(int32*)a>(int32)b.integer;}
int compare_integer32_le(void *a, FINDVALUE b) { return *(int32*)a<=(int32)b.integer;}
int compare_integer32_ge(void *a, FINDVALUE b) { return *(int32*)a>=(int32)b.integer;}

int compare_integer64_eq(void *a, FINDVALUE b) { return *(int64*)a==(int64)b.integer;}
int compare_integer64_ne(void *a, FINDVALUE b) { return *(int64*)a!=(int64)b.integer;}
int compare_integer64_lt(void *a, FINDVALUE b) { return *(int64*)a<(int64)b.integer;}
int compare_integer64_gt(void *a, FINDVALUE b) { return *(int64*)a>(int64)b.integer;}
int compare_integer64_le(void *a, FINDVALUE b) { return *(int64*)a<=(int64)b.integer;}
int compare_integer64_ge(void *a, FINDVALUE b) { return *(int64*)a>=(int64)b.integer;}

int compare_integer_eq(void *a, FINDVALUE b) { return *(int32*)a==(int32)b.integer;}
int compare_integer_ne(void *a, FINDVALUE b) { return *(int32*)a!=(int32)b.integer;}
int compare_integer_lt(void *a, FINDVALUE b) { return *(int32*)a<(int32)b.integer;}
int compare_integer_gt(void *a, FINDVALUE b) { return *(int32*)a>(int32)b.integer;}
int compare_integer_le(void *a, FINDVALUE b) { return *(int32*)a<=(int32)b.integer;}
int compare_integer_ge(void *a, FINDVALUE b) { return *(int32*)a>=(int32)b.integer;}

int compare_real_eq(void *a, FINDVALUE b) { return *(double*)a==b.real;}
int compare_real_ne(void *a, FINDVALUE b) { return *(double*)a!=b.real;}
int compare_real_lt(void *a, FINDVALUE b) { return *(double*)a<b.real;}
int compare_real_gt(void *a, FINDVALUE b) { return *(double*)a>b.real;}
int compare_real_le(void *a, FINDVALUE b) { return *(double*)a<=b.real;}
int compare_real_ge(void *a, FINDVALUE b) { return *(double*)a>=b.real;}

int compare_isa(void *a, FINDVALUE b) { return object_isa((OBJECT*)a,b.string); }

/* NOTE: this only works with short-circuiting logic! */
int compare_string_eq(void *a, FINDVALUE b) {
	int one = (char **)a != NULL;
	int two = strcmp((char*)a,b.string)==0;
	return (char *)a != NULL && strcmp((char*)a,b.string)==0;
}
int compare_string_ne(void *a, FINDVALUE b) { return *(char **)a != NULL && strcmp(*(char**)a,b.string)!=0;}
int compare_string_lt(void *a, FINDVALUE b) { return *(char **)a != NULL && strcmp(*(char**)a,b.string)<0;}
int compare_string_gt(void *a, FINDVALUE b) { return *(char **)a != NULL && strcmp(*(char**)a,b.string)>0;}
int compare_string_le(void *a, FINDVALUE b) { return *(char **)a != NULL && strcmp(*(char**)a,b.string)<=0;}
int compare_string_ge(void *a, FINDVALUE b) { return *(char **)a != NULL && strcmp(*(char**)a,b.string)>=0;}

int compare_pointer_li(void *a, FINDVALUE b) {return 0;}

int compare_integer_li(void *a, FINDVALUE b) {
	char temp[256];
	if(convert_from_int64(temp, 256, &(b.integer), NULL))
		return match(*(char **)a, temp);
	return 0;
}

int compare_real_li(void *a, FINDVALUE b) {
	char temp[256];
	if(convert_from_double(temp, 256, &(b.real), NULL))
		return match(*(char **)a, temp);
	return 0;
}

int compare_string_li(void *a, FINDVALUE b) {return match(*(char **)a, b.string);}

int compare_integer16_li(void *a, FINDVALUE b) {
	char temp[256];
	if(convert_from_int16(temp, 256, &(b.integer), NULL))
		return match(*(char **)a, temp);
	return 0;
}

int compare_integer32_li(void *a, FINDVALUE b) {
	char temp[256];
	if(convert_from_int32(temp, 256, &(b.integer), NULL))
		return match(*(char **)a, temp);
	return 0;
}

int compare_integer64_li(void *a, FINDVALUE b) {
	char temp[256];
	if(convert_from_int64(temp, 256, &(b.integer), NULL))
		return match(*(char **)a, temp);
	return 0;
}

int compare_pointer_nl(void *a, FINDVALUE b) {return 1;}

int compare_integer_nl(void *a, FINDVALUE b) {
	char temp[256];
	if(convert_from_int64(temp, 256, &(b.integer), NULL))
		return 1 != match(*(char **)a, temp);
	return 0;
}

int compare_real_nl(void *a, FINDVALUE b) {
	char temp[256];
	if(convert_from_double(temp, 256, &(b.real), NULL))
		return 1 != match(*(char **)a, temp);
	return 0;
}

int compare_string_nl(void *a, FINDVALUE b) {
	return 1 != match(*(char **)a, b.string);
}

int compare_integer16_nl(void *a, FINDVALUE b) {
	char temp[256];
	if(convert_from_int16(temp, 256, &(b.integer), NULL))
		return 1 != match(*(char **)a, temp);
	return 0;
}

int compare_integer32_nl(void *a, FINDVALUE b) {
	char temp[256];
	if(convert_from_int32(temp, 256, &(b.integer), NULL))
		return 1 != match(*(char **)a, temp);
	return 0;
}

int compare_integer64_nl(void *a, FINDVALUE b) {
	char temp[256];
	if(convert_from_int64(temp, 256, &(b.integer), NULL))
		return 1 != match(*(char **)a, temp);
	return 0;
}

/** Make a copy of a findlist.  This is necessary
	because find_object changes the list it's given
	so if you want to run a search repeatedly, you
	need to work on a copy.  Don't forget to free
	the copy when you're done using it.
	@return the copy
 **/
FINDLIST *findlist_copy(FINDLIST *list)
{
	unsigned int size = sizeof(FINDLIST)+(list->result_size>>3);
	FINDLIST *new_list = module_malloc(size);
	memcpy(new_list,list,size);
	return new_list;
}

void findlist_add(FINDLIST *list, OBJECT *obj)
{
	ADDOBJ(*list,obj->id);
}
void findlist_del(FINDLIST *list, OBJECT *obj)
{
	DELOBJ(*list,obj->id);
}
void findlist_clear(FINDLIST *list)
{
	DELALL(*list);
}

static void findlist_nop(FINDLIST *list, OBJECT *obj)
{
}

PGMCONSTFLAGS find_pgmconstants(FINDPGM *pgm)
{
	if (pgm==NULL)
		return 0;

	/* find the end of the program */
	while (pgm->next!=NULL) pgm=pgm->next;
	return pgm->constflags;

}

static FINDPGM *add_pgm(FINDPGM **pgm, COMPAREFUNC op, unsigned short target, FINDVALUE value, FOUNDACTION pos, FOUNDACTION neg)
{
	/* create program entry */
	FINDPGM *item = (FINDPGM*)malloc(sizeof(FINDPGM));
	if (item!=NULL)
	{
		item->constflags = CF_CONSTANT; /* initially the result is invariant */
		item->op = op;
		item->target = target;
		item->value = value;
		item->pos = pos;
		item->neg = neg;
		item->next = NULL;

		/* attach to existing program */
		if (*pgm!=NULL)
		{
			FINDPGM *tail = *pgm;

			item->constflags = tail->constflags; /* inherit flags from previous result set */

			/* find tail of existing program */
			while (tail->next!=NULL) tail=tail->next;
			tail->next = item;
		}
		else
			*pgm = item;
	}
	else
		errno = ENOMEM;

	return item;
}

/** Runs a search engine built by find_mkpgm **/
FINDLIST *find_runpgm(FINDLIST *list, FINDPGM *pgm)
{
	if (list==NULL)
	{
		list=new_list(object_get_count());
		ADDALL(*list);
	}
	if (pgm!=NULL)
	{
		OBJECT *obj;
		for (obj=find_first(list); obj!=NULL; obj=find_next(list,obj))
		{
			if ((*pgm->op)((void*)(((char*)obj)+pgm->target),pgm->value))
			{	if (pgm->pos) (*pgm->pos)(list,obj); }
			else
			{	if (pgm->neg) (*pgm->neg)(list,obj); }
		}
		find_runpgm(list,pgm->next);
	}
	return list;
}

#define PARSER char *_p
#define START int _m=0, _n=0;
#define ACCEPT { _n+=_m; _p+=_m; _m=0; }
#define HERE (_p+_m)
#define OR {_m=0;}
#define REJECT { return 0; }
#define WHITE (_m+=white(HERE))
#define LITERAL(X) ((_m+=literal(HERE,(X)))>0)
#define TERM(X) ((_m+=(X))>0)
#define COPY(X) {size--; (X)[_n++]=*_p++;}
#define DONE return _n;

void syntax_error(char *p)
{
	char context[16], *nl;
	strncpy(context,p,15);
	nl = strchr(context,'\n');
	if (nl!=NULL) *nl='\0'; else context[15]='\0';
	if (strlen(context)>0)
		output_message("find expression syntax error at '%s...'", context);
	else
		output_message("find expression syntax error");
}

static int white(PARSER)
{
	if (*_p=='\0' || !isspace(*_p))
		return 0;
	/* tail recursion to keep consuming whitespace until none left*/
	return white(_p+1)+1;
}

static int comment(PARSER)
{
	int _n = white(_p);
	if (_p[_n]=='#')
	{
		while (_p[_n]!='\n')
			_n++;
	}
	return _n;
}

static int literal(PARSER, char *text)
{
	if (strncmp(_p,text,strlen(text))==0)
		return (int)strlen(text);
	return 0;
}

static int name(PARSER, char *result, int size)
{	/* basic name */
	START;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}

static int value(PARSER, char *result, int size)
{	/* everything to a semicolon */
	START;
	while (size>1 && *_p!='\0' && *_p!=';' && *_p!='\n') COPY(result);
	result[_n]='\0';
	return _n;
}

static int token(PARSER, char *result, int size)
{	/* everything to a semicolon */
	START;
	while (size>1 && *_p!='\0' && *_p!=';' && *_p!='\n' && !isspace(*_p) ) COPY(result);
	result[_n]='\0';
	return _n;
}

static int integer(PARSER, int64 *value)
{
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi64(result);
	return _n;
}

static int _real(PARSER, double *value)
{
	char result[256];
	int size=sizeof(result);
	START;
	if (*_p=='+' || *_p=='-') COPY(result);
	while (size>1 && isdigit(*_p)) COPY(result);
	if (*_p=='.') COPY(result);
	while (size>1 && isdigit(*_p)) COPY(result);
	if (*_p=='E' || *_p=='e') COPY(result);
	if (*_p=='+' || *_p=='-') COPY(result);
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atof(result);
	return _n;
}

static int time_value_seconds(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("s")) { *t *= TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("S")) { *t *= TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

static int time_value_minutes(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("m")) { *t *= 60*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("M")) { *t *= 60*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

static int time_value_hours(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("h")) { *t *= 3600*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("H")) { *t *= 3600*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

static int time_value_days(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("d")) { *t *= 86400*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("D")) { *t *= 86400*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

static int time_value_datetime(PARSER, TIMESTAMP *t)
{
	int64 Y,m,d,H,M,S;
	START;
	if WHITE ACCEPT;
	if ( (LITERAL("'")||LITERAL("\""))
		&& TERM(integer(HERE,&Y)) && LITERAL("-")
		&& TERM(integer(HERE,&m)) && LITERAL("-")
		&& TERM(integer(HERE,&d)) && LITERAL(" ")
		&& TERM(integer(HERE,&H)) && LITERAL(":")
		&& TERM(integer(HERE,&M)) && LITERAL(":")
		&& TERM(integer(HERE,&S)) && (LITERAL("'")||LITERAL("\"")))
	{
		DATETIME dt = {(unsigned short)Y,(unsigned short)m,(unsigned short)d,(unsigned short)H,(unsigned short)M,(unsigned short)S};
		TIMESTAMP tt = mkdatetime(&dt);
		if (tt!=TS_INVALID) {*t=(int64)tt*TS_SECOND; ACCEPT; DONE;}
	}
	REJECT;
}

static int time_value(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if TERM(time_value_seconds(HERE,t)) {ACCEPT; DONE; }
	OR
	if TERM(time_value_minutes(HERE,t)) {ACCEPT; DONE; }
	OR
	if TERM(time_value_hours(HERE,t)) {ACCEPT; DONE; }
	OR
	if TERM(time_value_days(HERE,t)) {ACCEPT; DONE; }
	OR
	if TERM(time_value_datetime(HERE,t)) {ACCEPT; DONE; }
	OR
		if TERM(integer(HERE,t)) { ACCEPT; DONE; }
	else REJECT;
	DONE;
}

static int compare_op(PARSER, FINDOP *op)
{
	/* these first! */
	if (strncmp(_p,"!=", 2)==0) {*op=NE; return 2;}
	if (strncmp(_p,"<=", 2)==0) {*op=LE; return 2;}
	if (strncmp(_p,">=", 2)==0) {*op=GE; return 2;}
	if (strncmp(_p,"!~", 2)==0) {*op=UNLIKE; return 2;}

	if (strncmp(_p,"=", 1)==0) {*op=EQ; return 1;}
	if (strncmp(_p,"<", 1)==0) {*op=LT; return 1;}
	if (strncmp(_p,">", 1)==0) {*op=GT; return 1;}
	if (strncmp(_p,"~", 1)==0) {*op=LIKE; return 1;}

	if (strncmp(_p,":", 1)==0) {*op=ISA; return 1;}
	return 0;
}

struct {
	COMPAREFUNC pointer, integer, real, string;
} comparemap[] = /** @todo add other integer sizes  (ticket #23) */
{	{compare_pointer_eq, compare_integer_eq, compare_real_eq, compare_string_eq}, // EQ=0
	{compare_pointer_lt, compare_integer_lt, compare_real_lt, compare_string_lt}, // LT=1
	{compare_pointer_gt, compare_integer_gt, compare_real_gt, compare_string_gt}, // GT=2
	{compare_pointer_ne, compare_integer_ne, compare_real_ne, compare_string_ne}, // NE=3
	{compare_pointer_le, compare_integer_le, compare_real_le, compare_string_le}, // LE=4
	{compare_pointer_ge, compare_integer_ge, compare_real_ge, compare_string_ge}, // GE=5
	{compare_pointer_li, compare_integer_li, compare_real_li, compare_string_li}, // 
	{compare_pointer_nl, compare_integer_nl, compare_real_nl, compare_string_nl},
};

struct {
	COMPAREFUNC pointer, integer, real, string, int_16, int_32, int_64;	/*	int_size to avoid #define int64 (platform.h) */
} comparemap_ext[] =
{	{compare_pointer_eq, compare_integer_eq, compare_real_eq, compare_string_eq, compare_integer16_eq, compare_integer32_eq, compare_integer64_eq},
	{compare_pointer_lt, compare_integer_lt, compare_real_lt, compare_string_lt, compare_integer16_lt, compare_integer32_lt, compare_integer64_lt},
	{compare_pointer_gt, compare_integer_gt, compare_real_gt, compare_string_gt, compare_integer16_gt, compare_integer32_gt, compare_integer64_gt},
	{compare_pointer_ne, compare_integer_ne, compare_real_ne, compare_string_ne, compare_integer16_ne, compare_integer32_ne, compare_integer64_ne},
	{compare_pointer_le, compare_integer_le, compare_real_le, compare_string_le, compare_integer16_le, compare_integer32_le, compare_integer64_le},
	{compare_pointer_ge, compare_integer_ge, compare_real_ge, compare_string_ge, compare_integer16_ge, compare_integer32_ge, compare_integer64_ge},
	{compare_pointer_li, compare_integer_li, compare_real_li, compare_string_li, compare_integer16_li, compare_integer32_li, compare_integer64_li},
	{compare_pointer_nl, compare_integer_nl, compare_real_nl, compare_string_nl, compare_integer16_nl, compare_integer32_nl, compare_integer64_nl},
};

/* int compare_integer32_eq(void *a, FINDVALUE b) { return *(int32*)a==(int32)b.integer;} */

static int expression(PARSER, FINDPGM **pgm)
{
	char32 pname;
	char256 pvalue;
	FINDOP op = EQ;
	START;
	if WHITE ACCEPT;
	if (TERM(name(HERE,pname,sizeof(pname))) && WHITE,TERM(compare_op(HERE,&op)) && WHITE,TERM(token(HERE,pvalue,sizeof(pvalue))))
	{
		/* NOTE: this will seg fault if op's value is an invalid index! */
		OBJECT _t;
#define OFFSET(X) (unsigned short)((char*)&(_t.X) - (char*)&_t)
		if WHITE ACCEPT;
		if(op < 0 || op >= FINDOP_END){
			output_error("expression(): invalid find op!");
			/*	TROUBLESHOOT
				A search rule used a term that isn't valid.  
				Check the search rule syntax and try again.
			 */
			REJECT;
		}
		if (strcmp(pname,"class")==0)
		{
			FINDVALUE v;
			CLASS *oclass = class_get_class_from_classname(pvalue);
			if (oclass==NULL)
				output_error("class '%s' not found", pvalue);
				/*	TROUBLESHOOT
					A search rule specified a class that doesn't exist.  
					Check the class name to make sure it exists and try again.
				 */
			else
			{
				v.pointer=(void*)oclass;
				add_pgm(pgm,comparemap[op].pointer,OFFSET(oclass),v,NULL,findlist_del);
				(*pgm)->constflags |= CF_CLASS; /* this will always reduce in a set class of fixed class, leaving it invariant if already so */
				ACCEPT;	DONE;
			}
		}
		if (strcmp(pname,"isa")==0)
		{
			FINDVALUE v;
			CLASS *oclass = class_get_class_from_classname(pvalue);
			if (oclass==NULL)
				output_error("class '%s' not found", pvalue);
				/*	TROUBLESHOOT
					A search rule specified a class that doesn't exist.  
					Check the class name to make sure it exists and try again.
				 */
			else
			{
				v.pointer=(void*)oclass;
				add_pgm(pgm,compare_isa,OFFSET(oclass),v,NULL,findlist_del);
				(*pgm)->constflags |= CF_CLASS; /* this will always reduce in a set class of fixed class, leaving it invariant if already so */
				ACCEPT;	DONE;
			}
		}
		if (strcmp(pname,"groupid")==0)
		{
			FINDVALUE v;
			strcpy(v.string, pvalue);
			//printf("find(): v.string=\"%s\", pvalue=\"%s\"\n", v.string, pvalue);
			add_pgm(pgm, comparemap[op].string, OFFSET(groupid), v, NULL, findlist_del);
			(*pgm)->constflags |= CF_NAME;
			ACCEPT;
			DONE;
		}
		if (strcmp(pname,"module")==0)
		{
			FINDVALUE v;
			MODULE *mod = module_find(pvalue);
			if (mod==NULL)
				output_error("module '%s' not found", pvalue);
				/* TROUBLESHOOT
					A search rule specified a module that hasn't been loaded.
					Check the module name to make sure it's been loaded and try again.
				 */
			else
			{
				v.pointer=(void*)mod;
				add_pgm(pgm,comparemap[op].pointer,OFFSET(oclass),v,NULL,findlist_del);
				(*pgm)->constflags |= CF_MODULE; 
				ACCEPT;	DONE;
			}
		}
		else if (strcmp(pname,"id")==0)
		{
			FINDVALUE v;
			int idnum = atoi(pvalue);
			if(idnum < 0){
				output_error("object id %d not found", idnum);
				/* TROUBLESHOOT 
					A search rule specified an object id that doesn't exist.
					Check the object list to make it exists and try again.
				 */
			} else {
				v.integer = idnum;
				add_pgm(pgm,comparemap[op].integer,OFFSET(id),v,NULL,findlist_del);
				(*pgm)->constflags |= CF_ID;
				ACCEPT;
				DONE;
			}
		}
		else if (strcmp(pname, "name") == 0)
		{
			/* Accept implicitly.  If it's bad, it's bad. -MH */
			FINDVALUE v;
			strcpy(v.string, pvalue);
			add_pgm(pgm, comparemap[op].string, OFFSET(name), v, NULL, findlist_del);
			(*pgm)->constflags |= CF_NAME;
			ACCEPT;
			DONE;
		}
		else if (strcmp(pname,"parent")==0)
		{
			FINDVALUE v;
			OBJECT *parent = object_find_name(pvalue);
			if (parent==NULL && strcmp(pvalue, "root") != 0 && strcmp(pvalue, "ROOT") != 0)
				output_error("parent '%s' not found", pvalue);
				/* TROUBLESHOOT 
					A search rule specified a parent that isn't defined.
					Check the parent name and try again.
				 */
			else
			{
				v.pointer = (void*)parent;
				add_pgm(pgm,comparemap[op].pointer,OFFSET(parent),v,NULL,findlist_del);
				(*pgm)->constflags |= CF_PARENT;
				ACCEPT; DONE;
			}
		}
		else if (strcmp(pname,"rank")==0)
		{
			FINDVALUE v;
			int rank = atoi(pvalue);
			if (rank<0)
				output_error("rank %s is invalid", pvalue);
				/* TROUBLESHOOT
					A search rule specified an object rank that is negative.
					Make sure the rank is zero or positive and try again.
				 */
			else
			{
				v.integer = rank;
				add_pgm(pgm,comparemap[op].integer,OFFSET(rank),v,NULL,findlist_del);
				(*pgm)->constflags |= CF_RANK;
				ACCEPT; DONE;
			}
		}
		else if (strcmp(pname, "latitude") == 0)
		{
			/* mostly borrowed from load.c */
			FINDVALUE v;
			int32 d = 0, m = 0;
			double s = 0.0, val = 0.0;
			char ns;
			if (sscanf(pvalue, "%d%c%d'%lf\"", &d, &ns, &m, &s) > 0) /* 12N34'56" */
			{
				val = (double)d+(double)m/60+s/3600;
				if (val >= 0 && val <= 90)
				{
					if (ns=='S')
						val = -val;
					else if (ns=='N')
						;
					else
						REJECT;
				} else {
					REJECT;
				}
				v.real = val;
				add_pgm(pgm, comparemap[op].real, OFFSET(latitude), v, NULL, findlist_del);
				(*pgm)->constflags |= CF_LAT;
				ACCEPT; DONE;
			}
		}
		else if (strcmp(pname, "longitude") == 0)
		{
			/* mostly borrowed from load.c */
			FINDVALUE v;
			int32 d = 0, m = 0;
			double s = 0.0, val = 0.0;
			char ns;
			if (sscanf(pvalue, "%d%c%d'%lf\"", &d, &ns, &m, &s) > 0) /* 12N34'56" */
			{
				val = (double)d+(double)m/60+s/3600;
				if (val >= 0 && val <= 180)
				{
					if (ns=='W')
						val = -val;
					else if (ns=='E')
						;
					else
						REJECT;
				} else {
					REJECT;
				}
				v.real = val;
				add_pgm(pgm, comparemap[op].real, OFFSET(longitude), v, NULL, findlist_del);
				(*pgm)->constflags |= CF_LONG;
				ACCEPT; DONE;
			}
		}
		else if (strcmp(pname, "clock") == 0)
		{
			FINDVALUE v;
			v.integer = convert_to_timestamp(pvalue);
			if(v.integer == TS_NEVER)
				REJECT;
			add_pgm(pgm, comparemap[op].integer, OFFSET(clock), v, NULL, findlist_del);
			(*pgm)->constflags |= CF_CLOCK;
			ACCEPT; DONE;
		}
		else if ( strcmp(pname,"insvc")==0 || strcmp(pname,"in")==0 )	/* format? */
		{
			FINDVALUE v;
			v.integer = convert_to_timestamp(pvalue);
			printf("find insvc=%i\n", v.integer);
			if(v.integer == TS_NEVER)
				REJECT;
			add_pgm(pgm, comparemap[op].integer, OFFSET(in_svc), v, NULL, findlist_del);
			(*pgm)->constflags |= CF_INSVC;
			ACCEPT; DONE;
		}
		else if ( strcmp(pname,"outsvc")==0 || strcmp(pname,"out")==0 )	/* format? */
		{
			FINDVALUE v;
			v.integer = convert_to_timestamp(pvalue);
			if(v.integer == TS_NEVER)
				REJECT;
			add_pgm(pgm, comparemap[op].integer, OFFSET(out_svc), v, NULL, findlist_del);
			(*pgm)->constflags |= CF_OUTSVC;
			ACCEPT; DONE;
		}
		else if (strcmp(pname, "flags") == 0)
		{
			/** @todo support searches on flags (PLC, lock) */
			/* still need to think about how to input flags without hardcoding the flag name. -mh */
			output_error("find expression on %s not supported", pname);
			/* TROUBLESHOOT
				A search criteria attempted to search on the flags of an object, which is supported yet.
				Remove the "flags" criteria and try again.
			 */
		}
		else
			output_error("find expression refers to unknown or unsupported property '%s'", pname);
			/* TROUBLESHOOT
				A search criteria used an expression that isn't recognized.  
				Fix the search rule and try again.
			 */
	}
	REJECT;
}

static int expression_list(PARSER, FINDPGM **pgm)
{
	START;
	if TERM(expression(HERE,pgm)) ACCEPT;
	if (WHITE,LITERAL(";") && TERM(expression_list(HERE,pgm))) { ACCEPT; DONE; }
	if (WHITE,LITERAL("AND") && TERM(expression_list(HERE,pgm))) {ACCEPT; DONE; }
	if (WHITE,LITERAL("and") && TERM(expression_list(HERE,pgm))) {ACCEPT; DONE; }
#if 0
	if (WHITE,LITERAL("OR"){
		FINDPGM *newpgm;
		if(TERM(expression_list(HERE,&newpgm))){
			ACCEPT;
			DONE;
		}
	}
#endif
	DONE;
}

/** Constructs a search engine for find_objects **/
FINDPGM *find_mkpgm(char *search)
{
	STATUS status=FAILED;
	FINDPGM *pgm = NULL;
	char *p = search;
	while (*p!='\0')
	{
		int move = expression_list(p,&pgm);
		if (move==0)
			break;
		p+=move;
	}
	return pgm;
}

/** Search for a file in the specified path
	(or in \p GLPATH environment variable)
	access flags: 0 = exist only, 2 = write, 4 = read, 6 = read/write
	@return the first occurance of the file
	having the desired access mode
 **/
char *find_file(char *name, /**< the name of the file to find */
				char *path, /**< the path to search (or NULL to search the GLPATH environment) */
				int mode, /**< the file access mode to use, see access() for valid modes */
				char *buffer, /**< the buffer into which the full path is written */
				int len) /**< the len of the buffer */
{
	char filepath[1024];
	char tempfp[1024];
	char envbuf[1024];
	char *glpath;
	char *dir;

#ifdef WIN32
#	define delim ";"
#	define pathsep "\\"
#else
#	define delim ":"
#	define pathsep "/"
#endif

	if(path == 0){
		glpath = getenv("GLPATH");
	} else {
		glpath = path;
	}

	/*
	CAVEAT
		Access() is a potential security hole and should never be used.
		-HMUG man page
	 */
	/* The alternative is to use stat(), which will not report whether
		the user is able to access the requested modes for the specified
		file. -mhauer */
	if(access(name, mode) == 0)
	{
		strncpy(buffer, name, len);
		return buffer;
	}

	/* locate unit file on GLPATH if not found locally */
	if(glpath != 0){
		strncpy(envbuf, glpath, sizeof(envbuf));
		dir = strtok(envbuf, delim);
		while (dir)
		{
			snprintf(filepath, sizeof(filepath), "%s%s%s", dir, pathsep, name);
			if (!access(filepath,mode))
			{
				strncpy(buffer,filepath,len);
				return buffer;
			}
			dir = strtok(NULL, delim);
		}
	}

#ifdef WIN32
	if(module_get_exe_path(filepath, 1024)){
		snprintf(tempfp, sizeof(tempfp), "%s%s", filepath, name);
		if(access(tempfp, mode) == 0)
		{
			strncpy(buffer,tempfp,len);
			return buffer;
		}
		snprintf(tempfp, sizeof(tempfp), "%setc\\%s", filepath, name);
		if(access(tempfp, mode) == 0)
		{
			strncpy(buffer,tempfp,len);
			return buffer;
		}
		snprintf(tempfp, sizeof(tempfp), "%slib\\%s", filepath, name);
		if(access(tempfp, mode) == 0)
		{
			strncpy(buffer,tempfp,len);
			return buffer;
		}
	}
#else
	snprintf(tempfp, sizeof(tempfp), "/usr/local/lib/gridlabd/%s", name);
	if(access(tempfp, mode) == 0)
	{
		strncpy(buffer,tempfp,len);
		return buffer;
	}
	snprintf(tempfp, sizeof(tempfp), "/usr/local/share/gridlabd/%s", name);
	if(access(tempfp, mode) == 0)
	{
		strncpy(buffer,tempfp,len);
		return buffer;
	}
#endif
	return NULL;
}

/***********************************************************************************
 * Object list handling (new in V3.0)
 ***********************************************************************************/
#define INITSIZE 256

OBJLIST *objlist_create(CLASS *oclass, PROPERTY *match_property, char *part, char *match_op, void *match_value1, void *match_value2 )
{
	OBJLIST *list = malloc(sizeof(OBJLIST));
	
	/* check parameters */
	if ( !list ) return output_error("find_create(): memory allocation failed"),NULL;
	
	/* setup object list structure */
	list->asize = INITSIZE;
	list->size = 0;
	list->objlist = malloc(sizeof(OBJECT*)*INITSIZE);
	if ( !list->objlist ) return output_error("find_create(): memory allocation failed"),free(list),NULL;
	list->oclass = oclass;

	/* perform search */
	objlist_add(list,match_property,part,match_op,match_value1,match_value2);
	return list;
}
OBJLIST *objlist_search(char *group)
{
	FINDLIST *result;
	OBJECT *obj;
	OBJLIST *list;
	int n;
	FINDPGM *pgm = find_mkpgm(group);
	
	// a null group  should return all objects
	if ( pgm==NULL && strcmp(group,"")!=0 ) 
	{
		return NULL;
	}
	result=find_runpgm(NULL,pgm);
	if ( result==NULL ) 
	{
		return NULL;
	}
	list = malloc(sizeof(OBJLIST));
	if ( !list ) return NULL;
	list->oclass = NULL;
	list->asize = list->size = result->hit_count;
	list->objlist = malloc(sizeof(OBJECT*)*result->hit_count);
	if ( !list->objlist ) return NULL;
	for ( obj=find_first(result),n=0 ; obj!=NULL ; obj=find_next(result,obj),n++ )
		list->objlist[n] = obj;
	return list;
}

void objlist_destroy(OBJLIST *list)
{
	if ( list )
	{
		if ( list->objlist ) free(list->objlist);
		free(list);
	}
}

size_t objlist_add(OBJLIST *list, PROPERTY *match, char *match_part, char *match_op, void *match_value1, void *match_value2)
{
	OBJECT *obj;
	PROPERTYCOMPAREOP op = property_compare_op(match->ptype, match_op);
	for ( obj=object_get_first() ; obj!=NULL ; obj=object_get_next(obj) )
	{
		void *x = GETADDR(obj,match);
		if ( obj->oclass!=list->oclass ) continue;
		if ( property_compare_basic(match->ptype,op,x,match_value1,match_value2,match_part) )
		{
			if ( list->size==list->asize )
			{	// grow the list
				OBJECT **larger = (OBJECT**)malloc(sizeof(OBJECT*)*list->asize*2);
				memcpy(larger,list->objlist,sizeof(OBJECT*)*list->asize);
				memset(larger+list->asize,0,sizeof(OBJECT*)*list->asize);
				list->asize*=2;
				free(list->objlist);
				list->objlist = larger;
			}
			list->objlist[list->size++] = obj;
		}
	}
	return list->size;
}

size_t objlist_del(OBJLIST *list, PROPERTY *match, char *match_part, char *match_op, void *match_value1, void *match_value2)
{
	PROPERTYCOMPAREOP op = property_compare_op(match->ptype, match_op);
	int n, m;

	// scan list and mark objects to be removed
	for ( n=0 ; n<list->size ; n++ )
	{
		OBJECT *obj = list->objlist[n];
		void *x = (void*)((char*)(obj+1) + (int64)match->addr);
		if ( list->oclass!=NULL && obj->oclass!=list->oclass ) continue;
		if ( property_compare_basic(match->ptype,op,x,match_value1,match_value2,match_part) )
			list->objlist[n] = NULL; // marked for deletion
	}

	// compress list
	for ( n=0,m=0 ; n<list->size ; n++ )
	{
		if ( list->objlist[n]==NULL ) continue;
		if ( n>m ) list->objlist[m++] = list->objlist[n];
	}
	list->size = m;
	return list->size;
}
size_t objlist_size(OBJLIST *list)
{
	return list->size;
}
struct s_object_list *objlist_get(OBJLIST *list,size_t n)
{
	return (n<list->size)?list->objlist[n]:NULL;
}

/** Apply the function to each object in the object list.

	The function must accept an object pointer as first argument,
	a pointer to additional data structure as second argument, and
	the third argument is a position indicator.
	For the last call the object pointer is NULL and the position indicate is -1. 

	The function's return value must indicate success (non-zero) or failure (zero).

	@example The following example illustrates how to use #objlist_apply by performing
	a log mean calculation on a property of objects in a particular:
	
	// define the arg structure passed to the logmean function
	struct s_arg {
		size_t addr; // object property addr of value used by logmean 
		double sum;		// accumulator for sum
		unsigned int n;	// accumulator for count
		double ans;  // storage for answer	
	};
	int logmean(OBJECT *obj,void *arg)
	{
		if ( obj ) // normal call
		{
			double x = *(double*)GETADDR(obj,arg->addr);
			arg->sum += log(x);
			arg->n++;
		} // last call
		else
		{
			arg->ans = arg->sum / arg->n;
		}
		return 1;
	}

	double example(char *classname, char *propertyname) 
	{
		struct s_arg arg = {0,0.0,00.0};
		CLASS *oclass = class_get_class_from_classname(classname);
		PROPERTY *prop = oclass?class_find_property(oclass,propertyname):NULL;
		double zero=0;
		if ( prop==NULL ) return QNAN;
		OBJLIST *list = objlist_create(oclass,prop,NULL,">",&zero,NULL);
		if ( list==NULL ) return QNAN;
		arg.addr = prop->addr;
		return objlist_apply(list,&arg,logmean);
	}

	@returns a positive value if successful, zero or negative if failed 
	(negative value indicate how many objects were successfully processed before failure).
 **/
int objlist_apply(OBJLIST *list, /**< object list */
				  void *arg, /**< pointer to additional data given to function */
				  int (*function)(OBJECT *,void *,int)) /**< function to call */
{
	int n;
	for ( n=0 ; n<list->size ; n++ )
	{
		OBJECT *obj = list->objlist[n];
		if ( function(obj,arg,n)==0 )
			return -n;
	}
	if ( function(NULL,arg,-1)==0 )
		return -n;
	else
		return n;
}

/**@}*/
