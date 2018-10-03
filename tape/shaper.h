//
// Created by lauraleist on 9/26/18.
//

#ifndef TAPE_SHAPER_H
#define TAPE_SHAPER_H

#include "property.h"
#include "tape.h"

//typedef struct s_tape_operations TAPEOPS;
//struct TAPEOPS;
//FILETYPE;

/** @}
	@addtogroup shaper
	@{
 **/
typedef struct s_shapertarget {
    double *addr; /**< the address of the target property */
    TIMESTAMP ts; /**< the current timestamp to shape for */
    double value; /**< the current value to shape */
} SHAPERTARGET; /**< the shaper target structure */

struct shaper {
    /* public */
    char1024 file; /**< the name of the shaper source */
    char8 filetype; /**< the type of the shaper source */
    char32 mode;
    char256 property; /**< the target property */
    char256 group; /**< the object grouping to use in choosing the shape target */
    double magnitude;	/**< the magnitude of a queue event (only used when \p events > 0) */
    double events;		/* the number of queue events per interval (\p events = 0 => direct shape)*/
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
    char lasterr[1024];
    int16 interval;	/* the interval over which events is counted (usually 24) */
    int16 step;		/* the duration of a single step in the shape integral (usually 3600s) */
    double scale;	/* the scaling of the shape over the interval */
    int32 loopnum;
    unsigned char shape[12][31][7][24];
#define SHAPER_QUEUE 0x0001
    unsigned int n_targets;
    SHAPERTARGET *targets;
};

#endif //_SHAPER_H
