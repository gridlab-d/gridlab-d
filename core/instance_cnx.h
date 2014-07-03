#ifndef _INSTANCE_CNX_H
#define _INSTANCE_CNX_H

#include <math.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef WIN32
#define _WIN32_WINNT 0x0400
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include <pthread.h>

#include "property.h"
#include "timestamp.h"
#include "pthread.h"
#include "linkage.h"
#include "output.h"
#include "globals.h"
#include "random.h"
#include "exec.h"
#include "instance.h"

#include "gld_sock.h"

STATUS instance_connect(instance *inst);

#endif //	_INSTANCE_CNX_H