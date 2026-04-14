## VFD

!!! warning
    This page was automatically generated and requires review.

### VFD Parameters

#### Properties

**vfd** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| rated_motor_speed | double | 1/min | ✓ |  | ⚠️ Rated speed of the VFD in RPM. Default = 1800 RPM |
| desired_motor_speed | double | 1/min | ✓ | ✓ | ⚠️ Desired speed of the VFD In ROM. Default = 1800 RPM (max) |
| motor_poles | double | N/A | ✓ |  | ⚠️ Number of Motor Poles. Default = 4 |
| rated_output_voltage | double | V | ✓ |  | ⚠️ Line to Line Voltage - VFD Rated voltage. Default to TO node nominal_voltage |
| rated_horse_power | double | hp | ✓ |  | ⚠️ Rated Horse Power of the VFD. Default = 75 HP |
| nominal_output_frequency | double | Hz | ✓ |  | ⚠️ Nominal VFD output frequency. Default = 60 Hz |
| desired_output_frequency | double | Hz | ✓ | ✓ | ⚠️ VFD desired output frequency based on the desired RPM |
| current_output_frequency | double | Hz | ✓ | ✓ | ⚠️ VFD currently output frequency |
| efficiency | double | % | ✓ | ✓ | ⚠️ Current VFD efficiency based on the load/VFD output Horsepower |
| stable_time | double | s | ✓ |  | ⚠️ Time taken by the VFD to reach desired frequency (based on RPM). Default = 1.45 seconds |
| vfd_state | enumeration | N/A | ✓ | ✓ | ⚠️ Current state of the VFD Valid values: `OFF`, `CHANGING`, `STEADY_STATE`. |
