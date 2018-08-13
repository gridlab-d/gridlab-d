/** $Id: exception.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file exception.c
	@addtogroup exception Exception handling
	@ingroup core
	
	Exception handlers are created and caught using the exception module

	Note that the exception handlers are only necessary for C code.  This
	will usually by something like this:

	@code
	TRY {
		// some code...
		if (errno)
			THROW("Error %d: %s", errno, strerror(errno));
		// more code...
	} CATCH(char *msg) {
		output_error("Exception caught: %s", msg);
	} ENDCATCH
	@endcode
	
	In C++ code, you can use THROW() to throw an exception that is
	to be caught by the main system exception handler. 

	The recommended format for exception messages is as follows:

	- <b>Core exceptions</b> should include the offending function
	  call and the specifics of the error in context, as in	
		\code 
	  	THROW("object_tree_add(obj='%s:%d', name='%s'): memory allocation failed (%s)", 
			obj->oclass->name, obj->id, name, strerror(errno));
		\endcode
	  For functions that a \p static, you should include the file in which
	  it is implemented, as in
		\code
		THROW("object.c/addto_tree(tree.name='%s', item.name='%s'): strcmp() returned unexpected value %d", tree->name, item->name, rel);
		\endcode
	  One important situation to consider are exception that occur during I/O
	  where the context in GridLAB may not be as important the context in the
	  I/O stream, as in
		\code
		THROW("%s(%d): unexpected EOF", filename, linenum);
		\endcode

	- <b>Module exception</b> should include the offending object as well
	  as information about the location and nature of the exception, as in
		\code
		GL_THROW("%s:%d circuit %d has an invalid circuit type (%d)", obj->oclass->name, obj->id, c->id, (int)c->type);
		\endcode

 @{
 **/

#include <string.h>
#include "exec.h"
#include "exception.h"
#include "output.h"

EXCEPTIONHANDLER *handlers = NULL;

/** Creates an exception handler for use in a try block 
	@return a pointer to an EXCEPTIONHANDLER structure
 **/
EXCEPTIONHANDLER *create_exception_handler(void)
{
	EXCEPTIONHANDLER *ptr = malloc(sizeof(EXCEPTIONHANDLER));
	if(ptr == NULL){
		output_fatal("create_exception_handler(): malloc failure");
		return NULL;
	}
	ptr->next = handlers;
	ptr->id = (handlers==NULL?0:handlers->id)+1;
	memset(ptr->msg,0,sizeof(ptr->msg));
	handlers = ptr;
	return ptr;
}

/** Deletes an exception handler from the handler list
 **/
void delete_exception_handler(EXCEPTIONHANDLER *ptr) /**< a pointer to the exception handler */
{
	EXCEPTIONHANDLER *target;
	if(ptr == NULL){
		output_fatal("delete_exception_handler(): ending an exception handler block where no exception handler was present");
		return;
	}
	target = ptr->next;
	while (handlers!=target)
	{
		ptr = handlers;
		handlers=ptr->next;
		free(ptr);
		ptr = NULL;
		/* if(handlers == NULL) break; */
	}
}

/** Throw an exception
 **/
void throw_exception(char *format, /**< the format string */
					 ...) /**< the parameters of the message */
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,format,ptr);
	va_end(ptr);

	if (handlers)
	{
		strncpy(handlers->msg,buffer,sizeof(handlers->msg));
		// do not use output_* because they use functions that can throw exception
		//fprintf(stderr,"EXCEPTION: %s\n", buffer);
		longjmp(handlers->buf,handlers->id);
	}
	else
	{
		// do not use output_* because they use functions that can throw exception
		fprintf(stderr,"UNHANDLED EXCEPTION: %s\n", buffer);
		/*	TROUBLESHOOT
			An exception was generated that can't be handled by
			the system.  This usually occurs because some part
			of a module or external library isn't properly
			compiled or linked.
		*/
		exit(XC_EXCEPTION);
	}
}

/** Retrieves the message of the most recently thrown exception
	@return a \e char* pointer to the message
 **/
char *exception_msg(void)
{
	return handlers->msg;
}

/**@}*/
