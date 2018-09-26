//
// Created by lauraleist on 9/26/18.
//

#ifndef _RECORDER_H
#define _RECORDER_H

#include "tape.h"

/** @}
	@addtogroup recorder
	@{
 **/
struct recorder {
    /* public */
    char1024 file;
    char8 filetype;
    char32 mode;
    char1024 multifile;
    char1024 multitempfile;
    FILE *multifp, *inputfp;
    int16 multirun_ct;
    char1024 multirun_header;
    int16 format; /* 0=YYYY-MM-DD HH:MM:SS; 1=timestamp */
    double dInterval;
    TIMESTAMP interval;
    int32 limit;
    char1024 property;
    char1024 out_property;
    PLOTFILE output; /* {EPS|GIF|JPG|PDF|PNG|SVG} More can be added */
    char1024 plotcommands;
    char32 xdata;
    char32 columns;
    char32 trigger;
    int32 flush;
    /* private */
    RECORDER_MAP *rmap;
    TAPEOPS *ops;
    FILETYPE type;
    HEADERUNITS header_units;
    LINEUNITS line_units;
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
        int64 ns;
        char1024 value;
    } last;
    int32 samples;
    PROPERTY *target;
};

#endif //_RECORDER_H
