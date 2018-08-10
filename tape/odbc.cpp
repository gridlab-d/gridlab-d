/* $Id: odbc.c 4738 2014-07-03 00:55:39Z dchassin $
 *	Copyright (C) 2008 Battelle Memorial Institute
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "tape.h"
#include "odbc.h"

/*******************************************************************
 * players
 */
int odbc_open_player(player *my, char *fname, char *flags)
{
    return 0;
}

char *odbc_read_player(player *my,char *buffer,unsigned int size)
{
    return NULL;
}

int odbc_rewind_player(player *my)
{
    return 0;
}

void odbc_close_player(player *my)
{
}

/*******************************************************************
 * shaper generators
 */
int odbc_open_shaper(shaper *my, char *fname, char *flags)
{
    return 0;
}

char *odbc_read_shaper(shaper *my,char *buffer,unsigned int size)
{
    return NULL;
}

int odbc_rewind_shaper(shaper *my)
{
    return 0;
}

void odbc_close_shaper(shaper *my)
{
}

/*******************************************************************
 * recorders
 */
int odbc_open_recorder(recorder *my, char *fname, char *flags)
{
    return 0;
}

int odbc_write_recorder(recorder *my, char *timestamp, char *value)
{
    return 0;
}

void odbc_close_recorder(recorder *my)
{
}

/*******************************************************************
 * collectors
 */
int odbc_open_collector(collector *my, char *fname, char *flags)
{
    return 0;
}

int odbc_write_collector(collector *my, char *timestamp, char *value)
{
    return 0;
}

void odbc_close_collector(collector *my)
{
}
