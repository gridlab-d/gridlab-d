//
// Created by lauraleist on 9/26/18.
//
#ifndef TAPE_COLLECTOR_H
#define TAPE_COLLECTOR_H

#include "property.h"
#include "tape.h"

/** @}
	@addtogroup collector
	@{
 **/
struct collector {
    /* public */
    char1024 file;
    char8 filetype;
    char32 mode;
    int16 format; /* 0=YYYY-MM-DD HH:MM:SS; 1=timestamp */
    double dInterval;
    TIMESTAMP interval;
    int32 limit;
    char1024 property;
    PLOTFILE output; /* {EPS|GIF|JPG|PDF|PNG|SVG} More can be added */
    char1024 plotcommands;
    char32 xdata;
    char32 columns;
    char32 trigger;
    char256 group;
    int32 flush;
    /* private */
    TAPEOPS *ops;
    FILETYPE type;
    union {
        FILE *fp;
        MEMORY *memory;
        void *tsp;
        /** add handles for other type of sources as needed */
    };
    TAPESTATUS status;
    char8 delim;
    struct {
        TIMESTAMP ts;
        char1024 value;
    } last;
    int32 samples;
    AGGREGATION *aggr;
};

EXPORT TIMESTAMP sync_collector(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass);
EXPORT TIMESTAMP sync_collector_error(OBJECT **obj, struct collector **my, char2048 buffer);

#endif //_COLLECTOR_H