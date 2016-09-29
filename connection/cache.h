/// $Id$
/// @file
/// @addtogroup connection
/// @{

#ifndef _CACHE_H
#define _CACHE_H

#include "connection.h"
#include "message.h"
#include "varmap.h"

typedef enum {
	TF_NONE, ///< no action
	TF_SCHEMA, ///< varmap schema
	TF_DATA, ///< gridlabd data
	TF_TUPLE, ///< tag value pair
} TRANSLATIONFLAG;

typedef int (TRANSLATOR)(char *local, size_t local_len, char *remote, size_t remote_len, TRANSLATIONFLAG flags, ...);

typedef unsigned int CACHEID; ///< cache item id number
#define BADCACHEID (CACHEID)(-1) ///< indicates that the cache id is not valid
class cacheitem { ///< cacheitem class
private:
	static size_t indexsize; 
	static cacheitem **index;
	static cacheitem *first;
	CACHEID id;
	unsigned int lock;
	VARMAP *var;
	char *value;
	TRANSLATOR *xltr;
	cacheitem *next;
	bool marked; // true indicate value needs to be sync'd
public:
	cacheitem(VARMAP *var); ///< creates a new cache entry (invalidates pre-existing CACHEIDs)
	static void init();
private:
	static void grow();
public:
	inline void mark(void) { marked=true; };
	inline void unmark(void) { marked=false; };
	inline bool is_marked(void) { return marked; };
	static CACHEID get_id(VARMAP *var, size_t modulo=0); ///< get the CACHEID for a cache tuple
	static inline cacheitem *get_item(CACHEID id) ///< get the cache item from the id
		{ return index[id];};
	inline CACHEID get_id(void) { return id; };
	inline VARMAP *get_var() ///< get the variable map for this item
		{ return var; };
	inline OBJECT *get_object() ///< get the object corresponding to the item
		{ return var->obj->get_object(); }; 
	inline PROPERTY *get_property() 
		{ return var->obj->get_property(); }; ///< get the property corresponding to the item
	inline gld_property get_object_property(void) { return gld_property(get_object(),get_property()); }
	inline char *get_remote() ///< get the remote name corresponding to the item
		{ return var->remote_name; }; 
	bool read(char *buffer, size_t len); ///< read the item from the cache if buffer is large enough
	bool write(char *value); ///< write the item to the cache if space available
	bool copy_from_object(void);
	bool copy_to_object(void);
	inline void set_translator(TRANSLATOR *fn) ///< set the translator to use when copying cache to and from transport buffers
		{ xltr=fn; };
	inline void translate_tuple(char *from, size_t flen, char *tag, char *val);
	inline char *get_buffer(void) { return value;};
	inline size_t get_size(void) { return 1024;};
};

class cache { ///< cache class
private:
	size_t size;
	size_t tail;
	CACHEID *list;
public:
	cache(void); // constructs a cache
	~cache(void);
public:
	int option(char *command);
	void set_size(size_t); ///< sets the size of a connection cache
	inline size_t get_count(void) ///< gets the size of the connection cache
		{ return tail; }; 
	CACHEID get_id(size_t n) { return list[n];};
	cacheitem *get_item(size_t n) { return cacheitem::get_item(list[n]); };
	cacheitem *add_item(VARMAP *var);
	bool write(VARMAP *var, TRANSLATOR *xltr=NULL); ///< write to a cache item
	bool read(VARMAP *var, TRANSLATOR *xltr=NULL); ///< read from a cache item
	cacheitem *find_item(VARMAP *var);
	void dump(void);
};

#endif /// @} _CACHE_H
