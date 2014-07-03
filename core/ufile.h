/* $Id: ufile.h 4738 2014-07-03 00:55:39Z dchassin $
 * Copyright (C) 2012 Battelle Memorial Institute
 */

#ifndef _UFILE_H
#define _UFILE_H

typedef struct s_ufile {
	enum {UFT_NONE, UFT_FILE, UFT_HTTP} type;
	void *handle;
} UFILE;

UFILE *uopen(char *fname, void *arg);
size_t uread(void *buffer, size_t count, UFILE *rp);

#endif
