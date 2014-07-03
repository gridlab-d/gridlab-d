/* $Id: memory.h 4738 2014-07-03 00:55:39Z dchassin $
 *	Copyright (C) 2008 Battelle Memorial Institute
 */

#ifndef _MEMORY_H
#define _MEMORY_H

struct player;
struct shaper;
struct recorder;
struct collector;

typedef struct s_memory {
	GLOBALVAR *buffer;
	unsigned short index;
} MEMORY;

int memory_open_player(struct player *my, char *fname, char *flags);
char *memory_read_player(struct player *my,char *buffer,unsigned int size);
int memory_rewind_player(struct player *my);
void memory_close_player(struct player *my);

int memory_open_shaper(struct shaper *my, char *fname, char *flags);
char *memory_read_shaper(struct shaper *my,char *buffer,unsigned int size);
int memory_rewind_shaper(struct shaper *my);
void memory_close_shaper(struct shaper *my);

int memory_open_recorder(struct recorder *my, char *fname, char *flags);
int memory_write_recorder(struct recorder *my, char *timestamp, char *value);
void memory_close_recorder(struct recorder *my);

int memory_open_collector(struct collector *my, char *fname, char *flags);
int memory_write_collector(struct collector *my, char *timestamp, char *value);
void memory_close_collector(struct collector *my);

#endif
