#ifndef _INSTANCE_CNX_H
#define _INSTANCE_CNX_H

#include <math.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef _WIN32
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// #include <pthread.h>

#include "exec.h"
#include "gldrandom.h"
#include "globals.h"
#include "instance.h"
#include "linkage.h"
#include "output.h"
#include "property.h"
#include "timestamp.h"

#include "gld_sock.h"

// STATUS instance_connect(instance *inst);

#endif //	_INSTANCE_CNX_H
