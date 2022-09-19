/**
 * $Id: sync_ctrl.h
 * Copyright (C) 2020 Battelle Memorial Institute
**/

#ifndef GLD_GENERATORS_SYNC_CTRL_H_
#define GLD_GENERATORS_SYNC_CTRL_H_

#include "generators.h"

EXPORT int isa_sync_ctrl(OBJECT *obj, char *classname);
EXPORT SIMULATIONMODE interupdate_sync_ctrl(OBJECT *, unsigned int64, unsigned long, unsigned int);

class pid_ctrl;
class sync_ctrl : public gld_object
{
public:
    static CLASS *oclass;

public:
    sync_ctrl(MODULE *mod);
    int isa(char *classname);

    int create(void);
    int init(OBJECT * = nullptr);

    TIMESTAMP presync(TIMESTAMP, TIMESTAMP);
    TIMESTAMP sync(TIMESTAMP, TIMESTAMP);
    TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

    SIMULATIONMODE inter_deltaupdate_sync_ctrl(unsigned int64, unsigned long, unsigned int);

private: //Member Funcs: Utilities
    /* Get property pointer */
    template <class T>
    gld_property *get_prop_ptr(T *, const char *, bool (gld_property::*)(), bool (gld_property::*)());

    /* Get property value */
    template <class T, class T1>
    T get_prop_value(T1 *, const char *, bool (gld_property::*)(), bool (gld_property::*)(), T (gld_property::*)());
    template <class T, class T1>
    T *get_prop_value(T1 *, const char *, bool (gld_property::*)(), bool (gld_property::*)(), T *(gld_property::*)());
    template <class T>
    T get_prop_value(const char *, bool (gld_property::*)(), bool (gld_property::*)(), T (gld_property::*)());
    template <class T>
    T get_prop_value(gld_property *, T (gld_property::*)(), bool = true);
    template <class T>
    T *get_prop_value(gld_property *, T *(gld_property::*)(), bool = true);

    /* Set & get property value */
    template <class T>
    void set_prop(gld_property *, T);

    template <class T> // This one was created to avoid modifying gridlabd.h (note that func get_bool() was not implemented in the gridlabd.h, but had been added later on)
    void get_prop(gld_property *, T);

private: //Member Funcs: Init, Sanity Check, and Reset
    /* Mainly used in create() */
    void init_vars();
    void init_pub_prop();
    void init_hidden_prop(double = -1);
    void init_hidden_prop_controllers(double = -1);

    /* Mainly used in init() */
    void init_data_sanity_check();
    void init_deltamode_check();

    void init_nom_values();
    void init_sensors();
    void init_controllers();

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

    void cgu_ctrl(double);

    void dm_data_sanity_check(); //Parameter/data sanity check

    void dm_reset_after_disarmed(); //Reset once sync_ctrl is disarmed
    void dm_reset_controllers();    //Reset both controllers

private: //Published (Hidden) Properties
    /* Signals & flags for controllers */
    bool sct_volt_cv_arm_flag; //True - apply the volt controlled variable, False - do not set the related property
    double cgu_volt_set_mpv;
    double cgu_volt_set_cv;

    bool sct_freq_cv_arm_flag; //True - apply the freq controlled variable, False - do not set the related property
    double cgu_freq_set_mpv;
    double cgu_freq_set_cv;

private: //Published (Public) Properties
    //==Flag
    bool sct_armed_flag;

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
    double pi_freq_ub_pu;
    double pi_freq_lb_pu;

    double pi_volt_mag_kp;
    double pi_volt_mag_ki;
    double pi_volt_mag_ub_pu;
    double pi_volt_mag_lb_pu;

private: //Variables
    //==Flags & Status
    enum class SCT_MODE_ENUM
    {
        MODE_A,
        MODE_B
    } mode_status; //Indicates whether in mode A or mode B.

    enum SWT_STATUS_ENUM
    {
        OPEN = 0,
        CLOSED = 1
    } swt_status;

    bool sck_armed_status;    //Action functionality status of the specified sync_check object of this sync_ctrl object. Valid states are: True - This sync_check object is functional, False - This sync_check object is disabled.
    bool reg_dm_flag;         //Flag for indicating the registration of deltamode (array & func)
    bool deltamode_inclusive; //Boolean for deltamode calls - pulled from object flags

    //==Time
    double timer_mode_A_sec; //The total period (initialized as 0) during which both metrics have been satisfied continuously when this sync_ctrl object is in mode A and PI controllers are working
    double timer_mode_B_sec; //The total period (initialized as 0) during which the both metrics have been satisfied continuously when this sync_ctrl object is in mode B and monitoring.

    //==System Info
    double sys_nom_freq_hz;

    //==Controller
    pid_ctrl *pi_ctrl_cgu_volt_set;
    pid_ctrl *pi_ctrl_cgu_freq_set;

    bool pi_ctrl_cgu_volt_set_fsu_flag; //fsu: first step update
    bool pi_ctrl_cgu_freq_set_fsu_flag;

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

    /* cgu */
    gld_property *prop_cgu_P_f_droop_setting_mode_ptr;
    enum class PF_DROOP_MODE
    {
        FSET_MODE = 0,
        PSET_MODE = 1
    } cgu_P_f_droop_setting_mode;

    enum class CGU_TYPE
    {
        UNKNOWN_CGU_TYPE = 0,
        DG = 1,
        INV = 2
    } cgu_type;

    const char *prop_cgu_volt_set_name_cc_ptr;
    gld_property *prop_cgu_volt_set_ptr;
    const char *prop_cgu_freq_set_name_cc_ptr;
    gld_property *prop_cgu_freq_set_ptr;
};

/* ================================================
PID Controller
================================================ */
class pid_ctrl
{
private:
    double kp;
    double ki;
    double kd;

    double dt;
    double cv_max;
    double cv_min;

    double cv_init;

    double pre_ev;
    double integral;

public:
    /*
        kp: proportional gain factor
        ki: integral gain factor
        kd: derivative gain facor
        dt: time interval
        cv_max: upper bound of the control variable
        cv_min: lower bound of the control variable
        cv_init: initial value of the control variable (e.g., when error starts with 0)
    */
    pid_ctrl(double kp, double ki, double kd, double dt = 0, double cv_max = 1, double cv_min = 0, double cv_init = 0);
    ~pid_ctrl();

    //Update the cv_init
    void set_cv_init(double);

    //Returns the control variable, with respect to the setpoint and measured process value as inputs
    double step_update(double setpoint, double mpv, double cur_dt = 0);
};

#endif // GLD_GENERATORS_SYNC_CTRL_H_
