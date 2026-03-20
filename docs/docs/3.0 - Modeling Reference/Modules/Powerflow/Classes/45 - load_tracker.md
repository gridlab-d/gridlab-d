## Load Tracker

!!! warning
    This page was automatically generated and requires review.

### Inheritance

| Parent Class |
| --- |

### Load Tracker Parameters

#### Properties

| Property Name | Type | Unit | Input | Updates | Description | Evidence (delete after review) |
| --- | --- | --- | --- | --- | --- | --- |
| target | object | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">target object to track the load of</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: init</div> |
| target_property | char256 | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">property on the target object representing the load</div> | <div style="white-space: normal; overflow-wrap: anywhere;">default access=PA_PUBLIC; no writes detected in source scan</div> |
| operation | enumeration | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">operation to perform on complex property types Valid values: REAL, IMAGINARY, MAGNITUDE, ANGLE.</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| full_scale | double | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">magnitude of the load at full load, used for feed-forward control</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: init</div> |
| setpoint | double | N/A |  | ✓ | <div style="white-space: normal; overflow-wrap: anywhere;">load setpoint to track to</div> | <div style="white-space: normal; overflow-wrap: anywhere;">runtime writes: presync</div> |
| deadband | double | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">percentage deadband</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| damping | double | N/A | ✓ |  | <div style="white-space: normal; overflow-wrap: anywhere;">load setpoint to track to</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: create</div> |
| output | double | N/A | ✓ | ✓ | <div style="white-space: normal; overflow-wrap: anywhere;">output scaling value</div> | <div style="white-space: normal; overflow-wrap: anywhere;">init writes: init; runtime writes: presync</div> |
| feedback | double | N/A |  | ✓ | <div style="white-space: normal; overflow-wrap: anywhere;">the feedback signal, for reference purposes</div> | <div style="white-space: normal; overflow-wrap: anywhere;">runtime writes: presync, update_feedback_variable</div> |
