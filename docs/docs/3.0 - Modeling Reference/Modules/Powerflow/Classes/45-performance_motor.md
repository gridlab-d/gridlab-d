# Performance Motor

!!! warning

	This class is considered in development but is not actively maintained and is not validated.

The **performance_motor** class implements the LD1PAC load characteristic model, based on the WECC Air Conditioner Motor Model Test Report and the PowerWorld LD1PAC specification. This model represents compressor and HVAC equipment behavior, including motor states (RUNNING, STALLED, TRIPPED, OFF), voltage-dependent stalling thresholds, reconnection timing, and frequency sensitivity. For detailed technical information on the LD1PAC model, refer to the [WECC Air Conditioner Motor Model Test Report](https://www.wecc.org/Reliability/WECC%20Air%20Conditioner%20Motor%20Model%20Test%20Report--%20Final.pdf) and the [PowerWorld LD1PAC User Manual](https://www.powerworld.com/WebHelp/Content/TransientModels_HTML/Load%20Characteristic%20LD1PAC.htm).

### Performance Motor Parameters

#### Properties

**performance_motor** objects are derived from **[node](03-node.md)** objects, so any parameters of the **[node](03-node.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: performance_motor table 1 { #tbl:45-performance-motor-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **motor_status** | enumeration | N/A | O | the current status of the motor Valid values: `RUNNING`, `STALLED`, `TRIPPED`, `OFF`. |
| **motor_status_number** | int32 | N/A | O | the current status of the motor as an integer |
| **Pbase** | double | W | I | motor rated size |
| **power_value** | complex | VA | IO | motor power consumption |
| **power_factor** | double | pu | I | Compressor power factor |
| **delta_f_val** | double | N/A | I | Frequency change value for compressor sensitivities |
| **Vbrk** | double | pu | I | Compressor motor breakdown voltage |
| **Vstall** | double | pu | I | Compressor stall threshold voltage |
| **Vrst** | double | pu | I | Voltage at which motor can restart |
| **K_p1** | double | pu | I | Real power coefficient for running state 1, puP/puV |
| **K_q1** | double | pu | I | Reactive power coefficient for running state 1, puQ/puV |
| **K_p2** | double | pu | I | Real power coefficient for running state 2, puP/puV |
| **K_q2** | double | pu | I | Reactive power coefficient for running state 2, puQ/puV |
| **N_p1** | double | N/A | I | Real power exponent for running state 1 |
| **N_q1** | double | N/A | I | Reactive power exponent for running state 1 |
| **N_p2** | double | N/A | I | Real power exponent for running state 2 |
| **N_q2** | double | N/A | I | Reactive power exponent for running state 2 |
| **CmpKpf** | double | pu | I | Real power frequency sensitivity in puP/puf |
| **CmpKqf** | double | pu | I | Reactive power frequency sensitivity in puQ/puf |
| **stall_impedance** | complex | pu | I | compressor stall imepdance |
| **stall_resistance** | double | pu | I | compressor stall resistance |
| **stall_reactance** | double | pu | I | compressor stall reactance |
| **Tstall** | double | s | I | stall time |
| **reconnect_time** | double | s | I | reconnect time after a trip |
| **Pinit** | double | pu | I | Initial assumed real power loading of connected terminal |
| **Vinit** | double | pu | I | Initial assumed voltage value of connected terminal |

??? note "Internal Properties"

	#### Internal Properties
	These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

	Table: performance_motor table 2 { #tbl:45-performance-motor-2 }

	| Property Name | Type | Unit | I/O | Description |
	| --- | --- | --- | --- | --- |
	| Vstallbrk | double | N/A | — | Intersection point for power curve and power stall curve - calculated |
	| P_val | double | pu | O | real power portion of motor power consumption calculation |
	| Q_val | double | pu | O | reactive power portion of motor power consumption calculation |
	| P_0 | double | pu | — | Computed power curve initialization point - real power |
	| Q_0 | double | pu | — | Computed power curve initialization point - reactive power |
	| stall_time_tracker | double | s | — | internal stall time tracker variable |
	| reconnect_time_tracker | double | s | — | internal reconnect time tracker variable |


