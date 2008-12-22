/* locale.c
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
	char *tz = getenv("TZ");
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
		else
			tzset();
		free(next);
	}
}
