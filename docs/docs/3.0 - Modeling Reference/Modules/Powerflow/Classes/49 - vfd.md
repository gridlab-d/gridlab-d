## VFD

!!! warning
    This page was automatically generated and requires review.

### Inheritance

| Parent Class |
| --- |
| link |

### VFD Parameters

#### Properties

| Property Name | Type | Unit | Input | Updates | Description | Evidence (delete after review) |
| --- | --- | --- | --- | --- | --- | --- |
| rated_motor_speed | double | 1/min | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Rated speed of the VFD in RPM. Default = 1800 RPM</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| desired_motor_speed | double | 1/min | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Desired speed of the VFD In ROM. Default = 1800 RPM (max)</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create; other writes: CheckParameters</div> |
| motor_poles | double | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Number of Motor Poles. Default = 4</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| rated_output_voltage | double | V | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Line to Line Voltage - VFD Rated voltage. Default to TO node nominal_voltage</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create, init</div> |
| rated_horse_power | double | hp | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Rated Horse Power of the VFD. Default = 75 HP</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| nominal_output_frequency | double | Hz | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Nominal VFD output frequency. Default = 60 Hz</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create, init</div> |
| desired_output_frequency | double | Hz | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">VFD desired output frequency based on the desired RPM</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create; other writes: CheckParameters</div> |
| current_output_frequency | double | Hz | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">VFD currently output frequency</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create, init; other writes: VFD_current_injection</div> |
| efficiency | double | % | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Current VFD efficiency based on the load/VFD output Horsepower</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create; other writes: VFD_current_injection</div> |
| stable_time | double | s | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">Time taken by the VFD to reach desired frequency (based on RPM). Default = 1.45 seconds</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| vfd_state | enumeration | N/A | ✓ | ✓ | <div style="white-space: normal; overflow-wrap: anywhere;">Current state of the VFD Valid values: OFF, CHANGING, STEADY_STATE.</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create, init; runtime writes: inter_deltaupdate_vfd, postsync; other writes: CheckParameters, VFD_current_injection</div> |
