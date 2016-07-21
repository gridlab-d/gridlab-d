// $Id: kill.cpp 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#include <stdlib.h>
#include <ctype.h>

extern "C" {
#define KILLONLY
#include "kill.c"
}

int main(int argc, char* argv[])
{
	unsigned short pid;
	unsigned int sig;
	if (argc!=3)
	{
		fprintf(stderr,"Syntax: kill -[<signum>|<signame>] <pid>\n");
		return(1);
	}
	if (isalpha(argv[1][2]))
	{
		if (strcmp(argv[1],"-SIGINT")==0 || strcmp(argv[1],"-INT")==0)
			sig = SIGINT;
		else if (strcmp(argv[1],"-SIGTERM")==0 || strcmp(argv[1],"-TERM")==0)
			sig = SIGTERM;
		else
		{
			fprintf(stderr,"kill: signal %s is not recognized\n", argv[1]);
			exit(1);
		}
	}
	else
		sig = -atoi(argv[1]);
	pid = (unsigned short)atoi(argv[2]);

	kill(pid,sig);
	printf("\n");
}

