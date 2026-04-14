## Load Tracker

!!! warning
    This page was automatically generated and requires review.

### Load Tracker Parameters

#### Properties

**load_tracker** does not declare inherited parent classes.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| target | object | N/A | ✓ |  | ⚠️ target object to track the load of |
| target_property | char256 | N/A | ✓ |  | ⚠️ property on the target object representing the load |
| operation | enumeration | N/A | ✓ |  | ⚠️ operation to perform on complex property types Valid values: `REAL`, `IMAGINARY`, `MAGNITUDE`, `ANGLE`. |
| full_scale | double | N/A | ✓ |  | ⚠️ magnitude of the load at full load, used for feed-forward control |
| setpoint | double | N/A |  | ✓ | ⚠️ load setpoint to track to |
| deadband | double | N/A | ✓ |  | ⚠️ percentage deadband |
| damping | double | N/A | ✓ |  | ⚠️ load setpoint to track to |
| output | double | N/A | ✓ | ✓ | ⚠️ output scaling value |
| feedback | double | N/A |  | ✓ | ⚠️ the feedback signal, for reference purposes |
