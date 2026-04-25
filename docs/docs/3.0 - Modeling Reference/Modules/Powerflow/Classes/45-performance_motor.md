## Performance Motor

!!! warning
    This page was automatically generated and requires review.

### Performance Motor Parameters

#### Properties

**performance_motor** objects are derived from **[node](03-node.md)** objects, so any parameters of the **[node](03-node.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| motor_status | enumeration | N/A |  | ✓ | ⚠️ the current status of the motor Valid values: `RUNNING`, `STALLED`, `TRIPPED`, `OFF`. |
| motor_status_number | int32 | N/A |  | ✓ | ⚠️ the current status of the motor as an integer |
| Pbase | double | W | ✓ |  | ⚠️ motor rated size |
| power_value | complex | VA | ✓ | ✓ | ⚠️ motor power consumption |
| power_factor | double | pu | ✓ |  | ⚠️ Compressor power factor |
| delta_f_val | double | N/A | ✓ |  | ⚠️ Frequency change value for compressor sensitivities |
| Vbrk | double | pu | ✓ |  | ⚠️ Compressor motor breakdown voltage |
| Vstall | double | pu | ✓ |  | ⚠️ Compressor stall threshold voltage |
| Vrst | double | pu | ✓ |  | ⚠️ Voltage at which motor can restart |
| K_p1 | double | pu | ✓ |  | ⚠️ Real power coefficient for running state 1, puP/puV |
| K_q1 | double | pu | ✓ |  | ⚠️ Reactive power coefficient for running state 1, puQ/puV |
| K_p2 | double | pu | ✓ |  | ⚠️ Real power coefficient for running state 2, puP/puV |
| K_q2 | double | pu | ✓ |  | ⚠️ Reactive power coefficient for running state 2, puQ/puV |
| N_p1 | double | N/A | ✓ |  | ⚠️ Real power exponent for running state 1 |
| N_q1 | double | N/A | ✓ |  | ⚠️ Reactive power exponent for running state 1 |
| N_p2 | double | N/A | ✓ |  | ⚠️ Real power exponent for running state 2 |
| N_q2 | double | N/A | ✓ |  | ⚠️ Reactive power exponent for running state 2 |
| CmpKpf | double | pu | ✓ |  | ⚠️ Real power frequency sensitivity in puP/puf |
| CmpKqf | double | pu | ✓ |  | ⚠️ Reactive power frequency sensitivity in puQ/puf |
| stall_impedance | complex | pu | ✓ |  | ⚠️ compressor stall imepdance |
| stall_resistance | double | pu | ✓ |  | ⚠️ compressor stall resistance |
| stall_reactance | double | pu | ✓ |  | ⚠️ compressor stall reactance |
| Tstall | double | s | ✓ |  | ⚠️ stall time |
| reconnect_time | double | s | ✓ |  | ⚠️ reconnect time after a trip |
| Pinit | double | pu | ✓ |  | ⚠️ Initial assumed real power loading of connected terminal |
| Vinit | double | pu | ✓ |  | ⚠️ Initial assumed voltage value of connected terminal |

#### Developer Properties

These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| Vstallbrk | double | N/A |  |  | ⚠️ Intersection point for power curve and power stall curve - calculated |
| P_val | double | pu |  | ✓ | ⚠️ real power portion of motor power consumption calculation |
| Q_val | double | pu |  | ✓ | ⚠️ reactive power portion of motor power consumption calculation |
| P_0 | double | pu |  |  | ⚠️ Computed power curve initialization point - real power |
| Q_0 | double | pu |  |  | ⚠️ Computed power curve initialization point - reactive power |
| stall_time_tracker | double | s |  |  | ⚠️ internal stall time tracker variable |
| reconnect_time_tracker | double | s |  |  | ⚠️ internal reconnect time tracker variable |
