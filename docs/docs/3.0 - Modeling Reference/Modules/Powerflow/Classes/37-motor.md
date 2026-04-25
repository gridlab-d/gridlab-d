## Motor

!!! warning
    This page was automatically generated and requires review.

In Development. 

### Motor Parameters

#### Properties

**motor** objects are derived from **[node](03-node.md)** objects, so any parameters of the **[node](03-node.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| base_power | double | W | ✓ |  | ⚠️ base power |
| n | double | N/A | ✓ |  | ⚠️ ratio of stator auxiliary windings to stator main windings |
| Rds | double | ohm | ✓ |  | ⚠️ d-axis resistance - single-phase model |
| Rqs | double | ohm | ✓ |  | ⚠️ q-asis resistance - single-phase model |
| Rs | double | ohm | ✓ |  | ⚠️ stator resistance - three-phase model |
| Rr | double | ohm | ✓ |  | ⚠️ rotor resistance |
| Xm | double | ohm | ✓ |  | ⚠️ magnetizing reactance |
| Xr | double | ohm | ✓ |  | ⚠️ rotor reactance |
| Xs | double | ohm | ✓ |  | ⚠️ stator leakage reactance - three-phase model |
| Xc_run | double | ohm | ✓ |  | ⚠️ running capacitor reactance - single-phase model |
| Xc_start | double | ohm | ✓ |  | ⚠️ starting capacitor reactance - single-phase model |
| Xd_prime | double | ohm | ✓ |  | ⚠️ d-axis reactance - single-phase model |
| Xq_prime | double | ohm | ✓ |  | ⚠️ q-axis reactance - single-phase model |
| A_sat | double | N/A | ✓ |  | ⚠️ flux saturation parameter, A - single-phase model |
| b_sat | double | N/A | ✓ |  | ⚠️ flux saturation parameter, b - single-phase model |
| H | double | s | ✓ |  | ⚠️ inertia constant |
| J | double | kg*m^2 | ✓ |  | ⚠️ moment of inertia |
| number_of_poles | double | N/A | ✓ |  | ⚠️ number of poles |
| To_prime | double | s | ✓ |  | ⚠️ rotor time constant |
| capacitor_speed | double | % | ✓ |  | ⚠️ percentage speed of nominal when starting capacitor kicks in |
| trip_time | double | s | ✓ |  | ⚠️ time motor can stay stalled before tripping off |
| uv_relay_install | enumeration | N/A | ✓ | ✓ | ⚠️ is under-voltage relay protection installed on this motor Valid values: `INSTALLED`, `UNINSTALLED`. |
| uv_relay_trip_time | double | s | ✓ |  | ⚠️ time low-voltage condition must exist for under-voltage protection to trip |
| uv_relay_trip_V | double | pu | ✓ |  | ⚠️ pu minimum voltage before under-voltage relay trips |
| contactor_state | enumeration | N/A | ✓ | ✓ | ⚠️ the current status of the motor Valid values: `OPEN`, `CLOSED`. |
| contactor_open_Vmin | double | pu | ✓ |  | ⚠️ pu voltage at which motor contactor opens |
| contactor_close_Vmax | double | pu | ✓ |  | ⚠️ pu voltage at which motor contactor recloses |
| reconnect_time | double | s | ✓ |  | ⚠️ time before tripped motor reconnects |
| time_step_divider | int32 | N/A | ✓ |  | ⚠️ divide time step by n - single- and three- phase models |
| mechanical_torque | double | pu | ✓ | ✓ | ⚠️ mechanical torque applied to the motor |
| torque_usage_method | enumeration | N/A | ✓ | ✓ | ⚠️ Approach for using Tmech on both types Valid values: `DIRECT`, `SPEEDFOUR`. |
| iteration_count | int32 | N/A | ✓ |  | ⚠️ maximum number of iterations for steady state model |
| delta_mode_voltage_trigger | double | % | ✓ |  | ⚠️ percentage voltage of nominal when delta mode is triggered |
| delta_mode_rotor_speed_trigger | double | % | ✓ |  | ⚠️ percentage speed of nominal when delta mode is triggered |
| delta_mode_voltage_exit | double | % | ✓ |  | ⚠️ percentage voltage of nominal to exit delta mode |
| delta_mode_rotor_speed_exit | double | % | ✓ |  | ⚠️ percentage speed of nominal to exit delta mode |
| maximum_speed_error | double | N/A | ✓ |  | ⚠️ maximum speed error for transitioning modes |
| rotor_speed | double | rad/s | ✓ | ✓ | ⚠️ rotor speed |
| motor_status | enumeration | N/A | ✓ | ✓ | ⚠️ the current status of the motor Valid values: `RUNNING`, `STALLED`, `TRIPPED`, `OFF`. |
| motor_status_number | int32 | N/A | ✓ | ✓ | ⚠️ the current status of the motor as an integer |
| desired_motor_state | enumeration | N/A | ✓ | ✓ | ⚠️ Should the motor be on or off Valid values: `ON`, `OFF`. |
| connected_house | object | N/A | ✓ |  | ⚠️ house object to monitor the XXX property to determine if the motor is running |
| connected_house_assumed_mode | enumeration | N/A | ✓ |  | ⚠️ Assumed operation mode of connected_house object Valid values: `NONE`, `COOLING`, `HEATING`. |
| motor_operation_type | enumeration | N/A | ✓ | ✓ | ⚠️ current operation type of the motor - deltamode related Valid values: `SINGLE-PHASE`, `THREE-PHASE`. |
| triplex_connection_type | enumeration | N/A | ✓ | ✓ | ⚠️ Describes how the motor will connect to the triplex devices Valid values: `TRIPLEX_1N`, `TRIPLEX_2N`, `TRIPLEX_12`. |
| motor_trip | bool | N/A | ✓ | ✓ | ⚠️ boolean variable to check if motor is tripped |

#### Developer Properties

These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| mechanical_torque_state_var | double | pu |  | ✓ | ⚠️ Internal state variable torque - three-phase model |
| wb | double | rad/s |  |  | ⚠️ base speed |
| ws | double | rad/s |  | ✓ | ⚠️ system speed |
| psi_b | complex | N/A |  | ✓ | ⚠️ backward rotating flux |
| psi_f | complex | N/A |  | ✓ | ⚠️ forward rotating flux |
| psi_dr | complex | N/A |  | ✓ | ⚠️ Rotor d axis flux |
| psi_qr | complex | N/A |  | ✓ | ⚠️ Rotor q axis flux |
| Ids | complex | N/A |  | ✓ | ⚠️ time before tripped motor reconnects |
| Iqs | complex | N/A |  | ✓ | ⚠️ time before tripped motor reconnects |
| If | complex | N/A |  | ✓ | ⚠️ forward current |
| Ib | complex | N/A |  | ✓ | ⚠️ backward current |
| Is | complex | N/A |  | ✓ | ⚠️ motor current |
| electrical_power | complex | VA |  | ✓ | ⚠️ motor power |
| electrical_torque | double | pu |  | ✓ | ⚠️ electrical torque |
| Vs | complex | N/A |  | ✓ | ⚠️ motor voltage |
| trip | double | N/A |  | ✓ | ⚠️ current time in tripped state |
| reconnect | double | N/A |  | ✓ | ⚠️ current time since motor was tripped |
| phips | complex | N/A |  | ✓ | ⚠️ positive sequence stator flux |
| phins_cj | complex | N/A |  | ✓ | ⚠️ conjugate of negative sequence stator flux |
| phipr | complex | N/A |  | ✓ | ⚠️ positive sequence rotor flux |
| phinr_cj | complex | N/A |  | ✓ | ⚠️ conjugate of negative sequence rotor flux |
| per_unit_rotor_speed | double | pu |  | ✓ | ⚠️ rotor speed in per-unit |
| Ias | complex | pu |  | ✓ | ⚠️ motor phase-a stator current |
| Ibs | complex | pu |  | ✓ | ⚠️ motor phase-b stator current |
| Ics | complex | pu |  | ✓ | ⚠️ motor phase-c stator current |
| Vas | complex | pu |  | ✓ | ⚠️ motor phase-a stator-to-ground voltage |
| Vbs | complex | pu |  | ✓ | ⚠️ motor phase-b stator-to-ground voltage |
| Vcs | complex | pu |  | ✓ | ⚠️ motor phase-c stator-to-ground voltage |
| Ips | complex | N/A |  | ✓ | ⚠️ positive sequence stator current |
| Ipr | complex | N/A |  | ✓ | ⚠️ positive sequence rotor current |
| Ins_cj | complex | N/A |  | ✓ | ⚠️ conjugate of negative sequence stator current |
| Inr_cj | complex | N/A |  | ✓ | ⚠️ conjugate of negative sequence rotor current |
| Ls | double | N/A |  |  | ⚠️ stator synchronous reactance |
| Lr | double | N/A |  |  | ⚠️ rotor synchronous reactance |
| sigma1 | double | N/A |  |  | ⚠️ intermediate variable 1 associated with synch. react. |
| sigma2 | double | N/A |  |  | ⚠️ intermediate variable 2 associated with synch. react. |

### Motor State of Development

In Development. 
