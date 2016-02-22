/// $Id$
/// @file message.h
/// @addtogroup connection
/// @{

#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <stdlib.h>

class message {
private:
	size_t len;
	char *buffer;
public:
	inline message(void) { buffer=NULL; len=0; };
	inline void set(char *ptr, size_t n=0) ///< set the message to a cache position
		{ buffer=ptr; len=(n>0?n:strlen(ptr)); };
	inline size_t get_length(void) ///< get the length of the message
		{ return len; };
	inline char * get_buffer(void) ///< get the text of the message
		{ return buffer; };
};

#endif /// @} _MESSAGE_H
