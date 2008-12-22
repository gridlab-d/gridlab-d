/*	$id$
	Copyright (C) 2008 Battelle Memorial Institute

 *	Abstract header for tape streams.
 *
 *	author: Matt Hauer, matthew.hauer@pnl.gov, 5/30/07 - ***
 */

#ifndef _CTAPESTREAM_H
#define _CTAPESTREAM_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//#include "tape.h"
typedef enum {TSO_INIT, TSO_OPEN, TSO_DONE, OS_ERROR} TAPEOBJSTATUS;

//	abstract class
class TapeStream{
public:
	TapeStream();
	virtual ~TapeStream();

	virtual char *ReadLine(){return 0;}
	virtual char *ReadLine(char *, unsigned int)=0;
	virtual int ReadShape(char *, float *)=0;
	virtual int Write(char*)=0;
	virtual int Write(char*, char*)=0;
	//int Write(int, OBJECT *);
	virtual void Close()=0;
	virtual int Rewind()=0;
	void Reset();

	virtual void PrintHeader(char *, char *, char *, char *, char *, char *, long, long)=0;

//	TAPESTATUS GetStreamState(){return state;}
	int GetStreamState(){return (int)state;}
	char *GetFileMode(){return filemode;}
	void SetFileMode(char *);
protected:
	int		state;
	//TAPESTATUS	state;
	char	filemode[3];
};

class tapepair{
public:
	tapepair(){tape=0; name=0;}
	tapepair(TapeStream *t, void *n){tape=t; name=n;}
	~tapepair(){delete tape;}
	TapeStream *tape;
	void *name;
};	//	glorified struct

#endif
