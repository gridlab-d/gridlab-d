// powernet/message.h
// Copyright (C) 2016, Stanford University
// Author: dchassin@slac.stanford.edu

#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "powernet.h"

class item {
	char *tag;
	char *value;
	item *next;
public:
	item(const char *arg1, const char *arg=NULL);
	~item(void);
	inline void set_next(item *item) { next = item; };
	inline item *get_next(void) { return next; };
	void set_tag(const char *str);
	const char *set_value(const char *fmt,...);
	inline const char *get_tag(void) { return tag; };
	inline const char *get_value(void) { return value; };
	inline int compare_tag(const char *str) { strcmp(tag,str); };
	inline int compare_value(const char *str) { strcmp(value,str); };
	int compare_valuef(const char *fmt,...);
	inline bool is_last(void) { return next==NULL; };
};

class message {
private:
	item *data;
public:
	message(const char *buffer=NULL);
	~message(void);
public:
	void clear(void);
	const char *get(const char *tag); // retrieve item identified by tag
	item *set(const char *tag,const char *format,...);
	void remove(const char *tag);
	item *add(const char *tag,const char *format,...);
	item *find_tag(const char *tag);
	item *find_value(const char *format,...);
	inline item *get_first(void) { return data; };
	int read(const char *str, char *errormsg=NULL, size_t len=0);
	size_t write(char *str, size_t len);
	void dump(void);
};

#endif // _MESSAGE_H
