/* $Id: ufile.c 4738 2014-07-03 00:55:39Z dchassin $
 * Copyright (C) 2012 Battelle Memorial Institute
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "http_client.h"
#include "ufile.h"

UFILE *uopen(char *fname, void *arg)
{
	UFILE *rp = NULL;
	errno = 0;
	if ( strncmp(fname,"http://",7)==0)
	{
		HTTP *http = hopen(fname,*(int*)arg);
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
		return hread(static_cast<char *>(buffer), count, static_cast<HTTP *>(rp->handle));
	case UFT_FILE:
		return fread(buffer, 1, count, static_cast<FILE *>(rp->handle));
	default:
		return static_cast<size_t>(-1);
	}
}
