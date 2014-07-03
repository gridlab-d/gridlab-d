/* $Id: odbc.h 4738 2014-07-03 00:55:39Z dchassin $
 *	Copyright (C) 2008 Battelle Memorial Institute
 */

#ifndef _ODBC_H
#define _ODBC_H

int odbc_open_player(struct player *my, char *fname, char *flags);
char *odbc_read_player(struct player *my,char *buffer,unsigned int size);
int odbc_rewind_player(struct player *my);
void odbc_close_player(struct player *my);

int odbc_open_shaper(struct shaper *my, char *fname, char *flags);
char *odbc_read_shaper(struct shaper *my,char *buffer,unsigned int size);
int odbc_rewind_shaper(struct shaper *my);
void odbc_close_shaper(struct shaper *my);

int odbc_open_recorder(struct recorder *my, char *fname, char *flags);
int odbc_write_recorder(struct recorder *my, char *timestamp, char *value);
void odbc_close_recorder(struct recorder *my);

int odbc_open_collector(struct collector *my, char *fname, char *flags);
int odbc_write_collector(struct collector *my, char *timestamp, char *value);
void odbc_close_collector(struct collector *my);

#endif
