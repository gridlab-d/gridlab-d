// $Id$
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TAPE_ODBC_H
#define _TAPE_ODBC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "gridlabd.h"
#include "../tape/tape.h"
#include "../tape/file.h"
#include "ODBCTapeStream.h"

EXPORT int open_player(struct player *my, char *fname, char *flags);
EXPORT char *read_player(struct player *my,char *buffer,unsigned int size);
EXPORT int rewind_player(struct player *my);
EXPORT void close_player(struct player *my);

EXPORT int open_shaper(struct shaper *my, char *fname, char *flags);
EXPORT char *read_shaper(struct shaper *my,char *buffer,unsigned int size);
EXPORT int rewind_shaper(struct shaper *my);
EXPORT void close_shaper(struct shaper *my);

EXPORT int open_recorder(struct recorder *my, char *fname, char *flags);
EXPORT int write_recorder(struct recorder *my, char *timestamp, char *value);
EXPORT void close_recorder(struct recorder *my);

EXPORT int open_collector(struct collector *my, char *fname, char *flags);
EXPORT int write_collector(struct collector *my, char *timestamp, char *value);
EXPORT void close_collector(struct collector *my);

#endif
