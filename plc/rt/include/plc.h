/* $Id: plc.h 4738 2014-07-03 00:55:39Z dchassin $
 * Copyright (C) 2008 Battelle Memorial Institute
 * PLC runtime header 
 */

#define enumeration int
#define BEGIN_DATA struct { char *name; enum {DT_INTEGER=6,DT_DOUBLE=1,DT_ENUM=3,DT_SET=4,DT_BOOL=14} type; void *addr; } data[] = {
typedef void (*SENDFN)(void *src,char *to,void *msg,unsigned int sz);
typedef int (*RECVFN)(void *dst,char *from,void *data, unsigned int sz);
#define INTEGER(N) {#N,DT_INTEGER,0},
#define DOUBLE(N) {#N,DT_DOUBLE,0},
#define ENUM(N) {#N,DT_ENUM,0},
#define SET(N) {#N,DT_SET,0},
#define BOOL(N) {#N,DT_BOOL,0},
#define END_DATA {0}};
#define DATA(T,N) (*(T*)(data[N].addr))
#define INIT int init(void)
#define CODE(DT,DEV) int code(void *my, double DT, PLCDEV *DEV)
/* this structure must match the plc/machine.h PLCDEV structure */
typedef struct {
	unsigned int time;
	struct {
		SENDFN send;
		RECVFN recv;
	} net;
} PLCDEV;
#define NEVER (0x7fffffff)
#define SNDMSG(DST,MSG) dev->net.send(my,DST,MSG,0)
#define SNDDAT(DST,PTR,SZ) dev->net.send(my,DST,PTR,SZ)
#define RCVMSG(SRC,BUF) dev->net.recv(my,SRC,BUF,sizeof(BUF)-1)
#define RCVDAT(SRC,BUF,SZ) dev->net.recv(my,SRC,BUF,SZ)
#define TIME dev->time

