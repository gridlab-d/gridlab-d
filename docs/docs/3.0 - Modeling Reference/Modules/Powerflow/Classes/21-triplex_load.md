## Triplex Load

Triplex load is similar to **load** and **ZIPload** in that load can be specified as a direct value, or as a base load, then a ZIP fraction applied to that base load. The load can be placed on phase 1 (120V), phase 2 (120V) or phase 12 (240V). Much like the **load** object, **player** objects can be used to vary the load with time. **triplex_load** objects provide a means to implement constant current, constant power, and constant impedance losses or generation into the system. The convention is a load is a positive quantity, so generation would need to be represented as a negative number. 

### Example Triplex Load

Loads can be a mixture of the constant current, constant impedance, and constant power types. A typical, mixed load would be implemented as 
    
    
    object triplex_load {
    	phases "AS";
    	name tplex_load;
    	constant_current_1 -0.586139+9.765222j;
    	constant_impedance_2 221.915014+104.430595j;
    	constant_power_12 4200.00+2100.00j;
    	nominal_voltage 120.0;
    	}

### Triplex Load Parameters

**triplex_load** objects are derived from **[triplex_node](19-triplex_node.md)** objects, so any parameters of the **[triplex_node](19-triplex_node.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

#### General Properties

Table: triplex_load table 1 { #tbl:21-triplex-load-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **load_class** | enumeration | N/A | I | Describes the type of load the object represents. Used for informational purposes only. Valid types may be referenced by keyword or number <br/> 0 - `U` \- Unknown load type <br/> 1 - `R` \- Residential load <br/> 2 - `C` \- Commercial load <br/> 3 - `I` \- Industrial load <br/> 4 - `A` \- Agricultural load |
| **load_priority** | enumeration | N/A | I | Load classification based on priority. Valid values: `DISCRETIONARY`, `PRIORITY`, `CRITICAL`. |

#### ZIP Load Properties

These properties define constant power, constant current, and constant impedance loads directly. The `_1`, `_2`, and `_12` variants represent loads on phase 1 (to neutral), phase 2 (to neutral), and phase 12 (between phases/delta) respectively. The `_real` and `_reac` suffixed variants provide access to the real and imaginary components individually.

Table: triplex_load table 2 { #tbl:21-triplex-load-2 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **constant_power_1** | complex | VA | IO | Constant power load on phase 1 (to neutral). |
| **constant_power_2** | complex | VA | IO | Constant power load on phase 2 (to neutral). |
| **constant_power_12** | complex | VA | IO | Constant power load on phase 12 (between phases, delta connection). |
| **constant_power_1_real** | double | W | IO | Constant power load on phase 1, real only. |
| **constant_power_2_real** | double | W | IO | Constant power load on phase 2, real only. |
| **constant_power_12_real** | double | W | IO | Constant power load on phase 12, real only. |
| **constant_power_1_reac** | double | VAr | IO | Constant power load on phase 1, imaginary only. |
| **constant_power_2_reac** | double | VAr | IO | Constant power load on phase 2, imaginary only. |
| **constant_power_12_reac** | double | VAr | IO | Constant power load on phase 12, imaginary only. |
| **constant_current_1** | complex | A | IO | Constant current load on phase 1 (to neutral). |
| **constant_current_2** | complex | A | IO | Constant current load on phase 2 (to neutral). |
| **constant_current_12** | complex | A | IO | Constant current load on phase 12 (between phases, delta connection). |
| **constant_current_1_real** | double | A | IO | Constant current load on phase 1, real only. |
| **constant_current_2_real** | double | A | IO | Constant current load on phase 2, real only. |
| **constant_current_12_real** | double | A | IO | Constant current load on phase 12, real only. |
| **constant_current_1_reac** | double | A | IO | Constant current load on phase 1, imaginary only. |
| **constant_current_2_reac** | double | A | IO | Constant current load on phase 2, imaginary only. |
| **constant_current_12_reac** | double | A | IO | Constant current load on phase 12, imaginary only. |
| **constant_impedance_1** | complex | Ohm | IO | Constant impedance load on phase 1 (to neutral). |
| **constant_impedance_2** | complex | Ohm | IO | Constant impedance load on phase 2 (to neutral). |
| **constant_impedance_12** | complex | Ohm | IO | Constant impedance load on phase 12 (between phases, delta connection). |
| **constant_impedance_1_real** | double | Ohm | IO | Constant impedance load on phase 1, real only. |
| **constant_impedance_2_real** | double | Ohm | IO | Constant impedance load on phase 2, real only. |
| **constant_impedance_12_real** | double | Ohm | IO | Constant impedance load on phase 12, real only. |
| **constant_impedance_1_reac** | double | Ohm | IO | Constant impedance load on phase 1, imaginary only. |
| **constant_impedance_2_reac** | double | Ohm | IO | Constant impedance load on phase 2, imaginary only. |
| **constant_impedance_12_reac** | double | Ohm | IO | Constant impedance load on phase 12, imaginary only. |

#### Measurement Properties

These properties are computed by the simulation and are output only. Note that the `measured_voltage` properties lag by one powerflow iteration; for the most current values, read the corresponding `voltage_*` properties from the parent **[triplex_node](19-triplex_node.md)** object directly.

Table: triplex_load table 3 { #tbl:21-triplex-load-3 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **measured_voltage_1** | complex | V | O | Measured voltage on phase 1. |
| **measured_voltage_2** | complex | V | O | Measured voltage on phase 2. |
| **measured_voltage_12** | complex | V | O | Measured voltage on phase 12 (between phases). |
| **indiv_measured_power_1** | complex | VA | O | Measured power on phase 1. |
| **indiv_measured_power_2** | complex | VA | O | Measured power on phase 2. |
| **indiv_measured_power_12** | complex | VA | O | Measured power on phase 12. |
| **measured_power** | complex | VA | O | Total measured power. |

#### Base Power and ZIP Fraction Properties

These properties provide an alternative way to specify ZIP loads using a base power value with fraction and power factor breakdowns per phase, following the same conventions as the ZIPload object. When `base_power` is nonzero for a phase, it overrides the corresponding `constant_power`, `constant_current`, and `constant_impedance` values for that phase.

Table: triplex_load table 4 { #tbl:21-triplex-load-4 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **base_power_1** | double | VA | I | Nominal power on phase 1 before applying ZIP fractions. |
| **base_power_2** | double | VA | I | Nominal power on phase 2 before applying ZIP fractions. |
| **base_power_12** | double | VA | I | Nominal power on phase 12 before applying ZIP fractions. |
| **power_pf_1** | double | pu | I | Power factor of the phase 1 constant power portion of load. |
| **current_pf_1** | double | pu | I | Power factor of the phase 1 constant current portion of load. |
| **impedance_pf_1** | double | pu | I | Power factor of the phase 1 constant impedance portion of load. |
| **power_pf_2** | double | pu | I | Power factor of the phase 2 constant power portion of load. |
| **current_pf_2** | double | pu | I | Power factor of the phase 2 constant current portion of load. |
| **impedance_pf_2** | double | pu | I | Power factor of the phase 2 constant impedance portion of load. |
| **power_pf_12** | double | pu | I | Power factor of the phase 12 constant power portion of load. |
| **current_pf_12** | double | pu | I | Power factor of the phase 12 constant current portion of load. |
| **impedance_pf_12** | double | pu | I | Power factor of the phase 12 constant impedance portion of load. |
| **power_fraction_1** | double | pu | IO | Constant power fraction of base power on phase 1. May be overwritten if ZIP fractions do not sum to 1. |
| **current_fraction_1** | double | pu | I | Constant current fraction of base power on phase 1. |
| **impedance_fraction_1** | double | pu | I | Constant impedance fraction of base power on phase 1. |
| **power_fraction_2** | double | pu | IO | Constant power fraction of base power on phase 2. May be overwritten if ZIP fractions do not sum to 1. |
| **current_fraction_2** | double | pu | I | Constant current fraction of base power on phase 2. |
| **impedance_fraction_2** | double | pu | I | Constant impedance fraction of base power on phase 2. |
| **power_fraction_12** | double | pu | IO | Constant power fraction of base power on phase 12. May be overwritten if ZIP fractions do not sum to 1. |
| **current_fraction_12** | double | pu | I | Constant current fraction of base power on phase 12. |
| **impedance_fraction_12** | double | pu | I | Constant impedance fraction of base power on phase 12. |

### Triplex Load State of Development

Triplex_load is considered a well developed and validated model, with a number of features. Additional features may be included as needed. 
