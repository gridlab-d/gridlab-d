/*
 *  gui.c
 *  gridlabd
 *
 *  Created by d3g637-local on 10/26/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <stdlib.h>
#include "complex.h"
#include "object.h"
#include "load.h"
#include "output.h"
#include "random.h"
#include "convert.h"
#include "schedule.h"
#include "gui.h"

static GUIENTITY *gui_root = NULL;
static GUIENTITY *gui_last = NULL;
static FILE *fp = NULL;

GUIENTITY *gui_get_root(void)
{
	return gui_root;
}

GUIENTITY *gui_get_last(void)
{
	return gui_last;
}

GUIENTITYTYPE gui_get_type(GUIENTITY *entity)
{
	return entity->type;
}

int gui_is_header(GUIENTITY *entity)
{
	if (entity->type==GUI_TITLE && !(entity->parent!=NULL && entity->parent->type==GUI_GROUP)) return 1;
	return 0;
}

GUIENTITY *gui_create_entity()
{
	GUIENTITY *entity = (GUIENTITY*)malloc(sizeof(GUIENTITY));
	if (entity==NULL)
		return NULL;
	memset(entity,0,sizeof(GUIENTITY));
	if (gui_root==NULL) gui_root = entity;
	if (gui_last!=NULL) gui_last->next = entity;
	gui_last = entity;
	return entity;
}

int gui_is_grouping(GUIENTITY *entity)
{
	return (entity->type<_GUI_GROUPING_END);
}

/* SET OPERATIONS */
void gui_set_type(GUIENTITY *entity, GUIENTITYTYPE type)
{
	entity->type = type;
}
void gui_set_value(GUIENTITY *entity, char *value)
{
	strncpy(entity->value,value,sizeof(entity->value));
}
void gui_set_variablename(GUIENTITY *entity, char *globalname)
{
	strncpy(entity->globalname,globalname,sizeof(entity->globalname));
}
void gui_set_objectname(GUIENTITY *entity, char *objectname)
{
	entity->data = NULL;
	strncpy(entity->objectname,objectname,sizeof(entity->objectname));
}
void gui_set_propertyname(GUIENTITY *entity, char *propertyname)
{
	entity->data = NULL;
	strncpy(entity->propertyname,propertyname,sizeof(entity->propertyname));
}
void gui_set_span(GUIENTITY *entity, int span)
{
	entity->span = span;
}
void gui_set_unit(GUIENTITY *entity, char *unit)
{
	entity->unit = unit_find(unit);
}
void gui_set_next(GUIENTITY *entity, GUIENTITY *next)
{
	entity->next = next;
}
void gui_set_parent(GUIENTITY *entity, GUIENTITY *parent)
{
	entity->parent = parent;
}

/* GET OPERATIONS */
GUIENTITY *gui_get_parent(GUIENTITY *entity)
{
	return entity->parent;
}
GUIENTITY *gui_get_next(GUIENTITY *entity)
{
	return entity->next;
}
char *gui_get_value(GUIENTITY *entity)
{
	OBJECT *obj = object_find_name(entity->objectname);
	if (obj)
	{
		if (strcmp(entity->propertyname,"name")==0)
			strcpy(entity->value,object_name(obj));
		else if (strcmp(entity->propertyname,"class")==0)
			strcpy(entity->value,obj->oclass->name);
		else if (strcmp(entity->propertyname,"parent")==0)
			strcpy(entity->value,obj->parent?object_name(obj->parent):"");
		else if (strcmp(entity->propertyname,"rank")==0)
			sprintf(entity->value,"%d",obj->rank);
		else if (strcmp(entity->propertyname,"clock")==0)
			convert_from_timestamp(obj->clock,entity->value,sizeof(entity->value));
		else if (strcmp(entity->propertyname,"valid_to")==0)
			convert_from_timestamp(obj->valid_to,entity->value,sizeof(entity->value));
		else if (strcmp(entity->propertyname,"in_svc")==0)
			convert_from_timestamp(obj->in_svc,entity->value,sizeof(entity->value));
		else if (strcmp(entity->propertyname,"out_svc")==0)
			convert_from_timestamp(obj->out_svc,entity->value,sizeof(entity->value));
		else if (strcmp(entity->propertyname,"latitude")==0)
			convert_from_latitude(obj->latitude,entity->value,sizeof(entity->value));
		else if (strcmp(entity->propertyname,"longitude")==0)
			convert_from_longitude(obj->longitude,entity->value,sizeof(entity->value));
		else if (!object_get_value_by_name(obj,entity->propertyname,entity->value,sizeof(entity->value)))
			output_error("gui_get_value(GUIENTITY *entity={object='%s',property='%s',...} refers to a non-existent property", entity->objectname, entity->propertyname);
	}
	else if (gui_get_variable(entity))
	{
		entity->var = global_find(entity->globalname);
		global_getvar(entity->globalname,entity->value,sizeof(entity->value));
	}
	return entity->value;
}
char *gui_get_name(GUIENTITY *entity)
{
	OBJECT *obj = object_find_name(entity->objectname);
	GLOBALVAR *var = gui_get_variable(entity);
	if (obj)
	{
		static char buffer[1024];
		sprintf(buffer,"%s.%s", entity->objectname, entity->propertyname); 
		return buffer;
	}
	else if (var)
		return var->name;
	else
		return "";
}
void *gui_get_data(GUIENTITY *entity)
{
	if (entity->data==NULL)
	{
		OBJECT *obj = object_find_name(entity->objectname);
		if (!obj) return NULL;
		entity->data = object_get_addr(obj,entity->propertyname);
	}
	return entity->data;
}
GLOBALVAR *gui_get_variable(GUIENTITY *entity)
{
	if (entity->var==NULL) entity->var = global_find(entity->globalname);
	return entity->var;
}
int gui_get_span(GUIENTITY *entity)
{
	return entity->span;
}
UNIT *gui_get_unit(GUIENTITY *entity)
{
	return entity->unit;
}

#ifdef X11
/* X11 operations */
void gui_X11_start(void)
{
}
#endif

/* HTML OPERATIONS */
void gui_html_start(void)
{
	// TODO
}

static void gui_entity_html(GUIENTITY *entity)
{
	switch (entity->type) {
	case GUI_TITLE: 
		if (!gui_is_header(entity))
			fprintf(fp,"<LEGEND>%s</LEGEND>\n",gui_get_value(entity));
		else
			fprintf(fp,"<TITLE>%s</TITLE>\n",gui_get_value(entity));
		break;
	case GUI_STATUS: 
		fprintf(fp,"<SCRIPT LANG=\"jscript\"> window.status=\"%s\";</SCRIPT>\n", gui_get_value(entity));
		break;
	case GUI_ROW: 
		break;
	case GUI_TAB: 
		break;
	case GUI_PAGE: 
		break;
	case GUI_GROUP: 
		break;
	case GUI_SPAN: 
		break;
	case GUI_TEXT: 
		fprintf(fp," %s ",gui_get_value(entity));
		break;
	case GUI_INPUT:
		fprintf(fp,"<INPUT TYPE=\"text\" NAME=\"%s\" VALUE=\"%s\" />\n",gui_get_name(entity),gui_get_value(entity));
		break;
	case GUI_CHECK: 
		// TODO expand for each member
		fprintf(fp,"<INPUT TYPE=\"check\" NAME=\"%s\" VALUE=\"%s\" />\n",gui_get_name(entity),gui_get_value(entity));
		break;
	case GUI_RADIO: 
		// TODO expand for each member
		fprintf(fp,"<INPUT TYPE=\"radio\" NAME=\"%s\" VALUE=\"%s\" />\n",gui_get_name(entity),gui_get_value(entity));
		break;
	case GUI_SELECT: 
		// TODO expand for each member
		break;
	case GUI_ACTION: 
		fprintf(fp,"<INPUT TYPE=\"submit\" NAME=\"%s\" VALUE=\"%s\" />\n",gui_get_name(entity),gui_get_value(entity));
		break;
	default: break;
	}
}

STATUS gui_html_output_all(void)
{
	GUIENTITY *entity;
	fp = stdout;

	// header entities
	fprintf(fp,"<HTML>\n<HEAD>\n");
	for (entity=gui_get_root(); entity!=NULL; entity=entity->next)
	{
		if (gui_is_header(entity))
			gui_entity_html(entity);
	}
	fprintf(fp,"</HEAD>\n");

	// body entities
	fprintf(fp,"<BODY>\n");
	for (entity=gui_get_root(); entity!=NULL; entity=entity->next)
	{
		if (!gui_is_header(entity))
			gui_entity_html(entity);
	}
	fprintf(fp,"</BODY>\n</HTML>\n");
	return SUCCESS;
}

