# VFD

!!! warning

    This class is unmaintained and will likely be removed in a future release.


The vfd object models a variable frequency drive (VFD) motor controller.

### Sample

    object meter {
        phases "ABC";
        name node4;
        nominal_voltage 4200;
    }

    object vfd {
        from node4;
        to node5;
        object player {
            property desired_motor_speed;
            file ../data_vfd_speedPlayer_delta.player;
            flags DELTAMODE;
        };
        phases "ABC";
    }

    object load {
        name node5;
        bustype SWING;
        phases "ABC";
        base_power_A 1000;
        base_power_B 1000;
        base_power_C 1000;
        current_pf_A 0.9;
        current_pf_B 0.9;
        current_pf_C 0.9;
        current_fraction_A 1;
        current_fraction_B 1;
        current_fraction_C 1;
        nominal_voltage 4200;
    }

### VFD Parameters

#### Properties

**vfd** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: vfd table 1 { #tbl:49-vfd-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **rated_motor_speed** | double | 1/min | I | Rated speed of the VFD in RPM. Default = 1800 RPM |
| **desired_motor_speed** | double | 1/min | I | Desired speed of the VFD In ROM. Default = 1800 RPM (max) |
| **motor_poles** | double | N/A | I | Number of Motor Poles. Default = 4 |
| **rated_output_voltage** | double | V | I | Line to Line Voltage - VFD Rated voltage. Default to TO node nominal_voltage |
| **rated_horse_power** | double | hp | I | Rated Horse Power of the VFD. Default = 75 HP |
| **nominal_output_frequency** | double | Hz | I | Nominal VFD output frequency. Default = 60 Hz |
| **desired_output_frequency** | double | Hz | IO | VFD desired output frequency based on the desired RPM |
| **current_output_frequency** | double | Hz | IO | VFD currently output frequency |
| **efficiency** | double | % | IO | Current VFD efficiency based on the load/VFD output Horsepower |
| **stable_time** | double | s | I | Time taken by the VFD to reach desired frequency (based on RPM). Default = 1.45 seconds |
| **vfd_state** | enumeration | N/A | IO | Current state of the VFD Valid values: `OFF`, `CHANGING`, `STEADY_STATE`. |
