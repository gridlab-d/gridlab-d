/*
 *  gui.h
 *  gridlabd
 *
 *  Created by d3g637-local on 10/26/10.
 *  Copyright 2010 Battelle Memorial Institute. All rights reserved.
 *
 */

#ifndef _GUI_H
#define _GUI_H

typedef enum {
	GUI_UNKNOWN=0,

	/* DO NOT CHANGE THE ORDER OF THE GROUPS!!! */
	GUI_ROW, // a row group
	GUI_TAB, // a tab group (includes tabs at top)
	GUI_PAGE, // a page group (includes navigation |< < > >| buttons at top)
	GUI_GROUP, // a group of entities with a labeled border around it
	GUI_SPAN, // a group of entities that are not in columns
	_GUI_GROUPING_END, // end of grouping entities

	GUI_TITLE, // the title of the page, tab, or block
	GUI_STATUS, // the status message of the page
	GUI_TEXT, // a plain text entity 
	_GUI_LABELING_END, // end of labeling entities

	GUI_INPUT, // an input textbox
	GUI_CHECK, // a check box (set)
	GUI_RADIO, // a radio button (enumeration)
	GUI_SELECT, // a select drop down (enumeration)
	_GUI_INPUT_END, // end of input entities

	GUI_BROWSE, // a text browsing entity
	GUI_TABLE, // a tabulate presentation
	GUI_GRAPH, // a graphing presentation
	_GUI_OUTPUT_END, // end of output entities

	GUI_ACTION, // an action button
	_GUI_ACTION_END, // end of action entities

} GUIENTITYTYPE;
typedef enum {GUIACT_NONE=0, GUIACT_WAITING=1, GUIACT_PENDING=2, GUIACT_HALT=3} GUIACTIONSTATUS;

typedef struct s_guientity {
	GUIENTITYTYPE type;	// gui entity type (see GE_*)
	char srcref[1024]; // reference to source file location
	char value[1024]; // value (if provided)
	char globalname[64]; // ref to variable
	char objectname[64]; // ref object
	char propertyname[64]; // ref property
	char action[64]; // action value
	int span; // col span
	int size; // size spec
	int height; // height spec
	int width; // width spec
	GUIACTIONSTATUS action_status;
	char wait_for[64];
	char source[1024]; // source file for data (output only)
	char options[1024]; // options for output
	char gnuplot[4096];	// gnuplot script
	int hold; // gui will remain on at the end of the simulation
	struct s_guientity *next;
	struct s_guientity *parent;
	/* internal variables */
	GLOBALVAR *var;
	char *env;
	OBJECT *obj;
	PROPERTY *prop;
	void *data;
	UNIT *unit;
} GUIENTITY;

GUIENTITY *gui_create_entity();
typedef  int (*GUISTREAMFN)(void*,char*,...);
void gui_set_html_stream(void *ref,GUISTREAMFN stream);

void gui_set_srcref(GUIENTITY *entity, char *filename, int linenum);
void gui_set_type(GUIENTITY *entity, GUIENTITYTYPE type);
void gui_set_value(GUIENTITY *entity, char *value);
void gui_set_variablename(GUIENTITY *entity, char *globalname);
void gui_set_objectname(GUIENTITY *entity, char *objectname);
void gui_set_propertyname(GUIENTITY *entity, char *propertyname);
void gui_set_span(GUIENTITY *entity, int span);
void gui_set_unit(GUIENTITY *entity, char *unit);
void gui_set_next(GUIENTITY *entity, GUIENTITY *next);
void gui_set_parent(GUIENTITY *entity, GUIENTITY *parent);
void gui_set_source(GUIENTITY *entity, char *source);
void gui_set_options(GUIENTITY *entity, char *source);
void gui_set_wait(GUIENTITY *entity, char *wait);

STATUS gui_startup(int argc, char *argv[]);
int gui_post_action(char *action);

GUIENTITY *gui_get_root(void);
GUIENTITY *gui_get_last(void);
GUIENTITYTYPE gui_get_type(GUIENTITY *entity);
GUIENTITY *gui_get_parent(GUIENTITY *entity);
GUIENTITY *gui_get_next(GUIENTITY *entity);
char *gui_get_name(GUIENTITY *entity);
OBJECT *gui_get_object(GUIENTITY *entity);
PROPERTY *gui_get_property(GUIENTITY *entity);
char *gui_get_value(GUIENTITY *entity);
void *gui_get_data(GUIENTITY *entity);
GLOBALVAR *gui_get_variable(GUIENTITY *entity);
char *gui_get_environment(GUIENTITY *entity);
int gui_get_span(GUIENTITY *entity);
UNIT *gui_get_unit(GUIENTITY *entity);

int gui_is_grouping(GUIENTITY *entity);
int gui_is_labeling(GUIENTITY *entity);
int gui_is_input(GUIENTITY *entity);
int gui_is_output(GUIENTITY *entity);
int gui_is_action(GUIENTITY *entity);

int gui_is_header(GUIENTITY *entity);

void gui_html_start(void);
void gui_X11_start(void);

void gui_wait_status(GUIACTIONSTATUS status);

int gui_html_output_page(char *page);
STATUS gui_html_output_all(void);

size_t gui_glm_write_all(FILE *fp);

int gui_wait(void);

#endif

