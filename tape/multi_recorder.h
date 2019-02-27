//
// Created by lauraleist on 9/26/18.
//

#ifndef _MULTI_RECORDER_H
#define _MULTI_RECORDER_H


#include "tape.h"

EXPORT TIMESTAMP sync_multi_recorder(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass);
EXPORT TIMESTAMP sync_multi_recorder_error(OBJECT **obj, struct recorder **my, char2048 buffer);

#endif //_MULTI_RECORDER_H
