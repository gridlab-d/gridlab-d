## Load Tracker
!!! warning

    The load_tracker class is deprecated and will be removed in a future release. 

The load_tracker object monitors a property on some "target" object (e.g., real power, magnitude of complex power, etc.) and adjusts an output scaling value so that the measured load tracks a user-defined setpoint. Other objects in the model can then read this output value to scale their behavior.

### Sample

    object load_tracker
    {
        name RECLOSER_1_LOAD_TRACKER;
        target RECLOSER_1;
        target_property power_in;
        operation MAGNITUDE;
        setpoint 145000;
        deadband 1.0; // Ensure double_assert.within > 219000 * deadband (as a percent)
        damping  0.0;
        full_scale 219000; // Note: This is the sum of the base powers 
                        //       from the 2 downstream loads. It does
                        //       not take account of line losses but
                        //       is the expected value users would enter
    }

    object load
    {
        name LOAD_2;
        parent NODE_4;
        phases ABC;
        nominal_voltage 6350.85;
        base_power_A RECLOSER_1_LOAD_TRACKER.output*22000; 
        base_power_B RECLOSER_1_LOAD_TRACKER.output*22000;
        base_power_C RECLOSER_1_LOAD_TRACKER.output*22000;
        power_fraction_A 0.5;
        power_fraction_B 0.5;
        power_fraction_C 0.5;
        current_fraction_A 0.25;
        current_fraction_B 0.25;
        current_fraction_C 0.25;
        impedance_fraction_A 0.25;
        impedance_fraction_B 0.25;
        impedance_fraction_C 0.25;
        power_pf_A 0.9;
        power_pf_B 0.9;
        power_pf_C 0.9;
        current_pf_A 0.8;
        current_pf_B 0.8;
        current_pf_C 0.8;
        impedance_pf_A 0.8;
        impedance_pf_B 0.8;
        impedance_pf_C 0.8;
    };

### Load Tracker Parameters

#### Properties

**load_tracker** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: load_tracker table 1 { #tbl:46-load-tracker-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **target** | object | N/A | I | target object to track the load of |
| **target_property** | char256 | N/A | I | property on the target object representing the load |
| **operation** | enumeration | N/A | I | operation to perform on complex property types Valid values: `REAL`, `IMAGINARY`, `MAGNITUDE`, `ANGLE`. |
| **full_scale** | double | N/A | I | magnitude of the load at full load, used for feed-forward control |
| **setpoint** | double | N/A | I | load setpoint to track to |
| **deadband** | double | N/A | I | percentage deadband |
| **damping** | double | N/A | I | load setpoint to track to |
| **output** | double | N/A | IO | output scaling value |
| **feedback** | double | N/A | O | the feedback signal, for reference purposes |
