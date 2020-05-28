// $Id: sync_ctrl.h
// Copyright (C) 2020 Battelle Memorial Institute

#ifndef GLD_GENERATORS_SYNC_CTRL_H_
#define GLD_GENERATORS_SYNC_CTRL_H_

#include "generators.h"

EXPORT SIMULATIONMODE interupdate_sync_ctrl(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class sync_ctrl : public gld_object
{
public:
    static CLASS *oclass;

public:
    sync_ctrl(MODULE *mod);

    int create(void);
    int init(OBJECT *parent = NULL);

    TIMESTAMP presync(TIMESTAMP, TIMESTAMP);
    TIMESTAMP sync(TIMESTAMP, TIMESTAMP);
    TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

    SIMULATIONMODE inter_deltaupdate_sync_ctrl(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

private:
    bool arm_flag;
};

#endif // GLD_GENERATORS_SYNC_CTRL_H_
