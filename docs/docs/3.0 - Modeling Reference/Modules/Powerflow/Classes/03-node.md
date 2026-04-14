## Node

The node object is equivalent to a bus of the distribution system. It provides a connection point for **link**-based objects and a point of known voltages on the system. Three phase voltage is typically available in either wye-connected or delta-connected three phase. Wye-connected voltages are contained in `voltage_A`, `voltage_B`, and `voltage_C`. Delta-connected voltages are available in `voltage_AB`, `voltage_BC`, and `voltage_CA`. 

### Default Node

A minimalist node could be created with 
    
    
    object node {
    	name NodeOne;
    	phases ABC;
    	nominal_voltage 7200.0;
    	}
    

which is the same as specifying 
    
    
    object node {
    	name NodeOne;
    	phases ABC;
    	nominal_voltage 7200.0;
    	voltage_A 7200.0+0d;
    	voltage_B 7200.0-120.0d;
    	voltage_C 7200.0+120.0d;
    	bustype PQ;
    	}
    

### Node Parameters

**node** objects are derived from **[powerflow_object](02-powerflow_object.md)** objects, so any parameters of the **[powerflow_object](02-powerflow_object.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| bustype | enumeration | N/A | ✓ | ✓ | The type of bus the node represents. The different bus distinctions are only valid for the Gauss-Seidel and Newton-Raphson solver methods. The Forward-Back Sweep method (Kersting's method) does not presently incorporate anything other than the `PQ` bus. Valid choices are <br/> - `PQ` for a constant power bus (default) <br/> - `PV` for a voltage-controlled (magnitude) bus <br/> - `SWING` for the infinite bus of a system. |
| busflags | set | N/A | ✓ |  | A flag to indicate if the current bus has a source or not. Mainly used for `PV` implementations. The only valid entries are `HASSOURCE` to indicate it is a supported bus, or an empty value indicating it is not. Unused at this time. |
| reference_bus | object | N/A | ✓ |  | A reference node elsewhere in the system that the **node** will use to obtain frequency information if necessary (unimplemented in GridLAB-D™ at this point). |
| maximum_voltage_error | double | V | ✓ |  | The maximum voltage error for convergence checks in the different powerflow solvers. If left blank, it is derived from the `nominal_voltage` parameter. |
| voltage_A | complex | V | ✓ | ✓ | The voltage on phase `A` of a three-phase system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats. |
| voltage_B | complex | V | ✓ | ✓ | The voltage on phase `B` of a three-phase system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats. |
| voltage_C | complex | V | ✓ | ✓ | The voltage on phase `C` of a three-phase system. This may be specified in rectangular (7200.0+0.0j) or polar (7200.0+0.0d) formats. |
| voltage_AB | complex | V | ✓ | ✓ | The voltage on phase `AB` of a delta-connected three-phase system. This is a derived quantity and can be read, but it is not recommended you set this value. |
| voltage_BC | complex | V | ✓ | ✓ | The voltage on phase `BC` of a delta-connected three-phase system. This is a derived quantity and can be read, but it is not recommended you set this value. |
| voltage_CA | complex | V | ✓ | ✓ | The voltage on phase `CA` of a delta-connected three-phase system. This is a derived quantity and can be read, but it is not recommended you set this value. |
| mean_repair_time | double | s | ✓ |  | Time after a fault clears for the object to be considered back in service. Mainly used for **reliability** module interactions at this time. |
| frequency_measure_type | enumeration | N/A | ✓ | ✓ | ⚠️ Frequency measurement dynamics-capable implementation Valid values: `NONE`, `SIMPLE`, `PLL`. |
| sfm_Tf | double | s | ✓ |  | ⚠️ Transducer time constant for simplified frequency measurement (seconds) |
| pll_Kp | double | pu | ✓ |  | ⚠️ Proportional gain of PLL frequency measurement |
| pll_Ki | double | pu | ✓ |  | ⚠️ Integration gain of PLL frequency measurement |
| measured_angle_A | double | rad | ✓ | ✓ | ⚠️ bus angle measurement, phase A |
| measured_frequency_A | double | Hz | ✓ | ✓ | ⚠️ frequency measurement, phase A |
| measured_angle_B | double | rad | ✓ | ✓ | ⚠️ bus angle measurement, phase B |
| measured_frequency_B | double | Hz | ✓ | ✓ | ⚠️ frequency measurement, phase B |
| measured_angle_C | double | rad | ✓ | ✓ | ⚠️ bus angle measurement, phase C |
| measured_frequency_C | double | Hz | ✓ | ✓ | ⚠️ frequency measurement, phase C |
| measured_frequency | double | Hz | ✓ | ✓ | ⚠️ frequency measurement - average of present phases |
| service_status | enumeration | N/A | ✓ | ✓ | ⚠️ In and out of service flag Valid values: `IN_SERVICE`, `OUT_OF_SERVICE`. |
| service_status_double | double | N/A | ✓ | ✓ | ⚠️ In and out of service flag - type double - will indiscriminately override service_status - useful for schedules |
| previous_uptime | double | min | ✓ | ✓ | ⚠️ Previous time between disconnects of node in minutes |
| current_uptime | double | min | ✓ | ✓ | ⚠️ Current time since last disconnect of node in minutes |
| GFA_enable | bool | N/A | ✓ |  | ⚠️ Disable/Enable Grid Friendly Appliance(TM)-type functionality |
| GFA_freq_low_trip | double | Hz | ✓ |  | ⚠️ Low frequency trip point for Grid Friendly Appliance(TM)-type functionality |
| GFA_freq_high_trip | double | Hz | ✓ |  | ⚠️ High frequency trip point for Grid Friendly Appliance(TM)-type functionality |
| GFA_volt_low_trip | double | pu | ✓ |  | ⚠️ Low voltage trip point for Grid Friendly Appliance(TM)-type functionality |
| GFA_volt_high_trip | double | pu | ✓ |  | ⚠️ High voltage trip point for Grid Friendly Appliance(TM)-type functionality |
| GFA_reconnect_time | double | s | ✓ |  | ⚠️ Reconnect time for Grid Friendly Appliance(TM)-type functionality |
| GFA_freq_disconnect_time | double | s | ✓ |  | ⚠️ Frequency violation disconnect time for Grid Friendly Appliance(TM)-type functionality |
| GFA_volt_disconnect_time | double | s | ✓ |  | ⚠️ Voltage violation disconnect time for Grid Friendly Appliance(TM)-type functionality |
| GFA_status | bool | N/A | ✓ | ✓ | ⚠️ Grid Friendly Appliance(TM)-type functionality - whether it is in service (not tripped) or not |
| GFA_trip_method | enumeration | N/A | ✓ | ✓ | ⚠️ Reason for GFA trip - what caused the GFA to activate Valid values: `NONE`, `UNDER_FREQUENCY`, `OVER_FREQUENCY`, `UNDER_VOLTAGE`, `OVER_VOLTAGE`. |
| topological_parent | object | N/A | ✓ |  | ⚠️ topological parent as per GLM configuration |
| behaving_as_swing | bool | N/A | ✓ | ✓ | ⚠️ Indicator flag for if a bus is behaving as a reference voltage source - valid for a SWING or SWING_PQ |


### Node State of Development

Node is considered a highly developed and validated model. 

