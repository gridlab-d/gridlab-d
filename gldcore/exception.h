/** $Id: exception.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file exception.h
	@addtogroup exception
	@ingroup core
@{
 **/

#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>

#ifndef __cplusplus
#define TRY { EXCEPTIONHANDLER *_handler = create_exception_handler(); if (_handler==NULL) output_error("%s(%d): core exception handler creation failed",__FILE__,__LINE__); else if (setjmp(_handler->buf)==0) {
/* TROUBLESHOOT
	This error is caused when the system is unable to implement an exception handler for the core. 
	This is an internal error and should be reported to the core development team.
 */
#define THROW(...) throw_exception(__VA_ARGS__);
#define CATCH(X) } else {X = exception_msg();
#define ENDCATCH } delete_exception_handler(_handler);}
#endif

typedef struct s_exception_handler {
	int id; /**< the exception handler id */
	jmp_buf buf; /**< the \p jmpbuf containing the context for the exception handler */
	char msg[1024]; /**< the message thrown */
	struct s_exception_handler *next; /**< the next exception handler */
} EXCEPTIONHANDLER; /**< the exception handler structure */

#ifdef __cplusplus
extern "C" {
#endif

	EXCEPTIONHANDLER *create_exception_handler();
	void delete_exception_handler(EXCEPTIONHANDLER *ptr);
	void throw_exception(char *msg, ...);
	char *exception_msg(void);

#ifdef __cplusplus
}
#endif

#endif

/**@}*/
