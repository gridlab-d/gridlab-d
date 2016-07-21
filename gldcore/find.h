/** $Id: find.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file find.h
	@addtogroup find
 **/

#ifndef _FIND_H
#define _FIND_H

#include <stdarg.h>
#include <stdlib.h>
#include "object.h"
#include "match.h"
#include "convert.h"

struct s_object_list;

/* the values are important because they're used to index lookups in find.c */
typedef enum {EQ=0,LT=1,GT=2,NE=3,LE=4,GE=5,NOT=6,BETWEEN=7,BEFORE=8,AFTER=9,SAME=10,DIFF=11,MATCH=12,LIKE=13,UNLIKE=14,ISA=15,FINDOP_END} FINDOP;
typedef enum {OR=-2,AND=-1,FT_END=0, FT_ID=1, FT_SIZE=2, FT_CLASS=3, FT_PARENT=4, FT_RANK=5, FT_CLOCK=6, FT_PROPERTY=7, FT_NAME=8,
	FT_LAT=9, FT_LONG=10, FT_INSVC=11, FT_OUTSVC=12, FT_FLAGS=13, FT_MODULE=14, FT_GROUPID=15, FT_ISA=16} FINDTYPE;

typedef struct s_findlist {
	unsigned int result_size;
	unsigned int hit_count;
	char result[1]; /* this will expand according to need */
} FINDLIST;

typedef union {
	int64 integer;
	void *pointer;
	double real;
	char string[256];
} FINDVALUE; /**< a search value */

typedef int (*COMPAREFUNC)(void*, FINDVALUE);
typedef void (*FOUNDACTION)(FINDLIST *, struct s_object_list *);
typedef unsigned int PGMCONSTFLAGS; /**< find program constant criteria flags */

#define CF_SIZE		0x0001 /**< size criteria is invariant */
#define CF_ID		0x0002 /**< id criteria is invariant */
#define CF_CLASS	0x0004 /**< class criteria is invariant */
#define CF_RANK		0x0008 /**< rank criteria is invariant */
#define CF_CLOCK	0x0010 /**< clock criteria is variant */
#define CF_PARENT	0x0020 /**< parent criteria is invariant */
#define CF_PROPERTY	0x0040 /**< property criteria is variant */
#define CF_NAME		0x0080 /**< name criteria is invariant */
#define CF_LAT		0x0100 /**< latitude criteria is invariant */
#define CF_LONG		0x0200 /**< longitude criteria is invariant */
#define CF_INSVC	0x0400 /**< in-service criteria is invariant */
#define CF_OUTSVC	0x0800 /**< out-service criteria is invariant */
#define CF_FLAGS	0x1000 /**< flags criteria is variant */
#define CF_MODULE	0x2000 /**< module criteria is invariant */
#define CF_GROUPID	0x4000 /**< groupid criteria is invariant */
#define CF_CONSTANT 0x8000 /**< entire criteria is invariant */

typedef struct s_findpgm {
	PGMCONSTFLAGS constflags; /* bits to indicate which criteria result in constant sets */
	COMPAREFUNC op;
	unsigned short target; /* offset from start of object header */
	FINDVALUE value;
	FOUNDACTION pos, neg;
	struct s_findpgm *next;
} FINDPGM;


#ifdef __cplusplus
extern "C" {
#endif

FINDLIST *find_objects(FINDLIST *list, ...);
FINDLIST *findlist_copy(FINDLIST *list);
void findlist_add(FINDLIST *list, struct s_object_list *obj);
void findlist_del(FINDLIST *list, struct s_object_list *obj);
void findlist_clear(FINDLIST *list);
struct s_object_list *find_first(FINDLIST *list);
struct s_object_list *find_next(FINDLIST *list, struct s_object_list *obj);
int find_makearray(FINDLIST *list, struct s_object_list ***objs);
FINDLIST *find_runpgm(FINDLIST *list, FINDPGM *pgm);
FINDPGM *find_mkpgm(char *search);
PGMCONSTFLAGS find_pgmconstants(FINDPGM *pgm);
char *find_file(char *name, char *path, int mode, char *buffer, int len);
FINDPGM *find_make_invariant(FINDPGM *pgm, int mode);

#ifdef __cplusplus
}
#endif

#define FL_NEW (FINDLIST*)(0)
#define FL_GROUP (FINDLIST*)(-1)

/* advanced find */
typedef struct s_objlist {
	CLASS *oclass;		/**< class of object */
	size_t asize;		/**< size of arrays */
	size_t size;		/**< number of items in list */
	struct s_object_list **objlist;	/**< pointer to array of objects that match */
} OBJLIST;

#ifdef __cplusplus
extern "C" {
#endif

OBJLIST *objlist_create(CLASS *oclass, PROPERTY *match_property, char *match_part, char *match_op, void *match_value1, void *match_value2);
OBJLIST *objlist_search(char *group);
void objlist_destroy(OBJLIST *list);
size_t objlist_add(OBJLIST *list, PROPERTY *match_property, char *match_part, char *match_op, void *match_value1, void *match_value2);
size_t objlist_del(OBJLIST *list, PROPERTY *match_property, char *match_part, char *match_op, void *match_value1, void *match_value2);
size_t objlist_size(OBJLIST *list);
struct s_object_list *objlist_get(OBJLIST *list,size_t n);
int objlist_apply(OBJLIST *list, void *arg, int (*function)(struct s_object_list *,void *,int pos));

#ifdef __cplusplus
}
#endif

#endif

