## Triplex Load

Triplex load is similar to **load** and **ZIPload** in that load can be specified as a direct value, or as a base load, then a ZIP fraction applied to that base load. The load can be placed on phase 1 (120V), phase 2 (120V) or phase 12 (240V). Much like the **load** object, **player** objects can be used to vary the load with time. **triplex_load** objects provide a means to implement constant current, constant power, and constant impedance losses or generation into the system. The convention is a load is a positive quantity, so generation would need to be represented as a negative number. 

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

**triplex_load** objects are derived from the **triplex_node** objects, so all of the same properties apply. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**load_class**  | enumeration  | N/A  | Describes the type of load the object represents. Used for informational purposes only. Valid types may be referenced by keyword or number <br/> 0 - `U` \- Unknown load type <br/> 1 - `R` \- Residential load <br/> 2 - `C` \- Commercial load <br/> 3 - `I` \- Industrial load <br/> 4 - `A` \- Agricultural load  
**load_priority**  | enumeration  | N/A  | Describes how the load could be treated for any prioritization schemes. Used for informational purposes only, or via external controls (no built in functionality uses these). Valid types may be referenced by keyword or number <br/> 0 - `DISCRETIONARY` <br/> 1 - `PRIORITY` <br/> 2 - `CRITICAL`  
**measured_voltage_1**  | complex  | Volts  | A point to measure the voltage on phase `1` of the load. Note that this value will be from the previous powerflow iteration, so may not be up to date. For best results, read `voltage_1` directly.   
**measured_voltage_2**  | complex  | Volts  | A point to measure the voltage on phase `2` of the load. Note that this value will be from the previous powerflow iteration, so may not be up to date. For best results, read `voltage_2` directly.   
**measured_voltage_12**  | complex  | Volts  | A point to measure the voltage between phase `1` and `2` (e.g., 240-volt connection) of the load. Note that this value will be from the previous powerflow iteration, so may not be up to date. For best results, read `voltage_12` directly.   

_The following terms define the load in classical format as constant power, current, and impedance loads on each phase._  

Property Name  | Type  | Unit  | Description   
---|---|---|--- 
**constant_power_1**  | complex  | Volt-Amperes  | The constant power quantity of the load attached to phase `1` and the neutral/ground.   
**constant_power_2**  | complex  | Volt-Amperes  | The constant power quantity of the load attached to phase `2` and the neutral/ground.   
**constant_power_12**  | complex  | Volt-Amperes  | The constant power quantity of the load attached between phase `1` and `2` \- effectively as a delta connection.   
**constant_current_1**  | complex  | Amperes  | The constant current quantity of the load attached to phase `1` and the neutral/ground.   
**constant_current_2**  | complex  | Amperes  | The constant current quantity of the load attached to phase `2` and the neutral/ground.   
**constant_current_12**  | complex  | Amperes  | The constant current quantity of the load attached between phase `1` and `2` \- effectively as a delta connection.   
**constant_impedance_1**  | complex  | Ohms  | The constant impedance quantity of the load attached to phase `1` and the neutral/ground.   
**constant_impedance_2**  | complex  | Ohms  | The constant impedance quantity of the load attached to phase `2` and the neutral/ground.   
**constant_impedance_12**  | complex  | Ohms  | The constant impedance quantity of the load attached between phase `1` and `2` \- effectively as a delta connection. 

_The following terms are NOT used in conjunction with the previous set._  
_These terms are used in the manner of a[ZIPload] \- base power (in VA) is specified on a by-phase basis, then power factor and ZIP fractions for each are specified. All phase rotations are handled internally._  

Property Name  | Type  | Unit  | Description   
---|---|---|--- 
**base_power_1**  | double  | VA  | in similar format as ZIPload this represents the nominal power on phase 1 before applying ZIP fractions   
**base_power_2**  | double  | VA  | in similar format as ZIPload this represents the nominal power on phase 2 before applying ZIP fractions   
**base_power_12**  | double  | VA  | in similar format as ZIPload this represents the nominal power on phase connection 12 before applying ZIP fractions   
**power_pf_1**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase 1 constant power portion of load   
**current_pf_1**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase 1 constant current portion of load   
**impedance_pf_1**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase 1 constant impedance portion of load   
**power_pf_2**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase 2 constant power portion of load   
**current_pf_2**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase 2 constant current portion of load   
**impedance_pf_2**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase 2 constant impedance portion of load   
**power_pf_12**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase connection 12 constant power portion of load   
**current_pf_12**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase connection 12 constant current portion of load   
**impedance_pf_12**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase connection 12 constant impedance portion of load   
**power_fraction_1**  | double  | pu  | this is the constant power fraction of base power on phase 1   
**current_fraction_1**  | double  | pu  | this is the constant current fraction of base power on phase 1   
**impedance_fraction_1**  | double  | pu  | this is the constant impedance fraction of base power on phase 1   
**power_fraction_2**  | double  | pu  | this is the constant power fraction of base power on phase 2   
**current_fraction_2**  | double  | pu  | this is the constant current fraction of base power on phase 2   
**impedance_fraction_2**  | double  | pu  | this is the constant impedance fraction of base power on phase 2   
**power_fraction_12**  | double  | pu  | this is the constant power fraction of base power on phase connection 12   
**current_fraction_12**  | double  | pu  | this is the constant current fraction of base power on phase connection 12   
**impedance_fraction_12**  | double  | pu  | this is the constant impedance fraction of base power on phase connection 12   
  
### Triplex Load State of Development

Triplex_load is considered a well developed and validated model, with a number of features. Additional features may be included as needed. 

