// powernet/message.h
// Copyright (C) 2016, Stanford University
// Author: dchassin@slac.stanford.edu

#include "message.h"

#define BSZ 32
#define SIZE(X) (((size_t)(strlen(X)/BSZ))*BSZ)

item::item(const char *arg1, const char *arg2) : tag(NULL), value(NULL), next(NULL)
{
	set_tag(arg1);
	set_value(arg2);
}

item::~item(void)
{
	if ( tag != NULL ) delete [] tag;
	if ( value != NULL ) delete [] value;
}

void item::set_tag(const char *str)
{
	size_t sz = SIZE(str);
	if ( tag == NULL || SIZE(tag) < sz )
	{
		if ( tag != NULL ) delete [] tag;
		tag = new char(sz);
	}
	strcpy(tag,str);
}

const char *item::set_value(const char *fmt,...)
{
	va_list ptr;
	va_start(ptr,fmt);
	char buffer[1024];
	size_t sz = ((size_t)(vsprintf(buffer,fmt,ptr)/BSZ))*BSZ;
	va_end(ptr);
	if ( value == NULL || SIZE(value) < sz )
	{
		if ( value != NULL ) delete [] value;
		value = new char(sz);
	}
	strcpy(value,buffer);
	return value;
}

message::message(const char *buffer) : data(NULL)
{
	read(buffer);
}

message::~message(void)
{
	clear();
}

void message::clear(void)
{
	while ( data != NULL ) {
		item *next = data->get_next();
		delete data;
		data = next;
	}
}

const char *message::get(const char *tag)
{
	item *part;
	for ( part = data ; part != NULL ; part = part->get_next() )
	{
		if ( part->compare_tag(tag)==0 )
			return part->get_value();
	}
	return NULL;
}

item *message::set(const char *tag,const char *format,...)
{
	item *part = find_tag(tag);
	if ( tag == NULL )
		return NULL;
	va_list ptr;
	va_start(ptr,format);
	char value[1024];
	vsprintf(value,format,ptr);
	va_end(ptr);
	part->set_value("%s",value);
	return part;
}
void message::remove(const char *tag)
{
	item *part, *last=NULL;
	for ( part = data ; part != NULL ; last = part, part = part->get_next() )
	{
		if ( part->compare_tag(tag)==0 )
		{
			if ( last != NULL )
				last->set_next(part->get_next());
			delete part;
			return ;
		}
	}
}
item *message::add(const char *tag,const char *format,...)
{
	va_list ptr;
	va_start(ptr,format);
	char value[1024];
	vsprintf(value,format,ptr);
	va_end(ptr);
	item *part = new item(tag,value);
	part->set_next(data);
	data = part;
	return NULL;
}
item *message::find_tag(const char *tag)
{
	item *part;
	for ( part = data ; part != NULL ; part = part->get_next() )
	{
		if ( part->compare_tag(tag)==0 )
			return part;
	}
	return NULL;
}
item *message::find_value(const char *format,...)
{
	va_list ptr;
	va_start(ptr,format);
	char value[1024];
	vsprintf(value,format,ptr);
	va_end(ptr);
	item *part;
	for ( part = data ; part != NULL ; part = part->get_next() )
	{
		if ( part->compare_value(value)==0 )
			return part;
	}
	return NULL;
}
int message::read(const char *str, char *errormsg, size_t len)
{
	item *list = NULL;
	char tag[1024], value[1024], *out=NULL;
	const char *in=str;
	enum {INIT=0, OTAG=1, ITAG=2, ETAG=3, OVAL=4, IVAL=5, EVAL=6, DONE=7} state = INIT;
	size_t pos = 0;
	int count = 0;
	for ( in=str ; *in != NULL ; in++, pos++ )
	{
		printf("message::read(const char *buffer='%s'): pos=%d, in='%c', state=%d",str,pos,*in,state);
		switch ( state ) { // state machine
		case INIT: // starting: ignore white; '{'->OTAG; error.
			if ( isspace(*in) ) {} // ignore whitespace
			else if ( *in == '{' ) state = OTAG;
			else if ( errormsg ) return -snprintf(errormsg,len,"expected '{' at pos %d", pos);
			else return -1;
			break;
		case OTAG: // open tag: ignore white; '"'->ITAG & set output to tag; error.
			if ( isspace(*in) ) {} // ignore whitespace
			else if ( *in == '"' ) { state = ITAG; out = tag; }
			else if ( errormsg ) return -snprintf(errormsg,len,"expected '\"' at pos %d", pos);
			else return -1;
			break;
		case ITAG: // in tag: '"'->ETAG; escape ; copy to output.
			if ( *in == '"' ) state = ETAG; // end tag
			else { // copy
				if ( *in == '\\' ) in++; // escape
				*out++ = *in;
			}
			break;
		case ETAG: // end tag: ignore white; ':'->OVAL; error.
			if ( isspace(*in) ) {} // ignore whitepsace
			else if ( *in == ':' ) state = OVAL;
			else if ( errormsg ) return -snprintf(errormsg,len,"expected ':' at pos %d", pos);
			else return -1;
			break;
		case OVAL: // open value: ignore white; '"'->IVAL & clear buffer; error
			if ( isspace(*in) ) {} // ignore whitespace
			else if ( *in == '"' ) { state = ITAG; out = value; }
			else if ( errormsg ) return -snprintf(errormsg,len,"expected '\"' at pos %d", pos);
			else return -1;
			break;
		case IVAL: // in value: '"'->EVAL & save buffer to value; copy to buffer.
			if ( *in == '"' )
			{
				state = EVAL; // end value
				item *part = new item(tag,value);
				part->set_next(list);
				list = part;
				count++;
			}
			else { // copy
				if ( *in == '\\' ) in++; // escape
				*out++ = *in;
			}
			break;
		case EVAL: // end value: ignore white; ','->OTAG; '}'->DONE; error
			if ( isspace(*in) ) {} // ignore whitepsace
			else if ( *in == ',' ) state = OTAG;
			else if ( *in == '}' ) state = DONE;
			else if ( errormsg ) return -snprintf(errormsg,len,"expected ',' or '}' at pos %d", pos);
			else return -1;
			break;
		case DONE: // return
			list->set_next(data);
			data = list;
			return count;
			break;
		default:
			if ( errormsg ) return -snprintf(errormsg,len,"internal state error at pos %d", pos);
			else return -1;
			break;
		}
		printf(" -> %d\n",state);
	}
}
size_t message::write(char *str, size_t len)
{
	size_t pos = sprintf(str,"%s","{");
	item *part;

	for ( part = data ; pos<len && part != NULL ; part = part->get_next() )
	{
		pos += sprintf(str+pos," \"%s\": \"%s\"", part->get_tag(), part->get_value());
		if ( !part->is_last() )
			pos += sprintf(str+pos," %s",",");
	}
	pos += sprintf(str+pos,"%s","}");
	return pos;
}
void message::dump(void)
{
	printf("{\n");
	item *part;
	for ( part = data ; part != NULL ; part = part->get_next() )
	{
		printf("\t\"%s\" : \"%s\"%s\n", part->get_tag(), part->get_value(), part->is_last()?"":",");
	}
	printf("}\n");
}
