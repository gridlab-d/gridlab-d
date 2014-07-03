/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
**/

#ifndef GLD_MATCH
#define GLD_MATCH

int match(char *regexp, char *text);
int matchhere(char *regexp, char *text);
int matchstar(int c, char *regexp, char *text);

#endif

/* EOF */
