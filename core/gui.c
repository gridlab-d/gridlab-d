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

GUIENTITY *gui_get_root(void)
{
	return gui_root;
}

GUIENTITY *gui_get_last(void)
{
	return gui_last;
}

GUIENTITY *gui_create_entity()
{
	GUIENTITY *entity = (GUIENTITY*)malloc(sizeof(GUIENTITY));
	if (entity==NULL)
		return NULL;
	memset(entity,0,sizeof(entity));
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
	entity->obj = NULL;
	strncpy(entity->objectname,objectname,sizeof(entity->objectname));
}
void gui_set_propertyname(GUIENTITY *entity, char *propertyname)
{
	entity->prop = NULL;
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
	if (gui_get_object(entity)!=NULL)
	{
		entity->prop = object_get_property(entity->obj,entity->propertyname);
		entity->addr = object_get_addr(entity->obj,entity->propertyname);
		class_property_to_string(entity->prop,entity->addr,entity->value,sizeof(entity->value));
	}
	else if (gui_get_variable(entity))
	{
		entity->var = global_find(entity->globalname);
	}
	return entity->value;
}
GLOBALVAR *gui_get_variable(GUIENTITY *entity)
{
	if (entity->var==NULL) entity->var = global_find(entity->globalname);
	return entity->var;
}
OBJECT *gui_get_object(GUIENTITY *entity)
{
	if (entity->obj==NULL) entity->obj = object_find_name(entity->objectname);
	return entity->obj;
}
PROPERTY *gui_get_property(GUIENTITY *entity)
{
	OBJECT *obj = gui_get_object(entity);
	if (entity->prop==NULL && obj!=NULL) entity->prop = object_get_property(obj,entity->propertyname);
	return entity->prop;
}
int gui_get_span(GUIENTITY *entity)
{
	return entity->span;
}
UNIT *gui_get_unit(GUIENTITY *entity)
{
	return entity->unit;
}

/* HTML OPERATIONS */
static void gui_html_output_entity(GUIENTITY *entity)
{
	switch (entity->type) {
	case GUI_TITLE: break;
	case GUI_STATUS: break;
	case GUI_ROW: break;
	case GUI_TAB: break;
	case GUI_PAGE: break;
	case GUI_GROUP: break;
	case GUI_SPAN: break;
	case GUI_TEXT: break;
	case GUI_INPUT: break;
	case GUI_CHECK: break;
	case GUI_RADIO: break;
	case GUI_SELECT: break;
	case GUI_ACTION: break;
	default: break;
	}
}

static void gui_html_output(GUIENTITY *entity)
{
	gui_html_output_entity(entity);
	if (entity->next)
		gui_html_output(entity->next);
}

void gui_html_output_all(void)
{
	gui_html_output(gui_root);
}

