//
// Created by lauraleist on 9/26/18.
//

#ifndef TAPE_RECORDER_H
#define TAPE_RECORDER_H

#include "property.h"
#include "tape.h"


/* recorder-specific enums */
typedef enum {HU_DEFAULT, HU_ALL, HU_NONE} HEADERUNITS;
typedef enum {LU_DEFAULT, LU_ALL, LU_NONE} LINEUNITS;

/** @}
	@addtogroup recorder
	@{
 **/
class recorder : public gld_object {
public:
    static recorder *defaults;
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
public: //Does this need to be Private?
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

extern int read_properties(struct recorder *my, OBJECT *obj, PROPERTY *prop, char *buffer, int size);
EXPORT TIMESTAMP sync_recorder(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass);
EXPORT TIMESTAMP sync_recorder_error(OBJECT **obj, struct recorder **my, char2048 buffer);

#endif //_RECORDER_H
