## Load

Load objects present a method for taking power out of the system in controlled, known amounts. While implemented as a constant load, **player** objects can be used to vary the load with time. **load** objects provide a means to implement constant current, constant power, and constant impedance losses or generation into the system. The convention is a load is a positive quantity, so generation would need to be represented as a negative number. 

Loads can be a mixture of the constant current, constant impedance, and constant power types. A typical, mixed load would be implemented as 
	
	
	object load {
		phases "ABCD";
		name 841;
		constant_current_C -0.586139+9.765222j;
		constant_impedance_B 221.915014+104.430595j;
		constant_power_A 42000.000000+21000.000000j;
		nominal_voltage 4800;
		}

### Load Parameters

**load** objects are derived from **[node](03-node.md)** objects, so any parameters of the **[node](03-node.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

#### General Properties

Table: 17-load table 1 { #tbl:17-load-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| load_class | enumeration | N/A | I | Describes the type of load the object represents. Used for informational purposes only. Valid types may be referenced by keyword or number <br/> 0 - `U` \- Unknown load type <br/> 1 - `R` \- Residential load <br/> 2 - `C` \- Commercial load <br/> 3 - `I` \- Industrial load <br/> 4 - `A` \- Agricultural load |
| load_priority | enumeration | N/A | I | Load classification based on priority. Valid values: `DISCRETIONARY`, `PRIORITY`, `CRITICAL`. |
| phase_loss_protection | bool | N/A | I | Trip all three phases of the load if a fault occurs. |

#### ZIP Load Properties

These properties define constant power, constant current, and constant impedance loads directly. The `_A`, `_B`, `_C` variants automatically map to wye (A–N, B–N, C–N) or delta (A–B, B–C, C–A) depending on the object's phase configuration. The `_real` and `_reac` suffixed variants provide access to the real and imaginary components individually.

Table: 17-load table 2 { #tbl:17-load-2 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| constant_power_A | complex | VA | IO | Constant power load on phase A (wye A–N, delta A–B). |
| constant_power_B | complex | VA | IO | Constant power load on phase B (wye B–N, delta B–C). |
| constant_power_C | complex | VA | IO | Constant power load on phase C (wye C–N, delta C–A). |
| constant_power_A_real | double | W | IO | Constant power load on phase A, real only. |
| constant_power_B_real | double | W | IO | Constant power load on phase B, real only. |
| constant_power_C_real | double | W | IO | Constant power load on phase C, real only. |
| constant_power_A_reac | double | VAr | IO | Constant power load on phase A, imaginary only. |
| constant_power_B_reac | double | VAr | IO | Constant power load on phase B, imaginary only. |
| constant_power_C_reac | double | VAr | IO | Constant power load on phase C, imaginary only. |
| constant_current_A | complex | A | IO | Constant current load on phase A (wye A–N, delta A–B). |
| constant_current_B | complex | A | IO | Constant current load on phase B (wye B–N, delta B–C). |
| constant_current_C | complex | A | IO | Constant current load on phase C (wye C–N, delta C–A). |
| constant_current_A_real | double | A | IO | Constant current load on phase A, real only. |
| constant_current_B_real | double | A | IO | Constant current load on phase B, real only. |
| constant_current_C_real | double | A | IO | Constant current load on phase C, real only. |
| constant_current_A_reac | double | A | IO | Constant current load on phase A, imaginary only. |
| constant_current_B_reac | double | A | IO | Constant current load on phase B, imaginary only. |
| constant_current_C_reac | double | A | IO | Constant current load on phase C, imaginary only. |
| constant_impedance_A | complex | Ohm | IO | Constant impedance load on phase A (wye A–N, delta A–B). |
| constant_impedance_B | complex | Ohm | IO | Constant impedance load on phase B (wye B–N, delta B–C). |
| constant_impedance_C | complex | Ohm | IO | Constant impedance load on phase C (wye C–N, delta C–A). |
| constant_impedance_A_real | double | Ohm | IO | Constant impedance load on phase A, real only. |
| constant_impedance_B_real | double | Ohm | IO | Constant impedance load on phase B, real only. |
| constant_impedance_C_real | double | Ohm | IO | Constant impedance load on phase C, real only. |
| constant_impedance_A_reac | double | Ohm | IO | Constant impedance load on phase A, imaginary only. |
| constant_impedance_B_reac | double | Ohm | IO | Constant impedance load on phase B, imaginary only. |
| constant_impedance_C_reac | double | Ohm | IO | Constant impedance load on phase C, imaginary only. |

#### Explicit Wye-Connected Load Properties

These properties specify ZIP loads on an explicit wye (phase-to-neutral) connection, regardless of the object's phase configuration. They are input only. The `_real` and `_reac` suffixed variants provide access to the real and imaginary components individually.

Table: 17-load table 3 { #tbl:17-load-3 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| constant_power_AN | complex | VA | I | Constant power load on phase A. |
| constant_power_BN | complex | VA | I | Constant power load on phase B. |
| constant_power_CN | complex | VA | I | Constant power load on phase C. |
| constant_power_AN_real | double | W | I | Constant power load on phase A, real only. |
| constant_power_BN_real | double | W | I | Constant power load on phase B, real only. |
| constant_power_CN_real | double | W | I | Constant power load on phase C, real only. |
| constant_power_AN_reac | double | VAr | I | Constant power load on phase A, imaginary only. |
| constant_power_BN_reac | double | VAr | I | Constant power load on phase B, imaginary only. |
| constant_power_CN_reac | double | VAr | I | Constant power load on phase C, imaginary only. |
| constant_current_AN | complex | A | I | Constant current load on phase A. |
| constant_current_BN | complex | A | I | Constant current load on phase B. |
| constant_current_CN | complex | A | I | Constant current load on phase C. |
| constant_current_AN_real | double | A | I | Constant current load on phase A, real only. |
| constant_current_BN_real | double | A | I | Constant current load on phase B, real only. |
| constant_current_CN_real | double | A | I | Constant current load on phase C, real only. |
| constant_current_AN_reac | double | A | I | Constant current load on phase A, imaginary only. |
| constant_current_BN_reac | double | A | I | Constant current load on phase B, imaginary only. |
| constant_current_CN_reac | double | A | I | Constant current load on phase C, imaginary only. |
| constant_impedance_AN | complex | Ohm | I | Constant impedance load on phase A. |
| constant_impedance_BN | complex | Ohm | I | Constant impedance load on phase B. |
| constant_impedance_CN | complex | Ohm | I | Constant impedance load on phase C. |
| constant_impedance_AN_real | double | Ohm | I | Constant impedance load on phase A, real only. |
| constant_impedance_BN_real | double | Ohm | I | Constant impedance load on phase B, real only. |
| constant_impedance_CN_real | double | Ohm | I | Constant impedance load on phase C, real only. |
| constant_impedance_AN_reac | double | Ohm | I | Constant impedance load on phase A, imaginary only. |
| constant_impedance_BN_reac | double | Ohm | I | Constant impedance load on phase B, imaginary only. |
| constant_impedance_CN_reac | double | Ohm | I | Constant impedance load on phase C, imaginary only. |

#### Explicit Delta-Connected Load Properties

These properties specify ZIP loads on an explicit delta (phase-to-phase) connection, regardless of the object's phase configuration. They are input only. The `_real` and `_reac` suffixed variants provide access to the real and imaginary components individually.

Table: 17-load table 4 { #tbl:17-load-4 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| constant_power_AB | complex | VA | I | Constant power load on phases AB. |
| constant_power_BC | complex | VA | I | Constant power load on phases BC. |
| constant_power_CA | complex | VA | I | Constant power load on phases CA. |
| constant_power_AB_real | double | W | I | Constant power load on phases AB, real only. |
| constant_power_BC_real | double | W | I | Constant power load on phases BC, real only. |
| constant_power_CA_real | double | W | I | Constant power load on phases CA, real only. |
| constant_power_AB_reac | double | VAr | I | Constant power load on phases AB, imaginary only. |
| constant_power_BC_reac | double | VAr | I | Constant power load on phases BC, imaginary only. |
| constant_power_CA_reac | double | VAr | I | Constant power load on phases CA, imaginary only. |
| constant_current_AB | complex | A | I | Constant current load on phases AB. |
| constant_current_BC | complex | A | I | Constant current load on phases BC. |
| constant_current_CA | complex | A | I | Constant current load on phases CA. |
| constant_current_AB_real | double | A | I | Constant current load on phases AB, real only. |
| constant_current_BC_real | double | A | I | Constant current load on phases BC, real only. |
| constant_current_CA_real | double | A | I | Constant current load on phases CA, real only. |
| constant_current_AB_reac | double | A | I | Constant current load on phases AB, imaginary only. |
| constant_current_BC_reac | double | A | I | Constant current load on phases BC, imaginary only. |
| constant_current_CA_reac | double | A | I | Constant current load on phases CA, imaginary only. |
| constant_impedance_AB | complex | Ohm | I | Constant impedance load on phases AB. |
| constant_impedance_BC | complex | Ohm | I | Constant impedance load on phases BC. |
| constant_impedance_CA | complex | Ohm | I | Constant impedance load on phases CA. |
| constant_impedance_AB_real | double | Ohm | I | Constant impedance load on phases AB, real only. |
| constant_impedance_BC_real | double | Ohm | I | Constant impedance load on phases BC, real only. |
| constant_impedance_CA_real | double | Ohm | I | Constant impedance load on phases CA, real only. |
| constant_impedance_AB_reac | double | Ohm | I | Constant impedance load on phases AB, imaginary only. |
| constant_impedance_BC_reac | double | Ohm | I | Constant impedance load on phases BC, imaginary only. |
| constant_impedance_CA_reac | double | Ohm | I | Constant impedance load on phases CA, imaginary only. |

#### Base Power and ZIP Fraction Properties

These properties provide an alternative way to specify ZIP loads using a base power value with fraction and power factor breakdowns per phase, following the same conventions as the ZIPload object. When `base_power` is nonzero for a phase, it overrides the corresponding `constant_power`, `constant_current`, and `constant_impedance` values for that phase.

Table: 17-load table 5 { #tbl:17-load-5 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| base_power_A | double | VA | I | Nominal power on phase A before applying ZIP fractions. |
| base_power_B | double | VA | I | Nominal power on phase B before applying ZIP fractions. |
| base_power_C | double | VA | I | Nominal power on phase C before applying ZIP fractions. |
| power_pf_A | double | pu | I | Power factor of the phase A constant power portion of load. |
| current_pf_A | double | pu | I | Power factor of the phase A constant current portion of load. |
| impedance_pf_A | double | pu | I | Power factor of the phase A constant impedance portion of load. |
| power_pf_B | double | pu | I | Power factor of the phase B constant power portion of load. |
| current_pf_B | double | pu | I | Power factor of the phase B constant current portion of load. |
| impedance_pf_B | double | pu | I | Power factor of the phase B constant impedance portion of load. |
| power_pf_C | double | pu | I | Power factor of the phase C constant power portion of load. |
| current_pf_C | double | pu | I | Power factor of the phase C constant current portion of load. |
| impedance_pf_C | double | pu | I | Power factor of the phase C constant impedance portion of load. |
| power_fraction_A | double | pu | IO | Constant power fraction of base power on phase A. May be overwritten if ZIP fractions do not sum to 1. |
| current_fraction_A | double | pu | I | Constant current fraction of base power on phase A. |
| impedance_fraction_A | double | pu | I | Constant impedance fraction of base power on phase A. |
| power_fraction_B | double | pu | IO | Constant power fraction of base power on phase B. May be overwritten if ZIP fractions do not sum to 1. |
| current_fraction_B | double | pu | I | Constant current fraction of base power on phase B. |
| impedance_fraction_B | double | pu | I | Constant impedance fraction of base power on phase B. |
| power_fraction_C | double | pu | IO | Constant power fraction of base power on phase C. May be overwritten if ZIP fractions do not sum to 1. |
| current_fraction_C | double | pu | I | Constant current fraction of base power on phase C. |
| impedance_fraction_C | double | pu | I | Constant impedance fraction of base power on phase C. |

#### Measurement Properties

These properties are computed by the simulation and are output only. Note that the `measured_voltage` properties lag by one powerflow iteration; for the most current values, read the corresponding `voltage_*` properties from the parent **[node](03-node.md)** object directly.

Table: 17-load table 6 { #tbl:17-load-6 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| measured_voltage_A | complex | V | O | Measured voltage on phase A. |
| measured_voltage_B | complex | V | O | Measured voltage on phase B. |
| measured_voltage_C | complex | V | O | Measured voltage on phase C. |
| measured_voltage_AB | complex | V | O | Measured voltage on delta-phase AB. |
| measured_voltage_BC | complex | V | O | Measured voltage on delta-phase BC. |
| measured_voltage_CA | complex | V | O | Measured voltage on delta-phase CA. |
| measured_power_A | complex | VA | O | Measured power on phase A. |
| measured_power_B | complex | VA | O | Measured power on phase B. |
| measured_power_C | complex | VA | O | Measured power on phase C. |
| measured_power | complex | VA | O | Total measured power. |

#### Deltamode / In-Rush Properties

These properties control the numerical integration method used for in-rush current calculations during deltamode simulation. If left as `UNDEFINED`, the object inherits the global integration method setting.

Table: 17-load table 7 { #tbl:17-load-7 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| inrush_integration_method_capacitance | enumeration | N/A | I | Selected integration method to use for capacitive elements of the load. Valid values: `NONE`, `UNDEFINED`, `TRAPEZOIDAL`, `BACKWARD_EULER`. |
| inrush_integration_method_inductance | enumeration | N/A | I | Selected integration method to use for inductive elements of the load. Valid values: `NONE`, `UNDEFINED`, `TRAPEZOIDAL`, `BACKWARD_EULER`. |

### Load State of Development

Load is considered a well developed and validated model, with a number of features. Additional features may be included as needed. 
