// $Id: sync_ctrl.h
// Copyright (C) 2020 Battelle Memorial Institute

#ifndef GLD_GENERATORS_SYNC_CTRL_H_
#define GLD_GENERATORS_SYNC_CTRL_H_

#include "generators.h"

EXPORT SIMULATIONMODE interupdate_sync_ctrl(OBJECT *, unsigned int64, unsigned long, unsigned int);

class sync_ctrl : public gld_object
{
public:
    static CLASS *oclass;

public:
    sync_ctrl(MODULE *mod);
    int isa(char *classname);

    int create(void);
    int init(OBJECT *parent = nullptr);

    TIMESTAMP presync(TIMESTAMP, TIMESTAMP);
    TIMESTAMP sync(TIMESTAMP, TIMESTAMP);
    TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

    SIMULATIONMODE inter_deltaupdate_sync_ctrl(unsigned int64, unsigned long, unsigned int);

private: //Utility Member Funcs (@TODO: These should be moved to an independent file as regular functions instead of memerber functions, once it is permitted to set up a file. Another option is to improve the package/implementtation of gld_property, but it may affect too much.)
    // gld_property *get_prop_ptr(char *, bool (gld_property::*)(), bool (gld_property::*)());
    template <class T>
    gld_property *get_prop_ptr(T *, char *, bool (gld_property::*)(), bool (gld_property::*)());

    template <class T, class T1>
    T get_prop_value(T1 *, char *, bool (gld_property::*)(), bool (gld_property::*)(), T (gld_property::*)());
    template <class T>
    T get_prop_value(char *, bool (gld_property::*)(), bool (gld_property::*)(), T (gld_property::*)());
    template <class T>
    T get_prop_value(gld_property *, T (gld_property::*)());

private: //Init & Check Member Funcs
    /* Mainly used in create() */
    void init_vars();
    void init_pub_prop();

    /* Mainly used in init() */
    void data_sanity_check(OBJECT *par);
    void deltamode_check();
    // void init_nom_values(OBJECT *par = nullptr);
    // void init_sensors(OBJECT *par = nullptr);

private: //QSTS
    /* Pre-sync */
    void deltamode_reg();

private: //Published Properties
    //==Flag
    bool arm_flag;

    //==Object
    OBJECT *sck_obj_pt;
    OBJECT *cgu_obj_pt;

    //==Tolerance
    double sct_freq_tol_ub_hz;
    double sct_freq_tol_lb_hz;
    double sct_volt_mag_tol_pu;

    //==Time
    double pp_t_ctrl_sec;
    double pp_t_mon_sec;

    //==Controller
    double pi_freq_kp;
    double pi_freq_ki;
    double pi_volt_mag_kp;
    double pi_volt_mag_ki;

private: //Variables
    //==Flags & Status
    bool mode_flag; //Indicates whether in mode A or not: True - This object is working in mode A, False - This object is working in mode B.
    enum SWT_STATUS_ENUM
    {
        OPEN = 0,
        CLOSED = 1
    } swt_status;
    bool sck_armed_status; //Action functionality status of the specified sync_check object of this sync_ctrl object. Valid states are: True - This sync_check object is functional, False - This sync_check object is disabled.

    bool reg_dm_flag;         // Flag for indicating the registration of deltamode (array & func)
    bool deltamode_inclusive; // Boolean for deltamode calls - pulled from object flags

    //==Time
    double timer_mode_A_sec; //The total period (initialized as 0) during which both metrics have been satisfied continuously when this sync_ctrl object is in mode A and PI controllers are working
    double timer_mode_B_sec; //The total period (initialized as 0) during which the both metrics have been satisfied continuously when this sync_ctrl object is in mode B and monitoring.
    double dt_dm_sec;        //Current deltamode timestep.

    //==System Info
    double nom_freq_hz;

    //==Controller
};

#endif // GLD_GENERATORS_SYNC_CTRL_H_
