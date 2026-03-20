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

**load** objects are derived from the **node** objects, so all of the same properties apply. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**load_class**  | enumeration  | N/A  | Describes the type of load the object represents. Used for informational purposes only. Valid types may be referenced by keyword or number <br/> 0 - `U` \- Unknown load type <br/> 1 - `R` \- Residential load <br/> 2 - `C` \- Commercial load <br/> 3 - `I` \- Industrial load <br/> 4 - `A` \- Agricultural load  
**measured_voltage_A**  | complex  | Volts  | A point to measure the voltage on phase `A` of the load. Note that this value will be from the previous powerflow iteration, so may not be up to date. For best results, read `voltage_A` directly.   
**measured_voltage_B**  | complex  | Volts  | A point to measure the voltage on phase `B` of the load. Note that this value will be from the previous powerflow iteration, so may not be up to date. For best results, read `voltage_B` directly.   
**measured_voltage_C**  | complex  | Volts  | A point to measure the voltage on phase `C` of the load. Note that this value will be from the previous powerflow iteration, so may not be up to date. For best results, read `voltage_C` directly.   
**measured_voltage_AB**  | complex  | Volts  | A point to measure the voltage on delta-phase `AB` of the load. Note that this value will be from the previous powerflow iteration, so may not be up to date. For best results, read `voltage_AB` directly.   
**measured_voltage_BC**  | complex  | Volts  | A point to measure the voltage on delta-phase `BC` of the load. Note that this value will be from the previous powerflow iteration, so may not be up to date. For best results, read `voltage_BC` directly.   
**measured_voltage_CA**  | complex  | Volts  | A point to measure the voltage on delta-phase `CA` of the load. Note that this value will be from the previous powerflow iteration, so may not be up to date. For best results, read `voltage_CA` directly.   
_The following terms define the load in classical format as constant power, current, and impedance loads on each phase._  
**constant_power_A**  | complex  | Volt-Amperes  | The constant power quantity of the load attached to phase `A` in a wye connection and phase `AB` in a delta connection.   
**constant_power_B**  | complex  | Volt-Amperes  | The constant power quantity of the load attached to phase `B` in a wye connection and phase `BC` in a delta connection.   
**constant_power_C**  | complex  | Volt-Amperes  | The constant power quantity of the load attached to phase `C` in a wye connection and phase `CA` in a delta connection.   
**constant_current_A**  | complex  | Amperes  | The constant current quantity of the load attached to phase `A` in a wye connection and phase `AB` in a delta connection.   
**constant_current_B**  | complex  | Amperes  | The constant current quantity of the load attached to phase `B` in a wye connection and phase `BC` in a delta connection.   
**constant_current_C**  | complex  | Amperes  | The constant current quantity of the load attached to phase `C` in a wye connection and phase `CA` in a delta connection.   
**constant_impedance_A**  | complex  | Ohms  | The constant impedance quantity of the load attached to phase `A` in a wye connection and phase `AB` in a delta connection.   
**constant_impedance_B**  | complex  | Ohms  | The constant impedance quantity of the load attached to phase `B` in a wye connection and phase `BC` in a delta connection.   
**constant_impedance_C**  | complex  | Ohms  | The constant impedance quantity of the load attached to phase `C` in a wye connection and phase `CA` in a delta connection.   

_The following terms are NOT used in conjunction with the previous set._  
_These terms are used in the manner of a **ZIPload** \- base power (in VA) is specified on a by-phase basis, then power factor and ZIP fractions for each are specified. All phase rotations are handled internally._  

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**base_power_A**  | double  | VA  | in similar format as ZIPload this represents the nominal power on phase A before applying ZIP fractions   
**base_power_B**  | double  | VA  | in similar format as ZIPload this represents the nominal power on phase B before applying ZIP fractions   
**base_power_C**  | double  | VA  | in similar format as ZIPload this represents the nominal power on phase C before applying ZIP fractions   
**power_pf_A**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase A constant power portion of load   
**current_pf_A**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase A constant current portion of load   
**impedance_pf_A**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase A constant impedance portion of load   
**power_pf_B**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase B constant power portion of load   
**current_pf_B**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase B constant current portion of load   
**impedance_pf_B**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase B constant impedance portion of load   
**power_pf_C**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase C constant power portion of load   
**current_pf_C**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase C constant current portion of load   
**impedance_pf_C**  | double  | pu  | in similar format as ZIPload this is the power factor of the phase C constant impedance portion of load   
**power_fraction_A**  | double  | pu  | this is the constant power fraction of base power on phase A   
**current_fraction_A**  | double  | pu  | this is the constant current fraction of base power on phase A   
**impedance_fraction_A**  | double  | pu  | this is the constant impedance fraction of base power on phase A   
**power_fraction_B**  | double  | pu  | this is the constant power fraction of base power on phase B   
**current_fraction_B**  | double  | pu  | this is the constant current fraction of base power on phase B   
**impedance_fraction_B**  | double  | pu  | this is the constant impedance fraction of base power on phase B   
**power_fraction_C**  | double  | pu  | this is the constant power fraction of base power on phase C   
**current_fraction_C**  | double  | pu  | this is the constant current fraction of base power on phase C   
**impedance_fraction_C**  | double  | pu  | this is the constant impedance fraction of base power on phase C   
  
### Load State of Development

Load is considered a well developed and validated model, with a number of features. Additional features may be included as needed. 

