/* $Id$
 */

#ifndef _LINK_H
#define _LINK_H

#ifdef __cplusplus

typedef struct s_linklist {
	char *name;
	void *data;
	struct s_linklist *next;
} LINKLIST;

class link {
private: // target data link
	char target[64]; ///< name of target
	LINKLIST *globals; ///< list of globals to export to target
	LINKLIST *objects; ///< list of objects to export to target
	char command[1024]; ///< OS command to load target
	char function[1024]; ///< function to pass to target
	LINKLIST *arguments; ///< list of arguments to pass to target
	void *data;	///< other data associated with this link

private: // target function link
	void *handle;
	bool (*settag)(link *mod, char *,char*);
	bool (*init)(link *mod);
	TIMESTAMP (*sync)(link *mod, TIMESTAMP t0);
	bool (*term)(link *mod);
	LINKLIST *copyto; // list of items to copy to target
	LINKLIST *copyfrom; // list of items to copy from target

public:
	void *get_handle();
	inline bool do_init() { return init==NULL ? false : (*init)(this); };
	inline TIMESTAMP do_sync(TIMESTAMP t) { return sync==NULL ? TS_NEVER : (*sync)(this,t); };
	inline bool do_term(void) { return term==NULL ? true : (*term)(this); };

private: // link list
	class link *next; ///< pointer to next link target
	static class link *first; ///< pointer for link target

public: // link list accessors
	inline static class link *get_first() { return first; }
	inline class link *get_next() { return next; };

public: // construction/destruction
	link(char *file);
	~link(void);

public: // 
	bool set_target(char *data);
	inline char *get_target(void) { return target; };
	LINKLIST *add_global(char *name);
	LINKLIST * add_object(char *name);
	inline void set_command(char *cmd) { strncpy(command,cmd,sizeof(command)); };
	void set_function(char *fn) { strncpy(function,fn,sizeof(function)); };
	LINKLIST * add_argument(char *name);
	inline void set_data(void *ptr) { data=ptr; };
	void *get_data(void) { return data; };
	inline LINKLIST *add_copyfrom(void *from,void*to) { /* TODO */ return NULL; };
	inline LINKLIST *add_copyto(void*from,void*to) { /* TODO */ return NULL; };
public:
	inline LINKLIST *get_globals(void) { return globals; };
	inline LINKLIST *get_objects(void) { return objects; };
	inline LINKLIST *get_arguments(void) { return arguments; };
	inline LINKLIST *get_next(LINKLIST *item) { return item->next; };
	inline GLOBALVAR *get_globalvar(LINKLIST *item) { return (GLOBALVAR*)item->data; };
	inline OBJECT *get_object(LINKLIST *item) { return (OBJECT*)item->data; };
	inline char *get_name(LINKLIST *item) { return (char*)item->name; }; 
};

extern "C" {
#endif

int link_create(char *name);
int link_initall(void);
TIMESTAMP link_syncall(TIMESTAMP t0);
int link_termall();

#ifdef __cplusplus
}
#endif

#endif
