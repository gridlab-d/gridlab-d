/* $Id: odbc.c 4738 2014-07-03 00:55:39Z dchassin $
 *	Copyright (C) 2008 Battelle Memorial Institute
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "odbc.h"
#include "tape.h"

/*******************************************************************
 * players
 */
int odbc_open_player(struct player *my, char *fname, char *flags) { return 0; }

char *odbc_read_player(struct player *my, char *buffer, unsigned int size) {
  return nullptr;
}

int odbc_rewind_player(struct player *my) { return 0; }

void odbc_close_player(struct player *my) {}

/*******************************************************************
 * shaper generators
 */
int odbc_open_shaper(struct shaper *my, char *fname, char *flags) { return 0; }

char *odbc_read_shaper(struct shaper *my, char *buffer, unsigned int size) {
  return nullptr;
}

int odbc_rewind_shaper(struct shaper *my) { return 0; }

void odbc_close_shaper(struct shaper *my) {}

/*******************************************************************
 * recorders
 */
int odbc_open_recorder(struct recorder *my, char *fname, char *flags) {
  return 0;
}

int odbc_write_recorder(struct recorder *my, char *timestamp, char *value) {
  return 0;
}

void odbc_close_recorder(struct recorder *my) {}

/*******************************************************************
 * collectors
 */
int odbc_open_collector(struct collector *my, char *fname, char *flags) {
  return 0;
}

int odbc_write_collector(struct collector *my, char *timestamp, char *value) {
  return 0;
}

void odbc_close_collector(struct collector *my) {}
