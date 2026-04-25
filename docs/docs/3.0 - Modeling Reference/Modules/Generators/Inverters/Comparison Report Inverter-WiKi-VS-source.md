# Updated Comparison Report
## Inverter(WiKi) vs Inverter(source)

## Summary
- **Properties in Inverter(WiKi)**: 50 properties [1]
- **Properties in Inverter(source)**: Approximately 89 properties [2]
- **Common properties**: 39 properties (78% of Inverter(WiKi) properties)
- **Unique to Inverter(WiKi)**: 11 properties
- **Unique to Inverter(source)**: Approximately 50 properties

---

## Common Properties (39 total)

| Property Name | Present in Both |
|---------------|----------------|
| inverter_type | ✓ |
| generator_status | ✓ |
| generator_mode | ✓ |
| four_quadrant_control_mode | ✓ |
| V_In | ✓ |
| I_In | ✓ |
| Vdc | ✓ |
| power_factor | ✓ |
| P_Out | ✓ |
| Q_Out | ✓ |
| use_multipoint_efficiency | ✓ |
| inverter_efficiency | ✓ |
| inverter_manufacturer | ✓ |
| maximum_dc_power | ✓ |
| maximum_dc_voltage | ✓ |
| minimum_dc_power | ✓ |
| c_o / c_0 | ✓ |
| c_1 | ✓ |
| c_2 | ✓ |
| c_3 | ✓ |
| sense_object | ✓ |
| max_charge_rate | ✓ |
| max_discharge_rate | ✓ |
| charge_on_threshold | ✓ |
| charge_off_threshold | ✓ |
| discharge_on_threshold | ✓ |
| discharge_off_threshold | ✓ |
| excess_input_power | ✓ |
| charge_lockout_time | ✓ |
| discharge_lockout_time | ✓ |
| V1 | ✓ |
| V2 | ✓ |
| V3 | ✓ |
| V4 | ✓ |
| Q1 | ✓ |
| Q2 | ✓ |
| Q3 | ✓ |
| Q4 | ✓ |
| V_base | ✓ |

---

## Properties Only in Inverter(WiKi) (11 properties)

| Property Name | Type | Unit | Description |
|---------------|------|------|-------------|
| **pf_reg_activate** | double | none | Lowest acceptable power-factor level below which power-factor regulation activates (default 0.8) [1] |
| **pf_reg_deactivate** | double | none | Power-factor level above which regulation is not needed (default 0.95) [1] |
| **pf_reg_activate_lockout_time** | double | s | Mandatory pause between deactivation and reactivation of PF regulation (default 60s) [1] |
| **charge_threshold** | double | W | Level at which all group inverters begin charging batteries [1] |
| **discharge_threshold** | double | W | Level at which all group inverters begin discharging batteries [1] |
| **group_max_charge_rate** | double | W | Sum of charge rates of batteries in group load-following [1] |
| **group_max_discharge_rate** | double | W | Sum of discharge rates of batteries in group load-following [1] |
| **group_rated_power** | double | W | Sum of inverter power ratings in group power-factor regulation [1] |

---

## Properties Only in Inverter(source) (approximately 50 properties)

### **Control and Operating Modes**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **pf_reg** | - | enumeration | Power factor regulation activation |
| **islanded_state** | - | bool | Boolean for control modes under island conditions |
| **IEEE_1547_trip_method** | - | enumeration | Reason for IEEE 1547 disconnect [2] |
| **enable_1547_checks** | - | bool | Enable IEEE 1547-2003 disconnect checking [2] |
| **reconnect_time** | s | double | Time delay after IEEE 1547-2003 violation clears before resuming generation [2] |
| **inverter_1547_status** | - | bool | Indicator if the inverter is curtailed due to a 1547 violation or not [2] |
| **IEEE_1547_version** | - | enumeration | Version of IEEE 1547 to use to populate defaults [2] |

### **Convergence and Initialization**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **inverter_convergence_criterion** | - | double | Maximum change in error threshold for exiting deltamode |
| **current_convergence** | A | double | Convergence criterion for current changes on first timestep |

### **Power and Voltage Parameters**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **P_In** | W | double | DC power |
| **VA_Out** | VA | complex | AC power [2] |
| **power_in** | W | double | Legacy model - no longer used |
| **rated_power** | VA | double | The rated power of the inverter |
| **rated_battery_power** | W | double | Rated power of battery when attached |

### **Battery Management**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **battery_soc** | pu | double | State of charge of attached battery |
| **soc_reserve** | pu | double | Reserve state of charge for islanding cases |

### **Frequency Parameters**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **nominal_frequency** | Hz | double | Nominal frequency |
| **over_freq_high_cutout** | Hz | double | OF2 set point for IEEE 1547a [2] |
| **over_freq_high_disconnect_time** | s | double | OF2 clearing time for IEEE1547a [2] |
| **over_freq_low_cutout** | Hz | double | OF1 set point for IEEE 1547a [2] |
| **over_freq_low_disconnect_time** | s | double | OF1 clearing time for IEEE 1547a [2] |
| **under_freq_high_cutout** | Hz | double | UF2 set point for IEEE 1547a [2] |
| **under_freq_high_disconnect_time** | s | double | UF2 clearing time for IEEE1547a [2] |
| **under_freq_low_cutout** | Hz | double | UF1 set point for IEEE 1547a [2] |
| **under_freq_low_disconnect_time** | s | double | UF1 clearing time for IEEE 1547a [2] |

### **Under-Voltage Protection**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **under_voltage_low_cutout** | pu | double | Lowest voltage threshold for undervoltage [2] |
| **under_voltage_middle_cutout** | pu | double | Middle-lowest voltage threshold for undervoltage [2] |
| **under_voltage_high_cutout** | pu | double | High value of low voltage threshold for undervoltage [2] |
| **under_voltage_low_disconnect_time** | s | double | Lowest voltage clearing time for undervoltage [2] |
| **under_voltage_middle_disconnect_time** | s | double | Middle-lowest voltage clearing time for undervoltage [2] |
| **under_voltage_high_disconnect_time** | s | double | Highest voltage clearing time for undervoltage [2] |

### **Over-Voltage Protection**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **over_voltage_low_cutout** | pu | double | Lowest voltage value for overvoltage [2] |
| **over_voltage_high_cutout** | pu | double | High voltage value for overvoltage [2] |
| **over_voltage_low_disconnect_time** | s | double | Lowest voltage clearing time for overvoltage [2] |
| **over_voltage_high_disconnect_time** | s | double | Highest voltage clearing time for overvoltage [2] |

### **Phase-Specific Measurements**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **phases** | - | set | The phases the inverter is attached to [2] |
| **phaseA_V_Out** | V | complex | AC voltage on A phase in three-phase system; 240-V connection on a triplex system [2] |
| **phaseB_V_Out** | V | complex | AC voltage on B phase [2] |
| **phaseC_V_Out** | V | complex | AC voltage on C phase [2] |
| **phaseA_I_Out** | V | complex | AC current on A phase in three-phase system; 240-V connection on a triplex system [2] |
| **phaseB_I_Out** | V | complex | AC current on B phase [2] |
| **phaseC_I_Out** | V | complex | AC current on C phase [2] |
| **power_A** | VA | complex | AC power on A phase in three-phase system; 240-V connection on a triplex system [2] |
| **power_B** | VA | complex | AC power on B phase [2] |
| **power_C** | VA | complex | AC power on C phase [2] |
| **curr_VA_out_A** | VA | complex | AC power on A phase in three-phase system; 240-V connection on a triplex system [2] |
| **curr_VA_out_B** | VA | complex | AC power on B phase [2] |
| **curr_VA_out_C** | VA | complex | AC power on C phase [2] |
| **prev_VA_out_A** | VA | complex | AC power on A phase in three-phase system; 240-V connection on a triplex system [2] |
| **prev_VA_out_B** | VA | complex | AC power on B phase [2] |
| **prev_VA_out_C** | VA | complex | AC power on C phase [2] |

### **Grid-Forming/Following Currents**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **phaseA_I_Forming** | A | complex | AC current on A phase, only for grid_forming [2] |
| **phaseB_I_Forming** | A | complex | AC current on B phase, only for grid_forming [2] |
| **phaseC_I_Forming** | A | complex | AC current on C phase, only for grid_forming [2] |
| **phaseA_I_Following** | A | complex | AC current on A phase, only for grid_following [2] |
| **phaseB_I_Following** | A | complex | AC current on B phase, only for grid_following [2] |
| **phaseC_I_Following** | A | complex | AC current on C phase, only for grid_following [2] |

### **DELTAMODE Control Parameters**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **Pref** | - | double | Real power reference |
| **Qref** | - | double | Reactive power reference |
| **kpd** | - | double | D-axis integration gain for current modulation PI controller |
| **kpq** | - | double | Q-axis integration gain for current modulation PI controller |
| **kid** | - | double | D-axis proportional gain for current modulation PI controller |
| **mdA** | - | double | The d-axis current modulation for phase A or triplex phase [2] |
| **mdB** | - | double | The d-axis current modulation for phase B [2] |
| **mdC** | - | double | The d-axis current modulation for phase C [2] |
| **mqA** | - | double | The q-axis current modulation for phase A or triplex phase [2] |
| **mqB** | - | double | The q-axis current modulation for phase B [2] |
| **mqC** | - | double | The q-axis current modulation for phase C [2] |
| **delta_mdA** | - | double | The change in d-axis current modulation for phase A or triplex phase [2] |
| **delta_mdB** | - | double | The change in d-axis current modulation for phase B [2] |
| **delta_mdC** | - | double | The change in d-axis current modulation for phase C [2] |
| **delta_mqA** | - | double | The change in q-axis current modulation for phase A or triplex phase [2] |
| **delta_mqB** | - | double | The change in q-axis current modulation for phase B [2] |
| **delta_mqC** | - | double | The change in q-axis current modulation for phase C [2] |
| **IdqA** | - | complex | The dq-axis current for phase A or triplex phase [2] |
| **IdqB** | - | complex | The dq-axis current for phase B [2] |
| **IdqC** | - | complex | The dq-axis current for phase C [2] |
| **Tfreq_delay** | - | double | The time constant for delayed frequency seen by the inverter [2] |

### **Internal Grid-Forming Source Parameters**
| Property Name | Unit | Type | Description |
|---------------|------|------|-------------|
| **e_source_A** | - | double | Internal voltage of grid-forming source, phase A [2] |
| **e_source_B** | - | double | Internal voltage of grid-forming source, phase B [2] |
| **e_source_C** | - | double | Internal voltage of grid-forming source, phase C [2] |
| **V_angle_A** | - | double | Internal angle of grid-forming source, phase A [2] |
| **V_angle_B** | - | double | Internal angle of grid-forming source, phase B [2] |
| **V_angle_C** | - | double | Internal angle of grid-forming source, phase C [2] |
| **pCircuit_V_Avg** | - | double | Three-phase average value of terminal voltage [2] |

---

## Key Observations

1. **Inverter(source) is significantly more comprehensive**: It includes approximately 50 additional properties not found in Inverter(WiKi) [2]

2. **Enhanced IEEE 1547 compliance**: Inverter(source) includes extensive voltage and frequency protection parameters for grid interconnection standards [2]

3. **Advanced control features**: Inverter(source) adds DELTAMODE control with PI controller parameters and detailed current modulation capabilities [2]

4. **Detailed phase monitoring**: Inverter(source) provides individual phase voltage, current, and power measurements for three-phase systems [2]

5. **Simplified power factor regulation**: Inverter(source) appears to use a different approach to PF regulation compared to the three separate properties in Inverter(WiKi) [1]

6. **Grid-forming/following support**: Inverter(source) distinguishes between grid-forming and grid-following operation modes with separate current measurements [2]

7. **Battery integration**: Both tables support battery integration, with Inverter(source) likely adding more explicit state-of-charge tracking capabilities

8. **Islanding support**: Inverter(source) adds explicit islanded operation support

9. **Group control features**: Inverter(WiKi) includes specific group control properties (group_max_charge_rate, group_max_discharge_rate, group_rated_power) that are not explicitly present in Inverter(source) [1]

Inverter(source) represents a more advanced and feature-rich version with enhanced grid compliance, monitoring capabilities, and control sophistication, particularly for DELTAMODE simulations and IEEE 1547 standard compliance.