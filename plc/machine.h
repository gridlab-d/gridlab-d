/* $Id: machine.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	Defines the PLC machine
 */

#ifndef _MACHINE_H
#define _MACHINE_H

#include "gridlabd.h"
#include "comm.h"

typedef struct s_data {char *name; PROPERTYTYPE type; void *addr;} *PLCDATA;
typedef int (*PLCINIT)();
/* this structure must match the PLCDEV structure in plc/rt/include/plc.h */
typedef struct {
	unsigned int time;
	struct {
		void (*send)(class machine*,char*,void*,unsigned int);
		int (*recv)(class machine*,char *,void*,unsigned int);
	} comm;
} PLCDEV;
typedef int (*PLCCODE)(void*,double,PLCDEV*);
extern int load_library(char *, PLCCODE*, PLCINIT*, PLCDATA*);

class machine : gld_object {
private:
	PLCDATA _data;	///< machine data block
	PLCINIT _init;	///< machine init code
	PLCCODE _code;	///< machine run code
	Queue _link;	///< incoming message queue
	class comm *net;		///< network interface
	int wait;
public:
	machine();
	~machine();
public:
	void send(Message *msg);
	Message *receive(void);
protected:
	int compile(char *source);
	int init(OBJECT *parent);
	int run(double dt);
	void connect(comm *ptr);
	void deliver(Message *msg);
	void send(char *to, void *msg, size_t len=0);

	friend class plc;
	friend class comm;
};

#endif
