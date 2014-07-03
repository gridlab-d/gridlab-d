// $Id: tape_file.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TAPE_FILE_H
#define _TAPE_FILE_H

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
