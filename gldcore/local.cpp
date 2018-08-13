/* $Id: local.c 4738 2014-07-03 00:55:39Z dchassin $
 * Copyright (C) 2008 Battelle Memorial Institute
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "output.h"
#include "local.h"

#ifdef WIN32
#define tzset _tzset
#endif

static LOCALE *stack=NULL;
void locale_push(void)
{
	char *tz = timestamp_current_timezone();
	LOCALE *locale = (LOCALE*)malloc(sizeof(LOCALE));
	if (locale==NULL)
	{
		output_error("locale push failed; no memory");
		return;
	}
	else
	{
		locale->next=stack;
		stack=locale;
		if (tz==NULL)
			output_warning("locale TZ is empty");
			/* TROUBLESHOOT
				This warning indicates that the TZ environment variable has not be set.  
				This variable is used to specify the default timezone to use while
				GridLAB-D is running.  Supported timezones are listed in the 
				<a href="http://gridlab-d.svn.sourceforge.net/viewvc/gridlab-d/trunk/core/tzinfo.txt?view=markup">tzinfo.txt</a>
				file.
			 */
		strncpy(locale->tz,tz?tz:"",sizeof(locale->tz));
		return;
	}
}

void locale_pop(void)
{
	if (stack==NULL)
	{
		output_error("locale pop failed; stack empty");
		return;
	}
	else
	{
		LOCALE *next = stack;
		char tz[32];
		stack = stack->next;
		sprintf(tz,"TZ=%s",next->tz);
		if (putenv(tz)!=0)
			output_warning("locale pop failed");
			/* TROUBLESHOOT
				This is an internal error causes by a corrupt locale stack.  
			 */
		else
			tzset();
		free(next);
	}
}
