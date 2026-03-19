## Performance Motor

!!! warning
    This page was automatically generated and requires review.

### Inheritance

| Parent Class |
| --- |
| node |

### Performance Motor Parameters

#### Properties

| Property Name | Type | Unit | Input | Updates | Description | Evidence (delete after review) |
| --- | --- | --- | --- | --- | --- | --- |
| motor_status | enumeration | N/A |  | ✓ | <div style="white-space: normal; overflow-wrap: anywhere;">the current status of the motor Valid values: RUNNING, STALLED, TRIPPED, OFF.</div> | <div style="white-space: normal; overflow-wrap: anywhere;">runtime writes: update_motor_equations</div> |
| motor_status_number | int32 | N/A |  | ✓ | <div style="white-space: normal; overflow-wrap: anywhere;">the current status of the motor as an integer</div> | <div style="white-space: normal; overflow-wrap: anywhere;">runtime writes: update_motor_equations</div> |
| Pbase | double | W | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">motor rated size</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| power_value | complex | VA | ✓ | ✓ | <div style="white-space: normal; overflow-wrap: anywhere;">motor power consumption</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create; runtime writes: update_motor_equations</div> |
| power_factor | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Compressor power factor</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| delta_f_val | double | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Frequency change value for compressor sensitivities</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| Vbrk | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Compressor motor breakdown voltage</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| Vstallbrk | double | N/A |  |  | <div style="white-space: normal; overflow-wrap: anywhere;">Intersection point for power curve and power stall curve - calculated</div> | <div style="white-space: normal; overflow-wrap: anywhere;">access=PA_HIDDEN; init writes: create, init</div> |
| Vstall | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Compressor stall threshold voltage</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| Vrst | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Voltage at which motor can restart</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| P_val | double | pu |  | ✓ | <div style="white-space: normal; overflow-wrap: anywhere;">real power portion of motor power consumption calculation</div> | <div style="white-space: normal; overflow-wrap: anywhere;">access=PA_HIDDEN; init writes: create; runtime writes: update_motor_equations</div> |
| Q_val | double | pu |  | ✓ | <div style="white-space: normal; overflow-wrap: anywhere;">reactive power portion of motor power consumption calculation</div> | <div style="white-space: normal; overflow-wrap: anywhere;">access=PA_HIDDEN; init writes: create; runtime writes: update_motor_equations</div> |
| P_0 | double | pu |  |  | <div style="white-space: normal; overflow-wrap: anywhere;">Computed power curve initialization point - real power</div> | <div style="white-space: normal; overflow-wrap: anywhere;">access=PA_HIDDEN; init writes: create, init</div> |
| Q_0 | double | pu |  |  | <div style="white-space: normal; overflow-wrap: anywhere;">Computed power curve initialization point - reactive power</div> | <div style="white-space: normal; overflow-wrap: anywhere;">access=PA_HIDDEN; init writes: create, init</div> |
| K_p1 | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Real power coefficient for running state 1, puP/puV</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| K_q1 | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Reactive power coefficient for running state 1, puQ/puV</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| K_p2 | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Real power coefficient for running state 2, puP/puV</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| K_q2 | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Reactive power coefficient for running state 2, puQ/puV</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| N_p1 | double | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Real power exponent for running state 1</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| N_q1 | double | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Reactive power exponent for running state 1</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| N_p2 | double | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Real power exponent for running state 2</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| N_q2 | double | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Reactive power exponent for running state 2</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| CmpKpf | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Real power frequency sensitivity in puP/puf</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| CmpKqf | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Reactive power frequency sensitivity in puQ/puf</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| stall_impedance | complex | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">compressor stall imepdance</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| stall_resistance | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">compressor stall resistance</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| stall_reactance | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">compressor stall reactance</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| Tstall | double | s | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">stall time</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| reconnect_time | double | s | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">reconnect time after a trip</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| Pinit | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Initial assumed real power loading of connected terminal</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| Vinit | double | pu | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Initial assumed voltage value of connected terminal</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| stall_time_tracker | double | s |  |  | <div style="white-space: normal; overflow-wrap: anywhere;">internal stall time tracker variable</div> | <div style="white-space: normal; overflow-wrap: anywhere;">access=PA_HIDDEN; init writes: create</div> |
| reconnect_time_tracker | double | s |  |  | <div style="white-space: normal; overflow-wrap: anywhere;">internal reconnect time tracker variable</div> | <div style="white-space: normal; overflow-wrap: anywhere;">access=PA_HIDDEN; init writes: create</div> |
