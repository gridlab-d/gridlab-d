## Motor

!!! warning
    This page was automatically generated and requires review.

In Development. 

### Motor Parameters

#### Properties

**motor** objects are derived from **[node](03-node.md)** objects, so any parameters of the **[node](03-node.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| base_power | double | W | I | ⚠️ base power |
| n | double | N/A | I | ⚠️ ratio of stator auxiliary windings to stator main windings |
| Rds | double | ohm | I | ⚠️ d-axis resistance - single-phase model |
| Rqs | double | ohm | I | ⚠️ q-asis resistance - single-phase model |
| Rs | double | ohm | I | ⚠️ stator resistance - three-phase model |
| Rr | double | ohm | I | ⚠️ rotor resistance |
| Xm | double | ohm | I | ⚠️ magnetizing reactance |
| Xr | double | ohm | I | ⚠️ rotor reactance |
| Xs | double | ohm | I | ⚠️ stator leakage reactance - three-phase model |
| Xc_run | double | ohm | I | ⚠️ running capacitor reactance - single-phase model |
| Xc_start | double | ohm | I | ⚠️ starting capacitor reactance - single-phase model |
| Xd_prime | double | ohm | I | ⚠️ d-axis reactance - single-phase model |
| Xq_prime | double | ohm | I | ⚠️ q-axis reactance - single-phase model |
| A_sat | double | N/A | I | ⚠️ flux saturation parameter, A - single-phase model |
| b_sat | double | N/A | I | ⚠️ flux saturation parameter, b - single-phase model |
| H | double | s | I | ⚠️ inertia constant |
| J | double | kg*m^2 | I | ⚠️ moment of inertia |
| number_of_poles | double | N/A | I | ⚠️ number of poles |
| To_prime | double | s | I | ⚠️ rotor time constant |
| capacitor_speed | double | % | I | ⚠️ percentage speed of nominal when starting capacitor kicks in |
| trip_time | double | s | I | ⚠️ time motor can stay stalled before tripping off |
| uv_relay_install | enumeration | N/A | I | ⚠️ is under-voltage relay protection installed on this motor Valid values: `INSTALLED`, `UNINSTALLED`. |
| uv_relay_trip_time | double | s | I | ⚠️ time low-voltage condition must exist for under-voltage protection to trip |
| uv_relay_trip_V | double | pu | I | ⚠️ pu minimum voltage before under-voltage relay trips |
| contactor_state | enumeration | N/A | IO | ⚠️ the current status of the motor Valid values: `OPEN`, `CLOSED`. |
| contactor_open_Vmin | double | pu | I | ⚠️ pu voltage at which motor contactor opens |
| contactor_close_Vmax | double | pu | I | ⚠️ pu voltage at which motor contactor recloses |
| reconnect_time | double | s | I | ⚠️ time before tripped motor reconnects |
| time_step_divider | int32 | N/A | I | ⚠️ divide time step by n - single- and three- phase models |
| mechanical_torque | double | pu | IO | ⚠️ mechanical torque applied to the motor |
| torque_usage_method | enumeration | N/A | I | ⚠️ Approach for using Tmech on both types Valid values: `DIRECT`, `SPEEDFOUR`. |
| iteration_count | int32 | N/A | I | ⚠️ maximum number of iterations for steady state model |
| delta_mode_voltage_trigger | double | % | I | ⚠️ percentage voltage of nominal when delta mode is triggered |
| delta_mode_rotor_speed_trigger | double | % | I | ⚠️ percentage speed of nominal when delta mode is triggered |
| delta_mode_voltage_exit | double | % | I | ⚠️ percentage voltage of nominal to exit delta mode |
| delta_mode_rotor_speed_exit | double | % | I | ⚠️ percentage speed of nominal to exit delta mode |
| maximum_speed_error | double | N/A | I | ⚠️ maximum speed error for transitioning modes |
| rotor_speed | double | rad/s | IO | ⚠️ rotor speed |
| motor_status | enumeration | N/A | O | ⚠️ the current status of the motor Valid values: `RUNNING`, `STALLED`, `TRIPPED`, `OFF`. |
| motor_status_number | int32 | N/A | O | ⚠️ the current status of the motor as an integer |
| desired_motor_state | enumeration | N/A | IO | ⚠️ Should the motor be on or off Valid values: `ON`, `OFF`. |
| connected_house | object | N/A | I | ⚠️ house object to monitor the XXX property to determine if the motor is running |
| connected_house_assumed_mode | enumeration | N/A | I | ⚠️ Assumed operation mode of connected_house object Valid values: `NONE`, `COOLING`, `HEATING`. |
| motor_operation_type | enumeration | N/A | I | ⚠️ current operation type of the motor - deltamode related Valid values: `SINGLE-PHASE`, `THREE-PHASE`. |
| triplex_connection_type | enumeration | N/A | I | ⚠️ Describes how the motor will connect to the triplex devices Valid values: `TRIPLEX_1N`, `TRIPLEX_2N`, `TRIPLEX_12`. |
| motor_trip | bool | N/A | IO | ⚠️ boolean variable to check if motor is tripped |

#### Developer Properties

These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| mechanical_torque_state_var | double | pu | O | ⚠️ Internal state variable torque - three-phase model |
| wb | double | rad/s | — | ⚠️ base speed |
| ws | double | rad/s | O | ⚠️ system speed |
| psi_b | complex | N/A | O | ⚠️ backward rotating flux |
| psi_f | complex | N/A | O | ⚠️ forward rotating flux |
| psi_dr | complex | N/A | O | ⚠️ Rotor d axis flux |
| psi_qr | complex | N/A | O | ⚠️ Rotor q axis flux |
| Ids | complex | N/A | O | ⚠️ time before tripped motor reconnects |
| Iqs | complex | N/A | O | ⚠️ time before tripped motor reconnects |
| If | complex | N/A | O | ⚠️ forward current |
| Ib | complex | N/A | O | ⚠️ backward current |
| Is | complex | N/A | O | ⚠️ motor current |
| electrical_power | complex | VA | O | ⚠️ motor power |
| electrical_torque | double | pu | O | ⚠️ electrical torque |
| Vs | complex | N/A | O | ⚠️ motor voltage |
| trip | double | N/A | O | ⚠️ current time in tripped state |
| reconnect | double | N/A | O | ⚠️ current time since motor was tripped |
| phips | complex | N/A | O | ⚠️ positive sequence stator flux |
| phins_cj | complex | N/A | O | ⚠️ conjugate of negative sequence stator flux |
| phipr | complex | N/A | O | ⚠️ positive sequence rotor flux |
| phinr_cj | complex | N/A | O | ⚠️ conjugate of negative sequence rotor flux |
| per_unit_rotor_speed | double | pu | O | ⚠️ rotor speed in per-unit |
| Ias | complex | pu | O | ⚠️ motor phase-a stator current |
| Ibs | complex | pu | O | ⚠️ motor phase-b stator current |
| Ics | complex | pu | O | ⚠️ motor phase-c stator current |
| Vas | complex | pu | O | ⚠️ motor phase-a stator-to-ground voltage |
| Vbs | complex | pu | O | ⚠️ motor phase-b stator-to-ground voltage |
| Vcs | complex | pu | O | ⚠️ motor phase-c stator-to-ground voltage |
| Ips | complex | N/A | O | ⚠️ positive sequence stator current |
| Ipr | complex | N/A | O | ⚠️ positive sequence rotor current |
| Ins_cj | complex | N/A | O | ⚠️ conjugate of negative sequence stator current |
| Inr_cj | complex | N/A | O | ⚠️ conjugate of negative sequence rotor current |
| Ls | double | N/A | — | ⚠️ stator synchronous reactance |
| Lr | double | N/A | — | ⚠️ rotor synchronous reactance |
| sigma1 | double | N/A | — | ⚠️ intermediate variable 1 associated with synch. react. |
| sigma2 | double | N/A | — | ⚠️ intermediate variable 2 associated with synch. react. |

### Motor State of Development

In Development. 
