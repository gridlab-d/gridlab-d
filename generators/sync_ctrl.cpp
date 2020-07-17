/** 
 * $Id: sync_ctrl.cpp
 * Implements sychronization control functionality for the sychronization check function.
 * Copyright (C) 2020 Battelle Memorial Institute
**/

#include "sync_ctrl.h"

#include <iostream>
#include <cmath>

using namespace std;

/* UTIL MACROS */
#define STR(s) #s

static PASSCONFIG clockpass = PC_BOTTOMUP;

//////////////////////////////////////////////////////////////////////////
// sync_ctrl CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS *sync_ctrl::oclass = nullptr;

sync_ctrl::sync_ctrl(MODULE *mod)
{
    if (oclass == nullptr)
    {
        oclass = gl_register_class(mod, "sync_ctrl", sizeof(sync_ctrl), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_AUTOLOCK);
        if (oclass == nullptr)
            GL_THROW("unable to register object class implemented by %s", __FILE__);

        if (gl_publish_variable(oclass,
                                //==Flag
                                PT_bool, "armed", PADDR(arm_flag), PT_DESCRIPTION, "Flag to arm the synchronization control functionality.",
                                //==Object
                                PT_object, "sync_check_object", PADDR(sck_obj_ptr), PT_DESCRIPTION, "The object reference/name of the sync_check object, which works with this sync_ctrl object.",
                                PT_object, "controlled_generation_unit", PADDR(cgu_obj_ptr), PT_DESCRIPTION, "The object reference/name of the controlled generation unit (i.e., a diesel_dg/inverter_dyn object), which serves as the actuator of the PI controllers of this sync_ctrl object.",
                                //==Tolerance
                                PT_double, "frequency_tolerance_ub_Hz[Hz]", PADDR(sct_freq_tol_ub_hz), PT_DESCRIPTION, "The user-specified tolerance in Hz for checking the upper bound of the frequency metric.",
                                PT_double, "frequency_tolerance_lb_Hz[Hz]", PADDR(sct_freq_tol_lb_hz), PT_DESCRIPTION, "The user-specified tolerance in Hz for checking the lower bound of the frequency metric.",
                                PT_double, "voltage_magnitude_tolerance_pu[pu]", PADDR(sct_volt_mag_tol_pu), PT_DESCRIPTION, "The user-specified tolerance in per unit for the difference in voltage magnitudes for checking the voltage metric.",
                                //==Time
                                PT_double, "controlling_period[s]", PADDR(pp_t_ctrl_sec), PT_DESCRIPTION, "The user-defined period when both metrics are satisfied and this sync_ctrl object works in mode A.",
                                PT_double, "monitoring_period[s]", PADDR(pp_t_mon_sec), PT_DESCRIPTION, "The user-defined period when this sync_ctrl object keeps on monitoring in mode B, if both metrics are not violated and the switch object is not closed.",
                                //==Controller
                                PT_double, "PI_Frequency_Kp", PADDR(pi_freq_kp), PT_DESCRIPTION, "The user-defined proportional gain constant of the PI controller for adjusting the frequency setting.",
                                PT_double, "PI_Frequency_Ki", PADDR(pi_freq_ki), PT_DESCRIPTION, "The user-defined integral gain constant of the PI controller for adjusting the frequency setting.",
                                PT_double, "PI_Freq_Ub_pu[pu]", PADDR(pi_freq_ub_pu), PT_DESCRIPTION, "The upper bound for the Pset/fset in per unit.",
                                PT_double, "PI_Freq_Lb_pu[pu]", PADDR(pi_freq_lb_pu), PT_DESCRIPTION, "The lower bound for the Pset/fset in per unit.",
                                PT_double, "PI_Volt_Mag_Kp", PADDR(pi_volt_mag_kp), PT_DESCRIPTION, "The user-defined proportional gain constant of the PI controller for adjusting the voltage magnitude setting.",
                                PT_double, "PI_Volt_Mag_Ki", PADDR(pi_volt_mag_ki), PT_DESCRIPTION, "The user-defined integral gain constant of the PI controller for adjusting the voltage magnitude setting.",
                                PT_double, "PI_Volt_Mag_Ub_pu[pu]", PADDR(pi_volt_mag_ub_pu), PT_DESCRIPTION, "The upper bound for the Vset.",
                                PT_double, "PI_Volt_Mag_Lb_pu[pu]", PADDR(pi_volt_mag_lb_pu), PT_DESCRIPTION, "The lower bound for the Vset.",
                                //==Hidden ones for checking variables and debugging controls
                                PT_bool, "sct_cv_arm_flag", PADDR(sct_cv_arm_flag), PT_ACCESS, PA_HIDDEN,
                                PT_DESCRIPTION, "True - apply the controlled variable, False - do not set the related property.",
                                PT_double, "dg_volt_set_mpv", PADDR(dg_volt_set_mpv), PT_ACCESS, PA_HIDDEN,
                                PT_DESCRIPTION, "The measured process variable (i.e., the feedback signal).",
                                PT_double, "dg_volt_set_cv", PADDR(dg_volt_set_cv), PT_ACCESS, PA_HIDDEN,
                                PT_DESCRIPTION, "The control variable, i.e., u(t).",
                                PT_double, "dg_freq_set_mpv", PADDR(dg_freq_set_mpv), PT_ACCESS, PA_HIDDEN,
                                PT_DESCRIPTION, "The measured process variable (i.e., the feedback signal).", //@TODO: update the description
                                PT_double, "dg_freq_set_cv", PADDR(dg_freq_set_cv), PT_ACCESS, PA_HIDDEN,
                                PT_DESCRIPTION, "The control variable, i.e., u(t).", //@TODO: update the description
                                nullptr) < 1)
            GL_THROW("unable to publish properties in %s", __FILE__);

        if (gl_publish_function(oclass, "interupdate_controller_object", (FUNCTIONADDR)interupdate_sync_ctrl) == nullptr)
            GL_THROW("Unable to publish sync_ctrl deltamode function");
    }
}

int sync_ctrl::isa(char *classname)
{
    return strcmp(classname, "sync_ctrl") == 0;
}

int sync_ctrl::create(void)
{
    init_vars();
    init_pub_prop();
    init_hidden_prop(0);

    return 1;
}

int sync_ctrl::init(OBJECT *parent)
{
    init_data_sanity_check();
    init_deltamode_check();

    init_nom_values();
    init_sensors();
    init_controllers();

    return 1;
}

TIMESTAMP sync_ctrl::presync(TIMESTAMP t0, TIMESTAMP t1)
{
    deltamode_reg();

    return TS_NEVER; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

TIMESTAMP sync_ctrl::sync(TIMESTAMP t0, TIMESTAMP t1)
{
    return TS_NEVER;
}

TIMESTAMP sync_ctrl::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
    return TS_NEVER; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

// Deltamode call
// Module-level call
SIMULATIONMODE sync_ctrl::inter_deltaupdate_sync_ctrl(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
    if (arm_flag)
    {
        dm_update_measurements();
        // dm_data_sanity_check(); //@TODO: discuss with Frank and see if we want to keep this on

        if (swt_status == SWT_STATUS_ENUM::CLOSED)
        {
            arm_flag = false;
        }
        else
        {
            if (mode_status == SCT_MODE_ENUM::MODE_A)
            {
                //In Mode A
                if (sct_metrics_check_mode_A(dt))
                {
                    mode_transition(SCT_MODE_ENUM::MODE_B, true);
                }
                else
                {
                    if (sck_armed_flag)
                    {
                        set_prop(prop_sck_armed_ptr, false); //disarm sync_check if it is armed
                    }

                    cgu_ctrl((double)dt / (double)DT_SECOND);
                }
            }
            else
            {
                //In Mode B
                if (sct_metrics_check_mode_B())
                {
                    if (~sck_armed_flag)
                    {
                        set_prop(prop_sck_armed_ptr, true); // arm sync_check
                    }

                    //-- tick the timer
                    double dt_dm_sec = (double)dt / (double)DT_SECOND;
                    timer_mode_B_sec += dt_dm_sec;

                    //-- check the timer
                    if (timer_mode_B_sec >= pp_t_mon_sec)
                    {
                        mode_transition(SCT_MODE_ENUM::MODE_A, false);
                    }
                }
                else
                {
                    mode_transition(SCT_MODE_ENUM::MODE_A, false);
                }
            }
        }
    }
    return SM_EVENT;
}

void sync_ctrl::dm_update_measurements()
{
    swt_status = static_cast<SWT_STATUS_ENUM>(get_prop_value<enumeration>(prop_swt_status_ptr, &gld_property::get_enumeration, false));
    sck_armed_flag = get_prop_value<bool>(prop_sck_armed_ptr, &gld_property::get_bool, false);

    if (swt_status == SWT_STATUS_ENUM::OPEN) // If the switch is closed, there is no need to update other measured/calculated properties
    {
        sck_freq_diff_hz = get_prop_value<double>(prop_sck_freq_diff_hz_ptr, &gld_property::get_double, false);

        sck_volt_A_mag_diff_pu = get_prop_value<double>(prop_sck_volt_A_mag_diff_pu_ptr, &gld_property::get_double, false);
        sck_volt_B_mag_diff_pu = get_prop_value<double>(prop_sck_volt_B_mag_diff_pu_ptr, &gld_property::get_double, false);
        sck_volt_C_mag_diff_pu = get_prop_value<double>(prop_sck_volt_C_mag_diff_pu_ptr, &gld_property::get_double, false);
    }
}

bool sync_ctrl::sct_metrics_check_mode_A(unsigned long dt)
{
    if ((sck_freq_diff_hz >= sct_freq_tol_lb_hz) && (sck_freq_diff_hz <= sct_freq_tol_ub_hz) &&
        (sck_volt_A_mag_diff_pu <= sct_volt_mag_tol_pu) &&
        (sck_volt_B_mag_diff_pu <= sct_volt_mag_tol_pu) &&
        (sck_volt_C_mag_diff_pu <= sct_volt_mag_tol_pu))
    {
        double dt_dm_sec = (double)dt / (double)DT_SECOND;
        timer_mode_A_sec += dt_dm_sec;
    }

    else
    {
        timer_mode_A_sec = 0;
    }

    if (timer_mode_A_sec >= pp_t_ctrl_sec)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool sync_ctrl::sct_metrics_check_mode_B()
{
    if ((sck_freq_diff_hz >= sct_freq_tol_lb_hz) && (sck_freq_diff_hz <= sct_freq_tol_ub_hz) &&
        (sck_volt_A_mag_diff_pu <= sct_volt_mag_tol_pu) &&
        (sck_volt_B_mag_diff_pu <= sct_volt_mag_tol_pu) &&
        (sck_volt_C_mag_diff_pu <= sct_volt_mag_tol_pu))
        return true;
    else
        return false;
}

void sync_ctrl::mode_transition(SCT_MODE_ENUM sct_mode, bool sck_armed_flag)
{
    reset_timer();                                // Reset timers of both modes
    mode_status = sct_mode;                       // Mode A or B
    set_prop(prop_sck_armed_ptr, sck_armed_flag); // Arm or disarm sync_check
}

void sync_ctrl::cgu_ctrl(double dt)
{
    switch (cgu_type)
    {
    case CGU_TYPE::DG:
    {
        // PI controller for freq_diff_hz
        dg_freq_set_mpv = sck_freq_diff_hz / sys_nom_freq_hz;
        dg_freq_set_cv = pi_ctrl_dg_freq_set->step_update(
            (sct_freq_tol_ub_hz - sct_freq_tol_lb_hz) / 2 / sys_nom_freq_hz,
            dg_freq_set_mpv, dt); //@TODO: the setpoint may be defined by the user via a published property
        if (cgu_P_f_droop_setting_mode == PF_DROOP_MODE::FSET_MODE)
        {
            dg_freq_set_cv *= sys_nom_freq_hz;
        }
        if (sct_cv_arm_flag)
            set_prop(prop_cgu_freq_set_ptr, dg_freq_set_cv);

        // PI controller for avg(volt_mag_diff_ph_a_pu, volt_mag_diff_ph_b_pu, volt_mag_diff_ph_c_pu) //@TODO: may change to max()
        dg_volt_set_mpv = (sck_volt_A_mag_diff_pu + sck_volt_B_mag_diff_pu + sck_volt_C_mag_diff_pu) / 3;
        dg_volt_set_cv = pi_ctrl_dg_volt_set->step_update(0, dg_volt_set_mpv, dt); //@TODO: the setpoint may be defined by the user via a published property
        if (sct_cv_arm_flag)
            set_prop(prop_cgu_volt_set_ptr, dg_volt_set_cv);
        break;
    }
    case CGU_TYPE::INV:
    {
        break;
    }
    case CGU_TYPE::UNKNOWN_CGU_TYPE:
    {
        GL_THROW("The type of this controlled generation unit is unknown!");
        break;
    }
    }
}

/* For reset */
void sync_ctrl::reset_timer()
{
    timer_mode_A_sec = 0;
    timer_mode_B_sec = 0;
}

/* parameter/data sanity check */
void sync_ctrl::dm_data_sanity_check()
{
    OBJECT *obj = OBJECTHDR(this);

    double sck_metrics_period_sec = get_prop_value<double, OBJECT>(sck_obj_ptr, "metrics_period",
                                                                   &gld_property::is_valid,
                                                                   &gld_property::is_double,
                                                                   &gld_property::get_double);
    if (sck_metrics_period_sec >= pp_t_mon_sec)
    {
        gl_warning("%s:%d %s - The 'monitoring_period' is smaller or equal to the 'metrics_period' of %s.",
                   STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                   sck_obj_ptr->name);
        /*  TROUBLESHOOT
		The sck_metrics_period_sec should be smaller than the pp_t_mon_sec!
		If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
		*/
    }
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: sync_ctrl
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_sync_ctrl(OBJECT **obj, OBJECT *parent)
{
    try
    {
        *obj = gl_create_object(sync_ctrl::oclass);
        if (*obj != nullptr)
        {
            sync_ctrl *my = OBJECTDATA(*obj, sync_ctrl);
            gl_set_parent(*obj, parent);
            return my->create();
        }
        else
            return 0;
    }
    CREATE_CATCHALL(sync_ctrl);
}

EXPORT int init_sync_ctrl(OBJECT *obj)
{
    try
    {
        sync_ctrl *my = OBJECTDATA(obj, sync_ctrl);
        return my->init(obj->parent);
    }
    INIT_CATCHALL(sync_ctrl);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_sync_ctrl(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
    sync_ctrl *pObj = OBJECTDATA(obj, sync_ctrl);
    TIMESTAMP t1 = TS_INVALID;

    try
    {
        switch (pass)
        {
        case PC_PRETOPDOWN:
            t1 = pObj->presync(obj->clock, t0);
            break;
        case PC_BOTTOMUP:
            t1 = pObj->sync(obj->clock, t0);
            break;
        case PC_POSTTOPDOWN:
            t1 = pObj->postsync(obj->clock, t0);
            break;
        default:
            GL_THROW("invalid pass request (%d)", pass);
            break;
        }
        if (pass == clockpass)
            obj->clock = t0;
    }
    SYNC_CATCHALL(sync_ctrl);
    return t1;
}

// Deltamode export
EXPORT SIMULATIONMODE interupdate_sync_ctrl(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
    sync_ctrl *my = OBJECTDATA(obj, sync_ctrl);
    SIMULATIONMODE status = SM_ERROR;
    try
    {
        status = my->inter_deltaupdate_sync_ctrl(delta_time, dt, iteration_count_val);
        return status;
    }
    catch (char *msg)
    {
        gl_error("interupdate_sync_ctrl(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
        return status;
    }
}

//==Init & Check Member Funcs
void sync_ctrl::init_vars() // Init local variables with default settings
{
    //==Flag & Status
    mode_status = SCT_MODE_ENUM::MODE_A;
    swt_status = SWT_STATUS_ENUM::OPEN;
    sck_armed_status = false;

    //==Time
    timer_mode_A_sec = timer_mode_B_sec = dt_dm_sec = 0;

    //==System Info
    //--get the nominal frequency of the power system
    sys_nom_freq_hz = get_prop_value<double>("powerflow::nominal_frequency",
                                             &gld_property::is_valid,
                                             &gld_property::is_double,
                                             &gld_property::get_double);
    // std::cout << "Nominal Frequency = " << sys_nom_freq_hz << " (Hz)" << std::endl; // For verifying

    //==Controller
    pi_ctrl_dg_volt_set = nullptr;
    pi_ctrl_dg_freq_set = nullptr;

    //==Obj & Prop
    /* switch */
    obj_swt_ptr = nullptr;
    prop_swt_status_ptr = nullptr;

    /* sync_check */
    prop_sck_armed_ptr = nullptr;
    sck_armed_flag = false;

    prop_sck_freq_diff_hz_ptr = nullptr;
    prop_sck_volt_A_mag_diff_pu_ptr = nullptr;
    prop_sck_volt_B_mag_diff_pu_ptr = nullptr;
    prop_sck_volt_C_mag_diff_pu_ptr = nullptr;

    swt_nom_volt_v = sck_freq_diff_hz = 0;
    sck_volt_A_mag_diff_pu = sck_volt_B_mag_diff_pu = sck_volt_C_mag_diff_pu = 0;
}

//==Utility Member Funcs
/* Set */
template <class T>
void sync_ctrl::set_prop(gld_property *prop_ptr, T prop_value)
{
    gld_wlock *rlock;
    prop_ptr->setp<T>(prop_value, *rlock);
}
/* Get */
template <class T>
void sync_ctrl::get_prop(gld_property *prop_ptr, T prop_value)
{
    gld_wlock *rlock;
    prop_ptr->getp<T>(prop_value, *rlock);
}

/* Get Prop Value*/
template <class T, class T1>
T sync_ctrl::get_prop_value(T1 *obj_ptr, char *prop_name_char_ptr, bool (gld_property::*fp_is_valid)(), bool (gld_property::*fp_is_type)(), T (gld_property::*fp_get_type)())
{
    // Get the property pointer
    gld_property *prop_ptr = get_prop_ptr<T1>(obj_ptr, prop_name_char_ptr, fp_is_valid, fp_is_type);

    return get_prop_value(prop_ptr, fp_get_type);
}

template <class T, class T1>
T *sync_ctrl::get_prop_value(T1 *obj_ptr, char *prop_name_char_ptr, bool (gld_property::*fp_is_valid)(), bool (gld_property::*fp_is_type)(), T *(gld_property::*fp_get_type)())
{
    // Get the property pointer
    gld_property *prop_ptr = get_prop_ptr<T1>(obj_ptr, prop_name_char_ptr, fp_is_valid, fp_is_type);

    return get_prop_value(prop_ptr, fp_get_type);
}

template <class T>
T sync_ctrl::get_prop_value(char *prop_name_char_ptr, bool (gld_property::*fp_is_valid)(), bool (gld_property::*fp_is_type)(), T (gld_property::*fp_get_type)())
{
    // Get the property pointer
    gld_property *prop_ptr = get_prop_ptr<char>(nullptr, prop_name_char_ptr, fp_is_valid, fp_is_type);

    return get_prop_value(prop_ptr, fp_get_type);
}

template <class T>
T sync_ctrl::get_prop_value(gld_property *prop_ptr, T (gld_property::*fp_get_type)(), bool del_prop_ptr_flag /*= true*/)
{
    // Get the property value
    T prop_val = (prop_ptr->*fp_get_type)();

    // Clean & return
    if (del_prop_ptr_flag)
    {
        delete prop_ptr;
    }
    return prop_val;
}

template <class T>
T *sync_ctrl::get_prop_value(gld_property *prop_ptr, T *(gld_property::*fp_get_type)(), bool del_prop_ptr_flag /*= true*/)
{
    // Get the property value
    T *prop_val = (prop_ptr->*fp_get_type)();

    // Clean & return
    if (del_prop_ptr_flag)
    {
        delete prop_ptr;
    }
    return prop_val;
}

template <class T>
gld_property *sync_ctrl::get_prop_ptr(T *obj_ptr, char *prop_name_char_ptr, bool (gld_property::*fp_is_valid)(), bool (gld_property::*fp_is_type)())
{
    OBJECT *obj = OBJECTHDR(this);

    // Get property pointer
    gld_property *temp_prop_ptr;

    if (obj_ptr == nullptr)
        temp_prop_ptr = new gld_property(prop_name_char_ptr);
    else
        temp_prop_ptr = new gld_property(obj_ptr, prop_name_char_ptr);

    // Check the validity and type of the property pointer
    if (((temp_prop_ptr->*fp_is_valid)() != true) || ((temp_prop_ptr->*fp_is_type)() != true))
    {
        GL_THROW("%s:%d %s failed to map the property: '%s'",
                 STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"), prop_name_char_ptr);
        /*  TROUBLESHOOT
		While attempting to map the property named via variable 'prop_name_char_ptr', an error occurred. Please try again.
		If the error persists, please submit your GLM and a bug report to the ticketing system.
		*/
    }

    return temp_prop_ptr;
}

//==Init & Check Member Funcs
void sync_ctrl::init_pub_prop() // Init published properties with default settings
{
    //==Flag
    arm_flag = false; //Start as disabled

    //==Object
    sck_obj_ptr = nullptr;
    cgu_obj_ptr = nullptr;

    //==Tolerance
    sct_freq_tol_ub_hz = 1.1e-2 * sys_nom_freq_hz;
    sct_freq_tol_lb_hz = 0.2e-2 * sys_nom_freq_hz;
    sct_volt_mag_tol_pu = 0.01;

    //==Time
    pp_t_ctrl_sec = 1;
    pp_t_mon_sec = 10;

    //==Controller (@TODO: default values to be updated.)
    //--frequency
    pi_freq_kp = 1;
    pi_freq_ki = 1;

    pi_freq_ub_pu = 1;
    pi_freq_lb_pu = 0;

    //--voltage magnitude
    pi_volt_mag_kp = 1;
    pi_volt_mag_ki = 1;

    pi_volt_mag_ub_pu = 1;
    pi_volt_mag_lb_pu = 0;
}

void sync_ctrl::init_hidden_prop(double flag_val)
{
    sct_cv_arm_flag = true;
    dg_volt_set_mpv = flag_val;
    dg_volt_set_cv = flag_val;
}

void sync_ctrl::init_data_sanity_check()
{
    OBJECT *obj = OBJECTHDR(this);

    //==Flag
    // arm_flag does not need a sanity check at this stage

    //==Object
    //--sync_check object
    if (sck_obj_ptr == nullptr)
    {
        GL_THROW("%s:%d %s the %s property must be specified!",
                 STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                 STR(sck_obj_ptr));
        /*  TROUBLESHOOT
		The sck_obj_ptr property is not specified! Please try again.
		If the error persists, please submit your GLM and a bug report to the ticketing system.
		*/
    }
    else if (!gl_object_isa(sck_obj_ptr, "sync_check", "powerflow")) //Check if the sck_obj_ptr is pointing to a sync_check object
    {
        GL_THROW("%s:%d %s the %s property must be set as the name of a sync_check object!",
                 STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                 STR(sck_obj_ptr));
        /*  TROUBLESHOOT
		The sck_obj_ptr property must be set as the name of a sync_check object. Please try again.
		If the error persists, please submit your GLM and a bug report to the ticketing system.
		*/
    }

    //--controlled generation unit
    if (cgu_obj_ptr == nullptr)
    {
        GL_THROW("%s:%d %s the %s property must be specified!",
                 STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                 STR(cgu_obj_ptr));
        /*  TROUBLESHOOT
		The cgu_obj_ptr property is not specified! Please try again.
		If the error persists, please submit your GLM and a bug report to the ticketing system.
		*/
    }
    // @NOTE: the gl_object_isa is implemented in an "interesting" way that leads the isa() to return int (implicit bool to int conversion) instead of bool
    else if (!gl_object_isa(cgu_obj_ptr, "inverter_dyn", "generators") &&
             (!gl_object_isa(cgu_obj_ptr, "diesel_dg", "generators"))) //Check if the sck_obj_ptr is pointing to a sync_check object
    {
        GL_THROW("%s:%d %s the %s property must be set as the name of a DG or Inverter object!",
                 STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                 STR(cgu_obj_ptr));
        /*  TROUBLESHOOT
		The cgu_obj_ptr property must be set as the name of a DG or Inverter object. Please try again.
		If the error persists, please submit your GLM and a bug report to the ticketing system.
		*/
    }

    //==Tolerance
    if (sct_freq_tol_ub_hz <= 0)
    {
        sct_freq_tol_ub_hz = 1.1e-2 * sys_nom_freq_hz; //Default it to 1.1% of the nominal frequency

        gl_warning("%s:%d %s - %s was not set as a positive value, it is reset to %f [Hz].",
                   STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                   STR(sct_freq_tol_ub_hz), sct_freq_tol_ub_hz);
        /*  TROUBLESHOOT
		The sct_freq_tol_ub_hz was not set as a positive value!
		If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
		*/
    }

    if (sct_freq_tol_lb_hz <= 0)
    {
        sct_freq_tol_lb_hz = 0.2e-2 * sys_nom_freq_hz; //Default it to 0.2% of the nominal frequency

        //@TODO: There is an extra '.' at the end of the warning message. Without it, this message
        // is considered as a repeat if the previous sanity check fails. This should be a "bug" of the 'gl_warning',
        // which is not to be fixed here at this stage.
        gl_warning("%s:%d %s - %s was not set as a positive value, it is reset to %f [Hz]..",
                   STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                   STR(sct_freq_tol_lb_hz), sct_freq_tol_lb_hz);
        /*  TROUBLESHOOT
		The sct_freq_tol_lb_hz was not set as a positive value!
		If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
		*/
    }

    if (sct_freq_tol_ub_hz < sct_freq_tol_lb_hz)
    {
        swap(sct_freq_tol_lb_hz, sct_freq_tol_ub_hz);

        gl_warning("%s:%d %s - %s was set larger than the %s, their values are swapped. Now %s is %f [Hz], %s is %f [Hz].",
                   STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                   STR(sct_freq_tol_lb_hz), STR(sct_freq_tol_ub_hz),
                   STR(sct_freq_tol_lb_hz), sct_freq_tol_lb_hz,
                   STR(sct_freq_tol_ub_hz), sct_freq_tol_ub_hz);
        /*  TROUBLESHOOT
		The sct_freq_tol_lb_hz was set larger than the sct_freq_tol_ub_hz! Their values are swapped.
		If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
		*/
    }

    if (sct_freq_tol_lb_hz == sct_freq_tol_ub_hz)
    {
        sct_freq_tol_lb_hz /= 2;

        gl_warning("%s:%d %s - %s was set the same to the %s. The %s is halved as %f [Hz].",
                   STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                   STR(sct_freq_tol_lb_hz), STR(sct_freq_tol_ub_hz),
                   STR(sct_freq_tol_lb_hz), sct_freq_tol_lb_hz);
        /*  TROUBLESHOOT
		The sct_freq_tol_lb_hz was set was set the same to the sct_freq_tol_ub_hz!
        The %sct_freq_tol_lb_hz is halved.
		If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
		*/
    }

    if (sct_volt_mag_tol_pu <= 0)
    {
        sct_volt_mag_tol_pu = 1e-2; //Default it to 0.01 pu

        gl_warning("%s:%d %s - %s was not set as a positive value, it is reset to %f [pu].",
                   STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                   STR(sct_volt_mag_tol_pu), sct_volt_mag_tol_pu);
        /*  TROUBLESHOOT
		The sct_volt_mag_tol_pu was not set as a positive value!
		If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
		*/
    }

    //==Time
    if (pp_t_ctrl_sec <= 0)
    {
        pp_t_ctrl_sec = 1;

        gl_warning("%s:%d %s - %s was not set as a positive value, it is reset to %f [sec].",
                   STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                   STR(pp_t_ctrl_sec), pp_t_ctrl_sec);
        /*  TROUBLESHOOT
		The pp_t_ctrl_sec was not set as a positive value!
		If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
		*/
    }

    if (pp_t_mon_sec <= 0)
    {
        pp_t_mon_sec = 10;
        // That word 'now' is used to avoid being considerd as a repeat.
        gl_warning("%s:%d %s - %s was not set as a positive value, now it is reset to %f [sec].",
                   STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
                   STR(pp_t_mon_sec), pp_t_mon_sec);
        /*  TROUBLESHOOT
		The pp_t_mon_sec was not set as a positive value!
		If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
		*/
    }

    //==Controller @TODO
    // if (pi_freq_kp < 0)
    // {
    //     pi_freq_kp = 1; //@TODO

    //     gl_warning("%s:%d %s - %s was set as a negative value, it is reset to %f.",
    //                STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
    //                STR(pi_freq_kp), pi_freq_kp);
    //     /*  TROUBLESHOOT
    // 	The pi_freq_kp was set as a negative value!
    // 	If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
    // 	*/
    // }

    // if (pi_freq_ki < 0)
    // {
    //     pi_freq_ki = 1; //@TODO

    //     gl_warning("%s:%d %s - %s was set as a negative value, it is reset to %f.",
    //                STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
    //                STR(pi_freq_ki), pi_freq_ki);
    //     /*  TROUBLESHOOT
    // 	The pi_freq_ki was set as a negative value!
    // 	If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
    // 	*/
    // }

    // if (pi_volt_mag_kp < 0)
    // {
    //     pi_volt_mag_kp = 1; //@TODO

    //     gl_warning("%s:%d %s - %s was set as a negative value, it is reset to %f.",
    //                STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
    //                STR(pi_volt_mag_kp), pi_volt_mag_kp);
    //     /*  TROUBLESHOOT
    // 	The pi_volt_mag_kp was set as a negative value!
    // 	If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
    // 	*/
    // }

    // if (pi_volt_mag_ki < 0)
    // {
    //     pi_volt_mag_ki = 1; //@TODO

    //     gl_warning("%s:%d %s - %s was set as a negative value, it is reset to %f.",
    //                STR(sync_ctrl), obj->id, (obj->name ? obj->name : "Unnamed"),
    //                STR(pi_volt_mag_ki), pi_volt_mag_ki);
    //     /*  TROUBLESHOOT
    // 	The pi_volt_mag_ki was set as a negative value!
    // 	If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
    // 	*/
    // }
}

void sync_ctrl::init_deltamode_check()
{
    OBJECT *obj = OBJECTHDR(this);

    //==Set the deltamode flag, if desired
    if (obj->flags & OF_DELTAMODE)
    {
        deltamode_inclusive = true;
    }

    //==Check consistency of the module deltamode flag & object deltamode flag
    if (enable_subsecond_models != deltamode_inclusive)
    {
        if (deltamode_inclusive)
        {
            gl_warning("%s:%d %s - Deltamode is enabled for this '%s' object, but not the '%s' module!",
                       obj->oclass->name, obj->id, (obj->name ? obj->name : "Unnamed"),
                       obj->oclass->name, obj->oclass->module->name);
            /*  TROUBLESHOOT
			Deltamode is enabled for this 'sync_ctrl' object, but not the 'generators' module!
			If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
			*/
        }
        else
        {
            gl_warning("%s:%d %s - Deltamode is enabled for the '%s' module, but not this '%s' object!",
                       obj->oclass->name, obj->id, (obj->name ? obj->name : "Unnamed"),
                       obj->oclass->module->name, obj->oclass->name);
            /*  TROUBLESHOOT
			Deltamode is enabled for the 'generators' module, but not this 'sync_ctrl' object!
			If the warning persists and the object does, please submit your code and a bug report via the issue tracker.
			*/
        }
    }
    else if (deltamode_inclusive) // Both the 'generators' module and 'sync_ctrl' object are enabled for the deltamode
    {
        gen_object_count++;
        reg_dm_flag = true;
    }
}

void sync_ctrl::init_nom_values()
{
    swt_nom_volt_v = get_prop_value<double, OBJECT>(sck_obj_ptr, "nominal_volt_v",
                                                    &gld_property::is_valid,
                                                    &gld_property::is_double,
                                                    &gld_property::get_double);
}

void sync_ctrl::init_sensors()
{
    //==Switch (i.e., the parent of the sync_check object)
    obj_swt_ptr = sck_obj_ptr->parent; //@TODO: here may need to do a sanity check again, as this can be executed before the init of sync_check, where there is a sanity check
    prop_swt_status_ptr = get_prop_ptr(obj_swt_ptr, "status",
                                       &gld_property::is_valid,
                                       &gld_property::is_enumeration);
    swt_status = static_cast<SWT_STATUS_ENUM>(get_prop_value<enumeration>(prop_swt_status_ptr, &gld_property::get_enumeration, false));

    //==Sync_check
    prop_sck_armed_ptr = get_prop_ptr(sck_obj_ptr, "armed",
                                      &gld_property::is_valid,
                                      &gld_property::is_bool);
    sck_armed_flag = get_prop_value<bool>(prop_sck_armed_ptr, &gld_property::get_bool, false);    //i.e., get_prop(prop_sck_armed_ptr, sck_armed_flag);

    prop_sck_freq_diff_hz_ptr = get_prop_ptr(sck_obj_ptr, "freq_diff_noabs_hz",
                                             &gld_property::is_valid,
                                             &gld_property::is_double);
    sck_freq_diff_hz = get_prop_value<double>(prop_sck_freq_diff_hz_ptr, &gld_property::get_double, false);

    prop_sck_volt_A_mag_diff_pu_ptr = get_prop_ptr(sck_obj_ptr, "volt_A_mag_diff_noabs_pu",
                                                   &gld_property::is_valid,
                                                   &gld_property::is_double);
    sck_volt_A_mag_diff_pu = get_prop_value<double>(prop_sck_volt_A_mag_diff_pu_ptr, &gld_property::get_double, false);

    prop_sck_volt_B_mag_diff_pu_ptr = get_prop_ptr(sck_obj_ptr, "volt_B_mag_diff_noabs_pu",
                                                   &gld_property::is_valid,
                                                   &gld_property::is_double);
    sck_volt_B_mag_diff_pu = get_prop_value<double>(prop_sck_volt_B_mag_diff_pu_ptr, &gld_property::get_double, false);

    prop_sck_volt_C_mag_diff_pu_ptr = get_prop_ptr(sck_obj_ptr, "volt_C_mag_diff_noabs_pu",
                                                   &gld_property::is_valid,
                                                   &gld_property::is_double);
    sck_volt_C_mag_diff_pu = get_prop_value<double>(prop_sck_volt_C_mag_diff_pu_ptr, &gld_property::get_double, false);

    //==CGU (controlled generation unit)
    //--p_f_droop
    prop_cgu_P_f_droop_setting_mode_ptr = get_prop_ptr(cgu_obj_ptr, "P_f_droop_setting_mode",
                                                       &gld_property::is_valid,
                                                       &gld_property::is_enumeration);
    //@TODO: This may need to be moved into pre-delta update, or use OF_INIT
    cgu_P_f_droop_setting_mode = static_cast<PF_DROOP_MODE>(
        get_prop_value<enumeration>(prop_cgu_P_f_droop_setting_mode_ptr,
                                    &gld_property::get_enumeration, false));

    //--DG or INV
    if (gl_object_isa(cgu_obj_ptr, "inverter_dyn", "generators"))
    {
        cgu_type = CGU_TYPE::INV;
        prop_cgu_volt_set_name_cc_ptr = "Vset";
        if (cgu_P_f_droop_setting_mode == PF_DROOP_MODE::PSET_MODE)
        {
            prop_cgu_freq_set_name_cc_ptr = "Pset";
        }
        else if (cgu_P_f_droop_setting_mode == PF_DROOP_MODE::FSET_MODE)
        {
            prop_cgu_freq_set_name_cc_ptr = "fset"; //@TODO: double check with Frank
        }
        else
        {
            GL_THROW("Unknown P-F droop setting!");
        }
    }
    else if (gl_object_isa(cgu_obj_ptr, "diesel_dg", "generators"))
    {
        cgu_type = CGU_TYPE::DG;
        prop_cgu_volt_set_name_cc_ptr = "Vset_QV_droop";
        if (cgu_P_f_droop_setting_mode == PF_DROOP_MODE::PSET_MODE)
        {
            prop_cgu_freq_set_name_cc_ptr = "Pset";
        }
        else if (cgu_P_f_droop_setting_mode == PF_DROOP_MODE::FSET_MODE)
        {
            prop_cgu_freq_set_name_cc_ptr = "fset"; //@TODO: double check with Frank
        }
        else
        {
            GL_THROW("Unknown P-F droop setting!");
        }
    }
    else
    {
        cgu_type = CGU_TYPE::UNKNOWN_CGU_TYPE;
        gl_warning("The type of controlled generation unit is unkonwn!");
    }

    //--properties for the controlled variables //@TODO: maybe move to init_controllers()
    prop_cgu_volt_set_ptr = get_prop_ptr(cgu_obj_ptr, (char *)prop_cgu_volt_set_name_cc_ptr,
                                     &gld_property::is_valid,
                                     &gld_property::is_double);

    prop_cgu_freq_set_ptr = get_prop_ptr(cgu_obj_ptr, (char *)prop_cgu_freq_set_name_cc_ptr,
                                         &gld_property::is_valid,
                                         &gld_property::is_double);
}

void sync_ctrl::init_controllers()
{
    pi_ctrl_dg_volt_set = new pid_ctrl(pi_volt_mag_kp, pi_volt_mag_ki, 0,
                                   0, pi_volt_mag_ub_pu, pi_volt_mag_lb_pu);
    pi_ctrl_dg_freq_set = new pid_ctrl(pi_freq_kp, pi_freq_kp, 0,
                                       0, pi_freq_ub_pu, pi_freq_lb_pu);
}

//==QSTS Member Funcs
void sync_ctrl::deltamode_reg()
{
    if (reg_dm_flag) // Check if this object needs to be registered for running in deltamode
    {
        reg_dm_flag = false; // Turn off this one-time flag

        //==Call the allocation routine, if needed
        if ((gen_object_current == -1) || (delta_objects == nullptr))
        {
            allocate_deltamode_arrays();
        }

        //==Check limits
        OBJECT *obj = OBJECTHDR(this);
        if (gen_object_current >= gen_object_count)
        {
            GL_THROW("Too many objects tried to populate deltamode objects array in the '%s' module!",
                     obj->oclass->module->name);
            /*  TROUBLESHOOT
			While attempting to populate a reference array of deltamode-enabled objects for the 'generators'
			module, an attempt was made to write beyond the allocated array space. Please try again.
            If the error persists, please submit a bug report and your code via the issue tracker.
			*/
        }

        //==Map func
        delta_objects[gen_object_current] = obj; // Add this object into the list of deltamode objects
        delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "interupdate_controller_object"));

        //==Dobule check the mapped function
        if (delta_functions[gen_object_current] == nullptr)
        {
            gl_warning("Failure to map deltamode function for this device: %s", obj->name);
            /*  TROUBLESHOOT
			Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
			the object supports deltamode. This warning may simply be an indication that the object of interest
			does not support deltamode. If the warning persists and the object does, please submit your code and
			a bug report via the issue tracker.
			*/
        }

        //==Set the post delta function to nullptr, thus it does not need to be checked
        post_delta_functions[gen_object_current] = nullptr;

        //==Increment
        gen_object_current++;
    }
}

/* ================================================
PID Controller (@TODO: move to an independent file)
================================================ */
pid_ctrl::pid_ctrl(double kp, double ki, double kd,
                   double dt, double cv_max, double cv_min)
    : kp(kp),
      ki(ki),
      kd(kd),
      dt(dt),
      cv_max(cv_max),
      cv_min(cv_min),
      pre_ev(0),
      integral(0)
{
}

pid_ctrl::~pid_ctrl()
{
}

double pid_ctrl::step_update(double setpoint, double mpv, double cur_dt)
{
    //== Step time interval selection
    double step_dt;
    if (cur_dt > 0)
        step_dt = cur_dt;
    else if (dt > 0)
        step_dt = dt;
    else
    {
        // Post an error message & terminate (@TODO: use cerr & terminate())
        GL_THROW("The time step is not specified as positive in both the init and step update processes.");
    }

    //== Calculation
    double ev = setpoint - mpv; // Error value

    double p_term = kp * ev; // Proportional term

    integral += ev * step_dt;      // Integral accumulator
    double i_term = ki * integral; // Integral term

    double derivative = (ev - pre_ev) / step_dt; // Current derivative
    double d_term = kd * derivative;             // Derivative term

    double pid_ctrl_cv = p_term + i_term + d_term; // Control variable, i.e., the output

    //== Bounds
    if (pid_ctrl_cv > cv_max)
        pid_ctrl_cv = cv_max;
    else if (pid_ctrl_cv < cv_min)
        pid_ctrl_cv = cv_min;

    //== Update for next step
    pre_ev = ev;

    return pid_ctrl_cv;
}