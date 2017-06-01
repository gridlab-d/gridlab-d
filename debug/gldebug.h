/** $Id: gldebug.h 4738 2014-07-03 00:55:39Z dchassin $

 General purpose debug objects

 **/

#ifndef _DEBUG_H
#define _DEBUG_H

#include "gridlabd.h"

#define DBO_DETAILS 0x0001

typedef struct s_notification NOTIFICATION;
struct s_notification {
	OBJECT *recipient;	///< notice recipient
	OBJECT *target;		///< target of interest
	PROPERTY *prop;		///< property of interest
	int (*call)(NOTIFICATION*); ///< call on notice
	int (*chain)(OBJECT*,int,PROPERTY*,char*); ///< next notice in notification chain for this notice
	NOTIFICATION *next;	///< next notice in notification list
};

class g_debug : public gld_object {
private:
	static char **history;
	static unsigned long history_next;
	static unsigned long history_size;
	static unsigned long command_num;

	gld_property target_property;
	char1024 last_value;
public:
	typedef enum {
		DBT_NONE, 
		DBT_BREAK,		///< stop on condition
		DBT_WATCH,		///< stop on change
	} DEBUGTYPE;
	typedef enum {
		DBS_INACTIVE,	///< debug point is inactive
		DBS_ACTIVE,		///< debug point is active
	} DEBUGSTATUS;
	typedef enum {
		DBE_NONE		= 0x0000,	///< no event
		DBE_INIT		= 0x0001,	///< on init
		DBE_PRECOMMIT	= 0x0002,	///< on precommit
		DBE_PRESYNC		= 0x0004,	///< on presync
		DBE_SYNC		= 0x0008,	///< on sync
		DBE_POSTSYNC	= 0x0010,	///< on postsync
		DBE_COMMIT		= 0x0020,	///< on commit
		DBE_FINALIZE	= 0x0040,	///< on finalize
		DBE_ALL			= 0xffff,	///< all events
	} DEBUGEVENT;
	GL_ATOMIC(DEBUGTYPE,type);
	GL_ATOMIC(DEBUGSTATUS,status);
	GL_ATOMIC(set,event);
	GL_ATOMIC(TIMESTAMP,stoptime);
	GL_STRING(char1024,target);		
	GL_STRING(char32,part);
	GL_ATOMIC(PROPERTYCOMPAREOP,relation);
	GL_STRING(char1024,value);
	GL_STRING(char1024,value2);
	GL_BITFLAGS(set,options);

private:
	bool stop_condition(DEBUGEVENT context);
	bool get_command(void);
	size_t message(char *fmt=NULL, ...);
	bool cmd_print(char *args="");
	bool cmd_break(char *args="");
	bool cmd_watch(char *args="");
	bool cmd_list(char *args="");
	bool cmd_help(char *args="");
	bool cmd_signal(char *args="");
	bool cmd_history(char *args="");
	bool cmd_details(char *args="");
	bool cmd_info(char *args="");
	
private:
	void console_help(unsigned int page=0, unsigned int row=0);
	void console_load(void);
	void console_save(void);
	bool console_get_command(void);
	void console_show_objlist(gld_object *parent=NULL, int level=0);
	void console_show_object(void);
	void console_show_messages(void);
	bool show_line(int row, const char *label, const char *fmt=NULL, ...);
	bool console_confirm(const char *msg);
	int console_message(const char *fmt, ...);

public:
	int notify(NOTIFICATION *notice);

public:
	/* required implementations */
	g_debug(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
	int precommit(TIMESTAMP t);
	TIMESTAMP presync(TIMESTAMP t);
	TIMESTAMP sync(TIMESTAMP t);
	TIMESTAMP postsync(TIMESTAMP t);
	int finalize(void);

public:
	static CLASS *oclass;
	static g_debug *defaults;
};
#endif // _DEBUG_H