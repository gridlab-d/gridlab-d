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

#ifdef _DEBUG
#define TABLEOPTIONS " border=0 CELLPADDING=0 CELLSPACING=0"
#else
#define TABLEOPTIONS 
#endif

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
	/* resuse last entity if it remains unused */
	GUIENTITY *entity = gui_last;
	if (entity==NULL || entity->type!=GUI_UNKNOWN)
		entity = (GUIENTITY*)malloc(sizeof(GUIENTITY));
	if (entity==NULL)
		return NULL;
	memset(entity,0,sizeof(GUIENTITY));
	if (gui_root==NULL) gui_root = entity;
	if (gui_last!=NULL) gui_last->next = entity;
	gui_last = entity;
	return entity;
}

GUIENTITYTYPE gui_get_type(GUIENTITY *entity)
{
	return entity->type;
}

char *gui_get_typename(GUIENTITY *entity)
{
	switch (entity->type) {
#define CASE(X) case X: return #X;
	CASE(GUI_UNKNOWN)
	CASE(GUI_ROW)
	CASE(GUI_TAB)
	CASE(GUI_PAGE)
	CASE(GUI_GROUP)
	CASE(GUI_SPAN)
	CASE(_GUI_GROUPING_END)
	CASE(GUI_TITLE)
	CASE(GUI_STATUS)
	CASE(GUI_TEXT)
	CASE(GUI_INPUT)
	CASE(GUI_CHECK)
	CASE(GUI_RADIO)
	CASE(GUI_SELECT)
	CASE(GUI_ACTION)
	default: return "<INVALID>";
	}
}

int gui_is_header(GUIENTITY *entity)
{
	if (entity->type==GUI_TITLE && !(entity->parent!=NULL && entity->parent->type==GUI_GROUP)) return 1;
	return 0;
}

int gui_is_grouping(GUIENTITY *entity)
{
	return (entity->type<_GUI_GROUPING_END);
}

/* SET OPERATIONS */
void gui_set_srcref(GUIENTITY *entity, char *filename, int linenum)
{
	sprintf(entity->srcref,"%s(%d)", filename, linenum);
}
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
char *gui_get_dump(GUIENTITY *entity)
{
	static char buffer[1024];
	sprintf(buffer,"{type=%s,srcref='%s',value='%s',globalname='%s',object='%s',property='%s',action='%s',span=%d}",
		gui_get_typename(entity), entity->srcref, entity->value, entity->globalname, entity->objectname, entity->propertyname, entity->action, entity->span);
	return buffer;
}
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
	OBJECT *obj = gui_get_object(entity);
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
			output_error_raw("%s: ERROR: %s refers to a non-existent property '%s'", entity->srcref, gui_get_typename(entity), entity->propertyname);
	}
	else if (gui_get_variable(entity))
	{
		entity->var = global_find(entity->globalname);
		global_getvar(entity->globalname,entity->value,sizeof(entity->value));
	}
	else if (gui_get_environment(entity))
		strcpy(entity->value,entity->env);
	return entity->value;
}
OBJECT *gui_get_object(GUIENTITY *entity)
{
	if (!entity->obj) entity->obj = object_find_name(entity->objectname);
	return entity->obj;
}
PROPERTY *gui_get_property(GUIENTITY *entity)
{
	if (entity->prop==NULL)
	{
		if (gui_get_object(entity))
			entity->prop = class_find_property(entity->obj->oclass,entity->propertyname);
		else if (gui_get_variable(entity))
			entity->prop = entity->var->prop;
	}
	return entity->prop;
}
char *gui_get_name(GUIENTITY *entity)
{
	if (gui_get_object(entity))
	{
		static char buffer[1024];
		sprintf(buffer,"%s.%s", entity->objectname, entity->propertyname); 
		return buffer;
	}
	else if (gui_get_variable(entity))
		return entity->var->name;
	else
		return "";
}
void *gui_get_data(GUIENTITY *entity)
{
	if (entity->data==NULL)
	{
		if (gui_get_object(entity)) 
			entity->data = object_get_addr(entity->obj,entity->propertyname);
		else if (gui_get_variable(entity))
			entity->data = entity->var->prop->addr; 
		else
			entity->data=NULL;
	}
	return entity->data;
}
GLOBALVAR *gui_get_variable(GUIENTITY *entity)
{
	if (entity->var==NULL) entity->var = global_find(entity->globalname);
	return entity->var;
}
char *gui_get_environment(GUIENTITY *entity)
{
	if (entity->env==NULL) entity->env = getenv(entity->globalname);
	return entity->env;
}
int gui_get_span(GUIENTITY *entity)
{
	return entity->span;
}
UNIT *gui_get_unit(GUIENTITY *entity)
{
	return entity->unit;
}

/**************************************************************************/
/* COMMAND LINE OPERATIONS */
/**************************************************************************/
int gui_cmd_entity(int item, GUIENTITY *entity)
{
	switch (entity->type) {
	case GUI_TITLE: 
		fprintf(stdout, "%s", gui_get_value(entity));
		break;
	case GUI_STATUS: 
		break;
	case GUI_ROW: 
		if (item>0) fprintf(stdout,"\n%2d. ",item);
		return 1;
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
		break;
	case GUI_INPUT:
		fprintf(stdout," [%s]",gui_get_value(entity));
		break;
	case GUI_CHECK: 
		fprintf(stdout," [%s]",gui_get_value(entity));
		break;
	case GUI_RADIO: 
		fprintf(stdout," [%s]",gui_get_value(entity));
		break;
	case GUI_SELECT: 
		fprintf(stdout," [%s]",gui_get_value(entity));
		break;
	case GUI_ACTION: 
		break;	
	default:
		break;
	}
	return 0;
}

void gui_cmd_prompt(GUIENTITY *parent)
{
	char buffer[1024];
	int item=0, ans=-1;
	char *label;
	GUIENTITY *entity;
	for ( entity=gui_root ; entity!=NULL ; entity=entity->next )
	{
		if ( entity->parent==parent )
		{
			if ( entity->type==GUI_TITLE )
				label = gui_get_value(entity);
			else
				break;
		}
	}
Retry:
	fprintf(stdout,"\n%s> [%s] ",label, gui_get_value(entity));
	fflush(stdout);
	gets(buffer);
	if (strcmp(buffer,"")==0)
		return;
	if (entity->obj && !object_set_value_by_name(entity->obj,entity->propertyname,buffer))
	{
		fprintf(stdout,"Invalid input, try again.\n");
		goto Retry;
	}
	else if (entity->var && global_setvar(entity->globalname,buffer)==FAILED)
	{
		fprintf(stdout,"Invalid input, try again.\n");
		goto Retry;
	}
	else if (entity->env)
	{
		char env[1024];
		sprintf("%s=%s",entity->env,buffer);
		if (!_putenv(env))
		{
			fprintf(stdout,"Invalid input, try again.\n");
		goto Retry;
		}
	}
}

void gui_cmd_menu(GUIENTITY *parent)
{
	char buffer[1024];
	GUIENTITY *entity;
	GUIENTITY *list[100];

	while (1) {
		int item=0, ans=-1;

		for ( entity=gui_root ; entity!=NULL ; entity=entity->next )
		{
			if ( entity->parent==parent && gui_cmd_entity(item+1,entity))
				list[++item]=entity; 
			else if (item>0 && list[item]==entity->parent)
				gui_cmd_entity(item+1,entity);
		}
		if (item==0)
			break;
		fprintf(stdout,"\n 0. %s\n",parent?"Return":"Done");
Retry:
		fprintf(stdout,"\nGLM> [%d] ",ans<item?ans+1:0);
		fflush(stdout);
		gets(buffer);
		ans = atoi(buffer);
		if (ans<0 || ans>item)
		{
			fprintf(stdout,"Input must be between 0 and %d\n", item);
			goto Retry;
		}
		else if ( ans==0 )
			break;
		if ( list[ans]->type==GUI_ROW )
			gui_cmd_prompt(list[ans]);
		else if ( gui_is_grouping(list[ans]) )
			gui_cmd_menu(list[ans]);
	}
}

void gui_cmd_start(void)
{
	gui_cmd_menu(NULL);
} 

#ifdef X11
/**************************************************************************/
/* X11 OPERATIONS */
/**************************************************************************/
void gui_X11_start(void)
{
}
#endif

/**************************************************************************/
/* HTML OPERATIONS */
/**************************************************************************/
void gui_html_start(void)
{
	// TODO start server
}

#define MAX_TABLES 16
static int table=-1, row[MAX_TABLES], col[MAX_TABLES], span[MAX_TABLES];
static void startspan()
{
	if (table>=0) span[table]++;
}
static void endspan()
{
	if (table>=0) span[table]--;
}
static void newtable(GUIENTITY *entity)
{
	if (table<MAX_TABLES)
	{
		table++;
		row[table]=0;
		col[table]=0;
	}
	else
		output_error_raw("%s: ERROR: table nesting exceeded limit of %d", entity->srcref, MAX_TABLES);
}
static void newrow(GUIENTITY *entity)
{
	if (table<0) newtable(entity);
	if (row[table]==0) fprintf(fp,"\t<table" TABLEOPTIONS "> <!-- table %d %s -->\n",table,gui_get_typename(entity));
	if (col[table]>0) fprintf(fp,"\t</td> <!-- table %d col %d -->\n", table, col[table]);
	col[table]=0;
	if (row[table]>0) fprintf(fp,"\t</tr> <!--  table %d row %d -->\n", table, row[table]);
	row[table]++;
	fprintf(fp,"\t<tr> <!-- row %d -->\n", row[table]);
}
static void newcol(GUIENTITY *entity)
{
	if (span[table]>0) return;
	if (table<0 || row[table]==0) newrow(entity);
	if (col[table]>0) fprintf(fp,"\t</td> <!-- table %d col %d -->\n",  table, col[table]);
	col[table]++;
	if (entity->type==GUI_SPAN)
	{
		fprintf(fp,"\t<td colspan=\"%d\"> <!-- table %d col %d -->\n", entity->size, table, col[table]);
		if (entity->size==0) output_warning("%s: not all browsers accept span size 0 (meaning to end), span size may not work as expected", entity->srcref);
	}
	else
		fprintf(fp,"\t<td> <!-- table %d col %d -->\n", table, col[table]);
}
static void endtable()
{
	if (table<0) return;
	if (col[table]>0) fprintf(fp,"\t</td> <!-- table %d col %d -->\n", table, col[table]);
	if (row[table]>0) fprintf(fp,"\t</tr> <!-- table %d row %d -->\n", table, row[table]);
	fprintf(fp,"\t</table> <!-- table %d -->\n", table--);
}

static void gui_entity_html_content(GUIENTITY *entity)
{
	char *ptype = gui_get_property(entity) ? class_get_property_typename(entity->prop->ptype) : "";
	switch (entity->type) {
	case GUI_TITLE: 
		if (entity->parent==NULL)
			fprintf(fp,"<title>%s</title>\n",gui_get_value(entity));
		else if (entity->parent->type==GUI_GROUP)
			fprintf(fp,"<legend>%s</legend>\n",gui_get_value(entity));
		else 
			fprintf(fp,"<h%d>%s</h%d>\n",table+1,gui_get_value(entity),table+1);
		break;
	case GUI_STATUS: 
		fprintf(fp,"<script lang=\"jscript\"> window.status=\"%s\";</script>\n", gui_get_value(entity));
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
		if (!entity->parent || gui_get_type(entity->parent)!=GUI_SPAN) newcol(entity);
		fprintf(fp,"<span class=\"text\">%s</span>",gui_get_value(entity));
		break;
	case GUI_INPUT:
		if (!entity->parent || gui_get_type(entity->parent)!=GUI_SPAN) newcol(entity);
		fprintf(fp,"<input class=\"%s\" type=\"text\" name=\"%s\" value=\"%s\" onchange=\"update_%s(this)\"/>\n", ptype, gui_get_name(entity),gui_get_value(entity), ptype);
		break;
	case GUI_CHECK: 
		{	
			PROPERTY *prop = gui_get_property(entity);
			KEYWORD *key = NULL;
			if (!entity->parent || gui_get_type(entity->parent)!=GUI_SPAN) newcol(entity);
			for (key=prop->keywords; key!=NULL; key=key->next)
			{
				int value = *(int*)gui_get_data(entity);
				char *checked = (value==key->value)?"checked":"";
				char label[64], *p;
				strcpy(label,key->name);
				for (p=label; *p!='\0'; p++) if (*p=='_') *p=' '; else if (p>label && *p>='A' && *p<='Z') *p+='a'-'A';
				if (key->value!=0)
					fprintf(fp,"<nobr><input class=\"%s\"type=\"checkbox\" name=\"%s\" value=\"%"FMT_INT64"d\" %s onchange=\"update_%s(this)\"/>%s</nobr>\n",
						ptype, gui_get_name(entity), key->value, checked, ptype, label);
			}
		}
		break;
	case GUI_RADIO: 
		{
			PROPERTY *prop = gui_get_property(entity);
			KEYWORD *key = NULL;
			if (!entity->parent || gui_get_type(entity->parent)!=GUI_SPAN) newcol(entity);
			for (key=prop->keywords; key!=NULL; key=key->next)
			{
				int value = *(int*)gui_get_data(entity);
				char *checked = (value==key->value)?"checked":"";
				char label[64], *p;
				strcpy(label,key->name);
				for (p=label; *p!='\0'; p++) if (*p=='_') *p=' '; else if (p>label && *p>='A' && *p<='Z') *p+='a'-'A';
				fprintf(fp,"<nobr><input class=\"%s\" type=\"radio\" name=\"%s\" value=\"%"FMT_INT64"d\" %s onchange=\"update_%s(this)\" />%s</nobr>\n",
					ptype, gui_get_name(entity), key->value, checked, ptype, label);
			}
		}
		break;
	case GUI_SELECT: 
		{
			PROPERTY *prop = gui_get_property(entity);
			KEYWORD *key = NULL;
			char *multiple = (prop->ptype==PT_set?"multiple":"");
			char size[64] = "";
			if (entity->size>0) sprintf(size,"size=\"%d\"",entity->size);
			if (!entity->parent || gui_get_type(entity->parent)!=GUI_SPAN) newcol(entity);
			fprintf(fp,"<select class=\"%s\" name=\"%s\" %s %s onchange=\"update_%s(this)\">\n", ptype, gui_get_name(entity),multiple,size,ptype);
			for (key=prop->keywords; key!=NULL; key=key->next)
			{
				int value = *(int*)gui_get_data(entity);
				char *checked = (value==key->value)?"selected":"";
				char label[64], *p;
				strcpy(label,key->name);
				for (p=label; *p!='\0'; p++) if (*p=='_') *p=' '; else if (p>label && *p>='A' && *p<='Z') *p+='a'-'A';
				fprintf(fp,"<option value=\"%"FMT_INT64"d\" %s>%s</option>\n",
					key->value, checked, label);
			}
			fprintf(fp,"</select>\n");
		}
		break;
	case GUI_ACTION: 
		if (!entity->parent || gui_get_type(entity->parent)!=GUI_SPAN) newcol(entity);
		fprintf(fp,"<input class=\"action\" type=\"submit\" name=\"action\" value=\"%s\" onclick=\"click(this)\" />\n",entity->action);
		break;
	default: break;
	}
}
static void gui_entity_html_open(GUIENTITY *entity)
{
	switch (entity->type) {
	case GUI_TITLE: 
		break;
	case GUI_STATUS: 
		break;
	case GUI_ROW: 
		newrow(entity);
		break;
	case GUI_TAB:
		break;
	case GUI_PAGE: 
		break;
	case GUI_GROUP:
		newcol(entity);
		fprintf(fp,"<fieldset>\n");
		newtable(entity);
		break;
	case GUI_SPAN: 
		newcol(entity);
		startspan();
		break;
	case GUI_TEXT: 
		break;
	case GUI_INPUT:
		break;
	case GUI_CHECK: 
		break;
	case GUI_RADIO: 
		break;
	case GUI_SELECT: 
		break;
	case GUI_ACTION: 
		break;
	default: break;
	}
	gui_entity_html_content(entity);
}
static void gui_entity_html_close(GUIENTITY *entity)
{
	switch (entity->type) {
	case GUI_TITLE: 
		break;
	case GUI_STATUS: 
		break;
	case GUI_ROW: 
		newrow(entity);
		break;
	case GUI_TAB: 
		break;
	case GUI_PAGE: 
		break;
	case GUI_GROUP:
		endtable();
		fprintf(fp,"</fieldset>\n");
		break;
	case GUI_SPAN: 
		endspan();
		break;
	case GUI_TEXT: 
		break;
	case GUI_INPUT:
		break;
	case GUI_CHECK: 
		break;
	case GUI_RADIO: 
		break;
	case GUI_SELECT: 
		// TODO expand for each member
		break;
	case GUI_ACTION: 
		break;
	default: break;
	}
}
void gui_html_output_children(GUIENTITY *entity)
{
	GUIENTITY *child;
	if (entity!=NULL) gui_entity_html_open(entity);
	for (child=gui_get_root(); child!=NULL; child=child->next)
	{
		if (!gui_is_header(child) && child->parent==entity)
			gui_html_output_children(child);
	}
	if (entity!=NULL) gui_entity_html_close(entity);
}

void gui_include_element(char *tag, char *options, char *file)
{
	char *path = find_file(file,NULL,4);
	if (!path)
		output_error("unable to find '%s'", file);
	else
	{
		FILE *fin = fopen(path,"r");
		if (!fin)
			output_error("unable to open '%s'", path);
		else
		{
			char buffer[65536];
			size_t len = fread(buffer,1,sizeof(buffer),fin);
			if (len>=0)
			{
				buffer[len]='\0';
				fprintf(fp,"<%s%s%s>\n%s\n</%s>\n",tag,options?" ":"",options?options:"",buffer,tag);
			}
			else
				output_error("unable to read '%s'", path);
			fclose(fin);
		}
	}
}

STATUS gui_html_output_all(void)
{
	GUIENTITY *entity;
	fp = stdout;

	// header entities
	fprintf(fp,"<html>\n<head>\n");
	for (entity=gui_get_root(); entity!=NULL; entity=entity->next)
	{
		if (gui_is_header(entity))
			gui_entity_html_content(entity);
	}
	fprintf(fp,"</head>\n");

	gui_include_element("script","lang=\"jscript\"","gridlabd.js");
	gui_include_element("style",NULL,"gridlabd.css");

	// body entities
	fprintf(fp,"<body>\n");
	gui_html_output_children(NULL);
	endtable();
	fprintf(fp,"</body>\n</html>\n");
	return SUCCESS;
}

/**************************************************************************/
/* GLM OPERATIONS */
/**************************************************************************/
char *gui_glm_typename(GUIENTITYTYPE type)
{
	char *typename[] = {
		NULL, 
		"row", "tab", "page", "group", "span", NULL,
		"title", "status", "text", "input", "check", "radio", "select", "action", NULL,
	};
	if (type>=0 || type<sizeof(typename)/sizeof(typename[0]))
		return typename[type];
	else
		return NULL;
}
size_t gui_glm_write(FILE *fp, GUIENTITY *entity, int indent)
{
	size_t count=0;
	GUIENTITY *parent = entity;
	char *typename = gui_glm_typename(parent->type);
	char tabs[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	if (indent<0) tabs[0]='\0'; else if (indent<sizeof(tabs)) tabs[indent]='\0';
	if (typename==NULL)
		return FAILED;
	
	if (entity->type==GUI_ACTION)
	{
		count += fprintf(fp,"%saction %s;\n",tabs,entity->action);
	}
	else
	{
		count += fprintf(fp,"%s%s {\n",tabs,typename);

		if (gui_get_object(entity))
			count += fprintf(fp,"%s\tlink %s:%s;\n", tabs,entity->objectname,entity->propertyname);
		else if (gui_get_variable(entity))
			count += fprintf(fp,"%s\tglobal %s;\n", tabs,entity->globalname);
		else if (entity->value[0]!='\0')
			count += fprintf(fp,"%s\tvalue \"%s\";\n", tabs,entity->value);
		if (entity->unit)
			count += fprintf(fp,"%s\tunit \"%s\";\n", tabs,entity->unit);
		if (entity->size>0)
			count += fprintf(fp,"%s\tsize %d;\n", tabs,entity->size);
		if (entity->action[0]!='\0')
			count += fprintf(fp,"%s\taction %s;\n", tabs,entity->action);

		if (gui_is_grouping(entity))
			for ( entity=gui_root ; entity!=NULL ; entity=entity->next )
				if (entity->parent==parent)
					count+=gui_glm_write(fp,entity,indent+1);

		count += fprintf(fp,"%s}\n",tabs);
	}
	return count;
}
size_t gui_glm_write_all(FILE *fp)
{
	size_t count=0;
	GUIENTITY *entity;
	if (gui_root==NULL)
		return 0;
	count += fprintf(fp,"gui {\n");
	for ( entity=gui_get_root() ; entity!=NULL ; entity=entity->next )
	{
		if (entity->parent==NULL)
			count += gui_glm_write(fp,entity,1);
	}
	count += fprintf(fp,"}\n");
	return count;
}

/**************************************************************************/
/* LOAD OPERATIONS */
/**************************************************************************/
void gui_wait(void)
{
	char *env = global_getvar("environment",NULL,NULL);
	if (strcmp(env,"batch")==0)
		gui_cmd_start();
	// TODO HTML, X11 startup
}