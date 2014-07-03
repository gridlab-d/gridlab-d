/* $Id: ufile.c 4738 2014-07-03 00:55:39Z dchassin $
 * Copyright (C) 2012 Battelle Memorial Institute
 */

#include <stdio.h>
#include "http_client.h"
#include "ufile.h"

UFILE *uopen(char *fname, void *arg)
{
	UFILE *rp = NULL;
	errno = 0;
	if ( strncmp(fname,"http://",7)==0)
	{

		HTTP *http = hopen(fname,(size_t)arg);
		if ( http==NULL ) return NULL;
		rp = (UFILE*)malloc(sizeof(UFILE));
		if ( rp==NULL ) 
		{
			hclose(http);
			return NULL;
		}
		rp->type = UFT_HTTP;
		rp->handle = http;
		return rp;
	}
	else
	{
		FILE *fp = fopen(fname,(char*)arg);
		if ( fp==NULL ) return NULL;
		rp = (UFILE*)malloc(sizeof(UFILE));
		if ( rp==NULL )
		{
			fclose(fp);
			return NULL;
		}
		rp->type = UFT_FILE;
		rp->handle = fp;
		return rp;
	}
}

size_t uread(void *buffer, size_t count, UFILE *rp)
{
	switch( rp->type ) {
	case UFT_HTTP:
		return hread(buffer,count,rp->handle);
		break;
	case UFT_FILE:
		return fread(buffer,1,count,rp->handle);
		break;
	default:
		return -1;
		break;
	}
}
