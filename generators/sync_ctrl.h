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
    int isa(char *);

    int create(void);
    int init(OBJECT * = nullptr);

    TIMESTAMP presync(TIMESTAMP, TIMESTAMP);
    TIMESTAMP sync(TIMESTAMP, TIMESTAMP);
    TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

    SIMULATIONMODE inter_deltaupdate_sync_ctrl(unsigned int64, unsigned long, unsigned int);

private: //Utility Member Funcs (@TODO: These should be moved to an independent file as regular functions instead of memerber functions, once it is permitted to set up a file. Another option is to improve the package/implementtation of gld_property, but it may affect too much.)
    /* Get property */
    // gld_property *get_prop_ptr(char *, bool (gld_property::*)(), bool (gld_property::*)());
    template <class T>
    gld_property *get_prop_ptr(T *, char *, bool (gld_property::*)(), bool (gld_property::*)());

    /* Get property value */
    template <class T, class T1>
    T get_prop_value(T1 *, char *, bool (gld_property::*)(), bool (gld_property::*)(), T (gld_property::*)());
    template <class T, class T1>
    T *get_prop_value(T1 *, char *, bool (gld_property::*)(), bool (gld_property::*)(), T *(gld_property::*)());
    template <class T>
    T get_prop_value(char *, bool (gld_property::*)(), bool (gld_property::*)(), T (gld_property::*)());
    template <class T>
    T get_prop_value(gld_property *, T (gld_property::*)(), bool = true);
    template <class T>
    T *get_prop_value(gld_property *, T *(gld_property::*)(), bool = true);

    /* Set & get property*/
    template <class T>
    void set_prop(gld_property *, T);

    template <class T> // This one is created to avoid modifying gridlabd.h (note that func get_bool() is not implemented in the gridlabd.h)
    void get_prop(gld_property *, T);

private: //Init & Check Member Funcs
    /* Mainly used in create() */
    void init_vars();
    void init_pub_prop();

    /* Mainly used in init() */
    void init_data_sanity_check();
    void init_deltamode_check();
    void init_nom_values();
    void init_sensors();

    /* For reset */
    void reset_timer();

private: //QSTS
    /* Pre-sync */
    void deltamode_reg();

private: //Deltamode
    /* inter_deltaupdate_sync_ctrl */
    void dm_update_measurements();

    bool sct_metrics_check_mode_A(unsigned long);
    bool sct_metrics_check_mode_B();

    enum class SCT_MODE_ENUM;
    void mode_transition(SCT_MODE_ENUM, bool);

    void cgu_ctrl();

    /* parameter/data sanity check */
    void dm_data_sanity_check();

private: //Published Properties
    //==Flag
    bool arm_flag;

    //==Object
    OBJECT *sck_obj_ptr;
    OBJECT *cgu_obj_ptr;

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
    // bool mode_flag; //Indicates whether in mode A or not: True - This object is working in mode A, False - This object is working in mode B.
    enum class SCT_MODE_ENUM
    {
        MODE_A,
        MODE_B
    } mode_status;

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
    double sys_nom_freq_hz;

    //==Controller

    //==Obj & Prop
    /* switch */
    OBJECT *obj_swt_ptr;
    gld_property *prop_swt_status_ptr;

    /* sync_check */
    double swt_nom_volt_v;

    gld_property *prop_sck_armed_ptr;
    bool sck_armed_flag;

    gld_property *prop_sck_freq_diff_hz_ptr;
    double sck_freq_diff_hz;

    gld_property *prop_sck_volt_A_mag_diff_pu_ptr;
    double sck_volt_A_mag_diff_pu;
    gld_property *prop_sck_volt_B_mag_diff_pu_ptr;
    double sck_volt_B_mag_diff_pu;
    gld_property *prop_sck_volt_C_mag_diff_pu_ptr;
    double sck_volt_C_mag_diff_pu;
};

#endif // GLD_GENERATORS_SYNC_CTRL_H_
