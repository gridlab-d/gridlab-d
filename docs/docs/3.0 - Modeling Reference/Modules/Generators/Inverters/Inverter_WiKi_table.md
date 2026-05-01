Property name | Type | Unit | Description   
---|---|---|---  
**inverter_type** | enumeration | none | Defines type of inverter technology and efficiency of the unit (FOUR_QUADRANT , PWM, TWELVE_PULSE, SIX_PULSE, TWO_PULSE)   
**generator_status** | enumeration | none | Defines if generator is in operation or not (ONLINE, OFFLINE)   
**generator_mode** | enumeration | none | Control mode of the inverter (SUPPLY_DRIVEN, CONSTANT_PF, CONSTANT_PQ, CONSTANT_V, UNKNOWN)   
**four_quadrant_control_mode** | enumeration | none | Control mode of the inverter when FOUR_QUADRANT (NONE, CONSTANT_PQ, CONSTANT_PF, CONSTANT_V, VOLT_VAR)   
**V_In** | complex | V | DC voltage passed in by the DC object (e.g. solar panel or battery)   
**I_In** | complex | A | DC current passed in by the DC object (e.g. solar panel or battery)   
**Vdc** | complex | V | _Not used at this time_  
**power_factor** | double | unit | Defines desired power factor in generator mode CONSTANT_PF mode and in four quadrant control mode CONSTANT_PF  
**P_Out** | double | VA | Value to output in four quadrant control mode CONSTANT_PQ  
**Q_Out** | double | VAr | Value to output in four quadrant control mode CONSTANT_PQ  
**use_multipoint_efficiency** | bool | none | A boolean flag to toggle using Sandia National Laboratory's multipoint efficiency model   
**inverter_efficiency** | double | none | One-way (not round-trip) constant efficiency of the inverter   
**inverter_manufacturer** | enumeration | none | Defines default parameters for the multipoint efficiency model for an inverter from manufacturer (NONE, FRONIUS, SMA, XANTREX)   
**maximum_dc_power** | double | W | The maximum DC power rating of the inverter, only used when use_multipoint_efficiency is TRUE  
**maximum_dc_voltage** | double | V | The maximum DC voltage rating of the inverter, only used when use_multipoint_efficiency is TRUE  
**minimum_dc_power** | double | W | The minimum DC voltage rating of the inverter, only used when use_multipoint_efficiency is TRUE  
**c_o** | double | 1/W | The coefficient descibing the parabolic relationship between AC and DC power of the inverter, only used when use_multipoint_efficiency is TRUE  
**c_1** | double | 1/V | The coefficient allowing the maximum DC power to vary linearly with DC voltage, only used when use_multipoint_efficiency is TRUE  
**c_2** | double | 1/V | The coefficient allowing the minimum DC power to vary linearly with DC voltage, only used when use_multipoint_efficiency is TRUE  
**c_3** | double | 1/V | The coefficient allowing c_0 to vary linearly with DC voltage, only used when use_multipoint_efficiency is TRUE  
**sense_object** | object | none | FOUR QUADRANT MODEL: name of the object the inverter is trying to mitigate the load on (node/link) in LOAD_FOLLOWING and supplement mode pf_reg   
**max_charge_rate** | double | W | FOUR QUADRANT MODEL: name of the object the inverter is trying to mitigate the load on (node/link) in LOAD_FOLLOWING   
**max_discharge_rate** | double | W | FOUR QUADRANT MODEL: maximum rate the battery can be discharged in LOAD_FOLLOWING   
**charge_on_threshold** | double | W | FOUR QUADRANT MODEL: power level of the sense_object at which the inverter should try charging the battery in LOAD_FOLLOWING   
**charge_off_threshold** | double | W | FOUR QUADRANT MODEL: power level of the sense_object at which the inverter should cease charging the battery in LOAD_FOLLOWING   
**discharge_on_threshold** | double | W | FOUR QUADRANT MODEL: power level of the sense_object at which the inverter should try discharging the battery in LOAD_FOLLOWING   
**discharge_off_threshold** | double | W | FOUR QUADRANT MODEL: power level of the sense_object at which the inverter should cease discharging the battery in LOAD_FOLLOWING   
**excess_input_power** | double | W | FOUR QUADRANT MODEL: Excess power at the input of the inverter that is otherwise just lost, or could be shunted to a battery   
**charge_lockout_time** | double | s | FOUR QUADRANT MODEL: Lockout time when a charging operation occurs before another LOAD_FOLLOWING dispatch operation can occur   
**discharge_lockout_time** | double | s | FOUR QUADRANT MODEL: Lockout time when a discharging operation occurs before another LOAD_FOLLOWING dispatch operation can occur   
**pf_reg_activate** | double | none | FOUR QUADRANT MODEL: Lowest acceptable power-factor level of the sense_object below which power-factor regulation will activate. Default value is 0.8.   
**pf_reg_deactivate** | double | none | FOUR QUADRANT MODEL: Lowest acceptable power-factor of the sense_object above which no power-factor regulation is needed. Default value is 0.95.   
**pf_reg_activate_lockout_time** | double | s | FOUR QUADRANT MODEL: Mandatory pause between the deactivation of power-factor regulation and it reactivation. Default value is 60s.   
**charge_threshold** | double | W | FOUR QUADRANT MODEL: Level at which all inverters in the group will begin charging attached batteries. Regulated minimum load level   
**discharge_threshold** | double | W | FOUR QUADRANT MODEL: Level at which all inverters in the group will begin discharging attached batteries. Regulated maximum load level   
**group_max_charge_rate** | double | W | FOUR QUADRANT MODEL: Sum of the charge rates of the batteries involved in the group load-following   
**group_max_discharge_rate** | double | W | FOUR QUADRANT MODEL: Sum of the discharge rates of the batteries involved in the group load-following   
**group_rated_power** | double | W | FOUR QUADRANT MODEL: Sum of the inverter power ratings of the inverters involved in the group power-factor regulation   
**V_base** | double | V | FOUR QUADRANT MODEL: The base voltage on the grid side of the inverter. Used in VOLT_VAR control mode   
**V1** | double | pu | FOUR QUADRANT MODEL: voltage point 1 in volt/var curve. Used in VOLT_VAR control mode   
**V2** | double | pu | FOUR QUADRANT MODEL: voltage point 2 in volt/var curve. Used in VOLT_VAR control mode   
**V3** | double | pu | FOUR QUADRANT MODEL: voltage point 3 in volt/var curve. Used in VOLT_VAR control mode   
**V4** | double | pu | FOUR QUADRANT MODEL: voltage point 4 in volt/var curve. Used in VOLT_VAR control mode   
**Q1** | double | pu | FOUR QUADRANT MODEL: VAR point 1 in volt/var curve. Used in VOLT_VAR control mode   
**Q2** | double | pu | FOUR QUADRANT MODEL: VAR point 2 in volt/var curve. Used in VOLT_VAR control mode   
**Q3** | double | pu | FOUR QUADRANT MODEL: VAR point 3 in volt/var curve. Used in VOLT_VAR control mode   
**Q4** | double | pu | FOUR QUADRANT MODEL: VAR point 4 in volt/var curve. Used in VOLT_VAR control mode 