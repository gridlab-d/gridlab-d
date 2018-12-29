//
// Created by lauraleist on 9/26/18.
//

#ifndef TAPE_PLAYER_H
#define TAPE_PLAYER_H

#include "property.h"
#include "tape.h"

/** @}
  @addtogroup player
	@{
 **/
struct player {
    /* public */
    char1024 file; /**< the name of the player source */
    char8 filetype; /**< the type of the player source */
    char256 mode;
    char256 property; /**< the target property */
    int32 loop; /**< the number of time to replay the tape */
    /* private */
    FILETYPE type;
    union {
        FILE *fp;
        MEMORY *memory;
        void *tsp;
        /** add handles for other type of sources as needed */
    };
    TAPESTATUS status;
    int32 loopnum;
    struct {
        TIMESTAMP ts;
        int64 ns;
        char1024 value;
    } next;
    struct {
        TIMESTAMP ts;
        TIMESTAMP ns;
        char1024 value;
    } delta_track;	/* Added for deltamode fixes */
    PROPERTY *target;

    TAPEOPS *ops;
    char lasterr[1024];
}; /**< a player item */

extern TIMESTAMP player_read(OBJECT *obj);

#endif //_PLAYER_H